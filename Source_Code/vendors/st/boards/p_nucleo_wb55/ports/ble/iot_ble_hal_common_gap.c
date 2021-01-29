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
 * @file aws_ble_hal_common_gap.c
 * @brief Hardware Abstraction Layer for GAP ble stack.
 */

#include <string.h>
#include "FreeRTOS.h"
#include "iot_ble_hal_common.h"
#include "cmsis_os.h"
#include "ble_defs.h"
#include "iot_ble_config.h"

#include "bt_hal_manager_adapter_ble.h"
#include "bt_hal_manager.h"
#include "bt_hal_gatt_server.h"
#include "iot_ble_hal_internals.h"

#include "mutualauth_crypto.h"

/* Global variables ----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t bdaddr[] = { 0x12, 0x34, 0x00, 0xE1, 0x80, 0x02 };

LocalName_t xLocalName;

uint8_t xAuthReq;
uint8_t xIocap;
BTDeviceType_t xBTdevType = eBTdeviceDevtypeBle;

uint16_t service_handle = 0;
uint16_t dev_name_char_handle = 0;
uint16_t appearance_char_handle = 0;
/* Handle of the current connection */
uint16_t connection_handle = 0;

/* default settings */
BTProperties_t xProperties =
{
		.xPropertyIO = eBTIODisplayYesNo,
        .ulMtu = CFG_BLE_MAX_ATT_MTU,
		.bSecureConnectionOnly = true,
		.bBondable = true
};

uint32_t ulGAPEvtMngHandle;
BTCallbacks_t xBTCallbacks;

BTStatus_t prvBTManagerInit(const BTCallbacks_t * pxCallbacks);
BTStatus_t prvBTEnable(uint8_t ucGuestMode);
BTStatus_t prvBTDisable(void);
BTStatus_t prvBTGetAllDeviceProperties(void);
BTStatus_t prvBTGetDeviceProperty(BTPropertyType_t xType);
BTStatus_t prvBTSetDeviceProperty(const BTProperty_t *pxProperty);
BTStatus_t prvBTGetAllRemoteDeviceProperties(BTBdaddr_t *pxRemoteAddr);
BTStatus_t prvBTGetRemoteDeviceProperty(BTBdaddr_t *pxRemoteAddr, BTPropertyType_t type);
BTStatus_t prvBTSetRemoteDeviceProperty(BTBdaddr_t *pxRemoteAddr, const BTProperty_t *property);
BTStatus_t prvBTPair(const BTBdaddr_t * pxBdAddr, BTTransport_t xTransport, bool bCreateBond);
BTStatus_t prvBTCreateBondOutOfBand(const BTBdaddr_t * pxBdAddr, BTTransport_t xTransport, const BTOutOfBandData_t * pxOobData);
BTStatus_t prvBTCancelBond(const BTBdaddr_t *pxBdAddr);
BTStatus_t prvBTRemoveBond(const BTBdaddr_t *pxBdAddr);
BTStatus_t prvBTGetConnectionState(const BTBdaddr_t *pxBdAddr, bool * bConnectionState);
BTStatus_t prvBTPinReply(const BTBdaddr_t * pxBdAddr, uint8_t ucAccept, uint8_t ucPinLen, BTPinCode_t * pxPinCode);
BTStatus_t prvBTSspReply(const BTBdaddr_t * pxBdAddr, BTSspVariant_t xVariant, uint8_t ucAccept, uint32_t ulPasskey);
BTStatus_t prvBTReadEnergyInfo(void);
BTStatus_t prvBTDutModeConfigure(bool bEnable);
BTStatus_t prvBTDutModeSend(uint16_t usOpcode, uint8_t * pucBuf, size_t xLen);
BTStatus_t prvBTLeTestMode(uint16_t usOpcode, uint8_t * pucBuf, size_t xLen);
BTStatus_t prvBTConfigHCISnoopLog(bool bEnable);
BTStatus_t prvBTConfigClear(void);
BTStatus_t prvBTReadRssi(const BTBdaddr_t *pxBdAddr);
BTStatus_t prvBTGetTxpower(const BTBdaddr_t * pxBdAddr, BTTransport_t xTransport);
BTStatus_t prvBtManagerCleanup(void);
const void * prvGetClassicAdapter(void);
const void * prvGetLeAdapter(void);

