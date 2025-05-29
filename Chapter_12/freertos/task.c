#include "FreeRTOS.h"
#include "task.h"

TCB_t* volatile pxCurrentTCB = NULL;

List_t pxReadyTaskLists[ configMAX_PRIORITIES ];

static volatile UBaseType_t uxCurrentNumberOfTasks = ( UBaseType_t ) 0U;
static TaskHandle_t xIdleTaskHandle = NULL;
static volatile TickType_t xTickCount = ( TickType_t ) 0U;

static volatile UBaseType_t uxTopReadyPriority = taskIDLE_PRIORITY;

static List_t xDelayedTaskList1;
static List_t xDelayedTaskList2;

static List_t * volatile pxDeleyedTaskList;
static List_t * volatile pxOverflowDeleyedTaskList;

static volatile TickType_t xNextTaskUnBlockTime = ( TickType_t ) 0U;
static volatile BaseType_t xNumberOfOverflows = ( BaseType_t ) 0;

static void prvInitialiseNewTask( TASKFunction_t pxTaskCode,
                                  const char* const pcName,
                                  const uint32_t ulStackDepth,
                                  void* const pvParameters,
                                  UBaseType_t uxPriority,    
                                  TaskHandle_t * const pxCreatedTask,
                                  TCB_t *pxNewTCB);

static void prvResetNextTaskUnblockTime( void );                                  
                                  
#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )

    #define taskRECORD_READY_PRIORITY( uxPriority ) \
    {                                               \
        if( ( uxPriority ) > uxTopReadyPriority ){  \
            uxTopReadyPriority = ( uxPriority );    \
        }                                           \
    }/*taskRECORD_READY_PRIORITY*/

    #define taskSELECT_HIGHEST_PRIORITY_TASK()                                             \
    {                                                                                      \
        UBaseType_t uxTopPriority = uxTopReadyPriority;                                    \
        while ( listList_IS_EMPTY( &( pxReadyTaskLists[ uxTopPriority ] ) ) ){              \
            --uxTopPriority;                                                               \
        }                                                                                  \
        listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB, &( pxReadyTaskLists[ uxTopPriority ] ) ); \
        uxTopReadyPriority = uxTopPriority;                                                \
    }/*taskSELECT_HIGHEST_PRIORITY_TASK*/

#define taskRESET_READY_PRIORITY( uxPriority ) 
#define portRESET_READY_PRIORITY( uxPriority, uxTopReadyPriority )
   
#else /*configUSE_PORT_OPTIMISED_TASK_SELECTION*/ 
    
    #define taskRECORD_READY_PRIORITY( uxPriority ) \
            portRECORD_READY_PRIORITY( uxPriority, uxTopReadyPriority )
    
    #define taskSELECT_HIGHEST_PRIORITY_TASK()                                              \
    {                                                                                       \
        UBaseType_t uxTopPriority;                                                          \
        portGET_HIGHEST_PRIORITY( uxTopPriority, uxTopReadyPriority);                       \
        listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB, &( pxReadyTaskLists[ uxTopPriority ] ) ); \
    }/*taskSELECT_HIGHEST_PRIORITY_TASK*/
    
    #if 1
        
        #define taskRESET_READY_PRIORITY( uxPriority )                                                     \
        {                                                                                                  \
            if( listCURRENT_LIST_LENGTH( &( pxReadyTaskList[ ( uxPriority ) ] ) ) == ( UBaseType_t ) 0 ) { \
               portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority ) );                         \
            }                                                                                              \
        }
    
    #else
    
        #define taskRESET_READY_PRIORITY( uxPriority )                          \
        {                                                                       \
            portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority ) ); \
        }           

    #endif
    
#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

#define taskSWITCH_DELAYED_LISTS()                     \
    {                                                  \
        List_t* pxTemp;                                \
        pxTemp = pxDeleyedTaskList;                    \
        pxDeleyedTaskList = pxOverflowDeleyedTaskList; \
        pxOverflowDeleyedTaskList = pxTemp;            \
        xNumberOfOverflows ++;                         \
        prvResetNextTaskUnblockTime();                 \
    }                                                  
    
#define prvAddTaskToReadyList( pxTCB )                                                                   \
        taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );                                              \
        vListInsertEnd( &( pxReadyTaskLists[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xStateListItem ) );            
        
static void prvAddNewTaskToReadyList( TCB_t *pxNewTCB )
{
    taskENTER_CRITICAL();
    {
        uxCurrentNumberOfTasks ++;
        
        if( pxCurrentTCB == NULL ){
            pxCurrentTCB = pxNewTCB;
            
            if( uxCurrentNumberOfTasks == ( UBaseType_t ) 1 ){
                prvInitialiseTaskLists();            
            }
        } else {
            
            if( pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority ){
                pxCurrentTCB = pxNewTCB;
            }                 
        }
            
        prvAddTaskToReadyList( pxNewTCB );
    }
    taskEXIT_CRITICAL();
}    
        
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )

