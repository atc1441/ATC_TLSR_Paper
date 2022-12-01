OBJS += \
$(OUT_PATH)/app.o \
$(OUT_PATH)/app_att.o \
$(OUT_PATH)/battery.o \
$(OUT_PATH)/ble.o \
$(OUT_PATH)/epd_ble_service.o \
$(OUT_PATH)/i2c.o \
$(OUT_PATH)/cmd_parser.o \
$(OUT_PATH)/flash.o \
$(OUT_PATH)/time.o \
$(OUT_PATH)/epd_spi.o \
$(OUT_PATH)/epd.o \
$(OUT_PATH)/epd_bw_213.o \
$(OUT_PATH)/epd_bwr_213.o \
$(OUT_PATH)/epd_bwr_350.o \
$(OUT_PATH)/epd_bwy_350.o \
$(OUT_PATH)/epd_bw_213_ice.o \
$(OUT_PATH)/epd_bwr_154.o \
$(OUT_PATH)/ota.o \
$(OUT_PATH)/led.o \
$(OUT_PATH)/uart.o \
$(OUT_PATH)/nfc.o \
$(OUT_PATH)/tiffg4.o \
$(OUT_PATH)/one_bit_display.o \
$(OUT_PATH)/main.o

# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/%.o: ./src/%.c
	@echo 'Building file: $<'
	@$(TC32_COMPILER_PATH)tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"
