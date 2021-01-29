#!/bin/bash

version=1
projectDir=$1
bootloaderDirPath=${projectDir}/vendors/st/boards/p_nucleo_wb55/bootloader
elfFilePath=$2
binFilePath=$3
userAppDir=$4

if [[ -z ${userAppDir} ]]
 then
  userAppDir=${projectDir}
fi

postBuildScriptPath=${bootloaderDirPath}/2_Images_SECoreBin/postbuild.sh
${postBuildScriptPath} ${projectDir} ${userAppDir} ${elfFilePath} ${binFilePath} ${version}

