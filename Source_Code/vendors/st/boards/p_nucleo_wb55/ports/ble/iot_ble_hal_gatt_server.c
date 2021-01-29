/*
 * Amazon FreeRTOS
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

/**
 * @file aws_ble_hal_gatt_server.c
 * @brief Hardware Abstraction Layer for GATT server ble stack.
 */

#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "iot_ble_hal_common.h"

#include "bt_hal_manager_adapter_ble.h"
#include "bt_hal_manager.h"
#include "bt_hal_gatt_server.h"
#include "iot_ble_hal_internals.h"
#include "iot_doubly_linked_list.h"

extern uint16_t connection_handle;
BTBdaddr_t btadd;

/* All characteristics have default maximum length 36 bytes */
#define DEFAULT_CHARACTERISTIC_LEN  36
/* All descriptors have default maximum length 36 bytes */
#define DEFAULT_DESCRIPTOR_LEN      60
/* MQTT characteristics for read and write are MTU size */
#define RW_CHARACTERISTIC_LEN       CFG_BLE_MAX_ATT_MTU
/* Long characteristic has 512 bytes for passing tests */
#define LONG_CHARACTERISTIC_LEN     512

typedef struct{
	uint16_t Conn_Handle_To_Notify;
	uint16_t Service_Handle;
	uint16_t Char_Handle;
	uint8_t Update_Type;
	uint16_t Char_Length;
	uint16_t Value_Offset;
	uint8_t Value_Length;
    uint8_t Value[RW_CHARACTERISTIC_LEN];
}indicateValue_t;
indicateValue_t indicateValue;

typedef struct{
    uint16_t ExpectedLen;
    uint16_t CurrentLen;
    bool bPrepareWrite;
}prepareWriteState_t;

indicateValue_t * queueBuffer;
#define INDICATE_QUEUE_LENGTH       (1)
#define INDICATE_QUEUE_ITEM_SIZE    sizeof( indicateValue_t *)
#define INDICATE_QUEUE_TIMEOUT      (1000)


enum{
	WRITE_RESPONSE,
    READ_RESPONSE,
    NO_RESPONSE
};

typedef enum{
	ATTR_TYPE_SERVICE,
	ATTR_TYPE_CHAR,
	ATTR_TYPE_CHAR_DECL,
	ATTR_TYPE_DESCR,
}attributeTypes;

typedef struct{
	uint16_t usHandle;
	uint16_t usAttributeType;
	uint16_t Char_Handle; /* if it is a descriptor */
    prepareWriteState_t xPrepareWrite;
}AttributeElement_t;

typedef struct{
	Link_t xServiceList;
	AttributeElement_t * xAttributes;
	size_t xNbElements;
	uint16_t usLastCharHandle;
	uint16_t usStartHandle;
	uint16_t usEndHandle;
}serviceListElement_t;

Link_t xServiceListHead;
static StaticSemaphore_t xThreadSafetyMutex;
static StaticQueue_t xIndicateQueue;

static BTGattServerCallbacks_t xGattServerCb;

uint32_t ulGattServerIFhandle;

/*
 * For test
 * write 512 bytes execute write from RPI
 */
static char bufferWriteResponse[LONG_CHARACTERISTIC_LEN];

BTStatus_t prvCopytoBlueNRGCharUUID(Char_UUID_t * pxBlueNRGCharuuid, BTUuid_t * pxUuid);
BTStatus_t prvCopytoBlueNRGDescrUUID(Char_Desc_Uuid_t * pxBlueNRGDescruuid, BTUuid_t * pxUuid);
BTStatus_t prvCopytoBlueNRGServUUID(Service_UUID_t * pxBlueNRGServuuid, BTUuid_t * pxUuid);

static BTStatus_t prvBTRegisterServer(BTUuid_t * pxUuid);
static BTStatus_t prvBTUnregisterServer(uint8_t ucServerIf);
static BTStatus_t prvBTGattServerInit(const BTGattServerCallbacks_t * pxCallbacks);
static BTStatus_t prvBTConnect(uint8_t ucServerIf, const BTBdaddr_t * pxBdAddr, bool bIsDirect, BTTransport_t xTransport);
static BTStatus_t prvBTDisconnect(uint8_t ucServerIf, const BTBdaddr_t * pxBdAddr, uint16_t usConnId);
static BTStatus_t prvBTAddService(uint8_t ucServerIf, BTGattSrvcId_t * pxSrvcId, uint16_t usNumHandles);
static BTStatus_t prvBTAddIncludedService(uint8_t ucServerIf, uint16_t usServiceHandle, uint16_t usIncludedHandle);
static BTStatus_t prvBTAddCharacteristic(uint8_t ucServerIf, uint16_t usServiceHandle, BTUuid_t * pxUuid, BTCharProperties_t xProperties, BTCharPermissions_t xPermissions);
static BTStatus_t prvBTSetVal(BTGattResponse_t * pxValue);
static BTStatus_t prvBTAddDescriptor(uint8_t ucServerIf, uint16_t usServiceHandle, BTUuid_t * pxUuid, BTCharPermissions_t xPermissions);
static BTStatus_t prvBTStartService(uint8_t ucServerIf, uint16_t usServiceHandle, BTTransport_t xTransport);
static BTStatus_t prvBTStopService(uint8_t ucServerIf, uint16_t usServiceHandle);
static BTStatus_t prvBTDeleteService(uint8_t ucServerIf, uint16_t usServiceHandle);
static BTStatus_t prvBTSendIndication(uint8_t ucServerIf, uint16_t usAttributeHandle, uint16_t usConnId, size_t xLen, uint8_t * pucValue, bool bConfirm);
static BTStatus_t prvBTSendResponse(uint16_t usConnId, uint32_t ulTransId, BTStatus_t xStatus, BTGattResponse_t * pxResponse);
static BTStatus_t prvAddServiceBlob( uint8_t ucServerIf, BTService_t * pxService);
static void AppTick( void * pvParameters );

