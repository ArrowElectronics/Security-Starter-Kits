

/*
 * FreeRTOS V202002.00
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
 * @file iot_demo_freertos.c
 * @brief Generic demo runner for C SDK libraries on FreeRTOS.
 */

/* The config header is always included first. */
#include "iot_config.h"

#include <string.h>
#include "aws_clientcredential.h"
#include "aws_clientcredential_keys.h"
#include "iot_demo_logging.h"
#include "platform/iot_threads.h"
#include "aws_demo.h"
#include "iot_init.h"
#include "platform/iot_network_freertos.h"
#include "platform/iot_clock.h"

/* Remove dependency to MQTT */
#if ( defined( CONFIG_MQTT_DEMO_ENABLED ) || defined( CONFIG_SHADOW_DEMO_ENABLED ) || defined( CONFIG_DEFENDER_DEMO_ENABLED ) || defined( CONFIG_OTA_UPDATE_DEMO_ENABLED ) )
    #define MQTT_DEMO_TYPE_ENABLED
#endif

#if defined( MQTT_DEMO_TYPE_ENABLED )
    #include "iot_mqtt.h"
#endif

/**
 * @brief Maximum number of connection re-tries.
 */
#define demoWIFI_CONNECTION_RETRIES                 ( 5 )

/**
 * @brief Minimum time interval between two consecutive WiFi connection
 * re-tries.
 */
#define demoWIFI_CONNECTION_RETRY_INTERVAL_MS       ( 1000 )


static int enableLTE()
{
	return EXIT_SUCCESS;
}

/*-----------------------------------------------------------*/
static int initialize( void  )
{
    int status = EXIT_SUCCESS;

    /* Initialize common libraries required by demo. */
    if( IotSdk_Init() == true )
    {
        status = enableLTE();

        if( status == EXIT_FAILURE )
        {
             IotSdk_Cleanup();
        }
    }
    else
    {
        IotLogError( "Failed to initialize IoT SDK." );
        status = EXIT_FAILURE;
    }

    return status;
}

/*-----------------------------------------------------------*/

/**
 * @brief Clean up the common libraries and the MQTT library.
 */
static void cleanup( void )
{
    IotSdk_Cleanup();
}

/*-----------------------------------------------------------*/

