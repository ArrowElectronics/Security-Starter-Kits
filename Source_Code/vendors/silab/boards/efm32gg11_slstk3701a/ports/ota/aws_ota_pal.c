/*
 * FreeRTOS OTA PAL V1.0.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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

/* FreeRTOS include. */
#include "FreeRTOS.h"
#include "aws_iot_ota_pal.h"
#include "aws_iot_ota_agent_internal.h"

/* Board specific include */
#include "btl_interface.h"

/*OTA security specific include */
#include "aws_ota_codesigner_certificate.h"

//#include "pkcs11t.h"
#include "iot_pkcs11.h"
#include "iot_crypto.h"


/* Configure logs for the functions in this file. */
#ifdef IOT_LOG_LEVEL_PLATFORM
#define LIBRARY_LOG_LEVEL        IOT_LOG_LEVEL_PLATFORM
#else
#ifdef IOT_LOG_LEVEL_GLOBAL
#define LIBRARY_LOG_LEVEL    IOT_LOG_LEVEL_GLOBAL
#else
#define LIBRARY_LOG_LEVEL    IOT_LOG_NONE
#endif
#endif
#define LIBRARY_LOG_NAME    ( "OTA" )
#include "iot_logging_setup.h"
/*
 * -------------------------------------------------------------+
 * OTA USED AS INTERNAL FLASH SOTORAGE FOR SLOT 1 FOR 2MB FLASH |
 * -------------------------------------------------------------+
 *
 * */


#define SLOT_ID							(uint32_t) 0
#define SLOT_OFFSET						(uint32_t) 0
#define kOTA_HalfSecondDelay    		pdMS_TO_TICKS( 500UL )
#define BINARY_CHUNK_BUFF_SIZE			256

extern const ApplicationProperties_t 	sl_app_properties;

typedef struct {
	const OTA_FileContext_t 	* 		cur_ota;
	BootloaderInformation_t 			info;
	uint32_t 							slotId;
	uint32_t 							offset;
	int32_t 							valid_image;
	uint32_t 							data_write_len;
}si_ota_ctx_t;

/* ctx instance */
static si_ota_ctx_t ota_ctx;

/*Maintain image state and return to ota agent when aseked*/
static OTA_ImageState_t image_state;

/* Specify the OTA signature algorithm we support on this platform. */
const char cOTA_JSON_FileSignatureKey[ OTA_FILE_SIG_KEY_STR_MAX_LENGTH ] = "sig-sha256-ecdsa";   /* FIX ME. */


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
 @return A pointer to the signer certificate in the file system. NULL if the certificate cannot be read.
 * This returned pointer is the responsibility of the caller; if the memory is allocated the caller must free it.
 */
static uint8_t * prvPAL_ReadAndAssumeCertificate( const uint8_t * const pucCertName,
		uint32_t * const ulSignerCertSize );


static void _ota_ctx_clear( si_ota_ctx_t * ota_ctx )
{
	if( ota_ctx != NULL )
	{
		memset( ota_ctx, 0, sizeof( si_ota_ctx_t ) );
	}
}

static bool _ota_ctx_validate( OTA_FileContext_t * C )
{
	return( C != NULL && ota_ctx.cur_ota == C && C->pucFile == ( uint8_t * ) &ota_ctx );
}

static void _ota_ctx_close( OTA_FileContext_t * C )
{
	if( C != NULL )
	{
		C->pucFile = 0;
	}
	ota_ctx.cur_ota = 0;
}

/*-----------------------------------------------------------*/