static BTGattServerInterface_t xGATTserverInterface =
{
		.pxRegisterServer = prvBTRegisterServer,
		.pxUnregisterServer = prvBTUnregisterServer,
		.pxGattServerInit = prvBTGattServerInit,
		.pxConnect = prvBTConnect,
		.pxDisconnect = prvBTDisconnect,
		.pxAddService = prvBTAddService,
		.pxAddIncludedService = prvBTAddIncludedService,
		.pxAddCharacteristic = prvBTAddCharacteristic,
		.pxSetVal = prvBTSetVal,
		.pxAddDescriptor = prvBTAddDescriptor,
		.pxStartService = prvBTStartService,
		.pxStopService = prvBTStopService,
		.pxDeleteService = prvBTDeleteService,
		.pxSendIndication = prvBTSendIndication,
        .pxSendResponse = prvBTSendResponse,
        .pxAddServiceBlob = prvAddServiceBlob
};

serviceListElement_t * prvGetService(uint16_t usHandle)
{
    Link_t * pxTmpElem;
    serviceListElement_t * pxServiceElem = NULL;

    /* The service that was just added is the last in the list */
	if( xSemaphoreTake( ( SemaphoreHandle_t ) &xThreadSafetyMutex, portMAX_DELAY ) == pdPASS )
	{
	    listFOR_EACH( pxTmpElem, &xServiceListHead )
	    {
	        pxServiceElem = listCONTAINER( pxTmpElem, serviceListElement_t, xServiceList );

	        if(( usHandle >= pxServiceElem->usStartHandle )&&(usHandle <= pxServiceElem->usEndHandle ))
	        {
	            break;
	        }

	        pxServiceElem = NULL;
	    }
		xSemaphoreGive( ( SemaphoreHandle_t ) &xThreadSafetyMutex );
	}

    return pxServiceElem;
}

BTStatus_t prvCopytoBlueNRGCharUUID(Char_UUID_t * pxBlueNRGCharuuid, BTUuid_t * pxUuid) {
    BTStatus_t ret = eBTStatusSuccess;
	switch (pxUuid->ucType) {
	case eBTuuidType16:
		pxBlueNRGCharuuid->Char_UUID_16 = pxUuid->uu.uu16;
        ret = eBTStatusSuccess;
		break;
    case eBTuuidType32:
        ret = eBTStatusUnsupported;
        break;
	case eBTuuidType128:
        Osal_MemCpy(&pxBlueNRGCharuuid->Char_UUID_128, pxUuid->uu.uu128, 16);
        ret = eBTStatusSuccess;
        break;
	default:
        ret = eBTStatusParamInvalid;
		break;
	}
    return ret;
}

BTStatus_t prvCopytoBlueNRGDescrUUID(Char_Desc_Uuid_t * pxBlueNRGDescruuid, BTUuid_t * pxUuid) {
    BTStatus_t ret = eBTStatusSuccess;
	switch (pxUuid->ucType) {
	case eBTuuidType16:
		pxBlueNRGDescruuid->Char_UUID_16 = pxUuid->uu.uu16;
        ret = eBTStatusSuccess;
		break;
    case eBTuuidType32:
        ret = eBTStatusUnsupported;
        break;
	case eBTuuidType128:	
		Osal_MemCpy(&pxBlueNRGDescruuid->Char_UUID_128, pxUuid->uu.uu128, 16);
        ret = eBTStatusSuccess;
		break;
    default:
        ret = eBTStatusParamInvalid;
        break;
	}
    return ret;
}

BTStatus_t prvCopytoBlueNRGServUUID(Service_UUID_t * pxBlueNRGServuuid, BTUuid_t * pxUuid) {
    BTStatus_t ret = eBTStatusSuccess;
	switch (pxUuid->ucType) {
	case eBTuuidType16:
		pxBlueNRGServuuid->Service_UUID_16 = pxUuid->uu.uu16;
        ret = eBTStatusSuccess;
		break;
    case eBTuuidType32:
        ret = eBTStatusUnsupported;
        break;
	case eBTuuidType128:	
		Osal_MemCpy(&pxBlueNRGServuuid->Service_UUID_128, pxUuid->uu.uu128, 16);
        ret = eBTStatusSuccess;
		break;
    default:
        ret = eBTStatusParamInvalid;
        break;
	}
    return ret;
}

void hci_le_connection_complete_event(uint8_t Status, uint16_t Connection_Handle, uint8_t Role, uint8_t Peer_Address_Type, uint8_t Peer_Address[6], uint16_t Conn_Interval, uint16_t Conn_Latency, uint16_t Supervision_Timeout, uint8_t Master_Clock_Accuracy) {
    connection_handle = Connection_Handle;
    Osal_MemCpy(&btadd.ucAddress, Peer_Address, 6);
    if (xGattServerCb.pxConnectionCb != NULL) {
		xGattServerCb.pxConnectionCb(Connection_Handle, 0, true, &btadd);
	}
}


