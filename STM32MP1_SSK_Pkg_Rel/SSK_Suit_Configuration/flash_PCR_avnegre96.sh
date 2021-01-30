#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause

cleanup() {
	rm -f pcr_kernel_original.bin policy_pcr_kernel_original.out 
	tpm2_pcrreset 16

}
#trap cleanup EXIT

run_flash()
{
	echo "--> Creating sha256 of kernel Image <---"
	kernel_hash=$(sha256sum /boot/uImage | cut -d' ' -f1)
	echo " "
	echo "--> Printing Kernel Hash <---"
	echo $kernel_hash
	echo " "
	echo "--> Extending kernel Image to PCR BANK-16 <---"
	tpm2_pcrextend 16:sha256=$kernel_hash
	sleep 2
	echo " "
	echo "--> Measuring PCR BANK-16 Content <---"
	tpm2_pcrlist -L sha256:16 -o pcr_kernel_original.bin
	sleep 2
	echo " "
	echo "--> Creating PCR Policy for Kernel Image <---"
	tpm2_createpolicy --policy-pcr -L sha256:16 -F pcr_kernel_original.bin -o policy_pcr_kernel_original.out
	sleep 2 
	echo " "
	tpm2_nvlist | egrep "0x1500016"
	if [ $? == 0 ]; then
		echo "--> NV Address and Kernel hash Are Already defined <---"
		exit 0
	fi
	echo "--> Defining NV-RAM SPACE PCR Policy <---"
	tpm2_nvdefine -x 0x1500016 -a 0x40000001 -s 32  -L policy_pcr_kernel_original.out -b "policyread|policywrite|authread|authwrite|ownerwrite|ownerread"
	sleep 1
	echo " "
	echo "--> Extending PCR value to Secure NV RAM <---"
	tpm2_nvwrite -x 0x1500016 -a 0x1500016 -P pcr:sha256:16 pcr_kernel_original.bin
	echo " "
}    

run_flash
cleanup

echo "--> Exiting PCR Flashing with Measure of Kernel Image... <---"
