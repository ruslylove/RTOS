cmake_minimum_required(VERSION 3.20)
project("lab06-stm32")
enable_language(C ASM)

set(LAB_SOURCES
    Src/main.c
    Src/workload.c
    Src/latency_test.c
)

include(${CMAKE_SOURCE_DIR}/../cmake/stm32_lab_common.cmake)