OTA_Err_t prvPAL_CreateFileForRx( OTA_FileContext_t * const C )
{
	DEFINE_OTA_METHOD_NAME( "prvPAL_CreateFileForRx" );
	if( ( NULL == C ) || ( NULL == C->pucFilePath ) )
	{
		printf("OTA_Err:prvPAL_CreateFileForRx NULL instances\n");
		return kOTA_Err_RxFileCreateFailed;
	}

	bootloader_getInfo(&ota_ctx.info);
	if (ota_ctx.info.type == NO_BOOTLOADER) {
		printf(
				"\nOTA_Err:No Bootloader is present (first stage or main stage invalid)\n");
		return kOTA_Err_RxFileCreateFailed;

	}
#ifdef OTA_DBG
	printf("\nCurrent Silabs APP version: %d \n",sl_app_properties.app.version);
#endif

	// Initialize silabs Boot Loader
	if (bootloader_init() & BOOTLOADER_ERROR_INIT_BASE) {
		printf("OTA_Err:Bootloader init failed \n");
		return kOTA_Err_RxFileCreateFailed;
	}

	ota_ctx.cur_ota = C;
	ota_ctx.slotId = SLOT_ID;
	ota_ctx.offset = SLOT_OFFSET;
	ota_ctx.data_write_len = 0;
	/* not a valid image currently */
	ota_ctx.valid_image = BOOTLOADER_ERROR_INIT_BASE;

	C->pucFile = ( uint8_t * ) &ota_ctx;

	if(bootloader_eraseStorageSlot(ota_ctx.slotId) != BOOTLOADER_OK) {
		printf("OTA_Err:Bootloader erase sector failed \n");
		return kOTA_Err_RxFileCreateFailed;
	}

#ifdef OTA_DBG
	printf("OTA_Dbg: OTA begun successfully \n");
#endif

	return kOTA_Err_None;
}
/*-----------------------------------------------------------*/

OTA_Err_t prvPAL_Abort( OTA_FileContext_t * const C )
{
	DEFINE_OTA_METHOD_NAME( "prvPAL_Abort" );
	OTA_Err_t ota_ret = kOTA_Err_FileAbort;

	if( _ota_ctx_validate( C ) )
	{
		_ota_ctx_close( C );
		ota_ret = kOTA_Err_None;
	}
	else if( C && ( C->pucFile == NULL ) )
	{
		ota_ret = kOTA_Err_None;
	}

	return ota_ret;
}
/*-----------------------------------------------------------*/

/* Write a block of data to the specified file. */
int16_t prvPAL_WriteBlock( OTA_FileContext_t * const C,
		uint32_t ulOffset,
		uint8_t * const pacData,
		uint32_t ulBlockSize )
{
	DEFINE_OTA_METHOD_NAME( "prvPAL_WriteBlock" );

	if( _ota_ctx_validate( C ) )
	{

		uint32_t write_offset = ota_ctx.offset + ulOffset;

		int32_t ret = bootloader_writeStorage(ota_ctx.slotId,write_offset, pacData,ulBlockSize);
		if( ret != BOOTLOADER_OK )
		{
			printf("OTA_Err:Couldn't flash at the absolute offset %d\n", ulOffset);
			printf("OTA_Err:Couldn't flash at the actual offset %d\n", write_offset);
			return -1;
		}

		ota_ctx.data_write_len 	+= ulBlockSize;

	}
	else
	{
		printf("OTA_Err:Invalid OTA Context\n");
		return -1;
	}

	return ulBlockSize;
}
/*-----------------------------------------------------------*/

