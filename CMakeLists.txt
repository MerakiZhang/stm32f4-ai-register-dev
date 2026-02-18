cmake_minimum_required(VERSION 3.20)

project(firmware C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(FW_TARGET firmware)

# ---- 源码 ----
set(STARTUP_ASM startup/startup_stm32f407zgtx.s)
set(SYSTEM_SRC drivers/stm32f4xx/source/system_stm32f4xx.c)
set(LD_SCRIPT  linker/STM32F407ZGTX_FLASH.ld)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_SOURCE_DIR}/src/*.c")

add_executable(${FW_TARGET}.elf
    ${STARTUP_ASM}
    ${SYSTEM_SRC}
    ${APP_SOURCES}
)

# ---- 头文件 ----
target_include_directories(${FW_TARGET}.elf PRIVATE
    ${CMAKE_SOURCE_DIR}/drivers/cmsis/include
    ${CMAKE_SOURCE_DIR}/drivers/stm32f4xx/include
    ${CMAKE_SOURCE_DIR}/src
)

# 选择目标芯片头文件
target_compile_definitions(${FW_TARGET}.elf PRIVATE
    STM32F407xx
    HSE_VALUE=8000000
)

# ---- 编译参数（C/ASM）----
set(MCU_FLAGS -mcpu=cortex-m4 -mthumb)
set(FPU_FLAGS -mfpu=fpv4-sp-d16 -mfloat-abi=softfp)

target_compile_options(${FW_TARGET}.elf PRIVATE
    ${MCU_FLAGS}
    ${FPU_FLAGS}
    -ffunction-sections -fdata-sections
    -Wall -Wextra
)

# ---- 链接参数 ----
target_link_options(${FW_TARGET}.elf PRIVATE
    ${MCU_FLAGS}
    ${FPU_FLAGS}
    -T${CMAKE_SOURCE_DIR}/${LD_SCRIPT}
    -Wl,-Map=${FW_TARGET}.map,--gc-sections
    -specs=nano.specs
    -specs=nosys.specs
)

set_target_properties(${FW_TARGET}.elf PROPERTIES
    OUTPUT_NAME ${FW_TARGET}
)

# ---- 产物：bin/hex + size ----
add_custom_command(TARGET ${FW_TARGET}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${FW_TARGET}.elf> ${FW_TARGET}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${FW_TARGET}.elf> ${FW_TARGET}.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${FW_TARGET}.elf>
    BYPRODUCTS ${FW_TARGET}.bin ${FW_TARGET}.hex ${FW_TARGET}.map
    COMMENT "Generating ${FW_TARGET}.bin/.hex and printing size"
)