static BTInterface_t xBTinterface = {
		.pxBtManagerInit = prvBTManagerInit,
		.pxEnable = prvBTEnable,
		.pxDisable = prvBTDisable,
		.pxGetAllDeviceProperties = prvBTGetAllDeviceProperties,
		.pxGetDeviceProperty = prvBTGetDeviceProperty,
		.pxSetDeviceProperty = prvBTSetDeviceProperty,
		.pxGetAllRemoteDeviceProperties = prvBTGetAllRemoteDeviceProperties,
		.pxGetRemoteDeviceProperty = prvBTGetRemoteDeviceProperty,
		.pxSetRemoteDeviceProperty = prvBTSetRemoteDeviceProperty,
		.pxPair = prvBTPair,
		.pxCreateBondOutOfBand = prvBTCreateBondOutOfBand,
		.pxCancelBond = prvBTCancelBond,
		.pxRemoveBond = prvBTRemoveBond,
		.pxGetConnectionState = prvBTGetConnectionState,
		.pxPinReply = prvBTPinReply,
		.pxSspReply = prvBTSspReply,
		.pxReadEnergyInfo = prvBTReadEnergyInfo,
		.pxDutModeConfigure = prvBTDutModeConfigure,
		.pxDutModeSend = prvBTDutModeSend,
		.pxLeTestMode = prvBTLeTestMode,
		.pxConfigHCISnoopLog = prvBTConfigHCISnoopLog,
		.pxConfigClear = prvBTConfigClear,
		.pxReadRssi = prvBTReadRssi,
		.pxGetTxpower = prvBTGetTxpower,
		.pxGetClassicAdapter = prvGetClassicAdapter,
		.pxGetLeAdapter = prvGetLeAdapter,
        .pxBtManagerCleanup = prvBtManagerCleanup,
};

void aci_gap_numeric_comparison_value_event(uint16_t Connection_Handle, uint32_t Numeric_Value) {
    if(xBTCallbacks.pxSspRequestCb != NULL)
    {
        xBTCallbacks.pxSspRequestCb(&btadd, NULL, 0, eBTsspVariantPasskeyConfirmation, Numeric_Value);
    }
}

void aci_gap_pairing_complete_event(uint16_t Connection_Handle, uint8_t Status, uint8_t Reason){
    tBleStatus ret = BLE_STATUS_SUCCESS;
    uint8_t Security_Mode;
    uint8_t Security_Level;
    ret = aci_gap_get_security_level(Connection_Handle, &Security_Mode, &Security_Level);

    if ((xBTCallbacks.pxPairingStateChangedCb != NULL)) {
        xBTCallbacks.pxPairingStateChangedCb( ((Status == 0) && (ret == BLE_STATUS_SUCCESS)) ? eBTStatusSuccess : eBTStatusFail,
                                             &btadd,
                                             eBTbondStateBonded,
                                             (BTSecurityLevel_t) Security_Level,
                                             eBTauthSuccess);
    }
}

void hci_encryption_change_event(uint8_t Status, uint16_t Connection_Handle, uint8_t Encryption_Enabled) {

    // nothing to here.
}

void aci_gap_slave_security_initiated_event(void){

	// nothing to here.
}

BTStatus_t prvBTManagerInit(const BTCallbacks_t * pxCallbacks) {
	BTStatus_t xStatus = eBTStatusSuccess;

	if (pxCallbacks != NULL) {
		xBTCallbacks = *pxCallbacks;
	} else {
		xStatus = eBTStatusFail;
	}

	return xStatus;
}
#define BD_ADDR_SIZE_LOCAL    6
#define EUI64_ADDR_SIZE    8
#define EUI64_OID 0xF1D1

