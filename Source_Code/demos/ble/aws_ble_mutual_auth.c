/*
 * Amazon FreeRTOS V201912.00
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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

/**
 * @file aws_ble_mutual_auth.c
 * @brief Mutual authentication with App.
 */
#include "FreeRTOSConfig.h"
#include "iot_demo_logging.h"
#include "iot_ble_config.h"
#include "types/iot_network_types.h"
#include "aws_ble_mutual_auth.h"
#include "iot_ble.h"
#include "task.h"
#include "semphr.h"
#include "mutualauth_crypto.h"
#include "platform/iot_network.h"
#include "cmsis_os.h"
#include "authenticate_chip/pal_crypt.h"
#include <iot_ble_numericComparison.h>
#include <stm32wbxx_nucleo.h>
#include "main.h"
#include "aws_ota_codesigner_certificate.h"


static BaseType_t vGattDemoSvcHook( void );
extern const uint8_t ca_certificate[];
#define xServiceUUID_TYPE             \
    {                                 \
        .uu.uu128 = gattMutualauthSVC_UUID, \
        .ucType = eBTuuidType128      \
    }
#define xCharParamUUID_TYPE                  \
    {                                          \
        .uu.uu128 = gattCHAR_PARAM_UUID, \
        .ucType = eBTuuidType128               \
    }
#define xCharGWParamValueUUID_TYPE                  \
    {                                          \
        .uu.uu128 = gattCHAR_GW_PARAM_VALUE_UUID, \
        .ucType = eBTuuidType128               \
    }
#define xCharGatewayStatusUUID_TYPE                  \
    {                                          \
        .uu.uu128 = gattCHAR_GATEWAY_STATUS_UUID, \
        .ucType = eBTuuidType128               \
    }
#define xCharEdgeParamValueUUID_TYPE                  \
    {                                          \
        .uu.uu128 = gattCHAR_EDGE_PARAM_VALUE_UUID, \
        .ucType = eBTuuidType128               \
    }
#define xCharEdgeStatusUUID_TYPE                  \
    {                                          \
        .uu.uu128 = gattCHAR_EDGE_STATUS_UUID, \
        .ucType = eBTuuidType128               \
    }
#define xClientCharCfgUUID_TYPE                  \
    {                                            \
        .uu.uu16 = gattDemoCLIENT_CHAR_CFG_UUID, \
        .ucType = eBTuuidType16                  \
    }

static uint16_t usHandlesBuffer[ egattDemoNbAttributes ];


static const BTAttribute_t pxAttributeTable[] =
{
    {
        .xServiceUUID = xServiceUUID_TYPE
    },
    {
        .xAttributeType = eBTDbCharacteristic,
        .xCharacteristic =
        {
            .xUuid        = xCharParamUUID_TYPE,
            .xPermissions = ( IOT_BLE_CHAR_WRITE_PERM ),
            .xProperties  = ( eBTPropWrite )
        }
    },
    {
        .xAttributeType = eBTDbCharacteristic,
        .xCharacteristic =
        {
            .xUuid        = xCharGWParamValueUUID_TYPE,
            .xPermissions = ( IOT_BLE_CHAR_WRITE_PERM ),
            .xProperties  = ( eBTPropWrite )
        }
    },
    {
        .xAttributeType = eBTDbCharacteristic,
        .xCharacteristic =
        {
            .xUuid        = xCharGatewayStatusUUID_TYPE,
            .xPermissions = ( IOT_BLE_CHAR_READ_PERM ),
            .xProperties  = ( eBTPropRead | eBTPropNotify )
        }
    },
	{
        .xAttributeType = eBTDbDescriptor,
		.xCharacteristicDescr =
	    {
	    	.xUuid        = xClientCharCfgUUID_TYPE,
	        .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM )
	    }
	},
    {
        .xAttributeType = eBTDbCharacteristic,
        .xCharacteristic =
        {
            .xUuid        = xCharEdgeParamValueUUID_TYPE,
            .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
            .xProperties  = ( eBTPropRead )
        }
    },
    {
        .xAttributeType = eBTDbCharacteristic,
        .xCharacteristic =
        {
            .xUuid        = xCharEdgeStatusUUID_TYPE,
            .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
            .xProperties  = ( eBTPropWrite )
        }
    },
};

static const BTService_t xGattDemoService =
{
    .xNumberOfAttributes = egattDemoNbAttributes,
    .ucInstId            = 0,
    .xType               = eBTServiceTypePrimary,
    .pusHandlesBuffer    = usHandlesBuffer,
    .pxBLEAttributes     = ( BTAttribute_t * ) pxAttributeTable
};


static MUTUALAUTH_param_t Cur_param = 0;				/* store current parameter */
static uint8_t gw_status=0;				/* store gateway verification status */
/* mutex for lock the critical section */
SemaphoreHandle_t mutex;


static Random_Data_t device_token;
static Hash_Data_t devicetoken_digest;
static Random_Data_t gateway_token;
static Hash_Data_t gwtoken_digest;
static ECC_Pubkey_t ECDH_pubkey;		/* hold ecdh public key */
static Hash_Data_t hash_secret;
ECDH_Secret_t secret;

/**
 * @brief Should send the gateway status as a notification.
 */
BaseType_t xNotifyGatewayStatus = pdFALSE;

/**
 * @brief BLE connection ID to send the notification.
 */
uint16_t usBLEConnectionID;

extern const uint8_t ecc_pubkey_header[];

#define CHAR_HANDLE( svc, ch_idx )    ( ( svc )->pusHandlesBuffer[ ch_idx ] )
#define CHAR_UUID( svc, ch_idx )      ( ( svc )->pxBLEAttributes[ ch_idx ].xCharacteristic.xUuid )

/**
 * @brief Callback to receive parameter from a GATT client.
 *
 * @param[in] pxAttribute Attribute structure for the characteristic
 * @param pEventParam Event param for the read Request.
 */
void vParam( IotBleAttributeEvent_t * pEventParam );

/**
 * @brief Callback to receive gateway parameter values from a GATT client based on current parameter.
 *
 * @param pxAttribute
 * @param pEventParam
 */
void vGWParamValue( IotBleAttributeEvent_t * pEventParam );

/**
 * @brief Callback to send gateway verification status.
 *
 * @param pxAttribute
 * @param pEventParam
 */
void vGatewayStatus( IotBleAttributeEvent_t * pEventParam );
/**
 * @brief Callback to send edge/device parameter value to a GATT client based on current parameter.
 *
 * @param pxAttribute
 * @param pEventParam
 */
void vEdgeParamValue( IotBleAttributeEvent_t * pEventParam );
/**
 * @brief Callback to receive edge/device verification status from a GATT client.
 *
 * @param pxAttribute
 * @param pEventParam
 */
