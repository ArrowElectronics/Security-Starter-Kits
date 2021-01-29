#ifndef AFR_IMAGE_DESCR_H_
#define AFR_IMAGE_DESCR_H_

#include "FreeRTOS.h"
#include "aws_ota_pal_settings.h"

/**
 * @brief Gets last written image desciptor pointer
 * @param pxImgDescr: pointer to actual descriptor
 * @return pdPASS if success, pdFAIL in case of error.
 */
BaseType_t AFRImageDescrGetActual(ImageDescriptor_t * pxImgDescr);

/**
 * @brief AFRImageDescrSetActual
 * @param xImgDescr
 * @return
 */
BaseType_t AFRImageDescrSetActual(ImageDescriptor_t xImgDescr);

BaseType_t AFRImageDescrWriteInitial( OTA_FileContext_t * const C );


#endif /* AFR_IMAGE_DESCR_H_ */
