/**
  ******************************************************************************
  * @file    se_cks.c
  * @author  MCD Application Team
  * @brief   Secure Engine Customer Key Services.
  *          This file provides set of functions to manage communication with M0
  *          for key services.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

#include "se_cks.h"
#include "hw.h"
#include "hw_if.h"

/* Private defines -----------------------------------------------------------*/
#define CFG_TL_EVT_QUEUE_LENGTH 1
#define CFG_TL_MOST_EVENT_PAYLOAD_SIZE 16   /**< Set to 16 with the memory manager and the mailbox */
#define TL_EVENT_FRAME_SIZE ( TL_EVT_HDR_SIZE + CFG_TL_MOST_EVENT_PAYLOAD_SIZE )
#define POOL_SIZE (CFG_TL_EVT_QUEUE_LENGTH*4U*DIVC(( sizeof(TL_PacketHeader_t) + TL_EVENT_FRAME_SIZE ), 4U))

/* Private variables ---------------------------------------------------------*/
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t EvtPool[POOL_SIZE];
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static TL_CmdPacket_t SystemCmdBuffer;
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t SystemSpareEvtBuffer[sizeof(TL_PacketHeader_t) +
                                                                         TL_EVT_HDR_SIZE + 16U];
/* CFG_TLBLE_MOST_EVENT_PAYLOAD_SIZE=16 */

static __IO uint8_t FLAG_WAIT_CPU2_RDY = 1U;
static __IO uint8_t FLAG_WAIT_RESP = 0U;

/* Private function prototypes -----------------------------------------------*/
static void Tl_Init(void);
static void Reset_IPCC(void);

/**
  * @brief  Transport layer initialization
  * @note
  * @param  None
  * @retval None
  */
void CKS_Init(void)
{
  Reset_IPCC();
  Tl_Init();

}

/**
  * @brief  Transport layer initialization
  * @note
  * @param  None
  * @retval None
  */
static void Tl_Init(void)
{

  TL_MM_Config_t tl_mm_config;
  SHCI_TL_HciInitConf_t SHci_Tl_Init_Conf;
  /**< Reference table initialization */
  TL_Init();

  /**< System channel initialization */
  SHci_Tl_Init_Conf.p_cmdbuffer = (uint8_t *)&SystemCmdBuffer;
  SHci_Tl_Init_Conf.StatusNotCallBack = NULL;
  shci_init(NULL, (void *) &SHci_Tl_Init_Conf);

  /**< Memory Manager channel initialization */
  tl_mm_config.p_SystemSpareEvtBuffer = SystemSpareEvtBuffer;
  tl_mm_config.p_AsynchEvtPool = EvtPool;
  tl_mm_config.AsynchEvtPoolSize = POOL_SIZE;
  TL_MM_Init(&tl_mm_config);

  TL_Enable();

  while (FLAG_WAIT_CPU2_RDY == 1U);   /* Waiting acknowledge from CPU 2 */

  return;
}

/**
  * @brief  Reset IPCC
  * @note
  * @param  None
  * @retval None
  */
static void Reset_IPCC(void)
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

/**
  * @brief  Interrupt service routine called when the system channel
  *         reports a packet has been received
  * @param  pdata Packet
  * @retval None
  */
void shci_notify_asynch_evt(void *pdata)
{
  FLAG_WAIT_CPU2_RDY = 0U;
  return;
}

/**
  * @brief  This function is called when an System HCI command is sent and the response is
  *         received from the CPU2.
  * @param  flag: Release flag
  * @retval None
  */
void shci_cmd_resp_release(uint32_t flag)
{
  FLAG_WAIT_RESP = 1U;
  return;
}

/**
  * @brief  This function is called when an System HCO Command is sent and the response
  *         is waited from the CPU2.
  *         The application shall implement a mechanism to not return from this function
  *         until the waited event is received.
  *         This is notified to the application with shci_cmd_resp_release().
  *         It is called from the same context the System HCI command has been sent.
  * @param  timeout: Waiting timeout
  * @retval None
  */
void shci_cmd_resp_wait(uint32_t timeout)
{
  while ((FLAG_WAIT_RESP & 0x1) == 0U);
  FLAG_WAIT_RESP = 0U;
  return;
}



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
