#include "optiga/optiga_crypt.h"
#include "optiga/optiga_util.h"
#include "optiga_example.h"
#include "mutualauth_crypto.h"
#include "iot_demo_logging.h"
#include "mbedtls/aes.h"
#include "aws_ble_mutual_auth.h"
#define AES_KEY_BITS 256

extern ECDH_Secret_t secret;

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

/**
 * The below example demonstrates the generation of random using OPTIGA.
 *
 * Example for #optiga_crypt_random
 *
 */
optiga_lib_status_t mutualauth_optiga_crypt_random(Random_Data_t *random_num)
{
    optiga_crypt_t * me = NULL;
    optiga_lib_status_t return_status = 0;
    optiga_lib_status_t status = 0;
    OPTIGA_EXAMPLE_LOG_MESSAGE(__FUNCTION__);

    do
    {
        /**
         * 1. Create OPTIGA Crypt Instance
         *
         */
        me = optiga_crypt_create(0, optiga_crypt_callback, NULL);
        if (NULL == me)
        {
            break;
        }

        /**
         * 2. Generate Random -
         *       - Specify the Random type as TRNG
         */
        optiga_lib_status = OPTIGA_LIB_BUSY;

        return_status = optiga_crypt_random(me,
                                            OPTIGA_RNG_TYPE_TRNG,
											random_num->random_data_buffer,
                                            sizeof(random_num->random_data_buffer));

        if (OPTIGA_LIB_SUCCESS != return_status)
        {
            break;
        }

        while (OPTIGA_LIB_BUSY == optiga_lib_status)
        {
            //Wait until the optiga_crypt_random operation is completed
        }

        if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
        {
            return_status = optiga_lib_status;
            break;
        }
        return_status = OPTIGA_LIB_SUCCESS;

    } while (FALSE);
    OPTIGA_EXAMPLE_LOG_STATUS(return_status);
    status = return_status;					/* assign current status */

    if (me)
    {
        //Destroy the instance after the completion of usecase if not required.
        return_status = optiga_crypt_destroy(me);
        if(OPTIGA_LIB_SUCCESS != return_status)
        {
            //lint --e{774} suppress This is a generic macro
            OPTIGA_EXAMPLE_LOG_STATUS(return_status);
            status = return_status;
        }
    }
    return status;
}

/**
 * The below example demonstrates the generation of
 * ECC Key pair using #optiga_crypt_ecc_generate_keypair.
 *
 */
optiga_lib_status_t mutualauth_optiga_crypt_ecc_generate_keypair(ECC_Pubkey_t *pubkey)
{
    optiga_lib_status_t return_status = 0;
    optiga_key_id_t optiga_key_id;
    optiga_lib_status_t status = 0;
    uint8_t key_usage;

    optiga_crypt_t * me = NULL;
    OPTIGA_EXAMPLE_LOG_MESSAGE(__FUNCTION__);

    do
    {
        /**
         * 1. Create OPTIGA Crypt Instance
         */
        me = optiga_crypt_create(0, optiga_crypt_callback, NULL);
        if (NULL == me)
        {
            break;
        }

        /**
         * 2. Generate ECC Key pair
         *       - Use ECC NIST P 256 Curve
         *       - Specify the Key Usage (Key Agreement or Sign based on requirement)
         *       - Store the Private key in OPTIGA Key store
         *       - Export Public Key
         */
        optiga_lib_status = OPTIGA_LIB_BUSY;
        optiga_key_id = pubkey->optiga_key_id;
        key_usage = pubkey->key_usage;
        //for Session based, use OPTIGA_KEY_ID_SESSION_BASED as key id as shown below.
        //optiga_key_id = OPTIGA_KEY_ID_SESSION_BASED;
        return_status = optiga_crypt_ecc_generate_keypair(me,
                                                          OPTIGA_ECC_CURVE_NIST_P_256,
                                                          (uint8_t)key_usage,
                                                          FALSE,
                                                          &optiga_key_id,
														  pubkey->public_key,
                                                          &pubkey->Length);
        if (OPTIGA_LIB_SUCCESS != return_status)
        {
            break;
        }

        while (OPTIGA_LIB_BUSY == optiga_lib_status)
        {
            //Wait until the optiga_crypt_ecc_generate_keypair operation is completed
        }

        if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
        {
            //Key pair generation failed
            return_status = optiga_lib_status;
            break;
        }
        return_status = OPTIGA_LIB_SUCCESS;
    } while (FALSE);
    OPTIGA_EXAMPLE_LOG_STATUS(return_status);
    status = return_status;					/* assign current status */
    if (me)
    {
        //Destroy the instance after the completion of usecase if not required.
        return_status = optiga_crypt_destroy(me);
        if(OPTIGA_LIB_SUCCESS != return_status)
        {
            //lint --e{774} suppress This is a generic macro
            OPTIGA_EXAMPLE_LOG_STATUS(return_status);
            status = return_status;
        }
    }
    return status;
}