OTA_Err_t prvPAL_CloseFile( OTA_FileContext_t * const C )
{
	DEFINE_OTA_METHOD_NAME( "prvPAL_CloseFile" );
	printf("prvPAL_CloseFile \n");
	OTA_Err_t result = kOTA_Err_FileAbort;

	if( _ota_ctx_validate( C ) )
	{
		_ota_ctx_close( C );
		result = kOTA_Err_None;
	}
	if( C->pxSignature == NULL )
	{
		printf("Image Signature not found \n" );
		_ota_ctx_clear( &ota_ctx );
		result = kOTA_Err_SignatureCheckFailed;
	}
	else if( ota_ctx.data_write_len == 0 )
	{
		printf("No data written to partition \n" );
		result = kOTA_Err_SignatureCheckFailed;
	}
	else
	{
		/* Verify the file signature, close the file and return the signature verification result. */
		result = prvPAL_CheckFileSignature( C );
		printf("OTA signature result %d \n",result);
		if( result != kOTA_Err_None )
		{
			if(bootloader_eraseStorageSlot(ota_ctx.slotId) != BOOTLOADER_OK) {
				printf("OTA_Err:Bootloader erase sector failed \n");
				result = kOTA_Err_RxFileCreateFailed;
			}
		}
		else
		{
			printf("OTA signature DONE !!!\n");
		}

	}

	/* FIX ME. */
	//return kOTA_Err_FileClose;
	return result;
}
/*-----------------------------------------------------------*/
OTA_Err_t prvPAL_CheckFileSignature( OTA_FileContext_t * const C )
{
	DEFINE_OTA_METHOD_NAME( "prvPAL_CheckFileSignature" );
	OTA_Err_t result = kOTA_Err_None ;
	uint32_t ulSignerCertSize;
	void * pvSigVerifyContext;
	u8 * pucSignerCert = 0;
	//const void * buf = NULL;
	uint8_t *bin_buff = NULL;
	uint32_t read_offset = 0;
	uint32_t read_chunk = BINARY_CHUNK_BUFF_SIZE;
	uint32_t curr_bin_size = ota_ctx.data_write_len;

	/* Verify an ECDSA-SHA256 signature. */
	if( CRYPTO_SignatureVerificationStart( &pvSigVerifyContext, cryptoASYMMETRIC_ALGORITHM_ECDSA,
				cryptoHASH_ALGORITHM_SHA256 ) == pdFALSE )
	{
		printf("signature verification start failed \n");
		return kOTA_Err_SignatureCheckFailed;
	}

	pucSignerCert = prvPAL_ReadAndAssumeCertificate( ( const u8 * const ) C->pucCertFilepath, &ulSignerCertSize );
	if( pucSignerCert == NULL )
	{
		printf("cert read failed \n" );
		return kOTA_Err_BadSignerCert;
	}

	bin_buff = (uint8_t *)pvPortMalloc(BINARY_CHUNK_BUFF_SIZE);

	if(bin_buff == NULL)
	{
		printf("OTA malloc failed \n" );
		result = kOTA_Err_OutOfMemory;
		goto end;
	}

	do {

		int32_t ret = bootloader_readStorage(ota_ctx.slotId, read_offset, bin_buff,
				read_chunk);
		if (ret != BOOTLOADER_OK) {
			printf("OTA_Err:Couldn't read flash at offset %d \n",read_offset);
			result = kOTA_Err_FileAbort;
			goto end;
		}

		CRYPTO_SignatureVerificationUpdate(pvSigVerifyContext, bin_buff,
				read_chunk);

		curr_bin_size = curr_bin_size - read_chunk;
		read_offset = read_offset + read_chunk ;

		read_chunk = (curr_bin_size < BINARY_CHUNK_BUFF_SIZE ? curr_bin_size : BINARY_CHUNK_BUFF_SIZE) ;

		memset(bin_buff,0,BINARY_CHUNK_BUFF_SIZE);

	} while ( curr_bin_size > 0 );
	vPortFree(bin_buff);
	bin_buff = NULL;

	if( CRYPTO_SignatureVerificationFinal( pvSigVerifyContext, ( char * ) pucSignerCert, ulSignerCertSize,
				C->pxSignature->ucData, C->pxSignature->usSize ) == pdFALSE )
	{
		printf("signature verification failed \n" );
		result = kOTA_Err_SignatureCheckFailed;
	}
	else
	{
		result = kOTA_Err_None;
	}

end:

	/* Free the signer certificate that we now own after prvReadAndAssumeCertificate(). */
	if( pucSignerCert != NULL )
	{
		vPortFree( pucSignerCert );
	}

	if(bin_buff != NULL)
	{
		vPortFree(bin_buff);
	}

	return result;
}

