/*
 * Amazon FreeRTOS V1.4.7
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* EFM32GG11 Includes */
#include <stdint.h>
#include <stdbool.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "bsp.h"
#include "bsp_trace.h"
#include "retargetserial.h"

/* XBee hardware includes */
#include "xbee/platform.h"
#include "xbee/device.h"
#include "xbee/ipv4.h"
#include "xbee/atcmd.h"
#include "xbee/byteorder.h"
#include "xbee/xbee_config.h"

/* Trust M callback includes */
#include "get_optiga_certificate.h"
#include "optiga/pal/pal_os_event.h"
#include "optiga/common/optiga_lib_return_codes.h"

/* AWS System includes */
#include "aws_clientcredential.h"
#include "aws_application_version.h"

/* Demo includes */
#include "aws_demo.h"
#include "aws_dev_mode_key_provisioning.h"

/* Logging includes. */
#include "iot_logging_task.h"

/* Macro definitions */
#define	AWS_DEMO_RUNNER						1

#define mainLOGGING_TASK_PRIORITY           ( configMAX_PRIORITIES - 1 )
#define mainLOGGING_TASK_STACK_SIZE         ( configMINIMAL_STACK_SIZE * 10 )
#define mainLOGGING_MESSAGE_QUEUE_LENGTH    ( 15 )

#ifdef USER_INPUT_ARN
	#define ENDPOINT_STACK_SIZE				( configMINIMAL_STACK_SIZE + 50 )
	#define ENDPOINT_TASK_PRIORITY 			( tskIDLE_PRIORITY + 5 )
#endif

/* BAUD RATE for Xbee3 LTE module USART */
#define  XBEE_TARGET_BAUD					115200

#ifdef USER_INPUT_ARN
	uint8_t Endpoint[MAX_ENDPOINT_SIZE];
	uint16_t EndpointSize = MAX_ENDPOINT_SIZE;
	uint8_t button_pressed = 0;
#endif

/* Global var  */
uint8_t UUID[30] = {0};

/* Extern global var */
xbee_serial_t ser1;
xbee_dev_t Xbee_dev;

/* File local var */
static StaticSemaphore_t xCellularSemaphoreBuffer;
SemaphoreHandle_t xCellularSemaphoreHandle;

/* Prototypes */
static void prvSetupHardware();
static void getInput(unsigned char *buf);
static int initXBee(void);
static void initCommandLayer(void);
static bool isCmdQueryFinished(xbee_dev_t *xbee);
static void printDeviceDescription(void);
static void waitForXbeeNetwork(void);
static void storeUUID(void);
static void DEMO_RUNNER(void);
static void BSP_ButtonInit(void);
static int checkButton(void);

/**
 * @brief Application runtime entry point.x
 *
 *
 * Top level porting diagram
 --------------------------------------
 1.MQTT msgs (un-encrypted packets)
 --------------------------------------
 |
 -------------------------------------------------|    I2C  +------------------+
 2.MBEDTLS (encryption is done here)   ----PKCS11---------> |Optiga Trust-M IC |
 -------------------------------------------------|         +------------------+
 |
 ------------------------------------------------|------------------------------------|
 3.SSL --> (no TCP/IP as chip has internal stack)| SOC_Create, SOCK_Send,  SOCK_Recv  |
 ------------------------------------------------|  ^ --------  ^ --------- ^ --------|
 |                                               |  |           |           |         |
 ------------------------------------------------|  * --------- * --------- * --------|
 4.xbee_ansic_library xbee_socket.c  ->          | Xbee_Create, Xbee_send, Comman_recv|
 ------------------------------------------------|------------------------------------|
 |
 |
 | UART
 |
 ------------------------------------------------------------
 5.AT+ Commands to Xbee hardware + (encrypted) PAYLOAD
 ------------------------------------------------------------
 |
 ---------
 6.AWS
 --------
 *
 *
 */