/**
 * The below example demonstrates the signing of digest using
 * the Private key in OPTIGA Key store.
 *
 * Example for #optiga_crypt_ecdsa_sign
 *
 */
optiga_lib_status_t mutualauth_optiga_crypt_ecdsa_sign(ECC_Sign_t *sign)
{

	optiga_key_id_t optiga_key_id;
    optiga_crypt_t * me = NULL;
    optiga_lib_status_t return_status = 0;
    optiga_lib_status_t status = 0;
    OPTIGA_EXAMPLE_LOG_MESSAGE(__FUNCTION__);

    do
    {
        /**
         * 1. Create OPTIGA Crypt Instance
         */
        me = optiga_crypt_create(0, optiga_crypt_callback, NULL);
        if (NULL == me)
        {
            break;
        }

        /**
         * 2. Sign the digest using Private key from Key Store ID E0F0
         */
        optiga_key_id = sign->optiga_key_id;
        optiga_lib_status = OPTIGA_LIB_BUSY;
        return_status = optiga_crypt_ecdsa_sign(me,
        										sign->digest,
                                                sizeof(sign->digest),
												optiga_key_id,
												sign->signature,
                                                &sign->signature_length);

        if (OPTIGA_LIB_SUCCESS != return_status)
        {
            break;
        }

        while (OPTIGA_LIB_BUSY == optiga_lib_status)
        {
            //Wait until the optiga_crypt_ecdsa_sign operation is completed
        }

        if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
        {
            return_status = optiga_lib_status;
            break;
        }
        return_status = OPTIGA_LIB_SUCCESS;

    } while (FALSE);
    OPTIGA_EXAMPLE_LOG_STATUS(return_status);
    status = return_status;					/* assign current status */
    if (me)
    {
        //Destroy the instance after the completion of usecase if not required.
        return_status = optiga_crypt_destroy(me);
        if(OPTIGA_LIB_SUCCESS != return_status)
        {
            //lint --e{774} suppress This is a generic macro
            OPTIGA_EXAMPLE_LOG_STATUS(return_status);
            status = return_status;
        }
    }
    return status;
}

/**
 * The below example demonstrates the verification of signature using
 * the public key provided by host.
 *
 * Example for #optiga_crypt_ecdsa_verify
 *
 */
