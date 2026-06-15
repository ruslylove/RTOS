cmake_minimum_required(VERSION 3.20)

# Toolchain must be set before project()
set(CMAKE_TOOLCHAIN_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/armgcc_windows.cmake"
    CACHE STRING "")

project(lab08_s C ASM)

# ── SDK paths ─────────────────────────────────────────────────────────────────
set(SDK_ROOT      "C:/nxp_sdk/mcuxsdk/mcuxsdk")
set(SDK_DEVICE    "${SDK_ROOT}/devices/MCX/MCXN/MCXN236")
set(SDK_BOARD     "${SDK_ROOT}/examples/_boards/frdmmcxn236")
set(SDK_HELLO     "${SDK_BOARD}/freertos_examples/freertos_hello")

# ── CPU flags (-mcmse enables TrustZone Secure Extensions) ───────────────────
add_compile_options(-O0 -g -Wall -Wextra -ffunction-sections -fdata-sections -mcmse)
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -O0 -g" CACHE STRING "" FORCE)

# ── Preprocessor defines ──────────────────────────────────────────────────────
add_compile_definitions(
    CPU_MCXN236VDF
    MCXN236_SERIES
    __STARTUP_CLEAR_BSS
    "__START=main"
    DEBUG
    SDK_DEBUGCONSOLE=1
)

# ── Include directories ───────────────────────────────────────────────────────
include_directories(
    src
    ${SDK_DEVICE}
    ${SDK_DEVICE}/drivers
    ${SDK_ROOT}/devices/MCX/MCXN/periph1
    ${SDK_ROOT}/arch/arm/CMSIS/Core/Include
    ${SDK_ROOT}/arch/arm/CMSIS/Core/Include/m-profile
    ${SDK_ROOT}/drivers/common
    ${SDK_ROOT}/drivers/gpio
    ${SDK_ROOT}/drivers/lpflexcomm
    ${SDK_ROOT}/drivers/lpflexcomm/lpuart
    ${SDK_ROOT}/drivers/mcx_spc
    ${SDK_ROOT}/drivers/port
    ${SDK_ROOT}/components/assert
    ${SDK_ROOT}/components/debug_console_lite
    ${SDK_ROOT}/components/str
    ${SDK_ROOT}/components/uart
    ${SDK_BOARD}
    ${SDK_HELLO}
)

# ── Source files ──────────────────────────────────────────────────────────────
set(APP_SRCS
    src/main_secure.c
    src/secure_api.c
    ${SDK_DEVICE}/gcc/startup_MCXN236.S
    ${SDK_DEVICE}/system_MCXN236.c
    ${SDK_DEVICE}/drivers/fsl_clock.c
    ${SDK_DEVICE}/drivers/fsl_reset.c
    ${SDK_ROOT}/drivers/common/fsl_common.c
    ${SDK_ROOT}/drivers/common/fsl_common_arm.c
    ${SDK_ROOT}/drivers/gpio/fsl_gpio.c
    ${SDK_ROOT}/drivers/lpflexcomm/fsl_lpflexcomm.c
    ${SDK_ROOT}/drivers/mcx_spc/fsl_spc.c
    ${SDK_ROOT}/drivers/lpflexcomm/lpuart/fsl_lpuart.c
    ${SDK_ROOT}/components/assert/fsl_assert.c
    ${SDK_ROOT}/components/debug_console_lite/fsl_debug_console.c
    ${SDK_ROOT}/components/str/fsl_str.c
    ${SDK_ROOT}/components/uart/fsl_adapter_lpuart.c
    ${SDK_ROOT}/components/misc_utilities/fsl_sbrk.c
    ${SDK_ROOT}/components/misc_utilities/fsl_syscall_stub.c
    ${SDK_BOARD}/board.c
    ${SDK_BOARD}/clock_config.c
    ${SDK_HELLO}/pin_mux.c
    ${SDK_HELLO}/hardware_init.c
)

# ── Target ────────────────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME} ${APP_SRCS})

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

target_link_options(${PROJECT_NAME} PRIVATE
    -T${SDK_DEVICE}/gcc/MCXN236_flash_s.ld
    -nostartfiles
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--cmse-implib
    -Wl,--out-implib=${CMAKE_CURRENT_BINARY_DIR}/cmse_implib.o
    -lm
)

# Generate .bin from .elf
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMENT "Generating ${PROJECT_NAME}.bin"
)

# Print size after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
)
