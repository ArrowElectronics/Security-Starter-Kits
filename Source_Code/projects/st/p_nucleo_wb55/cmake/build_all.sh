#!/bin/bash
if ! [[ "$1" == "tests" || "$1" == "demos" ]];then
    echo "Enter demos or tests param"
	exit 1
fi
#build AFR
config=$1
currentPath=$(pwd)
afrCmakePath=${currentPath}/../../../../
afrCmakeSrcPath=${afrCmakePath}
afrCmakeBuildPath=${afrCmakePath}/build
afrCmakeParams="-DAFR_ENABLE_DEMOS=ON -DAFR_ENABLE_TESTS=OFF"
if [ ${config} = "tests" ]
then
   afrCmakeParams="-DAFR_ENABLE_DEMOS=OFF -DAFR_ENABLE_TESTS=ON"
fi
cmake -DVENDOR=st -DBOARD=p_nucleo_wb55 -DCOMPILER=arm-gcc -S ${afrCmakeSrcPath} -B ${afrCmakeBuildPath} -G "Unix Makefiles" ${afrCmakeParams}
cd ${afrCmakeBuildPath}
make -C ${afrCmakeBuildPath}

#build Bootloader
cd ${currentPath}/bootloader
${currentPath}/bootloader/bootloader_build.sh ${afrCmakePath}

#create Firmware
version=1
projectDir=${afrCmakePath}
bootloaderDirPath=${projectDir}/vendors/st/boards/p_nucleo_wb55/bootloader
elfFilePath=${afrCmakeBuildPath}/aws_demos.elf
binFilePath=${afrCmakeBuildPath}/aws_demos.bin
if [ ${config} = "tests" ]
then
	elfFilePath=${afrCmakeBuildPath}/aws_tests.elf
	binFilePath=${afrCmakeBuildPath}/aws_tests.bin
fi
postBuildScriptPath=${bootloaderDirPath}/2_Images_SECoreBin/postbuild.sh
${postBuildScriptPath} ${projectDir} ${projectDir}/build ${elfFilePath} ${binFilePath} ${version}