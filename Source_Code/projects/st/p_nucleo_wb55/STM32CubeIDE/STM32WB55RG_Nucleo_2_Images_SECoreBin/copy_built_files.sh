#!/bin/bash

dirCopyFrom=$1
dirCopyTo=$2

mkdir -p $dirCopyTo

echo "Copying SeCoreBin.bin and SeCoreBin.elf to common build directory."
cp $dirCopyFrom/SeCoreBin.elf $dirCopyTo/SeCoreBin.elf
cp $dirCopyFrom/SeCoreBin.bin $dirCopyTo/SeCoreBin.bin

