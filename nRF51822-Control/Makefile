BOARD := BOARD_PCA10001

# Project Source
C_SOURCE_FILES += main.c
C_SOURCE_FILES += led.c
C_SOURCE_FILES += ble_ams_c.c

C_SOURCE_FILES += ble_srv_common.c
C_SOURCE_FILES += ble_sensorsim.c
C_SOURCE_FILES += softdevice_handler.c
C_SOURCE_FILES += ble_advdata.c
C_SOURCE_FILES += ble_error_log.c
C_SOURCE_FILES += ble_conn_params.c
C_SOURCE_FILES += app_timer.c
C_SOURCE_FILES += pstorage.c
C_SOURCE_FILES += crc16.c
C_SOURCE_FILES += device_manager_peripheral.c
C_SOURCE_FILES += app_trace.c
C_SOURCE_FILES += app_gpiote.c
C_SOURCE_FILES += app_button.c

OUTPUT_FILENAME := ble_app_ams
SDK_PATH = lib/nrf51_sdk_v6_0_0_43681/nrf51822/

DEVICE := NRF51
DEVICESERIES := nrf51

GDB_PORT_NUMBER := 2331

USE_LOADER := 0
USE_S110 := 1
SOFTDEVICE := lib/s110_nrf51822_7.0.0/s110_nrf51822_7.0.0_softdevice.hex
DEVICE_VARIANT := xxaa
USE_SOFTDEVICE := S110

ifeq ($(LINKER_SCRIPT),)
	ifeq ($(USE_SOFTDEVICE), S110)
		LINKER_SCRIPT = gcc_$(DEVICESERIES)_s110_$(DEVICE_VARIANT).ld
		OUTPUT_FILENAME := $(OUTPUT_FILENAME)_s110_$(DEVICE_VARIANT)
	else
		ifeq ($(USE_SOFTDEVICE), S210)
			LINKER_SCRIPT = gcc_$(DEVICESERIES)_s210_$(DEVICE_VARIANT).ld
			OUTPUT_FILENAME := $(OUTPUT_FILENAME)_s210_$(DEVICE_VARIANT)
		else
			LINKER_SCRIPT = gcc_$(DEVICESERIES)_blank_$(DEVICE_VARIANT).ld
			OUTPUT_FILENAME := $(OUTPUT_FILENAME)_$(DEVICE_VARIANT)
		endif
	endif
else
# Use externally defined settings
endif

SDK_INCLUDE_PATH = $(SDK_PATH)Include/
SDK_SOURCE_PATH = $(SDK_PATH)Source/
TEMPLATE_PATH += $(SDK_SOURCE_PATH)templates/gcc/

APP_COMMON_PATH += $(SDK_SOURCE_PATH)app_common
BLE_PATH += $(SDK_SOURCE_PATH)ble
BLE_PATH += $(SDK_SOURCE_PATH)ble/ble_services
BLE_PATH += $(SDK_SOURCE_PATH)ble/device_manager

OUTPUT_BINARY_DIRECTORY := build
ELF := $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out

SOFTDEVICE_OUTPUT = $(OUTPUT_BINARY_DIRECTORY)$(notdir $(SOFTDEVICE))

GNU_INSTALL_ROOT := tools/OSX/arm-cs-tools
GNU_VERSION := 4.8.3
GNU_PREFIX := arm-none-eabi

FLASH_START_ADDRESS = $(shell $(OBJDUMP) -h $(ELF) -j .text | grep .text | awk '{print $$4}')

CPU := cortex-m0

# Toolchain commands
CC       		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-gcc"
AS       		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-as"
AR       		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ar" -r
LD       		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ld"
NM       		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-nm"
OBJDUMP  		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objdump"
OBJCOPY  		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objcopy"
GDB       		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-gdb"
CGDB            := "/usr/local/bin/cgdb"

MK 				:= mkdir
RM 				:= rm -rf

# Programmer
JLINK = -tools/OSX/jlink/JLinkExe
JLINKGDBSERVER = tools/OSX/jlink/JLinkGDBServer

OBJECT_DIRECTORY := obj
LISTING_DIRECTORY := bin

C_SOURCE_FILES += system_$(DEVICESERIES).c
ASSEMBLER_SOURCE_FILES += gcc_startup_$(DEVICESERIES).s

# Linker flags
LDFLAGS += -L"$(GNU_INSTALL_ROOT)/arm-none-eabi/lib/armv6-m"
LDFLAGS += -L"$(GNU_INSTALL_ROOT)/lib/gcc/arm-none-eabi/$(GNU_VERSION)/armv6-m"
LDFLAGS += -Xlinker -Map=$(LISTING_DIRECTORY)/$(OUTPUT_FILENAME).map
LDFLAGS += -mcpu=$(CPU) -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T$(LINKER_SCRIPT)

