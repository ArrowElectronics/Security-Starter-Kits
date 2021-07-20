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
 * @file aws_ble_gatt_server_demo.h
 * @brief Sample demo for a BLE GATT server
 */
#ifndef AWS_BLE_GATT_SERVER_DEMO_H_
#define AWS_BLE_GATT_SERVER_DEMO_H_

#include "FreeRTOS.h"
/* The config header is always included first. */
#include "iot_config.h"
#include "platform/iot_network.h"
//#define MUTUALAUTH_DEBUG_ENABLE
#define LENGTH_OPTIGA_CERT 512

/**
 * @brief  GATT service, characteristics and descriptor UUIDs used by the sample.
 */
#define gattMutualauthSVC_UUID            { 0x00, 0xFF, 0x69, 0xD6, 0xC6, 0xBF, 0x14, 0x90, 0x25, 0x41, 0xE7, 0x49, 0xE3, 0xD9, 0xF2, 0xC6 }
#define gattDemoCHAR_UUID_MASK           0x69, 0xD6, 0xC6, 0xBF, 0x14, 0x90, 0x25, 0x41, 0xE7, 0x49, 0xE3, 0xD9, 0xF2, 0xC6
#define gattCHAR_PARAM_UUID       	 	 { 0x01, 0xFF, gattDemoCHAR_UUID_MASK }
#define gattCHAR_GW_PARAM_VALUE_UUID     { 0x02, 0xFF, gattDemoCHAR_UUID_MASK }
#define gattCHAR_GATEWAY_STATUS_UUID     { 0x03, 0xFF, gattDemoCHAR_UUID_MASK }
#define gattCHAR_EDGE_PARAM_VALUE_UUID   { 0x04, 0xFF, gattDemoCHAR_UUID_MASK }
#define gattCHAR_EDGE_STATUS_UUID     	 { 0x05, 0xFF, gattDemoCHAR_UUID_MASK }
#define gattDemoCLIENT_CHAR_CFG_UUID     ( 0x2902 )

/**
 * @brief Enable Notification and Enable Indication values as defined by GATT spec.
 */
#define ENABLE_NOTIFICATION              ( 0x01 )
#define ENABLE_INDICATION                ( 0x02 )

/**
 * @brief Number of characteristics, descriptors and included services for GATT sample service.
 */
#define gattDemoNUM_CHARS                ( 2 )
#define gattDemoNUM_CHAR_DESCRS          ( 1 )
#define gattDemoNUM_INCLUDED_SERVICES    ( 0 )

/**
 * @brief Veification status macro.
 */
#define Certificate_Verify_Pass			0x01
#define Certificate_Verify_Fail			0x02
#define Sign_Token_Verify_Pass			0x03
#define Sign_Token_Verify_Fail			0x04
#define ECDSA_Signature_Pass			0x05
#define ECDSA_Signature_Fail			0x06
#define ECDH_Secret_Match				0x07
#define ECDH_Secret_Not_Match			0x08
#define Pub_Key_Ext_Pass				0x09 /* public key extract pass*/
#define Pub_Key_Ext_Fail				0x0A /* public key extract fail */
#define	Un_expected_state				0x0B /* Un expected state received */
#define	Token_CRC_Match_Fail			0x0C /* Token crc mis-match */
#define	Create_SHA256_Fail				0x0D /* Create SHA256 Fail */
#define	Create_sha256_EDCH_Fail			0x0E /* Create sha256 EDCH fail */
#define	Sha256_EDCH_Match_Fail			0x0F /* Sha256 EDCH fail */



// Logger levels
#define IOT_EXAMPLE                     "[Mutual Auth]  : "

#ifdef MUTUALAUTH_DEBUG_ENABLE
#define MUTUALAUTH_DEBUG( ... ) IotLog( IOT_LOG_INFO, NULL, __VA_ARGS__ )
#else
#define MUTUALAUTH_DEBUG( ... )
#endif
#define MUTUALAUTH_LOG_MESSAGE(msg) \
{\
	uint8_t buffer[400]; \
    sprintf((char *)buffer,"%s%s\r\n",IOT_EXAMPLE,msg); \
    configPRINT_STRING((char *)buffer); \
}
/**
 * @brief Characteristics used by the GATT sample service.
 */
typedef enum
{
    egattDemoService = 0,      /**< SGatt demo service. */
    egattParam,
    egattGatewayParamValue,
	egattGatewayStatus,
	egattGatewayStatusDescr,
	egattEdgeParamValue,
	egattEdgeStatus,
    egattDemoNbAttributes
} eGattDemoAttributes_t;


/**
 * @brief Creates and starts GATT demo service.
 *
 * @return pdTRUE if the GATT Service is successfully initialized, pdFALSE otherwise
 */
int vGattDemoSvcInit( bool awsIotMqttMode,
                      const char * pIdentifier,
                      void * pNetworkServerInfo,
                      void * pNetworkCredentialInfo,
                      const IotNetworkInterface_t * pNetworkInterface );


#endif /* AWS_BLE_GATT_SERVER_DEMO_H_ */