void vEdgeStatus( IotBleAttributeEvent_t * pEventParam );
/**
 * @brief Callback to enable notification when GATT client writes a value to the Client Characteristic
 * Configuration descriptor.
 *
 * @param pxAttribute  Attribute structure for the Client Characteristic Configuration Descriptor
 * @param pEventParam Write/Read event parametes
 */

void vEnableNotification( IotBleAttributeEvent_t * pEventParam );

/**
 * @brief Task used to Send data to Gateway
 *
 * @param pvParams NULL
 */

static void MUTUALAUTH_Send_Data(IotBleReadEventParams_t *pxReadParam);
/**
 * @brief Task used to Receive data from Gateway
 *
 * @param pvParams NULL
 */

static void MUTUALAUTH_Receive_Data(IotBleWriteEventParams_t *pxWriteParam);
extern void optiga_shell_init();
/**
 * @brief Callback for BLE connect/disconnect.
 *
 * Stops the Counter Update task on disconnection.
 *
 * @param[in] xStatus Status indicating result of connect/disconnect operation
 * @param[in] connId Connection Id for the connection
 * @param[in] bConnected true if connection, false if disconnection
 * @param[in] pxRemoteBdAddr Remote address of the BLE device which connected or disconnected
 */
static void _connectionCallback( BTStatus_t xStatus,
                                 uint16_t connId,
                                 bool bConnected,
                                 BTBdaddr_t * pxRemoteBdAddr );
/* ---------------------------------------------------------------------------------------*/

void IotBle_AddCustomServicesCb( void )
{
#if ( BLE_MUTUAL_AUTH_SERVICES == 1 )

        vGattDemoSvcHook();
#endif
}

static const IotBleAttributeEventCallback_t pxCallBackArray[ egattDemoNbAttributes ] =
{
    NULL,
    vParam,
    vGWParamValue,
	vGatewayStatus,
	vEnableNotification,
	vEdgeParamValue,
	vEdgeStatus,
};


