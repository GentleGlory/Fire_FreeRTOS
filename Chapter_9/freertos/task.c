#include "FreeRTOS.h"
#include "task.h"

TCB_t* volatile pxCurrentTCB = NULL;

List_t pxReadyTaskLists[ configMAX_PRIORITIES ];

static volatile UBaseType_t uxCurrentNumberOfTasks = ( UBaseType_t ) 0U;
static TaskHandle_t xIdleTaskHandle = NULL;
static volatile TickType_t xTickCount = ( TickType_t ) 0U;

static void prvInitialiseNewTask( TASKFunction_t pxTaskCode,
                                  const char* const pcName,
                                  const uint32_t ulStackDepth,
                                  void* const pvParameters,
                                  TaskHandle_t * const pxCreatedTask,
                                  TCB_t *pxNewTCB);



#if ( configSUPPORT_STATIC_ALLOCATION == 1 )

TaskHandle_t xTaskCreateStatic( TASKFunction_t pxTaskCode,
                                const char* const pcName,
                                const uint32_t ulStackDepth,
                                void * const pvParameters,
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
                              &xReturn,
                              pxNewTCB);    
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
                                          ( StackType_t * ) pxIdleTaskStackBuffer, 
                                          ( TCB_t * ) pxIdleTaskTCBBuffer );                                             
    
    vListInsertEnd( &( pxReadyTaskLists[0] ), 
                    &( ( ( TCB_t * ) pxIdleTaskTCBBuffer )->xStateListItem ) );
                                              
    pxCurrentTCB = &Task1TCB;
    
    xTickCount = ( TickType_t ) 0U;                                          
    
    if( xPortStartScheduler() != pdFALSE ) {
        //Should never reach here if successful.    
    }
}

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

void vTaskDelay( const TickType_t xTicksToDelay )
{
    TCB_t *pxTCB = NULL;
    
    pxTCB = pxCurrentTCB;
    
    pxTCB->xTicksToDelay = xTicksToDelay;
    
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
                }                             
            }while(pxNextTCB != pxFirstTCB);                      
        }        
    }
    
    taskYIELD();
}

