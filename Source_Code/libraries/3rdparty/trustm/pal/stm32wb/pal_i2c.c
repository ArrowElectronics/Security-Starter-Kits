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

/**********************************************************************************************************************
 * HEADER FILES
 *********************************************************************************************************************/
#include "optiga/pal/pal_i2c.h"
#include "stm32wbxx_hal.h"
#include "stm32wbxx_hal_i2c.h"
#include "stm32wbxx_hal_i2c_ex.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/**********************************************************************************************************************
 * MACROS
 *********************************************************************************************************************/
#define PAL_I2C_MASTER_MAX_BITRATE  (400U)
/// @cond hidden
/*********************************************************************************************************************
 * LOCAL DATA
 *********************************************************************************************************************/
/* Varibale to indicate the re-entrant count of the i2c bus acquire function*/
//static volatile uint32_t g_entry_count = 0;

_STATIC_H volatile uint32_t g_entry_count = 0;
_STATIC_H pal_i2c_t * gp_pal_i2c_current_ctx;

/**< OPTIGA™ Trust X I2C module queue. */
QueueHandle_t trustx_i2cresult_queue;

typedef struct stm_i2c_params
{
	I2C_TypeDef* i2c_inst;
	uint32_t     speed;
	uint32_t     timing;
}stm_i2c_params_t;

typedef struct stm_i2c_ctx
{
	I2C_HandleTypeDef 	i2c;
	stm_i2c_params_t    i2c_params;
}stm_i2c_ctx_t;

typedef struct i2c_result {
	/// Pointer to store upper layer callback context (For example: Ifx i2c context)
	pal_i2c_t * i2c_ctx;
	/// I2C Transmission result (e.g. PAL_I2C_EVENT_SUCCESS)
	uint16_t i2c_result;
}i2c_result_t;

//lint --e{715} suppress "This is implemented for overall completion of API"
_STATIC_H pal_status_t pal_i2c_acquire(const void * p_i2c_context)
{
    if (0 == g_entry_count)
    {
        g_entry_count++;
        if (1 == g_entry_count)
        {
            return PAL_STATUS_SUCCESS;
        }
    }
    return PAL_STATUS_FAILURE;
}

//lint --e{715} suppress "The unused p_i2c_context variable is kept for future enhancements"
_STATIC_H void pal_i2c_release(const void * p_i2c_context)
{
    g_entry_count = 0;
}
/// @endcond

void invoke_upper_layer_callback (const pal_i2c_t * p_pal_i2c_ctx, optiga_lib_status_t event)
{
    upper_layer_callback_t upper_layer_handler;
    //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
    upper_layer_handler = (upper_layer_callback_t)p_pal_i2c_ctx->upper_layer_event_handler;

    upper_layer_handler(p_pal_i2c_ctx->p_upper_layer_ctx, event);

    //Release I2C Bus
    pal_i2c_release(p_pal_i2c_ctx->p_upper_layer_ctx);
}

/// @cond hidden

// !!!OPTIGA_LIB_PORTING_REQUIRED
// The next 5 functions are required only in case you have interrupt based i2c implementation
void i2c_master_end_of_transmit_callback(void)
{
    invoke_upper_layer_callback(gp_pal_i2c_current_ctx, PAL_I2C_EVENT_SUCCESS);
}

void i2c_master_end_of_receive_callback(void)
{
    invoke_upper_layer_callback(gp_pal_i2c_current_ctx, PAL_I2C_EVENT_SUCCESS);
}

void i2c_master_error_detected_callback(void)
{
    invoke_upper_layer_callback(gp_pal_i2c_current_ctx, PAL_I2C_EVENT_ERROR);
}

void i2c_master_nack_received_callback(void)
{
    i2c_master_error_detected_callback();
}

void i2c_master_arbitration_lost_callback(void)
{
    i2c_master_error_detected_callback();
}


