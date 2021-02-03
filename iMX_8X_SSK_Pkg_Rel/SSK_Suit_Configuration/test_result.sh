#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause

tpm_test()
{
	echo " "
	echo "---> Running TPM Self-Test <---"
	tpm2_selftest --fulltest
	echo " "
	echo "---> Checking TPM Self-Test Result <---"
	tpm2_gettestresult
	echo " "
	echo "---> Verifying Token Handle Created or Not <---"
	tpm2_listpersistent
	echo " "
	echo "---> Verifying Token Module Created with Provide Lable or Not <---"
	p11-kit list-modules
	echo " "
	echo "---> Validating TPM2.0 Security Keys are Loaded inside the Protected/Shielded Area <---"
	echo " "
	echo "---> Please Provide User-Pin Below(Provided in guide) <---"
	p11tool --list-privkeys pkcs11:manufacturer=Infineon
	echo " "
}
tpm_test

echo "---> Done Exiting TPM-Full Test after Production Flash Process <---"
	
