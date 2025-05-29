#ifndef _PORT_MACRO_H
#define _PORT_MACRO_H

#include "stdint.h"
#include "stddef.h"
#include "cmsis_armclang.h"
#include "FreeRTOSConfig.h"

#define portCHAR    char
#define portFLOAT    float
#define portDOUBLE    double
#define portLONG    long
#define portSHORT    short
#define portSTACK_TYPE    uint32_t
#define portBASE_TYPE    long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

#if( configUSE_16_BIT_TICKS == 1)
 typedef uint16_t TickType_t;
 #define portMAX_DELAY (TickType_t) 0xffff
#else
 typedef uint32_t TickType_t;
 #define portMAX_DELAY (TickType_t) 0xffffffffUL
#endif

#define portNVIC_INT_CTRL_REG     ( * ( ( volatile uint32_t * ) 0xe000ed04 ) )
#define portNVIC_PENDSVSET_BIT    ( 1UL << 28UL )

#define portSY_FULL_READ_WRITE    ( 15 )

#define portYIELD()                                    \
{                                                      \
    portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;    \
    __dsb( portSY_FULL_READ_WRITE );                   \
    __isb( portSY_FULL_READ_WRITE );                   \
}

extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

#define portDISABLE_INTERRUPTS()       vPortRaiseBASEPRI()
#define portENABLE_INTERRUPTS()        vPortSetBASEPRI( 0 )        
        
#define portENTER_CRITICAL()    vPortEnterCritical()
#define portEXIT_CRITICAL()    vPortExitCritical()

#define portSET_INTERRUPTS_MASK_FROM_ISR()          ulPortRaiseBASEPRI()
#define portCLEAR_INTERRUPTS_MASK_FROM_ISR()        vPortSetBASEPRI( x )                  

#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters ) 

#define portINLINE __inline

#ifndef portFORCE_INLINE
    #define portFORCE_INLINE    inline __attribute__((always_inline))
#endif

#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
    #define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#endif

#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 1)

    #if( configMAX_PRIORITIES > 32 )
        #error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when \
               configMAX_PRIORITIES is less than or equal to 32. \
               It is very rare that a system requires more than 10 to 15 different \
               priorities as tasks that share a priority will time slice.
    #endif

    #define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) ) )
    #define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) ) )
    
    #define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) ( uxTopPriority = ( 31UL - ( uint32_t ) __clz( ( uxReadyPriorities ) ) ) )

#endif /*configUSE_PORT_OPTIMISED_TASK_SELECTION*/    
    
static portFORCE_INLINE void vPortSetBASEPRI( uint32_t ulBASEORI )
{
    __ASM volatile ( 
        "msr basepri, %0"
        :
        : "r"(ulBASEORI)
    );
}

static portFORCE_INLINE void vPortRaiseBASEPRI( void )
{
    __ASM volatile (
        "mov r0, %0 \n"
        "msr basepri, r0 \n"
        "dsb \n"
        "isb \n"
        :
        :"I"(configMAX_SYSCALL_INTERRUPT_PRIORITY)
        :"memory"  
    );
}

static portFORCE_INLINE void vPortClearBASEPRIFromISR( void )
{
    __ASM volatile (
        "mov r0, #0 \n"
        "msr basepri, r0 \n"    
    );
}

static portFORCE_INLINE uint32_t ulPortRaiseBASEPRI()
{
    uint32_t ulReturn, ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;
    
    __ASM volatile (
        "mrs %0, basepri \n"
        "msr basepri %1 \n"
        "dsb \n"
        "isb \n"
        : "=r"(ulReturn)
        : "r"(ulNewBASEPRI)
        : "memory"    
    );
    
    return ulReturn;
}

#endif