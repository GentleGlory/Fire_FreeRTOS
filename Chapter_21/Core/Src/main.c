/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "queue.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define QUEUE_LEN     4
#define QUEUE_SIZE    4

#define CREATE_LED_TASK

#define KEY_DOWN_EVENT    ( 0x01 << 0 )
#define KEY_UP_EVENT      ( 0x01 << 1 )

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE BEGIN PV */
#ifdef CREATE_LED_TASK
osThreadId_t led1TaskHandle;
const osThreadAttr_t led1Task_attributes = {
  .name = "led1Task",
  .stack_size = 128*4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t led2TaskHandle;
const osThreadAttr_t led2Task_attributes = {
  .name = "led2Task",
  .stack_size = 128*4,
  .priority = (osPriority_t) osPriorityNormal,
};
#else
osThreadId_t lowPriorityTaskHandle;
const osThreadAttr_t lowPriorityTask_attributes = {
  .name = "lowPriorityTask",
  .stack_size = 128*4,
  .priority = (osPriority_t) osPriorityLow,
};

osThreadId_t midPriorityTaskHandle;
const osThreadAttr_t midPriorityTask_attributes = {
  .name = "midPriorityTask",
  .stack_size = 128*4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t highPriorityTaskHandle;
const osThreadAttr_t highPriorityTask_attributes = {
  .name = "highPriorityTask",
  .stack_size = 128*4,
  .priority = (osPriority_t) osPriorityHigh,
};
#endif

osThreadId_t keyTaskHandle;
const osThreadAttr_t keyTask_attributes = {
  .name = "keyTask",
  .stack_size = 128*4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t receiveTaskHandle;
const osThreadAttr_t receiveTask_attributes = {
  .name = "receiveTask",
  .stack_size = 128*4,
  .priority = (osPriority_t) osPriorityNormal,
};

osMessageQueueId_t testQueueHandle;

osSemaphoreId_t binarySemHandle;

osMutexId_t mutexHandle;
const osMutexAttr_t mutex_attr = {
  "myThreadMutex",                          // human readable mutex name
  osMutexPrioInherit,                       // attr_bits
  NULL,                                     // memory for control block   
  0U                                        // size for control block
};

osEventFlagsId_t eventHandle;

osTimerId_t Swtmr1Handle;
osTimerId_t Swtmr2Handle;

static uint32_t TmrCb_Count1 = 0;
static uint32_t TmrCb_Count2 = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */
static void AppCreateTask( void *parameter );
static void LED1_Task( void *parameter );
static void LED2_Task( void *parameter );
static void KEY_Task( void *parameter );
static void RECEIVE_Task( void *parameter );
static void LowPriority_Task( void *parameter );
static void MidPriority_Task( void *parameter );
static void HighPriority_Task( void *parameter );
static void Swtmr1Task( void *parameter );
static void Swtmr2Task( void *parameter );


static void LED1_Task(void* parameter)
{
    while(1)
    {
        HAL_GPIO_WritePin(GPIOD, LD3_Pin, GPIO_PIN_SET);
        osDelay(500);        
        
        HAL_GPIO_WritePin(GPIOD, LD3_Pin, GPIO_PIN_RESET);        
        osDelay(500); 
    }
}

static void LED2_Task(void* parameter)
{
    uint32_t eventFlag;
    
    while(1)
    {
//        HAL_GPIO_WritePin(GPIOD, LD4_Pin, GPIO_PIN_SET);
//        osDelay(1000);        
//        
//        HAL_GPIO_WritePin(GPIOD, LD4_Pin, GPIO_PIN_RESET);        
//        osDelay(1000); 
        
        eventFlag = osEventFlagsWait( eventHandle, 
            KEY_DOWN_EVENT | KEY_UP_EVENT,
            osFlagsWaitAny,  osWaitForever);
        
        if( eventFlag & osFlagsError )
        {
            printf("eventHandle wait error:%u. \r\n", eventFlag);
        }
        else if ( eventFlag & KEY_DOWN_EVENT )
        {
            HAL_GPIO_WritePin(GPIOD, LD4_Pin, GPIO_PIN_SET);
        }
        else if ( eventFlag & KEY_UP_EVENT )
        {
            HAL_GPIO_WritePin(GPIOD, LD4_Pin, GPIO_PIN_RESET);
        }
    }
}

static void KEY_Task( void *parameter )
{
    uint32_t send_data1 = 1;
    uint32_t send_data2 = 2;    
    osStatus_t xReturn;
    BaseType_t pressed = pdFALSE;
    
    while(1)
    {
        if( HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_SET )
        {
            if( pressed == pdFALSE )
            {
                printf("B1_Pin key down.\r\n");
                
                pressed = pdTRUE;
                
                osEventFlagsSet( eventHandle, KEY_DOWN_EVENT);
                
                xReturn = osSemaphoreRelease( binarySemHandle );
                if( xReturn == osOK )
                    printf("binarySemHandle released. \r\n");
                else
                    printf("binarySemHandle release error:%d.\r\n",xReturn);            
#ifdef CREATE_LED_TASK            
                if( eTaskGetState( led1TaskHandle ) == eSuspended )
                {
                    printf("In KEY_Task, Resume led1TaskHandle \r\n");
                    osThreadResume( led1TaskHandle );
                
                    xReturn = osMessageQueuePut( testQueueHandle, &send_data1, NULL, 0);
                    if( xReturn == osOK )
                        printf("send_data1 send. \r\n");
                    else
                        printf("send_data1 send failed.\r\n");
                }
                else
                {
                    printf("In KEY_Task, Suspend led1TaskHandle \r\n");
                    osThreadSuspend( led1TaskHandle );
                
                    xReturn = osMessageQueuePut( testQueueHandle, &send_data2, NULL, 0);
                    if( xReturn == osOK )
                        printf("send_data2 send. \r\n");
                    else
                        printf("send_data2 send failed.\r\n");
                }
#endif                       
            }                        
        }
        else
        {
            if( pressed == pdTRUE )
            {
                printf("B1_Pin key up.\r\n");
                
                pressed = pdFALSE;
                
                osEventFlagsSet( eventHandle, KEY_UP_EVENT);                
            }
        }
    
        osDelay(20);
    }
}

static void RECEIVE_Task( void *parameter )
{
    uint32_t r_queue;
    osStatus_t xReturn;
    
    while(1)
    {
        xReturn = osSemaphoreAcquire( binarySemHandle, osWaitForever);
        if( xReturn == osOK )
            printf("binarySemHandle accquired. \r\n");         
        else
            printf("binarySemHandle accquired error: %d. \r\n", xReturn);         
        
        xReturn = osMessageQueueGet( testQueueHandle, &r_queue, NULL, osWaitForever);
        
        if( xReturn == osOK )
            printf("Message received: %u. \r\n", r_queue);         
        else
            printf("Message received error: %d. \r\n", xReturn);         
    }
}

static void LowPriority_Task( void *parameter )
{
    static uint32_t i;
    osStatus_t xReturn;
    
    while(1)
    {
        printf("LowPriority_Task try to acquire mutex. \r\n");
        xReturn = osMutexAcquire( mutexHandle,  osWaitForever);
        
        if( xReturn == osOK )
            printf("LowPriority_Task Runing \r\n");        
        else
            printf("Mutex acquire error: %d. \r\n", xReturn);      
        
        for( i = 0; i < 2000000; i++ )
        {
            osThreadYield();
        }
        
        printf(" LowPriority_Task release mutex. \r\n");
        
        HAL_GPIO_TogglePin(GPIOD, LD3_Pin);
        
        osMutexRelease( mutexHandle );
        
        osDelay(500);
    }
}

static void MidPriority_Task( void *parameter )
{
    while(1)
    {
        printf(" MidPriority_Task running. \r\n");
        osDelay(500);
    }
}

static void HighPriority_Task( void *parameter )
{
    osStatus_t xReturn;
    
    while(1)
    {
        printf("HighPriority_Task try to acquire mutex. \r\n");
        xReturn = osMutexAcquire( mutexHandle,  osWaitForever );
        
        if( xReturn == osOK )
            printf("HighPriority_Task Runing \r\n");        
        else
            printf("Mutex acquire error: %d. \r\n", xReturn); 
        
        HAL_GPIO_TogglePin(GPIOD, LD3_Pin);
        
        printf(" HighPriority_Task release mutex. \r\n");
        osMutexRelease( mutexHandle );
        
        osDelay(500);
    }
}

static void Swtmr1Task( void *parameter )
{
    TickType_t tick_num1;
    
    TmrCb_Count1 ++;
    
    tick_num1 = osKernelGetTickCount();
    
    HAL_GPIO_TogglePin(GPIOD, LD5_Pin);
    
    printf("The swtmr1_callback function was executed %u times.\r\n", TmrCb_Count1);
    
    printf("Tick count: %u.\r\n", tick_num1);
}

static void Swtmr2Task( void *parameter )
{
    TickType_t tick_num2;
    
    TmrCb_Count2 ++;
    
    tick_num2 = osKernelGetTickCount();
    
    HAL_GPIO_TogglePin(GPIOD, LD6_Pin);
    
    printf("The swtmr2_callback function was executed %u times.\r\n", TmrCb_Count2);
    
    printf("Tick count: %u.\r\n", tick_num2);
}


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  printf("STM32F4_FreeRTOS: System Init successfully. \r\n");
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  printf("STM32F4_FreeRTOS: Scheduler Init successfully. \r\n");
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin */
  GPIO_InitStruct.Pin = LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
int fputc(int ch, FILE* f)
{
    while( huart2.gState != HAL_UART_STATE_READY)
    {
        osDelay(1);
    }
    
    HAL_UART_Transmit( &huart2, ( uint8_t* ) &ch, 1, 0xffff);
    return ch;
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  //for(;;)
  //{
    
    osStatus_t xReturn;
    
    taskENTER_CRITICAL();
    
    testQueueHandle = osMessageQueueNew( QUEUE_SIZE, QUEUE_LEN, NULL);    
    if( testQueueHandle != NULL)
    {
        printf("Create testQueue successfully. \r\n");
    }
    
    binarySemHandle = osSemaphoreNew( 1, 0, NULL);
    if(binarySemHandle != NULL)
    {
        printf("Create binarySem successfully. \r\n");
    }
    
    mutexHandle = osMutexNew(&mutex_attr);
    if (mutexHandle != NULL)  {
        printf("Create mutex successfully. \r\n");
    }
#ifdef CREATE_LED_TASK     
    led1TaskHandle = osThreadNew(LED1_Task, NULL, &led1Task_attributes);    
    if( led1TaskHandle != NULL )
    {
        printf("Create LED1_Task successfully. \r\n");
    }
    
    led2TaskHandle = osThreadNew(LED2_Task, NULL, &led2Task_attributes);
    if( led2TaskHandle != NULL )
    {
        printf("Create LED2_Task successfully. \r\n");
    }
#else
    lowPriorityTaskHandle = osThreadNew(LowPriority_Task, NULL, &lowPriorityTask_attributes);    
    if( lowPriorityTaskHandle != NULL )
    {
        printf("Create LowPriority_Task successfully. \r\n");
    }
    
    midPriorityTaskHandle = osThreadNew(MidPriority_Task, NULL, &midPriorityTask_attributes);    
    if( midPriorityTaskHandle != NULL )
    {
        printf("Create MidPriority_Task successfully. \r\n");
    }    
    
    highPriorityTaskHandle = osThreadNew(HighPriority_Task, NULL, &highPriorityTask_attributes);    
    if( highPriorityTaskHandle != NULL )
    {
        printf("Create HighPriority_Task successfully. \r\n");
    }    
        
#endif    
    keyTaskHandle = osThreadNew(KEY_Task, NULL, &keyTask_attributes);
    if( keyTaskHandle != NULL )
    {
        printf("Create KEY_Task successfully. \r\n");
    }
       
    receiveTaskHandle = osThreadNew(RECEIVE_Task, NULL, &receiveTask_attributes);    
    if( receiveTaskHandle != NULL )
    {
        printf("Create RECEIVE_Task successfully. \r\n");
    }
    
    eventHandle = osEventFlagsNew(NULL);
    if( eventHandle != NULL )
    {
        printf("Create eventHandle successfully. \r\n");
    }
    
    Swtmr1Handle = osTimerNew( Swtmr1Task, osTimerPeriodic, NULL, NULL );
    if( Swtmr1Handle != NULL )
    {
        printf("Create Swtmr1Handle successfully. \r\n");
        
        if( ( xReturn = osTimerStart( Swtmr1Handle, 1000 )) == osOK )
        {
            printf("Start Swtmr1Handle successfully. \r\n");
        }
        else
        {
            printf("Start Swtmr1Handle failed. error: %d.\r\n", xReturn);
        }
    }
    
    Swtmr2Handle = osTimerNew( Swtmr2Task, osTimerOnce, NULL, NULL );
    if( Swtmr2Handle != NULL )
    {
        printf("Create Swtmr2Handle successfully. \r\n");
        
        if( ( xReturn = osTimerStart( Swtmr2Handle, 5000 ) ) == osOK )
        {
            printf("Start Swtmr2Handle successfully. \r\n");
        }
        else
        {
            printf("Start Swtmr2Handle failed. error: %d.\r\n", xReturn);
        }
    }
    
    vTaskDelete( defaultTaskHandle );
    
    taskEXIT_CRITICAL();
  //}
  /* USER CODE END 5 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