void hci_disconnection_complete_event(uint8_t Status, uint16_t Connection_Handle, uint8_t Reason) {
    uint8_t cleanAddr[6] = { 0, 0, 0, 0, 0, 0 };
    if (xGattServerCb.pxConnectionCb != NULL) {
		xGattServerCb.pxConnectionCb(Connection_Handle, 0, false, &btadd);		
	}    
    Osal_MemCpy(&btadd.ucAddress, cleanAddr, 6);
}

void aci_gatt_read_permit_req_event(uint16_t Connection_Handle,
                                    uint16_t Attribute_Handle,
                                    uint16_t Offset)
{
    if(xGattServerCb.pxRequestReadCb != NULL)
    {
        xGattServerCb.pxRequestReadCb(Connection_Handle, READ_RESPONSE, &btadd, Attribute_Handle, Offset);
    }
}

void aci_gatt_write_permit_req_event(uint16_t Connection_Handle,
                                     uint16_t Attribute_Handle,
                                     uint8_t Data_Length,
                                     uint8_t Data[])
{
	memcpy(bufferWriteResponse, Data, Data_Length);
    if(xGattServerCb.pxRequestWriteCb != NULL)
    {
        xGattServerCb.pxRequestWriteCb(Connection_Handle, WRITE_RESPONSE, &btadd, Attribute_Handle, 0, Data_Length, true, false, Data);
    }
}

void aci_gatt_prepare_write_permit_req_event(uint16_t Connection_Handle,
                                             uint16_t Attribute_Handle,
                                             uint16_t Offset,
                                             uint8_t Data_Length,
                                             uint8_t Data[])
{
    uint16_t offset;
    serviceListElement_t * xServiceElement;
    xServiceElement = prvGetService(Attribute_Handle);
    offset = Attribute_Handle - xServiceElement->usStartHandle;
    uint16_t usHandle = xServiceElement->xAttributes[offset].usHandle;

    if(usHandle == Attribute_Handle) {
        xServiceElement->xAttributes[offset].xPrepareWrite.bPrepareWrite = true;
        memcpy(bufferWriteResponse, Data, Data_Length);
    }

    if(xGattServerCb.pxRequestWriteCb != NULL)
    {
        xGattServerCb.pxRequestWriteCb(Connection_Handle, WRITE_RESPONSE, &btadd, Attribute_Handle, Offset, Data_Length, true, true, Data);
    }

}


void aci_gatt_attribute_modified_event(uint16_t Connection_Handle,
                                       uint16_t Attribute_Handle,
                                       uint16_t Offset,
                                       uint16_t Data_Length,
                                       uint8_t Data[])
{
    uint16_t offset;
    serviceListElement_t * xServiceElement;
    xServiceElement = prvGetService(Attribute_Handle);
    offset = Attribute_Handle - xServiceElement->usStartHandle;
    uint16_t usHandle = xServiceElement->xAttributes[offset].usHandle;

    if( xServiceElement->xAttributes[offset].xPrepareWrite.bPrepareWrite && (usHandle == Attribute_Handle) )
    {
        /*
         * Bit 15: if the entire value of the attribute does not fit inside a single ACI_GATT_ATTRIBUTE_MODIFIED_EVENT event,
         * this bit is set to 1 to notify that other ACI_GATT_ATTRIBUTE_MODIFIED_EVENT
         * events will follow to report the remaining value.
         */
        if( (xGattServerCb.pxRequestExecWriteCb != NULL) && (~Offset & (1 << 15)))
        {
            xServiceElement->xAttributes[offset].xPrepareWrite.bPrepareWrite = false;
            xGattServerCb.pxRequestExecWriteCb( Connection_Handle,
                                                           Attribute_Handle,
                                                           &btadd,
                                                           true );
        }
    }
    else
    {
        memcpy(bufferWriteResponse, Data, Data_Length);
        if(xGattServerCb.pxRequestWriteCb != NULL)
        {
            xGattServerCb.pxRequestWriteCb(Connection_Handle, NO_RESPONSE, &btadd, Attribute_Handle, Offset, Data_Length, false, false, Data);
        }
    }    
}

void aci_gatt_server_confirmation_event(uint16_t Connection_Handle)
{
//    if (xGattServerCb.pxIndicationSentCb != NULL)
//    {
//        xGattServerCb.pxIndicationSentCb( Connection_Handle, eBTStatusSuccess);
//    }
}

void aci_att_exchange_mtu_resp_event(uint16_t Connection_Handle, uint16_t Server_RX_MTU) {
    uint8_t ret = BLE_STATUS_SUCCESS;

    if ( (ret == BLE_STATUS_SUCCESS) && (xGattServerCb.pxMtuChangedCb != NULL) ) {
		xGattServerCb.pxMtuChangedCb(Connection_Handle, Server_RX_MTU);
	}

}

BTStatus_t prvBTRegisterServer(BTUuid_t * pxUuid) {
	BTStatus_t xStatus = eBTStatusSuccess;
    if(xGattServerCb.pxRegisterServerCb != NULL)
    {
        xGattServerCb.pxRegisterServerCb(xStatus, ulGattServerIFhandle, pxUuid);
    }
	return eBTStatusSuccess;
}