optiga_lib_status_t mutualauth_optiga_crypt_ecdsa_verify(ECC_Verify_t *verify)
{
    public_key_from_host_t public_key_details = {
    											 verify->public_key,
												 sizeof(verify->public_key),
                                                 (uint8_t)OPTIGA_ECC_CURVE_NIST_P_256
                                                };

    optiga_lib_status_t return_status = 0;
    optiga_lib_status_t status = 0;
    optiga_crypt_t * me = NULL;
    OPTIGA_EXAMPLE_LOG_MESSAGE(__FUNCTION__);
    do
    {
        /**
         * 1. Create OPTIGA Crypt Instance
         */
        me = optiga_crypt_create(0, optiga_crypt_callback, NULL);
        if (NULL == me)
        {
            break;
        }

        /**
         * 2. Verify ECDSA signature using public key from host
         */
        optiga_lib_status = OPTIGA_LIB_BUSY;
        return_status = optiga_crypt_ecdsa_verify (me,
        										   verify->digest,
                                                   sizeof(verify->digest),
												   verify->signature,
												   verify->sig_length,
                                                   OPTIGA_CRYPT_HOST_DATA,
                                                   &public_key_details);

        if (OPTIGA_LIB_SUCCESS != return_status)
        {
            break;
        }

        while (OPTIGA_LIB_BUSY == optiga_lib_status)
        {
            //Wait until the optiga_crypt_ecdsa_verify operation is completed
        }

        if ((OPTIGA_LIB_SUCCESS != optiga_lib_status))
        {
            //Signature verification failed.
            return_status = optiga_lib_status;
            break;
        }
        return_status = OPTIGA_LIB_SUCCESS;
    } while (FALSE);
    OPTIGA_EXAMPLE_LOG_STATUS(return_status);
    status = return_status;					/* assign current status */

    if (me)
    {
        //Destroy the instance after the completion of usecase if not required.
        return_status = optiga_crypt_destroy(me);
        if(OPTIGA_LIB_SUCCESS != return_status)
        {
            //lint --e{774} suppress This is a generic macro
            OPTIGA_EXAMPLE_LOG_STATUS(return_status);
            status = return_status;
        }
    }
    return status;
}

/**
 * Prepare the hash context
 */
#define OPTIGA_HASH_CONTEXT_INIT(hash_context,p_context_buffer,context_buffer_size,hash_type) \
{                                                               \
    hash_context.context_buffer = p_context_buffer;             \
    hash_context.context_buffer_length = context_buffer_size;   \
    hash_context.hash_algo = hash_type;                         \
}

/**
 * The below example demonstrates the generation of digest using
 * optiga_crypt_hash_xxxx operations.
 *
 * Example for #optiga_crypt_hash_start, #optiga_crypt_hash_update,
 * #optiga_crypt_hash_finalize
 *
 */
optiga_lib_status_t mutualauth_optiga_crypt_hash(Hash_Data_t *hash)
{
    optiga_lib_status_t return_status = 0;

    optiga_lib_status_t status = 0;
    uint8_t hash_context_buffer [130];
    optiga_hash_context_t hash_context;

    hash_data_from_host_t hash_data_host;

    optiga_crypt_t * me = NULL;
    OPTIGA_EXAMPLE_LOG_MESSAGE(__FUNCTION__);

    do
    {
        /**
         * 1. Create OPTIGA Crypt Instance
         */
        me = optiga_crypt_create(0, optiga_crypt_callback, NULL);
        if (NULL == me)
        {
            break;
        }

        /**
         * 2. Initialize the Hash context
         */
        OPTIGA_HASH_CONTEXT_INIT(hash_context,hash_context_buffer,  \
                                 sizeof(hash_context_buffer),(uint8_t)OPTIGA_HASH_TYPE_SHA_256);

        optiga_lib_status = OPTIGA_LIB_BUSY;

        /**
         * 3. Initialize the hashing context at OPTIGA
         */
        return_status = optiga_crypt_hash_start(me, &hash_context);
        if (OPTIGA_LIB_SUCCESS != return_status)
        {
            break;
        }

        while (OPTIGA_LIB_BUSY == optiga_lib_status)
        {
            //Wait until the optiga_crypt_hash_start operation is completed
        }

        if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
        {
            return_status = optiga_lib_status;
            break;
        }

        /**
         * 4. Continue hashing the data
         */
        hash_data_host.buffer = hash->data_to_hash;
        hash_data_host.length = hash->Length;

        optiga_lib_status = OPTIGA_LIB_BUSY;
        return_status = optiga_crypt_hash_update(me,
                                                 &hash_context,
                                                 OPTIGA_CRYPT_HOST_DATA,
                                                 &hash_data_host);
        if (OPTIGA_LIB_SUCCESS != return_status)
        {
            break;
        }

        while (OPTIGA_LIB_BUSY == optiga_lib_status)
        {
            //Wait until the optiga_crypt_hash_update operation is completed
        }

        if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
        {
            return_status = optiga_lib_status;
            break;
        }

        /**
         * 5. Finalize the hash
         */
        optiga_lib_status = OPTIGA_LIB_BUSY;
        return_status = optiga_crypt_hash_finalize(me,
                                                   &hash_context,
												   hash->digest);

        if (OPTIGA_LIB_SUCCESS != return_status)
        {
            break;
        }

        while (OPTIGA_LIB_BUSY == optiga_lib_status)
        {
            //Wait until the optiga_crypt_hash_finalize operation is completed
        }

        if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
        {
            return_status = optiga_lib_status;
            break;
        }
        return_status = OPTIGA_LIB_SUCCESS;

    } while (FALSE);
    status = return_status;					/* assign current status */
    OPTIGA_EXAMPLE_LOG_STATUS(return_status);

    if (me)
    {
        //Destroy the instance after the completion of usecase if not required.
        return_status = optiga_crypt_destroy(me);
        if(OPTIGA_LIB_SUCCESS != return_status)
        {
            //lint --e{774} suppress This is a generic macro
            OPTIGA_EXAMPLE_LOG_STATUS(return_status);
            status = return_status;
        }
    }
    return status;
}