/*-----------------------------------------------------------*/
static void MUTUALAUTH_Receive_Data(IotBleWriteEventParams_t *pxWriteParam)
{
	static ECC_Verify_t verify_token;		/* hold signature of token and digest of token */
	static ECC_Pubkey_t gateway_pubkey;		/* hold gateway public key */
	static ECC_Verify_t ECDSA_verify;		/* hold signature of ecdh public key and digest of ecdh pub key */
	static Hash_Data_t ECDH_key_digest;		/* hold ecdh key digest */
	static uint16_t recv_byte = 0;
	static uint16_t data_length = 0;
	static uint8_t *gateway_cert;
	static MUTUALAUTH_Edge_Data_t frame;

	int8_t crc_status;
	IotBleAttributeData_t xAttrData = { 0 };
	IotBleEventResponse_t xResp;
	xAttrData.handle = CHAR_HANDLE( &xGattDemoService, egattGatewayStatus );
	xAttrData.uuid = CHAR_UUID( &xGattDemoService, egattGatewayStatus );
	xResp.pAttrData = &xAttrData;
	xResp.attrDataOffset = 0;
	xResp.eventStatus = eBTStatusSuccess;
	xResp.rspErrorStatus = eBTRspErrorNone;
	optiga_lib_status_t return_status = 0;

	if (xSemaphoreTake(mutex, portMAX_DELAY ))
	{
		switch(Cur_param)
		{
		case DEVICE_CERTIFICATE:	/* Receive gateway certificate */

			if(recv_byte == 0)
			{
				ByteArrayToShort(pxWriteParam->pValue,0,&frame.Length);
				MUTUALAUTH_DEBUG("--DEVICE_CERTIFICATE length1 =%d\r\n",frame.Length);
				frame.pPayload = pvPortMalloc(frame.Length);
				gateway_cert = pvPortMalloc(frame.Length-4);
				data_length = frame.Length;
			}
			if(frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--DEVICE_CERTIFICATE length2 =%d\r\n",frame.Length);
				memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
				recv_byte = recv_byte + pxWriteParam->length;
				frame.Length = frame.Length - pxWriteParam->length;

				break;
			}
			MUTUALAUTH_DEBUG("--DEVICE_CERTIFICATE length3 =%d\r\n",frame.Length);


			memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
			crc_status = parse_and_validate_frame(frame.pPayload);
			if (crc_status == CRC_VERIFIED_PASS)
			{
				MUTUALAUTH_LOG_MESSAGE("RECEIVED: APP/GATEWAY CERTIFICATE");
				MUTUALAUTH_DEBUG("--DEVICE_CERTIFICATE CRC matched\r\n");
				memcpy(gateway_cert,frame.pPayload+2,data_length-4);
#ifdef CONFIG_MQTT_DEMO_ENABLED
				return_status = pal_crypt_verify_certificate(ca_certificate, sizeof(ca_certificate), gateway_cert, data_length-4);
				if(OPTIGA_LIB_SUCCESS != return_status)
				{
					IotLogError("Error:certificate verification failed. (status=0x%x)\r\r\n", return_status);
					break;
				}
				else
				{
					MUTUALAUTH_LOG_MESSAGE("RECEIVED: APP/GATEWAY CERTIFICATE verified");
				}
#endif
				gw_status = (return_status == OPTIGA_LIB_SUCCESS) ? Certificate_Verify_Pass : Certificate_Verify_Fail;
				if( xNotifyGatewayStatus == pdTRUE )
				{
					xAttrData.pData = ( uint8_t * ) &gw_status;
					xAttrData.size = sizeof( gw_status );
					( void ) IotBle_SendIndication( &xResp, usBLEConnectionID, false );
				}
				recv_byte = 0;
			}
			else
			{
				IotLogError("Error DEVICE_CERTIFICATE:CRC not match\r\n");
				gw_status = Certificate_Verify_Fail;
				if( xNotifyGatewayStatus == pdTRUE )
				{
					xAttrData.pData = ( uint8_t * ) &gw_status;
					xAttrData.size = sizeof( gw_status );
					( void ) IotBle_SendIndication( &xResp, usBLEConnectionID, false );
				}
				recv_byte = 0;
			}
			break;

		case DEVICE_PUBKEY:			/* Receive gateway public key */

			if(recv_byte == 0)
			{
				ByteArrayToShort(pxWriteParam->pValue,0,&frame.Length);
				frame.pPayload = pvPortMalloc(frame.Length);
				data_length = frame.Length;
				MUTUALAUTH_DEBUG("--DEVICE_PUBKEY length1 =%d\r\n",frame.Length);
			}
			if(frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--DEVICE_PUBKEY length2 =%d\r\n",frame.Length);
				memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
				recv_byte = recv_byte + pxWriteParam->length;
				frame.Length = frame.Length - pxWriteParam->length;

				break;
			}
			MUTUALAUTH_DEBUG("--DEVICE_PUBKEY length3 =%d\r\n",frame.Length);
			MUTUALAUTH_DEBUG("--DEVICE_PUBKEY pxWriteParam->length =%d\r\n",pxWriteParam->length);

			memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
			crc_status = parse_and_validate_frame(frame.pPayload);
			if (crc_status == CRC_VERIFIED_PASS)
			{
				MUTUALAUTH_LOG_MESSAGE("RECEIVED: APP/GATEWAY PUBKEY");
				MUTUALAUTH_DEBUG("--DEVICE_PUBKEY CRC matched\r\n");
				memcpy(gateway_pubkey.public_key,ecc_pubkey_header,PUBLIC_KEY_HEADER_SIZE);
				memcpy(gateway_pubkey.public_key + PUBLIC_KEY_HEADER_SIZE,frame.pPayload + 2,data_length-4);

				recv_byte = 0;
			}
			else
			{
				IotLogError("Error:CRC not match\r\n");
				recv_byte = 0;
			}
			break;

		case TOKEN:					/* Receive token/random number from gateway and create hash*/

			if(recv_byte == 0)
			{
				ByteArrayToShort(pxWriteParam->pValue,0,&frame.Length);
				frame.pPayload = pvPortMalloc(frame.Length);
				data_length = frame.Length;
				MUTUALAUTH_DEBUG("--TOKEN length1 =%d\r\n",frame.Length);
			}
			if(frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--TOKEN length2 =%d\r\n",frame.Length);

				memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
				recv_byte = recv_byte + pxWriteParam->length;
				frame.Length = frame.Length - pxWriteParam->length;

				break;
			}
			MUTUALAUTH_DEBUG("--TOKEN length3 =%d\r\n",frame.Length);

			memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
			crc_status = parse_and_validate_frame(frame.pPayload);
			if (crc_status == CRC_VERIFIED_PASS)
			{
				MUTUALAUTH_LOG_MESSAGE("RECEIVED: APP/GATEWAY RANDOM NUMBER");
				MUTUALAUTH_DEBUG("--TOKEN CRC matched\r\n");
				memcpy(gateway_token.random_data_buffer,frame.pPayload+2,data_length-4);
				recv_byte = 0;
				/* Create hash sha256 of the gateway token */
				MUTUALAUTH_LOG_MESSAGE("CREATE HASH OF THE APP/GATEWAY RANDOM NUMBER");
				gwtoken_digest.data_to_hash = gateway_token.random_data_buffer;
				gwtoken_digest.Length = sizeof(gateway_token.random_data_buffer);
				return_status = mutualauth_optiga_crypt_hash(&gwtoken_digest);
				if (OPTIGA_LIB_SUCCESS != return_status)
				{
					MUTUALAUTH_DEBUG("Failed to create sha256 hash of the GW token\r\n");
				}
			}
			else
			{
				IotLogError("Error: TOKEN CRC not match\r\n");
				recv_byte = 0;
			}
			break;

		case SIGN_TOKEN:			/* Verify received sign token and send verification status */

			if(recv_byte == 0)
			{
				ByteArrayToShort(pxWriteParam->pValue,0,&frame.Length);
				frame.pPayload = pvPortMalloc(frame.Length);
				data_length = frame.Length;
				MUTUALAUTH_DEBUG("--SIGN_TOKEN length1 =%d\r\n",frame.Length);
			}
			if(frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--SIGN_TOKEN length2 =%d\r\n",frame.Length);

				memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
				recv_byte = recv_byte + pxWriteParam->length;
				frame.Length = frame.Length - pxWriteParam->length;

				break;
			}
			MUTUALAUTH_DEBUG("--SIGN_TOKEN length3 =%d\r\n",frame.Length);

			memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
			crc_status = parse_and_validate_frame(frame.pPayload);
			if (crc_status == CRC_VERIFIED_PASS)
			{
				MUTUALAUTH_LOG_MESSAGE("RECEIVED: SIGN RANDOM NUMBER");
				MUTUALAUTH_DEBUG("--SIGN_TOKEN CRC matched\r\n");
				memcpy(verify_token.signature,frame.pPayload+2,data_length-4);
				memcpy(verify_token.digest,devicetoken_digest.digest,sizeof(devicetoken_digest.digest));
				memcpy(verify_token.public_key,gateway_pubkey.public_key,sizeof(gateway_pubkey.public_key));

				verify_token.sig_length = data_length-4;
				MUTUALAUTH_LOG_MESSAGE("VERIFY RECEIVED SIGN RANDOM NUMBER");
				return_status = mutualauth_optiga_crypt_ecdsa_verify(&verify_token);
				gw_status = (return_status == OPTIGA_LIB_SUCCESS) ? Sign_Token_Verify_Pass : Sign_Token_Verify_Fail;
				if( xNotifyGatewayStatus == pdTRUE )
				{
					xAttrData.pData = ( uint8_t * ) &gw_status;
					xAttrData.size = sizeof( gw_status );
					( void ) IotBle_SendIndication( &xResp, usBLEConnectionID, false );
				}
				recv_byte = 0;
			}
			else
			{
				IotLogError("Error: SIGN_TOKEN CRC not match\r\n");
				gw_status = Sign_Token_Verify_Fail;
				if( xNotifyGatewayStatus == pdTRUE )
				{
					xAttrData.pData = ( uint8_t * ) &gw_status;
					xAttrData.size = sizeof( gw_status );
					( void ) IotBle_SendIndication( &xResp, usBLEConnectionID, false );
				}
				recv_byte = 0;
			}
			break;

		case ECDH_PUBKEY:			/* Received ECDH public key and create hash */

			if(recv_byte == 0)
			{
				MUTUALAUTH_DEBUG("--ECDH_PUBKEY length1 =%d\r\n",frame.Length);
				ByteArrayToShort(pxWriteParam->pValue,0,&frame.Length);
				frame.pPayload = pvPortMalloc(frame.Length);
				data_length = frame.Length;
			}
			if(frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--ECDH_PUBKEY length2 =%d\r\n",frame.Length);

				memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
				recv_byte = recv_byte + pxWriteParam->length;
				frame.Length = frame.Length - pxWriteParam->length;

				break;
			}
			MUTUALAUTH_DEBUG("--ECDH_PUBKEY length3 =%d\r\n",frame.Length);

			memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
			crc_status = parse_and_validate_frame(frame.pPayload);
			if (crc_status == CRC_VERIFIED_PASS)
			{
				MUTUALAUTH_LOG_MESSAGE("RECEIVED: ECDH PUBLIC KEY");
				MUTUALAUTH_DEBUG("--ECDH_PUBKEY CRC matched\r\n");

				memcpy(ECDH_pubkey.public_key,ecc_pubkey_header,PUBLIC_KEY_HEADER_SIZE);
				memcpy(ECDH_pubkey.public_key + PUBLIC_KEY_HEADER_SIZE,frame.pPayload+2,data_length-4);
				recv_byte = 0;
				MUTUALAUTH_LOG_MESSAGE("CREATE HASH OF THE ECDH PUBLIC KEY");
				/* Create hash sha256 of the ecdh public key */
				ECDH_key_digest.data_to_hash = ECDH_pubkey.public_key + PUBLIC_KEY_HEADER_SIZE;
				ECDH_key_digest.Length = sizeof(ECDH_pubkey.public_key) - PUBLIC_KEY_HEADER_SIZE;
				return_status = mutualauth_optiga_crypt_hash(&ECDH_key_digest);
				if (OPTIGA_LIB_SUCCESS != return_status)
				{
					MUTUALAUTH_DEBUG("Failed to create sha256 hash of the ECDH Pubkey\r\n");
				}
			}
			else
			{
				IotLogError("Error: ECDH_PUBKEY CRC not match\r\n");
				recv_byte = 0;
			}
			break;

		case ECDH_SIGN:				/* Verify received sign public key and send verification status */

			if(recv_byte == 0)
			{
				MUTUALAUTH_DEBUG("--ECDH_SIGN length1 =%d\r\n",frame.Length);

				ByteArrayToShort(pxWriteParam->pValue,0,&frame.Length);
				frame.pPayload = pvPortMalloc(frame.Length);
				data_length = frame.Length;
			}
			if(frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--ECDH_SIGN length2 =%d\r\n",frame.Length);

				memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
				recv_byte = recv_byte + pxWriteParam->length;
				frame.Length = frame.Length - pxWriteParam->length;

				break;
			}
			MUTUALAUTH_DEBUG("--ECDH_SIGN length3 =%d\r\n",frame.Length);

			memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
			crc_status = parse_and_validate_frame(frame.pPayload);
			if (crc_status == CRC_VERIFIED_PASS)
			{
				MUTUALAUTH_LOG_MESSAGE("RECEIVED: ECDH SIGN PUBLIC KEY");
				MUTUALAUTH_DEBUG("--ECDH_SIGN CRC matched\r\n");
				memcpy(ECDSA_verify.signature,frame.pPayload+2,data_length-4);
				memcpy(ECDSA_verify.digest,ECDH_key_digest.digest,sizeof(ECDH_key_digest.digest));
				memcpy(ECDSA_verify.public_key,gateway_pubkey.public_key,sizeof(gateway_pubkey.public_key));
				ECDSA_verify.sig_length = data_length-4;
				MUTUALAUTH_LOG_MESSAGE("VERIFY ECDH SIGN PUBLIC KEY");
				return_status = mutualauth_optiga_crypt_ecdsa_verify(&ECDSA_verify);
				gw_status = (return_status == OPTIGA_LIB_SUCCESS) ? ECDSA_Signature_Pass : ECDSA_Signature_Fail;
				if( xNotifyGatewayStatus == pdTRUE )
				{
					xAttrData.pData = ( uint8_t * ) &gw_status;
					xAttrData.size = sizeof( gw_status );
					( void ) IotBle_SendIndication( &xResp, usBLEConnectionID, false );
				}
				recv_byte = 0;
			}
			else
			{
				IotLogError("Error: ECDH_SIGN CRC not match\r\n");
				gw_status = ECDSA_Signature_Fail;
				if( xNotifyGatewayStatus == pdTRUE )
				{
					xAttrData.pData = ( uint8_t * ) &gw_status;
					xAttrData.size = sizeof( gw_status );
					( void ) IotBle_SendIndication( &xResp, usBLEConnectionID, false );
				}
				recv_byte = 0;
			}
			break;

		case ECDH_SECRET:

			if(recv_byte == 0)
			{
				ByteArrayToShort(pxWriteParam->pValue,0,&frame.Length);
				frame.pPayload = pvPortMalloc(frame.Length);
				data_length = frame.Length;
				MUTUALAUTH_DEBUG("--ECDH_SECRET length1 =%d\r\n",frame.Length);
			}
			if(frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--ECDH_SECRET length2 =%d\r\n",frame.Length);

				memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
				recv_byte = recv_byte + pxWriteParam->length;
				frame.Length = frame.Length - pxWriteParam->length;

				break;
			}
			MUTUALAUTH_DEBUG("--ECDH_SECRET length3 =%d\r\n",frame.Length);

			memcpy(frame.pPayload + recv_byte,pxWriteParam->pValue,pxWriteParam->length);
			crc_status = parse_and_validate_frame(frame.pPayload);
			if (crc_status == CRC_VERIFIED_PASS)
			{
				MUTUALAUTH_LOG_MESSAGE("RECEIVED : HASH OF THE SECRET");
				MUTUALAUTH_DEBUG("--ECDH_SECRET CRC matched\r\n");
				return_status = memcmp(frame.pPayload+2,hash_secret.digest,sizeof(hash_secret.digest));
				gw_status = (return_status == 0) ? ECDH_Secret_Match : ECDH_Secret_Not_Match;
				if( xNotifyGatewayStatus == pdTRUE )
				{
					xAttrData.pData = ( uint8_t * ) &gw_status;
					xAttrData.size = sizeof( gw_status );
					( void ) IotBle_SendIndication( &xResp, usBLEConnectionID, false );
				}
				recv_byte = 0;
			}
			else
			{
				IotLogError("Error: ECDH_SECRET  CRC not match\r\n");
				gw_status = ECDH_Secret_Not_Match;
				if( xNotifyGatewayStatus == pdTRUE )
				{
					xAttrData.pData = ( uint8_t * ) &gw_status;
					xAttrData.size = sizeof( gw_status );
					( void ) IotBle_SendIndication( &xResp, usBLEConnectionID, false );
				}
				recv_byte = 0;
			}
			break;

		default:
			MUTUALAUTH_LOG_MESSAGE("RECEIVED: INVALID PARAMETER");
			break; /* DEFAULT */
		}
		/* Free the memory allocated for payload */
		if (recv_byte == 0)
			vPortFree(frame.pPayload);
		xSemaphoreGive(mutex);
	}
	else
	{
		IotLogError("Error:can't take mutex \r\n");
	}

	return;
}

