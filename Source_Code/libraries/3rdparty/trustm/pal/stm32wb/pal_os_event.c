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
* \file pal_os_event.c
*
* \brief   This file implements the platform abstraction layer APIs for os event/scheduler.
*
* \ingroup  grPAL
*
* @{
*/
#if 0
#include "optiga/pal/pal_os_event.h"
#include "optiga/common/optiga_lib_types.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "queue.h"
#include "optiga/pal/pal.h"
#include "stdio.h"
#include "optiga_example.h"
osTimerId otxTimer;
/// @cond hidden

TimerHandle_t otxTimers;

static pal_os_event_t pal_os_event_0 = {0};

void pal_os_event_start(pal_os_event_t * p_pal_os_event, register_callback callback, void * callback_args)
{
    if (FALSE == p_pal_os_event->is_event_triggered)
    {
        p_pal_os_event->is_event_triggered = TRUE;
        pal_os_event_register_callback_oneshot(p_pal_os_event,callback,callback_args,1000);
    }
}

void pal_os_event_stop(pal_os_event_t * p_pal_os_event)
{
    //lint --e{714} suppress "The API pal_os_event_stop is not exposed in header file but used as extern in
    //optiga_cmd.c"
    p_pal_os_event->is_event_triggered = FALSE;
}

void pal_os_event_registered_callback(void const * argument)
{
    register_callback callback;

	// !!!OPTIGA_LIB_PORTING_REQUIRED
    // User should take care to stop the timer if it sin't stoped automatically
    osTimerStop(otxTimer);
    xTimerReset(otxTimer,0);

    if (pal_os_event_0.callback_registered)
    {
    	 OPTIGA_EXAMPLE_LOG_MESSAGE("*******pal_os_event_registered_callback*******\n");
        callback = pal_os_event_0.callback_registered;
        callback((void * )pal_os_event_0.callback_ctx);
    }
}
pal_os_event_t * pal_os_event_create(register_callback callback, void * callback_args)
{
	osTimerDef(eventTimer, pal_os_event_registered_callback);
	otxTimer = osTimerCreate(osTimer(eventTimer), osTimerOnce, NULL);
    if (( NULL != callback )&&( NULL != callback_args ))
    {
        pal_os_event_start(&pal_os_event_0,callback,callback_args);
    }
    return (&pal_os_event_0);
}

void pal_os_event_trigger_registered_callback(void)
{
    register_callback callback;

	// !!!OPTIGA_LIB_PORTING_REQUIRED
    // User should take care to stop the timer if it sin't stoped automatically

    if (pal_os_event_0.callback_registered)
    {
        callback = pal_os_event_0.callback_registered;
        callback((void * )pal_os_event_0.callback_ctx);
    }
}

/// @endcond

void pal_os_event_register_callback_oneshot(pal_os_event_t * p_pal_os_event,
                                             register_callback callback,
                                             void * callback_args,
                                             uint32_t time_us)
{
    p_pal_os_event->callback_registered = callback;
    p_pal_os_event->callback_ctx = callback_args;

    if (time_us < 1000)
    {
    	time_us = 1000;
    }
    osTimerStart(otxTimer, pdMS_TO_TICKS(time_us / 1000));

   	// !!!OPTIGA_LIB_PORTING_REQUIRED
    // User should start the timer here with the
	// pal_os_event_trigger_registered_callback() function as a callback
}

//lint --e{818,715} suppress "As there is no implementation, pal_os_event is not used"
void pal_os_event_destroy(pal_os_event_t * pal_os_event)
{
    // User should take care to destroy the event if it's not required
}

/**
* @}
*/
#endif

#include "optiga/pal/pal_os_event.h"
#include "optiga/pal/pal.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "queue.h"

TimerHandle_t otxTimers;
/// @cond hidden

static pal_os_event_t pal_os_event_0 = {0};
void pal_os_event_trigger_registered_callback_b(TimerHandle_t xTimer);

void pal_os_event_start(pal_os_event_t * p_pal_os_event, register_callback callback, void * callback_args)
{
    if (FALSE == p_pal_os_event->is_event_triggered)
    {
        p_pal_os_event->is_event_triggered = TRUE;
        pal_os_event_register_callback_oneshot(p_pal_os_event,callback,callback_args,1000);
    }
}

void pal_os_event_stop(pal_os_event_t * p_pal_os_event)
{
    //lint --e{714} suppress "The API pal_os_event_stop is not exposed in header file but used as extern in
    //optiga_cmd.c"
    p_pal_os_event->is_event_triggered = FALSE;
}

void pal_os_event_trigger_registered_callback()
{
    register_callback callback;

    // !!!OPTIGA_LIB_PORTING_REQUIRED
    // User should take care to stop the timer if it sin't stoped automatically
    //xTimerStop(otxTimers, 0);
    //xTimerReset(otxTimers, 0);

    if (pal_os_event_0.callback_registered)
    {
        callback = pal_os_event_0.callback_registered;
        callback((void * )pal_os_event_0.callback_ctx);
    }
}

void pal_os_event_trigger_registered_callback_b(TimerHandle_t xTimer)
{
    register_callback callback;

    // !!!OPTIGA_LIB_PORTING_REQUIRED
    // User should take care to stop the timer if it sin't stoped automatically
    xTimerStop(otxTimers, 0);
    xTimerReset(otxTimers, 0);

    if (pal_os_event_0.callback_registered)
    {
        callback = pal_os_event_0.callback_registered;
        callback((void * )pal_os_event_0.callback_ctx);
    }
}
/// @endcond
pal_os_event_t * pal_os_event_create(register_callback callback, void * callback_args)
{
	otxTimers = xTimerCreate(  "evtTimer",        /* Just a text name, not used by the kernel. */
	    			1,    /* The timer period in ticks. */
	    			pdFALSE,         /* The timers will auto-reload themselves when they expire. */
	    			( void * ) 0,   /* Assign each timer a unique id equal to its array index. */
					pal_os_event_trigger_registered_callback_b  /* Each timer calls the same callback when it expires. */
	    );

    if (( NULL != callback )&&( NULL != callback_args ))
    {
        pal_os_event_start(&pal_os_event_0,callback,callback_args);
    }
    return (&pal_os_event_0);
}


void pal_os_event_register_callback_oneshot(pal_os_event_t * p_pal_os_event,
                                             register_callback callback,
                                             void * callback_args,
                                             uint32_t time_us)
{
    p_pal_os_event->callback_registered = callback;
    p_pal_os_event->callback_ctx = callback_args;

	// !!!OPTIGA_LIB_PORTING_REQUIRED
    // User should start the timer here with the
	// pal_os_event_trigger_registered_callback() function as a callback
//    otxTimer[i] = xTimerCreate(  tmr_name,        /* Just a text name, not used by the kernel. */
//							pdMS_TO_TICKS(100),    /* The timer period in ticks. */
//							pdFALSE,         /* The timers will auto-reload themselves when they expire. */
//							( void * ) i,   /* Assign each timer a unique id equal to its array index. */
//							vTimerCallback  /* Each timer calls the same callback when it expires. */
//							);
    if (time_us < 1000)
       {
       	time_us = 1000;
       }
  //  if( xTimerStart( otxTimers, time_us * 100 ) != pdPASS )
    if( xTimerStart( otxTimers, pdMS_TO_TICKS(time_us / 1000) ) != pdPASS )
	{
		/* The timer could not be set into the Active
		state. */
	}
}

//lint --e{818,715} suppress "As there is no implementation, pal_os_event is not used"
void pal_os_event_destroy(pal_os_event_t * pal_os_event)
{
    // User should take care to destroy the event if it's not required
}
