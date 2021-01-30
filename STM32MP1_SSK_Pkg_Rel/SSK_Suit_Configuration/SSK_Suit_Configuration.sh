#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause

run_command()
{
	echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo "Welcome to SECURITY-STARTER-KIT(SSK) Auto Flashing Tool"
	echo  "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo "-------------------------Validation--------------------------"
        echo "---> Security Chip Validation with SSK <---"
        tpm2_getrandom 4 > /dev/null 2>&1
        if [ $? != 0 ]; then
             echo "---> Security Chip Validation Failed!!! Exiting Setup <---"
             echo "---> Please Validate H/w Connection with Tresor2.0 Board is proper or not <---"
             exit 1
        fi
        echo "---> Security Chip Validation Sucessfull Proceeding Setup <---"
	echo " "
	echo "-------------------------Prerequisite--------------------------"
	echo " "
	echo "---> Please Enable Your Mobile Or Router Hotspot for internet connectivity <---"
	echo " "
	echo "---> Have you enable the hostpot? y/n <---"
	read reply
	case "$reply" in
		"y" | "Y")
			sleep 1	
			./wifi_avg.sh
			if [ $? == 1 ]; then
				exit 1
			fi
			sleep 1
			./core_setup.sh
			if [ $? == 1 ]; then
				exit 1
			fi
			sleep 1
			./flash_PCR_avnegre96.sh	
			if [ $? == 1 ]; then
				exit 1
			fi
			sleep 1
			tpm2_nvlist | egrep "0x1500100"
			if [ $? == 0 ]; then
				echo "---> EUI64 ID already Flashed in NV Area... <---"
			
			else
				python ./EUI_Flash.py	
				if [ $? == 1 ]; then
					exit 1
				fi
			fi
			sleep 1
			./flash_security_setup_Avenger.sh
			if [ $? == 1 ]; then
				exit 1
			fi
			sleep 1 
			;;
		"n" | "N")
			echo "---> Please Enable Your Mobile Or Router Hotspot for internet connectivity <---"
			echo "---> Exiting Setup.. <---"
			exit 1
			;;
		*)
			echo "---> Invalid Option <---"
			echo "---> Exiting Setup.. <---"
			exit 1
			;;
	esac

}	



if [ $# == 0 ]; then
	run_command
	adduser --system ggc_user
	addgroup --system ggc_group
elif [ $# == 1 ]; then
	if [ $1 == "tpm_clear" ]; then
		./clear_tpm.sh
		exit 1
	elif [ $1 == "setup_result" ]; then
		./test_result.sh
		exit 1
	else
		echo "Usage :"
		echo "SSK_Suit_Configuration.sh [Options]"
		echo "SSK_Suit_Configuration.sh tpm_clear"
		echo "SSK_Suit_Configuration.sh setup_result"
		exit 1
	fi
else
	echo "Usage :"
	echo "SSK_Suit_Configuration.sh [Options]"
	echo "SSK_Suit_Configuration.sh tpm_clear"
	echo "SSK_Suit_Configuration.sh setup_result"
	exit 1
fi
echo "Exiting Demo..."
