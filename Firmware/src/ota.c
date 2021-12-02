#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "drivers/8258/flash.h"
#include "ota.h"
#include "main.h"

RAM int custom_ota_adr_index = -1;
RAM uint32_t custom_blt_ota_start_tick;
RAM uint32_t custom_blt_ota_timeout_us = 30000000;  //default 30 second
uint32_t custom_blt_ota_finished_time = 0;
uint8_t  custom_blt_ota_finished_flag = 0;
uint8_t  custom_blt_ota_terminate_flag = 0;

RAM custom_ota_service_t custom_blcOta;

RAM	int		custom_ota_program_bootAddr = 0x20000;
    
RAM	uint32_t		custom_ota_program_offset = 0x20000;
RAM	uint32_t 	custom_ota_firmware_size_k = FW_SIZE_MAX_K;



RAM	custom_ota_startCb_t		custom_otaStartCb = NULL;
RAM	custom_ota_versionCb_t 	custom_otaVersionCb = NULL;
RAM	custom_ota_resIndicateCb_t custom_otaResIndicateCb = NULL;



_attribute_ram_code_ void uart_print(uint8_t *text,int len){	
	for(unsigned char i=0;i<len;i++){
		uart_ndma_send_byte(text[i]);
	}
	while(uart_tx_is_busy())
	{
		sleep_us(10);
	};
}

_attribute_ram_code_ unsigned short custom_crc16 (unsigned char *pD, int len)
{
    static unsigned short poly[2]={0, 0xa001};              //0x8005 <==> 0xa001
    unsigned short crc = 0xffff;
    //unsigned char ds;
    int i,j;

    for(j=len; j>0; j--)
    {
        unsigned char ds = *pD++;
        for(i=0; i<8; i++)
        {
            crc = (crc >> 1) ^ poly[(crc ^ ds ) & 1];
            ds = ds >> 1;
        }
    }

     return crc;
}

void custom_bls_ota_setBootFlagAddr(uint32_t bootFlag_addr)
{
	custom_bls_ota_bootFlagAddr = bootFlag_addr;
}


void custom_bls_ota_registerStartCmdCb(custom_ota_startCb_t cb)
{
	custom_otaStartCb = cb;
}

void custom_bls_ota_registerVersionReqCb(custom_ota_versionCb_t cb)
{
	custom_otaVersionCb = cb;
}

void custom_bls_ota_registerResultIndicateCb(custom_ota_resIndicateCb_t cb)
{
	custom_otaResIndicateCb = cb;
}

void custom_bls_ota_setTimeout(uint32_t timeout_us)
{
	custom_blt_ota_timeout_us = timeout_us;
}

_attribute_ram_code_ void custom_start_reboot(void)
{
	if(blt_miscParam.pad32k_en){
		analog_write(SYS_DEEP_ANA_REG, analog_read(SYS_DEEP_ANA_REG) & (~SYS_NEED_REINIT_EXT32K)); //clear
	}
	irq_disable ();
	REG_ADDR8(0x6f) = 0x20;  //reboot
	while (1);
}


_attribute_ram_code_ void custom_ota_set_flag()
{
	uint32_t flag = 0x4b;
	flash_write_page(custom_ota_program_offset + 8, 1, (uint8_t *)&flag);		//Set FW flag
	flag = 0;
	flash_write_page((custom_ota_program_offset ? 0 : custom_ota_program_bootAddr) + 8, 4, (uint8_t *)&flag);	//Invalid flag
}
uint8_t custom_fw_ota_value = 0;
_attribute_ram_code_ uint8_t get_custom_fw_ota_value()
{
	return custom_fw_ota_value;
}

_attribute_ram_code_ void custom_ota_save_data(uint32_t adr, uint8_t * data, uint16_t len){
	if (adr == 0)
	{
		custom_fw_ota_value = data[8];
		data[8] = 0xff;					//FW flag invalid
	}

	flash_write_page(custom_ota_program_offset + adr, len, data);
}
uint8_t custom_ota_reboot_flag = 1;// initial it will reboot 
_attribute_ram_code_ void set_custom_ota_reboot_flag(uint8_t flag)
{
	custom_ota_reboot_flag = flag;
	return ;
}

