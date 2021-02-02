
#if 1 //taking test so commented
/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* TLS includes. */
#include "iot_tls.h"
#include "iot_pkcs11.h"

/* Socket and WiFi interface includes. */
#include "iot_secure_sockets.h"

/*Xbee headers*/
#include "xbee/socket.h"
#include "xbee/socket_frames.h"
#include "xbee/atcmd.h"

#include "iot_config.h"
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
#define LIBRARY_LOG_NAME    ( "NETWORK" )
#include "iot_logging_setup.h"

/* WiFi configuration includes. */

//#undef _SECURE_SOCKETS_WRAPPER_NOT_REDEFINE

/**
 * @brief A Flag to indicate whether or not a socket is
 * secure i.e. it uses TLS or not.
 */
#define securesocketsSOCKET_SECURE_FLAG                 ( 1UL << 0 )

/**
 * @brief A flag to indicate whether or not a socket is closed
 * for receive.
 */
#define securesocketsSOCKET_READ_CLOSED_FLAG            ( 1UL << 1 )

/**
 * @brief A flag to indicate whether or not a socket is closed
 * for send.
 */
#define securesocketsSOCKET_WRITE_CLOSED_FLAG           ( 1UL << 2 )

/**
 * @brief A flag to indicate whether or not a non-default server
 * certificate has been bound to the socket.
 */
#define securesocketsSOCKET_TRUSTED_SERVER_CERT_FLAG    ( 1UL << 3 )

/**
 * @brief A flag to indicate whether or not the socket is connected.
 *
 */
#define securesocketsSOCKET_IS_CONNECTED                ( 1UL << 4 )

/**
 * @brief Block time wait
 *
 */
#define lteconfigMAX_SEMAPHORE_WAIT_TIME_MS  			( 60000UL )

/**
 * @brief Xbee send soft limit max 1500(HW limit)
 *
 */
#define lteMAX_SEND_BYTES								( 1000 )

/**
 * @brief  Receive timout
 *
 */

#define RECV_TIMEOUT  									( 500 )

/**
 * @brief Represents a secure socket.
 */
typedef struct SSocketContext_t
{
	xbee_sock_t xSocket;
	uint8_t Xprotocol;
	uint32_t ulFlags;                   /**< Various properties of the socket (secured etc.). */
	char * pcDestination;               /**< Destination URL. Set using SOCKETS_SO_SERVER_NAME_INDICATION option in SOCKETS_SetSockOpt function. */
	void * pvTLSContext;                /**< The TLS Context. */
	char * pcServerCertificate;         /**< Server certificate. Set using SOCKETS_SO_TRUSTED_SERVER_CERTIFICATE option in SOCKETS_SetSockOpt function. */
	uint32_t ulServerCertificateLength; /**< Length of the server certificate. */
	char ** ppcAlpnProtocols;
	uint32_t ulAlpnProtocolsCount;
	uint8_t ucInUse;                    /**< Tracks whether the socket is in use or not. */
	uint32_t recv_Flags;
	BaseType_t available;
	size_t offset;
} SSocketContext_t, * SSocketContextPtr_t;

/**
 * @brief Secure socket objects.
 *
 * An index in this array is returned to the user from SOCKETS_Socket
 * function.
 */
static SSocketContext_t xSockets[XBEE_SOCK_SOCKET_COUNT];

/**
 * @brief The global mutex to ensure that only one operation is accessing the
 * SSocketContext.ucInUse flag at one time.
 */
static SemaphoreHandle_t xUcInUse = NULL;
static SemaphoreHandle_t xTLSConnect = NULL;
static const TickType_t xMaxSemaphoreBlockTime =
pdMS_TO_TICKS( lteconfigMAX_SEMAPHORE_WAIT_TIME_MS );

/**
 * @brief pkcs11 optiga random number genrator
 */
static CK_SESSION_HANDLE xPkcs11Session = 0;
static CK_FUNCTION_LIST_PTR pxPkcs11FunctionList = NULL;

extern uint8_t sock_create_resp ;
extern uint8_t sock_connect_resp ;
extern int8_t tx_done;
extern uint8_t recv_cb_arrived;
extern uint32_t offset;
extern uint8_t buff_available;

extern recv_t recv;

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

static uint32_t prvGetFreeSocket( void )
{
	uint32_t ulSocketNumber;

	/* Obtain the socketInUse mutex. */
	if (xSemaphoreTake( xUcInUse, xMaxSemaphoreBlockTime) == pdTRUE)
	{
		/* Iterate over xSockets array to see if any free socket
		 * is available. */
		for (ulSocketNumber = 0 ; ulSocketNumber < ( uint32_t ) XBEE_SOCK_SOCKET_COUNT ; ulSocketNumber++)
		{
			if (xSockets[ ulSocketNumber ].ucInUse == 0U)
			{
				/* Mark the socket as "in-use". */
				xSockets[ ulSocketNumber ].ucInUse = 1;

				/* We have found a free socket, so stop. */
				break;
			}
		}

		/* Give back the socketInUse mutex. */
		xSemaphoreGive(xUcInUse);
	}
	else
	{
		ulSocketNumber = XBEE_SOCK_SOCKET_COUNT;
	}

	/* Did we find a free socket? */
	if (ulSocketNumber == (uint32_t) XBEE_SOCK_SOCKET_COUNT)
	{
		/* Return SOCKETS_INVALID_SOCKET if we fail to
		 * find a free socket. */
		ulSocketNumber = (uint32_t) SOCKETS_INVALID_SOCKET;
	}

	return ulSocketNumber;
}
/*-----------------------------------------------------------*/

