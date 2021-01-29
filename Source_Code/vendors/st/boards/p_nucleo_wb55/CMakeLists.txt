set(p_nucleo_wb55_dir "${AFR_VENDORS_DIR}/st/p_nucleo_wb55")
set(p_nucleo_wb55_iap_dir "${AFR_VENDORS_DIR}/st/p_nucleo_wb55/STM_IAP")
set(p_nucleo_wb55_bootloader_dir "${AFR_VENDORS_DIR}/st/boards/p_nucleo_wb55/bootloader")
set(p_nucleo_wb55_sbsfu_dir "${p_nucleo_wb55_bootloader_dir}/2_Images_SBSFU/SBSFU")
set(p_nucleo_wb55_secore_dir "${p_nucleo_wb55_bootloader_dir}/2_Images_SECoreBin")
set(p_nucleo_wb55_ld_dir "${p_nucleo_wb55_bootloader_dir}/Linker_Common/")
set(st_vendor_dir "${AFR_VENDORS_DIR}/st")
set(wpan_dir "${AFR_VENDORS_DIR}/st/STM32_WPAN")
set(bluenrg_dir "${AFR_VENDORS_DIR}/st/STM32_BlueNRG1")
set(wpan_ble_dir "${AFR_VENDORS_DIR}/st/STM32_WPAN/ble")
set(crypto_dir "${AFR_VENDORS_DIR}/st/STM32_WPAN/ble/cryptographic")
set(afr_ports_dir "${CMAKE_CURRENT_LIST_DIR}/ports")
set(board_demos_dir "${CMAKE_CURRENT_LIST_DIR}/aws_demos")
set(board_tests_dir "${CMAKE_CURRENT_LIST_DIR}/aws_tests")
if(AFR_IS_TESTING)
    set(board_dir "${board_tests_dir}")
else()
    set(board_dir "${board_demos_dir}")
endif()

# -------------------------------------------------------------------------------------------------
# Amazon FreeRTOS disabled libraries
# -------------------------------------------------------------------------------------------------
set(AFR_MODULE_defender 0 CACHE INTERNAL "")
# HTTPS is not supported as this board does not have WiFi/Ethernet.
set(AFR_MODULE_https 0 CACHE INTERNAL "")
set(AFR_MODULE_secure_sockets 0 CACHE INTERNAL "")

# -------------------------------------------------------------------------------------------------
# Amazon FreeRTOS Console metadata
# -------------------------------------------------------------------------------------------------

afr_set_board_metadata(NAME "p_nucleo_wb55")
afr_set_board_metadata(ID "p_nucleo_wb55")
afr_set_board_metadata(DISPLAY_NAME "P-NUCLEO-WB55")
afr_set_board_metadata(DESCRIPTION "Bluetooth™ 5 and 802.15.4 Nucleo Pack including USB dongle and Nucleo-68 with STM32WB55 MCUs, supports Arduino™ Uno V3 and ST morpho connectivity")
afr_set_board_metadata(VENDOR_NAME "STMicroelectronics")
afr_set_board_metadata(FAMILY_NAME "STM32WB55")
afr_set_board_metadata(DATA_RAM_MEMORY "256KB")
afr_set_board_metadata(PROGRAM_MEMORY "1MB")
afr_set_board_metadata(CODE_SIGNER "null")
afr_set_board_metadata(SUPPORTED_IDE "STM32CubeIDE")
afr_set_board_metadata(RECOMMENDED_IDE "STM32CubeIDE")
afr_set_board_metadata(IDE_STM32CubeIDE_NAME "STM32CubeIDE")
afr_set_board_metadata(IDE_STM32CubeIDE_COMPILER "arm-gcc")
afr_set_board_metadata(IDE_STM32CubeIDE_PROJECT_LOCATION "projects/st/p_nucleo_wb55/STM32CubeIDE/STM32WB55RG_Nucleo_AFR_demo")
afr_set_board_metadata(KEY_IMPORT_PROVISIONING "TRUE")
afr_set_board_metadata(IS_ACTIVE "TRUE")


afr_set_board_metadata(IDE_SystemWorkbench_PROJECT_LOCATION "${AFR_ROOT_DIR}/projects/st/p_nucleo_wb55/ac6/aws_demos")
afr_set_board_metadata(AWS_DEMOS_CONFIG_FILES_LOCATION "${CMAKE_CURRENT_LIST_DIR}/aws_demos/config_files")

# -------------------------------------------------------------------------------------------------
# Compiler settings
# -------------------------------------------------------------------------------------------------
afr_mcu_port(compiler)
set(
    defined_symbols
    USE_HAL_DRIVER
    USE_OFFLOAD_SSL
    STM32WB55xx
    USE_STM32WBXX_NUCLEO
)
target_compile_definitions(
    AFR::compiler::mcu_port
    INTERFACE $<$<NOT:$<COMPILE_LANGUAGE:ASM>>:${defined_symbols}>
)