# Compiler flags
CFLAGS += -mcpu=$(CPU) -mthumb -mabi=aapcs -D$(DEVICE)  -D$(BOARD) -DS110 --std=gnu99 -DBLE_STACK_SUPPORT_REQD
CFLAGS += -Wall -Werror
CFLAGS += -mfloat-abi=soft

#INCLUDEPATHS += -I../
INCLUDEPATHS += -Isrc
INCLUDEPATHS += -I$(GNU_INSTALL_ROOT)/$(GNU_PREFIX)/include
INCLUDEPATHS += -I$(GNU_INSTALL_ROOT)/lib/gcc/$(GNU_PREFIX)/$(GNU_VERSION)/include
INCLUDEPATHS += -I$(SDK_PATH)Include
INCLUDEPATHS += -I$(SDK_PATH)Include/boards
INCLUDEPATHS += -I$(SDK_PATH)Include/gcc
INCLUDEPATHS += -I$(SDK_PATH)Include/ext_sensors
INCLUDEPATHS += -I"$(SDK_PATH)Include/s110"
INCLUDEPATHS += -I"$(SDK_PATH)Include/ble"
INCLUDEPATHS += -I"$(SDK_PATH)Include/ble/device_manager"
INCLUDEPATHS += -I"$(SDK_PATH)Include/ble/ble_services"
INCLUDEPATHS += -I"$(SDK_PATH)Include/app_common"
INCLUDEPATHS += -I"$(SDK_PATH)Include/sd_common"
INCLUDEPATHS += -I"$(SDK_PATH)Include/sdk"


# Sorting removes duplicates
BUILD_DIRECTORIES := $(sort $(OBJECT_DIRECTORY) $(OUTPUT_BINARY_DIRECTORY) $(LISTING_DIRECTORY) )

####################################################################
# Rules                                                            #
####################################################################

C_SOURCE_FILENAMES = $(notdir $(C_SOURCE_FILES) )
ASSEMBLER_SOURCE_FILENAMES = $(notdir $(ASSEMBLER_SOURCE_FILES) )

# Make a list of source paths
C_SOURCE_PATHS = src $(SDK_SOURCE_PATH) $(APP_COMMON_PATH) $(BLE_PATH) $(TEMPLATE_PATH) $(wildcard $(SDK_SOURCE_PATH)*/)  $(wildcard $(SDK_SOURCE_PATH)ext_sensors/*/)
ASSEMBLER_SOURCE_PATHS = src $(SDK_SOURCE_PATH) $(TEMPLATE_PATH) $(wildcard $(SDK_SOURCE_PATH)*/)

C_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(C_SOURCE_FILENAMES:.c=.o) )
ASSEMBLER_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(ASSEMBLER_SOURCE_FILENAMES:.s=.o) )

# Set source lookup paths
vpath %.c $(C_SOURCE_PATHS)
vpath %.s $(ASSEMBLER_SOURCE_PATHS)

# Include automatically previously generated dependencies
-include $(addprefix $(OBJECT_DIRECTORY)/, $(COBJS:.o=.d))

## Default build target
.PHONY: all
all: release

