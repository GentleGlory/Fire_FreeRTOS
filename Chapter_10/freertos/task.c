#include "FreeRTOS.h"
#include "task.h"

TCB_t* volatile pxCurrentTCB = NULL;

List_t pxReadyTaskLists[ configMAX_PRIORITIES ];

static volatile UBaseType_t uxCurrentNumberOfTasks = ( UBaseType_t ) 0U;
static TaskHandle_t xIdleTaskHandle = NULL;
static volatile TickType_t xTickCount = ( TickType_t ) 0U;

static volatile UBaseType_t uxTopReadyPriority = taskIDLE_PRIORITY;

static void prvInitialiseNewTask( TASKFunction_t pxTaskCode,
                                  const char* const pcName,
                                  const uint32_t ulStackDepth,
                                  void* const pvParameters,
                                  UBaseType_t uxPriority,    
                                  TaskHandle_t * const pxCreatedTask,
                                  TCB_t *pxNewTCB);

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
    
    #if 0
        
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

void vTaskDelay( const TickType_t xTicksToDelay )
{
    TCB_t *pxTCB = NULL;
    
    pxTCB = pxCurrentTCB;
    
    pxTCB->xTicksToDelay = xTicksToDelay;
    
    taskRESET_READY_PRIORITY( pxTCB->uxPriority );
    
    taskYIELD();
}

void xTaskIncrementTick( void ) 
{
    TCB_t *pxFirstTCB = NULL, *pxNextTCB = NULL;
    BaseType_t i = 0;
    
    const TickType_t xConstTickCount = xTickCount + 1;
    xTickCount = xConstTickCount;
    
    for( i = 0; i < configMAX_PRIORITIES; i++ ) {
        
        if( listCURRENT_LIST_LENGTH( &pxReadyTaskLists[i] ) > ( UBaseType_t ) 0 ) {
            
            listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, &pxReadyTaskLists[i] );

            do {
                
                listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, &pxReadyTaskLists[i] );
                
                if( pxNextTCB->xTicksToDelay > 0 ) {
                    
                    pxNextTCB->xTicksToDelay --; 

                    if( pxNextTCB->xTicksToDelay == 0 ){
                        taskRECORD_READY_PRIORITY( pxNextTCB->uxPriority );   
                    }                    
                }                             
            }while(pxNextTCB != pxFirstTCB);                      
        }        
    }
    
    taskYIELD();
}

