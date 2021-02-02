#if 0 // enable to test xbee socket sample client model
/*
 * Copyright (c) 2019 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
 * ===========================================================================
 */

/*
    Sample: XBee Netcat

    A version of the netcat (nc) tool that uses the XBee sockets API to open a
    TCP or UDP socket and pass its data to/from stdout/stdin.
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "xbee/byteorder.h"
#include "xbee/device.h"
#include "xbee/ipv4.h"
#include "xbee/socket.h"


typedef enum netcat_state {
    NETCAT_STATE_INIT,
    NETCAT_STATE_SEND_ATAI,
    NETCAT_STATE_WAIT_FOR_ATAI,
    NETCAT_STATE_GOT_ATAI,
    NETCAT_STATE_SEND_ATMY,
    NETCAT_STATE_WAIT_FOR_ATMY,
    NETCAT_STATE_GOT_ATMY,
    NETCAT_STATE_WAIT_FOR_CREATE,
    NETCAT_STATE_CREATED,
    NETCAT_STATE_WAIT_FOR_ATTACH,
    NETCAT_STATE_WAIT_FOR_CONNECT,
    NETCAT_STATE_CONNECTED,
    NETCAT_STATE_INTERACTIVE,
    NETCAT_STATE_WAIT_FOR_CLOSE,
    NETCAT_STATE_DONE,
    NETCAT_STATE_ERROR
} netcat_state_t;

static netcat_state_t netcat_state;
uint8_t netcat_atai = 0;
uint32_t netcat_atmy = 0;
xbee_sock_t netcat_listen = 0;
xbee_sock_t netcat_socket = 0;

static void netcat_sock_notify_cb(xbee_sock_t socket,
                                  uint8_t frame_type, uint8_t message)
{
#if XBEE_TX_DELIVERY_STR_BUF_SIZE > XBEE_SOCK_STR_BUF_SIZE
    char buffer[XBEE_TX_DELIVERY_STR_BUF_SIZE];
#else
    char buffer[XBEE_SOCK_STR_BUF_SIZE];
#endif

    // make sure it's a status for our sockets
    if (socket != netcat_socket && socket != netcat_listen) {
        return;
    }

    switch (frame_type) {
    case XBEE_FRAME_TX_STATUS:          // message is an XBEE_TX_DELIVERY_xxx
        if (message != 0) {/*
            printf("%s: %s\n", "XBEE_TX_DELIVERY",
                   xbee_tx_delivery_str(message, buffer));*/
        }
        break;

    case XBEE_FRAME_SOCK_STATE:         // message is an XBEE_SOCK_STATE_xxx
        if (netcat_state == NETCAT_STATE_WAIT_FOR_CONNECT && message == 0) {
            netcat_state = NETCAT_STATE_CONNECTED;
        } else if (message != 0) {
            // ignore errors on listening socket
            if (socket == netcat_socket) {
                netcat_state = message == XBEE_SOCK_STATE_TRANSPORT_CLOSED
                    ? NETCAT_STATE_DONE : NETCAT_STATE_ERROR;
            }
            printf("%s: %s\n", "XBEE_SOCK_STATE",
                   xbee_sock_state_str(message, buffer));
        }
        break;

    case XBEE_FRAME_SOCK_CREATE_RESP:
        if (message == 0) {
            if (netcat_state == NETCAT_STATE_WAIT_FOR_CREATE) {
                netcat_state = NETCAT_STATE_CREATED;
            }
        } else {
            printf("%s: %s\n", "XBEE_SOCK_CREATE",
                   xbee_sock_status_str(message, buffer));
            netcat_state = NETCAT_STATE_ERROR;
        }
        break;

    case XBEE_FRAME_SOCK_CONNECT_RESP:
    case XBEE_FRAME_SOCK_LISTEN_RESP:
        if (message == 0) {
            if (netcat_state == NETCAT_STATE_WAIT_FOR_ATTACH) {
                netcat_state = NETCAT_STATE_WAIT_FOR_CONNECT;
            }
        } else {
            printf("%s: %s\n", "XBEE_SOCK_CONNECT/LISTEN",
                    xbee_sock_status_str(message, buffer));
            netcat_state = NETCAT_STATE_ERROR;
        }
        break;

    default:
        // we don't care about other possible notifications
        break;
    }

}