static BaseType_t prvReturnSocket( SSocketContextPtr_t pvContext )
{
	BaseType_t xResult = pdFAIL;

	/* Since multiple tasks can be accessing this simultaneously,
	 * this has to be in critical section. */
	/* Obtain the socketInUse mutex. */
	if (xSemaphoreTake( xUcInUse, xMaxSemaphoreBlockTime) == pdTRUE)
	{
		/* Mark the socket as free. */
		pvContext->ucInUse = 0;

		xResult = pdTRUE;

		/* Give back the socketInUse mutex. */
		xSemaphoreGive(xUcInUse);
	}

	return xResult;
}

/*-----------------------------------------------------------*/

static BaseType_t prvIsValidSocket( uint32_t ulSocketNumber )
{
	BaseType_t xValid = pdFALSE;

	/* Check that the provided socket number is within the valid index range. */
	if (ulSocketNumber < ( uint32_t ) XBEE_SOCK_SOCKET_COUNT )
	{
		/* Obtain the socketInUse mutex. */
		if( xSemaphoreTake( xUcInUse, xMaxSemaphoreBlockTime ) == pdTRUE )
		{
			/* Check that this socket is in use. */
			if (xSockets[ ulSocketNumber ].ucInUse == 1U )
			{
				/* This is a valid socket number. */
				xValid = pdTRUE;
			}

			/* Give back the socketInUse mutex. */
			xSemaphoreGive(xUcInUse);
		}
	}

	return xValid;
}
/*-----------------------------------------------------------*/

static BaseType_t prvNetworkSend( void * pvContext,
		const unsigned char * pucData,
		size_t xDataLength )
{
	BaseType_t lSentBytes = SOCKETS_SOCKET_ERROR;
	SSocketContextPtr_t pxContext = (SSocketContextPtr_t) pvContext;

	if ((pucData != NULL) && (pvContext != NULL))
	{
		/* RUTVIJ: Check for flags if we are allowed to send or not */
		if( xSemaphoreTake( xCellularSemaphoreHandle, xMaxSemaphoreBlockTime ) == pdTRUE )
		{
			xDataLength = (xDataLength > lteMAX_SEND_BYTES ? lteMAX_SEND_BYTES : xDataLength);

			lSentBytes = xbee_sock_send( pxContext->xSocket,
						0,// SET 0 as per API doc of Xbee3
					(void*)pucData,
					xDataLength);

			xbee_dev_t  *xbee_instance = NULL;
			xbee_instance = &Xbee_dev;

			while(!tx_done && !(lSentBytes < 0)) {
				xbee_cmd_tick();
				xbee_dev_tick(xbee_instance);
			}

			if(tx_done < 0) //if Tx error try next with current error report
			{
				lSentBytes = -1;
				IotLogInfo("Current Tx err reported\n");
			}

			// for next tx cycle
			tx_done = 0;

			if(lSentBytes < 0)
			{
				printf("NetworkIfcErr:: send-%ld\n",(long)lSentBytes);
				lSentBytes = SOCKETS_SOCKET_ERROR;
			}
			else if(lSentBytes >= 0)
			{
				lSentBytes = xDataLength;
			}
			( void ) xSemaphoreGive( xCellularSemaphoreHandle );

		}
#ifdef NETWORK_DBG
		else
		{
			printf(" NetworkIfcErr:: send - semaphore error\n" );
		}
#endif
	}
	//
	return lSentBytes;
}

static BaseType_t prvNetworkRecv( void * pvContext,
                                  unsigned char * pucReceiveBuffer,
                                  size_t xReceiveLength )
{

  BaseType_t lRetVal = 0;
  SSocketContextPtr_t pxContext = (SSocketContextPtr_t) pvContext;

  if ((pucReceiveBuffer != NULL) && (pvContext != NULL))
  {
		/* Wait till callback - or for TIMEOUT */
		int32_t receivedBytes;
		TickType_t xSemaphoreWait = pdMS_TO_TICKS( RECV_TIMEOUT );

			do
			{ //Always check for new frame arrival before recv()
				xbee_dev_t  *xbee_instance = NULL;
				xbee_instance = &Xbee_dev;

				xbee_dev_tick(xbee_instance);

				if( xSemaphoreTake( xCellularSemaphoreHandle, xSemaphoreWait ) == pdTRUE )
				{

					receivedBytes = xbee_sock_recv( pxContext->xSocket,
															pucReceiveBuffer,
															xReceiveLength,
															pxContext->recv_Flags );
				( void ) xSemaphoreGive( xCellularSemaphoreHandle );
				}
				vTaskDelay(20);

			}while(receivedBytes <= 0);
			/*RUTVIJ TODO - below condition not needded*/
			if(receivedBytes < 0)
			{
				IotLogInfo("NetworkIfcErr: Receive \n");
				return SOCKETS_SOCKET_ERROR;
			}

			return receivedBytes;
  }

  return lRetVal;
}

/*-----------------------------------------------------------*/