void BleGetBdAddress( uint8_t *bd_addr_udn )
{
  uint32_t udn;
  uint32_t company_id;
  uint32_t device_id;
  uint8_t buffer[30] = {0};
  uint8_t EUI_addr[EUI64_ADDR_SIZE]={0};
  uint16_t EUI64_size=EUI64_ADDR_SIZE;
  uint8_t board_name[] = {0x00,0x01};
  optiga_lib_status_t status;
  uint16_t oid = EUI64_OID;

  udn = LL_FLASH_GetUDN();
  status = mutualauth_optiga_read(oid,EUI_addr,&EUI64_size);
  if (OPTIGA_LIB_SUCCESS == status && EUI_addr[3] == board_name[0] && EUI_addr[4] == board_name[1])
  {
	  sprintf((char *)buffer,"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",EUI_addr[7],EUI_addr[6],EUI_addr[5],EUI_addr[4],EUI_addr[3],EUI_addr[2],EUI_addr[1],EUI_addr[0]);
	  configPRINT_STRING("\r\n=================== EUI64 Address ===================\r\n")
	  configPRINT_STRING("            [EUI64_Addr]:: ")
	  configPRINT_STRING((char *)buffer);
	  configPRINT_STRING("\r\n=====================================================\r\n")

	  bd_addr_udn[0] = EUI_addr[0];
	  bd_addr_udn[1] = EUI_addr[1];
	  bd_addr_udn[2] = EUI_addr[2];
	  bd_addr_udn[3] = EUI_addr[5];
	  bd_addr_udn[4] = EUI_addr[6];
	  bd_addr_udn[5] = EUI_addr[7];
  }
  else if(udn != 0xFFFFFFFF)
  {
    company_id = LL_FLASH_GetSTCompanyID();
    device_id = LL_FLASH_GetDeviceID();

    bd_addr_udn[0] = (uint8_t)(udn & 0x000000FF);
    bd_addr_udn[1] = (uint8_t)( (udn & 0x0000FF00) >> 8 );
    bd_addr_udn[2] = (uint8_t)( (udn & 0x00FF0000) >> 16 );
    bd_addr_udn[3] = (uint8_t)device_id;
    bd_addr_udn[4] = (uint8_t)(company_id & 0x000000FF);;
    bd_addr_udn[5] = (uint8_t)( (company_id & 0x0000FF00) >> 8 );

    /* Create EUI64 */
	EUI_addr[0] = bd_addr_udn[0];
	EUI_addr[1] = bd_addr_udn[1];
	EUI_addr[2] = bd_addr_udn[2];
	EUI_addr[3] = board_name[0];
	EUI_addr[4] = board_name[1];
	EUI_addr[5] = bd_addr_udn[3];
	EUI_addr[6] = bd_addr_udn[4];
	EUI_addr[7] = bd_addr_udn[5];

	status = mutualauth_optiga_write(oid,EUI_addr,EUI64_size);
    if (OPTIGA_LIB_SUCCESS != status)
    {
    	configPRINT_STRING("Error: Failed to Write Endpoint\n");
    }
  }
  sprintf((char *)buffer,"%02x:%02x:%02x:%02x:%02x:%02x",bd_addr_udn[5],bd_addr_udn[4],bd_addr_udn[3],bd_addr_udn[2],bd_addr_udn[1],bd_addr_udn[0]);
  configPRINT_STRING("\r\n=================== MAC Address ====================\r\n")
  configPRINT_STRING("            [MAC_Addr]:: ")
  configPRINT_STRING((char *)buffer);
  configPRINT_STRING("\r\n====================================================\r\n")
}

