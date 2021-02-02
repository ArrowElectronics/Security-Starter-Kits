/**
* \copyright
* MIT License
*
* Copyright (c) 2019 Infineon Technologies AG
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE
*
* \endcopyright
*
* \author Infineon Technologies AG
*
* \file pal_i2c.c
*
* \brief   This file implements the platform abstraction layer(pal) APIs for I2C.
*
* \ingroup  grPAL
*
* @{
*/

#include "optiga/pal/pal_i2c.h"
/* OS layer */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
/* hardware layer */
#include "em_gpio.h"
#include "em_i2c.h"
#include "i2cspm.h"

//#define PAL_I2C_MASTER_MAX_BITRATE  			(400U)
/* As per EFM32 silicon limitation max frequcny with 6:3 392157Hz */
/* But as per optiga data sheet upto 400 KHz support is available */
#define PAL_I2C_MASTER_MAX_BITRATE 				I2C_FREQ_FAST_MAX
//#define PAL_I2C_MASTER_MAX_BITRATE				400000U

/* I2C Pin configuration */
#define TRST_I2C_INSTANCE						I2C0
#define TRST_I2C_PORT							gpioPortC
#define TRST_I2C_SDA_PIN						0
#define TRST_I2C_SCL_PIN						1
#define TRST_I2C_PIN_LOC						4

/// @cond hidden
//_STATIC_H volatile uint32_t g_entry_count = 0;
_STATIC_H pal_i2c_t * gp_pal_i2c_current_ctx;

/**< OPTIGA™ Trust m I2C module queue. */
QueueHandle_t trustx_i2cresult_queue;
TaskHandle_t xIicCallbackTaskHandle = NULL;
SemaphoreHandle_t xIicSemaphoreHandle;

typedef struct i2c_result {
	/// Pointer to store upper layer callback context (For example: Ifx i2c context)
	pal_i2c_t * i2c_ctx;
	/// I2C Transmission result (e.g. PAL_I2C_EVENT_SUCCESS)
	uint16_t i2c_result;
}i2c_result_t;

/* Local i2c context */
#define I2CSPM_INIT_TRUST_M                                                   	   \
{	TRST_I2C_INSTANCE,				/* Use I2C instance 2 */                       \
	TRST_I2C_PORT,                 	/* SCL port */                                 \
	TRST_I2C_SCL_PIN,               /* SCL pin */                                  \
	TRST_I2C_PORT,                  /* SDA port */                                 \
	TRST_I2C_SDA_PIN,               /* SDA pin */                                  \
	TRST_I2C_PIN_LOC,               /* Location of SCL */                          \
	TRST_I2C_PIN_LOC,               /* Location of SDA */                          \
	0,                              /* Use currently configured reference clock */ \
	PAL_I2C_MASTER_MAX_BITRATE,     /* Set to standard rate  */                    \
	i2cClockHLRAsymetric,           /* Set to use 6:3 low/high duty cycle */       \
}



/* Hardware layer - i2c in master mode */
I2CSPM_Init_TypeDef i2cInit = I2CSPM_INIT_TRUST_M;

//lint --e{715} suppress "This is implemented for overall completion of API"
_STATIC_H pal_status_t pal_i2c_acquire(const void * p_i2c_context)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if ( xSemaphoreTakeFromISR(xIicSemaphoreHandle, &xHigherPriorityTaskWoken) == pdTRUE )
		return PAL_STATUS_SUCCESS;
	else
		return PAL_STATUS_FAILURE;
}

//lint --e{715} suppress "The unused p_i2c_context variable is kept for future enhancements"
_STATIC_H void pal_i2c_release(const void * p_i2c_context)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	xSemaphoreGiveFromISR(xIicSemaphoreHandle, &xHigherPriorityTaskWoken);
}
/// @endcond

void invoke_upper_layer_callback (const pal_i2c_t * p_pal_i2c_ctx, optiga_lib_status_t event)
{
#ifdef TRUST_M_DBG
	printf("TrustM-PalDbg: invoke_upper_layer_callback ENTER!!\n");
#endif
    upper_layer_callback_t upper_layer_handler;
    //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
    upper_layer_handler = (upper_layer_callback_t)p_pal_i2c_ctx->upper_layer_event_handler;

    upper_layer_handler(p_pal_i2c_ctx->p_upper_layer_ctx, event);

    //Release I2C Bus
    pal_i2c_release(p_pal_i2c_ctx->p_upper_layer_ctx);
#ifdef TRUST_M_DBG
	printf("TrustM-PalDbg: invoke_upper_layer_callback EXIT!!\n");
#endif
}

