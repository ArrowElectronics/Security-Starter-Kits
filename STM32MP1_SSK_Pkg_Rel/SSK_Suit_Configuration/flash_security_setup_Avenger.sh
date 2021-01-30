#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause

run_avenger_setup()
{
	cd /home/root/tpm2-pkcs11/tools
	echo "---> Initializing the keystore in TRUSTED-Chip TPM2.0 <---"
	echo " "
	p11-kit list-modules | egrep "greengrass"
	if [ $? == 0 ]; then
		echo "---> TPM2.0 Device Cerificate Are Already Existed <---"
		echo "---> If Want To Clear And Regenrate New Device Certificate <---"
		echo "---> Run ./SSK_Suit_Configuration.sh tpm_clear <---"
		echo " "
		exit 0
	fi
	./tpm2_ptool.py init --pobj-pin=1234 --path=/opt/tpm2-pkcs11/
	echo "---> Initializing the token in TRUSTED-Chip TPM2.0<---"
	echo " "
	sleep 2
	echo " "
	./tpm2_ptool.py addtoken --pid=1 --pobj-pin=1234 --sopin=1234 --userpin=1234 --label=greengrass --path=/opt/tpm2-pkcs11/
	echo "---> Adding a key in TRUSTED-Chip TPM2.0 <---"
	echo " "
	sleep 2 
	./tpm2_ptool.py addkey --algorithm=rsa2048 --label=greengrass --userpin=1234 --key-label=greenkey --path=/opt/tpm2-pkcs11/ 
	sleep 2
	cd 
	echo " "
	echo "---> Checking a Token Initialized Sucessfully in TRUSTED-Chip TPM2.0 or not <---"
	p11-kit list-modules
	sleep 5
	echo "---> Generate a certificate signing request in TRUSTED-Chip TPM2.0 <---"
	echo " "
	openssl req -engine pkcs11 -new -key "pkcs11:model=SLB9670;manufacturer=Infineon;token=greengrass;object=greenkey;type=private;pin-value=1234" -keyform engine -out /greengrass/certs/deviceCert.csr -subj /C="IN"/ST="GUJ"/L="AHM"/O="Arrow"/OU="eic"/CN="seed"
	echo " "
	echo "---> Create Device Certificates Signed By RootCA <---" 
	sleep 3
	openssl x509 -req -in /greengrass/certs/deviceCert.csr -CA /greengrass/certs/rootCA.pem -CAkey /greengrass/certs/rootCA.key -CAcreateserial -out /greengrass/certs/aws_device_cert.pem  -days 7000 -sha256
	if [ $? == 1 ]; then
		echo " "
		echo "---> Failure in Signature Check <---"
		echo "---> Please Check ROOTCA Authority** <---"
	else
		echo " "
		echo "---> TPM2.0-Device Cerificate Created Sucessfully <---"
	fi
}

run_avenger_setup

echo "---> Board is ready to Use for Demo <---"