int main(void) {
	/* Setup the microcontroller hardware for the demo. */
	prvSetupHardware();

	printf("\fSilabs Xbee3 LTE-M AWSFreeRTOS demo 2020 version %u.%u.%u\r\n",
			xAppFirmwareVersion.u.x.ucMajor, xAppFirmwareVersion.u.x.ucMinor,
			xAppFirmwareVersion.u.x.usBuild);

	/* AWS log task creation */
	xLoggingTaskInitialize( mainLOGGING_TASK_STACK_SIZE,
	mainLOGGING_TASK_PRIORITY,
	mainLOGGING_MESSAGE_QUEUE_LENGTH);

#ifdef USER_INPUT_ARN
	if (checkButton()) {
		button_pressed = 1;
		printf("Please Enter Your Endpoint ARN \r\n");
		printf("e.g- xxxxxxxxxxx-ats.iot.xx-xxx-1.amazonaws.com\r\n");
		printf("Endpoint ARN_URL = ");
		getInput(Endpoint);
		printf("\r\n");
	}
#endif

	vTaskStartScheduler();

	/* Should never get here! */
	return 0;
}

/***************************************************************************//**
 * @brief A top level function to call all init. and config. functions for XBee
 ******************************************************************************/
static int initXBee(void) {

	ser1.baudrate = XBEE_TARGET_BAUD;
	/* Initialize the XBee device (identifies device) */
	if (xbee_dev_init(&Xbee_dev, &ser1, NULL, NULL)) {
		configPRINTF("Could not open communication with XBee\n");
		return -EIO;
	}
	configPRINTF("\nXbee LTE-M init done !\n");
	/* Command layer initialization (AT commands) */
	configPRINTF("\nInitializing\n");
	initCommandLayer();

	/* Check for successful communication with the XBee */
	if (Xbee_dev.hardware_version == 0) {
		configPRINTF("Failed to initialize\nattempting to\nreconfigure the XBee\n");
#if defined(XBEE_DEMO_CONFIG) && !defined(XBEE_DEMO_HOLOGRAM_SIM)
		if (configureXBee(&Xbee_dev,
						XBEE_TARGET_BAUD,
						XBEE_CONFIG_MODE_API,
						XBEE_CONFIG_CARRIER_AUTO_DETECT,
						TRUE)) {
			configPRINTF("\fFailed to config\n"
					"check that the XBee\n"
					"is powered and is\n"
					"connected properly");
			return -EIO;
		}

		/* Reinitialize with correct settings */
		Xbee_dev.flags &= ~XBEE_DEV_FLAG_CMD_INIT;
		Xbee_dev.flags &= ~XBEE_DEV_FLAG_QUERY_BEGIN;
		configPRINTF("\fInitializing");
		initCommandLayer();

#else
		/* Without the config code, we cannot continue */
		configPRINTF("\nCould not reconfig,\ninclude config code");
		return -EIO;
#endif
	} else {
		configPRINTF("\nSuccessful initialization of XBee Done\n");
	}

#if defined(XBEE_DEMO_CONFIG) && defined(XBEE_CHANGE_APN)
	/* Attempt to Apply the correct APN as defined by XBEE_TARGET_APN */
	configPRINTF("\nAttempting to apply\nthe new APN\n");
	int ret = configureAPN(&Xbee_dev);
	if (ret < 0) {
		configPRINTF("\fFailed to apply\nthe new APN\n");
	} else if (ret == 1) {
		configPRINTF("The APN was already\nset correctly!\n");
	} else {
		configPRINTF("The new APN has been\nset correctly!\n");
	}
#endif

	/* create semaphore to serialise access to modem */
	xCellularSemaphoreHandle = xSemaphoreCreateMutexStatic(
			&(xCellularSemaphoreBuffer));
	/* Initialize semaphore. */
	xSemaphoreGive(xCellularSemaphoreHandle);
	return 0;
}

