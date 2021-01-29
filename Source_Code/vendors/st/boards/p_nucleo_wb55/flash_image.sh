#!/bin/bash
flashUtilRoot=$1
flashImagePath=$2

if [ ! "${flashImagePath}" ] || [ ! -f "${flashImagePath}" ]; then
	echo "Provide image file path!"
	exit -1
fi

flashUtilPath=${flashUtilRoot}"/STM32_Programmer_CLI"

# Flashing firmware
echo "Flashing firmware..."
$flashUtilPath -c port=SWD freq=4000 -w "$flashImagePath" 0x08000000 1>/dev/null
if [ ! $? == 0 ]; then
	echo "Failed."
	exit -1
else
	echo "Done."
fi