pal_status_t pal_i2c_init(const pal_i2c_t* p_i2c_context)
{
	uint16_t status = PAL_STATUS_FAILURE;
	stm_i2c_ctx_t * p_i2c_ctx = NULL;
	I2C_HandleTypeDef* p_i2c = NULL;
	GPIO_InitTypeDef  gpio_init_structure;

	do {

		if (p_i2c_context == NULL)
			break;

		p_i2c_ctx =  (stm_i2c_ctx_t *)p_i2c_context->p_i2c_hw_config;
		p_i2c = &p_i2c_ctx->i2c;

		/* I2C configuration */
		p_i2c->Instance              = p_i2c_ctx->i2c_params.i2c_inst;
		p_i2c->Init.Timing           = p_i2c_ctx->i2c_params.timing;
		p_i2c->Init.OwnAddress1      = 0;
		p_i2c->Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
		p_i2c->Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
		p_i2c->Init.OwnAddress2      = 0;
		p_i2c->Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
		p_i2c->Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

		/*** Configure the GPIOs ***/
		/* Enable GPIO clock */
		__HAL_RCC_GPIOB_CLK_ENABLE();

		__HAL_RCC_GPIOA_CLK_ENABLE();

		/* Configure I2C Tx, Rx as alternate function */
		gpio_init_structure.Pin = GPIO_PIN_8 | GPIO_PIN_9;
		gpio_init_structure.Mode = GPIO_MODE_AF_OD;
		gpio_init_structure.Pull = GPIO_NOPULL;
		gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		gpio_init_structure.Alternate = GPIO_AF4_I2C1;
		HAL_GPIO_Init(GPIOB, &gpio_init_structure);

		/*** Configure the I2C peripheral ***/
		/* Enable I2C clock */
		__HAL_RCC_I2C1_CLK_ENABLE();

		/* Force the I2C peripheral clock reset */
		__HAL_RCC_I2C1_FORCE_RESET();

		/* Release the I2C peripheral clock reset */
		__HAL_RCC_I2C1_RELEASE_RESET();

		/* Enable and set I2Cx Interrupt to a lower priority */
		HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0x0F, 0);
		HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);

		/* Enable and set I2Cx Interrupt to a lower priority */
		HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0x0F, 0);
		HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);

		if(HAL_I2C_Init(p_i2c) != HAL_OK)
		{
			break;
		}

		HAL_I2CEx_ConfigAnalogFilter(p_i2c, I2C_ANALOGFILTER_ENABLE);

		status = PAL_STATUS_SUCCESS;
	}while(0);

    return status;
}

pal_status_t pal_i2c_deinit(const pal_i2c_t* p_i2c_context)
{
    return PAL_STATUS_SUCCESS;
}

pal_status_t pal_i2c_write(pal_i2c_t* p_i2c_context,uint8_t* p_data , uint16_t length)
{
    pal_status_t status = PAL_STATUS_FAILURE;
    I2C_HandleTypeDef * p_i2c = NULL;

    //Acquire the I2C bus before read/write
    if((PAL_STATUS_SUCCESS == pal_i2c_acquire(p_i2c_context)) && (p_i2c_context != NULL))
    {
        gp_pal_i2c_current_ctx = p_i2c_context;
        p_i2c = &((stm_i2c_ctx_t *)(p_i2c_context->p_i2c_hw_config))->i2c;

       		//Invoke the low level i2c master driver API to write to the bus
       		while((status = HAL_I2C_Master_Transmit(p_i2c, (p_i2c_context->slave_address << 1), p_data, length, 50))!= HAL_OK)
        	{
            	//If I2C Master fails to invoke the write operation, invoke upper layer event handler with error.
        		if (HAL_I2C_GetError(p_i2c) != HAL_I2C_ERROR_AF)
        		{
            		//lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
        			((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                       (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_ERROR);
        			break;
        		}
        	}
        	if(status == HAL_OK)
        	{
            	//lint --e{611} suppress "void* function pointer is type casted to app_event_handler_t type"
            	((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                       (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_SUCCESS);
            	status = PAL_STATUS_SUCCESS;
        	}
	
        //Release I2C Bus
        pal_i2c_release((void *)p_i2c_context);
    }
    else
    {
        status = PAL_STATUS_I2C_BUSY;
        //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
        ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                        (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_BUSY);
    }
    return status;
}

pal_status_t pal_i2c_read(pal_i2c_t * p_i2c_context, uint8_t * p_data, uint16_t length)
{
    pal_status_t status = PAL_STATUS_FAILURE;
    I2C_HandleTypeDef * p_i2c = NULL;

    //Acquire the I2C bus before read/write
    if ((PAL_STATUS_SUCCESS == pal_i2c_acquire(p_i2c_context)) && (p_i2c_context != NULL))
    {
        gp_pal_i2c_current_ctx = p_i2c_context;
        p_i2c = &((stm_i2c_ctx_t *)(p_i2c_context->p_i2c_hw_config))->i2c;

        //Invoke the low level i2c master driver API to read from the bus
        while((status = HAL_I2C_Master_Receive(p_i2c, (p_i2c_context->slave_address << 1), p_data, length, 50)) != HAL_OK)
        {
            //If I2C Master fails to invoke the read operation, invoke upper layer event handler with error.
        	if (HAL_I2C_GetError(p_i2c) != HAL_I2C_ERROR_AF)
        	{
            //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
        		((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                       (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_ERROR);
            	break;
        	}

        }
        if(status == HAL_OK)
        {
            //lint --e{611} suppress "void* function pointer is type casted to app_event_handler_t type"
            ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                       (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_SUCCESS);
            status = PAL_STATUS_SUCCESS;
        }

        //Release I2C Bus
        pal_i2c_release((void *)p_i2c_context);
    }
    else
    {
        status = PAL_STATUS_I2C_BUSY;
        //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
        ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                        (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_BUSY);
    }
    return status;
}

pal_status_t pal_i2c_set_bitrate(const pal_i2c_t * p_i2c_context, uint16_t bitrate)
{
	pal_status_t return_status = PAL_STATUS_SUCCESS;

    return return_status;
}

/**
* @}
*/
