cmake_minimum_required(VERSION 3.20)
project("lab10-stm32")
enable_language(C ASM)

set(LAB_SOURCES
    Src/main.c
    Src/power_tasks.c
)
include(${CMAKE_SOURCE_DIR}/../cmake/stm32_lab_common.cmake)
