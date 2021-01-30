#!/bin/bash

# this function is called when Ctrl-C is sent


function trap_ctrlc ()
{
	# perform cleanup here
	echo "---> Ctrl-C caught...performing clean up <---"
	# perform cleanup here
	/greengrass/ggc/core/greengrassd stop 
	cd ${SCRIPT_PATH}/

	kill -9 $version_pid $switch_pid $traffic_pid  

	#kill $gatt_pid $pub_pid $sub_pid

	# exit shell script with error code 2
	# if omitted, shell script will continue execution
	exit 2
}
if [ $# == 0 ]
then
	echo "---> Please enter config file <---"
	echo "./Gateway_Demo_avg.sh Demo.config "
	exit
fi

#configuration file
filename="$1"
conf_parser()
{
	endpoint=$(awk '/^endpoint/{print $3}' $filename)
	echo "endpoint=$endpoint"

	switch_cert=$(awk '/^switch_cert/{print $3}' $filename)
	echo "switch_cert=$switch_cert"

	switch_key=$(awk '/^switch_key/{print $3}' $filename)
	echo "switch_key=$switch_key"

	traffic_cert=$(awk '/^traffic_cert/{print $3}' $filename)
	echo "traffic_cert=$traffic_cert"

	traffic_key=$(awk '/^traffic_key/{print $3}' $filename)
	echo "traffic_key=$traffic_key"

	rootca=$(awk '/^rootca/{print $3}' $filename)
	echo "rootca=$rootca"

	GG_switch=$(awk '/^GG_switch/{print $3}' $filename)
	echo "GG_switch=$GG_switch"

	GG_traffic=$(awk '/^GG_traffic/{print $3}' $filename)
	echo "GG_traffic=$GG_traffic"
}

conf_parser

SCRIPT_PATH="$( cd "$(dirname "$0")" ; pwd -P )"

#Actual Demo running in background
run_greengrass_demo()
{


	echo "VERSION-Gateway Connectivity publishing <---"
	python Version_Avenger.py --endpoint "$endpoint" --rootCA "$rootca" --cert "$switch_cert" --key "$switch_key" --mode both &
	version_pid="$!"
	echo "version_pid=$version_pid"
	sleep 1
	echo "SWITCH Press"
	python lightController.py --endpoint "$endpoint" --rootCA "$rootca" --cert "$switch_cert" --key "$switch_key" --thingName "$GG_traffic" --clientId "$GG_switch" &
	switch_pid="$!"
	echo "switch_pid=$switch_pid"

	echo "Tarffic light status"
	python trafficLight.py --endpoint "$endpoint" --rootCA "$rootca" --cert "$traffic_cert" --key "$traffic_key" --thingName "$GG_traffic" --clientId "$GG_traffic" &
	traffic_pid="$!"
	echo "traffic_pid=$traffic_pid"

}


run_demos()
{
	echo "Hello, $(whoami)!"	
	echo "########  Welcome to STM32MP1 SSK Security Demos [] ##########"
	echo "---> Prerequisite: Have you run SSK_Suit_Configuration Script before running This <---?"
	echo "---> Press: (y/n) <---"
	echo ""
	read REPLY
	case "$REPLY" in
		y|Y)
			/greengrass/ggc/core/greengrassd start 
			if [ $? == 1 ]; then
				echo "---> TPM2.0 Tokens Failure--Shutting Down Greengras Demo <---"
				exit 1
			fi
			sleep 2
			run_greengrass_demo
			;;

		n|N)
			echo "---> Please Run Production Flashing Script before above  procees <---"
			;;

		*)
			echo "Select correct option: (y/n)"
			;;
	esac
}



# initialise trap to call trap_ctrlc function
# when signal 2 (SIGINT) is received
trap "trap_ctrlc" 2
run_demos
while true
do
	sleep 1
done

echo "Exiting Demo..."

