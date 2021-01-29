/**
  ******************************************************************************
  * @file    stm32wbxx_it.c
  * @author  MCD Application Team
  * @brief   Interrupt Service Routines.
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
/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_it.h"
#include "hw.h"

/* External variables --------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Interruption and Exception Handlers         */
/******************************************************************************/


/**
  * @brief  This function handles SysTick exception.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
}


/******************************************************************************/
/*                 STM32WBxx Peripherals Interrupt Handlers                  */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32wbxx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles IPCC C1 RX interrupt request.
  * @param  None
  * @retval None
  */
void IPCC_C1_TX_IRQHandler(void)
{
  HW_IPCC_Tx_Handler();

  return;
}

/**
  * @brief  This function handles IPCC C1 TX interrupt request.
  * @param  None
  * @retval None
  */
void IPCC_C1_RX_IRQHandler(void)
{
  HW_IPCC_Rx_Handler();
  return;
}



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
