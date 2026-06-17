# stm32_lab_common.cmake
# Include this from each lab's CMakeLists.txt AFTER defining LAB_SOURCES and
# (optionally) LAB_EXTRA_HAL_SRCS and LAB_EXTRA_INCLUDES.
#
# Usage in a lab CMakeLists.txt:
#   set(LAB_SOURCES Src/main.c Src/lab_tasks.c)
#   include(${CMAKE_SOURCE_DIR}/../cmake/stm32_lab_common.cmake)

cmake_minimum_required(VERSION 3.20)

# ── Configurable paths ────────────────────────────────────────────────────────
if(NOT DEFINED STM32_CUBE_FW)
    set(STM32_CUBE_FW "C:/Users/rusle/STM32Cube/Repository/STM32Cube_FW_H5_V1.6.0"
        CACHE PATH "STM32CubeH5 firmware package root")
endif()

if(NOT DEFINED FREERTOS_KERNEL_PATH)
    set(FREERTOS_KERNEL_PATH "C:/FreeRTOS-Kernel"
        CACHE PATH "FreeRTOS-Kernel repo root (clone from github.com/FreeRTOS/FreeRTOS-Kernel)")
endif()

get_filename_component(STM32_SHARED_DIR "${CMAKE_SOURCE_DIR}/../shared" ABSOLUTE)
set(HAL_PATH         "${STM32_CUBE_FW}/Drivers/STM32H5xx_HAL_Driver")
set(CMSIS_DEV_PATH   "${STM32_CUBE_FW}/Drivers/CMSIS/Device/ST/STM32H5xx")
set(CMSIS_CORE_PATH  "${STM32_CUBE_FW}/Drivers/CMSIS/Core/Include")

# ── Common source files ───────────────────────────────────────────────────────
set(COMMON_SOURCES
    # Startup / syscalls / heap
    "${CMSIS_DEV_PATH}/Source/Templates/gcc/startup_stm32h503xx.s"
    "${CMSIS_DEV_PATH}/Source/Templates/system_stm32h5xx.c"
    "${STM32_SHARED_DIR}/Src/syscall.c"
    "${STM32_SHARED_DIR}/Src/sysmem.c"

    # HAL core (always needed)
    "${HAL_PATH}/Src/stm32h5xx_hal.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_rcc.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_rcc_ex.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_gpio.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_uart.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_uart_ex.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_pwr.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_pwr_ex.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_cortex.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_flash.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_flash_ex.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_icache.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_dma.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_dma_ex.c"
    "${HAL_PATH}/Src/stm32h5xx_hal_exti.c"

    # FreeRTOS kernel
    "${FREERTOS_KERNEL_PATH}/tasks.c"
    "${FREERTOS_KERNEL_PATH}/queue.c"
    "${FREERTOS_KERNEL_PATH}/list.c"
    "${FREERTOS_KERNEL_PATH}/timers.c"
    "${FREERTOS_KERNEL_PATH}/event_groups.c"
    "${FREERTOS_KERNEL_PATH}/stream_buffer.c"
    "${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c"
    "${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM33_NTZ/non_secure/port.c"
    "${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c"

    # Lab-specific sources (defined in each lab's CMakeLists.txt)
    ${LAB_SOURCES}
    ${LAB_EXTRA_HAL_SRCS}
)

# ── Include directories ───────────────────────────────────────────────────────
set(COMMON_INCLUDES
    "${CMAKE_SOURCE_DIR}/Inc"
    "${STM32_SHARED_DIR}/Inc"
    "${HAL_PATH}/Inc"
    "${CMSIS_DEV_PATH}/Include"
    "${CMSIS_CORE_PATH}"
    "${FREERTOS_KERNEL_PATH}/include"
    "${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM33_NTZ/non_secure"
    ${LAB_EXTRA_INCLUDES}
)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME} ${COMMON_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${COMMON_INCLUDES})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    STM32H503xx
    USE_HAL_DRIVER
    $<$<CONFIG:Debug>:DEBUG>
)

target_compile_options(${PROJECT_NAME} PRIVATE
    ${CPU_FLAGS}
    -fstack-usage
    -Wall
    -Wextra
    -Wno-unused-parameter
    $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp -MMD -MP>
    $<$<CONFIG:Debug>:-O0 -g3 -ggdb>
    $<$<CONFIG:Release>:-Os>
)

target_link_options(${PROJECT_NAME} PRIVATE
    ${CPU_FLAGS}
    -T${STM32_SHARED_DIR}/stm32h503xb_flash.ld
    -Wl,-Map=${PROJECT_NAME}.map
    --specs=nosys.specs
    -Wl,--start-group -lc -lm -Wl,--end-group
    -Wl,-z,max-page-size=8
    -Wl,--print-memory-usage
)

# Post-build: hex + bin + size
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE}              $<TARGET_FILE:${PROJECT_NAME}>
    VERBATIM
)
