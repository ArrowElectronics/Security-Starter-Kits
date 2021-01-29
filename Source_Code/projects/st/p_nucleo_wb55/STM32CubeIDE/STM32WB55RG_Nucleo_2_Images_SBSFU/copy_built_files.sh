#!/bin/bash

dirCopyFrom=$1
dirCopyTo=$2

mkdir -p $dirCopyTo

echo "Copying SBSFU.bin and SBSFU.elf to common build directory."
cp $dirCopyFrom/SBSFU.elf $dirCopyTo/SBSFU.elf
cp $dirCopyFrom/SBSFU.bin $dirCopyTo/SBSFU.bin

