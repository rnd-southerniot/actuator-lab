TARGET := mg513p30-f429i
BUILD_DIR := build

# CMSIS + STM32F4 HAL drivers are reused from the st-discovery reference clone
# (36 MB tree; not vendored here yet). Override on the CLI or env if your clone lives
# elsewhere:  make SHARED_DIR=/path/to/st-discovery/firmware/lcd-hello-f429i-disc1
# TODO(portability): vendor a trimmed Drivers/ into this folder (or a submodule) so the
# build does not depend on an external untracked path. Tracked in ../docs/NEXT_SESSION.md.
SHARED_DIR ?= $(HOME)/Developer/projects/robotics/st-discovery/firmware/lcd-hello-f429i-disc1

# Toolchain prefix. Needs a COMPLETE arm-none-eabi-gcc (WITH newlib). Homebrew's
# `arm-none-eabi-gcc` formula is compiler-only (no libc) and will NOT work. We auto-detect the
# vendored Arm GNU Toolchain under ~/Developer/projects/vendor/arm-gnu-toolchain-*/bin; if none is
# found, TOOLCHAIN_BIN is empty and we fall back to `arm-none-eabi-*` on PATH. Override explicitly:
#   make TOOLCHAIN_BIN=/path/to/arm-gnu-toolchain/bin
TOOLCHAIN_BIN ?= $(lastword $(sort $(wildcard $(HOME)/Developer/projects/vendor/arm-gnu-toolchain-*/bin)))
ifeq ($(TOOLCHAIN_BIN),)
  CROSS := arm-none-eabi-
else
  CROSS := $(TOOLCHAIN_BIN)/arm-none-eabi-
endif

CC := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE := $(CROSS)size

C_DEFS := -DUSE_HAL_DRIVER -DSTM32F429xx -DUSE_STM32F429I_DISCO
C_INCLUDES := \
  -ICore/Inc \
  -I$(SHARED_DIR)/Drivers/CMSIS/Include \
  -I$(SHARED_DIR)/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
  -I$(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Inc \
  -I$(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy \
  -I$(SHARED_DIR)/Drivers/BSP/STM32F429I-Discovery \
  -I$(SHARED_DIR)/Drivers/BSP/Components/ili9341 \
  -I$(SHARED_DIR)/Drivers/BSP/Components/Common \
  -I$(SHARED_DIR)/Utilities/Fonts

CPU_FLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS := $(CPU_FLAGS) $(C_DEFS) $(C_INCLUDES) -Og -g3 -Wall -Wextra -ffunction-sections -fdata-sections -fno-common
LDFLAGS := $(CPU_FLAGS) -TSTM32F429ZITX_FLASH.ld -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map --specs=nosys.specs -lm

APP_SRCS := \
  Core/Src/main.c \
  Core/Src/display.c \
  Core/Src/startup.c \
  Core/Src/system_stm32f4xx.c

# Step 3 HAL set: clock + GPIO + TIM (PWM/timebase/PID) + UART (console) + I2C (INA238).
HAL_SRCS := \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma2d.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_ltdc.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_ltdc_ex.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sdram.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c \
  $(SHARED_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_fmc.c

# Step 4 (optional LCD): ST BSP for the F429I-DISCO panel (ILI9341 + LTDC + SDRAM).
# Fonts are NOT listed here — stm32f429i_discovery_lcd.c #includes the font .c files
# directly, so adding them separately causes multiple-definition link errors.
BSP_SRCS := \
  $(SHARED_DIR)/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery.c \
  $(SHARED_DIR)/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_lcd.c \
  $(SHARED_DIR)/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_sdram.c \
  $(SHARED_DIR)/Drivers/BSP/Components/ili9341/ili9341.c

SRCS := $(APP_SRCS) $(HAL_SRCS) $(BSP_SRCS)
OBJS := $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))

.PHONY: all clean flash

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) STM32F429ZITX_FLASH.ld
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Requires st-flash (brew install stlink). Flashing is a hardware step — gated, not Step 1.
flash: $(BUILD_DIR)/$(TARGET).bin
	st-flash --reset write $(BUILD_DIR)/$(TARGET).bin 0x08000000

clean:
	rm -rf $(BUILD_DIR)