Socket_t SOCKETS_Socket( int32_t lDomain,
		int32_t lType,
		int32_t lProtocol )
{
	uint32_t ulSocketNumber;

#ifdef NETWORK_DBG
	IotLogInfo("NetworkIfcDbg: lDomain-%d ,lType-%d ,lProtocol-%d \n",lDomain,lType,lProtocol);
#endif
	/* Ensure that only supported values are supplied. */
	configASSERT( lDomain == SOCKETS_AF_INET );
	configASSERT( lType == SOCKETS_SOCK_STREAM );
	configASSERT( lProtocol == SOCKETS_IPPROTO_TCP );

	ulSocketNumber = prvGetFreeSocket();

	/* If we get a free socket, set its attributes. */
	if(ulSocketNumber != (uint32_t)SOCKETS_INVALID_SOCKET)
	{
		SSocketContextPtr_t pxContext = &xSockets[ulSocketNumber];

		pxContext->ulFlags = 0;
		pxContext->pcDestination = NULL;
		pxContext->pvTLSContext = NULL;
		pxContext->pcServerCertificate = NULL;
		pxContext->ulServerCertificateLength = 0;
		pxContext->ppcAlpnProtocols = NULL;
		pxContext->ulAlpnProtocolsCount = 0;
		pxContext->available = 0;
		pxContext->offset = 0;

		/*Map as per Xbee3*/
		switch(lProtocol)
		{
			case SOCKETS_SOCK_STREAM:
				pxContext->Xprotocol = XBEE_SOCK_PROTOCOL_UDP;
				break;
			case SOCKETS_IPPROTO_TCP:
				pxContext->Xprotocol = XBEE_SOCK_PROTOCOL_TCP;
				break;
			default:
				IotLogDebug("NetworkIfcErr:: Protocol not supported\n");
				return ( Socket_t ) SOCKETS_INVALID_SOCKET;

		}

		taskENTER_CRITICAL();

		xbee_dev_t  *xbee_instance = NULL;
		//Socket_t ulSocketNumber;
		xbee_instance = &Xbee_dev;
		if(xbee_instance != NULL)
		{
			/* Create local socket , active socket created when handler notifies */
			pxContext->xSocket = xbee_sock_create( 	xbee_instance,
					pxContext->Xprotocol,
					&notify_callback );
			if (pxContext->xSocket < 0)
			{
				ulSocketNumber = ( Socket_t ) SOCKETS_INVALID_SOCKET;
			}
			else
			{
				/* RUTVIJ : TODO set intial recev time out */
				while(!sock_create_resp) {	
					xbee_dev_tick(xbee_instance);
					IotLogInfo(".");
				}
				IotLogInfo("NetworkIfcInfo:: Xbee socket created %d,0x0%x\n",pxContext->xSocket,pxContext->xSocket);
			}
		}
		else
		{
			ulSocketNumber = ( Socket_t ) SOCKETS_INVALID_SOCKET;
		}

		taskEXIT_CRITICAL();
	}
	/* If we fail to get a free socket, we return SOCKETS_INVALID_SOCKET. */
	return (Socket_t)ulSocketNumber;
}
/*-----------------------------------------------------------*/

Socket_t SOCKETS_Accept( Socket_t xSocket,
		SocketsSockaddr_t * pxAddress,
		Socklen_t * pxAddressLength )
{
	return SOCKETS_INVALID_SOCKET;
}
/*-----------------------------------------------------------*/

