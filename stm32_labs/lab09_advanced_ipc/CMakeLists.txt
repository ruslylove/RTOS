cmake_minimum_required(VERSION 3.20)
project("lab09-stm32")
enable_language(C ASM)

set(LAB_SOURCES
    Src/main.c
    Src/ipc_demo.c
)
include(${CMAKE_SOURCE_DIR}/../cmake/stm32_lab_common.cmake)