static void netcat_sock_receive_cb(xbee_sock_t socket, uint8_t status,
                                   const void *payload, size_t payload_length)
{

    // Since payload isn't null-terminated, use ".*s" to limit length.
    printf("%.*s", (int)payload_length, (const char *)payload);

}


xbee_sock_receive_fn netcat_sock_ipv4_client_cb(xbee_sock_t listening_socket,
                                                xbee_sock_t client_socket,
                                                uint32_t remote_addr,
                                                uint16_t remote_port)
{
    char ipbuf[16];

   // xbee_ipv4_ntoa(ipbuf, htobe32(remote_addr));
    printf("Inbound connection from %s:%u...\n", ipbuf, remote_port);

    netcat_socket = client_socket;

    if (netcat_state == NETCAT_STATE_WAIT_FOR_CONNECT) {
        netcat_state = NETCAT_STATE_INTERACTIVE;
    }

    return &netcat_sock_receive_cb;
}

static struct {
    uint32_t addr;
    uint16_t port;
} netcat_last_datagram = { 0, 0 };

static void netcat_sock_receive_from_cb(xbee_sock_t socket, uint8_t status,
                                        uint32_t remote_addr,
                                        uint16_t remote_port,
                                        const void *datagram,
                                        size_t datagram_length)
{
    char ipbuf[16];

    netcat_last_datagram.addr = remote_addr;
    netcat_last_datagram.port = remote_port;

    //xbee_ipv4_ntoa(ipbuf, htobe32(remote_addr));
    // Since payload isn't null-terminated, use ".*s" to limit length.
    printf("Datagram from %s:%u:\n%.*s",
           ipbuf, remote_port, (int)datagram_length, (const char *)datagram);

    if (netcat_state == NETCAT_STATE_WAIT_FOR_CONNECT) {
        netcat_state = NETCAT_STATE_INTERACTIVE;
    }
}


int netcat_atcmd_callback( const xbee_cmd_response_t FAR *response)
{
    if ((response->flags & XBEE_CMD_RESP_MASK_STATUS) != XBEE_AT_RESP_SUCCESS) {
        printf("ERROR: sending AT%.2" PRIsFAR "\n",
                response->command.str);
        return XBEE_ATCMD_DONE;
    }

    switch (netcat_state) {
    case NETCAT_STATE_WAIT_FOR_ATAI:
        if (memcmp(response->command.str, "AI", 2) == 0) {
            netcat_atai = (uint8_t)response->value;
            netcat_state = NETCAT_STATE_GOT_ATAI;
        }
        break;
    case NETCAT_STATE_WAIT_FOR_ATMY:
        if (memcmp(response->command.str, "MY", 2) == 0) {
            netcat_atmy = response->value;
            netcat_state = NETCAT_STATE_GOT_ATMY;
        }
        break;
    default:
        // not our AT command, ignore result
        break;
    }

    return XBEE_ATCMD_DONE;
}

int netcat_atcmd(xbee_dev_t *xbee, const char *cmd)
{
    int request;

    request = xbee_cmd_create(xbee, cmd);
    if (request < 0) {
        printf("ERROR: couldn't create AT%s command (%d)\n",
                cmd, request);
    } else {
        xbee_cmd_set_callback(request, netcat_atcmd_callback, NULL);
        xbee_cmd_send(request);
    }

    return request;
}

