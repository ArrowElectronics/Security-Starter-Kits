/**
 ******************************************************************************
 * @file    app_entry.c
 * @author  MCD Application Team
 * @brief   Entry point of the Application
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
#include "app_common.h"

#include "main.h"
#include "app_entry.h"

#include "ble.h"
#include "tl.h"
#include "iot_test_ble_hal_common.h"

#include "cmsis_os.h"
#include "shci_tl.h"
#include "shci.h"
#include "hw_if.h"
#include "stm32_lpm.h"

#include "aws_test_runner.h"
#include "bluenrg_event_types.h"


/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define POOL_SIZE (CFG_TLBLE_EVT_QUEUE_LENGTH*4*DIVC(( sizeof(TL_PacketHeader_t) + TL_BLE_EVENT_FRAME_SIZE ), 4))
#define mainTEST_RUNNER_TASK_STACK_SIZE     ( configMINIMAL_STACK_SIZE * 12 )

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t EvtPool[POOL_SIZE];
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static TL_CmdPacket_t SystemCmdBuffer;
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t	SystemSpareEvtBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255];
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t	BleSpareEvtBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255];
PLACE_IN_SECTION("MB_MEM1") ALIGN(4) static TL_CmdPacket_t BleCmdBuffer;

/* Global variables ----------------------------------------------------------*/
extern osThreadId RF_ThreadId;
static osSemaphoreId SemHciId;
static osSemaphoreId SemShciId;
/* Global function prototypes ------------------------------------------------*/
extern bool IotTestNetwork_SelectNetworkType( uint16_t networkType );

/* Private function prototypes -----------------------------------------------*/
static void appe_Tl_Init( void );
static void APPE_SysStatusNot( SHCI_TL_CmdStatus_t status );
static void APPE_SysUserEvtRx( void * pPayload );
static void APP_BLE_Init( void );
static void BLE_UserEvtRx( void * pPayload );
static void BLE_StatusNot( HCI_TL_CmdStatus_t status );
static void Ble_Tl_Init( void );

/* Functions Definition ------------------------------------------------------*/
void APPE_Init( void )
{
  /* Initialize the TimerServer */
  HW_TS_Init(hw_ts_InitMode_Full, &hrtc);

  HAL_NVIC_SetPriority(IPCC_C1_RX_IRQn , 6, 0);
  HAL_NVIC_SetPriority(IPCC_C1_TX_IRQn , 6, 0);
  /* Initialize all transport layers */
  appe_Tl_Init();

  /**
   * From now, the application is waiting for the ready event ( VS_HCI_C2_Ready )
   * received on the system channel before starting the BLE Stack
   * This system event is received with APPE_SysUserEvtRx()
   */

  return;
}

// *************************************************************
// *
// * LOCAL FUNCTIONS
// *
// *************************************************************/
static void appe_Tl_Init( void )
{
  TL_MM_Config_t tl_mm_config;
  SHCI_TL_HciInitConf_t SHci_Tl_Init_Conf;

  /**< Reference table initialization */
  TL_Init();

  /**< System channel initialization */
  SHci_Tl_Init_Conf.p_cmdbuffer = (uint8_t*)&SystemCmdBuffer;
  SHci_Tl_Init_Conf.StatusNotCallBack = APPE_SysStatusNot;
  shci_init(APPE_SysUserEvtRx, (void*) &SHci_Tl_Init_Conf);

  /**< Memory Manager channel initialization */
  tl_mm_config.p_BleSpareEvtBuffer = BleSpareEvtBuffer;
  tl_mm_config.p_SystemSpareEvtBuffer = SystemSpareEvtBuffer;
  tl_mm_config.p_AsynchEvtPool = EvtPool;
  tl_mm_config.AsynchEvtPoolSize = POOL_SIZE;
  TL_MM_Init( &tl_mm_config );

  TL_Enable();

  return;
}

static void APPE_SysStatusNot( SHCI_TL_CmdStatus_t status )
{
  UNUSED(status);
  return;
}

