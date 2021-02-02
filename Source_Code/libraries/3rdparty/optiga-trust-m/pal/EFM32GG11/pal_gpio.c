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
* \file pal_gpio.c
*
* \brief   This file implements the platform abstraction layer APIs for GPIO.
*
* \ingroup  grPAL
*
* @{
*/

#include "optiga/pal/pal_gpio.h"
#include "FreeRTOS.h"
#include "task.h"

#include "em_cmu.h"

/**********************************************************************************************************************
 * LOCAL DATA
 *********************************************************************************************************************/
// cmuClock_HFPER
#define PERI_CLK			cmuClock_HFPER

#define TRST_GPIO_DIR		0
#define TRST_GPIO_MODE		gpioModePushPull

typedef struct efm_gpio_ctx
{
	const int			p_gpio_pin;
	GPIO_Port_TypeDef	p_port;
	uint8_t				init_flag;
}efm_gpio_ctx_t;



//lint --e{714,715} suppress "This is implemented for overall completion of API"
pal_status_t pal_gpio_init(const pal_gpio_t * p_gpio_context)
{
#ifdef TRUST_M_DBG
	printf("TrustM-PalDbg: pal_gpio_init\n");
#endif
    return PAL_STATUS_SUCCESS;
}

//lint --e{714,715} suppress "This is implemented for overall completion of API"
pal_status_t pal_gpio_deinit(const pal_gpio_t * p_gpio_context)
{
#ifdef TRUST_M_DBG
	printf("TrustM-PalDbg: pal_gpio_deinit\n");
#endif
    return PAL_STATUS_SUCCESS;
}

void pal_gpio_set_high(const pal_gpio_t * p_gpio_context)
{
	efm_gpio_ctx_t* p_io = NULL;
    if ((p_gpio_context != NULL) && (p_gpio_context->p_gpio_hw != NULL))
    {
    	p_io = (efm_gpio_ctx_t*)(p_gpio_context->p_gpio_hw);
    	if (p_io->init_flag == 0)
    	{
    		/* Initially pin state will be 0 */
    		GPIO_PinModeSet(p_io->p_port, p_io->p_gpio_pin,
    							TRST_GPIO_MODE, TRST_GPIO_DIR);
    		//init code
    		p_io->init_flag = 1;
    	}
    	GPIO_PinOutSet(p_io->p_port, p_io->p_gpio_pin);

	#ifdef TRUST_M_DBG
    	printf("TrustM-PalDbg: pal_gpio_init high !!\n");
	#endif
    }
}

void pal_gpio_set_low(const pal_gpio_t * p_gpio_context)
{
	efm_gpio_ctx_t* p_io = NULL;
    if ((p_gpio_context != NULL) && (p_gpio_context->p_gpio_hw != NULL))
    {
    	p_io = (efm_gpio_ctx_t*)(p_gpio_context->p_gpio_hw);
    	if (p_io->init_flag == 0)
    	{
    		/* Initially pin state will be 0 */
    		GPIO_PinModeSet(p_io->p_port, p_io->p_gpio_pin,
    							TRST_GPIO_MODE, TRST_GPIO_DIR);
    		// init code
    		p_io->init_flag = 1;
    	}
    	GPIO_PinOutClear(p_io->p_port, p_io->p_gpio_pin);

		#ifdef TRUST_M_DBG
    		printf("TrustM-PalDbg: pal_gpio_low !!\n");
		#endif
    }
}

/**
* @}
*/