/// Bitfields for \a flags parameter to netcat_simple()
#define NETCAT_FLAG_CRLF        (1<<0)  ///< send CR/LF for EOL sequence
#define NETCAT_FLAG_LISTEN      (1<<1)  ///< listen for incoming connections
#define NETCAT_FLAG_UDP         (1<<2)  ///< use UDP instead of TCP
/**
    @brief
    Pass data between a socket and stdout/stdin.

    @param[in]  xbee            XBee device for connection
    @param[in]  flags           One or more of the following:
        NETCAT_FLAG_CRLF        send CR/LF for EOL sequence
        NETCAT_FLAG_LISTEN      listen for incoming connection
        NETCAT_FLAG_UDP         use UDP instead of TCP
    @param[in]  remote_host     Remote host for connection, if not listening.
    @param[in]  remote_port     Port of remote host, if not listening.

    @retval     EXIT_SUCCCESS   clean exit from function
    @retval     EXIT_FAILURE    exited due to some failure
*/
int netcat_sample(xbee_dev_t *xbee, unsigned flags, uint16_t source_port,
                  const char *remote_host, uint16_t remote_port)
{
    int result;
    int last_ai = -1;
    char linebuf[256];

   // xbee_term_console_init();

    netcat_state = NETCAT_STATE_INIT;
    printf("Waiting for Internet connection...\n");
    while (netcat_state < NETCAT_STATE_DONE) {
        result = xbee_dev_tick(xbee);
        if (result < 0) {
            printf("Error %d calling xbee_dev_tick().\n", result);
            return EXIT_FAILURE;
        }
        switch (netcat_state) {
        case NETCAT_STATE_INIT:
            // fall through to sending ATAI to check for online status
        case NETCAT_STATE_SEND_ATAI:
            if (netcat_atcmd(xbee, "AI") >= 0) {
                netcat_state = NETCAT_STATE_WAIT_FOR_ATAI;
            }
            break;

        case NETCAT_STATE_WAIT_FOR_ATAI:
            break;

        case NETCAT_STATE_GOT_ATAI:
            // default action is to send ATAI until we get a 0 response
            netcat_state = NETCAT_STATE_SEND_ATAI;
            if (last_ai != netcat_atai) {
                if (netcat_atai == 0) {
                    printf("Cellular module connected...\n");
                    netcat_state = NETCAT_STATE_SEND_ATMY;
                } else {
                    printf("ATAI change from 0x%02X to 0x%02X\n",
                            last_ai, netcat_atai);
                    last_ai = netcat_atai;
                }
            }
            break;

        case NETCAT_STATE_SEND_ATMY:
            if (netcat_atcmd(xbee, "MY") >= 0) {
                netcat_state = NETCAT_STATE_WAIT_FOR_ATMY;
            }
            break;

        case NETCAT_STATE_WAIT_FOR_ATMY:
            break;

        case NETCAT_STATE_GOT_ATMY:
            xbee_ipv4_ntoa(linebuf, htobe32(netcat_atmy));
            printf("My IP is %s\n", linebuf);
            int proto =  XBEE_SOCK_PROTOCOL_TCP;
            netcat_socket = xbee_sock_create(xbee, proto,
                                             netcat_sock_notify_cb);
            if(netcat_socket > 0 )
            		printf("RUTVIJ socket create local response %d \n",netcat_socket);
            netcat_state = NETCAT_STATE_WAIT_FOR_CREATE;
            break;

        case NETCAT_STATE_WAIT_FOR_CREATE:
            break;

        case NETCAT_STATE_CREATED:

            printf("Created socket...\n");
            //flags = 0;
            if (flags & NETCAT_FLAG_LISTEN) {
                if (flags & NETCAT_FLAG_UDP) {
                    // bind UDP socket
                    printf("Binding to port %u...", source_port);
                    result = xbee_sock_bind(netcat_socket, source_port,
                                            netcat_sock_receive_from_cb);
                } else {
                    // listen for TCP connections
                    netcat_listen = netcat_socket;
                    netcat_socket = 0;
                    printf("Listening on port %u...", source_port);
                    result = xbee_sock_listen(netcat_listen, source_port,
                                              netcat_sock_ipv4_client_cb);
                }
            } else {
                printf("Connecting to %s:%u...",
                        remote_host, remote_port);
                result = xbee_sock_connect(netcat_socket, remote_port, 0,
                                           remote_host, netcat_sock_receive_cb);
            }

            if (result) {
                printf("error %d.\n", result);
                netcat_state = NETCAT_STATE_ERROR;
            } else {
                printf("success.\n");
                netcat_state = NETCAT_STATE_WAIT_FOR_ATTACH;
            }
            break;

        case NETCAT_STATE_WAIT_FOR_ATTACH:
            break;

        case NETCAT_STATE_WAIT_FOR_CONNECT:
            break;

        case NETCAT_STATE_CONNECTED:

            printf("Connected to %s:%u\n", remote_host, remote_port);

            netcat_state = NETCAT_STATE_INTERACTIVE;
            break;

        case NETCAT_STATE_INTERACTIVE:
            // leave room at end of linebuf for CRLF or LF
           result =  2 ;//xbee_readline(linebuf, sizeof linebuf - 2);
            if (result == -ENODATA) { // reached EOF

                printf("(got EOF, waiting for socket closure)\n");
                netcat_state = NETCAT_STATE_WAIT_FOR_CLOSE;
            } else if (result >= 0) { // send data to remote
                if (flags & NETCAT_FLAG_CRLF) {
                    linebuf[result++] = '\r';
                }
                linebuf[result++] = '\n';

                if ((flags & NETCAT_FLAG_UDP) && (flags & NETCAT_FLAG_LISTEN)) {
                    // bound UDP socket uses sendto
                    xbee_sock_sendto(netcat_socket, 0,
                                     netcat_last_datagram.addr,
                                     netcat_last_datagram.port,
                                     linebuf, result);
                } else {
                    xbee_sock_send(netcat_socket, 0, linebuf, result);
                }
            }
            break;

        case NETCAT_STATE_WAIT_FOR_CLOSE:
            // drain any input from keyboard/stdin
            //xbee_readline(linebuf, sizeof linebuf);
            break;

        case NETCAT_STATE_DONE:
            break;

        case NETCAT_STATE_ERROR:
            break;
        }
    }

   // xbee_term_console_restore();
    return netcat_state == NETCAT_STATE_ERROR ? EXIT_FAILURE : EXIT_SUCCESS;
}