static void MUTUALAUTH_Send_Data(IotBleReadEventParams_t *pxReadParam)
{
	static ECC_Sign_t sign_token;
	static ECC_Pubkey_t ecdh_pubkey;
	static ECC_Pubkey_t device_pubkey;
	static ECC_Sign_t ecdsa_sign;
	static Hash_Data_t ecdh_key_digest;

	IotBleAttributeData_t xAttrData = { 0 };
	IotBleEventResponse_t xResp;
	uint16_t i;
	optiga_lib_status_t return_status = 0;
	static MUTUALAUTH_Edge_Data_t frame;
	static uint16_t send_byte = 0;
	static uint8_t chip_cert[LENGTH_OPTIGA_CERT];
	static uint16_t chip_cert_size = LENGTH_OPTIGA_CERT;
	uint16_t chip_cert_oid;
	xAttrData.handle = CHAR_HANDLE( &xGattDemoService, egattEdgeParamValue );
	xAttrData.uuid = CHAR_UUID( &xGattDemoService, egattEdgeParamValue );
	xResp.pAttrData = &xAttrData;
	xResp.attrDataOffset = 0;
	xResp.eventStatus = eBTStatusSuccess;
	xResp.rspErrorStatus = eBTRspErrorNone;

	if (xSemaphoreTake(mutex, portMAX_DELAY ))
	{
		switch(Cur_param)
		{
		case DEVICE_CERTIFICATE:		/* Send edge device certificate */

			if (send_byte == 0)
			{
				chip_cert_oid = 0xE0E0;
				// Read security chip certificate
				return_status = read_device_cert(chip_cert_oid, chip_cert, &chip_cert_size);
				if(OPTIGA_LIB_SUCCESS != return_status)
				{
					IotLogError("Error: read chip cert failed (status=0x%x).\r\n", return_status);
					break;
				}

				frame.Length = chip_cert_size + 4;
				frame.pPayload = pvPortMalloc(frame.Length);
				MUTUALAUTH_DEBUG("--DEVICE_CERTIFICATE 1 length =%d\r\n",frame.Length);
				create_payload_frame(&frame,chip_cert,frame.Length-4);
			}
			if (frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--DEVICE_CERTIFICATE 2 length =%d\r\n",frame.Length);
				xResp.pAttrData->pData = frame.pPayload + send_byte;
				xResp.pAttrData->size = 20;

				IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
				frame.Length = frame.Length - 20;
				send_byte = send_byte + 20;
				break;
			}
			MUTUALAUTH_DEBUG("--DEVICE_CERTIFICATE 3 length =%d\r\n",frame.Length);
			xResp.pAttrData->pData = frame.pPayload + send_byte;
			xResp.pAttrData->size = frame.Length;

			MUTUALAUTH_LOG_MESSAGE("SEND: DEVICE CERTIFICATE");
			IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
			send_byte = 0;
			break;

		case DEVICE_PUBKEY:				/* Send edge device public key */

			if (send_byte == 0)
			{
				if(chip_cert[0] == 0)
				{
					chip_cert_oid = 0xE0E0;
					// Read security chip certificate
					return_status = read_device_cert(chip_cert_oid, chip_cert, &chip_cert_size);
					if(OPTIGA_LIB_SUCCESS != return_status)
					{
						IotLogError("Error: read chip cert failed (status=0x%x).\r\n", return_status);
						break;
					}
				}
				// Extract Public Key from the certificate
				device_pubkey.Length = sizeof(device_pubkey.public_key);
				return_status = pal_crypt_get_public_key(chip_cert, chip_cert_size, device_pubkey.public_key, &device_pubkey.Length);
				if(OPTIGA_LIB_SUCCESS != return_status)
				{
					IotLogError("Error: extract public key failed.");
					break;
				}


				/* from certi */
				frame.Length = device_pubkey.Length + 4;
				frame.pPayload = pvPortMalloc(frame.Length);
				MUTUALAUTH_DEBUG("--DEVICE_PUBKEY 1 length =%d\r\n",frame.Length);
				create_payload_frame(&frame,device_pubkey.public_key ,frame.Length-4);
			}
			if (frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--DEVICE_PUBKEY 2 length =%d\r\n",frame.Length);
				xResp.pAttrData->pData = frame.pPayload + send_byte;
				xResp.pAttrData->size = 20;

				IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
				frame.Length = frame.Length - 20;
				send_byte = send_byte + 20;
				break;
			}
			MUTUALAUTH_DEBUG("--DEVICE_PUBKEY 3 length =%d\r\n",frame.Length);
			xResp.pAttrData->pData = frame.pPayload + send_byte;
			xResp.pAttrData->size = frame.Length;


			MUTUALAUTH_LOG_MESSAGE("SEND: DEVICE PUBKEY\r\n");
			IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
			send_byte = 0;
			break;

		case TOKEN:						/* Send token/random number */

			if (send_byte == 0)
			{
				MUTUALAUTH_LOG_MESSAGE("GENERATE RANDOM NUMBER");
				return_status = mutualauth_optiga_crypt_random(&device_token);
				if (OPTIGA_LIB_SUCCESS != return_status)
				{
					IotLogError("Failed to generate Random number\r\n");
				}

				frame.Length = sizeof(device_token.random_data_buffer) + 4;
				frame.pPayload = pvPortMalloc(frame.Length);
				MUTUALAUTH_DEBUG("--TOKEN 1 length =%d\r\n",frame.Length);
				create_payload_frame(&frame,device_token.random_data_buffer,frame.Length-4);

				/* create hash sha256 of the token */
				devicetoken_digest.data_to_hash = device_token.random_data_buffer;
				devicetoken_digest.Length = sizeof(device_token.random_data_buffer);
				MUTUALAUTH_LOG_MESSAGE("CREATE HASH OF THE RANDOM NUMBER");
				return_status = mutualauth_optiga_crypt_hash(&devicetoken_digest);
				if (OPTIGA_LIB_SUCCESS != return_status)
				{
					IotLogError("Failed to create sha256 hash of the device token\r\n");
				}
			}
			if (frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--TOKEN 2 length =%d\r\n",frame.Length);
				xResp.pAttrData->pData = frame.pPayload + send_byte;
				xResp.pAttrData->size = 20;

				IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
				frame.Length = frame.Length - 20;
				send_byte = send_byte + 20;
				break;
			}

			MUTUALAUTH_DEBUG("--TOKEN 3 length =%d\r\n",frame.Length);
			xResp.pAttrData->pData = frame.pPayload + send_byte;
			xResp.pAttrData->size = frame.Length;

			MUTUALAUTH_LOG_MESSAGE("SEND: RANDOM NUMBER");
			IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
			send_byte = 0;

			break;

		case SIGN_TOKEN:				/* send sign token */

			if (send_byte == 0)
			{
				sign_token.optiga_key_id = OPTIGA_KEY_ID_E0F0;
				memcpy(sign_token.digest,gwtoken_digest.digest,sizeof(gwtoken_digest.digest));
				sign_token.signature_length = sizeof(sign_token.signature);
				MUTUALAUTH_LOG_MESSAGE("CREATE SIGNATURE OF THE RANDOM NUMBER");
				return_status = mutualauth_optiga_crypt_ecdsa_sign(&sign_token);
				if (OPTIGA_LIB_SUCCESS != return_status)
				{
					IotLogError("Failed to create Signature of the Gateway token\r\n");
				}
				frame.Length = sign_token.signature_length + 4;
				frame.pPayload = pvPortMalloc(frame.Length);
				MUTUALAUTH_DEBUG("--SIGN_TOKEN 1 length =%d\r\n",frame.Length);
				create_payload_frame(&frame,sign_token.signature,frame.Length-4);
			}
			if (frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--SIGN_TOKEN 2 length =%d\r\n",frame.Length);
				xResp.pAttrData->pData = frame.pPayload + send_byte;
				xResp.pAttrData->size = 20;

				IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
				frame.Length = frame.Length - 20;
				send_byte = send_byte + 20;
				break;
			}

			MUTUALAUTH_DEBUG("--SIGN_TOKEN 3 length =%d\r\n",frame.Length);
			xResp.pAttrData->pData = frame.pPayload + send_byte;
			xResp.pAttrData->size = frame.Length;

			MUTUALAUTH_LOG_MESSAGE("SEND: SIGN RANDOM NUMBER");
			IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
			send_byte = 0;
			break;

		case ECDH_PUBKEY:				/* Send ECDH Public key */

			if (send_byte == 0)
			{
				ecdh_pubkey.key_usage = OPTIGA_KEY_USAGE_KEY_AGREEMENT;
				ecdh_pubkey.optiga_key_id = OPTIGA_KEY_ID_E0F2;
				ecdh_pubkey.Length = sizeof(ecdh_pubkey.public_key);
				MUTUALAUTH_LOG_MESSAGE("CREATE ECDH KEY PAIR");
				return_status = mutualauth_optiga_crypt_ecc_generate_keypair(&ecdh_pubkey);
				if (OPTIGA_LIB_SUCCESS != return_status)
				{
					IotLogError("Failed to Generate ECC keypair\r\n");
				}

				frame.Length = ecdh_pubkey.Length - PUBLIC_KEY_HEADER_SIZE + 4;
				frame.pPayload = pvPortMalloc(frame.Length);
				MUTUALAUTH_DEBUG("--ECDH_PUBKEY 1 length =%d\r\n",frame.Length);
				create_payload_frame(&frame,ecdh_pubkey.public_key + PUBLIC_KEY_HEADER_SIZE,frame.Length-4);

				/* Create hash sha256 of the ecdh public key */
				ecdh_key_digest.data_to_hash = ecdh_pubkey.public_key + PUBLIC_KEY_HEADER_SIZE;
				ecdh_key_digest.Length = ecdh_pubkey.Length - PUBLIC_KEY_HEADER_SIZE;
				MUTUALAUTH_LOG_MESSAGE("CREATE HASH OF THE ECDH PUBLICKEY");
				return_status = mutualauth_optiga_crypt_hash(&ecdh_key_digest);
				if (OPTIGA_LIB_SUCCESS != return_status)
				{
					IotLogError("Failed to create sha256 hash of ECC public key\r\n");
				}
			}
			if (frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--ECDH_PUBKEY 2 length =%d\r\n",frame.Length);
				xResp.pAttrData->pData = frame.pPayload + send_byte;
				xResp.pAttrData->size = 20;

				IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
				frame.Length = frame.Length - 20;
				send_byte = send_byte + 20;
				break;
			}

			MUTUALAUTH_DEBUG("--ECDH_PUBKEY 3 length =%d\r\n",frame.Length);
			xResp.pAttrData->pData = frame.pPayload + send_byte;
			xResp.pAttrData->size = frame.Length;

			MUTUALAUTH_LOG_MESSAGE("SEND: ECDH PUBLIC KEY");
			IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
			send_byte = 0;
			break;

		case ECDH_SIGN:					/* Send sign ecdh public key */

			if (send_byte == 0)
			{
				ecdsa_sign.optiga_key_id = OPTIGA_KEY_ID_E0F0;
				memcpy(ecdsa_sign.digest,ecdh_key_digest.digest,sizeof(ecdh_key_digest.digest));
				ecdsa_sign.signature_length = sizeof(ecdsa_sign.signature);
				MUTUALAUTH_LOG_MESSAGE("CREATE ECDH SIGNATURE OF THE ECDH PUBLIC KEY");
				return_status = mutualauth_optiga_crypt_ecdsa_sign(&ecdsa_sign);
				if (OPTIGA_LIB_SUCCESS != return_status)
				{
					MUTUALAUTH_DEBUG("Failed to create Signature of ECC public key\r\n");
				}

				frame.Length = ecdsa_sign.signature_length + 4;
				frame.pPayload = pvPortMalloc(frame.Length);
				MUTUALAUTH_DEBUG("--ECDH_SIGN 1 length =%d\r\n",frame.Length);
				create_payload_frame(&frame,ecdsa_sign.signature,frame.Length-4);
			}
			if (frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--ECDH_SIGN 2 length =%d\r\n",frame.Length);
				xResp.pAttrData->pData = frame.pPayload + send_byte;
				xResp.pAttrData->size = 20;

				IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
				frame.Length = frame.Length - 20;
				send_byte = send_byte + 20;
				break;
			}

			MUTUALAUTH_DEBUG("--ECDH_SIGN 3 length =%d\r\n",frame.Length);
			xResp.pAttrData->pData = frame.pPayload + send_byte;
			xResp.pAttrData->size = frame.Length;


			MUTUALAUTH_LOG_MESSAGE("SEND: ECDH SIGN PUBLIC KEY");
			IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
			send_byte = 0;

			MUTUALAUTH_LOG_MESSAGE("GENERATE ECDH SECRET");
			memcpy(secret.peer_public_key,ECDH_pubkey.public_key,sizeof(ECDH_pubkey.public_key));
			secret.key_type = OPTIGA_ECC_CURVE_NIST_P_256;
			secret.optiga_key_id = OPTIGA_KEY_ID_E0F2;/*TODO need to check */
			return_status = mutualauth_optiga_crypt_ecdh(&secret);
			if (OPTIGA_LIB_SUCCESS != return_status)
			{
				IotLogError("Failed to create ECDH secret\r\n");
			}
			/* Create hash sha256 of the secret */
			hash_secret.data_to_hash = secret.shared_secret;
			hash_secret.Length = sizeof(secret.shared_secret);
			MUTUALAUTH_LOG_MESSAGE("CREATE HASH OF THE ECDH SECRET");
			return_status = mutualauth_optiga_crypt_hash(&hash_secret);
			if (OPTIGA_LIB_SUCCESS != return_status)
			{
				IotLogError("Failed to create sha256 hash of secret\r\n");
			}
			for(i=0; i<sizeof(secret.shared_secret);i++)
				MUTUALAUTH_DEBUG("ECDH_SECRET[%d]=%x\r\n",i,secret.shared_secret[i]);
			break;

		case ECDH_SECRET:
			if (send_byte == 0)
			{
				frame.Length = sizeof(hash_secret.digest) + 4;
				frame.pPayload = pvPortMalloc(frame.Length);
				MUTUALAUTH_DEBUG("--ECDH_SECRET_HASH 1 length =%d\r\n",frame.Length);
				create_payload_frame(&frame,hash_secret.digest,frame.Length-4);
			}
			if (frame.Length > 20)
			{
				MUTUALAUTH_DEBUG("--ECDH_SECRET_HASH 2 length =%d\r\n",frame.Length);
				xResp.pAttrData->pData = frame.pPayload + send_byte;
				xResp.pAttrData->size = 20;

				IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
				frame.Length = frame.Length - 20;
				send_byte = send_byte + 20;
				break;
			}

			MUTUALAUTH_DEBUG("--ECDH_SECRET_HASH 3 length =%d\r\n",frame.Length);
			xResp.pAttrData->pData = frame.pPayload + send_byte;
			xResp.pAttrData->size = frame.Length;

			MUTUALAUTH_LOG_MESSAGE("SEND: HASH OF THE SECRET");
			IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
			send_byte = 0;

			break;
		default:
			MUTUALAUTH_LOG_MESSAGE("SEND: INVALID PARAMETER");
			break; /* DEFAULT */
		}
		/* Free the memory allocated for payload */
		if (send_byte == 0)
			vPortFree(frame.pPayload);
		xSemaphoreGive(mutex);
	}
	else
	{
		IotLogError("Error:can't take mutex \r\n");
	}

	return;
}

