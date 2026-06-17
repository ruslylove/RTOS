cmake_minimum_required(VERSION 3.20)
project("lab05-stm32")
enable_language(C ASM)

set(STM32_CUBE_FW "C:/Users/rusle/STM32Cube/Repository/STM32Cube_FW_H5_V1.6.0" CACHE PATH "")
set(HAL_IWDG "${STM32_CUBE_FW}/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_iwdg.c")

set(LAB_SOURCES
    Src/main.c
    Src/watchdog.c
    Src/deadlock_tasks.c
)
set(LAB_EXTRA_HAL_SRCS ${HAL_IWDG})

include(${CMAKE_SOURCE_DIR}/../cmake/stm32_lab_common.cmake)