int32_t SOCKETS_Connect( Socket_t xSocket,
		SocketsSockaddr_t * pxAddress,
		Socklen_t xAddressLength )
{
	uint32_t ulSocketNumber = ( uint32_t ) xSocket;
	int32_t lRetVal = SOCKETS_SOCKET_ERROR;

	//printf("SOCKETS_Connect 501\n");
#ifdef NETWORK_DBG
	IotLogInfo("NetworkIfcDbg:port:%d\n",SOCKETS_ntohs( pxAddress->usPort ));
#endif

	(void)xAddressLength;

	if ((prvIsValidSocket( ulSocketNumber ) == pdTRUE) && (pxAddress != NULL) && (pxAddress->usPort != 0) && (pxAddress != 0))
	{
		char remote_addr[20] = { 0 } ;
		SSocketContextPtr_t pxContext = &xSockets[ulSocketNumber];

		/* Check that the socket is not already connected. */
		if ((pxContext->ulFlags & securesocketsSOCKET_IS_CONNECTED ) == 0UL)
		{
			SOCKETS_inet_ntoa(pxAddress->ulAddress, remote_addr);
			IotLogInfo("host_addr %s \n",remote_addr);
			/* wait for semaphore to  be taken till lte max time config */
			if( xSemaphoreTake( xCellularSemaphoreHandle, xMaxSemaphoreBlockTime ) == pdTRUE )
			{
				IotLogInfo("xbee_sock_connecting ...\n");
				lRetVal = xbee_sock_connect( pxContext->xSocket,
						SOCKETS_htons (pxAddress->usPort),//SOCKETS_ntohs( pxAddress->usPort ),SOCKETS_htons( pxAddress->usPort ),
						0, 			     //HEX Form IP
						remote_addr,      //STRING Form IP
						&receive_callback );
				if( lRetVal < 0 )
				{
					IotLogInfo("NetworkIfcErr:: connect - %d\n",lRetVal);
					lRetVal = SOCKETS_SOCKET_ERROR;
				}
				else if (lRetVal == 0)
				{  /*RUTVIJ : TODO handle instance as well as socket nagative replay here*/
					xbee_dev_t  *xbee_instance = NULL;

					xbee_instance = &Xbee_dev;
					if(xbee_instance == NULL)
					{
						lRetVal = SOCKETS_SOCKET_ERROR;
					}
					// wait for module socket response
					while(!sock_connect_resp) {
						xbee_dev_tick(xbee_instance);
						//vTaskDelay(pdMS_TO_TICKS(50));
						//IotLogInfo(".");
					}
					//printf("NetworkIfcInfo:: Xbee socket conncted \n");

					pxContext->ulFlags |= securesocketsSOCKET_IS_CONNECTED;
					lRetVal = SOCKETS_ERROR_NONE;
				}
				( void ) xSemaphoreGive( xCellularSemaphoreHandle );

			}
#ifdef NETWORK_DBG
			else
			{
				IotLogInfo("NetworkIfcErr:: connect - semaphore error\n");
			}
#endif


			/* Negotiate TLS if requested. */
			if ((lRetVal == SOCKETS_ERROR_NONE) && ((pxContext->ulFlags & securesocketsSOCKET_SECURE_FLAG) != 0))
			{

				TLSParams_t xTLSParams = { 0 };
				xTLSParams.ulSize = sizeof( xTLSParams );
				xTLSParams.pcDestination = pxContext->pcDestination;
				xTLSParams.pcServerCertificate = pxContext->pcServerCertificate;
				xTLSParams.ulServerCertificateLength = pxContext->ulServerCertificateLength;
				xTLSParams.ppcAlpnProtocols = ( const char ** ) pxContext->ppcAlpnProtocols;
				xTLSParams.ulAlpnProtocolsCount = pxContext->ulAlpnProtocolsCount;
				xTLSParams.pvCallerContext = pxContext;
				xTLSParams.pxNetworkRecv = prvNetworkRecv;
				xTLSParams.pxNetworkSend = prvNetworkSend;

				/* Initialize TLS. */

				if (TLS_Init(&(pxContext->pvTLSContext), &(xTLSParams)) == pdFREERTOS_ERRNO_NONE)
				{

					if (xSemaphoreTake(xTLSConnect, xMaxSemaphoreBlockTime) == pdTRUE)
					{

						/* Initiate TLS handshake. */
						if (TLS_Connect(pxContext->pvTLSContext) != pdFREERTOS_ERRNO_NONE)
						{
							IotLogInfo(" TLS negotiatin now 6\n");
							/* TLS handshake failed. */
							xbee_sock_close(pxContext->xSocket);
							pxContext->ulFlags &= ~securesocketsSOCKET_IS_CONNECTED;
							sock_connect_resp = 0;
							TLS_Cleanup(pxContext->pvTLSContext);
							pxContext->pvTLSContext = NULL;
							lRetVal = SOCKETS_TLS_HANDSHAKE_ERROR;
						}
						/* Release semaphore */
						xSemaphoreGive(xTLSConnect);
					}
				}
				else
				{
					/* TLS Initialization failed. */
					lRetVal = SOCKETS_TLS_INIT_ERROR;
				}
			}
		}
		else
		{
			lRetVal = SOCKETS_EISCONN;
		}
	}
	else
	{
		lRetVal = SOCKETS_EINVAL;
	}
	IotLogDebug(" connect reture %d \n", lRetVal);
	return lRetVal;
}
/*-----------------------------------------------------------*/

int32_t SOCKETS_Recv( Socket_t xSocket,
		void * pvBuffer,
		size_t xBufferLength,
		uint32_t ulFlags )
{
	int32_t lReceivedBytes = 0;
	uint32_t ulSocketNumber = (uint32_t)xSocket;
	//IotLogInfo("SOCKETS_Recv 501\n");
	/* Ensure that a valid socket was passed and the
	 * passed buffer is not NULL. */
	if ((prvIsValidSocket(ulSocketNumber) == pdTRUE) && (pvBuffer != NULL))
	{
		SSocketContextPtr_t pxContext = &xSockets[ulSocketNumber];
		pxContext->recv_Flags = ulFlags;
		if ((pxContext->ulFlags & securesocketsSOCKET_IS_CONNECTED) != 0UL)
		{
			/* Check that receive is allowed on the socket. */
			if ((pxContext->ulFlags & securesocketsSOCKET_READ_CLOSED_FLAG ) == 0UL)
			{
				if ((pxContext->ulFlags & securesocketsSOCKET_SECURE_FLAG) != 0UL)
				{
					//IotLogInfo("SOCKETS_Recv 515\n");
					/* Receive through TLS pipe, if negotiated. */
					lReceivedBytes = TLS_Recv(pxContext->pvTLSContext, pvBuffer, xBufferLength);

					/* Convert the error code. */
					if (lReceivedBytes < 0)
					{
						//IotLogInfo("SOCKETS_Recv 522\n");
						/* TLS_Recv failed. */
						return SOCKETS_TLS_RECV_ERROR;
					}
				}
				else
				{
					/* Receive un-encrypted. */
					lReceivedBytes = prvNetworkRecv(pxContext, pvBuffer, xBufferLength);
				}
			}
			else
			{
				//IotLogInfo("SOCKETS_Recv 535\n");
				/* The socket has been closed for read. */
				return SOCKETS_ECLOSED;
			}
		}
		else
		{
			//IotLogInfo("SOCKETS_Recv 542\n");
			/* The socket has been closed for read. */
			return SOCKETS_ECLOSED;
		}
	}
	else
	{
		//IotLogInfo("SOCKETS_Recv 549\n");
		return SOCKETS_EINVAL;
	}

	return lReceivedBytes;
}
/*-----------------------------------------------------------*/

