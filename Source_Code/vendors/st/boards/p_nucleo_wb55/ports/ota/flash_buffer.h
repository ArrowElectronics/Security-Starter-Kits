#ifndef FLASH_BUFFER_H
#define FLASH_BUFFER_H

typedef enum {
  FLASH_BUFFER_OK = 0,
  FLASH_BUFFER_FAILED = !FLASH_BUFFER_OK
}t_flashBufRet;

t_flashBufRet xPushFlashBufferByte(uint8_t ucByte, uint32_t ulPushAddress);
t_flashBufRet xPopFlashBufferByte(uint8_t * const pucByte, uint32_t ulPopAddress);
uint32_t xPopFlashBufferSequence(uint8_t * const pucByte, uint32_t ulStartAddress, uint32_t ulSequenceLength);
t_flashBufRet xPeekFlashBufferByte(uint8_t * const pucByte, uint32_t ulPeekAddress);
uint32_t ulFlashBufferItemCount(void);
t_flashBufRet xFlashBufferPeekFirstItemAddress(uint32_t *ulAddress);


#endif /* FLASH_BUFFER_H */
