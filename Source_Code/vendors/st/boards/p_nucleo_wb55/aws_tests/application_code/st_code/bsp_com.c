#include "bsp_com.h"
/**
  * @brief  Retargets the C library printf function to the USARTx.
  * @param  ch: character to send
  * @param  f: pointer to file (not used)
  * @retval The character transmitted
  */

void BSP_COM_Init(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* Peripheral clock enable */
  __HAL_RCC_USART1_CLK_ENABLE();
  
  __HAL_RCC_GPIOB_CLK_ENABLE();
  /**USART1 GPIO Configuration    
  PB6     ------> USART1_TX
  PB7     ------> USART1_RX 
  */
  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Configure the UART peripheral                                        */
  /* Put the USART peripheral in the Asynchronous mode (UART Mode) */
  /* UART configured as follows:
      - Word Length = 8 Bits
      - Stop Bit = One Stop bit
      - Parity = None
      - BaudRate = 115200 baud
      - Hardware flow control disabled (RTS and CTS signals) */
  huart->Instance                    = USART1;
  huart->Init.BaudRate               = 115200;
  huart->Init.WordLength             = UART_WORDLENGTH_8B;
  huart->Init.StopBits               = UART_STOPBITS_1;
  huart->Init.Parity                 = UART_PARITY_NONE;
  huart->Init.HwFlowCtl              = UART_HWCONTROL_NONE;
  huart->Init.Mode                   = UART_MODE_TX_RX;
  huart->Init.HwFlowCtl              = UART_HWCONTROL_NONE;
  huart->Init.OverSampling           = UART_OVERSAMPLING_16;
  huart->Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
  huart->Init.ClockPrescaler         = UART_PRESCALER_DIV1;
  huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  
  if(HAL_UART_DeInit(huart) != HAL_OK)
  {
    Error_Handler();
  }
  
  if(HAL_UART_Init(huart) != HAL_OK)
  {
    Error_Handler();
  }  
}
