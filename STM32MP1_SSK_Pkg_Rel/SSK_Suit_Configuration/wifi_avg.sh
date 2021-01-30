#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause

cleanup()
{
	rm ./wpa_supplicant.conf
}
wifi_initialize()
{
	echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo "Wifi Connection Provisioning Board "
	echo  "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo " "
	killall wpa_supplicant
	ip addr flush dev wlan0
	ifconfig eth0 down
	ifconfig usb0 down
	ifconfig wlan0 up
	sleep 1
	echo " "
	cp /etc/wpa_supplicant.conf .
	a=0
	while [ $a -lt 3 ]
	do
		echo "---> [WIFI] List of available Wifi devices in Range... <---"
		iw dev wlan0 scan | egrep "SSID"
		echo " "
		echo "---> Can you see your wifi devices:SSID? y/n <---"
		read reply
		if [ $reply == "y" -o $reply == "Y" ];then
			break
		fi
		a=`expr $a + 1`
	done
	if [ $a -eq 3 ]; then
		echo "---> Please check your Hostpot is ON or Not <---"
		echo "---> Then Try Again <---"
		exit 1
	fi

	echo "---> Please Enter the Name of your Wifi-Device SSID <---"
	read device_name
	echo "---> Can you please Provide the Password of your Wifi-Device <---"
	read device_pass
	wpa_passphrase $device_name $device_pass >> ./wpa_supplicant.conf	
	if [ $? == 1 ]; then
		exit 1
	fi
	sleep 1
	wpa_supplicant -B -Dnl80211 -iwlan0 -c ./wpa_supplicant.conf      
	sleep 5
	echo "---> [WIFI] Connection Confirmation Wifi devices in Range... <---"
	iw dev wlan0 link
	sleep 1
	echo  "---> Connected to the network IP address Requesting <---"
	dhclient wlan0
	echo " "
	echo " "
	if [ $(echo $?) == 1 ]; then
		echo "---> Failure in Connecting to Network <---"
		echo "---> Please Re-initiate the Connection or Reboot the Board** <---"
		echo " "
	else 
		echo "---> Sucessfully Connected to Network <---"
		echo " "
	fi
	sleep 1
}

wifi_initialize
cleanup

echo "---> Done ....... Exiting Wifi Initialization... <---"
echo " "
