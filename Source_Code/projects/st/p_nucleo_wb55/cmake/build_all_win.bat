echo off
REM Converting Windows path slash to Linux slash
set "arg1=%1"
set cd=%~dp0
set "cd=%cd:\=/%"
"%BUILD_TOOLS_PATH%/sh.exe" -c "%cd%/build_all.sh %arg1%"
