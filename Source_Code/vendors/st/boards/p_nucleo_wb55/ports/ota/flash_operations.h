#ifndef FLASH_OPERATIONS_H_
#define FLASH_OPERATIONS_H_

#include "FreeRTOS.h"


/**
 * @brief Erase data from flash
 *
 * @param[in] xFrom Starting address of flash to be erased
 * @param[in] xTo Ending address of flash to be erased
 *
 * @return pdPASS if success or pdFAIL in case of error
 */
BaseType_t FlashOpErasePages( size_t xFrom, size_t xTo );

/**
 * @brief Sinchronously write data to flash
 *
 * @param[in] ulOffset Position to write
 * @param[in] ulBlockSize Data size
 * @param[in] pacData Data to be written
 *
 * @return pdPASS or pdFAIL in case of error
 */
BaseType_t FlashOpWriteFlash( uint32_t ulOffset, uint32_t ulBlockSize, uint8_t * const pacData );

/**
 * @brief Reading data block from flash
 * @param[in] ulStartAddr Address to start reading from
 * @param[in] ulBlockSize Size of data block
 * @param[out] pucData Data buffer of appropriate size
 * @return pdPASS or pdFAIL in case of error
 */
BaseType_t FlashOpReadBlock(uint32_t ulStartAddr, uint32_t ulBlockSize, uint8_t * const pucData );

/**
 * @brief AFR Descriptor sector full erase
 * @return pdPASS or pdFAIL in case of error
 */
BaseType_t FlashOpClearDescriptorArea(void);

/**
 * @brief AFR image download area full erase
 * @return pdPASS or pdFAIL in case of error
 */
BaseType_t FlashOpClearDwlArea(void);



#endif /* FLASH_OPERATIONS_H_ */
