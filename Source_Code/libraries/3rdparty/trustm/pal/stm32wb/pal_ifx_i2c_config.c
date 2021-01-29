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
* \file pal_ifx_i2c_config.c
*
* \brief   This file implements platform abstraction layer configurations for ifx i2c protocol.
*
* \ingroup  grPAL
*
* @{
*/

/**********************************************************************************************************************
 * HEADER FILES
 *********************************************************************************************************************/
#include "optiga/pal/pal_gpio.h"
#include "optiga/pal/pal_i2c.h"
#include "optiga/ifx_i2c/ifx_i2c_config.h"
#include "stm32wbxx_hal.h"
#include "stm32wbxx_hal_i2c.h"
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

stm_i2c_ctx_t stm_i2c_ctx_0 = {
	.i2c_params = {
		.i2c_inst = I2C1,
		.speed = 100000,
		.timing = 0x00702681,
	}
};


// We use software reset to reset OPTIGA
GPIO_InitTypeDef  reset_pin_0 = {
		.Pin   = GPIO_PIN_9,
		.Mode  = GPIO_MODE_OUTPUT_PP,
		.Pull  = GPIO_PULLUP,
		.Speed = GPIO_SPEED_FREQ_HIGH,
};

typedef struct stm_gpio_ctx
{
	GPIO_InitTypeDef* 	p_gpio;
	GPIO_TypeDef *      p_port;
	uint8_t			    init_flag;
}stm_gpio_ctx_t;


stm_gpio_ctx_t stm_reset_ctx_0 = {
		.p_gpio = &reset_pin_0,
		.p_port = GPIOA,
		.init_flag = 0
};
/*********************************************************************************************************************
 * pal ifx i2c instance
 *********************************************************************************************************************/
/**
 * \brief PAL I2C configuration for OPTIGA. 
 */
pal_i2c_t optiga_pal_i2c_context_0 =
{
    /// Pointer to I2C master platform specific context
    (void*)&stm_i2c_ctx_0,
    /// Slave address
    0x30,
    /// Upper layer context
    NULL,
    /// Callback event handler
    NULL
};

/*********************************************************************************************************************
 * PAL GPIO configurations defined for XMC4500
 *********************************************************************************************************************/
/**
* \brief PAL vdd pin configuration for OPTIGA. 
 */
pal_gpio_t optiga_vdd_0 =
{
    // Platform specific GPIO context for the pin used to toggle Vdd.
    (void*)NULL
};

/**
 * \brief PAL reset pin configuration for OPTIGA.
 */
pal_gpio_t optiga_reset_0 =
{
    // Platform specific GPIO context for the pin used to toggle Reset.
    (void*)&stm_reset_ctx_0,
};


/**
* @}
*/

