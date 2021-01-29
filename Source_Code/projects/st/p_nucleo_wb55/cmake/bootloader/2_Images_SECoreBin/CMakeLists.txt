cmake_minimum_required(VERSION 3.13)

# build configurations
option (RELEASE_BUILD "release build option" OFF)
message ("RELEASE_BUILD: ${RELEASE_BUILD}")

# Find GNU C compiler for ARM.
find_program(SECOREBIN_COMPILER_CC arm-none-eabi-gcc PATHS $ENV{AFR_TOOLCHAIN_PATH} PATH_SUFFIXES bin NO_DEFAULT_PATH)

# Specify the cross compiler.
set(CMAKE_C_COMPILER ${SECOREBIN_COMPILER_CC})

# common compilation and linking flags
set (ARCH_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16")
set (CMAKE_C_FLAGS_INIT "${ARCH_FLAGS} -DSTM32WB55xx -DUSE_STM32WBXX_NUCLEO -DUSE_HAL_DRIVER -DCKS_ENABLED")
set (CMAKE_ASM_FLAGS_INIT "${ARCH_FLAGS} -g ")
set (CMAKE_EXE_LINKER_FLAGS_INIT "${ARCH_FLAGS} -specs=nano.specs -specs=nosys.specs -Xlinker -L${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/boards/p_nucleo_wb55/bootloader/Linker_Common -T${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/boards/p_nucleo_wb55/bootloader/2_Images_SECoreBin/stm32wb55xx_flash_cm4.ld -lm -lnosys -Wl,--gc-sections -Wl,-Map=output.map")

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

set(CMAKE_SYSTEM_NAME Generic)
project (SeCoreBin C ASM)

# compiler bins location
get_filename_component(C_COMPILER_PATH ${CMAKE_C_COMPILER} DIRECTORY)
set (TARGET_NAME SeCoreBin)

# additional compilation flags
if (RELEASE_BUILD)
    add_compile_options(-Os -g3)
    set (TARGET_NAME ${TARGET_NAME})
else ()
    add_compile_options(-Os -g3)
    set (TARGET_NAME ${TARGET_NAME})
endif (RELEASE_BUILD)
add_compile_options(-Wall -fmessage-length=0  -Wno-strict-aliasing -ffunction-sections -c -fmessage-length=0)
add_compile_options(-ffunction-sections -fdata-sections -specs=nano.specs -specs=nosys.specs)

# source dirs
set (ROOT_DIR  ${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/boards/p_nucleo_wb55/bootloader/2_Images_SECoreBin)
set (APP_DIR   ${ROOT_DIR})
set (HAL_DIR   ${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/p_nucleo_wb55/STM32WBxx_HAL_Driver)
set (WPAN_DIR  ${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/STM32_WPAN)
set (SE_DIR    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/STM32_Secure_Engine)
set (BUILD_DIR ${ROOT_DIR}/CMake/build)
set (CRYPTO_DIR ${WPAN_DIR}/ble/cryptographic)
set (VENDOR_ST_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st)

# Run prebuild script
if (CMAKE_HOST_UNIX)
	message("OS LINUX")
	execute_process(COMMAND sh -c "${APP_DIR}/prebuild.sh ${APP_DIR}")
elseif (CMAKE_HOST_WIN32)
	message("OS WIN32")
    if(NOT EXISTS "$ENV{BUILD_TOOLS_PATH}")
    	message(FATAL_ERROR "BUILD_TOOLS_PATH environment variable is not defined")
    endif(NOT EXISTS "$ENV{BUILD_TOOLS_PATH}")
    execute_process(COMMAND "$ENV{BUILD_TOOLS_PATH}/sh" -c "${APP_DIR}/prebuild.sh ${APP_DIR}")
else ()
	message(STATUS "OS OTHER")
endif ()

# include dirs
include_directories(	
	${ROOT_DIR}/Inc
        ${VENDOR_ST_DIR}/CMSIS/Device/ST/STM32WBxx/Include
        ${VENDOR_ST_DIR}/p_nucleo_wb55/BSP/P-NUCLEO-WB55.Nucleo
        ${HAL_DIR}/Inc
        ${VENDOR_ST_DIR}/STM32_Secure_Engine/Core
        ${VENDOR_ST_DIR}/STM32_Secure_Engine/Key
        ${ROOT_DIR}/../2_Images_SBSFU/SBSFU/App
        ${ROOT_DIR}/../Linker_Common
        ${VENDOR_ST_DIR}/STM32_WPAN/ble/cryptographic/Inc
        ${VENDOR_ST_DIR}/CMSIS/Include
        ${VENDOR_ST_DIR}/STM32_WPAN
        ${VENDOR_ST_DIR}/STM32_WPAN/interface/patterns/ble_thread/tl
        ${VENDOR_ST_DIR}/STM32_WPAN/interface/patterns/ble_thread
        ${VENDOR_ST_DIR}/STM32_WPAN/interface/patterns/ble_thread/shci
        ${VENDOR_ST_DIR}/STM32_WPAN/utilities
        ${CRYPTO_DIR}/Inc
)

# application source files
add_library(app_lib OBJECT
	${APP_DIR}/data_init.c
	${APP_DIR}/se_key.s
	${APP_DIR}/startup_stm32wb55xx_cm4.s
	${ROOT_DIR}/Src/hw_ipcc.c
	${ROOT_DIR}/Src/se_cks.c
	${ROOT_DIR}/Src/se_crypto_bootloader.c
	${ROOT_DIR}/Src/se_low_level.c
	${ROOT_DIR}/Src/stm32wbxx_hal_msp.c
	${ROOT_DIR}/Src/stm32wbxx_it.c
)

# HAL source files
add_library(hal_lib OBJECT
	${HAL_DIR}/Src/stm32wbxx_hal_cortex.c
	${HAL_DIR}/Src/stm32wbxx_hal_crc.c
	${HAL_DIR}/Src/stm32wbxx_hal_crc_ex.c
	${HAL_DIR}/Src/stm32wbxx_hal_cryp.c
	${HAL_DIR}/Src/stm32wbxx_hal_cryp_ex.c
	${HAL_DIR}/Src/stm32wbxx_hal_flash.c
	${HAL_DIR}/Src/stm32wbxx_hal_flash_ex
	${HAL_DIR}/Src/stm32wbxx_hal_pka.c
)

# WPAN source files
add_library(wpan_lib OBJECT
    ${WPAN_DIR}/interface/patterns/ble_thread/shci/shci.c
    ${WPAN_DIR}/interface/patterns/ble_thread/tl/shci_tl.c
    ${WPAN_DIR}/utilities/stm_list.c
    ${WPAN_DIR}/interface/patterns/ble_thread/tl/tl_mbox.c
)

# Secure Engine source files
add_library(se_lib OBJECT
    ${SE_DIR}/Core/se_bootinfo.c
    ${SE_DIR}/Core/se_callgate.c
    ${SE_DIR}/Core/se_crypto_common.c
    ${SE_DIR}/Core/se_exception.c
    ${SE_DIR}/Core/se_fwimg.c
    ${SE_DIR}/Core/se_startup.c
    ${SE_DIR}/Core/se_user_application.c
    ${SE_DIR}/Core/se_utils.c
)

# executable
add_executable(${TARGET_NAME}
    $<TARGET_OBJECTS:app_lib>
    $<TARGET_OBJECTS:hal_lib>
    $<TARGET_OBJECTS:wpan_lib>
    $<TARGET_OBJECTS:se_lib>
)

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

target_link_libraries(
    ${TARGET_NAME}
    PRIVATE
        "${WPAN_DIR}/ble/cryptographic/Lib/libSTM32CryptographicV3.0.0_CM4_GCC.a"
)

# post build commands
add_custom_command(TARGET ${TARGET_NAME}
    COMMAND ${C_COMPILER_PATH}/arm-none-eabi-size ${TARGET_NAME}.elf
    COMMENT "--- size of sections ---"
)

add_custom_command(TARGET ${TARGET_NAME}
    COMMAND ${C_COMPILER_PATH}/arm-none-eabi-objcopy -O binary -S ${TARGET_NAME}.elf ${TARGET_NAME}.bin
    COMMENT "--- elf to bin ---"
)