int32_t SOCKETS_Send( Socket_t xSocket,
		const void * pvBuffer,
		size_t xDataLength,
		uint32_t ulFlags )
{
	int32_t lSentBytes = SOCKETS_SOCKET_ERROR;
	uint32_t ulSocketNumber = (uint32_t)xSocket;

	//IotLogInfo("SCOKETS_SEND 560\n");
	/* Remove warning about unused parameters. */
	(void)ulFlags;

	if ((prvIsValidSocket(ulSocketNumber) == pdTRUE) && (pvBuffer != NULL))
	{
		SSocketContextPtr_t pxContext = &xSockets[ulSocketNumber];

		if ((pxContext->ulFlags & securesocketsSOCKET_IS_CONNECTED) != 0UL)
		{
			/* Check that send is allowed on the socket. */
			if ((pxContext->ulFlags & securesocketsSOCKET_WRITE_CLOSED_FLAG ) == 0UL)
			{
				if ((pxContext->ulFlags & securesocketsSOCKET_SECURE_FLAG) != 0UL)
				{
					//IotLogInfo("SCOKETS_SEND 575\n");
					/* Send through TLS pipe, if negotiated. */
					lSentBytes = TLS_Send(pxContext->pvTLSContext, pvBuffer, xDataLength);
					//printf("TLS_SEND %d \n",lSentBytes);
					/* Convert the error code. */
					if (lSentBytes < 0)
					{
						//IotLogInfo("SCOKETS_SEND 582\n");
						/* TLS_Recv failed. */
						return SOCKETS_TLS_SEND_ERROR;
					}
				}
				else
				{
					/* Send unencrypted. */
					lSentBytes = prvNetworkSend(pxContext, pvBuffer, xDataLength);
					/* Convert the error code. */
					if (lSentBytes < 0)
					{
						//IotLogInfo("SCOKETS_SEND 594\n");
						/* failed. */
						return SOCKETS_SOCKET_ERROR;
					}
				}
			}
			else
			{
				//IotLogInfo("SCOKETS_SEND 602\n");
				/* The socket has been closed for write. */
				return SOCKETS_ECLOSED;
			}
		}
		else
		{
			/* The supplied socket is closed. */
			//IotLogInfo("SCOKETS_SEND 610\n");
			return SOCKETS_ECLOSED;
		}
	}
	else
	{
		//IotLogInfo("SCOKETS_SEND 616\n");
		return SOCKETS_EINVAL;
	}

	return lSentBytes;
}
/*-----------------------------------------------------------*/

int32_t SOCKETS_Shutdown( Socket_t xSocket,
		uint32_t ulHow )
{
	uint32_t ulSocketNumber = ( uint32_t ) xSocket;

	/* Ensure that a valid socket was passed. */
	if (prvIsValidSocket(ulSocketNumber) == pdTRUE)
	{
		SSocketContextPtr_t pxContext = &xSockets[ulSocketNumber];

		switch( ulHow )
		{
			case SOCKETS_SHUT_RD:
				/* Further receive calls on this socket should return error. */
				pxContext->ulFlags |= securesocketsSOCKET_READ_CLOSED_FLAG;
				break;

			case SOCKETS_SHUT_WR:
				/* Further send calls on this socket should return error. */
				pxContext->ulFlags |= securesocketsSOCKET_WRITE_CLOSED_FLAG;
				break;

			case SOCKETS_SHUT_RDWR:
				/* Further send or receive calls on this socket should return error. */
				pxContext->ulFlags |= securesocketsSOCKET_READ_CLOSED_FLAG | securesocketsSOCKET_WRITE_CLOSED_FLAG;
				break;

			default:
				/* An invalid value was passed for ulHow. */
				return SOCKETS_EINVAL;
				break;
		}
	}
	else
	{
		return SOCKETS_EINVAL;
	}

	return SOCKETS_ERROR_NONE;
}
/*-----------------------------------------------------------*/

