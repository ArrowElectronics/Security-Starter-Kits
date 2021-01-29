/**
 ******************************************************************************
 * @file    main.c
 * @author  MCD Application Team
 * @brief   BLE application with BLE core
 *
  @verbatim
  ==============================================================================
                    ##### IMPORTANT NOTE #####
  ==============================================================================

  This application requests having the stm32wb5x_BLE_Stack_fw.bin binary
  flashed on the Wireless Coprocessor.
  If it is not the case, you need to use STM32CubeProgrammer to load the appropriate
  binary.

  All available binaries are located under following directory:
  /Projects/STM32_Copro_Wireless_Binaries

  Refer to UM2237 to learn how to use/install STM32CubeProgrammer.
  Refer to /Projects/STM32_Copro_Wireless_Binaries/ReleaseNote.html for the
  detailed procedure to change the Wireless Coprocessor binary.

  @endverbatim
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */


/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "app_common.h"

#include "app_entry.h"
#include "tl.h"
#include "cmsis_os.h"
#include "dbg_trace.h"
#include "shci_tl.h"

/* Demo includes */
#include "aws_demo.h"
#include "iot_logging_task.h"
#include "aws_clientcredential.h"
#include "iot_demo_logging.h"
/* Application version info. */
#include "aws_application_version.h"

#include "iot_ble.h"
#include "iot_ble_numericComparison.h"
#include "optiga/pal/pal_i2c.h"
#include "optiga/optiga_util.h"
#include "mutualauth_crypto.h"


/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define mainLOG_UART_RX_BUF_LEN     1
/* The SPI driver polls at a high priority. The logging task's priority must also
 * be high to be not be starved of CPU time. */
#define mainLOGGING_TASK_PRIORITY                         ( configMAX_PRIORITIES - 1 )
#define mainLOGGING_TASK_STACK_SIZE                       ( configMINIMAL_STACK_SIZE * 5 )
#define mainLOGGING_MESSAGE_QUEUE_LENGTH                  ( 15 )
/* Private macros ------------------------------------------------------------*/
/* Global variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc = { 0 }; /**< RTC handler declaration */
/* thread ID for processing hci and shci events */
osThreadId RF_ThreadId;
/* Console UART for receiving input characters */
QueueHandle_t UARTqueue = NULL;

/* Private variables ---------------------------------------------------------*/
uint8_t LogUartRxBuf[mainLOG_UART_RX_BUF_LEN];
extern pal_i2c_t optiga_pal_i2c_context_0;

/* Private function prototypes -----------------------------------------------*/
static void Reset_BackupDomain( void );
static void Init_RTC( void );
static void SystemClock_Config( void );
static void Reset_Device( void );
static void Reset_IPCC( void );
static void Init_Exti( void );
static void Console_UART_Init( void );

static void RF_Thread_Process(void const *argument);

/* Functions Definition ------------------------------------------------------*/

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main( void )
{
  HAL_Init();

  Reset_Device();

  /* When the application is expected to run at higher speed, it should be better to set the correct system clock
   * in system_stm32yyxx.c so that the initialization phase is running at max speed. */
  SystemClock_Config(); /**< Configure the system clock */

  Init_Exti( );

  Init_RTC();

  /* Configure UART for LOG out and password confirmation (Numeric comparison) */
  Console_UART_Init();

  NumericComparisonInit();
  pal_i2c_init(&optiga_pal_i2c_context_0);
  BUTTON_INIT();

  UARTqueue = xQueueCreate( 1, sizeof( INPUTMessage_t ) );

  /* Create thread for processing hci and shci events */
  osThreadDef(RF_STACK, RF_Thread_Process, osPriorityNormal, 0, 2048);
  RF_ThreadId = osThreadCreate(osThread(RF_STACK), NULL);

  /* Create tasks that are not dependent on the WiFi being initialized. */
  xLoggingTaskInitialize( mainLOGGING_TASK_STACK_SIZE,
                          mainLOGGING_TASK_PRIORITY,
                          mainLOGGING_MESSAGE_QUEUE_LENGTH );

  /* Start the scheduler.  Initialization that requires the OS to be running,
   * including the WiFi initialization, is performed in the RTOS daemon task
   * startup hook. */
  vTaskStartScheduler();

  /* We should never get here as control is now taken by the scheduler */
  for(;;);
}


/**
  * @brief  Function checks Log UART receive queue. Used for numeric comparison
  * @param  pxINPUTmessage: pointer to input message, copy uart input bytes to it
  * @param  xAuthTimeout: Timeout in milliseconds
  * @retval BaseType_t: pdTRUE or pdFALSE
  */