static void APPE_SysUserEvtRx( void * pPayload )
{
  /* Traces channel initialization */
  TL_TRACES_Init( );

  APP_BLE_Init( );

  /* Create the task to run tests. */
  xTaskCreate( TEST_RUNNER_RunTests_task,
               "TestRunner",
               mainTEST_RUNNER_TASK_STACK_SIZE,
               NULL,
               tskIDLE_PRIORITY + 1,
               NULL );
  return;
}


void APP_BLE_Init( void )
{
  SHCI_C2_Ble_Init_Cmd_Packet_t ble_init_cmd_packet =
  {
    {{0,0,0}},                              /**< Header unused */
    {0,                                  /** pBleBufferAddress not used */
    0,                                  /** BleBufferSize not used */
    CFG_BLE_NUM_GATT_ATTRIBUTES,
    CFG_BLE_NUM_GATT_SERVICES,
    CFG_BLE_ATT_VALUE_ARRAY_SIZE,
    CFG_BLE_NUM_LINK,
    CFG_BLE_DATA_LENGTH_EXTENSION,
    CFG_BLE_PREPARE_WRITE_LIST_SIZE,
    CFG_BLE_MBLOCK_COUNT,
    CFG_BLE_MAX_ATT_MTU,
    CFG_BLE_SLAVE_SCA,
    CFG_BLE_MASTER_SCA,
    CFG_BLE_LSE_SOURCE,
    CFG_BLE_MAX_CONN_EVENT_LENGTH,
    CFG_BLE_HSE_STARTUP_TIME,
    CFG_BLE_VITERBI_MODE,
    CFG_BLE_LL_ONLY,
    0},
  };

  /**
   * Initialize Ble Transport Layer
   */
  Ble_Tl_Init( );

  /**
   * Starts the BLE Stack on CPU2
   */
  SHCI_C2_BLE_Init( &ble_init_cmd_packet );

  return;
}

static void Ble_Tl_Init( void )
{
  HCI_TL_HciInitConf_t Hci_Tl_Init_Conf;

  SemHciId = xSemaphoreCreateBinary();
  SemShciId = xSemaphoreCreateBinary();
  Hci_Tl_Init_Conf.p_cmdbuffer = (uint8_t*)&BleCmdBuffer;
  Hci_Tl_Init_Conf.StatusNotCallBack = BLE_StatusNot;
  hci_init(BLE_UserEvtRx, (void*) &Hci_Tl_Init_Conf);

  return;
}

/**
 * @brief Initialize BLE stask for ST.
 * On ST it is just a stub.
 */
BTStatus_t bleStackInit( void )
{
  return eBTStatusSuccess;
}


/*************************************************************
 *
 * WRAP FUNCTIONS
 *
 *************************************************************/
void shci_notify_asynch_evt(void* pdata)
{
  UNUSED(pdata);
  osSignalSet( RF_ThreadId, (1<<CFG_TASK_SYSTEM_HCI_ASYNCH_EVT_ID) );
  return;
}

void shci_cmd_resp_release(uint32_t flag)
{
  UNUSED(flag);
  osSemaphoreRelease( SemShciId );
  return;
}

void shci_cmd_resp_wait(uint32_t timeout)
{
  UNUSED(timeout);
  osSemaphoreWait( SemShciId, osWaitForever );
  return;
}

void hci_notify_asynch_evt(void* pdata)
{
  UNUSED(pdata);
  osSignalSet( RF_ThreadId, (1<<CFG_TASK_HCI_ASYNCH_EVT_ID) );
  return;
}

void hci_cmd_resp_release(uint32_t flag)
{
  UNUSED(flag);
  osSemaphoreRelease( SemHciId );
  return;
}

void hci_cmd_resp_wait(uint32_t timeout)
{
  UNUSED(timeout);
  osSemaphoreWait( SemHciId, osWaitForever );
  return;
}

