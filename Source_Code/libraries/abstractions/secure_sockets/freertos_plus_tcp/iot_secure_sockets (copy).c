/*
 * FreeRTOS Secure Socket V1.0.0
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

/**
 * @file iot_secure_sockets.c
 * @brief WiFi and Secure Socket interface implementation.
 */

/* Define _SECURE_SOCKETS_WRAPPER_NOT_REDEFINE to prevent secure sockets functions
 * from redefining in iot_secure_sockets_wrapper_metrics.h */

#define _SECURE_SOCKETS_WRAPPER_NOT_REDEFINE

/* Socket and WiFi interface includes. */
#include <stdio.h>
#include "iot_secure_sockets.h"
/* freertos headers */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/*Xbee headers*/
#include "xbee/socket.h"
#include "xbee/socket_frames.h"
#include "xbee/atcmd.h"


//#undef _SECURE_SOCKETS_WRAPPER_NOT_REDEFINE
/* @brief WiFi semaphore timeout */
#define lteconfigMAX_SEMAPHORE_WAIT_TIME_MS  	( 60000 )
/* Receive timout */
#define RECV_TIMEOUT  							( 500 )

 /**
 * @brief Maximum time to wait in ticks for obtaining the WiFi semaphore
 * before failing the operation.
 */
 static const TickType_t xSemaphoreWaitTicks =
		 	 pdMS_TO_TICKS( lteconfigMAX_SEMAPHORE_WAIT_TIME_MS );


/*---------- callback handlers ------------*/
extern void notify_callback(xbee_sock_t socket,
                            uint8_t frame_type, uint8_t message);
extern void receive_callback(xbee_sock_t socket, uint8_t status,
                             const void *payload, size_t payload_length);
extern void option_callback(xbee_sock_t socket, uint8_t option_id,
                            uint8_t status, const void *data, size_t data_len);

/*-----------------------------------------------------------*/
/*Generic Callback handlers registration for socket activity*/
const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_SOCK_FRAME_HANDLERS,
    XBEE_FRAME_TABLE_END
};

Socket_t SOCKETS_Socket( int32_t lDomain,
                         int32_t lType,
                         int32_t lProtocol )
{
	 /* Ensure that only supported values are supplied. */
	configASSERT( lDomain == SOCKETS_AF_INET );
	configASSERT( ( lType == SOCKETS_SOCK_STREAM && lProtocol == SOCKETS_IPPROTO_TCP ) );
	uint8_t Xprotocol;
	/*RUTVIJ: As per data sheet no SSL supported */
	switch(lProtocol)
	{
		 case SOCKETS_SOCK_STREAM:
			 Xprotocol = XBEE_SOCK_PROTOCOL_UDP;
			 break;
		 case SOCKETS_IPPROTO_TCP:
			 Xprotocol = XBEE_SOCK_PROTOCOL_TCP;
			 break;
		 default:
			 printf("NetworkIfcErr:: Protocol not supported\n");
			 return ( Socket_t ) SOCKETS_INVALID_SOCKET;

	}

	taskENTER_CRITICAL();

	xbee_dev_t  *xbee_instance = NULL;
	Socket_t ulSocketNumber;
	xbee_instance = &Xbee_dev;
	if(xbee_instance != NULL)
	{
		/* Create local socket , active socket created when handler notifies */
		ulSocketNumber = xbee_sock_create( xbee_instance,
										   Xprotocol,
										   &notify_callback );
	}
	else
	{
		ulSocketNumber = ( Socket_t ) SOCKETS_INVALID_SOCKET;
	}

	taskEXIT_CRITICAL();
#ifdef NETWORK_DBG
	printf("NetworkIfcDbg: SOCKETS_Socket-ulSocketNumber=%d\n",ulSocketNumber);
#endif
    return ulSocketNumber;
}
/*-----------------------------------------------------------*/

Socket_t SOCKETS_Accept( Socket_t xSocket,
                         SocketsSockaddr_t * pxAddress,
                         Socklen_t * pxAddressLength )
{
#ifdef NETWORK_DBG
	printf("NetworkIfcDbg:SOCKETS_Accept\n");
#endif
	/*RUTVIJ: This activity requires at server AWS side not at client */
    return SOCKETS_ERROR_NONE;
}
/*-----------------------------------------------------------*/

