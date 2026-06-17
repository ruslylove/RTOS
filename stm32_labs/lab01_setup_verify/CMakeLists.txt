cmake_minimum_required(VERSION 3.20)
project("lab01-stm32")
enable_language(C ASM)

set(LAB_SOURCES
    Src/main.c
)

include(${CMAKE_SOURCE_DIR}/../cmake/stm32_lab_common.cmake)