_attribute_ram_code_ int custom_otaWrite(void * p)
{
	rf_packet_att_data_t *req = (rf_packet_att_data_t*)p;
	printf("OTA write 0x%02x%02x\r\n",req->dat[0],req->dat[1]);
	static uint32_t fw_check_val = 0;
	static uint8_t need_check_type = 0;//=1:crc val sum
	static uint16_t ota_pkt_total = 0;

	uint8_t err_flg = custom_OTA_SUCCESS;
	uint16_t ota_adr =  req->dat[0] | (req->dat[1]<<8);
	uint16_t crc;
	
	if(ota_adr == custom_CMD_OTA_FW_VERSION){
	printf("OTA CMD version\r\n");
		//to be add
		//log_event(TR_T_ota_version);
		custom_blt_ota_start_tick = clock_time()|1;  //mark time
		if(custom_otaVersionCb){
			custom_otaVersionCb();
		}
	}
	else if(ota_adr == custom_CMD_OTA_START){
	printf("OTA CMD start\r\n");
		custom_ota_adr_index = -1;
		if(custom_ota_reboot_flag){
			custom_blcOta.custom_ota_start_flag = 1;   //set flag
			custom_blt_ota_start_tick = clock_time()|1;  //mark time
			if(custom_otaStartCb){
				custom_otaStartCb();
			}
		}
		
		//log_event(TR_T_ota_start);
	}
	else if(ota_adr == custom_CMD_OTA_END){
	printf("OTA CMD end\r\n");
		//log_event(TR_T_ota_end);

		uint16_t adrIndex_max	   = req->dat[2] | (req->dat[3]<<8);
		uint16_t adrIndex_max_check = req->dat[4] | (req->dat[5]<<8);


		//if no index_max check, set ota success directly, otherwise we check if any index_max match
		if( req->l2cap == 9 && (adrIndex_max ^ adrIndex_max_check) == 0xffff){  //index_max valid, we can check
			if(adrIndex_max != custom_ota_adr_index){  //last one or more packets missed
				printf("OTA CMD uncomplete1\r\n");
				err_flg = custom_OTA_DATA_UNCOMPLETE;
			}
		}
		if(ota_pkt_total != custom_ota_adr_index + 1){
			printf("OTA CMD uncomplete2\r\n");
			err_flg = custom_OTA_DATA_UNCOMPLETE;
		}

		custom_custom_blt_ota_finished_flag_set(err_flg);
	}
	else{
	printf("OTA CMD data\r\n");
		//log_task_begin(TR_T_ota_data);
		if(custom_ota_adr_index + 1 == ota_adr){   //correct OTA data index
			custom_blt_ota_start_tick = clock_time()|1;  //mark time
			crc = (req->dat[19]<<8) | req->dat[18];
			if(crc == custom_crc16(req->dat, 18)){
				if(ota_adr == 0){
				    if(req->dat[8] == 0x5D){
				    	need_check_type = req->dat[9] ;
				    }
				    if(need_check_type == 1){
				    	fw_check_val = (req->dat[18] | req->dat[19]<<8);
				    }
				}else if(ota_adr == 1){
					ota_pkt_total = (((req->dat[10]) |( (req->dat[11] << 8) & 0xFF00) | ((req->dat[12] << 16) & 0xFF0000) | ((req->dat[13] << 24) & 0xFF000000)) + 15)/16;
					if(ota_pkt_total < 3){
						// invalid fw
						printf("OTA CMD sts1\r\n");
						err_flg = custom_OTA_ERR_STS;
					}else if(need_check_type == 1){
						fw_check_val += (req->dat[18] | req->dat[19]<<8);
					}
				}else if(ota_adr < ota_pkt_total - 1 && need_check_type == 1){
					fw_check_val += (req->dat[18] | req->dat[19]<<8);
				}else if(ota_adr == ota_pkt_total - 1 && need_check_type == 1){
					if(fw_check_val != ((req->dat[2]) |( (req->dat[3] << 8) & 0xFF00) | ((req->dat[4] << 16) & 0xFF0000) | ((req->dat[5] << 24) & 0xFF000000)) ){
						printf("OTA CMD sts2\r\n");
						err_flg = custom_OTA_ERR_STS;
					}
				}
				//log_data(TR_24_ota_adr,ota_adr);
				if((ota_adr<<4)>=(custom_ota_firmware_size_k<<10)){
					printf("OTA CMD overflow\r\n");
					err_flg = custom_OTA_OVERFLOW;
				}else{
					custom_ota_save_data (ota_adr<<4, req->dat + 2, 16);

					uint8_t flash_check[16];

					flash_read_page(custom_ota_program_offset + (ota_adr<<4),16,flash_check);

					if(!memcmp(flash_check,req->dat + 2,16)){  //OK
						custom_ota_adr_index = ota_adr;
					}
					else{ //flash write err
						err_flg = custom_OTA_WRITE_FLASH_ERR;
					}
				}
				
			}
			else{  //crc err
				printf("OTA CMD crc err\r\n");
				err_flg = custom_OTA_DATA_CRC_ERR;
			}
		}
		else if(custom_ota_adr_index >= ota_adr){  //maybe repeated OTA data, we neglect it, do not consider it ERR

		}
		else{  //adr index err, missing at least one OTA data
			printf("OTA CMD loss\r\n");
			err_flg = custom_OTA_PACKET_LOSS;
		}
		//log_task_end(TR_T_ota_data);

	}

	if(err_flg){
		//log_event(TR_T_ota_err);
		printf("11111OTA finish flag set %i\r\n",err_flg);
		custom_custom_blt_ota_finished_flag_set(err_flg);
	}

	return 0;
}