int32_t SOCKETS_Connect( Socket_t xSocket,
                         SocketsSockaddr_t * pxAddress,
                         Socklen_t xAddressLength )
{
#ifdef NETWORK_DBG
	printf("NetworkIfcDbg:Remote_addr:%d,port:%d\n",pxAddress->ulAddress,
									SOCKETS_ntohs( pxAddress->usPort ));
#endif

	/* Removed warning */
	(void)xAddressLength;
	int32_t  lRetVal = SOCKETS_SOCKET_ERROR;
	/* wait for semaphore to  be taken till lte max time config */
	if( xSemaphoreTake( xCellularSemaphoreHandle, xSemaphoreWaitTicks ) == pdTRUE )
	{
		lRetVal = xbee_sock_connect( xSocket,
								     SOCKETS_ntohs( pxAddress->usPort ),
								     pxAddress->ulAddress,
									 0,
									 &receive_callback );
		if( lRetVal < 0 )
		{
			printf("NetworkIfcErr:: connect - %d\n",lRetVal);
			lRetVal = SOCKETS_SOCKET_ERROR;
		}
		( void ) xSemaphoreGive( xCellularSemaphoreHandle );
	}
#ifdef NETWORK_DBG
	else
	{
		printf("NetworkIfcErr:: connect - semaphore error\n");
	}
#endif
    return lRetVal;
}
/*-----------------------------------------------------------*/

int32_t SOCKETS_Recv( Socket_t xSocket,
                      void * pvBuffer,
                      size_t xBufferLength,
                      uint32_t ulFlags )
{
	/* Wait till callback - or for TIMEOUT */
	int32_t receivedBytes;
	TickType_t xSemaphoreWait = pdMS_TO_TICKS( RECV_TIMEOUT );
	if( xSemaphoreTake( xCellularSemaphoreHandle, xSemaphoreWait ) == pdTRUE )
	{
		receivedBytes = xbee_sock_recv( xSocket,
                                    	pvBuffer,
										xBufferLength,
										ulFlags );

		if(receivedBytes < 0)
		{
			( void ) xSemaphoreGive( xCellularSemaphoreHandle );
			printf("NetworkIfcErr: Receive \n");
			return SOCKETS_SOCKET_ERROR;
		}
		( void ) xSemaphoreGive( xCellularSemaphoreHandle );
		return receivedBytes;
	}
#ifdef NETWORK_DBG
	else
	{
		printf("NetworkIfcErr:: receive - semaphore error\n");
	}
#endif
	return SOCKETS_ERROR_NONE;
}
/*-----------------------------------------------------------*/

int32_t SOCKETS_Send( Socket_t xSocket,
                      const void * pvBuffer,
                      size_t xDataLength,
                      uint32_t ulFlags )
{
	int32_t lSentBytes = SOCKETS_SOCKET_ERROR;
	/* RUTVIJ: Check for flags if we are allowed to send or not */
	/* RUTVIJ: send un-encrypted data - convert to TLS*/
	if( xSemaphoreTake( xCellularSemaphoreHandle, xSemaphoreWaitTicks ) == pdTRUE )
	{
		lSentBytes = xbee_sock_send( xSocket,
                                 	 (uint8_t)ulFlags,
									 pvBuffer,
									 xDataLength);
		if(lSentBytes < 0)
		{
			printf("NetworkIfcErr:: send-%d\n",lSentBytes);
			lSentBytes = SOCKETS_SOCKET_ERROR;
		}
		( void ) xSemaphoreGive( xCellularSemaphoreHandle );
	}
#ifdef NETWORK_DBG
	else
	{
		printf("NetworkIfcErr:: send - semaphore error\n");
	}
#endif

    return lSentBytes;
}
/*-----------------------------------------------------------*/

