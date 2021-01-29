/*
 * Amazon FreeRTOS OTA PAL V1.0.0
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/* C Runtime includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Amazon FreeRTOS include. */
#include "FreeRTOS.h"
#include "aws_iot_ota_pal.h"
#include "aws_iot_ota_agent_internal.h"
#include "aws_ota_codesigner_certificate.h"
#include "AFR_image_desc.h"
#include "flash_operations.h"


/* Libraries include */
#include "mbedtls/base64.h"
#include "asn1utility.h"

/* ST include*/
#include "sfu_app_new_image.h"
#include "stm32wbxx_hal.h"
#include "flash_if.h"
#include "aws_ota_pal_settings.h"
#include "stm32wbxx_hal_def.h"

/*ST Crypto includes*/
#include "crypto.h"
#include "HASH/hash.h"

/* Bootloader includes */
#include "se_def_metadata.h"
#include "sfu_fwimg_regions.h"
#include "flash_buffer.h"
                                        /* fileCntx->ulFileSize must exceed 100 bytes limit to pass */
#define ALL_FIELDS_EMPTY(fileCntx)  (   (fileCntx->ulFileSize == 0 || fileCntx->ulFileSize < 100) &&\
                                        fileCntx->ulBlocksRemaining == 0 &&\
                                        fileCntx->ulFileAttributes == 0 &&\
                                        fileCntx->ulServerFileID == 0 &&\
                                        fileCntx->pucJobName == (void*)0 &&\
                                        fileCntx->pucStreamName == (void*)0 &&\
                                        fileCntx->pxSignature == (void*)0 &&\
                                        fileCntx->pucRxBlockBitmap == (void*)0 &&\
                                        fileCntx->pucCertFilepath == (void*)0 &&\
                                        fileCntx->ulUpdaterVersion == 0 &&\
                                        fileCntx->xIsInSelfTest == 0    )


/* Specify the OTA signature algorithm we support on this platform. */
const char cOTA_JSON_FileSignatureKey[ OTA_FILE_SIG_KEY_STR_MAX_LENGTH ] = "sig-sha256-ecdsa";   /* FIX ME. */
static int prvNextFreeFileHandle = 1;

/* The static functions below (prvPAL_CheckFileSignature and prvPAL_ReadAndAssumeCertificate) 
 * are optionally implemented. If these functions are implemented then please set the following macros in 
 * aws_test_ota_config.h to 1:
 * otatestpalCHECK_FILE_SIGNATURE_SUPPORTED
 * otatestpalREAD_AND_ASSUME_CERTIFICATE_SUPPORTED
 */

/**
 * @brief Verify the signature of the specified file.
 * 
 * This function should be implemented if signature verification is not offloaded
 * to non-volatile memory io functions.
 * 
 * This function is called from prvPAL_Close(). 
 * 
 * @param[in] C OTA file context information.
 * 
 * @return Below are the valid return values for this function.
 * kOTA_Err_None if the signature verification passes.
 * kOTA_Err_SignatureCheckFailed if the signature verification fails.
 * kOTA_Err_BadSignerCert if the if the signature verification certificate cannot be read.
 * 
 */
static OTA_Err_t prvPAL_CheckFileSignature( OTA_FileContext_t * const C );

/**
 * @brief Read the specified signer certificate from the filesystem into a local buffer.
 * 
 * The allocated memory returned becomes the property of the caller who is responsible for freeing it.
 * 
 * This function is called from prvPAL_CheckFileSignature(). It should be implemented if signature
 * verification is not offloaded to non-volatile memory io function.
 * 
 * @param[in] pucCertName The file path of the certificate file.
 * @param[out] ulSignerCertSize The size of the certificate file read.
 * 
 * @return A pointer to the signer certificate in the file system. NULL if the certificate cannot be read.
 * This returned pointer is the responsibility of the caller; if the memory is allocated the caller must free it.
 */
static uint8_t * prvPAL_ReadAndAssumeCertificate( const uint8_t * const pucCertName,
                                                  uint32_t * const ulSignerCertSize );
/**
 * @brief Flush entire buffer to flash
 * @return pdPASS if success or pdFAIL in case of error
 */
static BaseType_t prvFlushFlashBuffer();