/**
 * The below example demonstrates the generation of
 * shared secret using #optiga_crypt_ecdh with a session based approach.
 *
 */
optiga_lib_status_t mutualauth_optiga_crypt_ecdh(ECDH_Secret_t *secret)
{
    optiga_lib_status_t return_status = 0;
    optiga_lib_status_t status = 0;

    optiga_key_id_t optiga_key_id;
    static public_key_from_host_t peer_public_key_details;
    peer_public_key_details.public_key = (uint8_t *)secret->peer_public_key;
    peer_public_key_details.length = sizeof(secret->peer_public_key);
    peer_public_key_details.key_type = (uint8_t)secret->key_type;

    optiga_crypt_t * me = NULL;
    OPTIGA_EXAMPLE_LOG_MESSAGE(__FUNCTION__);

    do
    {
        /**
         * 1. Create OPTIGA Crypt Instance
         */
        me = optiga_crypt_create(0, optiga_crypt_callback, NULL);
        if (NULL == me)
        {
            break;
        }

        /**
         * 2. Perform ECDH using the Peer Public key
         *       - Use ECC NIST P 256 Curve
         *       - Provide the peer public key details
         *       - Export the generated shared secret with protected I2C communication
         */
        optiga_lib_status = OPTIGA_LIB_BUSY;
        optiga_key_id = secret->optiga_key_id;	/*Default OPTIGA_KEY_ID_SESSION_BASED */
        OPTIGA_CRYPT_SET_COMMS_PROTECTION_LEVEL(me, OPTIGA_COMMS_COMMAND_PROTECTION);
        OPTIGA_CRYPT_SET_COMMS_PROTOCOL_VERSION(me, OPTIGA_COMMS_PROTOCOL_VERSION_PRE_SHARED_SECRET);
        return_status = optiga_crypt_ecdh(me,
                                          optiga_key_id,
                                          &peer_public_key_details,
                                          TRUE,
										  secret->shared_secret);
        if (OPTIGA_LIB_SUCCESS != return_status)
        {
            break;
        }

        while (OPTIGA_LIB_BUSY == optiga_lib_status)
        {
            //Wait until the optiga_crypt_ecdh operation is completed
        }

        if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
        {
            //ECDH Operation failed.
            return_status = optiga_lib_status;
            break;
        }
        return_status = OPTIGA_LIB_SUCCESS;
    } while (FALSE);
    OPTIGA_EXAMPLE_LOG_STATUS(return_status);
    status = return_status;					/* assign current status */

    if (me)
    {
        //Destroy the instance after the completion of usecase if not required.
        return_status = optiga_crypt_destroy(me);
        if(OPTIGA_LIB_SUCCESS != return_status)
        {
            //lint --e{774} suppress This is a generic macro
            OPTIGA_EXAMPLE_LOG_STATUS(return_status);
            status = return_status;
        }
    }
    return status;
}