int32_t SOCKETS_Shutdown( Socket_t xSocket,
                          uint32_t ulHow )
{
	int32_t lRetVal = SOCKETS_SOCKET_ERROR;
	/*RUTVIJ: Can hold send receive operation for a while using WR flags- dummy */
	switch( ulHow )
	{
	    case SOCKETS_SHUT_RD:
	#ifdef NETWORK_DBG
	    	printf("NetworkIfcDbg: SOCKETS_SHUT_RD request \n");
	#endif
	    	/* Return success to the user. */
	        lRetVal = SOCKETS_ERROR_NONE;
	        break;

	    case SOCKETS_SHUT_WR:
	#ifdef NETWORK_DBG
	    	printf("NetworkIfcDbg: SOCKETS_SHUT_WR request \n");
	#endif
	        /* Return success to the user. */
	        lRetVal = SOCKETS_ERROR_NONE;
	        break;

	    case SOCKETS_SHUT_RDWR:
	#ifdef NETWORK_DBG
	    	printf("NetworkIfcDbg: SOCKETS_SHUT_RDWR request \n");
	#endif
	        /* Return success to the user. */
	        lRetVal = SOCKETS_ERROR_NONE;
	        break;

	    default:
	    	printf("NetworkIfcDbg: shutdown request Invalid\n");
	        /* An invalid value was passed for ulHow. */
	        lRetVal = SOCKETS_EINVAL;
	        break;
	}

    return lRetVal;
}
/*-----------------------------------------------------------*/

int32_t SOCKETS_Close( Socket_t xSocket )
{
	int32_t lRetVal = SOCKETS_SOCKET_ERROR;
	if( xSemaphoreTake( xCellularSemaphoreHandle, xSemaphoreWaitTicks ) == pdTRUE )
	{
		lRetVal = xbee_sock_close( xSocket );
		(void)xSemaphoreGive( xCellularSemaphoreHandle );
	}
#ifdef NETWORK_DBG
	else
	{
		printf("NetworkIfcErr:: close - semaphore error\n");
	}
	printf("NetworkIfcDbg: socket closed, retval=%d\n",lRetVal);
#endif
    /* RUTVIJ: May hardware needs to power down here */
	return lRetVal;
}
/*-----------------------------------------------------------*/

int32_t SOCKETS_SetSockOpt( Socket_t xSocket,
                            int32_t lLevel,
                            int32_t lOptionName,
                            const void * pvOptionValue,
                            size_t xOptionLength )
{
	/*RUTVIJ: May be server side activity not require at client side */
	int32_t lRetVal = SOCKETS_SOCKET_ERROR;
#ifdef NETWORK_DBG
    	printf("NetworkIfcDbg: SOCKETS_SetSockOpt -"
    			"lLevel=%d,lOptionName=%d\n",
				lLevel,
				lOptionName);

#endif
	lRetVal = xbee_sock_option( xSocket,
								lOptionName,
								&pvOptionValue,
								xOptionLength,
								option_callback );
	return lRetVal;
}
/*-----------------------------------------------------------*/

uint32_t SOCKETS_GetHostByName( const char * pcHostName )
{
#ifdef NETWORK_DBG
    	printf("NetworkIfcDbg: SOCKETS_GetHostByName\n");
#endif
    	/*RUTVIJ: Can do DNS lookup for WiFi */
    return 0;
}
/*-----------------------------------------------------------*/

BaseType_t SOCKETS_Init( void )
{
#ifdef NETWORK_DBG
    	printf("NetworkIfcDbg: SOCKETS_Init\n");
#endif
    	/* RUTVIJ: we may perform zero for any buffer,
    	 * sock_options for WR, socket
    	 * list to available*/
    	BaseType_t ret = pdFAIL;
    	taskENTER_CRITICAL();

    	xbee_dev_t  *xbee_instance = NULL ;
    	xbee_instance = &Xbee_dev;
    	if(xbee_instance != NULL)
    	{
    		if(xbee_sock_reset(xbee_instance) == 0)
    			ret = pdPASS;
    	}
    	taskEXIT_CRITICAL();
    	return ret;
}
/*-----------------------------------------------------------*/
/*RUTVIJ: may not needed below */
uint32_t ulRand( void )
{
#ifdef NETWORK_DBG
    	printf("NetworkIfcDbg: ulRand\n");
#endif

    /* FIX ME. */
    return 0;
}
/*-----------------------------------------------------------*/
/*RUTVIJ: may not needed below */
BaseType_t xApplicationGetRandomNumber( uint32_t * pulNumber )
{
#ifdef NETWORK_DBG
    	printf("NetworkIfcDbg: xApplicationGetRandomNumber\n");
#endif

    *( pulNumber ) = 0uL;
    return pdFALSE;
}
