/**
  ******************************************************************************
  * @file    Loader_Bin.c
  * @author  MCD Application Team
  * @brief   Include LOADER binary.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright(c) 2017 STMicroelectronics International N.V.
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
#include "string.h"

#if defined (__GNUC__)
asm(".section LOADER_Bin,\"a\";"
                             ".incbin \"./../../../../../../projects/st/p_nucleo_wb55/cmake/bootloader/2_Images_SECoreBin/build/Loader.bin\";"
);
#endif 
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
