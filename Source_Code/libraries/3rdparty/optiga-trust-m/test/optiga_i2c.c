#include "optiga/pal/pal_i2c.h"
#include "em_gpio.h"

#include "FreeRTOS.h"
#include "task.h"


static volatile uint32_t optiga_pal_event_status;
static void optiga_pal_i2c_event_handler(void* upper_layer_ctx, uint8_t event);

pal_i2c_t optiga_test_i2c_context_0 =
{
    /// Pointer to I2C master platform specific context
    NULL,
    /// Slave address
    0x30,
    /// Upper layer context
    NULL,
    /// Callback event handler
    NULL,
};


static void optiga_rst_init()
{
	 // Reset pin
	  GPIO_PinModeSet(gpioPortI, 9, gpioModePushPull, 0);
}

static void optiga_rst_high()
{
	 //reset pin high for 15 ms
	  GPIO_PinOutSet(gpioPortI, 9);
	  vTaskDelay(15);
}

pal_status_t optiga_i2c_init()
{
	pal_status_t pal_return_status = PAL_STATUS_FAILURE;
	/* peform RST */
	optiga_rst_init();

	pal_return_status = pal_i2c_init(&optiga_test_i2c_context_0);
	if (PAL_STATUS_FAILURE == pal_return_status)
	{
		/* I2C init failed !! */
		return pal_return_status;
	}
	/*rst high for 15ms */
	optiga_rst_high();

	return PAL_STATUS_SUCCESS;
}

// OPTIGA pal i2c event handler
static void optiga_pal_i2c_event_handler(void* upper_layer_ctx, uint8_t event)
{
    optiga_pal_event_status = event;
}

/* Function to verify I2C communication with OPTIGA */
pal_status_t test_optiga_communication(uint8_t is_i2c_init)
{
		pal_status_t pal_return_status = PAL_STATUS_FAILURE;
		uint8_t data_buffer[10] = {0x82};
		uint8_t write_fail = 0;
		uint8_t read_fail = 0;


		// set callback handler for pal i2c
		optiga_test_i2c_context_0.upper_layer_event_handler =
				optiga_pal_i2c_event_handler;

		if(!is_i2c_init)
		{
			pal_return_status = optiga_i2c_init();
			if (PAL_STATUS_FAILURE == pal_return_status)
			{
				return pal_return_status;
			}
		}

		// Send 0x82 to read I2C_STATE from optiga
		do
		{
				optiga_pal_event_status = PAL_I2C_EVENT_BUSY;
				optiga_pal_event_status = pal_i2c_write(&optiga_test_i2c_context_0, data_buffer, 1);
				if (PAL_STATUS_FAILURE == optiga_pal_event_status)
				{
						// Pal I2C write failed due to I2C busy is in busy
						// state or low level driver failures
					printf("TRUST_M_I2C_TEST Write failed !!\n");
					write_fail = 1 ;
					break;
				}

				// Wait until writing to optiga is completed
		} while (PAL_I2C_EVENT_SUCCESS != optiga_pal_event_status);

		/*Delay between read after write*/
		vTaskDelay(1);

		/* reset read buffer */
		memset(data_buffer,0,sizeof(data_buffer));

		// Read the I2C_STATE from OPTIGA
		do
		{
				optiga_pal_event_status = PAL_I2C_EVENT_BUSY;
				optiga_pal_event_status = pal_i2c_read(&optiga_test_i2c_context_0, data_buffer, 4);
				// Pal I2C read failed due to I2C busy is in busy
				// state or low level driver failures
				if (PAL_STATUS_FAILURE == optiga_pal_event_status)
				{
					printf("TRUST_M_I2C_TEST Read failed !!\n");
					read_fail = 1 ;
					break;
				}
				// Wait until reading from optiga is completed
		} while (PAL_I2C_EVENT_SUCCESS != optiga_pal_event_status);

		/* check the read buff */
		int i = 0;
		for(;i<=sizeof(data_buffer);i++)
			printf("data_buff[%d]=0x%x\n",i,data_buffer[i]);

		/* test is done manager the resources here */
		if(is_i2c_init)
		{
			pal_i2c_deinit(&optiga_test_i2c_context_0);

		}

		if(write_fail || read_fail )
				return PAL_I2C_EVENT_ERROR;



		return pal_return_status;
}

