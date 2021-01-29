echo off
REM Converting Windows path slash to Linux slash
set "projDir=%1"
set "binaryDir=%2"
set "elfFilePath=%3"
set "binFilePath=%4"
set "projDir=%projDir:\=/%"
set "binaryDir=%binaryDir:\=/%"
set "elfFilePath=%elfFilePath:\=/%"
set "binFilePath=%binFilePath:\=/%"
"%BUILD_TOOLS_PATH%\sh.exe" -c "%projDir%/vendors/st/boards/p_nucleo_wb55/image_build.sh %projDir% %elfFilePath% %binFilePath% %binaryDir%"

