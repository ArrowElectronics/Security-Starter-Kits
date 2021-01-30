#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause

clear_tpm()
{
	rm /opt/tpm2-pkcs11/tpm2_pkcs11.sqlite3
	rm /greengrass/certs/deviceCert.csr
	rm /greengrass/certs/aws_device_cert.pem
	echo "---> TPM2.0 Cleared with Tokens and Labels <---"
	tpm2_nvlist | egrep "0x1500016"
	if [ $? == 1 ]; then
		echo "TPM2.0 Tokens and Labels Are Already cleared"
		exit 1
	fi
	tpm2_nvrelease -x 0x1500016 -a 0x40000001
	tpm2_nvrelease -x 0x1500100 -a 0x40000001
	tpm2_clear -Q
}

clear_tpm
echo "---> Clearing Done Exiting <---"
