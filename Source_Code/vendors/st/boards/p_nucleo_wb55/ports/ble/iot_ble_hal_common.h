#include <string.h>
#include <stdint.h>

#include <ble.h>
#include <ble_common.h>

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/

/**
 * @anchor secure_connections_support
 * @name Secure connection support option code
 * Error codes in @ref aci_gap_set_authentication_requirement API
 */
#define SC_IS_NOT_SUPPORTED (0x00) /* Not supported */
#define SC_IS_SUPPORTED     (0x01) /* Supported but optional (i.e. a Legacy Pairing may be accepted) */
#define SC_IS_MANDATORY     (0x02) /* Supported but mandatory (i.e. do not accept Legacy Pairing but only Secure Connections v.4.2 Pairing) */

/**
 * @anchor secure_connections_support
 * @name Secure connection key press notification option code
 * Error codes in @ref aci_gap_set_authentication_requirement API
 */
#define KEYPRESS_IS_NOT_SUPPORTED (0x00)
#define KEYPRESS_IS_SUPPORTED     (0x01)


// Bluetooth device and local name
typedef struct {
	char * local_name;
	uint8_t local_name_size;
} LocalName_t;