/// @cond hidden

// !!!OPTIGA_LIB_PORTING_REQUIRED
// The next 5 functions are required only in case you have interrupt based i2c implementation
void i2c_master_end_of_transmit_callback(void)
{
	i2c_result_t i2_result;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	i2_result.i2c_ctx = gp_pal_i2c_current_ctx;
	i2_result.i2c_result = PAL_I2C_EVENT_SUCCESS;

#ifdef TRUST_M_DBG
	printf("TrustM-PalDbg: i2c_master_end_of_transmit_callback!!\n");
#endif
    /*
     * You cann't call callback from the timer callback, this might lead to a corruption
     * Use queues instead to activate corresponding handler
     * */
	xQueueSendFromISR( trustx_i2cresult_queue, ( void * ) &i2_result, &xHigherPriorityTaskWoken );
    //invoke_upper_layer_callback(gp_pal_i2c_current_ctx, PAL_I2C_EVENT_SUCCESS);
}

void i2c_master_end_of_receive_callback(void)
{
	i2c_result_t i2_result;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	i2_result.i2c_ctx = gp_pal_i2c_current_ctx;
	i2_result.i2c_result = PAL_I2C_EVENT_SUCCESS;

#ifdef TRUST_M_DBG
	printf("TrustM-PalDbg: i2c_master_end_of_receive_callback!!\n");
#endif
    /*
     * You cann't call callback from the timer callback, this might lead to a corruption
     * Use queues instead to activate corresponding handler
     * */
	xQueueSendFromISR( trustx_i2cresult_queue, ( void * ) &i2_result, &xHigherPriorityTaskWoken );
    //invoke_upper_layer_callback(gp_pal_i2c_current_ctx, PAL_I2C_EVENT_SUCCESS);
}

void i2c_master_error_detected_callback(void)
{
	i2c_result_t i2_result;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	i2_result.i2c_ctx = gp_pal_i2c_current_ctx;
	i2_result.i2c_result = PAL_I2C_EVENT_ERROR;

#ifdef TRUST_M_DBG
	printf("TrustM-PalDbg: i2c_master_error_detected_callback!!\n");
#endif

	xQueueSendFromISR( trustx_i2cresult_queue, ( void * ) &i2_result, &xHigherPriorityTaskWoken );
}

void i2c_master_nack_received_callback(void)
{
    i2c_master_error_detected_callback();
}

void i2c_master_arbitration_lost_callback(void)
{
    i2c_master_error_detected_callback();
}
/// @endcond

void i2c_result_handler( void * pvParameters )
{
	i2c_result_t i2_result;

	do {
		if( xQueueReceive( trustx_i2cresult_queue, &( i2_result ), ( TickType_t ) portMAX_DELAY ) )
		{

		#ifdef TRUST_M_DBG
			printf("TrustM-PalDbg: i2c_result_handler Queue received !!\n");
		#endif

			invoke_upper_layer_callback(i2_result.i2c_ctx, i2_result.i2c_result);
		}
	} while(1);

	vTaskDelete( NULL );
}

pal_status_t pal_i2c_init(const pal_i2c_t * p_i2c_context)
{
	uint16_t status = PAL_STATUS_SUCCESS;
	if (xIicCallbackTaskHandle == NULL)
	{
			xIicSemaphoreHandle = xSemaphoreCreateBinary();

			I2CSPM_Init(&i2cInit);

			/* Create the handler for the callbacks. */
			xTaskCreate( i2c_result_handler,       /* Function that implements the task. */
						"TrstI2CXHndlr",          /* Text name for the task. */
						configMINIMAL_STACK_SIZE,      /* Stack size in words, not bytes. */
						NULL,    /* Parameter passed into the task. */
						tskIDLE_PRIORITY,/* Priority at which the task is created. */
						&xIicCallbackTaskHandle );      /* Used to pass out the created task's handle. */

			/* Create a queue for results. Not more than 2 interrupts one by one are expected*/
			trustx_i2cresult_queue = xQueueCreate( 2, sizeof( i2c_result_t ) );

			#ifdef TRUST_M_DBG
				printf("TrustM-PalDbg: I2C init done !!\n");
			#endif

			pal_i2c_release(p_i2c_context);
	}
	else
	{
		// Seems like the task has been started already
		status = PAL_STATUS_SUCCESS;
	}

    return status;
}

