#include <stdint.h>

#include "flash_buffer.h"
#include "iot_linear_containers.h"

typedef struct
{
    IotLink_t   xLink;
    uint32_t    ulAddress;
    uint8_t     ucData;
}t_bufferData;

/* Init flash buffer first item*/
static IotListDouble_t xFlashBuffer = {
                                        .pPrevious = &xFlashBuffer,
                                        .pNext = &xFlashBuffer
                                      };

static bool prvMatchAddress(const IotLink_t * const, void * );
static int32_t prvAddressCompare(const IotLink_t * const, const IotLink_t * const );

static bool prvMatchAddress(const IotLink_t * const pCurrent, void * pMatch)
{
    if(IotLink_Container(t_bufferData, pCurrent, xLink)->ulAddress == *((uint32_t *)pMatch) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

static int32_t prvAddressCompare(const IotLink_t * const pLink, const IotLink_t * const pCurrent)
{
    if(IotLink_Container(t_bufferData, pLink, xLink)->ulAddress <
       IotLink_Container(t_bufferData, pCurrent, xLink)->ulAddress)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

uint32_t xPopFlashBufferSequence(uint8_t * const pucByte, uint32_t ulStartAddress, uint32_t ulSequenceLength)
{
    uint32_t ulReadBytes = 0;

    for(uint8_t i = 0; ulReadBytes != ulSequenceLength; i++)
    {
        uint8_t ucTempByte = 0;

        if(FLASH_BUFFER_OK == xPopFlashBufferByte(&ucTempByte, ulStartAddress + i))
        {
            ulReadBytes++;
            pucByte[i] = ucTempByte;
        }
        else
        {
            break;
        }
    }

    return ulReadBytes;
}

t_flashBufRet xPopFlashBufferByte(uint8_t * const pucByte, uint32_t ulPopAddress)
{
    t_flashBufRet xReturnCode = FLASH_BUFFER_FAILED;
    IotLink_t *pMatchedItem;

    if(IotListDouble_IsEmpty(&xFlashBuffer))
    {
        return FLASH_BUFFER_FAILED;
    }

    pMatchedItem = IotListDouble_FindFirstMatch(&xFlashBuffer, IotListDouble_PeekHead(&xFlashBuffer), prvMatchAddress, &ulPopAddress);
    if(NULL == pMatchedItem)
    {
        xReturnCode = FLASH_BUFFER_FAILED;
    }
    else
    {
        *pucByte = IotLink_Container(t_bufferData, pMatchedItem, xLink)->ucData;
        xReturnCode = FLASH_BUFFER_OK;
    }

    IotListDouble_Remove(pMatchedItem);
    vPortFree(IotLink_Container(t_bufferData, pMatchedItem, xLink));

    return xReturnCode;
}

t_flashBufRet xPeekFlashBufferByte(uint8_t * const pucByte, uint32_t ulPeekAddress)
{
    t_flashBufRet xReturnCode = FLASH_BUFFER_FAILED;
    IotLink_t *pMatchedItem;

    if(IotListDouble_IsEmpty(&xFlashBuffer))
    {
        return FLASH_BUFFER_FAILED;
    }

    pMatchedItem = IotListDouble_FindFirstMatch(&xFlashBuffer, IotListDouble_PeekHead(&xFlashBuffer), prvMatchAddress, &ulPeekAddress);
    if(NULL == pMatchedItem)
    {
        xReturnCode = FLASH_BUFFER_FAILED;
    }
    else
    {
        *pucByte = IotLink_Container(t_bufferData, pMatchedItem, xLink)->ucData;
        xReturnCode = FLASH_BUFFER_OK;
    }

    return xReturnCode;
}

t_flashBufRet xPushFlashBufferByte(uint8_t ucByte, uint32_t ulPushAddress)
{
    t_flashBufRet xReturnCode = FLASH_BUFFER_FAILED;
    t_bufferData *pxData;

    pxData = (t_bufferData *)pvPortMalloc(sizeof(t_bufferData));

    pxData->ucData = ucByte;
    pxData->ulAddress = ulPushAddress;

    IotListDouble_InsertSorted(&xFlashBuffer, &pxData->xLink, &prvAddressCompare);

    return xReturnCode;
}

t_flashBufRet xFlashBufferPeekFirstItemAddress(uint32_t *ulAddress)
{
    if(IotListDouble_IsEmpty(&xFlashBuffer))
    {
        return FLASH_BUFFER_FAILED;
    }

    *ulAddress = IotLink_Container(t_bufferData, IotListDouble_PeekHead(&xFlashBuffer), xLink)->ulAddress;

    return FLASH_BUFFER_OK;
}

uint32_t ulFlashBufferItemCount(void)
{
    return (uint32_t)IotListDouble_Count(&xFlashBuffer);
}