/*-----------------------------------------------------------*/
int vGattDemoSvcInit( bool awsIotMqttMode,
                      const char * pIdentifier,
                      void * pNetworkServerInfo,
                      void * pNetworkCredentialInfo,
                      const IotNetworkInterface_t * pNetworkInterface )
{
    int status = EXIT_SUCCESS;


    while( 1 )
    {
        vTaskDelay( 1000 );
    }

    return status;
}
#define LENGTH_ENDPOINT 50
#define ENDPOINT_OID 0xF1D0
#define OTA_OID 0xE0E8

extern uint8_t Endpoint[LENGTH_ENDPOINT];
extern uint16_t EndpointSize;
extern volatile uint8_t Get_Endpoint;
char signingcredentialSIGNING_CERTIFICATE_PEM[OTA_CERT_SIZE]={0};
static BaseType_t vGattDemoSvcHook( void )
{
    BaseType_t xRet = pdFALSE;
    BTStatus_t xStatus;
    IotBleEventsCallbacks_t xCallback;
    uint16_t oid = ENDPOINT_OID;
    uint16_t ota_oid = OTA_OID;
    optiga_lib_status_t status;
    uint8_t *postfix="amazonaws.com";
    uint8_t *ret;
    uint16_t otaSize = sizeof(ota_cert);

    /* Initialize Optiga turstM */
    //optiga_shell_init();
#ifdef CONFIG_OTA_UPDATE_DEMO_ENABLED
#if OTA_CERT_STORE
    status = mutualauth_optiga_write(ota_oid,ota_cert,otaSize);
    if (OPTIGA_LIB_SUCCESS != status)
    {
		IotLogError("Error: Failed to Write Code signing certificate\n");
    }
    else
    {
		IotLogInfo("OTA Cert write successfully Done \n");
		IotLogInfo("Please Disable the \"OTA_CERT_STORE\" Macro and Flash again\n");
		return status;

    }
#endif

    status = mutualauth_optiga_read(ota_oid,signingcredentialSIGNING_CERTIFICATE_PEM,&otaSize);
    if (OPTIGA_LIB_SUCCESS != status)
    {
		IotLogError("Error: Failed to Read Code signing certificate\n");
    }
    else
    {
       IotLogInfo("Certificate Read Successfully from the Optiga TrustM\n");
    }
#endif
    status = mutualauth_optiga_read(oid,Endpoint,&EndpointSize);
    if (OPTIGA_LIB_SUCCESS != status)
    {
    	IotLogError("Error: Failed to Read Endpoint\n");
    }
    IotLogInfo("Read Endpoint = %s\r\n",Endpoint);

    ret = strstr((char *)Endpoint,(char *)postfix);
    if(ret == NULL || BUTTON_PUSHED())
    {
    	Get_Endpoint = 1;
    	configPRINT_STRING("========== Please Enter Your Endpoint URL ==========\r\n");
    	configPRINT_STRING("e.g- xxxxxxxxxxx-ats.iot.xx-xxx-x.amazonaws.com\r\n");
    	while(Get_Endpoint);

    		configPRINT_STRING("\r\nEntered Endpoint URL=");
    		configPRINT_STRING((char *)Endpoint);
        	configPRINT_STRING("\r\n===================================================\r\n");
        	configPRINT_STRING("\r\n---Write Endpoint in TrustM---\r\n");

    		status = mutualauth_optiga_write(oid,Endpoint,EndpointSize);
    		if (OPTIGA_LIB_SUCCESS != status)
    		{
    			IotLogError("Error: Failed to Write Endpoint\n");
    		}
    }
    configPRINT_STRING("\n");
    MUTUALAUTH_LOG_MESSAGE("=====   BLE MUTUAL AUTHENTICATION   =====");

    /* Select the handle buffer. */
    xStatus = IotBle_CreateService( ( BTService_t * ) &xGattDemoService, ( IotBleAttributeEventCallback_t * ) pxCallBackArray );

    mutex = xSemaphoreCreateMutex();
    if( xStatus == eBTStatusSuccess )
    {
        xRet = pdTRUE;
    }

    if( xRet == pdTRUE )
    {
        xCallback.pConnectionCb = _connectionCallback;

        if( IotBle_RegisterEventCb( eBLEConnection, xCallback ) != eBTStatusSuccess )
        {
            xRet = pdFAIL;
        }
    }

    return xRet;

}