int32_t SOCKETS_Close( Socket_t xSocket )
{
	int32_t lRetVal = SOCKETS_SOCKET_ERROR;
	uint32_t ulSocketNumber = (uint32_t)xSocket;

	/* Ensure that a valid socket was passed. */
	if (prvIsValidSocket(ulSocketNumber) == pdTRUE)
	{
		SSocketContextPtr_t pxContext = &xSockets[ulSocketNumber];

		pxContext->ulFlags |= securesocketsSOCKET_READ_CLOSED_FLAG;
		pxContext->ulFlags |= securesocketsSOCKET_WRITE_CLOSED_FLAG;

		/* Clean-up destination string. */
		if (pxContext->pcDestination != NULL)
		{
			vPortFree(pxContext->pcDestination);
			pxContext->pcDestination = NULL;
		}

		/* Clean-up server certificate. */
		if (pxContext->pcServerCertificate != NULL)
		{
			vPortFree(pxContext->pcServerCertificate);
			pxContext->pcServerCertificate = NULL;
		}

		/* Clean-up application protocol array. */
		if (pxContext->ppcAlpnProtocols != NULL)
		{
			for (uint32_t ulProtocol = 0; ulProtocol < pxContext->ulAlpnProtocolsCount; ulProtocol++)
			{
				if (pxContext->ppcAlpnProtocols[ ulProtocol ] != NULL)
				{
					vPortFree( pxContext->ppcAlpnProtocols[ ulProtocol ] );
					pxContext->ppcAlpnProtocols[ ulProtocol ] = NULL;
				}
			}

			vPortFree(pxContext->ppcAlpnProtocols);
			pxContext->ppcAlpnProtocols = NULL;
		}

		/* Clean-up TLS context. */
		if ((pxContext->ulFlags & securesocketsSOCKET_SECURE_FLAG) != 0UL)
		{
			TLS_Cleanup(pxContext->pvTLSContext);
			pxContext->pvTLSContext = NULL;
		}

		if ((pxContext->ulFlags & securesocketsSOCKET_IS_CONNECTED) != 0UL)
		{

			if( xSemaphoreTake( xCellularSemaphoreHandle, xMaxSemaphoreBlockTime ) == pdTRUE )
			{
				lRetVal = xbee_sock_close( pxContext->xSocket );
				pxContext->ulFlags &= ~securesocketsSOCKET_IS_CONNECTED;
				(void)xSemaphoreGive( xCellularSemaphoreHandle );
			}
#ifdef NETWORK_DBG
			else
			{
				IotLogInfo("NetworkIfcErr:: close - semaphore error\n");
			}
#endif
		}
		else
		{
			lRetVal = SOCKETS_ERROR_NONE;
		}

		/* Return the socket back to the free socket pool. */
		if (prvReturnSocket(pxContext) != pdTRUE)
		{
			lRetVal = SOCKETS_SOCKET_ERROR;
		}

	}
	else
	{
		lRetVal = SOCKETS_EINVAL;
	}
#ifdef NETWORK_DBG
	IotLogInfo("NetworkIfcDbg: socket closed, retval=%d\n",lRetVal);
#endif
	return lRetVal;
}
/*-----------------------------------------------------------*/

int32_t SOCKETS_SetSockOpt( Socket_t xSocket,
		int32_t lLevel,
		int32_t lOptionName,
		const void * pvOptionValue,
		size_t xOptionLength )
{
	int32_t lRetCode = SOCKETS_ERROR_NONE;
	uint32_t ulSocketNumber = ( uint32_t ) xSocket;
	TickType_t xTimeout;

	/* Remove compiler warnings about unused parameters. */
	(void)lLevel;

	/* Ensure that a valid socket was passed. */
	if (prvIsValidSocket(ulSocketNumber) == pdTRUE)
	{
		SSocketContextPtr_t pxContext = &xSockets[ulSocketNumber];

		switch( lOptionName )
		{
			case SOCKETS_SO_SERVER_NAME_INDICATION:
				if ((pxContext->ulFlags & securesocketsSOCKET_IS_CONNECTED) == 0)
				{
					/* Non-NULL destination string indicates that SNI extension should
					 * be used during TLS negotiation. */
					pxContext->pcDestination = ( char * ) pvPortMalloc( 1U + xOptionLength );

					if (pxContext->pcDestination == NULL)
					{
						lRetCode = SOCKETS_ENOMEM;
					}
					else
					{
						if (pvOptionValue != NULL)
						{
							memcpy(pxContext->pcDestination, pvOptionValue, xOptionLength );
							pxContext->pcDestination[ xOptionLength ] = '\0';
						}
						else
						{
							lRetCode = SOCKETS_EINVAL;
						}
					}
				}
				else
				{
					lRetCode = SOCKETS_EISCONN;
				}

				break;

			case SOCKETS_SO_TRUSTED_SERVER_CERTIFICATE:
				if ((pxContext->ulFlags & securesocketsSOCKET_IS_CONNECTED) == 0)
				{
					/* Non-NULL server certificate field indicates that the default trust
					 * list should not be used. */
					pxContext->pcServerCertificate = ( char * ) pvPortMalloc( xOptionLength );

					if (pxContext->pcServerCertificate == NULL)
					{
						lRetCode = SOCKETS_ENOMEM;
					}
					else
					{
						if (pvOptionValue != NULL)
						{
							memcpy( pxContext->pcServerCertificate, pvOptionValue, xOptionLength );
							pxContext->ulServerCertificateLength = xOptionLength;
						}
						else
						{
							lRetCode = SOCKETS_EINVAL;
						}
					}
				}
				else
				{
					lRetCode = SOCKETS_EISCONN;
				}

				break;

			case SOCKETS_SO_REQUIRE_TLS:
				if ((pxContext->ulFlags & securesocketsSOCKET_IS_CONNECTED) == 0)
				{
					pxContext->ulFlags |= securesocketsSOCKET_SECURE_FLAG;
				}
				else
				{
					/* Do not set the ALPN option if the socket is already connected. */
					lRetCode = SOCKETS_EISCONN;
				}
				break;

			case SOCKETS_SO_ALPN_PROTOCOLS:
				if ((pxContext->ulFlags & securesocketsSOCKET_IS_CONNECTED) == 0)
				{
					/* Allocate a sufficiently long array of pointers. */
					pxContext->ulAlpnProtocolsCount = 1 + xOptionLength;

					if (NULL == (pxContext->ppcAlpnProtocols = (char **)pvPortMalloc(pxContext->ulAlpnProtocolsCount * sizeof(char *))))
					{
						lRetCode = SOCKETS_ENOMEM;
					}
					else
					{
						pxContext->ppcAlpnProtocols[pxContext->ulAlpnProtocolsCount - 1 ] = NULL;
					}

					/* Copy each protocol string. */
					for (uint32_t ulProtocol = 0; (ulProtocol < pxContext->ulAlpnProtocolsCount - 1) && (lRetCode == pdFREERTOS_ERRNO_NONE); ulProtocol++)
					{
						char ** ppcAlpnIn = ( char ** ) pvOptionValue;
						size_t xLength = strlen(ppcAlpnIn[ ulProtocol ]);

						if ((pxContext->ppcAlpnProtocols[ ulProtocol ] = (char *)pvPortMalloc(1 + xLength)) == NULL)
						{
							lRetCode = SOCKETS_ENOMEM;
						}
						else
						{
							memcpy( pxContext->ppcAlpnProtocols[ ulProtocol ], ppcAlpnIn[ ulProtocol ], xLength );
							pxContext->ppcAlpnProtocols[ ulProtocol ][ xLength ] = '\0';
						}
					}
				}
				else
				{
					/* Do not set the ALPN option if the socket is already connected. */
					lRetCode = SOCKETS_EISCONN;
				}

				break;

			case SOCKETS_SO_SNDTIMEO:
				break;

			case SOCKETS_SO_RCVTIMEO:
				xTimeout = *( ( const TickType_t * ) pvOptionValue ); /*lint !e9087 pvOptionValue passed should be of TickType_t. */
				/*RUTVIJ : FIXME */

				break;

			case SOCKETS_SO_NONBLOCK:
				if ((pxContext->ulFlags & securesocketsSOCKET_IS_CONNECTED ) != 0)
				{
					/* Set the timeouts to the smallest value possible.
					 * This isn't true nonblocking, but as close as we can get. */
					//esp_netconn_set_receive_timeout(pxContext->xSocket, 1);
					/*RUTVIJ : FIXME */
				}
				else
				{
					lRetCode = SOCKETS_EISCONN;
				}
				break;

			default:
				lRetCode = SOCKETS_EINVAL;
		}
	}
	else
	{
		lRetCode = SOCKETS_EINVAL;
	}

	return lRetCode;
}
/*-----------------------------------------------------------*/