BaseType_t getUserMessage( INPUTMessage_t * pxINPUTmessage, TickType_t xAuthTimeout )
{
      BaseType_t xReturnMessage = pdFALSE;
      INPUTMessage_t xTmpINPUTmessage;

      pxINPUTmessage->pcData = (uint8_t *)pvPortMalloc(sizeof(uint8_t));
      pxINPUTmessage->xDataSize = 1;
      if(pxINPUTmessage->pcData != NULL)
      {
          if (xQueueReceive(UARTqueue, (void * )&xTmpINPUTmessage, (portTickType) xAuthTimeout ))
          {
              *pxINPUTmessage->pcData = *xTmpINPUTmessage.pcData;
              pxINPUTmessage->xDataSize = xTmpINPUTmessage.xDataSize;
              xReturnMessage = pdTRUE;
          }
      }

      return xReturnMessage;
}


/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/

void vApplicationDaemonTaskStartupHook( void )
{
}

/**
  * @brief  Rx Transfer completed callback
  * @param  UartHandle: UART handle
  * @note   This example shows a simple way to report end of DMA Rx transfer, and
  *         you can add your own implementation.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  /* Set transmission flag: transfer complete */


}

/**
 * @brief  Thread used to processing hci and shci events
 * @param  Thread not used
 * @retval None
 */
static void RF_Thread_Process(void const *argument)
{
  (void) argument;
  osEvent event;

  /* Init all transport layers for BLE, start BLE stack on CPU2 and then run Demo */
  APPE_Init( );

  for(;;)
  {
    /* Wait events from BLE stack */
    event = osSignalWait( 0, osWaitForever);

    if(event.value.signals & (1<<CFG_TASK_HCI_ASYNCH_EVT_ID))
    {
      hci_user_evt_proc();
    }

    if(event.value.signals & (1<<CFG_TASK_SYSTEM_HCI_ASYNCH_EVT_ID))
    {
      shci_user_evt_proc();
    }
  }
}

/*-----------------------------------------------------------*/

static void Init_Exti( void )
{
  /**< Disable all wakeup interrupt on CPU1  except IPCC(36), HSEM(38) */
  LL_EXTI_DisableIT_0_31(~0);
  LL_EXTI_DisableIT_32_63( (~0) & (~(LL_EXTI_LINE_36 | LL_EXTI_LINE_38)) );

  return;
}

/*-----------------------------------------------------------*/

static void Reset_Device( void )
{
#if ( CFG_HW_RESET_BY_FW == 1 )
  Reset_BackupDomain();

  Reset_IPCC();
#endif

  return;
}

/*-----------------------------------------------------------*/