pal_status_t pal_i2c_deinit(const pal_i2c_t * p_i2c_context)
{
    if ( xIicCallbackTaskHandle != NULL)
        vTaskDelete(xIicCallbackTaskHandle);

    /*FIXME : i2c_deinit*/

    pal_i2c_acquire(p_i2c_context);

    vSemaphoreDelete(xIicSemaphoreHandle);

	#ifdef TRUST_M_DBG
		printf("TrustM-PalDbg: I2C De-init done !!\n");
	#endif

    return PAL_STATUS_SUCCESS;

}

pal_status_t pal_i2c_write(pal_i2c_t * p_i2c_context, uint8_t * p_data, uint16_t length)
{
    pal_status_t status = PAL_STATUS_FAILURE;

    // Transfer structure
    I2C_TransferSeq_TypeDef i2cTransfer;
    I2C_TransferReturn_TypeDef ret;

    //Acquire the I2C bus before read/write
    if (PAL_STATUS_SUCCESS == pal_i2c_acquire(p_i2c_context))
    {
        gp_pal_i2c_current_ctx = p_i2c_context;

        // Initializing I2C transfer
        i2cTransfer.addr          = p_i2c_context->slave_address << 1;
        i2cTransfer.flags         = I2C_FLAG_WRITE;
        i2cTransfer.buf[0].data   = p_data;
        i2cTransfer.buf[0].len    = length;

        ret = I2CSPM_Transfer(i2cInit.port,&i2cTransfer);
        if (ret != i2cTransferDone)
        {

		#ifdef TRUST_M_DBG
        	printf("TrustM-PalDbg: I2C Write failed !!\n");
		#endif
            //If I2C Master fails to invoke the write operation, invoke upper layer event handler with error.
            ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                       (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_ERROR);

        }
        else
        {

    		((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
    	        												(p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_SUCCESS);
        	  status = PAL_STATUS_SUCCESS;
        }

        //Release I2C Bus
        pal_i2c_release((void *)p_i2c_context);
    }
    else
    {
	#ifdef TRUST_M_DBG
    	printf("TrustM-PalDbg: I2C Write bus acquisition failed !!\n");
	#endif
    	status = PAL_STATUS_I2C_BUSY;
    	((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
    	                                        		(p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_BUSY);

    }
    return status;
}

pal_status_t pal_i2c_read(pal_i2c_t * p_i2c_context, uint8_t * p_data, uint16_t length)
{
    pal_status_t status = PAL_STATUS_FAILURE;
    I2C_TransferSeq_TypeDef    i2cTransfer;
    I2C_TransferReturn_TypeDef ret;
    /* Acuirq the I2C bus */
    if (PAL_STATUS_SUCCESS == pal_i2c_acquire(p_i2c_context))
    {
    	gp_pal_i2c_current_ctx = p_i2c_context;

    	i2cTransfer.addr  = p_i2c_context->slave_address << 1;
    	i2cTransfer.flags = I2C_FLAG_READ;
    	/* Select location/length of data to be read */
    	i2cTransfer.buf[0].data = p_data;
    	i2cTransfer.buf[0].len  = length;

    	ret = I2CSPM_Transfer(i2cInit.port,&i2cTransfer);
    	if (ret != i2cTransferDone)
    	{
		#ifdef TRUST_M_DBG
    		printf("TrustM-PalDbg: I2C Read failed !!\n");
		#endif
    	    ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
    	                                                       (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_ERROR);

    	}
    	else
    	{
    		((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
    	        												(p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_SUCCESS);
    		status = PAL_STATUS_SUCCESS;
    	}

    	/* Release I2C bus after read operation */
    	pal_i2c_release((void * )p_i2c_context);
    }
    else
    {
	#ifdef TRUST_M_DBG
    	printf("TrustM-PalDbg: I2C Read bus acquisition failed !!\n");
	#endif
    	status = PAL_STATUS_I2C_BUSY;
        ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                        (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_BUSY);
    }

    return status;
}

pal_status_t pal_i2c_set_bitrate(const pal_i2c_t * p_i2c_context, uint16_t bitrate)
{
	return PAL_STATUS_SUCCESS;
}

/**
* @}
*/