#if defined(XBEE_DEMO_CONFIG) && defined(XBEE_DEMO_HOLOGRAM_SIM)
/***************************************************************************//**
 * @brief Custom configuration of XBee to work correctly with Hologram SIM card
 * @retval 0  Successfully configured XBee
 * @retval -EIO Could not configure XBeee
 ******************************************************************************/
int configureXbeeForHologramSim(void) {
	char buff[64];
	uint8_t i;
	uint32_t rxTimeout;
	int ch;

	if (configureXBee(&Xbee_dev,
	XBEE_TARGET_BAUD, XBEE_CONFIG_MODE_BYPASS, XBEE_CONFIG_CARRIER_AUTO_DETECT,
	TRUE)) {
		return -EIO;
	} else {
		/* Disable echo so that response to AT+URAT=7 command will be predictable */
		vTaskDelay(8000);
		xbee_ser_write(&Xbee_dev.serport, "ATE0\r", 10);
		vTaskDelay(1000);
		xbee_ser_rx_flush(&Xbee_dev.serport);

		/* Disable NB-IoT */
		xbee_ser_write(&Xbee_dev.serport, "AT+URAT=7\r", 10);

		/* Wait to receive "OK" response */
		rxTimeout = xbee_millisecond_timer();
		i = 0;
		while (xbee_millisecond_timer() - rxTimeout < 2000) {
			ch = xbee_ser_getchar(&Xbee_dev.serport);
			if (ch >= 0) {
				buff[i] = ch;
				i++;
				if (i >= strlen("\r\nOK\r\n")) {
					buff[i] = '\0';
					break;
				}
			}
		}
	}
	/* Make sure the device responded to the AT+URAT=7 command with "OK" */
	if (strncmp(buff, "\r\nOK\r\n", strlen("\r\nOK\r\n"))) {
		return -EIO;
	}
	/* Place the XBee into API mode */
	if (configureXBee(&Xbee_dev,
	XBEE_TARGET_BAUD, XBEE_CONFIG_MODE_API, XBEE_CONFIG_CARRIER_AUTO_DETECT,
	TRUE)) {
		return -EIO;
	}
	/* Reinitialize with correct settings */
	Xbee_dev.flags &= ~XBEE_DEV_FLAG_CMD_INIT;
	Xbee_dev.flags &= ~XBEE_DEV_FLAG_QUERY_BEGIN;

	return 0;
}
#endif

/***************************************************************************//**
 * @brief Gets basic device information, only calls init once
 ******************************************************************************/
static void initCommandLayer(void) {
	//configPRINTF("Initializing\n");
	xbee_cmd_init_device(&Xbee_dev);
	/* Wait for XBee to respond */
	while (!isCmdQueryFinished(&Xbee_dev)) {
		xbee_dev_tick(&Xbee_dev);
		configPRINTF(".");
		vTaskDelay(100);
	}
	configPRINTF("\n");
}

/***************************************************************************//**
 * @brief Helper Function for initializing the command layer
 ******************************************************************************/
static bool isCmdQueryFinished(xbee_dev_t *xbee) {
	int ret = xbee_cmd_query_status(xbee);

	if (ret == XBEE_COMMAND_LIST_DONE || ret == XBEE_COMMAND_LIST_TIMEOUT) {
		return true;
	}
	return false;
}

/***************************************************************************//**
 * @brief Prints relevant information about the connected XBee
 ******************************************************************************/
