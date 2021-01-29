#!/bin/bash

export AFR_TOOLCHAIN_PATH="/opt/gcc-arm/bin"

SCRIPTDIR=$(dirname "$(readlink -fn "$0")")

cd "$SCRIPTDIR/../../../../"

CURRENTDIR=$PWD
enableTests=$2
imageBuildPath="$CURRENTDIR/vendors/st/boards/p_nucleo_wb55/image_build.sh"
boardDir="/vendors/st/boards/p_nucleo_wb55"

if [[ $enableTests -eq 1 ]]
then
    nameProj=aws_tests
else
    nameProj=aws_demos
fi  

buildDir="${CURRENTDIR}${boardDir}/${nameProj}/build"
userAppDir="${CURRENTDIR}${boardDir}/${nameProj}"

rm -r "$buildDir"

echo "Running CMake..."
cmake -DVENDOR=st -DBOARD=p_nucleo_wb55 -DCOMPILER=arm-gcc -DAFR_ENABLE_TESTS=$enableTests -S . -B $buildDir 1>/dev/null

cd $buildDir
echo "Building binaries..."
make all -j4 1>/dev/null

$imageBuildPath "${CURRENTDIR}" "./$nameProj.elf" "./$nameProj.bin" "${userAppDir}"