static uint8_t * prvPAL_ReadAndAssumeCertificate( const uint8_t * const pucCertName,
		uint32_t * const ulSignerCertSize )
{
	DEFINE_OTA_METHOD_NAME( "prvPAL_ReadAndAssumeCertificate" );
	uint8_t * pucCertData;
	uint32_t ulCertSize;
	uint8_t * pucSignerCert = NULL;
	CK_RV xResult;

	printf("Using aws_ota_codesigner_certificate.h.\r\n",
			(const char *) pucCertName);

	/* Allocate memory for the signer certificate plus a terminating zero so we can copy it and return to the caller. */
	ulCertSize = sizeof(signingcredentialSIGNING_CERTIFICATE_PEM);
	pucSignerCert = pvPortMalloc(ulCertSize); /*lint !e9029 !e9079 !e838 malloc proto requires void*. */
	pucCertData = (uint8_t *) signingcredentialSIGNING_CERTIFICATE_PEM; /*lint !e9005 we don't modify the cert but it could be set by PKCS11 so it's not const. */

	if (pucSignerCert != NULL) {
		memcpy(pucSignerCert, pucCertData, ulCertSize);
		*ulSignerCertSize = ulCertSize;
	} else {
		printf(
				"Error: No memory for certificate in prvPAL_ReadAndAssumeCertificate!\r\n");
	}

	return pucSignerCert;
}
/*-----------------------------------------------------------*/

OTA_Err_t prvPAL_ResetDevice( void )
{
	DEFINE_OTA_METHOD_NAME("prvPAL_ResetDevice");
	/* Short delay for debug log output before reset. */
	vTaskDelay( kOTA_HalfSecondDelay );
	/*House keeping + NVIC Reset*/
	bootloader_rebootAndInstall();
	return kOTA_Err_None;
}
/*-----------------------------------------------------------*/

OTA_Err_t  prvPAL_ActivateNewImage( void )
{
	DEFINE_OTA_METHOD_NAME("prvPAL_ActivateNewImage");
	int32_t ret = bootloader_verifyImage(ota_ctx.slotId, NULL);
	if(ret != BOOTLOADER_OK)
	{
		printf("OTA_Err:BTL Image verification failed \n");
	}
	_ota_ctx_clear( &ota_ctx );
	prvPAL_ResetDevice();
	/* this should not be hit*/
	return kOTA_Err_None;
}
/*-----------------------------------------------------------*/

OTA_Err_t prvPAL_SetPlatformImageState( OTA_ImageState_t eState )
{
	DEFINE_OTA_METHOD_NAME( "prvPAL_SetPlatformImageState" );
	OTA_Err_t ret = kOTA_Err_None;	
#if 1
	switch( eState )
	{
		case eOTA_ImageState_Accepted:
			ret = kOTA_Err_None;
			image_state = eOTA_ImageState_Accepted;
			break;

		case eOTA_ImageState_Rejected:
			ret = kOTA_Err_RejectFailed;
			image_state = eOTA_ImageState_Rejected;
			break;

		case eOTA_ImageState_Aborted:
			ret = kOTA_Err_AbortFailed;
			image_state = eOTA_ImageState_Aborted;
			break;

		case eOTA_ImageState_Testing:
			ret = kOTA_Err_None;
			image_state = eOTA_ImageState_Testing;
			break;

		default:
			ret = kOTA_Err_BadImageState;
			image_state = eOTA_ImageState_Unknown;
	}
#endif
	/* FIX ME. */
	// return kOTA_Err_BadImageState;
	return ret;
}
/*-----------------------------------------------------------*/

OTA_PAL_ImageState_t prvPAL_GetPlatformImageState( void )
{
	DEFINE_OTA_METHOD_NAME( "prvPAL_GetPlatformImageState" );
	OTA_PAL_ImageState_t ePalState = eOTA_PAL_ImageState_Valid;
#if 1
	switch (image_state)
	{
		case eOTA_ImageState_Testing:
			ePalState = eOTA_PAL_ImageState_PendingCommit;
			break;
		case eOTA_ImageState_Accepted:
			ePalState = eOTA_PAL_ImageState_Valid;
			break;
		case eOTA_ImageState_Rejected:
		case eOTA_ImageState_Aborted:
		default:
			ePalState = eOTA_PAL_ImageState_Invalid;
			break;
	}
#endif
	return ePalState;
}
/*-----------------------------------------------------------*/

/* Provide access to private members for testing. */
#ifdef AMAZON_FREERTOS_ENABLE_UNIT_TESTS
// #include "aws_ota_pal_test_access_define.h"
#endif
