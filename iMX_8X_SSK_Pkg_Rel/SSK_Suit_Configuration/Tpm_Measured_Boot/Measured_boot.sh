#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Source function library.
. /etc/init.d/functions
cleanup() {
	rm -f pcr_kernel_original.bin pcr_kernel_measured.bin
	tpm2_pcrreset 16

}
trap cleanup EXIT
Path_Kernel_Image=/run/media/mmcblk1p1/Image
run_measure()
{
	echo "---> Security Chip Connection Validation <---"                          
  	ls /dev/tpm0 -lrt                                                             
  	if [ $? != 0 ]; then                                                          
        	echo "---> Security Chip Validation Failed Measure Boot Check!!! <---"
        	echo "---> Please Validate H/w Connection with Tresor2.0 Board is proper or not <---"
        	exit 1                                                                
  	fi       
  	echo "---> Security Chip Validation Sucessfull Proceeding Measure Boot Validation <---"
	echo "Creating sha256 of kernel Image"
	kernel_hash=$(sha256sum $Path_Kernel_Image | cut -d' ' -f1)
	echo "Printing Kernel Hash------"
	echo $kernel_hash
	echo "Extending kernel Image to PCR BANK-16"
	tpm2_pcrextend 16:sha256=$kernel_hash
	sleep 2
	echo "Extending kernel Image to PCR BANK-16"
	tpm2_pcrlist -L sha256:16 -o pcr_kernel_measured.bin
	sleep 2
	tpm2_nvread -x 0x1500016 -a 0x1500016 -s 32 >> pcr_kernel_original.bin
	sleep 2

	echo "Printing Status of Measured Boot Success"
	cmp -s pcr_kernel_measured.bin pcr_kernel_original.bin
	if [ $? == 1 ]; then
		echo "Failure at **Kernel Platform Integrity Check  Need to Replace Original Kernel**"
		exit 1
	else 
		echo "Sucessfully Authenticated Measured boot"
		exit 1
	fi
}    

run_measure
while true
do
	sleep 1
done

echo "Exiting Demo..."
