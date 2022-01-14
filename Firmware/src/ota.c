#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "drivers/8258/flash.h"
#include "ota.h"
#include "main.h"

#define OTA_BANK_START 0x20000 // 131kb about
#define OTA_MAX_SIZE 0x20000   // 131kb about

RAM uint8_t out_buffer[20] = {0};

RAM uint8_t ramd_to_flash_temp_buffer[0x100];
RAM uint16_t ram_position = 0;

_attribute_ram_code_ int custom_otaWrite(void *p)
{
	rf_packet_att_write_t *req = (rf_packet_att_write_t *)p;
	uint8_t *payload = &req->value;
	uint8_t data_len = req->l2capLen - 3;
	uint32_t address = 0;
	uint32_t read_offset = 0;
	uint16_t crc_out = 0;
	if (data_len >= 5)
	{
		address = (payload[1] << 24) | (payload[2] << 16) | (payload[3] << 8) | payload[4];
	}

	switch (payload[0])
	{
	case 0:																						   // just a reboot to test
		analog_write(SYS_DEEP_ANA_REG, analog_read(SYS_DEEP_ANA_REG) & (~SYS_NEED_REINIT_EXT32K)); // clear
		irq_disable();
		REG_ADDR8(0x6f) = 0x20; // reboot
		while (1)
		{
		}
		break;
	case 1: // erasing a sector of the flash, better be careful here ^^ could erase the running firmware
		if (address >= OTA_BANK_START && address < (OTA_BANK_START + OTA_MAX_SIZE - 0x100))
		{
			flash_erase_sector(address);
		}
		memset(ramd_to_flash_temp_buffer, 0x00, sizeof(ramd_to_flash_temp_buffer));
		ram_position = 0;
		bls_att_pushNotifyData(OTA_CMD_OUT_DP_H, payload, data_len);
		break;
	case 2: // writing one bank (256byte) to flash at given sector
		if (address >= OTA_BANK_START && address < (OTA_BANK_START + OTA_MAX_SIZE - 0x100))
		{
			flash_write_page(address, ram_position, ramd_to_flash_temp_buffer);
		}
		memset(ramd_to_flash_temp_buffer, 0x00, sizeof(ramd_to_flash_temp_buffer));
		ram_position = 0;
		// bls_att_pushNotifyData(OTA_CMD_OUT_DP_H, payload, data_len);
		break;
	case 3: // write into the temporary buffer that will later be written to flash
		if (ram_position + (data_len - 1) > 0x100)
			return 0;
		memcpy(&ramd_to_flash_temp_buffer[ram_position], &payload[1], (data_len - 1));
		ram_position += (data_len - 1);
		// bls_att_pushNotifyData(OTA_CMD_OUT_DP_H, &ram_position, sizeof(ram_position));
		break;
	case 4: // read real flash to verify
		flash_read_page(address, sizeof(out_buffer), out_buffer);
		bls_att_pushNotifyData(OTA_CMD_OUT_DP_H, out_buffer, sizeof(out_buffer));
		break;
	case 5: // read real flash to verify
		memcpy(out_buffer, &ramd_to_flash_temp_buffer[address], sizeof(out_buffer));
		bls_att_pushNotifyData(OTA_CMD_OUT_DP_H, out_buffer, sizeof(out_buffer));
		break;
	case 6: // CRC Checking the uploaded part
		for (int i = 0; i < OTA_MAX_SIZE; i += 0x100)
		{
			flash_read_page(OTA_BANK_START + i, sizeof(ramd_to_flash_temp_buffer), ramd_to_flash_temp_buffer);
			for (int c = 0; c < 0x100; c++)
			{
				crc_out += ramd_to_flash_temp_buffer[c];// yeah thats not real CRC, but its better than nothing for now
			}
		}
		out_buffer[0] = 0x07;
		out_buffer[1] = crc_out >> 8;
		out_buffer[2] = crc_out;
		bls_att_pushNotifyData(OTA_CMD_OUT_DP_H, out_buffer, 3);
		break;
	case 7:						  // when upload is done flash the firmware with this cmd, better do some CRC or checking before as it could brick the device
		if (address = 0xC001CEED) // Only flash if "magic word" is send
			write_ota_firmware_to_flash();
		break;
	}

	return 0;
}

_attribute_ram_code_ int write_ota_firmware_to_flash()
{
	irq_disable();
	uint32_t address = 0;
	while (address < OTA_MAX_SIZE)
	{
		flash_erase_sector(address);
		address += 0x1000;
	}
	address = 0;
	while (address < OTA_MAX_SIZE)
	{
		flash_read_page(OTA_BANK_START + address, 0x100, ramd_to_flash_temp_buffer);
		flash_write_page(address, 0x100, ramd_to_flash_temp_buffer);
		address += 0x100;
	}
	analog_write(SYS_DEEP_ANA_REG, analog_read(SYS_DEEP_ANA_REG) & (~SYS_NEED_REINIT_EXT32K));
	REG_ADDR8(0x6f) = 0x20;
	while (1)
	{
	}
}