static void printDeviceDescription(void) {

	uint8_t * imei = NULL;
	uint8_t uid_buff[100] = {0};

	printf("Xbee3 info : Hardware Version:%x, "
			"Firmware Version:%x, "
			"Baud Rate:%u, "
			"Connection:%d.\n", (unsigned int) Xbee_dev.hardware_version,
			(unsigned int) Xbee_dev.firmware_version,
			(unsigned int) Xbee_dev.serport.baudrate,
			(WPAN_DEV_IS_JOINED(&(Xbee_dev.wpan_dev))));


	imei = getIMEI(&Xbee_dev);
	printf("Module IMEI:");
	for (uint8_t i = 0; i < 15; i++) {
		printf("%c", (uint8_t) imei[i]);
	}
	printf("\n");

	sprintf(uid_buff,"UID: %c%c:%c%c:%c%c:10:00:%c%c:%c%c:%c%c",
				(uint8_t) imei[0],(uint8_t) imei[1],(uint8_t) imei[2],
				(uint8_t) imei[3],(uint8_t) imei[4],(uint8_t) imei[5],
				(uint8_t) imei[9],(uint8_t) imei[10],(uint8_t) imei[11],
				(uint8_t) imei[12],(uint8_t) imei[13],(uint8_t) imei[14]);
	printf("%s\n",uid_buff);

	sprintf(UUID,"%c%c:%c%c:%c%c:10:00:%c%c:%c%c:%c%c",
				(uint8_t) imei[0],(uint8_t) imei[1],(uint8_t) imei[2],
				(uint8_t) imei[3],(uint8_t) imei[4],(uint8_t) imei[5],
				(uint8_t) imei[9],(uint8_t) imei[10],(uint8_t) imei[11],
				(uint8_t) imei[12],(uint8_t) imei[13],(uint8_t) imei[14]);

}

/***************************************************************************//**
 * @brief Connect XBee to available carrier option
 ******************************************************************************/
static void waitForXbeeNetwork(void) {
	/* Reset the connection flag to guarantee repolling of connection status */
	Xbee_dev.wpan_dev.flags &= ~(WPAN_FLAG_JOINED);

	/* Blocking wait for a cell connection */
	configPRINTF("\nWait for cell signal\n");
	do {
		checkConnection(&Xbee_dev);
		vTaskDelay(500);
		configPRINTF(".");
	} while (!(Xbee_dev.wpan_dev.flags & WPAN_FLAG_JOINED));
	configPRINTF("\ncell signal available \n");
}

/***************************************************************************//**
 * @brief Store IMEI generated UUID into Optiga Trust-M
 ******************************************************************************/
static void  storeUUID(void) {

	configPRINTF("Storing UUID in Optiga Trust-M\n");

	optiga_lib_status_t status;
	status = optiga_direct_write(UUID_OID, UUID, sizeof(UUID));

	if (OPTIGA_LIB_SUCCESS != status) {
		printf("Error: Failed to Write UUID\n");
	}
}

/***************************************************************************//**
 * @brief Sets the pin-mode of push button 0 to an input with a pull up
 ******************************************************************************/
static void BSP_ButtonInit(void) {
	// The library already starts the GPIO clock, no need to call CMU_ClockEnable
	GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInputPull, 1);
}

/***************************************************************************//**
 * @brief Checks the state of push button 0
 * @ret 1       if the button is pressed
 * @ret 0       if the button is not pressed
 ******************************************************************************/
static int checkButton(void) {
	return (0 == GPIO_PinInGet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN));
}

/***************************************************************************//**
 * @brief
 *   Get text input from user.
 *
 * @param buf
 *   Buffer to hold the input string.
 ******************************************************************************/
#ifdef USER_INPUT_ARN
static void getInput(unsigned char *buf) {
	int c;
	size_t i;

	for (i = 0; i < MAX_ENDPOINT_SIZE - 1; i++) {
		// Wait for valid input
		while ((c = RETARGET_ReadChar()) < 0) {

		}

		// User inputed backspace
		if (c == 0x08) {
			if (i) {
				RETARGET_WriteChar('\b');
				RETARGET_WriteChar(' ');
				RETARGET_WriteChar('\b');
				buf[--i] = '\0';
			}

			i--;
			continue;
		} else if (c == '\r' || c == '\n') {
			if (i) {
				RETARGET_WriteChar('\n');
				break;
			} else {
				i--;
				continue;
			}
		}

		RETARGET_WriteChar(c); // Echo to terminal
		buf[i] = c;
	}

	buf[i] = '\0';
}
#endif

