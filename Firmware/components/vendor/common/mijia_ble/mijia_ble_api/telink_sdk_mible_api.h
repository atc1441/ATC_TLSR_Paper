#ifndef _TELINK_SDK_MIBLE_API_C_
#define _TELINK_SDK_MIBLE_API_C_
#include "mible_api.h"
#include "tl_common.h"
#include "stack/ble/ll/ll.h"
#include "stack/ble/blt_config.h"
#include "stack/ble/service/ble_ll_ota.h"
#include "../../ev.h"
#include "../libs/common/mible_beacon.h"
#include "../libs/mijia_profiles/mi_service_server.h"
#include "../libs/standard_auth/mible_standard_auth.h"


u8 telink_ble_mi_app_event(uint8_t sub_code , uint8_t *p, int n);
u8 telink_ble_mi_event_callback(uint8_t opcode,uint8_t *p);
int telink_ble_mi_event_cb_att(u16 conn, u8 * p);
u8 telink_ble_mi_att_sync(uint8_t att_op,mible_gatts_srv_db_t *p);

u8 telink_write_flash(u32 *p_adr,u8 *p_buf,u8 len );

mible_status_t telink_ble_mi_get_gap_address(uint8_t *p_mac);
mible_status_t telink_ble_mi_adv_start(mible_gap_adv_param_t *p_param);
mible_status_t telink_ble_mi_gap_adv_data_set(uint8_t const * p_data,
        uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen);

mible_status_t telink_ble_mi_adv_stop();
mible_status_t telink_ble_mi_update_conn_params(uint16_t conn_handle,mible_gap_conn_param_t conn_params);
mible_status_t telink_ble_mi_gatts_service_init(mible_gatts_db_t *p_server_db);

mible_status_t telink_ble_mi_gatts_value_set(uint16_t srv_handle, uint16_t value_handle,
    uint8_t offset, uint8_t* p_value,
    uint8_t len);
mible_status_t telink_ble_mi_gatts_value_get(uint16_t srv_handle, uint16_t value_handle,
    uint8_t* p_value, uint8_t *p_len);
mible_status_t telink_ble_mi_gatts_notify_or_indicate(uint16_t conn_handle, uint16_t srv_handle,
    uint16_t char_value_handle, uint8_t offset, uint8_t* p_value,
    uint8_t len, uint8_t type);
mible_status_t telink_ble_mi_rand_num_generator(uint8_t* p_buf, uint8_t len);
mible_status_t telink_ble_mi_aes128_encrypt(const uint8_t* key,
    const uint8_t* plaintext, uint8_t plen,
    uint8_t* ciphertext);
mible_status_t telink_record_create(uint16_t record_id, uint8_t len);
mible_status_t telink_record_write(uint16_t record_id, uint8_t* p_data,uint8_t len);
u8 find_record_adr(uint16_t record_id,u32 *p_adr);
mible_status_t telink_record_read(uint16_t record_id, uint8_t* p_data,uint8_t len);
mible_status_t telink_record_delete(uint16_t record_id);

mible_status_t telink_mi_task_post(mible_handler_t handler, void *arg);
mible_status_t telink_mi_task_exec();
mible_status_t telink_mi_timer_create(void** p_timer_id,
    mible_timer_handler timeout_handler,
    mible_timer_mode mode);
mible_status_t telink_mi_timer_delete(void *timer_id);
mible_status_t telink_mi_timer_start(void* timer_id, uint32_t timeout_value,
    void* p_context);
mible_status_t telink_mi_timer_stop(void* timer_id);

void telink_record_eve_cb(uint16_t record_id,mible_status_t sts,mible_arch_event_t eve);
void mi_certify_part_init();
void set_adv_mi_prehandler(rf_packet_adv_t *p);
void advertise_init();
void telink_record_part_init();
uint8_t buf_is_empty_or_not(uint8_t* p_data,uint8_t len);
void telink_gatt_event_loop();
void mi_set_encrypt_busy_flag(u8 flag);
u8 mi_get_encrypt_busy_flag();
void mi_set_log_loop_proc();


u8 mi_api_loop_run();

u8 test_mi_api_part();
#define RECORD_RESERVE_SPACE 	16
#define EV_TIMER_MAX_CNT 	4
typedef struct{
	uint16_t rec_id;
	uint8_t len ;
	uint8_t dat[13];
}telink_record_t;
extern telink_record_t telink_record;
#define RECORD_MAX_LEN 	250
#define RAND_NUM_MAX_LEN 	RECORD_MAX_LEN
my_fifo_t mi_task_api_func_t;
typedef struct{
	mible_handler_t cb;
	void *arg;
}mi_task_fun_t;
extern attribute_t* gAttributes;
typedef enum{
	NOTIFY_CMD_CODE =1,
	INDICATION_CMD_CODE =2,
}operation_cmd_enum_t;
typedef enum {
    mi_mesh_unreg       = 0x00,
    mi_mesh_unprov      = 0x01,
    mi_mesh_unconfig    = 0x02,
    mi_mesh_avail       = 0x03
} mi_mesh_stat_t;

#endif