/* RUTVIJ : reference code
 * from cloud xbee demo from silabs check http_client.c
 * uint32_t resolveFQDN(xbee_dev_t *xbee, char *fqdnStr)
 * */
uint32_t SOCKETS_GetHostByName(const char * pcHostName)
{

#ifdef NETWORK_DBG
	IotLogInfo("NetworkIfcDbg: SOCKETS_GetHostByName\n");
	IotLogInfo("NetworkIfcDbg: %s\n",pcHostName);
#endif
	uint32_t ret = 0;

	/* take semaphore */
	if( xSemaphoreTake( xCellularSemaphoreHandle, xMaxSemaphoreBlockTime ) == pdTRUE )
	{
		xbee_dev_t  *xbee_instance = NULL ;
		xbee_instance = &Xbee_dev;
		if(xbee_instance != NULL)
		{
			/* Retrive IP address of AWS */
			ret = resolveFQDN(xbee_instance, pcHostName);

			/* RUTVIJ : FIXME do we need to convert in network byte order ?*/
#ifdef NETWORK_DBG
			//IotLogInfo("NetworkIfcDbg: Resolved ipAddr = %d\n",ret);
#endif
		}
		/*Give semaphore back*/
		( void ) xSemaphoreGive( xCellularSemaphoreHandle);
	}
	else
	{
		IotLogInfo("NetworkIfcErr: SOCKETS_GetHostByName semaphore error\n");
	}

	return ret;
}
/*-----------------------------------------------------------*/

BaseType_t SOCKETS_Init( void )
{
	uint32_t ulIndex;
	BaseType_t ret = pdFAIL;

	/* Mark all the sockets as free */
	for (ulIndex = 0 ; ulIndex < (uint32_t)XBEE_SOCK_SOCKET_COUNT ; ulIndex++)
	{
		xSockets[ ulIndex ].ucInUse = 0;
		xSockets[ ulIndex ].ulFlags = 0;

		xSockets[ ulIndex ].ulFlags |= securesocketsSOCKET_READ_CLOSED_FLAG;
		xSockets[ ulIndex ].ulFlags |= securesocketsSOCKET_WRITE_CLOSED_FLAG;
	}

	/* Create the global mutex which is used to ensure
	 * that only one socket is accessing the ucInUse bits in
	 * the socket array. */
	xUcInUse = xSemaphoreCreateMutex();
	if (xUcInUse == NULL)
	{
		return pdFAIL;
	}

	/* Create the global mutex which is used to ensure
	 * that only one socket is accessing the ucInUse bits in
	 * the socket array. */
	xTLSConnect = xSemaphoreCreateMutex();
	if (xTLSConnect == NULL)
	{
		return pdFAIL;
	}

	taskENTER_CRITICAL();

	xbee_dev_t  *xbee_instance = NULL ;
	xbee_instance = &Xbee_dev;
	if(xbee_instance != NULL)
	{
		// make sure there aren't any open sockets on <xbee> from a previous program
		if(xbee_sock_reset(xbee_instance) == 0)
		{
			ret = pdPASS;
		}
	}
	taskEXIT_CRITICAL();

	return ret;
}
/*-----------------------------------------------------------*/
uint32_t ulRand( void )
{
	CK_RV xResult = 0;
	CK_C_GetFunctionList pxCkGetFunctionList = NULL;
	CK_ULONG ulCount = 1;
	uint32_t ulRandomValue = 0;
	CK_SLOT_ID xSlotId = 0;

	portENTER_CRITICAL();

	if( 0 == xPkcs11Session )
	{
		/* One-time initialization. */

		/* Ensure that the PKCS#11 module is initialized. */
		if( 0 == xResult )
		{
			pxCkGetFunctionList = C_GetFunctionList;
			xResult = pxCkGetFunctionList( &pxPkcs11FunctionList );
		}

		if( 0 == xResult )
		{
			xResult = pxPkcs11FunctionList->C_Initialize( NULL );
		}

		/* Get the default slot ID. */
		if( 0 == xResult )
		{
			xResult = pxPkcs11FunctionList->C_GetSlotList(
					CK_TRUE,
					&xSlotId,
					&ulCount );
		}

		/* Start a session with the PKCS#11 module. */
		if( 0 == xResult )
		{
			xResult = pxPkcs11FunctionList->C_OpenSession(
					xSlotId,
					CKF_SERIAL_SESSION,
					NULL,
					NULL,
					&xPkcs11Session );
		}
	}

	if( 0 == xResult )
	{
		/* Request a sequence of cryptographically random byte values using
		 * PKCS#11. */
		xResult = pxPkcs11FunctionList->C_GenerateRandom( xPkcs11Session,
				( CK_BYTE_PTR ) &ulRandomValue,
				sizeof( ulRandomValue ) );
	}

	portEXIT_CRITICAL();

	/* Check if any of the API calls failed. */
	if( 0 != xResult )
	{
		ulRandomValue = 0;
	}

	return ulRandomValue;
}
#endif //test enabled macro

