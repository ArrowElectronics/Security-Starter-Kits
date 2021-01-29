#ifndef __AWS_OTA_PAL_SETTINGS
#define __AWS_OTA_PAL_SETTINGS

#include "stdint.h"
#include "aws_iot_ota_agent.h"
#include "flash_if.h"
/* Number of bytes to represent an component (R,S) of the the curve. */
#define ECC_NUM_BYTES_PER_SIG_COMPONENT     ( 32U )

/* Number of components of signature. */
#define ECC_NUM_SIG_COMPONENTS              ( 2U )

/* Signature overall bytes */
#define ECC_SIGNATURE_BYTES                 ( ECC_NUM_BYTES_PER_SIG_COMPONENT * ECC_NUM_SIG_COMPONENTS )

#define ECC_PUBLIC_KEY_SIZE                 ( 64U )

#define otapalDESCRIPTOR_SIZE               ( sizeof(ImageDescriptor_t) )                      /* The size of the firmware descriptor */
#define otapalMAGICK_SIZE                   ( 7U )

const char pcOTA_PAL_Magick[ otapalMAGICK_SIZE ] = "@AFRTOS";

typedef enum
{
    otapalIMAGE_FLAG_NEW = 0xFF,            /* If the application image is running for the first time and never executed before. */
    otapalIMAGE_FLAG_COMMIT_PENDING = 0xFE, /* The application image is marked to execute for test boot. */
    otapalIMAGE_FLAG_VALID = 0xFC,          /*The application image is marked valid and committed. */
    otapalIMAGE_FLAG_INVALID = 0xF8         /*The application image is marked invalid. */
} ImageFlags_t;

typedef struct __attribute__((aligned(FLASH_IF_MIN_WRITE_LEN)))
{
    uint8_t pMagick[ otapalMAGICK_SIZE ];           /* 7 byte pattern used to identify if application image is present on the image slot in flash. */
    uint8_t usImageFlags;                           /* Image flags */
    uint32_t ulSequenceNumber;                      /* A 32 bit field in image descriptor and incremented after image is received and validated by OTA module */
    uint32_t ulStartAddress;                        /* Starting address of the application image */
    uint32_t ulEndAddress;                          /* End address of the application image */
    uint32_t ulExecutionAddress;                    /* The load/execution address of the application image */
    uint32_t ulHardwareID;                          /* 32 bit ID that can be generated unique for a particular platform */
    uint32_t ulSignatureSize;                       /* Size of the crypto signature being used */
    uint8_t pSignature[ kOTA_MaxSignatureSize ]; /* Crypto signature of the application image excluding the descriptor */
} ImageDescriptor_t;

#endif /*__AWS_OTA_PAL_SETTINGS */