/*-----------------------------------------------------------*/

void vParam( IotBleAttributeEvent_t * pEventParam )
{
    IotBleWriteEventParams_t * pxWriteParam;
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;

    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( ( pEventParam->xEventType == eBLEWrite ) || ( pEventParam->xEventType == eBLEWriteNoResponse ) )
    {
        pxWriteParam = pEventParam->pParamWrite;
        xResp.pAttrData->handle = pxWriteParam->attrHandle;

        if( pxWriteParam->length == 1 )	/* Check length if it is 1 byte then assigned it to cur_param */
        {
        	Cur_param = pxWriteParam->pValue[ 0 ];
        	MUTUALAUTH_DEBUG("Cur_param=%d",Cur_param );
            xResp.eventStatus = eBTStatusSuccess;
        }

        if( pEventParam->xEventType == eBLEWrite )
        {
            xResp.pAttrData->pData = pxWriteParam->pValue;
            xResp.attrDataOffset = pxWriteParam->offset;
            xResp.pAttrData->size = pxWriteParam->length;
            IotBle_SendResponse( &xResp, pxWriteParam->connId, pxWriteParam->transId );
        }
    }
}

/*-----------------------------------------------------------*/

void vGWParamValue( IotBleAttributeEvent_t * pEventParam )
{
    IotBleWriteEventParams_t * pxWriteParam;
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;
    MUTUALAUTH_DEBUG( "*********vGWParamValue************\r\n" );
    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( ( pEventParam->xEventType == eBLEWrite ) || ( pEventParam->xEventType == eBLEWriteNoResponse ) )
    {
    	pxWriteParam = pEventParam->pParamWrite;
    	xResp.pAttrData->handle = pxWriteParam->attrHandle;

    	MUTUALAUTH_Receive_Data(pxWriteParam);

    	xResp.eventStatus = eBTStatusSuccess;

    	if( pEventParam->xEventType == eBLEWrite )
    	{
    		xResp.pAttrData->pData = pxWriteParam->pValue;
    		xResp.attrDataOffset = pxWriteParam->offset;
    		xResp.pAttrData->size = pxWriteParam->length;
    		IotBle_SendResponse( &xResp, pxWriteParam->connId, pxWriteParam->transId );
    	}
    }
}