TaskHandle_t xTaskCreateStatic( TASKFunction_t pxTaskCode,
                                const char* const pcName,
                                const uint32_t ulStackDepth,
                                void * const pvParameters,
                                UBaseType_t uxPriority,
                                StackType_t * const puxStackBuffer,
                                TCB_t * const pxTaskBuffer)
{
    TCB_t *pxNewTCB;
    TaskHandle_t xReturn;
    
    if( ( puxStackBuffer != NULL ) && ( pxTaskBuffer != NULL ) ) {
        
        pxNewTCB = pxTaskBuffer;
        pxNewTCB->pxStack = puxStackBuffer;
        
        prvInitialiseNewTask( pxTaskCode,
                              pcName,
                              ulStackDepth,
                              pvParameters,
                              uxPriority,
                              &xReturn,
                              pxNewTCB); 
        
        prvAddNewTaskToReadyList( pxNewTCB );
            
    } else {
    
        xReturn = NULL;        
    }
    
    return xReturn;
}

#endif

static void prvInitialiseNewTask( TASKFunction_t pxTaskCode,
                                  const char* const pcName,
                                  const uint32_t ulStackDepth,
                                  void* const pvParameters,
                                  UBaseType_t uxPriority,    
                                  TaskHandle_t * const pxCreatedTask,
                                  TCB_t *pxNewTCB)
{
    StackType_t * pxTopOfStack;    
    UBaseType_t x;

     
    pxTopOfStack = pxNewTCB->pxStack + ( ulStackDepth - ( uint32_t ) 1 );
    //Align with 8 bytes    
    pxTopOfStack = ( StackType_t*) ( ( ( uint32_t ) pxTopOfStack ) & ( ~( ( uint32_t ) 0x0007 ) ) ); 
    
    for ( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x ++ ) {        
   
        pxNewTCB->pcTaskName[ x ] = pcName[ x ];       
        
        if( pcName[ x ] == 0 )
            break;
    }
    
    pxNewTCB->pcTaskName[ configMAX_TASK_NAME_LEN - 1 ] = 0x00;
    
    vListInitialiseItem( &(pxNewTCB->xStateListItem) );
    
    listSET_LIST_ITEM_OWNER( &(pxNewTCB->xStateListItem), pxNewTCB);
           
    if( uxPriority >= ( UBaseType_t ) configMAX_PRIORITIES ){
        uxPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
    }
    pxNewTCB->uxPriority = uxPriority;
    
    pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters );

    if ( ( void * ) pxCreatedTask != NULL ) {
        
        *pxCreatedTask = ( TaskHandle_t ) pxNewTCB;
    }     
}


void prvInitialiseTaskLists( void )
{
    UBaseType_t uxPriority;
    
    for( uxPriority = ( UBaseType_t ) 0U; uxPriority < ( UBaseType_t ) configMAX_PRIORITIES; uxPriority ++ ) {
    
        vListInitialise( &pxReadyTaskLists[ uxPriority ] );    
    }
    
    vListInitialise( &xDelayedTaskList1 );
    vListInitialise( &xDelayedTaskList2 );
    
    pxDeleyedTaskList = &xDelayedTaskList1;
    pxOverflowDeleyedTaskList = &xDelayedTaskList2;
}

extern TCB_t Task1TCB;
extern TCB_t Task2TCB;
extern TCB_t IdleTaskTCB;

void vApplicationGetIdleTaskMemory( TCB_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize );

static portTASK_FUNCTION( prvIdleTask, pvParameters )
{
    ( void ) pvParameters;
    
    for(;;){
    
    }
}

void vTaskStarScheduler( void )
{ 
    TCB_t *pxIdleTaskTCBBuffer = NULL;
    StackType_t* pxIdleTaskStackBuffer = NULL;
    uint32_t ulIdleTaskStackSize;
    
    vApplicationGetIdleTaskMemory( &pxIdleTaskTCBBuffer,
                                   &pxIdleTaskStackBuffer,
                                   &ulIdleTaskStackSize);
    
    xIdleTaskHandle = xTaskCreateStatic ( ( TASKFunction_t ) prvIdleTask,
                                          ( char * ) "IDLE",
                                          ( uint32_t ) ulIdleTaskStackSize,
                                          ( void * ) NULL,
                                          ( UBaseType_t ) taskIDLE_PRIORITY,    
                                          ( StackType_t * ) pxIdleTaskStackBuffer, 
                                          ( TCB_t * ) pxIdleTaskTCBBuffer );                                             
    
    //vListInsertEnd( &( pxReadyTaskLists[0] ), 
    //                &( ( ( TCB_t * ) pxIdleTaskTCBBuffer )->xStateListItem ) );
                                              
    //pxCurrentTCB = &Task1TCB;
    
    xNextTaskUnBlockTime = portMAX_DELAY;
    xTickCount = ( TickType_t ) 0U;                                          
    
    if( xPortStartScheduler() != pdFALSE ) {
        //Should never reach here if successful.    
    }
}

#if 1

void vTaskSwitchContext( void )
{
    taskSELECT_HIGHEST_PRIORITY_TASK();
}