/**
  * @brief  Write the header of the firmware to install
  * @param  pfw_header pointer to header to write.
  * @retval HAL_OK on success otherwise HAL_ERROR
  */
static BaseType_t prvWriteInstallHeader(uint8_t *pfw_header);

/**
  * @brief  SHA256 HASH digest compute.
  * @param  pucInputMessage: pointer to input message to be hashed.
  * @param  ulInputMessageLength: input data message length in byte.
  * @param  pucMessageHash: pointer to output parameter that will handle message digest
  * @param  plMessageHashLength: pointer to output digest length.
  * @retval error status: can be HASH_SUCCESS if success or one of
  *         HASH_ERR_BAD_PARAMETER, HASH_ERR_BAD_CONTEXT,
  *         HASH_ERR_BAD_OPERATION if error occured.
  */
static BaseType_t prvSHA256HashCompute(const uint8_t *pucInputMessage, const uint32_t ulInputMessageLength,
                                                   uint8_t *pucMessageHash, int32_t *plMessageHashLength);

static BaseType_t prvVerifySignature( uint8_t * pucHash,
                               size_t ulHashSize,
                               uint8_t * pucSignature,
                               size_t ulSignatureSize,
                               uint8_t * pucPublicKey,
                               uint32_t ulPublicKeySize);

/**
 * @brief Erase flash areas responsible for receiving new AFR image
 * @return pdPASS or pdFAIL in case of error
 */
static BaseType_t prvFlashPrepareForRX();


static BaseType_t prvFlashPrepareForRX()
{
    BaseType_t xReturn = pdPASS;

    if(pdFAIL == FlashOpClearDwlArea())
    {
        xReturn = pdFAIL;
    }

    if(pdFAIL == FlashOpClearDescriptorArea())
    {
        xReturn = pdFAIL;
    }

    return xReturn;
}

/*-----------------------------------------------------------*/

static BaseType_t prvWriteInstallHeader(uint8_t *pfw_header)
{
  BaseType_t xReturnCode = HAL_ERROR;

  if(NULL == pfw_header)
  {
      return HAL_ERROR;
  }

  if (pdPASS == FlashOpErasePages(SFU_IMG_SWAP_REGION_BEGIN_VALUE, SFU_IMG_SWAP_REGION_BEGIN_VALUE + SFU_IMG_IMAGE_OFFSET))
  {
    if(pdPASS == FlashOpWriteFlash(SFU_IMG_SWAP_REGION_BEGIN_VALUE, SE_FW_HEADER_TOT_LEN, pfw_header))
    {
        xReturnCode = HAL_OK;
    }
  }

  return xReturnCode;
}

static BaseType_t prvFlushFlashBuffer()
{
    BaseType_t xReturnCode = pdPASS;

    if(ulFlashBufferItemCount() > 0)
    {
        uint8_t ucWriteBuff[FLASH_IF_MIN_WRITE_LEN] = {0};
        uint32_t ulStartAddress = 0;
        uint32_t ulSequenceReadBytes = 0;

        /*Take intial item...*/
        xFlashBufferPeekFirstItemAddress(&ulStartAddress);

        /*Then fetching one time at a time*/
        while(0 != (ulSequenceReadBytes = xPopFlashBufferSequence(ucWriteBuff, ulStartAddress, FLASH_IF_MIN_WRITE_LEN)))
        {
            /* Writing to flash */
            if(pdFAIL == FlashOpWriteFlash( ulStartAddress, ulSequenceReadBytes, ucWriteBuff))
            {
                return pdFAIL;
            }

            xFlashBufferPeekFirstItemAddress(&ulStartAddress);
        }
    }

    return xReturnCode;
}

