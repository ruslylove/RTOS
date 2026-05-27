cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lab11_zephyr)
target_sources(app PRIVATE
    src/main.c
    src/sensor_thread.c
    src/logger_thread.c
    src/monitor_thread.c
)
