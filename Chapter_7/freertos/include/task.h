#ifndef _TASK_H
#define _TASK_H

#include "FreeRTOS.h"


#define taskYIELD() portYIELD()

typedef void * TaskHandle_t;

#if( configSUPPORT_STATIC_ALLOCATION == 1)

TaskHandle_t xTaskCreateStatic( TASKFunction_t pxTaskCode,
                                const char* const pcName,
                                const uint32_t ulStackDepth,
                                void * const pvParameters,
                                StackType_t * const puxStackBuffer,
                                TCB_t * const pxTaskBuffer);                                    
#endif

void prvInitialiseTaskLists( void );
void vTaskStarScheduler( void );
void vTaskSwitchContext( void );                                
                                
                                
#endif