BTStatus_t prvBTUnregisterServer(uint8_t ucServerIf) {
    BTStatus_t xStatus = eBTStatusSuccess;

    if(xGattServerCb.pxUnregisterServerCb != NULL)
    {
        xGattServerCb.pxUnregisterServerCb(xStatus, ucServerIf);
    }
    return eBTStatusSuccess;
}

BTStatus_t prvBTGattServerInit(const BTGattServerCallbacks_t * pxCallbacks) {
	BTStatus_t xStatus = eBTStatusSuccess;
	if (xStatus == eBTStatusSuccess) {
		if (pxCallbacks != NULL) {
			xGattServerCb = *pxCallbacks;
			listINIT_HEAD( &xServiceListHead );
            ( void ) xSemaphoreCreateMutexStatic( &xThreadSafetyMutex );
            xQueueCreateStatic( INDICATE_QUEUE_LENGTH,
                                INDICATE_QUEUE_ITEM_SIZE,
                                (void *)&queueBuffer,
                                &xIndicateQueue );

            if( xTaskCreate( AppTick, "Notify", configMINIMAL_STACK_SIZE * 5, NULL, tskIDLE_PRIORITY + 1, NULL ) != pdPASS )
            {
                xStatus = eBTStatusFail;
            }

		} else {
			xStatus = eBTStatusParamInvalid;
		}
	}
	return xStatus;
}

