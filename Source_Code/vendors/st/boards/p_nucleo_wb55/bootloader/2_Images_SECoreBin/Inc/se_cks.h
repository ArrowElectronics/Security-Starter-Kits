/**
  ******************************************************************************
  * @file    se_cks.h
  * @author  MCD Application Team
  * @brief   This file contains definitions for Secure Engine Customer Key Services
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SE_CKS_H
#define SE_CKS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "tl.h"
#include "shci_tl.h"
#include "shci.h"

/* Exported constants --------------------------------------------------------*/
#define SBSFU_AES_KEY_IDX 0x01U

/* Exported functions ------------------------------------------------------- */
void CKS_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* SE_CKS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