#if 0 //Xbee direct Without TLS

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
			IotLogInfo("NetworkIfcErr:: Protocol not supported\n");
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
	IotLogInfo("NetworkIfcDbg: SOCKETS_Socket-ulSocketNumber=%d\n",ulSocketNumber);
#endif
	return ulSocketNumber;
}
/*-----------------------------------------------------------*/

Socket_t SOCKETS_Accept( Socket_t xSocket,
		SocketsSockaddr_t * pxAddress,
		Socklen_t * pxAddressLength )
{
#ifdef NETWORK_DBG
	IotLogInfo("NetworkIfcDbg:SOCKETS_Accept\n");
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
	IotLogInfo("NetworkIfcDbg:Remote_addr:%d,port:%d\n",pxAddress->ulAddress,
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
			IotLogInfo("NetworkIfcErr:: connect - %d\n",lRetVal);
			lRetVal = SOCKETS_SOCKET_ERROR;
		}
		( void ) xSemaphoreGive( xCellularSemaphoreHandle );
	}
#ifdef NETWORK_DBG
	else
	{
		IotLogInfo("NetworkIfcErr:: connect - semaphore error\n");
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
			IotLogInfo("NetworkIfcErr: Receive \n");
			return SOCKETS_SOCKET_ERROR;
		}
		( void ) xSemaphoreGive( xCellularSemaphoreHandle );
		return receivedBytes;
	}
#ifdef NETWORK_DBG
	else
	{
		IotLogInfo("NetworkIfcErr:: receive - semaphore error\n");
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
			IotLogInfo("NetworkIfcErr:: send-%d\n",lSentBytes);
			lSentBytes = SOCKETS_SOCKET_ERROR;
		}
		( void ) xSemaphoreGive( xCellularSemaphoreHandle );
	}
#ifdef NETWORK_DBG
	else
	{
		IotLogInfo("NetworkIfcErr:: send - semaphore error\n");
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
			IotLogInfo("NetworkIfcDbg: SOCKETS_SHUT_RD request \n");
#endif
			/* Return success to the user. */
			lRetVal = SOCKETS_ERROR_NONE;
			break;

		case SOCKETS_SHUT_WR:
#ifdef NETWORK_DBG
			IotLogInfo("NetworkIfcDbg: SOCKETS_SHUT_WR request \n");
#endif
			/* Return success to the user. */
			lRetVal = SOCKETS_ERROR_NONE;
			break;

		case SOCKETS_SHUT_RDWR:
#ifdef NETWORK_DBG
			IotLogInfo("NetworkIfcDbg: SOCKETS_SHUT_RDWR request \n");
#endif
			/* Return success to the user. */
			lRetVal = SOCKETS_ERROR_NONE;
			break;

		default:
			IotLogInfo("NetworkIfcDbg: shutdown request Invalid\n");
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
		IotLogInfo("NetworkIfcErr:: close - semaphore error\n");
	}
	IotLogInfo("NetworkIfcDbg: socket closed, retval=%d\n",lRetVal);
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
	IotLogInfo("NetworkIfcDbg: SOCKETS_SetSockOpt -"
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
	IotLogInfo("NetworkIfcDbg: SOCKETS_GetHostByName\n");
#endif
	/*RUTVIJ: Can do DNS lookup for WiFi */
	return 0;
}
/*-----------------------------------------------------------*/

BaseType_t SOCKETS_Init( void )
{
#ifdef NETWORK_DBG
	IotLogInfo("NetworkIfcDbg: SOCKETS_Init\n");
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
	IotLogInfo("NetworkIfcDbg: ulRand\n");
#endif

	/* FIX ME. */
	return 0;
}
/*-----------------------------------------------------------*/
/*RUTVIJ: may not needed below */
BaseType_t xApplicationGetRandomNumber( uint32_t * pulNumber )
{
#ifdef NETWORK_DBG
	IotLogInfo("NetworkIfcDbg: xApplicationGetRandomNumber\n");
#endif

	*( pulNumber ) = 0uL;
	return pdFALSE;
}
#endif // xbee wihtout TLS
