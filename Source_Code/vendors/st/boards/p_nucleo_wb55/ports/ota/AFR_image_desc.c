#include "string.h"
#include "AFR_image_desc.h"
#include "aws_ota_pal_settings.h"
#include "mapping_export.h"
#include "flash_operations.h"

static ImageDescriptor_t *prvGetPointerToLastDesc(void);

static ImageDescriptor_t *prvGetPointerToNewDesc(void);

static ImageDescriptor_t *prvGetPointerToLastDesc(void);

static ImageDescriptor_t *prvGetPointerToNewDesc(void);

static ImageDescriptor_t *prvGetPointerToLastDesc(void)
{
    ImageDescriptor_t * pxIterator = (ImageDescriptor_t *)REGION_DESCRIPTOR_START;

    /* Seeking for last written descriptor  */
    while(0 == strncmp((char *)(pxIterator->pMagick), pcOTA_PAL_Magick, otapalMAGICK_SIZE))
    {
        pxIterator++;

        /* Out of borders check */
        if(((uint32_t)pxIterator + sizeof(ImageDescriptor_t)) > REGION_DESCRIPTOR_END)
        {
            return NULL;
        }
    }

    if(REGION_DESCRIPTOR_START != (uint32_t)pxIterator)
    {
        /* Found last descriptor. Shifting pointer back to it. */
        pxIterator--;
    }

    return pxIterator;
}

static ImageDescriptor_t *prvGetPointerToNewDesc(void)
{
    ImageDescriptor_t *pxNewDescr;

    pxNewDescr = prvGetPointerToLastDesc();

    if(NULL == pxNewDescr)
    {
        return NULL;
    }

    if(0 == strncmp((char *)(pxNewDescr->pMagick), pcOTA_PAL_Magick, otapalMAGICK_SIZE))
    {
        pxNewDescr++;
    }

    return pxNewDescr;
}

BaseType_t AFRImageDescrSetActual(ImageDescriptor_t xImgDescr)
{
    ImageDescriptor_t *pxNewDescr;

    pxNewDescr = prvGetPointerToNewDesc();

    if(NULL == pxNewDescr)
    {
        return pdFAIL;
    }

    /* Out of borders check */
    if((uint32_t)pxNewDescr + sizeof(ImageDescriptor_t) > REGION_DESCRIPTOR_END)
    {
        return pdFAIL;
    }

    return FlashOpWriteFlash((uint32_t)pxNewDescr, sizeof(ImageDescriptor_t), (uint8_t *)&xImgDescr);
}

BaseType_t AFRImageDescrGetActual(ImageDescriptor_t * pxImgDescr)
{
    ImageDescriptor_t *pxActualDescr;

    pxActualDescr = prvGetPointerToLastDesc();

    if(NULL == pxActualDescr)
    {
        return pdFAIL;
    }

    memcpy(pxImgDescr, pxActualDescr, sizeof(ImageDescriptor_t));

   return pdPASS;
}

BaseType_t AFRImageDescrWriteInitial( OTA_FileContext_t * const C )
{
    ImageDescriptor_t xDescriptor;

    memset( &xDescriptor, 0, sizeof( xDescriptor ) );

    /* Fill initial descriptor fields */
    memcpy( &xDescriptor.pMagick, pcOTA_PAL_Magick, otapalMAGICK_SIZE );
    xDescriptor.ulStartAddress = REGION_DESCRIPTOR_START + otapalDESCRIPTOR_SIZE;
    xDescriptor.ulEndAddress = xDescriptor.ulStartAddress + C->ulFileSize;
    xDescriptor.ulExecutionAddress = xDescriptor.ulStartAddress;
    xDescriptor.ulHardwareID = 0;
    xDescriptor.usImageFlags = otapalIMAGE_FLAG_NEW;
    xDescriptor.ulSignatureSize = C->pxSignature->usSize;
    xDescriptor.ulSequenceNumber = 1;

    if( C->pxSignature != NULL)
    {
      if( C->pxSignature->usSize <= kOTA_MaxSignatureSize)
       {
          memcpy( &xDescriptor.pSignature, C->pxSignature->ucData, C->pxSignature->usSize );
       }
     }

    return FlashOpWriteFlash( REGION_DESCRIPTOR_START, sizeof( xDescriptor ), (uint8_t *)&xDescriptor );
}