static void Reset_IPCC( void )
{
  LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_IPCC);

  LL_C1_IPCC_ClearFlag_CHx(
      IPCC,
      LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4
      | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

  LL_C2_IPCC_ClearFlag_CHx(
      IPCC,
      LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4
      | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

  LL_C1_IPCC_DisableTransmitChannel(
      IPCC,
      LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4
      | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

  LL_C2_IPCC_DisableTransmitChannel(
      IPCC,
      LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4
      | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

  LL_C1_IPCC_DisableReceiveChannel(
      IPCC,
      LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4
      | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

  LL_C2_IPCC_DisableReceiveChannel(
      IPCC,
      LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4
      | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

  return;
}

/*-----------------------------------------------------------*/

static void Reset_BackupDomain( void )
{
  if ((LL_RCC_IsActiveFlag_PINRST() != FALSE) && (LL_RCC_IsActiveFlag_SFTRST() == FALSE))
  {
    HAL_PWR_EnableBkUpAccess(); /**< Enable access to the RTC registers */

    /**
     *  Write twice the value to flush the APB-AHB bridge
     *  This bit shall be written in the register before writing the next one
     */
    HAL_PWR_EnableBkUpAccess();

    __HAL_RCC_BACKUPRESET_FORCE();
    __HAL_RCC_BACKUPRESET_RELEASE();
  }

  return;
}

/*-----------------------------------------------------------*/

/**
  * @brief RTC initialization
  * @retval None
  */
static void Init_RTC( void )
{
  HAL_PWR_EnableBkUpAccess(); /**< Enable access to the RTC registers */

  /**
   *  Write twice the value to flush the APB-AHB bridge
   *  This bit shall be written in the register before writing the next one
   */
  HAL_PWR_EnableBkUpAccess();

  __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE); /**< Select LSI as RTC Input */

  __HAL_RCC_RTC_ENABLE(); /**< Enable RTC */

  hrtc.Instance = RTC; /**< Define instance */

  /**
   * Set the Asynchronous prescaler
   */
  hrtc.Init.AsynchPrediv = CFG_RTC_ASYNCH_PRESCALER;
  hrtc.Init.SynchPrediv = CFG_RTC_SYNCH_PRESCALER;
  HAL_RTC_Init(&hrtc);

  MODIFY_REG(RTC->CR, RTC_CR_WUCKSEL, CFG_RTC_WUCKSEL_DIVIDER);

  return;
}

/*-----------------------------------------------------------*/

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI1
                              |RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4|RCC_CLOCKTYPE_HCLK2
                              |RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the peripherals clocks
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS|RCC_PERIPHCLK_RFWAKEUP
                              |RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_LPUART1;
  PeriphClkInitStruct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInitStruct.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  PeriphClkInitStruct.RFWakeUpClockSelection = RCC_RFWKPCLKSOURCE_LSI;
  PeriphClkInitStruct.SmpsClockSelection = RCC_SMPSCLKSOURCE_HSE;
  PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLKDIV_RANGE0;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/*-----------------------------------------------------------*/
#define LENGTH_ENDPOINT 50
uint8_t Endpoint[LENGTH_ENDPOINT];
uint16_t EndpointSize = LENGTH_ENDPOINT;
volatile uint8_t Get_Endpoint = 0;
/**
 * @brief Log Uart input callback. Send recieved bytes to queue and start interrupt recieve again
 */
static void LogUartRxCallback(void)
{
    BaseType_t xHigherPriorityTaskWoken;
    INPUTMessage_t xInputMessage;
    xInputMessage.pcData = (uint8_t *)&LogUartRxBuf;
    xInputMessage.xDataSize = sizeof(LogUartRxBuf);

#if ( BLE_MUTUAL_AUTH_SERVICES == 1 )
	static uint16_t offset=0;

	if (Get_Endpoint)
	{
		HW_UART_Receive_IT(hw_uart1, LogUartRxBuf, sizeof(LogUartRxBuf), LogUartRxCallback);
		if(LogUartRxBuf[0] != '\r' && LogUartRxBuf[0] != '\n' )
		{
			Endpoint[offset++] = LogUartRxBuf[0];
		}
		else
		{
			EndpointSize = offset;
			Get_Endpoint = 0;
			offset = 0;
		}
	}
	else
#endif
	{
		xQueueSendFromISR(UARTqueue, (void * )&xInputMessage, &xHigherPriorityTaskWoken);
		HW_UART_Receive_IT(hw_uart1, LogUartRxBuf, sizeof(LogUartRxBuf), LogUartRxCallback);
		if( xHigherPriorityTaskWoken )
		{
			portYIELD_FROM_ISR(&xHigherPriorityTaskWoken);
		}
	}
}

/*-----------------------------------------------------------*/

/**
 * @brief UART console initialization function.
 */
static void Console_UART_Init( void )
{
    HW_UART_Init(hw_uart1);
    HW_UART_Receive_IT(hw_uart1, LogUartRxBuf, sizeof(LogUartRxBuf), LogUartRxCallback);
}

/*-----------------------------------------------------------*/

/**
 * @brief  This function is executed in case of error occurrence.
 */
void Error_Handler( void )
{
    while( 1 )
    {
        BSP_LED_Toggle( LED_GREEN );
        HAL_Delay( 200 );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Warn user if pvPortMalloc fails.
 *
 * Called if a call to pvPortMalloc() fails because there is insufficient
 * free memory available in the FreeRTOS heap.  pvPortMalloc() is called
 * internally by FreeRTOS API functions that create tasks, queues, software
 * timers, and semaphores.  The size of the FreeRTOS heap is set by the
 * configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h.
 *
 */
void vApplicationMallocFailedHook()
{
    taskDISABLE_INTERRUPTS();

    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Loop forever if stack overflow is detected.
 *
 * If configCHECK_FOR_STACK_OVERFLOW is set to 1,
 * this hook provides a location for applications to
 * define a response to a stack overflow.
 *
 * Use this hook to help identify that a stack overflow
 * has occurred.
 *
 */
void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    char * pcTaskName )
{
    portDISABLE_INTERRUPTS();

    /* Loop forever */
    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
}
/*-----------------------------------------------------------*/

void * malloc( size_t xSize )
{
    configASSERT( xSize == ~0 );

    return NULL;
}
/*-----------------------------------------------------------*/


void vOutputChar( const char cChar,
                  const TickType_t xTicksToWait )
{
    ( void ) cChar;
    ( void ) xTicksToWait;
}
/*-----------------------------------------------------------*/

void vMainUARTPrintString( char * pcString )
{
    const uint32_t ulTimeout = 3000UL;
    HW_UART_Transmit(hw_uart1, ( uint8_t * ) pcString, strlen( pcString ),  ulTimeout);
}

/*************************************************************
 *
 * WRAP FUNCTIONS
 *
 *************************************************************/


/******************* (C) COPYRIGHT 2019 STMicroelectronics *****END OF FILE****/
