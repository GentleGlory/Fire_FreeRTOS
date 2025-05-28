#include "FreeRTOS.h"
#include "task.h"

portCHAR flag1;
portCHAR flag2;

extern List_t pxReadyTaskLists[ configMAX_PRIORITIES ];

TaskHandle_t Task1_Handle;
#define TASK1_STACK_SIZE    32
StackType_t Task1Stack[TASK1_STACK_SIZE];
TCB_t Task1TCB;

TaskHandle_t Task2_Handle;
#define TASK2_STACK_SIZE    32
StackType_t Task2Stack[TASK2_STACK_SIZE];
TCB_t Task2TCB;

StackType_t IdleTaskStack[ configMINIMAL_STACK_SIZE ];
TCB_t IdleTaskTCB;

void vApplicationGetIdleTaskMemory( TCB_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize )
{
    *ppxIdleTaskTCBBuffer = &IdleTaskTCB;
    *ppxIdleTaskStackBuffer = IdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void delay(uint32_t count);
void Task1_Entry(void *p_arg);
void Task2_Entry(void *p_arg);


void delay(uint32_t count)
{
    for(;count != 0; count--);
}

void Task1_Entry(void *p_arg)
{
    for( ;; ){
#if 0        
        flag1 = 1;
        delay( 100 );
        flag1 = 0; 
        delay( 100 );
        taskYIELD();
#else
        flag1 = 1;
        vTaskDelay( 2 );
        flag1 = 0;
        vTaskDelay( 2 );
#endif        
        
    }
}

void Task2_Entry(void *p_arg)
{
    for( ;; ){
#if 0        
        flag2 = 1;
        delay( 100 );
        flag2 = 0; 
        delay( 100 );
        
        taskYIELD();
#else
        flag2 = 1;
        vTaskDelay( 2 );
        flag2 = 0;
        vTaskDelay( 2 );
#endif        
    }
}

int main(void)
{
    prvInitialiseTaskLists();
    
    Task1_Handle = xTaskCreateStatic( (TASKFunction_t)Task1_Entry,
                                      (char*)"Taks1",
                                      (uint32_t)TASK1_STACK_SIZE,
                                      (void*)NULL,
                                      (StackType_t *)Task1Stack,
                                      (TCB_t*)&Task1TCB);                                      
    
    vListInsertEnd( &( pxReadyTaskLists[2] ), &( Task1TCB.xStateListItem));                                       
                                          
                                      
    Task2_Handle = xTaskCreateStatic( (TASKFunction_t)Task2_Entry,
                                      (char*)"Taks2",
                                      (uint32_t)TASK2_STACK_SIZE,
                                      (void*)NULL,
                                      (StackType_t *)Task2Stack,
                                      (TCB_t*)&Task2TCB);                                      
    
    vListInsertEnd( &( pxReadyTaskLists[2] ), &( Task2TCB.xStateListItem));                                       
                                      
    vTaskStarScheduler();                                      
                                      
    for(;;){
		
		}	
    return 0;
}


