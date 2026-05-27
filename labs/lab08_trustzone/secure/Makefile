TARGET   = lab08_s

SDK_ROOT   = /Users/rus/Development/NXP_project/SDK/mcuxsdk/mcuxsdk
SDK_DEVICE = $(SDK_ROOT)/devices/MCX/MCXN/MCXN236
SDK_BOARD  = $(SDK_ROOT)/examples/_boards/frdmmcxn236
SDK_HELLO  = $(SDK_BOARD)/freertos_examples/freertos_hello

TOOLCHAIN ?= /opt/homebrew/Cellar/arm-gcc-bin@12/12.2.Rel1/bin
CC       = $(TOOLCHAIN)/arm-none-eabi-gcc
OBJCOPY  = $(TOOLCHAIN)/arm-none-eabi-objcopy
GDB      = $(TOOLCHAIN)/arm-none-eabi-gdb
SIZE     = $(TOOLCHAIN)/arm-none-eabi-size

CPU_FLAGS  = -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -mcmse
OPT_FLAGS  = -O0 -g
WARN_FLAGS = -Wall -Wextra

# -mcmse enables Cortex-M Security Extensions (TrustZone) in the compiler.
# The Secure image is linked at 0x00000000 (start of flash).

DEFINES = \
    -DCPU_MCXN236VDF \
    -DMCXN236_SERIES \
    -D__STARTUP_CLEAR_BSS \
    -DDEBUG \
    -DSDK_DEBUGCONSOLE=1

INCLUDES = \
    -I src \
    -I $(SDK_DEVICE) \
    -I $(SDK_DEVICE)/drivers \
    -I $(SDK_ROOT)/devices/MCX/MCXN/periph1 \
    -I $(SDK_ROOT)/arch/arm/CMSIS/Core/Include \
    -I $(SDK_ROOT)/arch/arm/CMSIS/Core/Include/m-profile \
    -I $(SDK_ROOT)/drivers/common \
    -I $(SDK_ROOT)/drivers/gpio \
    -I $(SDK_ROOT)/drivers/lpflexcomm \
    -I $(SDK_ROOT)/drivers/lpflexcomm/lpuart \
    -I $(SDK_ROOT)/drivers/mcx_spc \
    -I $(SDK_ROOT)/drivers/port \
    -I $(SDK_ROOT)/components/assert \
    -I $(SDK_ROOT)/components/debug_console_lite \
    -I $(SDK_ROOT)/components/str \
    -I $(SDK_ROOT)/components/uart \
    -I $(SDK_BOARD) \
    -I $(SDK_HELLO)

CFLAGS = $(CPU_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) \
         -ffunction-sections -fdata-sections \
         $(INCLUDES) $(DEFINES)

LDFLAGS = -T $(SDK_DEVICE)/gcc/MCXN236_flash.ld \
          -nostartfiles \
          -Wl,--gc-sections \
          -Wl,-Map=$(TARGET).map \
          -lm

# Generate CMSE import library for the Non-Secure project to link against.
# This allows NS code to call NSC (Non-Secure Callable) functions via SG veneers.
CMSE_LIB = cmse_implib.o
LDFLAGS  += -Wl,--cmse-implib -Wl,--out-implib=$(CMSE_LIB)

APP_SRCS = \
    src/main_secure.c \
    src/secure_api.c \
    $(SDK_DEVICE)/gcc/startup_MCXN236.S \
    $(SDK_DEVICE)/system_MCXN236.c \
    $(SDK_DEVICE)/drivers/fsl_clock.c \
    $(SDK_DEVICE)/drivers/fsl_reset.c \
    $(SDK_ROOT)/drivers/common/fsl_common.c \
    $(SDK_ROOT)/drivers/common/fsl_common_arm.c \
    $(SDK_ROOT)/drivers/gpio/fsl_gpio.c \
    $(SDK_ROOT)/drivers/lpflexcomm/fsl_lpflexcomm.c \
    $(SDK_ROOT)/drivers/mcx_spc/fsl_spc.c \
    $(SDK_ROOT)/drivers/lpflexcomm/lpuart/fsl_lpuart.c \
    $(SDK_ROOT)/components/assert/fsl_assert.c \
    $(SDK_ROOT)/components/debug_console_lite/fsl_debug_console.c \
    $(SDK_ROOT)/components/str/fsl_str.c \
    $(SDK_ROOT)/components/uart/fsl_adapter_lpuart.c \
    $(SDK_ROOT)/components/misc_utilities/fsl_sbrk.c \
    $(SDK_ROOT)/components/misc_utilities/fsl_syscall_stub.c \
    $(SDK_BOARD)/board.c \
    $(SDK_BOARD)/clock_config.c \
    $(SDK_HELLO)/pin_mux.c \
    $(SDK_HELLO)/hardware_init.c

SRCS = $(APP_SRCS)
OBJS = $(SRCS:.c=.o)
OBJS := $(OBJS:.S=.o)
OBJS := $(OBJS:.s=.o)

LINKSERVER ?= /Applications/LinkServer_26.3.123/LinkServer

.PHONY: all clean flash debug setup

all: $(TARGET).bin
	@$(SIZE) $(TARGET).elf

$(TARGET).elf: $(OBJS)
	$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(LDFLAGS) $^ -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -D__START=main -c $< -o $@

%.o: %.s
	$(CC) $(CPU_FLAGS) $(OPT_FLAGS) -c $< -o $@

clean:
	rm -f $(TARGET).elf $(TARGET).bin $(TARGET).map $(CMSE_LIB)
	find . -name "*.o" -delete
	rm -f $(OBJS)

flash: $(TARGET).bin
	$(LINKSERVER) flash MCXN236:FRDM-MCXN236 load $(TARGET).elf
	@echo "Secure flash complete. Now build and flash the Non-Secure image."

setup:
	@echo "Secure-world image for Lab 08 TrustZone (no FreeRTOS)"
	@echo "After building, copy $(CMSE_LIB) to ../nonsecure/ for NS linking."

debug: $(TARGET).bin
	@echo "Starting LinkServer GDB server..."
	@$(LINKSERVER) gdbserver MCXN236:FRDM-MCXN236 &
	@sleep 2
	$(GDB) $(TARGET).elf \
	    -ex "target remote localhost:3333" \
	    -ex "load" \
	    -ex "break main" \
	    -ex "continue"