static BaseType_t prvSHA256HashCompute(const uint8_t *pucInputMessage, const uint32_t ulInputMessageLength,
                                                   uint8_t *pucMessageHash, int32_t *plMessageHashLength)
{
  SHA256ctx_stt P_pSHA256ctx;
  const uint8_t ucSHA256BlockSize = 64;
  BaseType_t error_status;

  /* Set the size of the desired hash digest */
  P_pSHA256ctx.mTagSize = CRL_SHA256_SIZE;

  /* Set flag field to default value */
  P_pSHA256ctx.mFlags = E_HASH_DEFAULT;

  error_status = SHA256_Init(&P_pSHA256ctx);

  /* check for initialization errors */
  if (error_status == HASH_SUCCESS)
  {
    uint32_t ulBytesLeft = ulInputMessageLength;
    uint32_t ucDataStartAddress = (uint32_t)(void *)pucInputMessage;

    while(ulBytesLeft != 0)
    {
        uint8_t ucSHA256Databuff[ucSHA256BlockSize];
        uint32_t ucSHA256DataSize = 0;

        if(ulBytesLeft < ucSHA256BlockSize)
        {
            ucSHA256DataSize = ulBytesLeft;
            FlashOpReadBlock(ucDataStartAddress, ulBytesLeft, ucSHA256Databuff);
            ulBytesLeft -= ulBytesLeft;
            ucDataStartAddress += ulBytesLeft;
        }
        else
        {
            ucSHA256DataSize = ucSHA256BlockSize;
            FlashOpReadBlock(ucDataStartAddress, ucSHA256BlockSize, ucSHA256Databuff);
            ulBytesLeft -= ucSHA256BlockSize;
            ucDataStartAddress += ucSHA256BlockSize;
        }

        /* Add data to be hashed */
        error_status = SHA256_Append(&P_pSHA256ctx,
                                     ucSHA256Databuff,
                                     ucSHA256DataSize);

        if(error_status != HASH_SUCCESS)
        {
            break;
        }

    }

    if (error_status == HASH_SUCCESS)
    {
      /* retrieve */
      error_status = SHA256_Finish(&P_pSHA256ctx, pucMessageHash, plMessageHashLength);
    }
  }

  return error_status;
}

