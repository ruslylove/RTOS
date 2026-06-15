set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN "C:/Users/rusle/.mcuxpressotools/arm-gnu-toolchain-14.2.rel1-mingw-w64-x86_64-arm-none-eabi/bin/arm-none-eabi")

set(CMAKE_C_COMPILER   "${TOOLCHAIN}-gcc.exe"     CACHE STRING "")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN}-g++.exe"     CACHE STRING "")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN}-gcc.exe"     CACHE STRING "")
set(CMAKE_OBJCOPY      "${TOOLCHAIN}-objcopy.exe" CACHE STRING "")
set(CMAKE_SIZE         "${TOOLCHAIN}-size.exe"    CACHE STRING "")

set(CPU_FLAGS "-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16")
set(CMAKE_C_FLAGS_INIT   "${CPU_FLAGS}" CACHE STRING "")
set(CMAKE_CXX_FLAGS_INIT "${CPU_FLAGS}" CACHE STRING "")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS}" CACHE STRING "")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
