CUBE_DIR ?= ../STM32CubeF1
TARGET := firmware
BUILD := build
CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size

HAL := $(CUBE_DIR)/Drivers/STM32F1xx_HAL_Driver
CMSIS := $(CUBE_DIR)/Drivers/CMSIS
USBDEV := $(CUBE_DIR)/Middlewares/ST/STM32_USB_Device_Library

CFLAGS := -mcpu=cortex-m3 -mthumb -std=gnu11 -O1 -g3 -Wall -Wextra \
          -ffunction-sections -fdata-sections -DUSE_HAL_DRIVER -DSTM32F103xB
INCLUDES := -ICore/Inc -IStorage -IUSB -IUSB/Target \
            -I$(HAL)/Inc -I$(CMSIS)/Include \
            -I$(CMSIS)/Device/ST/STM32F1xx/Include \
            -I$(USBDEV)/Core/Inc -I$(USBDEV)/Class/MSC/Inc
LDFLAGS := -mcpu=cortex-m3 -mthumb -TSTM32CubeIDE/STM32F103C8TX_FLASH.ld \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map \
           --specs=nano.specs --specs=nosys.specs

APP_SRC := Core/Src/main.c Core/Src/stm32f1xx_it.c Core/Src/sysmem.c \
           Storage/storage.c USB/usb_device.c USB/usbd_storage_if.c \
           USB/Target/usbd_conf.c USB/Target/usbd_desc.c
HAL_SRC := $(HAL)/Src/stm32f1xx_hal.c \
           $(HAL)/Src/stm32f1xx_hal_rcc.c $(HAL)/Src/stm32f1xx_hal_rcc_ex.c \
           $(HAL)/Src/stm32f1xx_hal_gpio.c $(HAL)/Src/stm32f1xx_hal_cortex.c \
           $(HAL)/Src/stm32f1xx_hal_flash.c $(HAL)/Src/stm32f1xx_hal_flash_ex.c \
           $(HAL)/Src/stm32f1xx_hal_pwr.c $(HAL)/Src/stm32f1xx_hal_pcd.c \
           $(HAL)/Src/stm32f1xx_hal_pcd_ex.c
USB_SRC := $(wildcard $(USBDEV)/Core/Src/*.c) $(wildcard $(USBDEV)/Class/MSC/Src/*.c)
CMSIS_SRC := $(CMSIS)/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c
STARTUP := $(CMSIS)/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s

.PHONY: all clean check-deps
all: check-deps $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).hex

check-deps:
	@test -f "$(HAL)/Inc/stm32f1xx_hal.h" || (echo "Missing STM32CubeF1 at $(CUBE_DIR)."; exit 1)
	@test -f "$(USBDEV)/Class/MSC/Inc/usbd_msc.h" || (echo "Missing ST USB Device Library."; exit 1)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/$(TARGET).elf: $(APP_SRC) $(HAL_SRC) $(USB_SRC) $(CMSIS_SRC) $(STARTUP) | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $(APP_SRC) $(HAL_SRC) $(USB_SRC) $(CMSIS_SRC) $(STARTUP) $(LDFLAGS) -lc -lm -lnosys -o $@
	$(SIZE) $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

clean:
	rm -rf $(BUILD)