BTStatus_t prvBTEnable(uint8_t ucGuestMode) {
    BTProperty_t pxProperty;

    /*HCI Reset to synchronise BLE Stack*/
    hci_reset();
    optiga_shell_init();

    HAL_Delay(100);
#if ( BLE_MUTUAL_AUTH_SERVICES == 1 )
    BleGetBdAddress(bdaddr);
#endif

	for (;;) {

		/* Set the device public address */
		if (aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, bdaddr))
			break;

		/* Set the TX power -2 dBm */
		if (aci_hal_set_tx_power_level(1, 4))
			break;

		/* GATT Init */
		if (aci_gatt_init())
			break;

		/* GAP Init */
		if (aci_gap_init(GAP_PERIPHERAL_ROLE, 0, 0x07, &service_handle, &dev_name_char_handle, &appearance_char_handle))
			break;

		/* Update device name */
		if (aci_gatt_update_char_value(service_handle, dev_name_char_handle, 0, xLocalName.local_name_size, (uint8_t*) xLocalName.local_name))
			break;

        /* Default bondable */
        pxProperty.xType = eBTpropertyBondable;
        pxProperty.xLen = sizeof(xProperties.bBondable);
        pxProperty.pvVal = &xProperties.bBondable;
        if ( prvBTSetDeviceProperty( &pxProperty ) != eBTStatusSuccess )
            break;

        /* Default IO */
        pxProperty.xType = eBTpropertyIO;
        pxProperty.xLen = sizeof(xProperties.xPropertyIO);
        pxProperty.pvVal = &xProperties.xPropertyIO;
        if ( prvBTSetDeviceProperty( &pxProperty ) != eBTStatusSuccess )
            break;

        /* Default IO */
        pxProperty.xType = eBTpropertySecureConnectionOnly;
        pxProperty.xLen = sizeof(xProperties.bSecureConnectionOnly);
        pxProperty.pvVal = &xProperties.bSecureConnectionOnly;
        if ( prvBTSetDeviceProperty( &pxProperty ) != eBTStatusSuccess )
            break;

		if(xBTCallbacks.pxDeviceStateChangedCb != NULL)
		{
			xBTCallbacks.pxDeviceStateChangedCb( eBTstateOn );
		}

		return eBTStatusSuccess;
	}

	return eBTStatusFail;
}

BTStatus_t prvBTDisable() {
    /* No disable function in WPAN so we use reset */
    hci_reset();
    if(xBTCallbacks.pxDeviceStateChangedCb != NULL)
    {
        xBTCallbacks.pxDeviceStateChangedCb( eBTstateOff );
    }
	return eBTStatusSuccess;
}

BTStatus_t prvBtManagerCleanup(void)
{
    if(xLocalName.local_name != NULL)
    {
        vPortFree(xLocalName.local_name);
        xLocalName.local_name = NULL;
    }
    return eBTStatusSuccess;
}

