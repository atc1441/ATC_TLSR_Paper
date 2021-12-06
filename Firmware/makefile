TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8258

ifeq ($(OS), Windows_NT)
	TC32_COMPILER_PATH := tc32_windows\\bin\\
	WINDOWS_PREFIX :=$(TC32_COMPILER_PATH)
	LINUX_CHMOD := 
else
	TC32_COMPILER_PATH := ./tc32_linux/bin/
	WINDOWS_PREFIX :=
	LINUX_CHMOD := chmod 777 -R ./tc32_linux/
endif

LIBS :=  -llt_8258

TEL_PATH := ./

PROJECT_NAME := ATC_Paper

PROJECT_PATH := ./src
PROJECT_PATH_STATIC := ./static_src
OUT_PATH :=./out

ifneq ($(TEL_PATH)/make/makefile, $(wildcard $(TEL_PATH)/make/makefile))
$(error "Please check SDK_Path")
endif

OBJ_SRCS := 
S_SRCS := 
ASM_SRCS := 
C_SRCS := 
S_UPPER_SRCS := 
O_SRCS := 
FLASH_IMAGE := 
ELFS := 
OBJS := 
LST := 
SIZEDUMMY := 
OUT_DIR :=

GCC_FLAGS := \
-ffunction-sections \
-fdata-sections \
-Wall \
-O2 \
-fpack-struct \
-fshort-enums \
-finline-small-functions \
-std=gnu99 \
-funsigned-char \
-fshort-wchar \
-fms-extensions \
-Wno-unused

INCLUDE_PATHS := -I$(TEL_PATH)/components -I$(PROJECT_PATH)

GCC_FLAGS += $(TEL_CHIP)

LS_FLAGS := $(PROJECT_PATH_STATIC)/boot.link

#include SDK makefile
-include $(TEL_PATH)/make/application.mk
-include $(TEL_PATH)/make/common.mk
-include $(TEL_PATH)/make/vendor_common.mk
-include $(TEL_PATH)/make/tinyFlash.mk
-include $(TEL_PATH)/make/drivers_8258.mk
-include $(PROJECT_PATH_STATIC)/div_mod.mk

ifeq ($(USE_FREE_RTOS), 1)
-include $(TEL_PATH)/make/freertos.mk
GCC_FLAGS += -DUSE_FREE_RTOS
endif

#include Project makefile
-include $(PROJECT_PATH)/project.mk
-include $(PROJECT_PATH_STATIC)/boot.mk

# Add inputs and outputs from these tool invocations to the build variables 
LST_FILE := $(OUT_PATH)/$(PROJECT_NAME).lst
BIN_FILE := $(OUT_PATH)/../$(PROJECT_NAME).bin
ELF_FILE := $(OUT_PATH)/$(PROJECT_NAME).elf

# All Target
all: chmod_all pre-build main-build
# Main-build Target
main-build: $(ELF_FILE) secondary-outputs

# Tool invocations
$(ELF_FILE): $(OBJS) $(USER_OBJS)
	@echo 'Building Standard target: $@'
	@$(TC32_COMPILER_PATH)tc32-elf-ld --gc-sections -L $(TEL_PATH)/components/proj_lib -T $(LS_FLAGS) -o $(ELF_FILE) $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

$(LST_FILE): $(ELF_FILE)
	@echo 'Invoking: TC32 Create Extended Listing'
	@$(TC32_COMPILER_PATH)tc32-elf-objdump -x -D -l -S  $(ELF_FILE)  > $(LST_FILE)
	@echo 'Finished building: $@'
	@echo ' '

$(BIN_FILE): $(ELF_FILE)
	@echo 'Create Flash image (binary format)'
	@$(TC32_COMPILER_PATH)tc32-elf-objcopy -v -O binary $(ELF_FILE)  $(BIN_FILE)
	@python3 $(TEL_PATH)/make/tl_firmware_tools.py add_crc $(BIN_FILE)
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: $(ELF_FILE)
	@echo 'Invoking: Print Size'
	$(TC32_COMPILER_PATH)tc32-elf-size -t $(ELF_FILE)
	@echo 'Finished building: $@'
	@echo ' '

clean:
	-$(WINDOWS_PREFIX)rm $(FLASH_IMAGE) $(ELFS) $(OBJS) $(LST) $(ELF_FILE) $(BIN_FILE) $(LST_FILE)
	-@echo ' '

pre-build:
	$(WINDOWS_PREFIX)mkdir -p $(foreach s,$(OUT_DIR),$(OUT_PATH)$(s))
	-@echo ' '

chmod_all:
	-@$(LINUX_CHMOD)
	-@echo 'chmod_all'

secondary-outputs: $(BIN_FILE) $(LST_FILE) sizedummy

.PHONY: all clean
.SECONDARY: main-build 