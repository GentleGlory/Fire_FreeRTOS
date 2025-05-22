#include "FreeRTOS.h"
#include "task.h"

TCB_t* volatile pxCurrentTCB = NULL;

List_t pxReadyTaskLists[ configMAX_PRIORITIES ];

static volatile UBaseType_t uxCurrentNumberOfTasks = ( UBaseType_t ) 0U;

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

void vTaskStarScheduler( void )
{ 
    pxCurrentTCB = &Task1TCB;
    
    if( xPortStartScheduler() != pdFALSE ) {
        //Should never reach here if successful.    
    }
}

void vTaskSwitchContext( void )
{
    if( pxCurrentTCB == &Task1TCB) {
        
        pxCurrentTCB = &Task2TCB;
    } else {
        pxCurrentTCB = &Task1TCB;
    }
}