#!/bin/bash
#Post build for SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256
# arg1 is the project root directory
# arg2 is the user application folder
# arg3 is the elf file path+name
# arg4 is the bin file path+name
# arg5 is the version
# arg6 when present forces "bigelf" generation

projectdir=$1
FileName=${4##*/}
execname=${FileName%.*}
userAppFolder=$2
elf=$3
bin=$4
version=$5

SecureEngine=${0%/*}

userAppBinary="${userAppFolder}"

sfb=$userAppBinary"/"$execname".sfb"
sign=$userAppBinary"/"$execname".sign"
headerbin=$userAppBinary"/"$execname"_sfuh.bin"
bigbinary=$userAppBinary"/SBSFU_"$execname".bin"
ecckey=$SecureEngine"/Binary/ECCKEY.txt"
current_directory=`pwd`
cd  $SecureEngine"/../../"
SecureDir=`pwd`
cd $current_directory
sbsfuelf="${projectdir}/build/SBSFU.elf"

current_directory=`pwd`
cd "${projectdir}/vendors/st/STM32_Secure_Engine/Utilities/KeysAndImages"
basedir=`pwd`
cd $current_directory
# test if window executeable useable
prepareimage=$basedir"/win/prepareimage/prepareimage.exe"
uname  | grep -i -e windows -e mingw > /dev/null 2>&1
if [ $? == 0 ] && [  -e "$prepareimage" ]; then
  echo "prepareimage with windows executeable"
  export PATH=$basedir"\win\prepareimage";$PATH > /dev/null 2>&1
  cmd=""
  prepareimage="prepareimage.exe"
else
  # line for python
  echo "prepareimage with python script"
  prepareimage=$basedir/prepareimage.py
  cmd="python"
fi

echo "$cmd $prepareimage" >> $userAppBinary/output.txt
# Make sure we have a Binary sub-folder in UserApp folder
if [ ! -e $userAppBinary ]; then
mkdir $userAppBinary
fi



command=$cmd" "$prepareimage" sha256 "$bin" "$sign
$command >> $userAppBinary"/output.txt"
ret=$?
if [ $ret == 0 ]; then 
  command=$cmd" "$prepareimage" pack  -k "$ecckey" -p 1 -r 44 -v "$version" -f "$bin" -t "$sign" "$sfb" -o 512" 
  $command >> $userAppBinary"/output.txt"
  ret=$?
  if [ $ret == 0 ]; then
    command=$cmd" "$prepareimage" header -k  "$ecckey"  -p 1 -r 44 -v "$version"  -f "$bin" -t "$sign" -o 512 "$headerbin
    $command >> $userAppBinary"/output.txt"
    ret=$?
    if [ $ret == 0 ]; then
      command=$cmd" "$prepareimage" merge -v 0 -e 1 -i "$headerbin" -s "$sbsfuelf" "$elf" "$bigbinary
      $command >> $userAppBinary"/output.txt"
      ret=$?
      if [ $ret == 0 ] && [ $# == 6 ]; then
        echo "Generating the global elf file (SBSFU and userApp)"
        echo "Generating the global elf file (SBSFU and userApp)" >> $userAppBinary"/output.txt"
        uname | grep -i -e windows -e mingw > /dev/null 2>&1
        if [ $? == 0 ]; then
          # Set to the default installation path of the Cube Programmer tool
          # If you installed it in another location, please update PATH.
          PATH="C:\\Program Files\\STMicroelectronics\\STM32Cube\\STM32CubeProgrammer\\bin";$PATH > /dev/null 2>&1
          programmertool="STM32_Programmer_CLI.exe"
        else
          echo "fix access path to STM32_Programmer_CLI.exe "
        fi
        command=$programmertool" -ms "$elf" "$headerbin" "$sbsfuelf
        $command >> $userAppBinary"/output.txt"
        ret=$?
      fi
    fi
  fi
fi

if [ $ret == 0 ]; then
  rm $sign
  rm $headerbin
  # Replace original file with bootable one.
  # rm $execname".bin"
  # mv $bigbinary $execname".bin"

  exit 0
else 
  echo "$command : failed" >> $userAppBinary"/output.txt"
  if [ -e  "$elf" ]; then
    rm  $elf
  fi
  if [ -e "$elfbackup" ]; then 
    rm  $elfbackup
  fi
  echo $command : failed
  read -n 1 -s
  exit 1
fi
