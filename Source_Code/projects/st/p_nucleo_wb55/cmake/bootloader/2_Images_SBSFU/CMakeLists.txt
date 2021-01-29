cmake_minimum_required(VERSION 3.13)

# build configurations
option (RELEASE_BUILD "release build option" OFF)
message ("RELEASE_BUILD: ${RELEASE_BUILD}")

# Find GNU C compiler for ARM.
find_program(SBSFU_COMPILER_CC arm-none-eabi-gcc PATHS $ENV{AFR_TOOLCHAIN_PATH} PATH_SUFFIXES bin NO_DEFAULT_PATH)

# Specify the cross compiler.
set(CMAKE_C_COMPILER ${SBSFU_COMPILER_CC})

# common compilation and linking flags
set (ARCH_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16")
set (CMAKE_C_FLAGS_INIT "${ARCH_FLAGS} -DSTM32WB55xx -DUSE_STM32WBXX_NUCLEO -DUSE_HAL_DRIVER -DCKS_ENABLED")
set (CMAKE_ASM_FLAGS_INIT "${ARCH_FLAGS} -g ")
set (CMAKE_EXE_LINKER_FLAGS_INIT "${ARCH_FLAGS} -specs=nano.specs -specs=nosys.specs -Xlinker -L${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/boards/p_nucleo_wb55/bootloader/Linker_Common -T${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/boards/p_nucleo_wb55/bootloader/2_Images_SBSFU/stm32wb55xx_flash_cm4.ld -lm -lnosys -Wl,--gc-sections -Wl,-Map=output.map")

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

set(CMAKE_SYSTEM_NAME Generic)
project (SBSFU C ASM)

# compiler bins location
get_filename_component(C_COMPILER_PATH ${CMAKE_C_COMPILER} DIRECTORY)
set (TARGET_NAME SBSFU)

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
set (ROOT_DIR  ${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/boards/p_nucleo_wb55/bootloader/2_Images_SBSFU)
set (APP_DIR   ${ROOT_DIR})
set (HAL_DIR   ${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/p_nucleo_wb55/STM32WBxx_HAL_Driver)
set (SE_DIR   ${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/STM32_Secure_Engine)
set (CMSIS_DIR  ${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/CMSIS)
set (SBSFU_DIR  ${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/boards/p_nucleo_wb55/bootloader/2_Images_SBSFU/SBSFU)
set (BSP_DIR    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../vendors/st/p_nucleo_wb55/BSP)

# include dirs
include_directories(	
    ${CMSIS_DIR}/Device/ST/STM32WBxx/Include
    ${HAL_DIR}/Inc
    ${BSP_DIR}/P-NUCLEO-WB55.Nucleo
    ${ROOT_DIR}/Core/Inc
    ${ROOT_DIR}/SBSFU/App
    ${ROOT_DIR}/SBSFU/Target
    ${ROOT_DIR}/../2_Images_SECoreBin/Inc
    ${ROOT_DIR}/../Linker_Common
    ${SE_DIR}/Core
    ${SE_DIR}/Interface
    ${CMSIS_DIR}/Include
)

# application source files
add_library(app_lib OBJECT
    ${ROOT_DIR}/Core/Src/main.c
    ${ROOT_DIR}/Core/Src/stm32wbxx_hal_msp.c
    ${ROOT_DIR}/Core/Src/stm32wbxx_it.c
    ${APP_DIR}/SE_CORE_Bin.c
    ${APP_DIR}/startup_stm32wb55xx_cm4.s
    ${APP_DIR}/syscalls.c
    ${CMSIS_DIR}/Device/ST/STM32WBxx/Source/Templates/system_stm32wbxx.c
    ${BSP_DIR}/P-NUCLEO-WB55.Nucleo/stm32wbxx_nucleo.c
)

# HAL source files
add_library(hal_lib OBJECT
    ${HAL_DIR}/Src/stm32wbxx_hal.c
    ${HAL_DIR}/Src/stm32wbxx_hal_adc.c
    ${HAL_DIR}/Src/stm32wbxx_hal_adc_ex.c
    ${HAL_DIR}/Src/stm32wbxx_hal_cortex.c
    ${HAL_DIR}/Src/stm32wbxx_hal_crc.c
    ${HAL_DIR}/Src/stm32wbxx_hal_crc_ex.c
    ${HAL_DIR}/Src/stm32wbxx_hal_dma.c
    ${HAL_DIR}/Src/stm32wbxx_hal_dma_ex.c
    ${HAL_DIR}/Src/stm32wbxx_hal_flash.c
    ${HAL_DIR}/Src/stm32wbxx_hal_flash_ex.c
    ${HAL_DIR}/Src/stm32wbxx_hal_gpio.c
    ${HAL_DIR}/Src/stm32wbxx_hal_i2c.c
    ${HAL_DIR}/Src/stm32wbxx_hal_i2c_ex.c
    ${HAL_DIR}/Src/stm32wbxx_hal_iwdg.c
    ${HAL_DIR}/Src/stm32wbxx_hal_pwr.c
    ${HAL_DIR}/Src/stm32wbxx_hal_pwr_ex.c
    ${HAL_DIR}/Src/stm32wbxx_hal_rcc.c
    ${HAL_DIR}/Src/stm32wbxx_hal_rcc_ex.c
    ${HAL_DIR}/Src/stm32wbxx_hal_rtc.c
    ${HAL_DIR}/Src/stm32wbxx_hal_rtc_ex.c
    ${HAL_DIR}/Src/stm32wbxx_hal_spi.c
    ${HAL_DIR}/Src/stm32wbxx_hal_spi_ex.c
    ${HAL_DIR}/Src/stm32wbxx_hal_tim.c
    ${HAL_DIR}/Src/stm32wbxx_hal_tim_ex.c
    ${HAL_DIR}/Src/stm32wbxx_hal_uart.c
    ${HAL_DIR}/Src/stm32wbxx_hal_uart_ex.c
)

# Secure Engine source files
add_library(se_lib OBJECT
    ${SE_DIR}/Core/se_interface_application.c
    ${SE_DIR}/Core/se_interface_bootloader.c
    ${SE_DIR}/Core/se_interface_common.c
)

# Secure Engine source files
add_library(sbsfu_lib OBJECT
    ${SBSFU_DIR}/App/sfu_boot.c
    ${SBSFU_DIR}/App/sfu_com_loader.c
    ${SBSFU_DIR}/App/sfu_com_trace.c
    ${SBSFU_DIR}/App/sfu_error.c
    ${SBSFU_DIR}/App/sfu_fwimg_core.c
    ${SBSFU_DIR}/App/sfu_fwimg_services.c
    ${SBSFU_DIR}/App/sfu_loader.c
    ${SBSFU_DIR}/App/sfu_new_image.c
    ${SBSFU_DIR}/App/sfu_test.c
    ${SBSFU_DIR}/Target/sfu_low_level.c
    ${SBSFU_DIR}/Target/sfu_low_level_flash.c
    ${SBSFU_DIR}/Target/sfu_low_level_security.c
)

# executable
add_executable(${TARGET_NAME}
    $<TARGET_OBJECTS:app_lib>
    $<TARGET_OBJECTS:hal_lib>
    $<TARGET_OBJECTS:se_lib>
    $<TARGET_OBJECTS:sbsfu_lib>
)

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# post build commands
add_custom_command(TARGET ${TARGET_NAME}
    COMMAND ${C_COMPILER_PATH}/arm-none-eabi-size ${TARGET_NAME}.elf
    COMMENT "--- size of sections ---"
)

add_custom_command(TARGET ${TARGET_NAME}
    COMMAND ${C_COMPILER_PATH}/arm-none-eabi-objcopy -O binary -S ${TARGET_NAME}.elf ${TARGET_NAME}.bin
    COMMENT "--- elf to bin ---"
)