set(common_flags "-mcpu=cortex-m4" "-mthumb" "-mfloat-abi=softfp" "-mfpu=fpv4-sp-d16" "--specs=nano.specs")
set(c_flags "-Wall" "-fmessage-length=0" "-ffunction-sections" "-g" "-O3")
set(linker_flags "-specs=nosys.specs" "-Wl,-Map=output.map,--gc-sections,-zmuldefs" "-Wl,--start-group" "-lc" "-lm" "-Wl,--end-group")

# Compiler flags
target_compile_options(
    AFR::compiler::mcu_port
    INTERFACE
        ${common_flags}
        $<$<NOT:$<COMPILE_LANGUAGE:ASM>>:${c_flags}>
)

# Linker flags
target_link_options(
    AFR::compiler::mcu_port
    INTERFACE ${common_flags} ${linker_flags}
)

# -------------------------------------------------------------------------------------------------
# Amazon FreeRTOS portable layers
# -------------------------------------------------------------------------------------------------
# Kernel
afr_mcu_port(kernel)
afr_glob_src(driver_src DIRECTORY "${p_nucleo_wb55_dir}" RECURSE)


afr_glob_src(stm32_wpan_ble_core_src DIRECTORY "${wpan_ble_dir}/core/Src/core" RECURSE)
afr_glob_src(stm32_wpan_utils_src DIRECTORY "${wpan_dir}/utilities" RECURSE)

set(stm32_wpan_includes
        "${wpan_dir}"
        "${wpan_dir}/ble"
        "${wpan_dir}/ble/core/template"
        "${wpan_dir}/ble/core/auto"
        "${wpan_dir}/ble/core"
        "${wpan_dir}/interface/patterns/ble_thread"
        "${wpan_dir}/interface/patterns/ble_thread/tl"
        "${wpan_dir}/interface/patterns/ble_thread/shci"
        "${wpan_dir}/utilities"
    )

target_sources(
    AFR::kernel::mcu_port
    INTERFACE
    ${driver_src}
    "${AFR_KERNEL_DIR}/portable/GCC/ARM_CM4F/port.c"
    "${AFR_KERNEL_DIR}/portable/GCC/ARM_CM4F/portmacro.h"
    "${AFR_KERNEL_DIR}/portable/MemMang/heap_4.c"
)
set(
    kernel_inc_dirs
    "${AFR_KERNEL_DIR}/portable/GCC/ARM_CM4F"
    "${p_nucleo_wb55_dir}/BSP/P-NUCLEO-WB55.Nucleo"
    "${p_nucleo_wb55_dir}/BSP/Components/Common"
    "${p_nucleo_wb55_dir}/BSP/CMSIS_RTOS"
    "${st_vendor_dir}/CMSIS/Include"
    "${st_vendor_dir}/CMSIS/Device/ST/STM32WBxx/Include"
    "${p_nucleo_wb55_dir}/STM32WBxx_HAL_Driver/Inc"
    "${p_nucleo_wb55_dir}/STM32WBxx_HAL_Driver/Inc/Legacy"
#    "${p_nucleo_wb55_dir}/../../../libraries/abstractions/secure_sockets/include"
    "${board_dir}/application_code"
    "${board_dir}/application_code/st_code"
    "${board_dir}/application_code/st_code/lpm/tiny_lpm"
    "${wpan_dir}/interface/patterns/ble_thread"
    "${wpan_dir}/interface/patterns/ble_thread/shci"
    "${wpan_dir}/utilities"
    "${bluenrg_dir}/SimpleBlueNRG1_HCI/includes_temp"
    "${bluenrg_dir}"
    "$<IF:${AFR_IS_TESTING},${AFR_TESTS_DIR},${AFR_DEMOS_DIR}>/include"
     ${stm32_wpan_includes}
)

target_include_directories(
    AFR::kernel::mcu_port
    INTERFACE
        $<$<NOT:$<COMPILE_LANGUAGE:ASM>>:${kernel_inc_dirs}>
        "${board_dir}/config_files"
)

#BLE
set(BLE_SUPPORTED 1 CACHE INTERNAL "BLE is supported on this platform.")