static SVCCTL_UserEvtFlowStatus_t SVCCTL_UserEvtCallback( void *pckt )
{
  SVCCTL_EvtAckStatus_t event_notification_status;
  SVCCTL_UserEvtFlowStatus_t return_status;
  uint32_t i;

  hci_uart_pckt *hci_pckt = (hci_uart_pckt *)pckt;
  event_notification_status = SVCCTL_EvtNotAck;

  /**
   * Find callback function in events tables, refer to bluenrg1_events.c
   */
  if(hci_pckt->type == HCI_EVENT_PKT_TYPE) {
    hci_event_pckt *event_pckt = (hci_event_pckt*)hci_pckt->data;

    if(event_pckt->evt == EVT_LE_META_EVENT) {
      evt_le_meta_event *evt = (void *)event_pckt->data;

      for (i = 0; i < (sizeof(hci_le_meta_events_table)/sizeof(hci_le_meta_events_table_type)); i++) {
        if (evt->subevent == hci_le_meta_events_table[i].evt_code) {
          hci_le_meta_events_table[i].process((void *)evt->data);
          event_notification_status = SVCCTL_EvtAckFlowEnable;
          APP_LOG_L1(("event = %d\r\n", evt->subevent));
        }
      }
    }
    else if(event_pckt->evt == EVT_VENDOR) {
      evt_blue_aci *blue_evt = (void*)event_pckt->data;

      for (i = 0; i < (sizeof(hci_vendor_specific_events_table)/sizeof(hci_vendor_specific_events_table_type)); i++) {
        if (blue_evt->ecode == hci_vendor_specific_events_table[i].evt_code) {
          hci_vendor_specific_events_table[i].process((void *)blue_evt->data);
          event_notification_status = SVCCTL_EvtAckFlowEnable;
          APP_LOG_L1(("event = %d\r\n", blue_evt->ecode));
        }
      }
    }
    else {
      for (i = 0; i < (sizeof(hci_events_table)/sizeof(hci_events_table_type)); i++) {
        if (event_pckt->evt == hci_events_table[i].evt_code) {
          hci_events_table[i].process((void *)event_pckt->data);
          event_notification_status = SVCCTL_EvtAckFlowEnable;
          APP_LOG_L1(("event = %d\r\n", event_pckt->evt));
        }
      }
    }
  }

  /**
   * When no registered handlers (either Service or Client) has acknowledged the GATT event, it is reported to the application
   * a GAP event is always reported to the applicaiton.
   */
  switch (event_notification_status)
  {
    case SVCCTL_EvtNotAck:
      /**
       *  The event has NOT been managed.
       *  It shall be passed to the application for processing
       */
      return_status = SVCCTL_UserEvtFlowEnable;
      break;

    case SVCCTL_EvtAckFlowEnable:
      return_status = SVCCTL_UserEvtFlowEnable;
      break;

    case SVCCTL_EvtAckFlowDisable:
      return_status = SVCCTL_UserEvtFlowDisable;
      break;

    default:
      return_status = SVCCTL_UserEvtFlowEnable;
      break;
  }

  return (return_status);
}

static void BLE_UserEvtRx( void * pPayload )
{
  SVCCTL_UserEvtFlowStatus_t svctl_return_status;
  tHCI_UserEvtRxParam *pParam;

  pParam = (tHCI_UserEvtRxParam *)pPayload;

  svctl_return_status = SVCCTL_UserEvtCallback((void *)&(pParam->pckt->evtserial));
  if (svctl_return_status != SVCCTL_UserEvtFlowDisable)
  {
    pParam->status = HCI_TL_UserEventFlow_Enable;
  }
  else
  {
    pParam->status = HCI_TL_UserEventFlow_Disable;
  }
}

static void BLE_StatusNot( HCI_TL_CmdStatus_t status )
{
  UNUSED(status);
  /**
   * This notification indicate a ACI command is pending so that the application can implement a mechanism to make sure there is no risk
   * to queue a second ACI command
   * As all code that may send an ACI command is called from the same Thread, there is nothing specific to do
   */
  return;
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