void print_help(void)
{
    puts("XBee Netcat");
    puts("Usage: xbee_netcat [options] [hostname] [port]");
    puts("");
    puts("  -b baudrate           Baudrate for XBee interface (default 115200)");
    puts("  -C                    Use CRLF for EOL sequence");
    puts("  -h                    Display this help screen");
    puts("  -p port               Specify source port to bind to/listen on");
    puts("  -u                    Use UDP instead of default TCP");
    puts("  -x serialport         Serial port for XBee interface");
    puts("                        ('COMxx' for Win32, '/dev/ttySxx' for POSIX)");
    puts("");
}

// returns ULONG_MAX if unable to completely parse <str> or <str> is empty
unsigned long parse_num(const char *str)
{
    char *endptr;
    unsigned long parsed_num = strtoul(str, &endptr, 0);

    return *str == '\0' || *endptr != '\0' ? ULONG_MAX : parsed_num;
}

//int main(int argc, char *argv[])
int sample_xbee_socket(xbee_dev_t xbee_dev)
{

    int c;

    // parsed command-line options with default values
    xbee_serial_t xbee_serial = { .baudrate = 115200 };
    int source_port = -1;
    unsigned opt_flags = 0;
    int parse_error = 0;
    //const char *hostname = "184.96.65.243";
    const char *hostname = "127.0.0.1";
    uint16_t remote_port = 8080;
    /*
    if (xbee_dev_init(&xbee_dev, &xbee_serial, NULL, NULL)) {
        printf("ERROR: Failed to initialize serial connection.\n");
        return EXIT_FAILURE;
    }*/

    // make sure there aren't any open sockets on <xbee> from a previous program
    xbee_sock_reset(&xbee_dev);

    int result = netcat_sample(&xbee_dev, opt_flags, source_port,
                               hostname, remote_port);

    printf("Closing serial port...\n");
    xbee_ser_close(&xbee_serial);

    return result;
}