BTStatus_t prvBTGetAllDeviceProperties() {
	return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/
BTStatus_t prvGetBondableDeviceList( void )
{
    BTStatus_t xStatus = eBTStatusSuccess;
    uint8_t usNbDevices;
    uint16_t usIndex;
    BTProperty_t xBondedDevices = { 0 };
    Bonded_Device_Entry_t xWpanBondedDevices[IOT_BLE_MAX_BONDED_DEVICES];
    tBleStatus ret = BLE_STATUS_SUCCESS;

    ret = aci_gap_get_bonded_devices( &usNbDevices, xWpanBondedDevices );

    if( ret != BLE_STATUS_SUCCESS )
    {
        return eBTStatusFail;
    }

    if( usNbDevices == 0 )
    {
        xBondedDevices.xLen = 0;
        xBondedDevices.xType = eBTpropertyAdapterBondedDevices;
        xBondedDevices.pvVal = NULL;

        if( xBTCallbacks.pxAdapterPropertiesCb != NULL )
        {
            xBTCallbacks.pxAdapterPropertiesCb( eBTStatusSuccess, 1, &xBondedDevices );
        }

        return eBTStatusSuccess;
    }

    xBondedDevices.pvVal = pvPortMalloc( sizeof( BTBdaddr_t ) * usNbDevices );

    if( !xBondedDevices.pvVal )
    {
        return eBTStatusFail;
    }

    for( usIndex = 0; usIndex < usNbDevices; usIndex++ )
    {
        memcpy( &( ( BTBdaddr_t * ) xBondedDevices.pvVal )[ usIndex ], xWpanBondedDevices[ usIndex ].Address, sizeof( BTBdaddr_t ) );
    }

    xBondedDevices.xLen = usNbDevices * sizeof( BTBdaddr_t );
    xBondedDevices.xType = eBTpropertyAdapterBondedDevices;

    if( xBTCallbacks.pxAdapterPropertiesCb != NULL )
    {
        xBTCallbacks.pxAdapterPropertiesCb( xStatus, 1, &xBondedDevices );
    }

    vPortFree( xBondedDevices.pvVal );

    return xStatus;
}

BTStatus_t prvBTGetDeviceProperty(BTPropertyType_t xType) {
	BTStatus_t xStatus = eBTStatusSuccess;
    BTProperty_t xReturnedProperty;
    bool xTRUEval;
    BTIOtypes_t xBTIOtype;

	if (xBTCallbacks.pxAdapterPropertiesCb != NULL) {
		xReturnedProperty.xType = xType;

		switch (xType) {
		case eBTpropertySecureConnectionOnly:
        	xReturnedProperty.xLen = sizeof(bool);
        	xReturnedProperty.xType = eBTpropertySecureConnectionOnly;
        	xReturnedProperty.pvVal = (void *)&xProperties.bSecureConnectionOnly;
            if (xBTCallbacks.pxAdapterPropertiesCb != NULL) {
                xBTCallbacks.pxAdapterPropertiesCb(eBTStatusSuccess, 1, &xReturnedProperty);
            }
			break;
        case eBTpropertyBdname:
            xReturnedProperty.pvVal = xLocalName.local_name;
			xReturnedProperty.xLen = xLocalName.local_name_size;
			xStatus = eBTStatusSuccess;
            if (xBTCallbacks.pxAdapterPropertiesCb != NULL) {
                xBTCallbacks.pxAdapterPropertiesCb(xStatus, 1, &xReturnedProperty);
            }
			break;
		case eBTpropertyBdaddr:
            xReturnedProperty.pvVal = bdaddr;
            xReturnedProperty.xLen = sizeof(bdaddr);
            xStatus = eBTStatusSuccess;
            if (xBTCallbacks.pxAdapterPropertiesCb != NULL) {
                xBTCallbacks.pxAdapterPropertiesCb(xStatus, 1, &xReturnedProperty);
            }
			break;
		case eBTpropertyTypeOfDevice:            
            xReturnedProperty.pvVal = (void*) &xBTdevType;
            xReturnedProperty.xLen = sizeof(xBTdevType);
            xStatus = eBTStatusSuccess;
            if (xBTCallbacks.pxAdapterPropertiesCb != NULL) {
                xBTCallbacks.pxAdapterPropertiesCb(xStatus, 1, &xReturnedProperty);
            }
			break;
		case eBTpropertyLocalMTUSize:
            xReturnedProperty.pvVal = &xProperties.ulMtu;
            xReturnedProperty.xLen = sizeof(xProperties.ulMtu);
			xStatus = eBTStatusSuccess;
            if (xBTCallbacks.pxAdapterPropertiesCb != NULL) {
                xBTCallbacks.pxAdapterPropertiesCb(xStatus, 1, &xReturnedProperty);
            }
			break;
        case eBTpropertyBondable:
			switch (xAuthReq) {
				case BONDING:
                    xTRUEval = true;
					break;
				case NO_BONDING:
				default:
                    xTRUEval = false;
					break;
			}
            xReturnedProperty.pvVal = (void*) &xTRUEval;
            xReturnedProperty.xLen = sizeof(xTRUEval);
			xStatus = eBTStatusSuccess;
            if (xBTCallbacks.pxAdapterPropertiesCb != NULL) {
                xBTCallbacks.pxAdapterPropertiesCb(xStatus, 1, &xReturnedProperty);
            }
			break;
		case eBTpropertyIO:
			switch (xIocap) {
			case IO_CAP_DISPLAY_ONLY:
                xBTIOtype = eBTIODisplayOnly;
				break;
			case IO_CAP_DISPLAY_YES_NO:
                xBTIOtype = eBTIODisplayYesNo;
				break;
			case IO_CAP_KEYBOARD_ONLY:
                xBTIOtype = eBTIOKeyboardOnly;
				break;
			case IO_CAP_KEYBOARD_DISPLAY:
                xBTIOtype = eBTIOKeyboardDisplay;
				break;
			default:
                xBTIOtype = eBTIONone;
				break;
			}
            xReturnedProperty.pvVal = (void*) &xBTIOtype;
            xReturnedProperty.xLen = sizeof(xBTIOtype);
			xStatus = eBTStatusSuccess;
            if (xBTCallbacks.pxAdapterPropertiesCb != NULL) {
                xBTCallbacks.pxAdapterPropertiesCb(xStatus, 1, &xReturnedProperty);
            }
			break;
        case eBTpropertyAdapterBondedDevices:
            xStatus = prvGetBondableDeviceList();
            break;
		default:
			xStatus = eBTStatusUnsupported;
		}
	}

	return xStatus;
}

BTStatus_t prvBTSetDeviceProperty(const BTProperty_t *pxProperty) {
	BTStatus_t xStatus = eBTStatusSuccess;

	uint8_t ret = BLE_STATUS_SUCCESS;

	switch (pxProperty->xType) {
    case eBTpropertyBdname:
        if(xLocalName.local_name != NULL) {
            vPortFree(xLocalName.local_name);
        }
        if(pxProperty->xLen < DEVICE_NAME_CHARACTERISTIC_LEN) {
            xLocalName.local_name = pvPortMalloc(pxProperty->xLen + 1);
            if (xLocalName.local_name != NULL) {
                memcpy(&xLocalName.local_name[0], pxProperty->pvVal, pxProperty->xLen);
                xLocalName.local_name[pxProperty->xLen] = '\0';
                xLocalName.local_name_size = pxProperty->xLen;
                ret = aci_gatt_update_char_value(service_handle, dev_name_char_handle, 0, xLocalName.local_name_size, (uint8_t*) xLocalName.local_name);
                if (ret != BLE_STATUS_SUCCESS) {
                    xStatus = eBTStatusFail;
                }
            } else {
                xStatus = eBTStatusNoMem;
            }
        }
        else {
            xStatus = eBTStatusParamInvalid;
        }
        if (xBTCallbacks.pxAdapterPropertiesCb != NULL) {
			xBTCallbacks.pxAdapterPropertiesCb(xStatus, 1, (BTProperty_t *) pxProperty);
		}
		break;

	case eBTpropertyBdaddr:
		xStatus = eBTStatusUnsupported;
		break;

	case eBTpropertyTypeOfDevice:
		xStatus = eBTStatusUnsupported;
		break;

	case eBTpropertyBondable:
		xProperties.bBondable = *((bool *) pxProperty->pvVal);
		if (*((bool *) pxProperty->pvVal) == true) {
			xAuthReq = BONDING;
		}

        if (xProperties.bSecureConnectionOnly){
            if ((xProperties.xPropertyIO != eBTIODisplayYesNo) && (xProperties.xPropertyIO != eBTIOKeyboardDisplay)) {
                configPRINT_STRING(("prvBTSetDeviceProperty / bSecureConnectionOnly: Input/output property are incompatible with secure connection only Mode\r\n"));
                xStatus = eBTStatusFail;
            } else{
                ret = aci_gap_set_authentication_requirement(xAuthReq, MITM_PROTECTION_REQUIRED, SC_IS_MANDATORY, KEYPRESS_IS_NOT_SUPPORTED, 7, 16, USE_FIXED_PIN_FOR_PAIRING, 123456, 0x00);
                if (ret != BLE_STATUS_SUCCESS) {
                    xStatus = eBTStatusFail;
                }
            }
        }
        else{
            ret = aci_gap_set_authentication_requirement(xAuthReq, MITM_PROTECTION_REQUIRED, SC_IS_SUPPORTED, KEYPRESS_IS_NOT_SUPPORTED, 7, 16, USE_FIXED_PIN_FOR_PAIRING, 123456, 0x00);
            if (ret != BLE_STATUS_SUCCESS) {
                xStatus = eBTStatusFail;
            }
        }

        if (xBTCallbacks.pxAdapterPropertiesCb != NULL) {
			xBTCallbacks.pxAdapterPropertiesCb(xStatus, 1, (BTProperty_t *) pxProperty);
		}
		break;

    case eBTpropertyLocalMTUSize:
        if(*((uint16_t *) pxProperty->pvVal) <= CFG_BLE_MAX_ATT_MTU) {
            xProperties.ulMtu = *((uint16_t *) pxProperty->pvVal);
            xStatus = eBTStatusSuccess;
        }
        else {
            xStatus = eBTStatusParamInvalid;
        }

        if (xBTCallbacks.pxAdapterPropertiesCb != NULL) {
            xBTCallbacks.pxAdapterPropertiesCb(xStatus, 1, (BTProperty_t *) pxProperty);
        }
        break;

	case eBTpropertyIO:
		xProperties.xPropertyIO = *((BTIOtypes_t *) pxProperty->pvVal);
		switch (*((BTIOtypes_t *) pxProperty->pvVal)) {
		case eBTIODisplayOnly:
			xIocap = IO_CAP_DISPLAY_ONLY;
			break;
		case eBTIODisplayYesNo:
			xIocap = IO_CAP_DISPLAY_YES_NO;
			break;
		case eBTIOKeyboardOnly:
			xIocap = IO_CAP_KEYBOARD_ONLY;
			break;
		case eBTIOKeyboardDisplay:
			xIocap = IO_CAP_KEYBOARD_DISPLAY;
			break;
		default:
			// RR
			if (xProperties.bSecureConnectionOnly == true)  {
				configPRINT_STRING(("prvBTSetDeviceProperty / eBTpropertyIO: WARNING!!! eBTpropertySecureConnectionOnly is true and setting eBTpropertyIO property as eBTIONone will result in pairing failure!\r\n"));
			}
			// RR
			xIocap = IO_CAP_NO_INPUT_NO_OUTPUT;
			break;
        }
		ret = aci_gap_set_io_capability(xIocap);
		if (ret != BLE_STATUS_SUCCESS) {
			xStatus = eBTStatusFail;
		}

        if (xBTCallbacks.pxAdapterPropertiesCb != NULL) {
			xBTCallbacks.pxAdapterPropertiesCb(xStatus, 1, (BTProperty_t *) pxProperty);
		}
		break;
	case eBTpropertySecureConnectionOnly:
		xProperties.bSecureConnectionOnly = *(( bool *) pxProperty->pvVal); /* update flag */
		if (xProperties.bSecureConnectionOnly){
			if ((xProperties.xPropertyIO != eBTIODisplayYesNo) && (xProperties.xPropertyIO != eBTIOKeyboardDisplay)) {
				configPRINT_STRING(("prvBTSetDeviceProperty / bSecureConnectionOnly: Input/output property are incompatible with secure connection only Mode\r\n"));
				xStatus = eBTStatusFail;
			} else{
				ret = aci_gap_set_authentication_requirement(xProperties.bBondable, MITM_PROTECTION_REQUIRED, SC_IS_MANDATORY, KEYPRESS_IS_NOT_SUPPORTED, 7, 16, USE_FIXED_PIN_FOR_PAIRING, 123456, 0x00);
				if (ret != BLE_STATUS_SUCCESS) {
					xStatus = eBTStatusFail;
				}
			}
		}
		else{
			ret = aci_gap_set_authentication_requirement(xProperties.bBondable, MITM_PROTECTION_REQUIRED, SC_IS_SUPPORTED, KEYPRESS_IS_NOT_SUPPORTED, 7, 16, USE_FIXED_PIN_FOR_PAIRING, 123456, 0x00);
			if (ret != BLE_STATUS_SUCCESS) {
				xStatus = eBTStatusFail;
			}
        }

		xStatus = prvToggleSecureConnectionOnlyMode(xProperties.bSecureConnectionOnly);
        if (xBTCallbacks.pxAdapterPropertiesCb != NULL) {
			xBTCallbacks.pxAdapterPropertiesCb(xStatus, 1, (BTProperty_t *) pxProperty);
		}
		break;
	default:
		xStatus = eBTStatusUnsupported;
		break;
	}

	return xStatus;

}

BTStatus_t prvBTGetAllRemoteDeviceProperties(BTBdaddr_t *pxRemoteAddr) {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTGetRemoteDeviceProperty(BTBdaddr_t *pxRemoteAddr, BTPropertyType_t type) {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTSetRemoteDeviceProperty(BTBdaddr_t *pxRemoteAddr, const BTProperty_t *property) {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTPair(const BTBdaddr_t * pxBdAddr, BTTransport_t xTransport, bool bCreateBond) {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTCreateBondOutOfBand(const BTBdaddr_t * pxBdAddr, BTTransport_t xTransport, const BTOutOfBandData_t * pxOobData) {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTCancelBond(const BTBdaddr_t *pxBdAddr) {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTRemoveBond(const BTBdaddr_t *pxBdAddr) {
    tBleStatus ret = BLE_STATUS_SUCCESS;
    uint8_t usNbDevices;
    Bonded_Device_Entry_t xWpanBondedDevices[IOT_BLE_MAX_BONDED_DEVICES];

    ret = aci_gap_get_bonded_devices( &usNbDevices, xWpanBondedDevices );
    if( ret != BLE_STATUS_SUCCESS )
    {
        return eBTStatusFail;
    }

    for(uint8_t i = 0; i < usNbDevices; i++)
    {
        if( memcmp(xWpanBondedDevices[i].Address, pxBdAddr, btADDRESS_LEN) == 0 )
        {
            ret = aci_gap_remove_bonded_device(xWpanBondedDevices[i].Address_Type, xWpanBondedDevices[i].Address);
            break;
        }
    }

    if ((xBTCallbacks.pxPairingStateChangedCb != NULL) && (ret == BLE_STATUS_SUCCESS)) {
        xBTCallbacks.pxPairingStateChangedCb((ret == BLE_STATUS_SUCCESS) ? eBTStatusSuccess : eBTStatusFail,
                                             (BTBdaddr_t *)pxBdAddr,
                                             eBTbondStateNone,
                                             eBTSecLevelNoSecurity,
                                             eBTauthSuccess);
    }
	return ret == BLE_STATUS_SUCCESS ? eBTStatusSuccess : eBTStatusFail;
}

BTStatus_t prvBTGetConnectionState(const BTBdaddr_t *pxBdAddr, bool * bConnectionState) {
    return eBTStatusUnsupported;
}

BTStatus_t prvBTPinReply(const BTBdaddr_t * pxBdAddr, uint8_t ucAccept, uint8_t ucPinLen, BTPinCode_t * pxPinCode) {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTSspReply(const BTBdaddr_t * pxBdAddr, BTSspVariant_t xVariant, uint8_t ucAccept, uint32_t ulPasskey) {
	BTStatus_t xStatus = eBTStatusSuccess;
	uint8_t xBleStatus = BLE_STATUS_SUCCESS;

    switch( xVariant )
    {
        case eBTsspVariantPasskeyConfirmation:

            if(ucAccept == true)
            {
                xBleStatus = aci_gap_numeric_comparison_value_confirm_yesno(connection_handle, 0x01);
            }else
            {
                xBleStatus = aci_gap_numeric_comparison_value_confirm_yesno(connection_handle, 0x00);
            }

            xStatus = (xBleStatus == BLE_STATUS_SUCCESS) ? eBTStatusSuccess : eBTStatusFail;
            break;

        case eBTsspVariantPasskeyEntry:
            xStatus = eBTStatusUnsupported;
            break;

        case eBTsspVariantPasskeyNotification:
            xStatus = eBTStatusUnsupported;
            break;
        case eBTsspVariantConsent:
            xStatus = eBTStatusUnsupported;
            break;
        default:
            break;
    }

    return xStatus;
}

BTStatus_t prvBTReadEnergyInfo() {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTDutModeConfigure(bool bEnable) {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTDutModeSend(uint16_t usOpcode, uint8_t * pucBuf, size_t xLen) {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTLeTestMode(uint16_t usOpcode, uint8_t * pucBuf, size_t xLen) {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTConfigHCISnoopLog(bool bEnable) {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTConfigClear() {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTReadRssi(const BTBdaddr_t *pxBdAddr) {
	return eBTStatusUnsupported;
}

BTStatus_t prvBTGetTxpower(const BTBdaddr_t * pxBdAddr, BTTransport_t xTransport) {
	return eBTStatusUnsupported;
}

const void * prvGetClassicAdapter() {
	return NULL;
}

const BTInterface_t * BTGetBluetoothInterface() {
	return &xBTinterface;
}