clean:
	$(RM) $(OUTPUT_BINARY_DIRECTORY)/*
	$(RM) $(OBJECT_DIRECTORY)/*
	$(RM) $(LISTING_DIRECTORY)/*
	- $(RM) JLink.log
	- $(RM) .gdbinit

## Program device
#.PHONY: flash
#flash: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex
#	nrfjprog --reset --program $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex

### Targets
.PHONY: debug
debug:    CFLAGS += -DDEBUG -g3 -O0
debug:    $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex

.PHONY: release
release:  CFLAGS += -DNDEBUG -O3
release:  $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex

echostuff:
	echo $(C_OBJECTS)
	echo $(C_SOURCE_FILES)

## Create build directories
$(BUILD_DIRECTORIES):
	$(MK) $@

## Create objects from C source files
$(OBJECT_DIRECTORY)/%.o: %.c
# Build header dependencies
	$(CC) $(CFLAGS) $(INCLUDEPATHS) -M $< -MF "$(@:.o=.d)" -MT $@
# Do the actual compilation
	$(CC) $(CFLAGS) $(INCLUDEPATHS) -c -o $@ $<

## Assemble .s files
$(OBJECT_DIRECTORY)/%.o: %.s
	$(CC) $(ASMFLAGS) $(INCLUDEPATHS) -c -o $@ $<

## Link C and assembler objects to an .out file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out: $(BUILD_DIRECTORIES) $(C_OBJECTS) $(ASSEMBLER_OBJECTS)
	$(CC) $(LDFLAGS) $(C_OBJECTS) $(ASSEMBLER_OBJECTS) -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out

## Create binary .bin file from the .out file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	$(OBJCOPY) -O binary $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin

## Create binary .hex file from the .out file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	$(OBJCOPY) -O ihex $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex

## Program device
flash: rm-flash.jlink flash.jlink stopdebug
	$(JLINK) $(OUTPUT_BINARY_DIRECTORY)/flash.jlink

rm-flash.jlink:
	-rm -rf $(OUTPUT_BINARY_DIRECTORY)/flash.jlink
	
flash.jlink:
	echo "device nrf51822\nspeed 4000\nr\nloadbin $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin $(FLASH_START_ADDRESS)\nr\ng\nexit\n" > $(OUTPUT_BINARY_DIRECTORY)/flash.jlink
	
flash-softdevice: erase-all flash-softdevice.jlink stopdebug
ifndef SOFTDEVICE
	$(error "You need to set the SOFTDEVICE command-line parameter to a path (without spaces) to the softdevice hex-file")
endif

	# Convert from hex to binary. Split original hex in two to avoid huge (>250 MB) binary file with just 0s.
	$(OBJCOPY) -Iihex -Obinary $(SOFTDEVICE) $(SOFTDEVICE_OUTPUT:.hex=.bin)
    
	$(JLINK) -device NRF51822 -if SWD -speed 4000 $(OUTPUT_BINARY_DIRECTORY)/flash-softdevice.jlink

flash-softdevice.jlink:
	# Do magic. Write to NVMC to enable erase, do erase all and erase UICR, reset, enable writing, load mainpart bin, load uicr bin. Reset.
	# Resetting in between is needed to disable the protections. 
	#echo "w4 4001e504 1\nloadbin \"$(OUTPUT_BINARY_DIRECTORY)/_mainpart.bin\" 0\nloadbin \"$(OUTPUT_BINARY_DIRECTORY)/_uicr.bin\" 0x10001000\nr\ng\nexit\n" > $(OUTPUT_BINARY_DIRECTORY)/flash-softdevice.jlink
	echo "w4 4001e504 1\nloadbin \"$(SOFTDEVICE_OUTPUT:.hex=.bin)\" 0\nr\ng\nexit\n" > $(OUTPUT_BINARY_DIRECTORY)/flash-softdevice.jlink
	#echo "w4 4001e504 1\nloadbin \"$(OUTPUT_BINARY_DIRECTORY)/softdevice.bin\" 0\nr\ng\nexit\n" > flash-softdevice.jlink

recover: recover.jlink erase-all.jlink pin-reset.jlink
	$(JLINK) $(OUTPUT_BINARY_DIRECTORY)/recover.jlink
	$(JLINK) $(OUTPUT_BINARY_DIRECTORY)/erase-all.jlink
	$(JLINK) $(OUTPUT_BINARY_DIRECTORY)/pin-reset.jlink

recover.jlink:
	echo "si 0\nt0\nsleep 1\ntck1\nsleep 1\nt1\nsleep 2\nt0\nsleep 2\nt1\nsleep 2\nt0\nsleep 2\nt1\nsleep 2\nt0\nsleep 2\nt1\nsleep 2\nt0\nsleep 2\nt1\nsleep 2\nt0\nsleep 2\nt1\nsleep 2\nt0\nsleep 2\nt1\nsleep 2\ntck0\nsleep 100\nsi 1\nr\nexit\n" > $(OUTPUT_BINARY_DIRECTORY)/recover.jlink

pin-reset.jlink:
	echo "device nrf51822\nw4 4001e504 2\nw4 40000544 1\nr\nexit\n" > $(OUTPUT_BINARY_DIRECTORY)/pin-reset.jlink

erase-all: erase-all.jlink
	$(JLINK) $(OUTPUT_BINARY_DIRECTORY)/erase-all.jlink

erase-all.jlink:
	echo "device nrf51822\nw4 4001e504 2\nw4 4001e50c 1\nw4 4001e514 1\nr\nexit\n" > $(OUTPUT_BINARY_DIRECTORY)/erase-all.jlink

startdebug: stopdebug debug.jlink .gdbinit
	$(JLINKGDBSERVER) -single -if swd -speed 1000 -port $(GDB_PORT_NUMBER) &
	sleep 1
	$(GDB) $(ELF)

stopdebug:
	-killall $(JLINKGDBSERVER)

.gdbinit:
	echo "target remote localhost:$(GDB_PORT_NUMBER)\nmonitor flash download = 1\nmonitor flash device = nrf51822\nbreak main\nmon reset\n" > .gdbinit

debug.jlink:
	echo "Device nrf51822" > $(OUTPUT_BINARY_DIRECTORY)/debug.jlink
	
.PHONY: flash flash-softdevice erase-all startdebug stopdebug 