optiga_lib_status_t mutualauth_optiga_write(uint16_t oid, uint8_t* data, uint16_t data_size)
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

optiga_lib_status_t mutualauth_optiga_read(uint16_t oid, uint8_t* data, uint16_t* data_size)
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
*
* Read End Device Certificate stored in OPTIGA™ Trust M
*
* \param[in]        chip_cert_oid          Certificate OID
* \param[in]        chip_cert	           Pointer to certificate key buffer
* \param[in,out]    chip_cert_size	   	   Pointer to certificate buffer size
*
* \retval    #OPTIGA_LIB_SUCCESS
* \retval    #OPTIGA_LIB_ERROR
*
*/
optiga_lib_status_t read_device_cert(uint16_t cert_oid, uint8_t* p_cert, uint16_t* p_cert_size)
{
	int32_t status  = (int32_t)OPTIGA_DEVICE_ERROR;
	uint8_t tmp_cert[LENGTH_OPTIGA_CERT];
	uint8_t* p_tmp_cert_pointer = tmp_cert;

	memset(tmp_cert, 0, LENGTH_OPTIGA_CERT);
	do
	{
		// Sanity check
		if ((NULL == p_cert) || (NULL == p_cert_size) ||
			(0 == cert_oid) || (0 == *p_cert_size) || (LENGTH_OPTIGA_CERT > *p_cert_size))
		{
			break;
		}
		status = mutualauth_optiga_read(cert_oid,p_tmp_cert_pointer,p_cert_size);
		if(OPTIGA_LIB_SUCCESS != status)
		{
			configPRINTF(("Error: read_device_cert error status=0x%x\r\n", status));
			break;
		}

		// Refer to the Solution Reference Manual (SRM) v1.35 Table 30. Certificate Types
		switch (p_tmp_cert_pointer[0])
		{
		/* One-Way Authentication Identity. Certificate DER coded The first byte
		*  of the DER encoded certificate is 0x30 and is used as Tag to differentiate
		*  from other Public Key Certificate formats defined below.
		*/
		case 0x30:
			/* The certificate can be directly used */
			status = OPTIGA_LIB_SUCCESS;
			break;
		/* TLS Identity. Tag = 0xC0; Length = Value length (2 Bytes); Value = Certificate Chain
		 * Format of a "Certificate Structure Message" used in TLS Handshake
		 */
		case 0xC0:
			/* There might be a certificate chain encoded.
			 * For this example we will consider only one certificate in the chain
			 */
			p_tmp_cert_pointer = p_tmp_cert_pointer + 9;
			*p_cert_size = *p_cert_size - 9;
			memcpy(p_cert, p_tmp_cert_pointer, *p_cert_size);
			status = OPTIGA_LIB_SUCCESS;
			break;
		/* USB Type-C identity
		 * Tag = 0xC2; Length = Value length (2 Bytes); Value = USB Type-C Certificate Chain [USB Auth].
		 * Format as defined in Section 3.2 of the USB Type-C Authentication Specification (SRM)
		 */
		case 0xC2:
		// Not supported for this example
		// Certificate type isn't supported or a wrong tag
		default:
			break;
		}

	}while(FALSE);


#if (PRINT_DEVICE_CERT ==1)
		int i=0;
        for(i=0; i<(*p_cert_size);)
        {
        	if((*p_cert_size)-i < 16)
        	{
        		break;
        	}
        	printf("%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\r\n",
        	     			p_tmp_cert_pointer[i+0], p_tmp_cert_pointer[i+1],p_tmp_cert_pointer[i+2],p_tmp_cert_pointer[i+3],
        	       			p_tmp_cert_pointer[i+4], p_tmp_cert_pointer[i+5],p_tmp_cert_pointer[i+6],p_tmp_cert_pointer[i+7],
        	       			p_tmp_cert_pointer[i+8], p_tmp_cert_pointer[i+9],p_tmp_cert_pointer[i+10],p_tmp_cert_pointer[i+11],
        	       			p_tmp_cert_pointer[i+12], p_tmp_cert_pointer[i+13],p_tmp_cert_pointer[i+14],p_tmp_cert_pointer[i+15]);
        	i+=16;
        }

        //FreeRTOS cannot print properly without newline.
        if((*p_cert_size)-i !=0)
		{
			int remainder=(*p_cert_size)-i;
			while(remainder!=0)
			{
				printf("%.2X\r\n",p_tmp_cert_pointer[i]);
				i++;
				remainder--;
			}
			configPRINTF(("\r\n"));
		}
#endif
	return status;
}
/**
 * aes cfb8 encryption.
 * \param[in]       payload_in          pointer to the input/plaintext payload
 * \param[out]      payload_out         pointer to the output/encrypted payload
 * \param[in]       frame          		length of the buffer
 *
 * \retval    #zero
 * \retval    #non zero
 */