static BaseType_t prvVerifySignature( uint8_t * pusHash,
                               size_t ulHashSize,
                               uint8_t * pucSignature,
                               size_t ulSignatureSize,
                               uint8_t * pucRawPublicKey,
                               uint32_t ulRawPublicKeySize)
{
    DEFINE_OTA_METHOD_NAME( "prvVerifySignature" );

    /******************************************************************************/
    /******** Parameters for Elliptic Curve P-256 SHA-256 from FIPS 186-3**********/
    /******************************************************************************/
    /* PKA operation need abs(a) */
    const uint8_t P_256_absA[] __attribute__((aligned(4))) =
    {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x03
    };
    /* PKA operation need the sign of A */
    const uint32_t P_256_a_sign = 1;
    const uint8_t P_256_p[] __attribute__((aligned(4))) =
    {
      0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    const uint8_t P_256_n[] __attribute__((aligned(4))) =
    {
      0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84, 0xF3, 0xB9,
      0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51
    };
    const uint8_t P_256_Gx[] __attribute__((aligned(4))) =
    {
      0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47, 0xF8, 0xBC, 0xE6, 0xE5, 0x63,
      0xA4, 0x40, 0xF2, 0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0, 0xF4, 0xA1,
      0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96
    };
    const uint8_t P_256_Gy[] __attribute__((aligned(4))) =
    {
      0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B, 0x8E, 0xE7, 0xEB, 0x4A, 0x7C,
      0x0F, 0x9E, 0x16, 0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE, 0xCB, 0xB6,
      0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5
    };

    BaseType_t xReturn = ERROR;
    PKA_HandleTypeDef hpka = {0};
    PKA_ECDSAVerifInTypeDef ECDSA_verif = {0};
    uint8_t ucPublicKey[ECC_PUBLIC_KEY_SIZE];
    uint8_t ucSignature[ECC_SIGNATURE_BYTES];
    uint8_t * pucPublicKey;
    const uint8_t *pSign_r;
    const uint8_t *pSign_s;
    const uint8_t *pPub_x;
    const uint8_t *pPub_y;
    uint32_t const halPkaTimeoutMs = 5000;

    memset( &ucPublicKey, 0, sizeof( ucPublicKey ) );
    memset( &ucSignature, 0, sizeof( ucSignature ) );

    /* The public key comes in the ASN.1 DER format,  and so we need everything
     * except the DER metadata which fortunately in this case is containded in the front part of buffer */
    pucPublicKey = pucRawPublicKey + ( ulRawPublicKeySize - ECC_PUBLIC_KEY_SIZE );

    /* The signature is also ASN.1 DER encoded, but here we need to decode it properly */
    if(ERROR == asn1_decodeSignature( ucSignature, pucSignature, pucSignature + ulSignatureSize ))
    {
        xReturn = ERROR;
    }
    else
    {
        /* Set the local variables dealing with the public key */
        pPub_x  =  pucPublicKey;
        pPub_y = (uint8_t *)(pucPublicKey + 32);

        /* signature to be verified with r and s components */
        pSign_r = ucSignature;
        pSign_s = (uint8_t *)(ucSignature + 32);

        /* HAL function call to initialize HW PKA and launch ECDSA verification */
        hpka.Instance = PKA;
        if (HAL_PKA_Init(&hpka) == HAL_OK)
        {
            /* Set input parameters */
            ECDSA_verif.primeOrderSize =  sizeof(P_256_n);
            ECDSA_verif.modulusSize =     sizeof(P_256_p);
            ECDSA_verif.coefSign =        P_256_a_sign;
            ECDSA_verif.coef =            P_256_absA;
            ECDSA_verif.modulus =         P_256_p;
            ECDSA_verif.basePointX =      P_256_Gx;
            ECDSA_verif.basePointY =      P_256_Gy;
            ECDSA_verif.primeOrder =      P_256_n;

            ECDSA_verif.pPubKeyCurvePtX = pPub_x;
            ECDSA_verif.pPubKeyCurvePtY = pPub_y;
            ECDSA_verif.RSign =           pSign_r;
            ECDSA_verif.SSign =           pSign_s;
            ECDSA_verif.hash =            pusHash;

            /* Launch the verification */
            if(HAL_OK == HAL_PKA_ECDSAVerif(&hpka, &ECDSA_verif, halPkaTimeoutMs))
            {
                /* Compare to expected result */
                if (HAL_PKA_ECDSAVerif_IsValidSignature(&hpka))
                {
                    OTA_LOG_L2("[%s] ECDSA signature valid!\n\r", OTA_METHOD_NAME);
                    xReturn = SUCCESS;
                }
                else
                {
                    OTA_LOG_L2("[%s] ECDSA signature invalid!\n\r", OTA_METHOD_NAME);
                    xReturn = ERROR;
                }
            }
            else
            {
                xReturn = ERROR;
            }
        }
    }

    /* HAL function call to deinitialize HW PKA peripheral */
    HAL_PKA_DeInit(&hpka);

    return xReturn;
}




/*-----------------------------------------------------------*/

OTA_Err_t prvPAL_CreateFileForRx( OTA_FileContext_t * const C )
{
    DEFINE_OTA_METHOD_NAME( "prvPAL_CreateFileForRx" );
    SFU_FwImageFlashTypeDef fw_image_dwl_area;
    OTA_Err_t xReturnCode = kOTA_Err_None;
    C->lFileHandle = prvNextFreeFileHandle;

    if(NULL == C)
    {
        OTA_LOG_L1("[%s] Error: context pointer is null.\r\n", OTA_METHOD_NAME);
        xReturnCode = kOTA_Err_RxFileCreateFailed;
    }
    else
    {
        /* Get info about the download area */
        if (SFU_APP_GetDownloadAreaInfo(&fw_image_dwl_area) != HAL_ERROR)
        {
            if( C->ulFileSize > fw_image_dwl_area.MaxSizeInBytes)
            {
                xReturnCode= kOTA_Err_OutOfMemory;
            }          
        }
    }

    if(pdFAIL == prvFlashPrepareForRX())
    {
        xReturnCode = kOTA_Err_RxFileCreateFailed;
    }

    return xReturnCode;
}
/*-----------------------------------------------------------*/

OTA_Err_t prvPAL_Abort( OTA_FileContext_t * const C )
{
    DEFINE_OTA_METHOD_NAME( "prvPAL_Abort" );
    OTA_Err_t xReturnCode = kOTA_Err_None;

    if( C == NULL )
    {
        OTA_LOG_L1( "[%s] Abort - Fail\r\n", OTA_METHOD_NAME );
        xReturnCode = kOTA_Err_AbortFailed;
    }
    else
    {
        OTA_LOG_L1( "[%s] Abort - OK\r\n", OTA_METHOD_NAME );

        prvFlushFlashBuffer();

        C->lFileHandle = ( int32_t ) NULL;
    }
    return xReturnCode;
}
/*-----------------------------------------------------------*/

/* Write a block of data to the specified file. */
int16_t prvPAL_WriteBlock( OTA_FileContext_t * const C,
                           uint32_t ulOffset,
                           uint8_t * const pauData,
                           uint32_t ulBlockSize )
{
    DEFINE_OTA_METHOD_NAME( "prvPAL_WriteBlock" );

    (void)C;
    int16_t xReturnCode = -1;
    SFU_FwImageFlashTypeDef fw_image_dwl_area;
    uint32_t ulAddress = 0;
    uint32_t ulDwlAreaEnd = 0;

    /* Get info about the download area */
    if (SFU_APP_GetDownloadAreaInfo(&fw_image_dwl_area) == HAL_OK)
    {
        ulAddress = fw_image_dwl_area.DownloadAddr + ulOffset;
        ulDwlAreaEnd = ulAddress + fw_image_dwl_area.MaxSizeInBytes;
    }
    else
    {
        return -1;
    }

    /* Out of boundaries check */
    if( (ulAddress + ulBlockSize) > ulDwlAreaEnd )
    {
        OTA_LOG_L1( "[%s] Error trying to write beyond download area.\r\n", OTA_METHOD_NAME);
        return -1;
    }

    /* Write in buffer if input data size is not aligned */
    if( (ulBlockSize % FLASH_IF_MIN_WRITE_LEN) != 0 ||
           (ulAddress & 0x07) !=0  )
    {
        for(uint8_t i = 0; i < ulBlockSize; i++)
        {
            xPushFlashBufferByte(pauData[i], ulAddress + i);
        }

        xReturnCode = pdPASS;
    }
    else
    {
        xReturnCode = FlashOpWriteFlash( ulAddress, ulBlockSize, pauData );
    }


    if( xReturnCode == pdPASS )
    {
        OTA_LOG_L1( "[%s] Write %u bytes at %u address\r\n",
                    OTA_METHOD_NAME,
                    ulBlockSize,
                    ulOffset );

        return (int16_t)ulBlockSize;
    }
    else
    {
        OTA_LOG_L1( "[%s] Write %ul bytes at %ul address failed.\r\n",
                    OTA_METHOD_NAME,
                    ulBlockSize,
                    ulOffset);

        return -1;
    }
}
/*-----------------------------------------------------------*/

OTA_Err_t prvPAL_CloseFile( OTA_FileContext_t * const C )
{
    DEFINE_OTA_METHOD_NAME( "prvPAL_CloseFile" );

    OTA_Err_t xError;

    prvFlushFlashBuffer();

    AFRImageDescrWriteInitial(C);

    xError = prvPAL_CheckFileSignature( C );

    /* Increment the file handler */
    prvNextFreeFileHandle++;

    return xError;
}
/*-----------------------------------------------------------*/

static OTA_Err_t prvPAL_CheckFileSignature( OTA_FileContext_t * const C )
{
    DEFINE_OTA_METHOD_NAME( "prvPAL_CheckFileSignature" );

    SFU_FwImageFlashTypeDef fw_image_dwl_area;
    uint8_t ucHashBuffer[CRL_SHA256_SIZE];
    int32_t lHashSize = 0;
    uint32_t ulPubKeySize = 0;
    OTA_Err_t xErrCode = kOTA_Err_None;

    /* Get info about the download area */
    if (SFU_APP_GetDownloadAreaInfo(&fw_image_dwl_area) != HAL_OK)
    {
        return -1;
    }

    uint8_t *pucPublicKey = prvPAL_ReadAndAssumeCertificate (NULL, &ulPubKeySize);
    if (pucPublicKey == NULL)
    {
        xErrCode = kOTA_Err_BadSignerCert;
    }
    else
    {
        if(HASH_SUCCESS == prvSHA256HashCompute((uint8_t *)fw_image_dwl_area.DownloadAddr, C->ulFileSize, (uint8_t *)ucHashBuffer, &lHashSize))
        {
            if(ERROR == prvVerifySignature( (uint8_t*) &ucHashBuffer, lHashSize, C->pxSignature->ucData, C->pxSignature->usSize, pucPublicKey, ulPubKeySize ))
            {
                xErrCode = kOTA_Err_SignatureCheckFailed;
            }
        }
        else
        {
            xErrCode = kOTA_Err_SignatureCheckFailed;
        }
    }

    vPortFree( pucPublicKey );

    return xErrCode;
}
/*-----------------------------------------------------------*/

static uint8_t * prvPAL_ReadAndAssumeCertificate(  const uint8_t * const pucCertName,
                                                   uint32_t * const ulSignerCertSize )
{
    DEFINE_OTA_METHOD_NAME( "prvPAL_ReadAndAssumeCertificate" );

    /* Tag by which the beginning of the ECDSA in the public key can be found */
    const char pcOTA_PAL_ASN_1_ECDSA_TAG[] = "\x06\x07\x2A\x86\x48\xCE\x3D\x02\x01\x06\x08\x2A\x86\x48\xCE\x3D\x03\x01\x07";

    const char pcOTA_PAL_CERT_BEGIN[] = "-----BEGIN CERTIFICATE-----";
    const char pcOTA_PAL_CERT_END[] = "-----END CERTIFICATE-----";

    uint8_t * pucPublicKey = NULL;
    uint32_t ulPubKeyLength = 0;

    *ulSignerCertSize = sizeof(signingcredentialSIGNING_CERTIFICATE_PEM);
    /* Skip the "BEGIN CERTIFICATE" */
    char* pucCertBegin = strstr (signingcredentialSIGNING_CERTIFICATE_PEM, pcOTA_PAL_CERT_BEGIN) ;

    if (pucCertBegin == NULL)
    {
       return NULL;
    }
    pucCertBegin += sizeof(pcOTA_PAL_CERT_BEGIN);
    /* Skip the "END CERTIFICATE" */
    char* pucCertEnd = strstr(pucCertBegin, pcOTA_PAL_CERT_END);
    if (pucCertEnd == NULL)
    {
       return NULL;
    }

    unsigned char * pucDecodedCertificate;
    size_t ulDecodedCertificateSize;

    /* Calculating decoded cert buffer size */
    mbedtls_base64_decode(NULL, 0, &ulDecodedCertificateSize, (unsigned char *)pucCertBegin, pucCertEnd - pucCertBegin);
    pucDecodedCertificate = (unsigned char*) pvPortMalloc(ulDecodedCertificateSize);

    if (pucDecodedCertificate == NULL)
    {
       return NULL;
    }

    mbedtls_base64_decode(pucDecodedCertificate, ulDecodedCertificateSize, &ulDecodedCertificateSize, (unsigned char *)pucCertBegin, pucCertEnd - pucCertBegin);
    /* Find the tag of the ECDSA public signature*/
    uint8_t * pucPublicKeyStart = pucDecodedCertificate;
    while (pucPublicKeyStart < pucDecodedCertificate + ulDecodedCertificateSize )
    {
       if (memcmp(pucPublicKeyStart, pcOTA_PAL_ASN_1_ECDSA_TAG, sizeof(pcOTA_PAL_ASN_1_ECDSA_TAG) - 1) == 0)
       {
           break;
       }
       pucPublicKeyStart ++;
    }

    /* Return error if decoded length is zero */
    if (0 == ulDecodedCertificateSize) {
       vPortFree(pucDecodedCertificate);
       return NULL;
    }
    pucPublicKeyStart -= 4;
    ulPubKeyLength = pucPublicKeyStart[1] + 2;
    vPortFree(pucDecodedCertificate);

    pucPublicKey = pvPortMalloc(ulPubKeyLength);
    if( pucPublicKey != NULL)
    {
       memcpy(pucPublicKey, pucPublicKeyStart, ulPubKeyLength);
       *ulSignerCertSize = ulPubKeyLength;
    }

    return pucPublicKey;
}
/*-----------------------------------------------------------*/

OTA_Err_t prvPAL_ResetDevice( void )
{
    DEFINE_OTA_METHOD_NAME("prvPAL_ResetDevice");
    NVIC_SystemReset();
    return kOTA_Err_None;
}
/*-----------------------------------------------------------*/

OTA_Err_t prvPAL_ActivateNewImage( void )
{
    OTA_Err_t xReturnCode = kOTA_Err_None;

    DEFINE_OTA_METHOD_NAME("prvPAL_ActivateNewImage");

    uint8_t  fw_header_input[SE_FW_HEADER_TOT_LEN];

    SFU_FwImageFlashTypeDef fw_image_dwl_area;

    if (SFU_APP_GetDownloadAreaInfo(&fw_image_dwl_area) != HAL_ERROR)
    {
          /* Read header in download area */
          memcpy((void *) fw_header_input, (void *) fw_image_dwl_area.DownloadAddr, sizeof(fw_header_input));

          if(HAL_ERROR == prvWriteInstallHeader(fw_header_input))
          {
              return kOTA_Err_ActivateFailed;
          }

          /* System Reboot*/
          OTA_LOG_L1("[%s] Image correctly downloaded - reboot\r\n", OTA_METHOD_NAME);
          HAL_Delay(1000U);
          NVIC_SystemReset();
    }
    else
    {
        xReturnCode = kOTA_Err_ActivateFailed;
    }

    return xReturnCode;
}
/*-----------------------------------------------------------*/

OTA_Err_t prvPAL_SetPlatformImageState( OTA_ImageState_t eState )
{
    DEFINE_OTA_METHOD_NAME( "prvPAL_SetPlatformImageState" );

    OTA_Err_t xStatus = kOTA_Err_None;

    /* Invalid image state */
    if(eState > eOTA_LastImageState)
    {
        return kOTA_Err_BadImageState;
    }

    ImageDescriptor_t old_descriptor = {};
    ImageDescriptor_t new_descriptor = {};

    if(pdFAIL == AFRImageDescrGetActual(&old_descriptor))
    {
        return kOTA_Err_CommitFailed;
    }

    memcpy(&new_descriptor, &old_descriptor, sizeof(ImageDescriptor_t));

    if (xStatus == kOTA_Err_None)
    {

        switch (eState)
        {

        case eOTA_ImageState_Accepted:
            /* Before accepting, valid descriptor must exists */
            if (0 != (strncmp((char *)(old_descriptor.pMagick), pcOTA_PAL_Magick, otapalMAGICK_SIZE)))
            {
                xStatus = kOTA_Err_CommitFailed;
            }
            else
            {
                new_descriptor.usImageFlags = otapalIMAGE_FLAG_VALID;
            }
            break;

        case eOTA_ImageState_Rejected:
        case eOTA_ImageState_Aborted:
            new_descriptor.usImageFlags = otapalIMAGE_FLAG_INVALID;
            break;

        case eOTA_ImageState_Testing:
            new_descriptor.usImageFlags = otapalIMAGE_FLAG_COMMIT_PENDING;
            break;

        default:
            xStatus = kOTA_Err_BadImageState;
            break;
        }
    }

    if(xStatus == kOTA_Err_None)
    {
        /* We don't wont to do anything with flash if it would leave it in the same state as it were */
        if ((new_descriptor.usImageFlags != old_descriptor.usImageFlags))
        {
            if(pdFALSE == AFRImageDescrSetActual(new_descriptor))
            {
                    xStatus = kOTA_Err_CommitFailed;
            }
        }
    }

    return xStatus;
}
/*-----------------------------------------------------------*/

OTA_PAL_ImageState_t prvPAL_GetPlatformImageState( void )
{
    DEFINE_OTA_METHOD_NAME( "prvPAL_GetPlatformImageState" );

    ImageDescriptor_t pxDescriptor;
    OTA_PAL_ImageState_t xImageState = eOTA_PAL_ImageState_Invalid;

    if(pdFAIL == AFRImageDescrGetActual(&pxDescriptor))
    {
        return eOTA_PAL_ImageState_Invalid;
    }

    switch( pxDescriptor.usImageFlags )
    {
        case otapalIMAGE_FLAG_COMMIT_PENDING:
            xImageState = eOTA_PAL_ImageState_PendingCommit;
            break;

        case otapalIMAGE_FLAG_VALID:
            xImageState = eOTA_PAL_ImageState_Valid;
            break;

        case otapalIMAGE_FLAG_INVALID:
            xImageState = eOTA_PAL_ImageState_Invalid;
            break;
    }

    return xImageState;
}
/*-----------------------------------------------------------*/

/* Provide access to private members for testing. */
#ifdef AMAZON_FREERTOS_ENABLE_UNIT_TESTS
    #include "aws_ota_pal_test_access_define.h"
#endif