#else
void vTaskSwitchContext( void )
{
#if 0

    if( pxCurrentTCB == &Task1TCB) {
        
        pxCurrentTCB = &Task2TCB;
    } else {
        pxCurrentTCB = &Task1TCB;
    }
#else
    
    if( pxCurrentTCB == &IdleTaskTCB ){
        
        if( Task1TCB.xTicksToDelay == 0 ){
            pxCurrentTCB = &Task1TCB;        
        } else if ( Task2TCB.xTicksToDelay == 0 ) {
            pxCurrentTCB = &Task2TCB;
        } else {
            return;   
        }
        
    } else {
        
        if( pxCurrentTCB == &Task1TCB ) {
            
            if( Task2TCB.xTicksToDelay == 0) {
                pxCurrentTCB = &Task2TCB;                
            } else if ( pxCurrentTCB->xTicksToDelay != 0 ){
                pxCurrentTCB = &IdleTaskTCB;
            } else {
                return;
            }
            
        } else {
            
            if( Task1TCB.xTicksToDelay == 0) {
                pxCurrentTCB = &Task1TCB;                
            } else if ( pxCurrentTCB->xTicksToDelay != 0 ){
                pxCurrentTCB = &IdleTaskTCB;
            } else {
                return;
            }                  
            
        }            
    }

#endif    
}
#endif

static void prvResetNextTaskUnblockTime( void )
{
    TCB_t *pxTCB;
    
    if( listLIST_IS_EMPTY( pxDeleyedTaskList ) != pdFALSE ){
        xNextTaskUnBlockTime = portMAX_DELAY;
    } else {
        ( pxTCB ) = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDeleyedTaskList );
        xNextTaskUnBlockTime = listGET_LIST_ITEM_VALUE( &( pxTCB->xStateListItem ) );
    }
}

static void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait )
{
    TickType_t xTimeToWake;
    
    const TickType_t xConstTickCount = xTickCount;
    
    if( uxListRemove( &( pxCurrentTCB->xStateListItem ) ) == ( UBaseType_t ) 0 ){
        portRESET_READY_PRIORITY( pxCurrentTCB->uxPriority, uxTopReadyPriority );    
    }
    
    xTimeToWake = xTickCount + xTicksToWait;
    
    listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ), xTimeToWake );
    
    //Overflow 
    if( xTimeToWake < xTickCount){
        vListInsert( pxOverflowDeleyedTaskList, &( pxCurrentTCB->xStateListItem ) );
    } else {
        vListInsert( pxDeleyedTaskList, &( pxCurrentTCB->xStateListItem ) ); 
    
        if( xTimeToWake < xNextTaskUnBlockTime ){
            xNextTaskUnBlockTime = xTimeToWake;
        }         
    }     
}

void vTaskDelay( const TickType_t xTicksToDelay )
{
    TCB_t *pxTCB = NULL;
    
    pxTCB = pxCurrentTCB;
    
    //pxTCB->xTicksToDelay = xTicksToDelay;
    prvAddCurrentTaskToDelayedList( xTicksToDelay );
    
    //taskRESET_READY_PRIORITY( pxTCB->uxPriority );
    
    taskYIELD();
}

BaseType_t xTaskIncrementTick( void ) 
{
    TCB_t *pxTCB = NULL;
    TickType_t xItemValue;
    BaseType_t xSwitchRequired = pdFALSE;
    
    const TickType_t xConstTickCount = xTickCount + 1;
    xTickCount = xConstTickCount;
    
    //Overflow
    if( xConstTickCount == ( TickType_t ) 0U ) {
        taskSWITCH_DELAYED_LISTS();
    }
    
    if( xConstTickCount >= xNextTaskUnBlockTime ){
        
        for(;;){
            
            //Delayed task list is empty.
            if( listLIST_IS_EMPTY( pxDeleyedTaskList ) != pdFALSE ){
                xNextTaskUnBlockTime = portMAX_DELAY;
                break;
            } else {
                //Delayed task list is not empty.
                pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDeleyedTaskList );
                xItemValue = listGET_LIST_ITEM_VALUE( &( pxTCB->xStateListItem ) );

                if( xConstTickCount < xItemValue ) {
                    xNextTaskUnBlockTime = xItemValue;
                    break;                    
                }        

                ( void ) uxListRemove( &( pxTCB->xStateListItem ) );

                prvAddTaskToReadyList( pxTCB );
                
                #if ( configUSE_PREEMPTION == 1 )
                {
                    if( pxTCB->uxPriority >= pxCurrentTCB->uxPriority ){
                        xSwitchRequired = pdTRUE;      
                    }                
                }
                #endif
            }         
        }
    }
          
    #if ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICE == 1 ) )
    {
        if( listCURRENT_LIST_LENGTH( &( pxReadyTaskLists[ pxCurrentTCB->uxPriority ] ) ) >
            ( UBaseType_t ) 1 ){
            xSwitchRequired = pdTRUE;
        }    
    }
    #endif
    
    return xSwitchRequired;
}

