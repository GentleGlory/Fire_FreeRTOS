#ifndef _FREE_RTOS_H
#define _FREE_RTOS_H

#include "FreeRTOSConfig.h"
#include "portable.h"
#include "projdefs.h"
#include "list.h"

typedef struct tskTaskControlBlock
{
    volatile StackType_t    *pxTopOfStack;     
    
    ListItem_t              xStateListItem;           
    
    StackType_t             *pxStack;
    
    char                    pcTaskName[ configMAX_TASK_NAME_LEN ];

    TickType_t              xTicksToDelay;

    UBaseType_t             uxPriority;    
}tskTCB;

typedef tskTCB TCB_t; 

#endif