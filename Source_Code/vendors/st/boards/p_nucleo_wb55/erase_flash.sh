#!/bin/bash
flashUtilRoot=$1
startSector=$2
endSector=$3
flashUtilPath=${flashUtilRoot}"/STM32_Programmer_CLI"

if [ ! "$startSector" ] || [ ! "$endSector" ]; then
	echo "Bad sectors parameters!"
	exit -1
	
fi 

if [ "$startSector" \> "$endSector" ]; then 
	echo "Start sector $startSector is greater than start $endSector sector!"
	exit -1
fi

#Checking connection
$flashUtilPath -c port=SWD freq=4000 ap=0 1>/dev/null
if [ ! $? == 0 ]; then
	echo "Failed."
	exit -1
fi

#Erasing active and download areas
for i in `seq "$startSector" "$endSector"`;
do
 echo "Erasing sector #${i}"
 $flashUtilPath -c port=SWD freq=4000 ap=0 -e $i 1>/dev/null
done

echo "Done."
