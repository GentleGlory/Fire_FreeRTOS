#ifndef _FREE_RTOS_CONFIG_H
#define _FREE_RTOS_CONFIG_H

#define configUSE_16_BIT_TICKS             0
#define configMAX_PRIORITIES               (5) 
#define configSUPPORT_STATIC_ALLOCATION    1
#define configMAX_TASK_NAME_LEN            (16) 

#define configKERNEL_INTERRUPT_PRIORITY         255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    191    

#define xPortPendSVHandler     PendSV_Handler
#define xPortSysTickHandler    SysTick_Handler
#define vPortSVCHandler        SVC_Handler

#endif