uint8_t aes_encrypt(uint8_t *payload_in, uint8_t *payload_out, uint32_t length)
{
	mbedtls_aes_context ctx;

    mbedtls_aes_init( &ctx );
    int ret = 0;
    uint8_t iv[16] = {0x65,0x69,0x6e,0x66,0x6f,0x63,0x68,0x69,0x70,0x73,0x5f,0x61,0x72,0x72,0x6f,0x77};

    if( ( ret = mbedtls_aes_setkey_enc( &ctx, secret.shared_secret, AES_KEY_BITS ) ) != 0 )
    	IotLogError("set AES key fail\r\n");

    ret = mbedtls_aes_crypt_cfb8(&ctx,MBEDTLS_AES_ENCRYPT,length,iv,payload_in, payload_out);
    if( ret != 0 )
    	IotLogError("AES encryption fail\r\n");

    return ret;
}

/**
 * aes cfb8 decryption.
 * \param[in]       payload_in          pointer to the input/encrypted payload
 * \param[out]      payload_out         pointer to the output/plaintext payload
 * \param[in]       frame          		length of the buffer
 *
 * \retval    #zero
 * \retval    #non zero
 */
uint8_t aes_decrypt(uint8_t *payload_in, uint8_t *payload_out, uint32_t length)
{
	mbedtls_aes_context ctx;

    mbedtls_aes_init( &ctx );
    int ret = 0;
    uint8_t iv[16] = {0x65,0x69,0x6e,0x66,0x6f,0x63,0x68,0x69,0x70,0x73,0x5f,0x61,0x72,0x72,0x6f,0x77};

    if( ( ret = mbedtls_aes_setkey_enc( &ctx, secret.shared_secret, AES_KEY_BITS ) ) != 0 )
    	IotLogError("set AES key fail\r\n");

    ret = mbedtls_aes_crypt_cfb8(&ctx,MBEDTLS_AES_DECRYPT,length,iv,payload_in, payload_out);
    if( ret != 0 )
    	IotLogError("AES decription fail\r\n");

    return ret;
}
#if 0 /* old crc */
/* CRC16 */
#define POLY 0x8408

