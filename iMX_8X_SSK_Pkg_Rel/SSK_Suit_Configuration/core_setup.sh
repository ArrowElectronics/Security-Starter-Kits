#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause


run_core_setup()
{

	echo "---> Installing Required Python Packages for Implementing Security Sectup on Board with TPM2.0 <---"
	echo " "
	echo "---> Process will take Time...Please Wait for 20mins <---"
	sleep 10	
	pip install --upgrade pip; pip install pyyaml==5.3.1 ; sleep 1 ; pip install cryptography==2.9.2 ; sleep 1 ; pip install paramiko==2.7.2 ;
	if [ $? == 1 ];then
		echo "---> Installation Fail !<---"
		echo "---> Please Check your intenet connectivity. <---"
		echo "---> If connectivity is Good please reboot board and re-run the script.. <---"
		exit 1
	fi
	sleep 3
	cd 
	git clone https://github.com/aws/aws-iot-device-sdk-python.git
	if [ $? == 1 ];then
		echo "--->Installation Fail ! <---"
		echo "---> Please Check your intenet connectivity. <---"
		echo "---> If connectivity is Good please reboot board and re-run the script.. <---"
		exit 1
	fi
	cd /home/root/aws-iot-device-sdk-python
	python setup.py install
	if [ $? == 1 ];then
		echo "---> Installation Fail ! <---"
		echo "---> Please Check your intenet connectivity. <---"
		echo "---> If connectivity is Good please reboot board and re-run the script.. <---"
		exit 1
	fi
	cd
	echo " "
	echo " "
	sleep 2
	echo "---> Installing Required TPM-PKCS Tool Packages for Implemetating Security Setup <---"
	sleep 3	
	git clone https://github.com/tpm2-software/tpm2-pkcs11
	if [ $? == 1 ];then
		echo "---> Installation Fail !  <---"
		echo "---> Please Check your intenet connectivity.  <---"
		echo "---> If connectivity is Good please reboot board and re-run the script..  <---"
		exit 1
	fi
	cd /home/root/tpm2-pkcs11/
	git checkout a82d0709c97c88cc2e457ba111b6f51f21c22260
	if [ $? == 1 ];then
		echo "---> Installation Fail ! <---"
		echo "---> Please Check your intenet connectivity. <---"
		echo " --->If connectivity is Good please reboot board and re-run the script.. <---"
		exit 1
	fi
	cd 
	echo " "
	echo "---> Packages Installation done <---"
	echo " "
	echo "---> Making Soft-Link <---"
	ln -sf /usr/lib/libtss2-tcti-tabrmd.so.0 /usr/lib/libtss2-tcti-tabrmd.so
	sleep 1
	ln -sf /usr/lib/engines/pkcs11.so /usr/lib/engines/libpkcs11.so
	sleep 1
}

run_core_setup