const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_SOCK_FRAME_HANDLERS,
    XBEE_FRAME_TABLE_END
};
#endif



















#if 0// socket sample test
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

//uint8_t host_addr[] = "184.96.65.243";
#define xbeetestECHO_SERVER_ADDR0         184
#define xbeetestECHO_SERVER_ADDR1         96
#define xbeetestECHO_SERVER_ADDR2         65
#define xbeetestECHO_SERVER_ADDR3         243

#define xbeePORT  						(801)
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

Socket_t ulSocketNumber = -1;
xbee_sock_t s = -1;
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


	xbee_dev_t  *xbee_instance = NULL;
	xbee_instance = &Xbee_dev;
	if(xbee_instance != NULL)
	{
		/* Create local socket , active socket created when handler notifies */
		s = xbee_sock_create( xbee_instance,
				Xprotocol,
				&notify_callback );
		ulSocketNumber = 1;
	}

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
	if(s < 0)
		return -1;

	/* Removed warning */
	/* wait for semaphore to  be taken till lte max time config */
	uint32_t lRetVal = xbee_sock_connect( s,
			SOCKETS_ntohs( pxAddress->usPort ),
			0,
			pxAddress->ulAddress,
			&receive_callback );
	if( lRetVal < 0 )
	{
		printf("NetworkIfcErr:: connect - %d\n",lRetVal);
		lRetVal = SOCKETS_SOCKET_ERROR;
	}
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
	if(s < 0)
		return -1;
	receivedBytes = xbee_sock_recv( s,
			pvBuffer,
			xBufferLength,
			ulFlags );

	if(receivedBytes < 0)
	{
		printf("NetworkIfcErr: Receive \n");
		return SOCKETS_SOCKET_ERROR;
	}
	return receivedBytes;
}
/*-----------------------------------------------------------*/

int32_t SOCKETS_Send( Socket_t xSocket,
		const void * pvBuffer,
		size_t xDataLength,
		uint32_t ulFlags )
{
	int32_t lSentBytes = SOCKETS_SOCKET_ERROR;
	if(s < 0)
		return -1;
	/* RUTVIJ: Check for flags if we are allowed to send or not */
	/* RUTVIJ: send un-encrypted data - convert to TLS*/
	lSentBytes = xbee_sock_send( s,
			(uint8_t)ulFlags,
			pvBuffer,
			xDataLength);
	if(lSentBytes < 0)
	{
		printf("NetworkIfcErr:: send-%d\n",lSentBytes);
		lSentBytes = SOCKETS_SOCKET_ERROR;
	}

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
	if(s < 0)
		return -1;

	uint32_t lRetVal = xbee_sock_close( s );
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
	if(s < 0)
		return -1;
	lRetVal = xbee_sock_option( s,
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
	BaseType_t ret = pdPASS;
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


void socket_comm_test(void)
{
	//uint32_t port = 801;
	//uint8_t host_addr[] = "184.96.65.243";
	char * hello = "hello world \n";
	SocketsSockaddr_t xEchoServerAddress;
 	xEchoServerAddress.usPort = SOCKETS_htons( xbeePORT );
	xEchoServerAddress.ulAddress = SOCKETS_inet_addr_quick( xbeetestECHO_SERVER_ADDR0,
                                                                    xbeetestECHO_SERVER_ADDR1,
                                                                    xbeetestECHO_SERVER_ADDR2,
                                                                    xbeetestECHO_SERVER_ADDR3 );
	char buffer[100] = {0};

	int sock  = SOCKETS_Socket(SOCKETS_AF_INET, SOCKETS_SOCK_STREAM, SOCKETS_IPPROTO_TCP);
	SOCKETS_Connect(sock,  &xEchoServerAddress, sizeof( xEchoServerAddress));
	SOCKETS_Send(sock , hello , strlen(hello) , 0 ); 
	printf("Hello message sent\n");
	int valread = SOCKETS_Recv( sock , buffer, sizeof(buffer),0);

	printf("%s\n",buffer );
	SOCKETS_Close(sock);
}
#endif// xbee socket sample test