uint16_t crc16(uint8_t *data_p, uint32_t length)
{
      uint8_t i;
      uint32_t data;
      uint32_t crc = 0xffff;
      IotLogInfo("data = %x\n",data_p[0]);
      if (length == 0)
            return (~crc);

      do
      {
            for (i=0, data=(uint32_t)0xff & *data_p++;
                 i < 8;
                 i++, data >>= 1)
            {
                  if ((crc & 0x0001) ^ (data & 0x0001))
                        crc = (crc >> 1) ^ POLY;
                  else  crc >>= 1;
            }
      } while (--length);

      crc = ~crc;
      data = crc;
      crc = (crc << 8) | (data >> 8 & 0xff);

      return (crc);
}
#endif
#if 1
uint16_t crc16(uint8_t* pData, uint32_t length)
{
    uint8_t i;
    uint16_t wCrc = 0xffff;
    while (length--) {
        wCrc ^= *(uint8_t *)pData++ << 8;
        for (i=0; i < 8; i++)
            wCrc = wCrc & 0x8000 ? (wCrc << 1) ^ 0x1021 : wCrc << 1;
    }
    return wCrc & 0xffff;
}
#endif
/**
 * Convert byte array to short
 * \param[in]       data          		pointer to byte array
 * \param[in]		offset	            offset of array
 * \param[in]    	s	   	   			pointer to short int
 *
 */
void ByteArrayToShort(uint8_t *data, uint16_t offset, uint16_t *s) {

        *s = (((data[offset] << 8)) | ((data[offset+1] & 0xff)));
}

/**
 * Convert short int to byte array
 * \param[in]       s          			short int
 * \param[in]		byte	            pointer to byte array
 */
void ShortToByteArray(uint16_t s,uint8_t *byte) {
        byte[0] = (uint8_t) ((s & 0xFF00) >> 8);
        byte[1] = (uint8_t) (s & 0x00FF);
}

/**
 * Create payload frame
 * \param[in]       frame          		pointer to the output frame
 * \param[in]		data	            pointer to input data
 * \param[in]		data_length	        data buffer length
 */
void create_payload_frame(MUTUALAUTH_Edge_Data_t *frame , uint8_t *data, uint16_t data_length)
{
	uint16_t crc_value=0;
	uint8_t crc_buf[2]={0};
	uint8_t length_buf[2]={0};
	MUTUALAUTH_DEBUG("create_payload_frame \n");
	crc_value = crc16(data,data_length);
	ShortToByteArray(crc_value,crc_buf);
	MUTUALAUTH_DEBUG("crc_buf[0]=%x ,crc_buf[1]=%x \n",crc_buf[0],crc_buf[1]);
	ShortToByteArray(frame->Length,length_buf);
	MUTUALAUTH_DEBUG("length_buf[0]=%x ,length_buf[1]=%x \n",length_buf[0],length_buf[1]);
	memset(frame->pPayload,0,frame->Length);

	memcpy(frame->pPayload,length_buf,sizeof(length_buf));
	memcpy(frame->pPayload+sizeof(length_buf),data,data_length);
	memcpy(frame->pPayload+(sizeof(length_buf)+data_length),crc_buf,sizeof(crc_buf));

}
/**
 * parse payload frame and check crc16
 * \param[in]       frame          		pointer to the input frame
 *
 * \retval    #CRC_VERIFIED_PASS
 * \retval    #CRC_VERIFIED_FAIL
 */
int8_t parse_and_validate_frame(uint8_t *frame)
{
	uint16_t frame_length=0;
	uint16_t frame_crc=0;
	uint16_t cal_crc=0;
	ByteArrayToShort(frame,0,&frame_length);
	MUTUALAUTH_DEBUG("payload frame length=%d \n",frame_length);
	ByteArrayToShort(frame,frame_length-2,&frame_crc);
	MUTUALAUTH_DEBUG("payload frame crc=%d \n",frame_crc);
	cal_crc = crc16(frame+2,frame_length-4);
	MUTUALAUTH_DEBUG("payload calculated crc=%d \n",cal_crc);
	return (cal_crc == frame_crc) ? CRC_VERIFIED_PASS : CRC_VERIFIED_FAIL;
}
/**
* @}
*/
