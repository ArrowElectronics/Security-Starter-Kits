#!/bin/bash
# AFR project root dir
projectDir=$1
bootloaderDirPath=${projectDir}/projects/st/p_nucleo_wb55/cmake/bootloader
# Make SeCoreBin project
cd ${bootloaderDirPath}/2_Images_SECoreBin
mkdir -p build
cd build
cmake -G "Unix Makefiles" .. && make
# Copy output files to AFR build directory
cp SeCoreBin.bin ./../../../../../../../build
cp SeCoreBin.elf ./../../../../../../../build
# Make SBSFU project
cd ${bootloaderDirPath}/2_Images_SBSFU
mkdir -p build
cd build
cmake -G "Unix Makefiles" .. && make
# Copy output files to AFR build directory
cp SBSFU.bin ./../../../../../../../build
cp SBSFU.elf ./../../../../../../../build