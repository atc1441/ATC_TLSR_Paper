#ifndef MIBLE_ARCH_H__
#define MIBLE_ARCH_H__

// Copyright [2017] [Beijing Xiaomi Mobile Software Co., Ltd]
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mible_port.h"
#include "mible_type.h"
#include "telink_sdk_mible_api.h"
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

void mible_gap_event_callback(mible_gap_evt_t evt, mible_gap_evt_param_t* param);

void mible_gatts_event_callback(mible_gatts_evt_t evt,
		mible_gatts_evt_param_t* param);

void mible_gattc_event_callback(mible_gattc_evt_t evt,
		mible_gattc_evt_param_t* param);

void mible_arch_event_callback(mible_arch_event_t evt,
		mible_arch_evt_param_t* param);

mible_status_t mible_gap_address_get(mible_addr_t mac);

mible_status_t mible_gap_scan_start(mible_gap_scan_type_t scan_type,
		mible_gap_scan_param_t scan_param);

mible_status_t mible_gap_scan_stop(void);

mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_adv_param);

mible_status_t mible_gap_adv_data_set(uint8_t const * p_data,
        uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen);
mible_status_t mible_gap_adv_stop(void);

mible_status_t mible_gap_connect(mible_gap_scan_param_t scan_param,
		mible_gap_connect_t conn_param);

mible_status_t mible_gap_update_conn_params(uint16_t conn_handle,
		mible_gap_conn_param_t conn_params);

mible_status_t mible_gap_disconnect(uint16_t conn_handle);

mible_status_t mible_gatts_service_init(
		mible_gatts_db_t *mible_service_database);

mible_status_t mible_gatts_value_set(uint16_t srv_handle, uint16_t char_handle,
		uint8_t offset, uint8_t* buf, uint8_t len);

mible_status_t mible_gatts_value_get(uint16_t srv_handle, uint16_t char_handle,
		uint8_t* pdata, uint8_t *plen);

mible_status_t mible_gatts_notify_or_indicate(uint16_t conn_handle,
		uint16_t srv_handle, uint16_t char_value_handle, uint8_t offset,
		uint8_t* p_value, uint8_t len, uint8_t type);

mible_status_t mible_gatts_rw_auth_reply(uint16_t conn_handle, uint8_t status,
		uint16_t char_value_handle, uint8_t offset, uint8_t* p_value,
		uint8_t len, uint8_t type);

mible_status_t mible_gattc_primary_service_discover_by_uuid(
		uint16_t conn_handle, mible_handle_range_t handle_range,
		mible_uuid_t* p_srv_uuid);

mible_status_t
mible_gattc_char_discover_by_uuid(uint16_t conn_handle,
		mible_handle_range_t handle_range, mible_uuid_t* p_char_uuid);

mible_status_t
mible_gattc_clt_cfg_descriptor_discover(uint16_t conn_handle,
		mible_handle_range_t handle_range);

mible_status_t
mible_gattc_read_char_value_by_uuid(uint16_t conn_handle,
		mible_handle_range_t handle_range, mible_uuid_t *p_char_uuid);

mible_status_t mible_gattc_write_with_rsp(uint16_t conn_handle, uint16_t handle,
		uint8_t* p_value, uint8_t len);

mible_status_t mible_gattc_write_cmd(uint16_t conn_handle, uint16_t handle,
		uint8_t* p_value, uint8_t len);

mible_status_t mible_timer_create(void** p_timer_id,
		mible_timer_handler timeout_handler, mible_timer_mode mode);

mible_status_t mible_timer_delete(void* timer_id);

mible_status_t mible_timer_start(void* timer_id, uint32_t timeout_value,
		void* p_context);

mible_status_t mible_timer_stop(void* timer_id);

mible_status_t mible_record_create(uint16_t record_id, uint8_t len);

mible_status_t mible_record_delete(uint16_t record_id);

mible_status_t mible_record_read(uint16_t record_id, uint8_t* p_data,
		uint8_t len);

mible_status_t mible_record_write(uint16_t record_id, const uint8_t* p_data,
		uint8_t len);

mible_status_t mible_rand_num_generator(uint8_t* p_buf, uint8_t len);

mible_status_t mible_aes128_encrypt(const uint8_t* key,
		const uint8_t* plaintext, uint8_t plen, uint8_t* ciphertext);

mible_status_t mible_task_post(mible_handler_t handler, void *param);

void mible_tasks_exec(void);

mible_status_t mible_iic_init(const iic_config_t * p_config, mible_handler_t handler);

void mible_iic_uninit(void);

mible_status_t mible_iic_tx(uint8_t addr, uint8_t * p_out, uint16_t len,
		bool no_stop);

mible_status_t mible_iic_rx(uint8_t addr, uint8_t * p_in, uint16_t len);

int mible_iic_scl_pin_read(uint8_t port, uint8_t pin);

int mible_gap_register(mible_gap_callback_t cb);

int mible_gattc_register(mible_gattc_callback_t cb);

int mible_gatts_register(mible_gatts_callback_t cb);

int mible_arch_register(mible_arch_callback_t cb);

int mible_gap_register(mible_gap_callback_t cb);
int mible_gattc_register(mible_gattc_callback_t cb);
int mible_gatts_register(mible_gatts_callback_t cb);
int mible_arch_register(mible_arch_callback_t cb);



#endif