void runDemoTask( void * pArgument )
{
    /* On FreeRTOS, credentials and server info are defined in a header
     * and set by the initializers. */

    demoFunction_t demoEntryPoint = ( demoFunction_t ) pArgument;
    const IotNetworkInterface_t * pNetworkInterface = NULL;
    IotNetworkServerInfo_t connectionParams;
    IotNetworkCredentials_t credentials;
    int status;

    #ifdef INSERT_DELAY_BEFORE_DEMO
        {
            /* DO NOT EDIT - The test framework relies on this delay to ensure that
             * the "STARTING DEMO" tag below is not missed while the framework opens
             * the serial port for reading output.*/
            vTaskDelay( pdMS_TO_TICKS( 5000UL ) );
        }
    #endif /* INSERT_DELAY_BEFORE_DEMO */

    #ifdef democonfigMEMORY_ANALYSIS
        democonfigMEMORY_ANALYSIS_STACK_DEPTH_TYPE xBeforeDemoTaskWaterMark, xAfterDemoTaskWaterMark = 0;
        xBeforeDemoTaskWaterMark = democonfigMEMORY_ANALYSIS_STACK_WATERMARK( NULL );

        size_t xBeforeDemoHeapSize, xAfterDemoHeapSize = 0;
        xBeforeDemoHeapSize = democonfigMEMORY_ANALYSIS_MIN_EVER_HEAP_SIZE();
    #endif /* democonfigMEMORY_ANALYSIS */


    /* DO NOT EDIT - This demo start marker is used in the test framework to
     * determine the start of a demo. */

    IotLogInfo( "---------STARTING DEMO---------\n" );

    status = initialize();

    if( status == EXIT_SUCCESS )
    {
        //IotLogInfo( "Successfully initialized the demo." );
        //printf( "Successfully initialized the demo.\n" );

        pNetworkInterface = IOT_NETWORK_INTERFACE_AFR;
#ifdef USER_INPUT_ARN
        connectionParams.pHostName = Endpoint;
#else
        connectionParams.pHostName = clientcredentialMQTT_BROKER_ENDPOINT;
#endif
        connectionParams.port = clientcredentialMQTT_BROKER_PORT;

        if( connectionParams.port == 443 )
        {
            credentials.pAlpnProtos = socketsAWS_IOT_ALPN_MQTT;
        }
        else
        {
            credentials.pAlpnProtos = NULL;
        }
        credentials.maxFragmentLength = 0;
        credentials.disableSni = false;
        credentials.pRootCa = NULL;//keyJITR_DEVICE_CERTIFICATE_AUTHORITY_PEM;
        credentials.rootCaSize = 0;//sizeof(keyJITR_DEVICE_CERTIFICATE_AUTHORITY_PEM);
        credentials.pClientCert = NULL;//keyCLIENT_CERTIFICATE_PEM;
        credentials.clientCertSize = 0;//sizeof( keyCLIENT_CERTIFICATE_PEM );
        credentials.pPrivateKey = NULL;
        credentials.privateKeySize = 0;// sizeof( keyCLIENT_PRIVATE_KEY_PEM );


        /* Run the demo. */
        status = demoEntryPoint( true,
                                 clientcredentialIOT_THING_NAME,
                                 &( connectionParams ),
                                 &( credentials ),
                                 pNetworkInterface );

        #ifdef democonfigMEMORY_ANALYSIS
            /* If memory analysis is enabled metrics regarding the heap and stack usage of the demo will print. */
            /* This format is used for easier parsing and creates an avenue for future metrics to be added. */
            xAfterDemoHeapSize = democonfigMEMORY_ANALYSIS_MIN_EVER_HEAP_SIZE();
            IotLogInfo( "memory_metrics::freertos_heap::before::bytes::%u", xBeforeDemoHeapSize );
            IotLogInfo( "memory_metrics::freertos_heap::after::bytes::%u", xAfterDemoHeapSize );

            xAfterDemoTaskWaterMark = democonfigMEMORY_ANALYSIS_STACK_WATERMARK( NULL );
            IotLogInfo( "memory_metrics::demo_task_stack::before::bytes::%u", xBeforeDemoTaskWaterMark );
            IotLogInfo( "memory_metrics::demo_task_stack::after::bytes::%u", xAfterDemoTaskWaterMark );
        #endif /* democonfigMEMORY_ANALYSIS */

        /* Log the demo status. */
        if( status == EXIT_SUCCESS )
        {
            /* DO NOT EDIT - This message is used in the test framework to
             * determine whether or not the demo was successful. */
            IotLogInfo( "Demo completed successfully." );
        }
        else
        {
            IotLogError( "Error running demo." );
        }

        cleanup();
    }
    else
    {
        IotLogError( "Failed to initialize the demo. exiting..." );
    }

    /* DO NOT EDIT - This demo end marker is used in the test framework to
     * determine the end of a demo. */
    IotLogInfo( "-------DEMO FINISHED-------\n" );
}

/*-----------------------------------------------------------*/

#if ( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 )

    BaseType_t xApplicationDNSQueryHook( const char * pcName )
    {
        BaseType_t xReturn;

        /* Determine if a name lookup is for this node.  Two names are given
         * to this node: that returned by pcApplicationHostnameHook() and that set
         * by mainDEVICE_NICK_NAME. */
        if( strcmp( pcName, pcApplicationHostnameHook() ) == 0 )
        {
            xReturn = pdPASS;
        }
        else if( strcmp( pcName, mainDEVICE_NICK_NAME ) == 0 )
        {
            xReturn = pdPASS;
        }
        else
        {
            xReturn = pdFAIL;
        }

        return xReturn;
    }

#endif /* if ( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 ) */

/*-----------------------------------------------------------*/

/**
 * @brief Warn user if pvPortMalloc fails.
 *
 * Called if a call to pvPortMalloc() fails because there is insufficient
 * free memory available in the FreeRTOS heap.  pvPortMalloc() is called
 * internally by FreeRTOS API functions that create tasks, queues, software
 * timers, and semaphores.  The size of the FreeRTOS heap is set by the
 * configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h.
 *
 */
void vApplicationMallocFailedHook()
{
    configPRINTF( ( "ERROR: Malloc failed to allocate memory\r\n" ) );
    printf( ( "ERROR: Malloc failed to allocate memory\r\n" ) );
    taskDISABLE_INTERRUPTS();

    /* Loop forever */
    for( ; ; )
    {
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Loop forever if stack overflow is detected.
 *
 * If configCHECK_FOR_STACK_OVERFLOW is set to 1,
 * this hook provides a location for applications to
 * define a response to a stack overflow.
 *
 * Use this hook to help identify that a stack overflow
 * has occurred.
 *
 */
void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    char * pcTaskName )
{
    configPRINTF( ( "ERROR: stack overflow with task %s\r\n", pcTaskName ) );
    portDISABLE_INTERRUPTS();

    /* Unused Parameters */
    ( void ) xTask;

    /* Loop forever */
    for( ; ; )
    {
    }
}

/*-----------------------------------------------------------*/