BTStatus_t prvBTConnect(uint8_t ucServerIf, const BTBdaddr_t * pxBdAddr, bool bIsDirect, BTTransport_t xTransport) {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTDisconnect(uint8_t ucServerIf, const BTBdaddr_t * pxBdAddr, uint16_t usConnId) {
    // 0x13 - Remote User Terminated Connection
    tBleStatus xBleStatus = hci_disconnect(connection_handle, 0x13);
    HAL_Delay(100);
    return ((xBleStatus == BLE_STATUS_SUCCESS) ? eBTStatusSuccess : eBTStatusFail);
}

BTStatus_t prvBTAddService(uint8_t ucServerIf, BTGattSrvcId_t * pxSrvcId, uint16_t usNumHandles) {

	Service_UUID_t xSrvUUID;
	uint8_t xSrvType = PRIMARY_SERVICE;
	uint8_t xSrv_UUID_Type = UUID_TYPE_128;
	uint16_t xServHandle = 0;
	uint8_t xBleStatus = BLE_STATUS_SUCCESS;
	serviceListElement_t * pxNewServiceElement;
	BTStatus_t xStatus = eBTStatusSuccess;

	if ((pxSrvcId->xServiceType) == eBTServiceTypeSecondary) {
		xSrvType = SECONDARY_SERVICE;
	}
	if (pxSrvcId->xId.xUuid.ucType == eBTuuidType16) {
		xSrv_UUID_Type = UUID_TYPE_16;
	}

    xStatus = prvCopytoBlueNRGServUUID(&xSrvUUID, &pxSrvcId->xId.xUuid);
    if(xStatus == eBTStatusSuccess)
    {
        xBleStatus = aci_gatt_add_service(xSrv_UUID_Type, &xSrvUUID, xSrvType, usNumHandles, &xServHandle);

        if(xBleStatus == BLE_STATUS_SUCCESS)
        {
            pxNewServiceElement = pvPortMalloc( sizeof( serviceListElement_t ) );

            if( pxNewServiceElement != NULL )
            {
                pxNewServiceElement->usStartHandle = xServHandle;
                pxNewServiceElement->usEndHandle = xServHandle;
                pxNewServiceElement->xAttributes = pvPortMalloc( sizeof( AttributeElement_t )*usNumHandles );
                pxNewServiceElement->xNbElements = usNumHandles;
                pxNewServiceElement->usLastCharHandle = 0;

                if(pxNewServiceElement->xAttributes != NULL)
                {
                    if( xSemaphoreTake( ( SemaphoreHandle_t ) &xThreadSafetyMutex, portMAX_DELAY ) == pdPASS )
                    {
                        listADD( &xServiceListHead,  &pxNewServiceElement->xServiceList );
                        xSemaphoreGive( ( SemaphoreHandle_t ) &xThreadSafetyMutex );
                    }
                }else
                {
                    xStatus = eBTStatusFail;
                }
            }
            else
            {
                xStatus = eBTStatusFail;
            }
        }else
        {
            xStatus = eBTStatusFail;
        }
    }

    if(xGattServerCb.pxServiceAddedCb != NULL)
    {
        xGattServerCb.pxServiceAddedCb((xBleStatus == BLE_STATUS_SUCCESS) ? eBTStatusSuccess : eBTStatusFail, ucServerIf, pxSrvcId, xServHandle);
    }

	return xStatus;
}

BTStatus_t prvBTAddIncludedService(uint8_t ucServerIf, uint16_t usServiceHandle, uint16_t usIncludedHandle) {
    BTStatus_t xStatus = eBTStatusUnsupported;
	return xStatus;
}

BTStatus_t prvConvertPermtoSTPerm(BTCharPermissions_t xPermissions, uint8_t * Security_Permissions )
{
    BTStatus_t xStatus = eBTStatusSuccess;
    *Security_Permissions = ATTR_PERMISSION_NONE;

	if(( xPermissions & eBTPermReadEncrypted) != 0)
	{
		*Security_Permissions |= ATTR_PERMISSION_ENCRY_READ;
	}

	if(( xPermissions & eBTPermReadEncryptedMitm) != 0)
	{
		*Security_Permissions |= ATTR_PERMISSION_AUTHEN_READ;
        *Security_Permissions |= ATTR_PERMISSION_ENCRY_READ;
	}

    if(( xPermissions & eBTPermWriteEncrypted) != 0)
	{
		*Security_Permissions |= ATTR_PERMISSION_ENCRY_WRITE;
	}

	if(( xPermissions & eBTPermWriteEncryptedMitm) != 0)
	{
		*Security_Permissions |= ATTR_PERMISSION_AUTHEN_WRITE;
        *Security_Permissions |= ATTR_PERMISSION_ENCRY_WRITE;
    }

	if(( xPermissions & eBTPermWriteSigned) != 0)
	{
		xStatus = eBTStatusUnsupported;
	}

	if(( xPermissions & eBTPermWriteSignedMitm) != 0)
	{
		xStatus = eBTStatusUnsupported;
	}

	return xStatus;
}

BTStatus_t prvBTAddCharacteristic(uint8_t ucServerIf, uint16_t usServiceHandle, BTUuid_t * pxUuid, BTCharProperties_t xProperties, BTCharPermissions_t xPermissions) {

	Char_UUID_t xCharUUID;
	uint8_t xChar_UUID_Type = UUID_TYPE_128;
	uint8_t xBleStatus = BLE_STATUS_SUCCESS;
    uint16_t xCharHandle = 0;
    uint16_t Char_Value_Length = DEFAULT_CHARACTERISTIC_LEN;
	serviceListElement_t * xServiceElement;
	uint16_t usAttributeIndex;
    BTStatus_t xStatus = eBTStatusSuccess;
	uint8_t Security_Permissions;
    uint8_t GATT_Evt_Mask = GATT_DONT_NOTIFY_EVENTS;

    /* According to property we enable appropriate event */
    if(xProperties & eBTPropRead)
    {
        GATT_Evt_Mask |= GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP;
    }
    if(xProperties & eBTPropWrite)
    {
        GATT_Evt_Mask |= GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP;
    }
    if(xProperties & eBTPropWriteNoResponse)
    {
        GATT_Evt_Mask |= GATT_NOTIFY_ATTRIBUTE_WRITE;
    }

	if (pxUuid->ucType == eBTuuidType16) {
		xChar_UUID_Type = UUID_TYPE_16;
	}

    xStatus = prvCopytoBlueNRGCharUUID(&xCharUUID, pxUuid);
    if(xStatus == eBTStatusSuccess)
    {
        xStatus = prvConvertPermtoSTPerm(xPermissions, &Security_Permissions);
        if(xStatus == eBTStatusSuccess)
        {
            /* Change read/write characteristics length of data transfer service */
            if((pxUuid->uu.uu128[1] == 0x00)&&(pxUuid->uu.uu128[2] == 0xC3))
            {
                switch(pxUuid->uu.uu128[0])
                {
                    case 0x02:
                    case 0x03:
                    case 0x04:
                    case 0x05:
                        Char_Value_Length = RW_CHARACTERISTIC_LEN;
                        break;
                    default:
                        break;
                }
            }
            /* Change cloud endpoint characteristic length of device information service */
            if((pxUuid->uu.uu128[0] == 0x02)&&(pxUuid->uu.uu128[1] == 0xFF)&&(pxUuid->uu.uu128[2] == 0x32))
            {
                Char_Value_Length = RW_CHARACTERISTIC_LEN;
            }
            /* For test write 512 bytes execute write from RPI */
            if((pxUuid->uu.uu128[0] == 0x02)&&(pxUuid->uu.uu128[1] == 0x00)&&(pxUuid->uu.uu128[2] == 0x32))
            {
                Char_Value_Length = LONG_CHARACTERISTIC_LEN;
            }

            xBleStatus = aci_gatt_add_char(usServiceHandle,
                                           xChar_UUID_Type,
                                           &xCharUUID,
                                           Char_Value_Length,
                                           (uint8_t) xProperties,
                                           (uint8_t) Security_Permissions,
                                           GATT_Evt_Mask,
                                           16,
                                           CHAR_VALUE_LEN_VARIABLE,
                                           &xCharHandle);
            /* The handle of the char declaration is returned, the char value is after. */
            xCharHandle++;

            if(xBleStatus == BLE_STATUS_SUCCESS)
            {
                xServiceElement = prvGetService(usServiceHandle);

                if(xServiceElement == NULL)
                {
                    xStatus = eBTStatusFail;
                }
            }else
            {
                xStatus = eBTStatusFail;
            }
        }
    }

	if(xStatus == eBTStatusSuccess)
	{
		if( xSemaphoreTake( ( SemaphoreHandle_t ) &xThreadSafetyMutex, portMAX_DELAY ) == pdPASS )
		{
			usAttributeIndex = xServiceElement->usEndHandle - xServiceElement->usStartHandle + 1;

			if(usAttributeIndex < xServiceElement->xNbElements)
			{
				xServiceElement->xAttributes[usAttributeIndex].usAttributeType = ATTR_TYPE_CHAR_DECL;
				xServiceElement->xAttributes[usAttributeIndex].usHandle = xCharHandle - 1;

				xServiceElement->xAttributes[usAttributeIndex + 1].usAttributeType = ATTR_TYPE_CHAR;
				xServiceElement->xAttributes[usAttributeIndex + 1].usHandle = xCharHandle;
				xServiceElement->usEndHandle += 2;

				xServiceElement->usLastCharHandle = xCharHandle;
			}else
			{
				xStatus = eBTStatusFail;
			}

			xSemaphoreGive( ( SemaphoreHandle_t ) &xThreadSafetyMutex );
		}else
		{
			xStatus = eBTStatusFail;
		}
	}else
	{
		xStatus = eBTStatusFail;
	}

    if(xGattServerCb.pxCharacteristicAddedCb != NULL)
    {
        xGattServerCb.pxCharacteristicAddedCb((xBleStatus == BLE_STATUS_SUCCESS) ? eBTStatusSuccess : eBTStatusFail, ucServerIf, pxUuid, usServiceHandle, xCharHandle);
    }

	return xStatus;
}

BTStatus_t prvBTSetVal(BTGattResponse_t * pxValue) {
    tBleStatus xBleStatus = 0;
    BTStatus_t xStatus = eBTStatusSuccess;
    uint16_t usServiceHandle;
    serviceListElement_t * xServiceElement;

    xServiceElement = prvGetService(pxValue->xAttrValue.usHandle);
    usServiceHandle = xServiceElement->usStartHandle;

    xBleStatus = aci_gatt_update_char_value(usServiceHandle,
                                            pxValue->usHandle,
                                            pxValue->xAttrValue.usOffset,
                                            pxValue->xAttrValue.xLen,
                                            pxValue->xAttrValue.pucValue);

    if(xBleStatus == BLE_STATUS_SUCCESS) {
        xStatus = eBTStatusSuccess;
    } else {
        xStatus = eBTStatusFail;
    }

    return xStatus;
}

BTStatus_t prvBTAddDescriptor(uint8_t ucServerIf, uint16_t usServiceHandle, BTUuid_t * pxUuid, BTCharPermissions_t xPermissions) {

	uint8_t xBleStatus = BLE_STATUS_ERROR;
	BTStatus_t xStatus = eBTStatusSuccess;
	Char_Desc_Uuid_t xDescrUUID;
	uint16_t xDescrHandle = 0;
	uint8_t xDescr_UUID_Type = UUID_TYPE_128;
    uint8_t xChar_Desc_Value_Max_Len = DEFAULT_DESCRIPTOR_LEN;
    uint8_t xChar_Desc_Value_Length = DEFAULT_DESCRIPTOR_LEN;
	uint8_t xAccess_Permissions = ATTR_ACCESS_READ_WRITE | ATTR_ACCESS_WRITE_WITHOUT_RESPONSE | ATTR_ACCESS_SIGNED_WRITE_ALLOWED; // Default
	uint8_t xGATT_Evt_Mask = GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP | GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP | GATT_NOTIFY_ATTRIBUTE_WRITE;
	serviceListElement_t * xServiceElement;
	uint16_t usAttributeIndex;
	uint8_t Security_Permissions;

	if (pxUuid->ucType == eBTuuidType16) {
		xDescr_UUID_Type = UUID_TYPE_16;
	}

    xStatus = prvCopytoBlueNRGDescrUUID(&xDescrUUID, pxUuid);
	xServiceElement = prvGetService(usServiceHandle);

	if(xServiceElement == NULL)
	{
		xStatus = eBTStatusFail;
	}

	if(xStatus == eBTStatusSuccess)
	{
		xStatus = prvConvertPermtoSTPerm(xPermissions, &Security_Permissions);
	}

	if(xStatus == eBTStatusSuccess)
	{
		if( xSemaphoreTake( ( SemaphoreHandle_t ) &xThreadSafetyMutex, portMAX_DELAY ) == pdPASS )
		{
			usAttributeIndex = xServiceElement->usEndHandle - xServiceElement->usStartHandle + 1;
			if(usAttributeIndex < xServiceElement->xNbElements)
			{
				xServiceElement->xAttributes[usAttributeIndex].usAttributeType = ATTR_TYPE_DESCR;
                /* Check if it is a CCCD handler as for indications and notifications it is registered automatically and we don't need to add it */
				if (((pxUuid->ucType == eBTuuidType16) && (xDescrUUID.Char_UUID_16 == 0x2902))||
					((pxUuid->ucType == eBTuuidType128) && (((xDescrUUID.Char_UUID_128[2]<<8) +xDescrUUID.Char_UUID_128[3])  == 0x2902)))
				{
                    /* CCCD descriptor has number next to characteristic handle */
					xDescrHandle = xServiceElement->xAttributes[usAttributeIndex - 1].usHandle + 1;
					xServiceElement->xAttributes[usAttributeIndex].usHandle = xDescrHandle;
					xServiceElement->xAttributes[usAttributeIndex].Char_Handle = xServiceElement->usLastCharHandle;
                } else {
                    /* Non-sequential addition doesn't work in current versions of WPAN
                     * usLastCharHandle is last char value handle, so we decrement it and get characteristic handle
                    */
                    xBleStatus = aci_gatt_add_char_desc(usServiceHandle,
                                                        xServiceElement->usLastCharHandle-1,
                                                        xDescr_UUID_Type, &xDescrUUID,
                                                        xChar_Desc_Value_Max_Len,
                                                        xChar_Desc_Value_Length,
                                                        NULL,
                                                        (uint8_t) Security_Permissions,
                                                        xAccess_Permissions,
                                                        xGATT_Evt_Mask,
                                                        16,
                                                        CHAR_VALUE_LEN_VARIABLE,
                                                        &xDescrHandle);
					if (xBleStatus == BLE_STATUS_SUCCESS)
					{
						xServiceElement->xAttributes[usAttributeIndex].usHandle = xDescrHandle;
						xServiceElement->xAttributes[usAttributeIndex].Char_Handle = xServiceElement->usLastCharHandle;
					}
					else
						xStatus = eBTStatusFail;
				}

				xServiceElement->usEndHandle++;
			}else
			{
				xStatus = eBTStatusFail;
			}


			xSemaphoreGive( ( SemaphoreHandle_t ) &xThreadSafetyMutex );
		}

        if(xGattServerCb.pxDescriptorAddedCb != NULL)
        {
            xGattServerCb.pxDescriptorAddedCb(xStatus, ucServerIf, pxUuid, usServiceHandle, xDescrHandle);
        }
	}

	return xStatus;
}

BTStatus_t prvBTStartService(uint8_t ucServerIf, uint16_t usServiceHandle, BTTransport_t xTransport) {
    // No ACI GATT command to start service. When we create service it automatically starts
    if(xGattServerCb.pxServiceStartedCb != NULL)
    {
        xGattServerCb.pxServiceStartedCb(eBTStatusSuccess, ucServerIf, usServiceHandle);
    }
	return eBTStatusSuccess;
}

BTStatus_t prvBTStopService(uint8_t ucServerIf, uint16_t usServiceHandle) {
    // No ACI GATT command to stop service. When we delete service it automatically stops
    if(xGattServerCb.pxServiceStoppedCb != NULL)
    {
        xGattServerCb.pxServiceStoppedCb(eBTStatusSuccess, ucServerIf, usServiceHandle);
    }
	return eBTStatusSuccess;
}

BTStatus_t prvBTDeleteService(uint8_t ucServerIf, uint16_t usServiceHandle) {

	BTStatus_t xStatus = eBTStatusSuccess;
	uint8_t xBleStatus = BLE_STATUS_SUCCESS;
	serviceListElement_t * xServiceElement;

	xBleStatus = aci_gatt_del_service(usServiceHandle);

	if (xBleStatus != BLE_STATUS_SUCCESS) {
		xStatus = eBTStatusFail;
	}

	if( xStatus == eBTStatusSuccess)
	{
		xServiceElement = prvGetService(usServiceHandle);

		if(xServiceElement != NULL)
		{
		    if( xSemaphoreTake( ( SemaphoreHandle_t ) &xThreadSafetyMutex, portMAX_DELAY ) == pdPASS )
		    {
				listREMOVE( &xServiceElement->xServiceList );
				vPortFree( xServiceElement->xAttributes );
				vPortFree( xServiceElement );
				( void ) xSemaphoreGive( ( SemaphoreHandle_t ) &xThreadSafetyMutex );
		    }
		}else
		{
			xStatus = eBTStatusFail;
		}
	}

    if(xGattServerCb.pxServiceDeletedCb != NULL)
    {
        xGattServerCb.pxServiceDeletedCb(xStatus, ucServerIf, usServiceHandle);
    }

	return xStatus;
}

BTStatus_t prvAddServiceBlob( uint8_t ucServerIf,
                              BTService_t * pxService)
{
    (void) ucServerIf;
    (void) pxService;
    return eBTStatusUnsupported;
}

static void AppTick( void * pvParameters )
{
	indicateValue_t * pxIndicateValue;
    tBleStatus xBleStatus;
    (void) pvParameters;

    while(1)
    {

        if(xQueueReceive((QueueHandle_t )&xIndicateQueue, &pxIndicateValue, (TickType_t) 10) == pdTRUE)
        {
            if( xSemaphoreTake( ( SemaphoreHandle_t ) &xThreadSafetyMutex, portMAX_DELAY ) == pdPASS )
            {
                xBleStatus = aci_gatt_update_char_value_ext(pxIndicateValue->Conn_Handle_To_Notify,
                        pxIndicateValue->Service_Handle,
                        pxIndicateValue->Char_Handle,
                        pxIndicateValue->Update_Type,
                        pxIndicateValue->Char_Length,
                        pxIndicateValue->Value_Offset,
                        pxIndicateValue->Value_Length,
                        pxIndicateValue->Value);

                if (xGattServerCb.pxIndicationSentCb != NULL)
                {
                    xGattServerCb.pxIndicationSentCb( pxIndicateValue->Conn_Handle_To_Notify, (xBleStatus == BLE_STATUS_SUCCESS) ? eBTStatusSuccess : eBTStatusFail );
                }
                ( void ) xSemaphoreGive( ( SemaphoreHandle_t ) &xThreadSafetyMutex );
            }
        }
    }
}

BTStatus_t prvBTSendIndication(uint8_t ucServerIf, uint16_t usAttributeHandle, uint16_t usConnId, size_t xLen, uint8_t * pucValue, bool bConfirm) {

	uint8_t xBleStatus = BLE_STATUS_ERROR;
	uint8_t xUpdate_Type = 0x00;
	uint16_t usServiceHandle;
	serviceListElement_t * xServiceElement;

	xServiceElement = prvGetService(usAttributeHandle);
	usServiceHandle = xServiceElement->usStartHandle;

	if (bConfirm == true)
		xUpdate_Type = 0x02; // Indication
	else
		xUpdate_Type = 0x01; // Notification

    if( xSemaphoreTake( ( SemaphoreHandle_t ) &xThreadSafetyMutex, portMAX_DELAY ) == pdPASS )
    {
		indicateValue.Conn_Handle_To_Notify = usConnId;
		indicateValue.Service_Handle = usServiceHandle;
        /* Char handle is char value handle - 1 */
		indicateValue.Char_Handle = usAttributeHandle - 1;
		indicateValue.Update_Type = xUpdate_Type;
		indicateValue.Char_Length = xLen;
		indicateValue.Value_Offset = 0;
		indicateValue.Value_Length = xLen;
		memcpy(indicateValue.Value, pucValue, xLen);
		indicateValue_t * pxIndicateValue = &indicateValue;
        if( xQueueSend((QueueHandle_t )&xIndicateQueue, (void *)&pxIndicateValue, (TickType_t)INDICATE_QUEUE_TIMEOUT) != pdPASS )
        {
            xBleStatus = BLE_STATUS_ERROR;
        }
        else
        {
            xBleStatus  = BLE_STATUS_SUCCESS;
        }
    	( void ) xSemaphoreGive( ( SemaphoreHandle_t ) &xThreadSafetyMutex );
    }
	return (xBleStatus == BLE_STATUS_SUCCESS) ? eBTStatusSuccess : eBTStatusFail;
}


BTStatus_t prvBTSendResponse(uint16_t usConnId, uint32_t ulTransId, BTStatus_t xStatus, BTGattResponse_t * pxResponse) {
	tBleStatus xBleStatus = 0;
	BTStatus_t xReturnStatus = eBTStatusSuccess;
	uint16_t usServiceHandle;
	serviceListElement_t * xServiceElement;
	uint8_t xUpdate_Type = 0x00; // No notification or indication (local characteristic value update)
	uint16_t offset;

	xServiceElement = prvGetService(pxResponse->xAttrValue.usHandle);
	usServiceHandle = xServiceElement->usStartHandle;
	offset = pxResponse->xAttrValue.usHandle - xServiceElement->usStartHandle;
    if( xSemaphoreTake( ( SemaphoreHandle_t ) &xThreadSafetyMutex, portMAX_DELAY ) == pdPASS )
    {
		switch(ulTransId)
		{
			case WRITE_RESPONSE:
            {
                memcpy(pxResponse->xAttrValue.pucValue, bufferWriteResponse, pxResponse->xAttrValue.xLen);
                xBleStatus = aci_gatt_write_resp(usConnId, pxResponse->xAttrValue.usHandle, xStatus, pxResponse->xAttrValue.xRspErrorStatus, pxResponse->xAttrValue.xLen, pxResponse->xAttrValue.pucValue);
                if(BLE_STATUS_SUCCESS != xBleStatus)
				{
					configPRINTF(("ERROR WRITTING MESSAGE\n"));
				}
				break;
			}
			case READ_RESPONSE:
			{
				if(xServiceElement->xAttributes[offset].usAttributeType == ATTR_TYPE_CHAR)
				{
                    xBleStatus = aci_gatt_update_char_value_ext(usConnId, usServiceHandle, pxResponse->xAttrValue.usHandle - 1, xUpdate_Type, pxResponse->xAttrValue.xLen + pxResponse->xAttrValue.usOffset, pxResponse->xAttrValue.usOffset, pxResponse->xAttrValue.xLen, pxResponse->xAttrValue.pucValue);
                }else
				{
                    xBleStatus = aci_gatt_set_desc_value(usServiceHandle, xServiceElement->xAttributes[offset].Char_Handle - 1, pxResponse->xAttrValue.usHandle, pxResponse->xAttrValue.usOffset, pxResponse->xAttrValue.xLen, pxResponse->xAttrValue.pucValue);
				}

				if(BLE_STATUS_SUCCESS == xBleStatus)
				{
					xBleStatus = aci_gatt_allow_read(usConnId);
				}else
				{
			    	configPRINTF(("ERROR\n"));
				}
				break;
			}
            case NO_RESPONSE:
                xBleStatus = BLE_STATUS_SUCCESS;
                break;
			default:;
		}
		( void ) xSemaphoreGive( ( SemaphoreHandle_t ) &xThreadSafetyMutex );
    }

    if (xGattServerCb.pxResponseConfirmationCb != NULL)
    {
        xGattServerCb.pxResponseConfirmationCb( (xBleStatus == BLE_STATUS_SUCCESS) ? eBTStatusSuccess : eBTStatusFail,
                                                pxResponse->xAttrValue.usHandle);
    }
	return (xBleStatus == BLE_STATUS_SUCCESS) ? eBTStatusSuccess : eBTStatusFail;
}

const void * prvBTGetGattServerInterface() {
	return &xGATTserverInterface;
}
