#include "FreeRTOS.h"
#include "task.h"
#include "ARMCM4.h"

static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

#define portINITIAL_XPSR         ( 0x01000000 )
#define portSTART_ADDRESS_MASK   ( ( StackType_t ) 0xfffffffeUL )


#define portNVIC_SYSPRI2_REG    ( * ( ( volatile uint32_t * ) 0xe000ed20 ) )

#define portNVIC_PENDSV_PRI     ( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 16UL)              
#define portNVIC_SYSTICK_PRI    ( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 24UL)              

#define portNVIC_SYSTICK_CTRL_REG    ( * ( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG    ( * ( ( volatile uint32_t * ) 0xe000e014 ) )    

#ifndef configSYSTICK_CLOCK_HZ
    #define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
    #define portNVIC_SYSTICK_CLK_BIT    ( 1UL << 2UL )
#else
    #define portNVIC_SYSTICK_CLK_BIT    ( 0 )
#endif

#define portNVIC_SYSTICK_INT_BIT       ( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT    ( 1UL << 0UL )

void prvStartFirstTask( void );
void vPortSVCHandler( void );
void xPortPendSVHandler( void );
void vPortSetupTimerInterrupt( void );
    
static void prvTaskExitError( void )
{
    for( ;; );
}

StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack,
                                    TASKFunction_t pxCode,
                                    void *pvParameters )
{
    //xPSR
    pxTopOfStack --;
    *pxTopOfStack = portINITIAL_XPSR;
    
    //PC
    pxTopOfStack --;
    *pxTopOfStack = ( ( StackType_t ) pxCode ) & portSTART_ADDRESS_MASK;
    
    //LR
    pxTopOfStack --;
    *pxTopOfStack = ( StackType_t ) prvTaskExitError;
    
    //R12, R3, R2, R1 initialised to 0
    pxTopOfStack -= 5;
    *pxTopOfStack = ( StackType_t ) pvParameters;
    
    //R11, R10, R9, R8, R7, R6, R5 and R4 initialised to 0
    pxTopOfStack -= 8;
    
    return pxTopOfStack;
}                                        
        
BaseType_t xPortStartScheduler( void )
{
    portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;
    
    uxCriticalNesting = 0;
    
    vPortSetupTimerInterrupt();
    
    prvStartFirstTask();
    
    //Shouldn't reach here
    return 0;
}

void prvStartFirstTask( void )
{
    __ASM volatile (   
        "ldr r0, =0xE000ED08 \n"
        "ldr r0, [r0] \n"
        "ldr r0, [r0] \n"
        "msr msp, r0 \n"
        "cpsie i \n"
        "cpsie f \n"
        "dsb \n"
        "isb \n"
        "svc 0 \n"
        "nop \n"
        "nop \n"         
    );    
}

void vPortSVCHandler( void )
{    
    __ASM volatile(
        "ldr r3, =pxCurrentTCB \n"
        "ldr r1, [r3] \n"
        "ldr r0, [r1] \n"
        "ldmia r0!, {r4-r11} \n"
        "msr psp, r0 \n"
        "isb \n"
        "mov r0, #0 \n"
        "msr basepri, r0 \n"
        "orr r14, #0xd \n"
        "bx r14"
    );    
}

void xPortPendSVHandler( void )
{
    __ASM volatile (
        "mrs r0, psp \n"
        "isb \n"    
        "ldr r3, =pxCurrentTCB \n"
        "ldr r2, [r3] \n"
        "stmdb r0!, {r4-r11} \n"
        "str r0, [r2] \n"
        "stmdb sp!, {r3, r14} \n"
        "mov r0, %0 \n"
        "msr basepri, r0 \n"
        "dsb \n"
        "isb \n"
        "bl vTaskSwitchContext \n"
        "mov r0, #0 \n"
        "msr basepri, r0 \n"
        "ldmia sp!, {r3, r14} \n"    
        "ldr r1, [r3] \n"
        "ldr r0, [r1] \n"
        "ldmia r0!, {r4-r11} \n"
        "msr psp, r0 \n"
        "isb \n"
        "bx r14 \n"    
        "nop \n"
        :
        :"I"(configMAX_SYSCALL_INTERRUPT_PRIORITY)				
    );
}

void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
    uxCriticalNesting ++;
    
    if( uxCriticalNesting == 1) {
        
    }
}

void vPortExitCritical( void )
{
    uxCriticalNesting --;
    
    if( uxCriticalNesting == 0 ) {
        portENABLE_INTERRUPTS();
    }    
}
     
void xPortSysTickHandler( void )
{
    vPortRaiseBASEPRI();
    
    if( xTaskIncrementTick() != pdFALSE ){
        taskYIELD();
    }
    
    vPortClearBASEPRIFromISR();    
}

void vPortSetupTimerInterrupt( void )
{
    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
    
    portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT |
                                  portNVIC_SYSTICK_INT_BIT |
                                  portNVIC_SYSTICK_ENABLE_BIT );
}

