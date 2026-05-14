cmake_minimum_required(VERSION 3.20)

project(gd32f303rct6_firmware C ASM)

set(FIRMWARE_NAME gd32f303rct6_firmware)
set(PROJECT1_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(SDK_ROOT "${PROJECT1_DIR}/GD32F303RxT6 KIT/3. SDK/GD32F303RCT6 Demo/Library/GD32F30x_Firmware_Library_V2.1.0/Firmware")
set(CMSIS_ROOT "${SDK_ROOT}/CMSIS")
set(GD32_CMSIS_ROOT "${CMSIS_ROOT}/GD/GD32F30x")
set(PERIPH_ROOT "${SDK_ROOT}/GD32F30x_standard_peripheral")
set(LCD_ROOT "${PROJECT1_DIR}/GD32F303RxT6 KIT/3. SDK/GD32F303RCT6 Demo/Examples/LCD 1.47inch/Components/Devices/TFTLCD")
set(LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/ld/GD32F303RCT6.ld")

if(NOT EXISTS "${SDK_ROOT}")
    message(FATAL_ERROR "GD32 SDK not found: ${SDK_ROOT}")
endif()

set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

add_executable(${FIRMWARE_NAME}
    src/main.c
    src/board.c
    src/lcd_spi.c
    src/systick.c
    src/gd32f30x_it.c
    startup/startup_gd32f30x_hd_gcc.S
    "${LCD_ROOT}/tftlcd.c"
    "${GD32_CMSIS_ROOT}/Source/system_gd32f30x.c"
    "${PERIPH_ROOT}/Source/gd32f30x_gpio.c"
    "${PERIPH_ROOT}/Source/gd32f30x_rcu.c"
    "${PERIPH_ROOT}/Source/gd32f30x_spi.c"
    "${PERIPH_ROOT}/Source/gd32f30x_usart.c"
)

set_target_properties(${FIRMWARE_NAME} PROPERTIES SUFFIX ".elf")

target_include_directories(${FIRMWARE_NAME} PRIVATE
    include
    "${CMSIS_ROOT}"
    "${GD32_CMSIS_ROOT}/Include"
    "${PERIPH_ROOT}/Include"
    "${LCD_ROOT}"
)

target_compile_definitions(${FIRMWARE_NAME} PRIVATE
    GD32F30X_HD
    USE_STDPERIPH_DRIVER
    HXTAL_VALUE=8000000
)

target_compile_options(${FIRMWARE_NAME} PRIVATE
    ${MCU_FLAGS}
    $<$<COMPILE_LANGUAGE:C>:-std=c11>
    $<$<COMPILE_LANGUAGE:C>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:C>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Wno-unused-parameter>
)

target_link_options(${FIRMWARE_NAME} PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    -nostdlib
    -Wl,--gc-sections
    -Wl,--no-warn-rwx-segments
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_NAME}.map
)

target_link_libraries(${FIRMWARE_NAME} PRIVATE gcc)

add_custom_command(TARGET ${FIRMWARE_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${FIRMWARE_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${FIRMWARE_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_NAME}.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${FIRMWARE_NAME}>
    BYPRODUCTS
        ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_NAME}.bin
        ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_NAME}.hex
        ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_NAME}.map
    VERBATIM
)