/*-----------------------------------------------------------*/

void vEnableNotification( IotBleAttributeEvent_t * pEventParam )
{
    IotBleWriteEventParams_t * pxWriteParam;
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;
    uint16_t ucCCFGValue;


    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( ( pEventParam->xEventType == eBLEWrite ) || ( pEventParam->xEventType == eBLEWriteNoResponse ) )
    {
        pxWriteParam = pEventParam->pParamWrite;

        xResp.pAttrData->handle = pxWriteParam->attrHandle;

        if( pxWriteParam->length == 2 )
        {
            ucCCFGValue = ( pxWriteParam->pValue[ 1 ] << 8 ) | pxWriteParam->pValue[ 0 ];

            if( ucCCFGValue == ( uint16_t ) ENABLE_NOTIFICATION )
            {
                MUTUALAUTH_LOG_MESSAGE( "Enabled Notification for Read Characteristic" );
                xNotifyGatewayStatus = pdTRUE;
                usBLEConnectionID = pxWriteParam->connId;
            }
            else if( ucCCFGValue == 0 )
            {
                xNotifyGatewayStatus = pdFALSE;
            }

            xResp.eventStatus = eBTStatusSuccess;
        }

        if( pEventParam->xEventType == eBLEWrite )
        {
            xResp.pAttrData->pData = pxWriteParam->pValue;
            xResp.pAttrData->size = pxWriteParam->length;
            xResp.attrDataOffset = pxWriteParam->offset;

            IotBle_SendResponse( &xResp, pxWriteParam->connId, pxWriteParam->transId );
        }
    }
}
void vGatewayStatus( IotBleAttributeEvent_t * pEventParam )
{
	MUTUALAUTH_LOG_MESSAGE( "SEND APP/GATEWAY STATUS" );
    IotBleReadEventParams_t * pxReadParam;
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;

    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( pEventParam->xEventType == eBLERead )
    {
        pxReadParam = pEventParam->pParamRead;
        xResp.pAttrData->handle = pxReadParam->attrHandle;
        xResp.pAttrData->pData = ( uint8_t * ) &gw_status;
        xResp.pAttrData->size = sizeof( gw_status );
        xResp.attrDataOffset = 0;
        xResp.eventStatus = eBTStatusSuccess;
        IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
    }
}
void vEdgeParamValue( IotBleAttributeEvent_t * pEventParam )
{
	MUTUALAUTH_DEBUG( "*********vEdgeParamValue************\r\n" );
    IotBleReadEventParams_t * pxReadParam;
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;

    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( pEventParam->xEventType == eBLERead )
    {
        pxReadParam = pEventParam->pParamRead;
        xResp.pAttrData->handle = pxReadParam->attrHandle;
        MUTUALAUTH_Send_Data(pxReadParam);
    }
}
void vEdgeStatus( IotBleAttributeEvent_t * pEventParam )
{
	MUTUALAUTH_DEBUG( "*********vEdgeStatus************\r\n" );
	IotBleWriteEventParams_t * pxWriteParam;
	IotBleAttributeData_t xAttrData = { 0 };
	IotBleEventResponse_t xResp;
	static uint8_t edge_status=0;				/* store edge verification status */

	xResp.pAttrData = &xAttrData;
	xResp.rspErrorStatus = eBTRspErrorNone;
	xResp.eventStatus = eBTStatusFail;
	xResp.attrDataOffset = 0;
	pxWriteParam = pEventParam->pParamWrite;
	xResp.pAttrData->handle = pxWriteParam->attrHandle;
	if( ( pEventParam->xEventType == eBLEWrite ) || ( pEventParam->xEventType == eBLEWriteNoResponse ) )
	{
		if( pxWriteParam->length == 1 )
		{
			edge_status = pxWriteParam->pValue[ 0 ];
			printf( "DEVICE STATUS=%d",edge_status );
			xResp.eventStatus = eBTStatusSuccess;
		}

		if( pEventParam->xEventType == eBLEWrite )
		{
			xResp.pAttrData->pData = pxWriteParam->pValue;
			xResp.pAttrData->size = pxWriteParam->length;
			xResp.attrDataOffset = pxWriteParam->offset;

			IotBle_SendResponse( &xResp, pxWriteParam->connId, pxWriteParam->transId );
		}
	}
}
/*-----------------------------------------------------------*/

static void _connectionCallback( BTStatus_t xStatus,
                                 uint16_t connId,
                                 bool bConnected,
                                 BTBdaddr_t * pxRemoteBdAddr )
{
    if( ( xStatus == eBTStatusSuccess ) && ( bConnected == false ) )
    {
        if( connId == usBLEConnectionID )
        {
            MUTUALAUTH_LOG_MESSAGE( " Disconnected from BLE device.\r\n" );
        }
    }
}
