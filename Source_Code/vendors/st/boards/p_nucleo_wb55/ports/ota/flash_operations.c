
/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_ll_hsem.h"
#include "string.h"
#include "flash_operations.h"
#include "flash_buffer.h"
#include "aws_ota_pal_settings.h"
#include "sfu_app_new_image.h"
#include "flash_driver.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Gets the sector of a given address
  * @param  uAddr: Address of the FLASH Memory
  * @retval The sector of a given address
  */
static uint32_t GetSector(uint32_t uAddr);

/* Private functions -------------------------------------------------------- */
uint32_t GetSector(uint32_t uAddr)
{
  uint32_t sector = 0U;

  if (uAddr < (FLASH_BASE + (FLASH_BANK_SIZE)))
  {
    /* Bank 1 */
	  sector = (uAddr - FLASH_BASE) / FLASH_PAGE_SIZE;
  }
  else
  {
    /* Bank 2 */
	  sector = (uAddr - (FLASH_BASE + (FLASH_BANK_SIZE))) / FLASH_PAGE_SIZE;
  }

  return sector;
}

/**
 * @brief Flash driver default semaphore wait procedure replace. This version switches task for saving CPU time,
 * while waiting for the semaphore.
 * @param  WaitedSemId: The semaphore ID this function should not return until it is free
 * @retval: WAITED_SEM_FREE when semaphore is free.
*/
WaitedSemStatus_t FD_WaitForSemAvailable(WaitedSemId_t WaitedSemId)
{
	 const TickType_t xDelay = 5 / portTICK_PERIOD_MS;

	 while(LL_HSEM_GetStatus(HSEM, WaitedSemId))
	 {
		 vTaskDelay(xDelay);
	 }

	return WAITED_SEM_FREE;
}

/* Public functions ----------------------------------------------------------*/
BaseType_t FlashOpReadBlock(uint32_t ulStartAddr, uint32_t ulBlockSize, uint8_t * const pucData )
{
    BaseType_t xReturnCode = pdPASS;

    for(uint8_t i = 0; i < ulBlockSize; i++)
    {
        uint8_t ucPopByte;

        /* Choosing data source */
        if(FLASH_BUFFER_OK == xPeekFlashBufferByte(&ucPopByte, ulStartAddr + i))
        {   /* From buffer */
            pucData[i] = ucPopByte;
        }
        else
        {
            /* Directly from flash */
            pucData[i] = *(char *)(ulStartAddr + i);
        }
    }

    return xReturnCode;
}

BaseType_t FlashOpWriteFlash( uint32_t ulOffset, uint32_t ulBlockSize, uint8_t * const pauData )
{
    BaseType_t xReturnCode = pdFAIL;
    uint32_t uBytesLeftToWrite = ulBlockSize;
    uint32_t uBytesCopied = 0;

    /* Check address alignment for 64-bit data */
    if( ulOffset & 0x07 )
    {
        OTA_LOG_L1( "Error! Unaligned flash memory write access\r\n" );
        return pdFAIL;
    }

    /* Skip zero length data */
    if(ulBlockSize == 0)
    {
        return pdPASS;
    }

    while(0 != uBytesLeftToWrite)
    {
        uint8_t uCopyBytes = 0;
        uint8_t uaWriteBuffer[FLASH_IF_MIN_WRITE_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

        if(uBytesLeftToWrite < FLASH_IF_MIN_WRITE_LEN)
        {
            uCopyBytes = uBytesLeftToWrite;
        }
        else
        {
            uCopyBytes = FLASH_IF_MIN_WRITE_LEN;
        }

        memcpy(uaWriteBuffer, pauData + uBytesCopied, uCopyBytes);

        if(0 == FD_WriteData((ulOffset + uBytesCopied), (uint64_t*)uaWriteBuffer, sizeof(uaWriteBuffer)/FLASH_IF_MIN_WRITE_LEN))
        {
            uBytesCopied += uCopyBytes;
            uBytesLeftToWrite -= uCopyBytes;

            xReturnCode = pdPASS;
        }
        else
        {
            xReturnCode = pdFAIL;
            break;
        }
    }

    return xReturnCode;
}

BaseType_t FlashOpErasePages( size_t xFromAddr, size_t xToAddr )
{
    BaseType_t xReturn = pdFAIL;
    uint32_t ulSectorsNumber = 0;
    uint32_t ulStartSector = GetSector(xFromAddr);
    uint32_t ulEndSector = GetSector(xToAddr);

    /* Check range is valid*/
    if( xFromAddr > xToAddr)
    {
        return pdFAIL;
    }

    /* Calculating erase size */
    if(ulStartSector == ulEndSector)
    {
    	ulSectorsNumber = 1;
    }
    else
    {
    	ulSectorsNumber = ulEndSector - ulStartSector;
    }

    if(0 == FD_EraseSectors(ulStartSector, ulSectorsNumber))
    {
        xReturn = pdPASS;
    }
    else
    {
        xReturn = pdFAIL;
    }

    return xReturn;
}

BaseType_t FlashOpClearDescriptorArea(void)
{
     BaseType_t xReturn = pdPASS;

     /* Clear image descriptor area */
     if(pdFAIL == FlashOpErasePages(REGION_DESCRIPTOR_START, REGION_DESCRIPTOR_START + otapalDESCRIPTOR_SIZE))
     {
         xReturn = pdFAIL;
     }

     return xReturn;
}

BaseType_t FlashOpClearDwlArea(void)
{
    BaseType_t xReturn = pdPASS;

    /* Clear download area */
    SFU_FwImageFlashTypeDef fw_image_dwl_area;

    SFU_APP_GetDownloadAreaInfo(&fw_image_dwl_area);

    if (pdFAIL == FlashOpErasePages(fw_image_dwl_area.DownloadAddr, fw_image_dwl_area.DownloadAddr + fw_image_dwl_area.MaxSizeInBytes))
    {
        xReturn = pdFAIL;
    }

    return xReturn;
}
