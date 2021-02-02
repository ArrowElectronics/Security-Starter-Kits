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
* \file optiga_util.c
*
* \brief   This file implements the OPTIGA utility module functionalities
*
* \ingroup  grOptigaUtil
*
* @{
*/

#include "optiga/optiga_util.h"
#include "optiga/common/optiga_lib_logger.h"
#include "optiga/common/optiga_lib_common_internal.h"
#include "optiga/pal/pal_os_memory.h"


/**
 * Callback when optiga_crypt_xxxx operation is completed asynchronously
 */
static volatile optiga_lib_status_t optiga_lib_status;
//lint --e{818} suppress "argument "context" is not used in the sample provided"
static void optiga_crypt_callback(void * context, optiga_lib_status_t return_status)
{
    optiga_lib_status = return_status;
    if (NULL != context)
    {
        // callback to upper layer here
    }
}


extern void optiga_cmd_set_shielded_connection_option(optiga_cmd_t * me, uint8_t value,
                                                      uint8_t shielded_connection_option);



_STATIC_H void optiga_util_generic_event_handler(void * me,
                                                 optiga_lib_status_t event)
{
    optiga_util_t * p_optiga_util = (optiga_util_t *)me;

    p_optiga_util->handler(p_optiga_util->caller_context, event);
    p_optiga_util->instance_state = OPTIGA_LIB_INSTANCE_FREE;
}

_STATIC_H void optiga_util_reset_protection_level(optiga_util_t * me)
{
#ifdef OPTIGA_LIB_DEBUG_NULL_CHECK
    if (NULL != me)
#endif
    {
        OPTIGA_UTIL_SET_COMMS_PROTECTION_LEVEL(me, OPTIGA_COMMS_DEFAULT_PROTECTION_LEVEL);
    }
}

_STATIC_H optiga_lib_status_t optiga_util_write_data_wrapper(optiga_util_t * me,
                                                             uint16_t optiga_oid,
                                                             uint8_t write_type,
                                                             uint16_t offset,
                                                             const uint8_t * p_buffer,
                                                             uint16_t length)
{
    optiga_lib_status_t return_value = OPTIGA_UTIL_ERROR;
    optiga_set_data_object_params_t * p_params;

    do
    {
#ifdef OPTIGA_LIB_DEBUG_NULL_CHECK
        if ((NULL == me) || (NULL == me->my_cmd) || (NULL == p_buffer))
        {
            return_value = OPTIGA_UTIL_ERROR_INVALID_INPUT;
            break;
        }
#endif

        if (OPTIGA_LIB_INSTANCE_BUSY == me->instance_state)
        {
            return_value = OPTIGA_UTIL_ERROR_INSTANCE_IN_USE;
            break;
        }

        me->instance_state = OPTIGA_LIB_INSTANCE_BUSY;
        p_params = (optiga_set_data_object_params_t *)&(me->params.optiga_set_data_object_params);
        pal_os_memset(&me->params,0x00,sizeof(optiga_util_params_t));

        if (OPTIGA_UTIL_COUNT_DATA_OBJECT == write_type)
        {
            p_params->count = p_buffer[0];
            p_params->buffer = NULL;
        }
        else
        {
            p_params->count = 0;
            p_params->buffer = p_buffer;
        }
        p_params->oid = optiga_oid;
        p_params->offset = offset;
        p_params->data_or_metadata = 0;//for Data
        p_params->size = length;
        p_params->written_size = 0;
        p_params->write_type = write_type;

        OPTIGA_PROTECTION_ENABLE(me->my_cmd, me);
        OPTIGA_PROTECTION_SET_VERSION(me->my_cmd, me);

        return_value = optiga_cmd_set_data_object(me->my_cmd, write_type, (optiga_set_data_object_params_t *)p_params);
        if (OPTIGA_LIB_SUCCESS != return_value)
        {
            me->instance_state = OPTIGA_LIB_INSTANCE_FREE;
        }
    } while (FALSE);
    optiga_util_reset_protection_level(me);

    return (return_value);
}

#ifdef OPTIGA_COMMS_SHIELDED_CONNECTION
void optiga_util_set_comms_params(optiga_util_t * me,
                                  uint8_t parameter_type,
                                  uint8_t value)
{
    switch (parameter_type)
    {
        case OPTIGA_COMMS_PROTECTION_LEVEL:
        {
            me->protection_level = value;
            break;
        }
        case OPTIGA_COMMS_PROTOCOL_VERSION:
        {
            me->protocol_version = value;
            break;
        }
        default:
        {
            break;
        }
    }
}
#endif

optiga_util_t * optiga_util_create(uint8_t optiga_instance_id,
                                   callback_handler_t handler,
                                   void * caller_context)
{
    optiga_util_t * me = NULL;

    do
    {
#ifdef OPTIGA_LIB_DEBUG_NULL_CHECK
        if (NULL == handler)
        {
            break;
        }
#endif
        me = (optiga_util_t *)pal_os_calloc(1, sizeof(optiga_util_t));
        if (NULL == me)
        {
            break;
        }

        me->handler = handler;
        me->caller_context = caller_context;
        me->instance_state = OPTIGA_LIB_SUCCESS;
#ifdef OPTIGA_COMMS_SHIELDED_CONNECTION
        me->protocol_version = OPTIGA_COMMS_PROTOCOL_VERSION_PRE_SHARED_SECRET;
        me->protection_level = OPTIGA_COMMS_DEFAULT_PROTECTION_LEVEL;
#endif
        me->my_cmd = optiga_cmd_create(optiga_instance_id, optiga_util_generic_event_handler, me);
        if (NULL == me->my_cmd)
        {
            pal_os_free(me);
            me = NULL;
        }
    } while (FALSE);

    return (me);
}

optiga_lib_status_t optiga_util_destroy(optiga_util_t * me)
{
    optiga_lib_status_t return_value;

    do
    {
#ifdef OPTIGA_LIB_DEBUG_NULL_CHECK
        if (NULL == me)
        {
            return_value = OPTIGA_UTIL_ERROR_INVALID_INPUT;
            break;
        }
#endif
        if (OPTIGA_LIB_INSTANCE_BUSY == me->instance_state)
        {
            return_value = OPTIGA_UTIL_ERROR_INSTANCE_IN_USE;
            break;
        }
        return_value = optiga_cmd_destroy(me->my_cmd);
        pal_os_free(me);
    } while (FALSE);
    return (return_value);
}

optiga_lib_status_t optiga_util_open_application(optiga_util_t * me,
                                                 bool_t perform_restore)
{
    optiga_lib_status_t return_value = OPTIGA_UTIL_ERROR;

    //OPTIGA_UTIL_LOG_MESSAGE(__FUNCTION__);
    do
    {
#ifdef OPTIGA_LIB_DEBUG_NULL_CHECK
        if ((NULL == me) || (NULL == me->my_cmd))
        {
            return_value = OPTIGA_UTIL_ERROR_INVALID_INPUT;
            break;
        }
#endif

        if (OPTIGA_LIB_INSTANCE_BUSY == me->instance_state)
        {
            return_value = OPTIGA_UTIL_ERROR_INSTANCE_IN_USE;
            break;
        }

        me->instance_state = OPTIGA_LIB_INSTANCE_BUSY;
        OPTIGA_PROTECTION_ENABLE(me->my_cmd, me);
        OPTIGA_PROTECTION_SET_VERSION(me->my_cmd, me);
#ifdef OPTIGA_COMMS_SHIELDED_CONNECTION
        if (FALSE == perform_restore)
        {
            OPTIGA_PROTECTION_MANAGE_CONTEXT(me->my_cmd, OPTIGA_COMMS_SESSION_CONTEXT_NONE);
        }
        else
        {
             OPTIGA_PROTECTION_MANAGE_CONTEXT(me->my_cmd, OPTIGA_COMMS_SESSION_CONTEXT_RESTORE);
        }
#endif //OPTIGA_COMMS_SHIELDED_CONNECTION

        return_value = optiga_cmd_open_application(me->my_cmd, perform_restore, NULL);
        if (OPTIGA_LIB_SUCCESS != return_value)
        {
            me->instance_state = OPTIGA_LIB_INSTANCE_FREE;
        }

    } while (FALSE);
    optiga_util_reset_protection_level(me);

    return (return_value);
}

optiga_lib_status_t optiga_util_close_application(optiga_util_t * me,
                                                  bool_t perform_hibernate)
{
    optiga_lib_status_t return_value = OPTIGA_UTIL_ERROR;
    OPTIGA_UTIL_LOG_MESSAGE(__FUNCTION__);

    do
    {
#ifdef OPTIGA_LIB_DEBUG_NULL_CHECK
        if ((NULL == me) || (NULL == me->my_cmd))
        {
            return_value = OPTIGA_UTIL_ERROR_INVALID_INPUT;
            break;
        }
#endif

        if (OPTIGA_LIB_INSTANCE_BUSY == me->instance_state)
        {
            return_value = OPTIGA_UTIL_ERROR_INSTANCE_IN_USE;
            break;
        }

        me->instance_state = OPTIGA_LIB_INSTANCE_BUSY;
        OPTIGA_PROTECTION_ENABLE(me->my_cmd, me);
        OPTIGA_PROTECTION_SET_VERSION(me->my_cmd, me);

#ifdef OPTIGA_COMMS_SHIELDED_CONNECTION
        if (FALSE == perform_hibernate)
        {
            OPTIGA_PROTECTION_MANAGE_CONTEXT(me->my_cmd, OPTIGA_COMMS_SESSION_CONTEXT_NONE);
        }
        else
        {
            OPTIGA_PROTECTION_MANAGE_CONTEXT(me->my_cmd, OPTIGA_COMMS_SESSION_CONTEXT_SAVE);
        }
#endif //OPTIGA_COMMS_SHIELDED_CONNECTION

        return_value = optiga_cmd_close_application(me->my_cmd, perform_hibernate, NULL);
        if (OPTIGA_LIB_SUCCESS != return_value)
        {
            me->instance_state = OPTIGA_LIB_INSTANCE_FREE;
        }

    } while (FALSE);
    optiga_util_reset_protection_level(me);

    return (return_value);
}

optiga_lib_status_t optiga_util_read_data(optiga_util_t * me,
                                          uint16_t optiga_oid,
                                          uint16_t offset,
                                          uint8_t * buffer,
                                          uint16_t * length)
{
    optiga_lib_status_t return_value = OPTIGA_UTIL_ERROR;
    optiga_get_data_object_params_t * p_params;
    OPTIGA_UTIL_LOG_MESSAGE(__FUNCTION__);
    do
    {
#ifdef OPTIGA_LIB_DEBUG_NULL_CHECK
        if ((NULL == me) || (NULL == me->my_cmd) ||
            (NULL == buffer) || (NULL == length))
        {
            return_value = OPTIGA_UTIL_ERROR_INVALID_INPUT;
            break;
        }
#endif

        if (OPTIGA_LIB_INSTANCE_BUSY == me->instance_state)
        {
            return_value = OPTIGA_UTIL_ERROR_INSTANCE_IN_USE;
            break;
        }

        me->instance_state = OPTIGA_LIB_INSTANCE_BUSY;
        p_params = (optiga_get_data_object_params_t *)&(me->params.optiga_get_data_object_params);
        pal_os_memset(&me->params,0x00,sizeof(optiga_util_params_t));

        p_params->oid = optiga_oid;
        p_params->offset = offset;
        // set option to read data
        p_params->data_or_metadata = 0;
        p_params->buffer = buffer;
        p_params->bytes_to_read = *length;
        p_params->ref_bytes_to_read = length;
        p_params->accumulated_size = 0;
        p_params->last_read_size = 0;

        OPTIGA_PROTECTION_ENABLE(me->my_cmd, me);
        OPTIGA_PROTECTION_SET_VERSION(me->my_cmd, me);

        return_value = optiga_cmd_get_data_object(me->my_cmd, p_params->data_or_metadata, p_params);
        if (OPTIGA_LIB_SUCCESS != return_value)
        {
            me->instance_state = OPTIGA_LIB_INSTANCE_FREE;
        }

    } while (FALSE);
    optiga_util_reset_protection_level(me);

    return (return_value);
}

optiga_lib_status_t optiga_util_read_metadata(optiga_util_t * me,
                                              uint16_t optiga_oid,
                                              uint8_t * buffer,
                                              uint16_t * length)
{
    optiga_lib_status_t return_value = OPTIGA_UTIL_ERROR;
    optiga_get_data_object_params_t * p_params;
    OPTIGA_UTIL_LOG_MESSAGE(__FUNCTION__);
    do
    {
#ifdef OPTIGA_LIB_DEBUG_NULL_CHECK
        if ((NULL == me) || (NULL == me->my_cmd) ||
            (NULL == buffer) || (NULL == length))
        {
            return_value = OPTIGA_UTIL_ERROR_INVALID_INPUT;
            break;
        }
#endif

        if (OPTIGA_LIB_INSTANCE_BUSY == me->instance_state)
        {
            return_value = OPTIGA_UTIL_ERROR_INSTANCE_IN_USE;
            break;
        }

        me->instance_state = OPTIGA_LIB_INSTANCE_BUSY;
        p_params = (optiga_get_data_object_params_t *)&(me->params.optiga_get_data_object_params);
        pal_os_memset(&me->params,0x00,sizeof(optiga_util_params_t));

        p_params->oid = optiga_oid;
        p_params->offset = 0;
        // set option to read metadata
        p_params->data_or_metadata = 1;//for metadata
        p_params->buffer = buffer;
        p_params->bytes_to_read = *length;
        p_params->ref_bytes_to_read = length;
        p_params->accumulated_size = 0;
        p_params->last_read_size = 0;
        OPTIGA_PROTECTION_ENABLE(me->my_cmd, me);
        OPTIGA_PROTECTION_SET_VERSION(me->my_cmd, me);

        return_value = optiga_cmd_get_data_object(me->my_cmd, p_params->data_or_metadata,
                                                  (optiga_get_data_object_params_t *)p_params);
        if (OPTIGA_LIB_SUCCESS != return_value)
        {
            me->instance_state = OPTIGA_LIB_INSTANCE_FREE;
        }
    } while (FALSE);
    optiga_util_reset_protection_level(me);

    return (return_value);
}

optiga_lib_status_t optiga_util_write_data(optiga_util_t * me,
                                           uint16_t optiga_oid,
                                           uint8_t write_type,
                                           uint16_t offset,
                                           const uint8_t * buffer,
                                           uint16_t length)
{
    optiga_lib_status_t return_value = OPTIGA_UTIL_ERROR_INVALID_INPUT;
    OPTIGA_UTIL_LOG_MESSAGE(__FUNCTION__);
    do
    {
        if ((OPTIGA_UTIL_WRITE_ONLY != write_type) && (OPTIGA_UTIL_ERASE_AND_WRITE != write_type))
        {
            break;
        }
        return_value =  optiga_util_write_data_wrapper(me,
                                                       optiga_oid,
                                                       write_type,
                                                       offset,
                                                       buffer,
                                                       length);
    } while (FALSE);
    return (return_value);
}

optiga_lib_status_t optiga_util_write_metadata(optiga_util_t * me,
                                               uint16_t optiga_oid,
                                               const uint8_t * buffer,
                                               uint8_t length)
{
    optiga_lib_status_t return_value = OPTIGA_UTIL_ERROR;
    optiga_set_data_object_params_t * p_params;
    OPTIGA_UTIL_LOG_MESSAGE(__FUNCTION__);
    do
    {
#ifdef OPTIGA_LIB_DEBUG_NULL_CHECK
        if ((NULL == me) || (NULL == me->my_cmd) || (NULL == buffer))
        {
            return_value = OPTIGA_UTIL_ERROR_INVALID_INPUT;
            break;
        }
#endif

        if (OPTIGA_LIB_INSTANCE_BUSY == me->instance_state)
        {
            return_value = OPTIGA_UTIL_ERROR_INSTANCE_IN_USE;
            break;
        }

        me->instance_state = OPTIGA_LIB_INSTANCE_BUSY;
        p_params = (optiga_set_data_object_params_t *)&(me->params.optiga_set_data_object_params);
        pal_os_memset(&me->params,0x00,sizeof(optiga_util_params_t));

        p_params->oid = optiga_oid;
        p_params->offset = 0;
        //for Metadata
        p_params->data_or_metadata = 1;
        p_params->buffer = buffer;
        p_params->size = length;
        p_params->write_type = 1;
        p_params->written_size = 0;
        OPTIGA_PROTECTION_ENABLE(me->my_cmd, me);
        OPTIGA_PROTECTION_SET_VERSION(me->my_cmd, me);

        return_value = optiga_cmd_set_data_object(me->my_cmd, p_params->write_type,
                                                  (optiga_set_data_object_params_t *)p_params);
        if (OPTIGA_LIB_SUCCESS != return_value)
        {
            me->instance_state = OPTIGA_LIB_INSTANCE_FREE;
        }
    } while (FALSE);
    optiga_util_reset_protection_level(me);

    return (return_value);
}

_STATIC_H optiga_lib_status_t optiga_util_protected_update(optiga_util_t * me,
                                                           uint8_t manifest_version,
                                                           const uint8_t * p_buffer,
                                                           uint16_t buffer_length,
                                                           optiga_set_obj_protected_tag_t set_obj_tag)
{
    optiga_lib_status_t return_value = OPTIGA_UTIL_ERROR;
    optiga_set_object_protected_params_t * p_params;

    do
    {
        if (OPTIGA_LIB_INSTANCE_BUSY == me->instance_state)
        {
            return_value = OPTIGA_UTIL_ERROR_INSTANCE_IN_USE;
            break;
        }

        me->instance_state = OPTIGA_LIB_INSTANCE_BUSY;
        p_params = (optiga_set_object_protected_params_t *)&(me->params.optiga_set_object_protected_params);

        if(OPTIGA_SET_PROTECTED_UPDATE_START == set_obj_tag)
        {
            pal_os_memset(&me->params,0x00,sizeof(optiga_util_params_t));
            p_params->manifest_version = manifest_version;
            OPTIGA_PROTECTION_ENABLE(me->my_cmd, me);
            OPTIGA_PROTECTION_SET_VERSION(me->my_cmd, me);
        }

        p_params->p_protected_update_buffer = p_buffer;
        p_params->p_protected_update_buffer_length = buffer_length;
        p_params->set_obj_protected_tag = set_obj_tag;

        return_value = optiga_cmd_set_object_protected(me->my_cmd, p_params->manifest_version,p_params);
        if (OPTIGA_LIB_SUCCESS != return_value)
        {
            me->instance_state = OPTIGA_LIB_INSTANCE_FREE;
        }
    } while (FALSE);

    return (return_value);
}

optiga_lib_status_t optiga_util_protected_update_start(optiga_util_t * me,
                                                       uint8_t manifest_version,
                                                       const uint8_t * manifest,
                                                       uint16_t manifest_length)
{
    optiga_lib_status_t return_value = OPTIGA_UTIL_ERROR;
    OPTIGA_UTIL_LOG_MESSAGE(__FUNCTION__);

    do
    {
#ifdef OPTIGA_LIB_DEBUG_NULL_CHECK
        if ((NULL == me) || (NULL == me->my_cmd) || (NULL == manifest))
        {
            return_value = OPTIGA_UTIL_ERROR_INVALID_INPUT;
            break;
        }
#endif

        return_value = optiga_util_protected_update(me,
                                                    manifest_version,
                                                    manifest,
                                                    manifest_length,
                                                    OPTIGA_SET_PROTECTED_UPDATE_START
                                                    );
    } while (FALSE);
    optiga_util_reset_protection_level(me);
    return (return_value);
}

optiga_lib_status_t optiga_util_protected_update_continue(optiga_util_t * me,
                                                          const uint8_t * fragment,
                                                          uint16_t fragment_length)
{
    optiga_lib_status_t return_value = OPTIGA_UTIL_ERROR;
    OPTIGA_UTIL_LOG_MESSAGE(__FUNCTION__);

    do
    {
#ifdef OPTIGA_LIB_DEBUG_NULL_CHECK
        if ((NULL == me) || (NULL == me->my_cmd) || (NULL == fragment))
        {
            return_value = OPTIGA_UTIL_ERROR_INVALID_INPUT;
            break;
        }
#endif

        return_value = optiga_util_protected_update(me,
                                                    0x00,
                                                    fragment,
                                                    fragment_length,
                                                    OPTIGA_SET_PROTECTED_UPDATE_CONTINUE);
    } while (FALSE);
    return (return_value);
}

optiga_lib_status_t optiga_util_protected_update_final(optiga_util_t * me,
                                                       const uint8_t * fragment,
                                                       uint16_t fragment_length)
{
    optiga_lib_status_t return_value = OPTIGA_UTIL_ERROR;
    OPTIGA_UTIL_LOG_MESSAGE(__FUNCTION__);

    do
    {
#ifdef OPTIGA_LIB_DEBUG_NULL_CHECK
        if ((NULL == me) || (NULL == me->my_cmd))
        {
            return_value = OPTIGA_UTIL_ERROR_INVALID_INPUT;
            break;
        }
#endif

        return_value = optiga_util_protected_update(me,
                                                    0x00,
                                                    fragment,
                                                    fragment_length,
                                                    OPTIGA_SET_PROTECTED_UPDATE_FINAL);
    } while (FALSE);
    return (return_value);
}

optiga_lib_status_t optiga_util_update_count(optiga_util_t * me,
                                             uint16_t optiga_counter_oid,
                                             uint8_t count)
{
    const uint8_t count_value[] = {count};
    OPTIGA_UTIL_LOG_MESSAGE(__FUNCTION__);
    return (optiga_util_write_data_wrapper(me,
                                          optiga_counter_oid,
                                          OPTIGA_UTIL_COUNT_DATA_OBJECT,
                                          0x0000,
                                          count_value,
                                          sizeof(count_value)));
}

optiga_lib_status_t optiga_direct_write(uint16_t oid, uint8_t* data, uint16_t data_size)
{
	int32_t status  = (int32_t)OPTIGA_DEVICE_ERROR;
	optiga_util_t * me = NULL;
	uint16_t offset;
	do
	{
		// Sanity check
		if ((NULL == data) || (0 == oid) || (0 == data_size))
		{
			break;
		}
		me = optiga_util_create(0, optiga_crypt_callback, NULL);
		if (NULL == me)
		{
			break;
		}
        // OPTIGA Comms Shielded connection settings to enable the protection
        OPTIGA_UTIL_SET_COMMS_PROTECTION_LEVEL(me, OPTIGA_COMMS_NO_PROTECTION);

        optiga_lib_status = OPTIGA_LIB_BUSY;
        offset = 0x00;
		//Write Data
        status = optiga_util_write_data(me,oid,OPTIGA_UTIL_ERASE_AND_WRITE,offset,data,data_size);
		if(OPTIGA_LIB_SUCCESS != status)
		{
			configPRINTF(("Error: optiga_util_write_data error status=0x%x\r\n", status));
			break;
		}

        while (OPTIGA_LIB_BUSY == optiga_lib_status)
        {
            //Wait until the optiga_util_read_data operation is completed
        }

        if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
        {
            //writing data to a data object failed.
            status = optiga_lib_status;
            break;
        }

	}while(FALSE);
	return status;
}

optiga_lib_status_t optiga_direct_read(uint16_t oid, uint8_t* data, uint16_t* data_size)
{
	int32_t status  = (int32_t)OPTIGA_DEVICE_ERROR;
	optiga_util_t * me = NULL;

	//memset(data, 0, *data_size);
	do
	{
		// Sanity check
		if ((NULL == data) || (NULL == data_size) ||
			(0 == oid) || (0 == *data_size))
		{
			break;
		}
		me = optiga_util_create(0, optiga_crypt_callback, NULL);
		if (NULL == me)
		{
			break;
		}
        // OPTIGA Comms Shielded connection settings to enable the protection
        OPTIGA_UTIL_SET_COMMS_PROTOCOL_VERSION(me, OPTIGA_COMMS_PROTOCOL_VERSION_PRE_SHARED_SECRET);
        OPTIGA_UTIL_SET_COMMS_PROTECTION_LEVEL(me, OPTIGA_COMMS_RESPONSE_PROTECTION);
        optiga_lib_status = OPTIGA_LIB_BUSY;

		//Read end-entity device certificate
		status = optiga_util_read_data((optiga_util_t *)me, oid, 0, data, data_size);
		if(OPTIGA_LIB_SUCCESS != status)
		{
			configPRINTF(("Error: optiga_util_read_data error status=0x%x\r\n", status));
			break;
		}

        while (OPTIGA_LIB_BUSY == optiga_lib_status)
        {
            //Wait until the optiga_util_read_data operation is completed
        }

        if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
        {
            //writing data to a data object failed.
            status = optiga_lib_status;
            break;
        }
	}while(FALSE);
	return status;
}


/**
* @}
*/
