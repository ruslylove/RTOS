TARGET   = lab01
TOOLCHAIN ?= /opt/homebrew/Cellar/arm-gcc-bin@12/12.2.Rel1/bin
CC       = $(TOOLCHAIN)/arm-none-eabi-gcc
OBJCOPY  = $(TOOLCHAIN)/arm-none-eabi-objcopy
GDB      = $(TOOLCHAIN)/arm-none-eabi-gdb
SIZE     = $(TOOLCHAIN)/arm-none-eabi-size

FREERTOS_DIR = FreeRTOS-Kernel

CPU_FLAGS  = -mcpu=cortex-m33+nodsp -mthumb
OPT_FLAGS  = -O0 -g
WARN_FLAGS = -Wall -Wextra

CFLAGS = $(CPU_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) \
         -ffunction-sections -fdata-sections \
         -I src \
         -I $(FREERTOS_DIR)/include \
         -I $(FREERTOS_DIR)/portable/GCC/ARM_CM33_NTZ/non_secure

LDFLAGS = -T linker_MCXN236.ld -nostartfiles \
          -Wl,--gc-sections -Wl,-Map=$(TARGET).map

# Application sources
APP_SRCS = src/startup_MCXN236.s \
           src/main.c \
           src/uart.c

# FreeRTOS kernel sources (no timers.c — lab01 doesn't use software timers)
KERNEL_SRCS = $(FREERTOS_DIR)/tasks.c \
              $(FREERTOS_DIR)/queue.c \
              $(FREERTOS_DIR)/list.c \
              $(FREERTOS_DIR)/portable/GCC/ARM_CM33_NTZ/non_secure/port.c \
              $(FREERTOS_DIR)/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c \
              $(FREERTOS_DIR)/portable/MemMang/heap_4.c

# Optional: build with SEGGER SystemView tracing
#   1. Copy SystemView source files into src/SystemView/
#      (SEGGER_SYSVIEW.c, SEGGER_RTT.c, SEGGER_RTT_printf.c,
#       SEGGER_SYSVIEW_Config_FreeRTOS.c, and all .h files)
#   2. Run:  make SYSVIEW=1
ifdef SYSVIEW
CFLAGS   += -DUSE_SYSVIEW -I src/SystemView
APP_SRCS += src/SystemView/SEGGER_SYSVIEW.c \
            src/SystemView/SEGGER_SYSVIEW_Config_FreeRTOS.c \
            src/SystemView/SEGGER_RTT.c \
            src/SystemView/SEGGER_RTT_printf.c
endif

SRCS = $(APP_SRCS) $(KERNEL_SRCS)
OBJS = $(SRCS:.c=.o)
OBJS := $(OBJS:.s=.o)

.PHONY: all clean flash debug setup sysview

all: $(TARGET).bin
	@$(SIZE) $(TARGET).elf

sysview:
	$(MAKE) SYSVIEW=1

$(TARGET).elf: $(OBJS)
	$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(LDFLAGS) $^ -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CPU_FLAGS) $(OPT_FLAGS) -c $< -o $@

clean:
	rm -f $(TARGET).elf $(TARGET).bin $(TARGET).map
	find . -name "*.o" -not -path "./$(FREERTOS_DIR)/.git/*" -delete

flash: $(TARGET).bin
	pyocd flash $(TARGET).elf -t mcxn236

setup:
	@if [ ! -d "$(FREERTOS_DIR)" ]; then \
	    git clone --depth=1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git $(FREERTOS_DIR); \
	else \
	    echo "$(FREERTOS_DIR) already exists, skipping clone"; \
	fi

debug: $(TARGET).bin
	@echo "Starting pyocd GDB server..."
	@pyocd gdbserver -t mcxn236 -S --port 3333 &
	@sleep 2
	@echo "Connecting GDB."
	$(GDB) $(TARGET).elf \
	    -ex "target remote localhost:3333" \
	    -ex "monitor semihosting enable" \
	    -ex "load" \
	    -ex "break main" \
	    -ex "continue"