afr_mcu_port(ble_hal)
target_sources(
    AFR::ble_hal::mcu_port
    INTERFACE
        "${afr_ports_dir}/ble/iot_ble_gap_config.h"
        "${afr_ports_dir}/ble/iot_ble_hal_common_gap.c"
        "${afr_ports_dir}/ble/iot_ble_hal_gap.c"
        "${afr_ports_dir}/ble/iot_ble_hal_gatt_server.c"
        "${afr_ports_dir}/ble/iot_ble_hal_internals.h"
        "${wpan_dir}/interface/patterns/ble_thread/tl/hci_tl.c"
        "${wpan_dir}/interface/patterns/ble_thread/tl/shci_tl.c"
        "${wpan_dir}/interface/patterns/ble_thread/tl/shci_tl_if.c"
        "${wpan_dir}/interface/patterns/ble_thread/tl/tl_mbox.c"
        "${wpan_dir}/interface/patterns/ble_thread/tl/hci_tl_if.c"
        "${wpan_dir}/interface/patterns/ble_thread/shci/shci.c"
        "${wpan_dir}/ble/core/auto/ble_hci_le.c"
        "${wpan_dir}/ble/core/auto/ble_gap_aci.c"
        "${wpan_dir}/ble/core/auto/ble_gatt_aci.c"
        "${wpan_dir}/ble/core/auto/ble_hal_aci.c"
        "${wpan_dir}/ble/core/auto/ble_l2cap_aci.c"
        "${wpan_dir}/utilities/stm_list.c"
        "${wpan_dir}/utilities/otp.c"
        "${wpan_dir}/utilities/stm_queue.c"
        "${wpan_dir}/utilities/dbg_trace.c"
        "${bluenrg_dir}/SimpleBlueNRG1_HCI/hci/bluenrg1_events.c"

        ${stm32_wpan_ble_core_src}
)
target_include_directories(
    AFR::ble_hal::mcu_port
    INTERFACE
        "${AFR_MODULES_C_SDK_DIR}/standard/common/include/private"
        "${afr_ports_dir}/ble"

)

#OTA
afr_mcu_port(ota)
target_sources(
    AFR::ota::mcu_port
    INTERFACE
        "${afr_ports_dir}/ota/aws_ota_pal.c"
        "${p_nucleo_wb55_iap_dir}/STM32WB55/flash_if.c"
        "${afr_ports_dir}/ota/sfu_app_new_image.c"
        "${afr_ports_dir}/ota/asn1utility.c"
        "${afr_ports_dir}/ota/flash_buffer.c"
        "${afr_ports_dir}/ota/flash_operations.c"
        "${afr_ports_dir}/ota/AFR_image_desc.c"

)
target_include_directories(
    AFR::ota::mcu_port
    INTERFACE
        "${p_nucleo_wb55_sbsfu_dir}/App"
        "${p_nucleo_wb55_sbsfu_dir}/Target"
        "${p_nucleo_wb55_ld_dir}"
        "${p_nucleo_wb55_secore_dir}/Inc"
        "${p_nucleo_wb55_iap_dir}/STM32WB55"
        "${crypto_dir}/Inc"

)

target_link_libraries(
    AFR::ota::mcu_port
    INTERFACE
    AFR::ota_mqtt
    3rdparty::tinycrypt
    -L"${wpan_dir}/ble/cryptographic/Lib"
    libSTM32CryptographicV3.0.0_CM4_GCC.a
    )

# -------------------------------------------------------------------------------------------------
# Amazon FreeRTOS demos and tests
# -------------------------------------------------------------------------------------------------
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

if(AFR_IS_TESTING)
    set(exe_target aws_tests)
else()
    set(exe_target aws_demos)
endif()

afr_glob_src(board_code_src DIRECTORY "${board_dir}/application_code/st_code")
afr_glob_src(config_files DIRECTORY "${board_dir}/config_files")

# Do not add demos or tests if they're turned off.
if(AFR_ENABLE_DEMOS OR AFR_ENABLE_TESTS)
    add_executable(
        ${exe_target}
        ${board_code_src}
        ${config_files}
        "${board_dir}/application_code/main.c"
    )

    link_directories("${wpan_dir}/ble/cryptographic/Lib")
    target_link_libraries(
        ${exe_target}
        PRIVATE
            3rdparty::jsmn
            AFR::ble_hal
            -T"${CMAKE_CURRENT_LIST_DIR}/STM32WB55xx_FLASH_CM4.ld"
            -L"${p_nucleo_wb55_ld_dir}"
    )

    if(AFR_METADATA_MODE)
        return()
    endif()

    find_program(gcc_objcopy arm-none-eabi-objcopy)
    find_program(gcc_size arm-none-eabi-size)
    if(NOT gcc_objcopy)
        message(FATAL_ERROR "Cannot find arm-none-eabi-objcopy.")
    endif()

    set(output_file "$<TARGET_FILE_DIR:${exe_target}>/${exe_target}.bin")
    add_custom_command(
        TARGET ${exe_target} POST_BUILD
        COMMAND "${gcc_objcopy}" -O binary "$<TARGET_FILE:${exe_target}>" "${output_file}"
        COMMAND "${gcc_size}" "$<TARGET_FILE:${exe_target}>"
    )	
endif()