/*-----------------------------------------------------------*/
void prvSetupHardware() {
	/* Initialize LED driver */
	BSP_LedsInit();

	/* Intialize User 0 button */
	BSP_ButtonInit();

	/* Initialize LEUART/USART and map LF to CRLF */
	RETARGET_SerialInit();
	RETARGET_SerialCrLf(1);

}

/*
 * in order to receive trust-m callback
 * in vApplicationDaemonTaskStartupHook context
 *
 * */
void vApplicationTickHook(void) {
	pal_os_event_trigger_registered_callback();

}

void vApplicationDaemonTaskStartupHook(void) {

	/* Initialize the command layer and print resulting description */
	if (initXBee() != 0) {
		while (1)
			; /* Blocks if there was an error with opening communications */
	}
	vTaskDelay(1000);

	printDeviceDescription();

	vTaskDelay(1000);

	waitForXbeeNetwork();

#ifdef USER_INPUT_ARN
	if (button_pressed) {
		vGetOptigaCertificate();
	}
#endif

#if (AWS_TEST_RUNNER == 1)
	TEST_RUNNER();
#elif (AWS_DEMO_RUNNER == 1)
	DEMO_RUNNER();
#else
	configPRINTF("Configure demo or test and compile again \n");
	while(1);
#endif
}

/*-----------------------------------------------------------*/
/* AWS demo task */
static void DEMO_RUNNER(void) {
	configPRINTF("\n-------------- DEMO_RUNNER -------------- \n");

	if (SYSTEM_Init() == pdPASS) {

		configPRINTF(( "SYSTEM_Init done.\r\n" ));

		vDevModeKeyProvisioning();

		configPRINTF( ("vDevModeKeyProvisioning Done\r\n") );

		storeUUID();

		/* start demo */
		DEMO_RUNNER_RunDemos();
	} else {
		configPRINTF(("System failed to initialize.\r\n"));
		/* Stop here as system init failed */
		configASSERT(pdFAIL);
	}
}

/**
 * @brief User defined Idle task function.
 *
 * @note Do not make any blocking operations in this function.
 */
void vApplicationIdleHook(void) {
	/* FIX ME. If necessary, update to application idle periodic actions. */
	//configPRINTF("vApplicationIdleHook \n");
	static TickType_t xLastPrint = 0;
	TickType_t xTimeNow;
	const TickType_t xconfigPRINTFrequency = pdMS_TO_TICKS(5000);

	xTimeNow = xTaskGetTickCount();

	if ((xTimeNow - xLastPrint) > xconfigPRINTFrequency) {
		configPRINT_STRING( ".\n" );
		xLastPrint = xTimeNow;
	}
}
/*-----------------------------------------------------------*/
/**
 * @brief User defined assertion call. This function is plugged into configASSERT.
 * See FreeRTOSConfig.h to define configASSERT to something different.
 */
void vAssertCalled(const char * pcFile, uint32_t ulLine) {
	/* FIX ME. If necessary, update to applicable assertion routine actions. */

	const uint32_t ulLongSleep = 1000UL;
	volatile uint32_t ulBlockVariable = 0UL;
	volatile char * pcFileName = (volatile char *) pcFile;
	volatile uint32_t ulLineNumber = ulLine;

	(void) pcFileName;
	(void) ulLineNumber;

	configPRINTF(("vAssertCalled \n %s,\n %ld\n", pcFile, (long) ulLine));
	//fflush(stdout);

	/* Setting ulBlockVariable to a non-zero value in the debugger will allow
	 * this function to be exited. */
	taskDISABLE_INTERRUPTS();
	{
		while (ulBlockVariable == 0UL) {
			vTaskDelay(pdMS_TO_TICKS(ulLongSleep));
		}
	}
	taskENABLE_INTERRUPTS();
}
/*-----------------------------------------------------------*/
