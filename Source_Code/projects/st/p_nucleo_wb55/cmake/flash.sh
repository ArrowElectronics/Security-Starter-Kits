#!/bin/bash
rootDir=$1
nameProj=$2
flashUtilPath="/home/klisevich/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI"

cd "$rootDir/vendors/st/boards/p_nucleo_wb55/$nameProj/build"

FLASHFILE="SBSFU_$nameProj.bin"

#Erasing active and download areas
for i in {0..55}
do
 echo "Erasing sector #${i}"
 $flashUtilPath -c port=SWD freq=4000 ap=0 -e $i 1>/dev/null
done

# Flashing firmware
echo "Flashing firmware..."
$flashUtilPath -c port=SWD freq=4000 -w $FLASHFILE 0x08000000 1>/dev/null