_attribute_ram_code_ int custom_otaRead(void * p)
{
	return 0;
}


_attribute_ram_code_ void custom_bls_ota_clearNewFwDataArea(void)
{
	printf("OTA prepaire\r\n");
	uint8_t cur_bank[4] = {0};
	flash_read_page(8,4,cur_bank);
	printf("OTA checking curr firmware bank 0 %02x\r\n", cur_bank[0]);
	if(cur_bank[0] == 0x4b)
		custom_ota_program_offset = 0x20000;
	else
		custom_ota_program_offset = 0x00;
	
		printf("OTA clear new data area\r\n");
		uint32_t tmp1 = 0;
		uint32_t tmp2 = 0;
		int cur_flash_setor;
		for(uint32_t i = 0; i < (custom_ota_firmware_size_k/0x1000); ++i)
		{
			cur_flash_setor = custom_ota_program_offset + i*0x1000;
			printf("OTA test sector 0x%x\r\n",cur_flash_setor);
			flash_read_page(cur_flash_setor, 		4, (uint8_t *)&tmp1);
			flash_read_page(cur_flash_setor + 2048, 4, (uint8_t *)&tmp2);

			if(tmp1 != ONES_32 || tmp2 != ONES_32)
			{
				printf("OTA clear sector 0x%x\r\n",cur_flash_setor);
				flash_erase_sector(cur_flash_setor);
			}
		}
}


_attribute_ram_code_ void custom_bls_ota_procTimeout(void)
{
	custom_blt_slave_ota_finish_handle();

	if(clock_time_exceed(custom_blt_ota_start_tick , custom_blt_ota_timeout_us)){  //OTA timeout
		custom_rf_link_slave_ota_finish_led_and_reboot(OTA_TIMEOUT);
	}
}

_attribute_ram_code_ void  custom_custom_blt_ota_finished_flag_set(uint8_t reset_flag)
{
	printf("OTA finish flag set %i\r\n",reset_flag);
	if(custom_blt_ota_finished_time == 0){
		custom_blt_ota_finished_flag = reset_flag;
		custom_blt_ota_finished_time = clock_time()|1;
	}
}

_attribute_ram_code_ void custom_rf_link_slave_ota_finish_led_and_reboot(uint8_t st)
{
	printf("OTA timeout %i\r\n",st);
	if(custom_OTA_SUCCESS == st){
        custom_ota_set_flag ();
    }
    else{
       if(custom_ota_adr_index>=0){
			/*irq_disable();

			//for(int i=0;i<=custom_ota_adr_index;i+=256)
			for(int i=(custom_ota_adr_index&0x1ff00); i>=0; i-=256) //erase from end to head
			{  //4K/16 = 256
				flash_erase_sector(custom_ota_program_offset + (i<<4));
			}*/
		}
    }
	
#if KEEP_ONOFF_STATE_AFTER_OTA 
	analog_write(DEEP_ANA_REG0, analog_read(DEEP_ANA_REG0) | BIT(custom_ota_reboot_flag));
#endif
	if(custom_otaResIndicateCb){
		custom_otaResIndicateCb(st);   // should be better at last.
	}
    irq_disable ();
    custom_start_reboot();
}

_attribute_ram_code_ void custom_blt_slave_ota_finish_handle()		
{	
    if(custom_blt_ota_finished_time){
		printf("OTA finish handle %i\r\n",custom_blt_ota_finished_time);
        static uint8_t terminate_cnt;
        uint8_t reboot_flag = 0;
        if((0 == terminate_cnt) && (custom_blt_ota_terminate_flag)){
               terminate_cnt = 6;
               bls_ll_terminateConnection(0x13);
        }
        
        if(terminate_cnt){
            terminate_cnt--;
            if(!terminate_cnt){
                reboot_flag = 1;
            }
        }
        
        if((!custom_blt_ota_terminate_flag)
         &&((uint32_t)(clock_time() - custom_blt_ota_finished_time) > 2000*1000 * sys_tick_per_us)){
            custom_blt_ota_terminate_flag = 1;    // for ios: no last read command
        }
        
        if(((uint32_t)(clock_time() - custom_blt_ota_finished_time) > 4000*1000 * sys_tick_per_us)){
            reboot_flag = 1;
        }
        
        if(reboot_flag){
            custom_rf_link_slave_ota_finish_led_and_reboot(custom_blt_ota_finished_flag);
            // have been reboot
        }
    }
}