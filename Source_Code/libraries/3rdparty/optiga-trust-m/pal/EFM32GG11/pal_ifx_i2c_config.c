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

#include "optiga/pal/pal_gpio.h"
#include "optiga/pal/pal_i2c.h"
#include "optiga/ifx_i2c/ifx_i2c_config.h"
#include "em_gpio.h"


#define TRST_RST_PIN			12
#define TRST_RST_PORT			gpioPortA

#define TRST_VDD_PIN			8
#define TRST_VDD_PORT			gpioPortI

/*
#define TRST_RST_PIN			9
#define TRST_RST_PORT			gpioPortI

#define TRST_VDD_PIN			8
#define TRST_VDD_PORT			gpioPortI
*/


// !!!OPTIGA_LIB_PORTING_REQUIRED
typedef struct locl_i2c_struct_to_descroibe_master
{
	// you parameters to control the master instance
	// See other implementation to get intuition on how to implement this part
}local_i2c_struct_to_descroibe_master_t;

local_i2c_struct_to_descroibe_master_t i2c_master_0;

/**
 * \brief PAL I2C configuration for OPTIGA. 
 */
pal_i2c_t optiga_pal_i2c_context_0 =
{
    /// Pointer to I2C master platform specific context
    (void*)&i2c_master_0,
    /// Slave address
    0x30,
    /// Upper layer context
    NULL,
    /// Callback event handler
    NULL
};

typedef struct efm_gpio_ctx
{
	const int			p_gpio_pin;
	GPIO_Port_TypeDef	p_port;
	uint8_t				init_flag;
}efm_gpio_ctx_t;

const efm_gpio_ctx_t pin_rst =
{
		.p_gpio_pin = TRST_RST_PIN,
		.p_port 	= TRST_RST_PORT,
		.init_flag 	= 0
};

const efm_gpio_ctx_t pin_vdd =
{
		.p_gpio_pin = TRST_VDD_PIN,
		.p_port 	= TRST_VDD_PORT,
		.init_flag 	= 0
};

/**
* \brief PAL vdd pin configuration for OPTIGA. 
 */
pal_gpio_t optiga_vdd_0 =
{
	(void*)NULL
};

/**
 * \brief PAL reset pin configuration for OPTIGA.
 */
pal_gpio_t optiga_reset_0 =
{
	(void*)&pin_rst
};

/**
* @}
*/
