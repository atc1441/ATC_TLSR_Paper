#pragma once

#ifndef	custom_BLE_OTA_ENABLE
#define custom_BLE_OTA_ENABLE		1
#endif

#define FW_SIZE_MAX_K							0x20000			//256K


#define custom_CMD_OTA_FW_VERSION					0xff00
#define custom_CMD_OTA_START						0xff01
#define custom_CMD_OTA_END							0xff02


typedef struct{
	uint8_t  custom_ota_start_flag;
}custom_ota_service_t;

extern custom_ota_service_t custom_blcOta;

void uart_print(uint8_t *text,int len);

extern int custom_blt_ota_start_flag;
extern uint32_t custom_blt_ota_start_tick;
extern uint32_t custom_blt_ota_timeout_us;

extern uint32_t	custom_ota_program_offset;
extern uint32_t 	custom_ota_firmware_size_k;
extern uint32_t	custom_bls_ota_bootFlagAddr;
extern int 	custom_ota_program_bootAddr ;


typedef void (*custom_ota_startCb_t)(void);
typedef void (*custom_ota_versionCb_t)(void);

typedef void (*custom_ota_resIndicateCb_t)(int result);

extern custom_ota_resIndicateCb_t custom_otaResIndicateCb;


enum{
	custom_OTA_SUCCESS = 0,     //success
	custom_OTA_PACKET_LOSS,     //lost one or more OTA PDU
	custom_OTA_DATA_CRC_ERR,     //data CRC err
	custom_OTA_WRITE_FLASH_ERR,  //write OTA data to flash ERR
 	custom_OTA_DATA_UNCOMPLETE,  //lost last one or more OTA PDU
 	custom_OTA_TIMEOUT, 		  //
 	custom_OTA_OVERFLOW,		 // the ota adr overflow to 0x30000
 	custom_OTA_ERR_STS,
	custom_OTA_SUCCESS_DEBUG,     //success
};

void custom_bls_ota_procTimeout(void);

//user interface
void custom_bls_ota_registerStartCmdCb(custom_ota_startCb_t cb);
void custom_bls_ota_registerVersionReqCb(custom_ota_versionCb_t cb);
void custom_bls_ota_registerResultIndicateCb(custom_ota_resIndicateCb_t cb);

void custom_bls_ota_setTimeout(uint32_t timeout_us);
void custom_blt_slave_ota_finish_handle();
void custom_blt_ota_finished_flag_set(uint8_t reset_flag);
void custom_rf_link_slave_ota_finish_led_and_reboot(uint8_t st);

extern int custom_otaWrite(void * p);
extern int custom_otaRead(void * p);

extern void custom_start_reboot(void);	// irq_disable() inside

//firmware_size_k  must be 4k aligned, ota_offset will be ignored in 8267/8269, valid in 8261/8266
void custom_bls_ota_setFirmwareSizeAndOffset(int firmware_size_k, uint32_t ota_offset);
void custom_bls_ota_set_fwSize_and_fwBootAddr(int firmware_size_k, int boot_addr);

//only valid for 8261/8266
void custom_bls_ota_setBootFlagAddr(uint32_t bootFlag_addr);

void custom_bls_ota_clearNewFwDataArea(void);

unsigned short custom_crc16(unsigned char *pD, int len);
uint8_t custom_ota_condition_enable();
void custom_set_ota_reboot_flag(uint8_t flag);
uint8_t custom_get_fw_ota_value();
void custom_custom_blt_ota_finished_flag_set(uint8_t reset_flag);