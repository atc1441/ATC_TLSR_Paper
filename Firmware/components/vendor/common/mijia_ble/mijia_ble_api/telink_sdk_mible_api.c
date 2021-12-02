#include "telink_sdk_mible_api.h"
#include "../libs/mi_config.h"
#include "../libs/standard_auth/mible_standard_auth.h"
#include "../libs/common/mible_beacon_internal.h"
#include "mible_log.h"
#include "proj_lib/ble/att.h"

MYFIFO_INIT(blt_gatt_event_buf, 0x30, 8);

telink_record_t telink_record;
// call back function ,will be call by the telink api 
u8 telink_ble_mi_app_event(u8 sub_code , u8 *p, int n)
{
	mible_gap_evt_t gap_evt_t = MIBLE_GAP_EVT_ADV_REPORT ;
	mible_gap_evt_param_t gap_para_t;
	gap_para_t.conn_handle =0;
	if(sub_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT){
		gap_evt_t = MIBLE_GAP_EVT_ADV_REPORT;
		mible_gap_adv_report_t *p_report = &(gap_para_t.report);
		event_adv_report_t *pa = (event_adv_report_t *)p;
		if(pa->adr_type){
			p_report->addr_type = MIBLE_ADDRESS_TYPE_RANDOM;
		}else{
			p_report->addr_type = MIBLE_ADDRESS_TYPE_PUBLIC;
		}
		if(pa->event_type == ADV_REPORT_EVENT_TYPE_NONCONN_IND){
			p_report->adv_type = ADV_DATA;
		}else if(pa->event_type == ADV_REPORT_EVENT_TYPE_SCAN_IND){
			p_report->adv_type = SCAN_RSP_DATA;
		}
		memcpy(p_report->peer_addr,pa->mac,sizeof(pa->mac));
		// memcpy adv data 
		p_report->data_len = pa->len;
		memcpy(p_report->data,pa->data,pa->len);
		// lost rssi;
	
	}else if(sub_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE){
		gap_evt_t = MIBLE_GAP_EVT_CONNECTED;
		mible_gap_connect_t *p_connect = &(gap_para_t.connect);
		event_connection_complete_t *pc = (event_connection_complete_t *)p;
		memcpy(p_connect->peer_addr,pc->mac,sizeof(pc->mac));
		if(pc->peer_adr_type){
			p_connect->type = MIBLE_ADDRESS_TYPE_RANDOM;
		}else{
			p_connect->type = MIBLE_ADDRESS_TYPE_PUBLIC;
		}
		if(pc->role == LL_ROLE_MASTER){
			p_connect->role = MIBLE_GAP_CENTRAL;
		}else if(pc->role == LL_ROLE_SLAVE){
			p_connect->role = MIBLE_GAP_PERIPHERAL;
		}
		p_connect->conn_param.min_conn_interval = pc->interval;
		p_connect->conn_param.max_conn_interval = pc->interval;
		p_connect->conn_param.slave_latency = pc->latency;
		p_connect->conn_param.conn_sup_timeout = pc->timeout;
	}else if (sub_code == HCI_EVT_DISCONNECTION_COMPLETE ){
		gap_evt_t =  MIBLE_GAP_EVT_DISCONNECT;
		mible_gap_disconnect_t *p_dis = &(gap_para_t.disconnect);
		event_disconnection_t *pd = (event_disconnection_t *)p;
		if(pd->reason == HCI_ERR_CONN_TIMEOUT){
			p_dis->reason = CONNECTION_TIMEOUT;
		}else if (pd->reason == HCI_ERR_REMOTE_USER_TERM_CONN){
			p_dis->reason = REMOTE_USER_TERMINATED;
		}else if (pd->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST){
			p_dis->reason = LOCAL_HOST_TERMINATED;
		}else{}
	}else if (sub_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE){
		gap_evt_t = MIBLE_GAP_EVT_CONN_PARAM_UPDATED;
		mible_gap_connect_update_t *p_up = &(gap_para_t.update_conn);
		event_connection_update_t *pu = (event_connection_update_t *)p;
		p_up->conn_param.conn_sup_timeout = pu->timeout;
		p_up->conn_param.max_conn_interval = pu->interval;
		p_up->conn_param.min_conn_interval = pu->interval;
		p_up->conn_param.slave_latency = pu->latency;
	}
	// set all the parameter part ,and call the function interface 
	mible_gap_event_callback(gap_evt_t,&gap_para_t);
	return 1;
}


// callback function ,will call by the telink part 
u8 telink_ble_mi_event_callback(u8 opcode,u8 *p)
{
	mible_gatts_evt_t evt = MIBLE_GATTS_EVT_WRITE;
	mible_gatts_evt_param_t evt_param_t;
	memset(&evt_param_t,0,sizeof(mible_gatts_evt_param_t));
	rf_packet_att_readBlob_t *p_r = (rf_packet_att_readBlob_t*)p;
	rf_packet_att_data_t *p_w = (rf_packet_att_data_t*)p;
	mible_gatts_write_t *p_write = &(evt_param_t.write);
	mible_gatts_read_t *p_read = &(evt_param_t.read);
	if(p_w->rf_len > 27){
		return 0;
	}
	u16 handle_val=0;
	extern u16 ota_att_handle;
	evt_param_t.conn_handle =0;
	if(opcode == ATT_OP_WRITE_REQ || opcode == ATT_OP_WRITE_CMD ){
		u8 mi_write_buf[64];
		handle_val = (p_w->hh<<8) + p_w->hl;
		evt = MIBLE_GATTS_EVT_WRITE;
		if(ota_att_handle == handle_val){
			return 0;
		}
		p_write->value_handle = handle_val;
		p_write->offset = 0;// the offset of the buffer part 
		p_write->len = (p_w->l2cap)-3;
		p_write->data = mi_write_buf;
		memcpy((u8 *)(p_write->data+(p_write->offset)),p_w->dat,p_write->len);
	}else if (opcode == ATT_OP_READ_REQ){
		handle_val = (p_r->handle1 <<8)+ p_r->handle;
		if(ota_att_handle == handle_val){
			return 0;
		}
		if(*(gAttributes[handle_val].p_perm) & ATT_PERMISSIONS_READ){
			evt = MIBLE_GATTS_EVT_READ_PERMIT_REQ;
		}
		p_read->value_handle = handle_val;
	}else if (opcode == ATT_OP_HANDLE_VALUE_CFM){
		evt = MIBLE_GATTS_EVT_IND_CONFIRM;
	}else{
		return 0;
	}
	mible_gatts_event_callback(evt,&evt_param_t);	
	return 1;
}
int telink_ble_mi_event_cb_att(u16 conn, u8 * p)
{
	if(blcOta.ota_start_flag){
		return TRUE;	
	}
	rf_packet_l2cap_req_t * req = (rf_packet_l2cap_req_t *)p;
	u8 opcode = req->opcode;
	conn++;
	telink_ble_mi_event_callback(opcode,p);
	return TRUE;
}

// callback function ,and will call by the telink part 
u8 telink_ble_mi_att_sync(u8 att_op,mible_gatts_srv_db_t *p)
{
	mible_arch_event_t evt = MIBLE_GATTS_EVT_WRITE;
	mible_arch_evt_param_t evt_param_t;
	mible_gatts_srv_db_t *p_gatts_db_ini = (evt_param_t.srv_init_cmp.p_gatts_db->p_srv_db);
	if(att_op == 0){
		evt_param_t.srv_init_cmp.status = MI_SUCCESS;
		evt_param_t.srv_init_cmp.p_gatts_db->srv_num =1;
		memcpy((u8 *)p_gatts_db_ini , (u8 *)p,sizeof(mible_gatts_srv_db_t));
		evt = MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP;
	}else if (att_op == 1){
		evt_param_t.record.status = MI_SUCCESS;
		evt_param_t.record.id = 1;
		evt = MIBLE_ARCH_EVT_RECORD_WRITE;
	}else{
		return 0;
	}
	mible_arch_event_callback(evt, &evt_param_t);
	return 1;
}

// user function ,and will call by the mi part 
extern u8	tbl_mac [6];
mible_status_t telink_ble_mi_get_gap_address(u8 *p_mac)
{
	memcpy(p_mac,tbl_mac,sizeof(tbl_mac));	
	return MI_SUCCESS;
}
// user function ,and will call by the mi part
u8 gap_adv_param_valid(mible_gap_adv_param_t *p_param)
{
	u8 valid =1;
	if(p_param->adv_interval_min > p_param->adv_interval_max){
		valid =0;
	}else if(p_param->adv_interval_min<20 || p_param->adv_interval_min > 0x4000){
		valid =0;
	}else if(p_param->adv_interval_max<20 || p_param->adv_interval_max > 0x4000){
		valid =0;
	}else if(p_param->adv_type < MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED || 
		p_param->adv_type > MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED	){
		valid =0;
	}
	return valid;
}
mible_status_t telink_ble_mi_adv_start(mible_gap_adv_param_t *p_param)
{
	if(blt_state == BLS_LINK_STATE_CONN){
		return MI_ERR_INVALID_STATE;
	}else if (!gap_adv_param_valid(p_param)){
		return MI_ERR_INVALID_PARAM;
	}
	u8 status =0;
	u8 chn_mask=0;
	if(p_param->ch_mask.ch_37_off){
		chn_mask &=~(BIT(0));
	}else{
		chn_mask |= BIT(0);
	}
	if(p_param->ch_mask.ch_38_off){
		chn_mask &=~(BIT(1));
	}else{
		chn_mask |= BIT(1);
	}
	if(p_param->ch_mask.ch_39_off){
		chn_mask &=~(BIT(2));
	}else{
		chn_mask |= BIT(2);
	}
	status = bls_ll_setAdvParam( p_param->adv_interval_min, p_param->adv_interval_max, \
			 	 	 	 	 	     p_param->adv_type, OWN_ADDRESS_PUBLIC, \
			 	 	 	 	 	     0,  NULL,  chn_mask, ADV_FP_NONE);
	bls_ll_setAdvEnable(1);  //adv enable
	return MI_SUCCESS;
}
// user function ,and will call by the mi part 
rf_packet_adv_t mi_adv_packet;
void set_adv_mi_certify(rf_packet_adv_t * p,uint8_t const * p_data,
        uint8_t dlen) 
{
	p->header.type = LL_TYPE_ADV_IND;
	memcpy(p->advA,tbl_mac,6);
	memcpy(p->data, p_data, dlen);
	p->rf_len = 6 + dlen;
	p->dma_len = p->rf_len + 2;	
	return ;
}
void set_adv_mi_prehandler(rf_packet_adv_t *p)
{
	memcpy(p,&mi_adv_packet,sizeof(rf_packet_adv_t));
	return ;
}

mible_status_t telink_ble_mi_gap_adv_data_set(uint8_t const * p_data,
        uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen)
{
	if(blt_state == BLS_LINK_STATE_CONN){
		return MI_ERR_INVALID_STATE;
	}else if (dlen>31 ){
		return MI_ERR_INVALID_PARAM;
	}
	//bls_ll_setAdvData((u8 *)p_data,dlen);
	if(dlen>0 &&p_data!=NULL){
		set_adv_mi_certify(&mi_adv_packet,(u8*)p_data,dlen);
	}
	// set the scanrsp for the provision part ;
	if(srdlen>0 && p_sr_data!=NULL){
		bls_ll_setScanRspData((u8 *)p_sr_data,srdlen);
	}
	return MI_SUCCESS;
}
// user function ,and will call by the mi part 

mible_status_t telink_ble_mi_adv_stop()
{
	if(blt_state == BLS_LINK_STATE_CONN){
		return MI_ERR_INVALID_STATE;
	}
	bls_ll_setAdvEnable(0);
	return MI_SUCCESS;
}

u8 mi_update_conn_params_valid(mible_gap_conn_param_t conn_params)
{
	u8 valid =1;
	if(conn_params.max_conn_interval < conn_params.min_conn_interval){
		valid =0;
	}else if(conn_params.max_conn_interval > 0x0C80 || conn_params.max_conn_interval < 0x0006){
		valid =0;
	}else if(conn_params.min_conn_interval > 0x0C80 || conn_params.min_conn_interval < 0x0006){
		valid =0;
	}else if(conn_params.slave_latency > 0x01F3 || conn_params.slave_latency < 0){
		valid =0;
	}else if(conn_params.conn_sup_timeout > 0x0C80 || conn_params.conn_sup_timeout < 0x000A){
		valid =0;
	}
	return valid;
}

// user function ,and will call by the mi part 
mible_status_t telink_ble_mi_update_conn_params(uint16_t conn_handle,mible_gap_conn_param_t conn_params)
{
	if(blt_state != BLS_LINK_STATE_CONN){
		return MI_ERR_INVALID_STATE;
	}else if(!mi_update_conn_params_valid(conn_params)){
		return MI_ERR_INVALID_PARAM;
	}
	conn_handle =0;
	bls_l2cap_requestConnParamUpdate(conn_params.min_conn_interval,conn_params.max_conn_interval,
										conn_params.slave_latency,conn_params.conn_sup_timeout);
	return MI_SUCCESS;
}

// user function ,and will call by the mi api
u16 find_handle_by_uuid_char(u8 mode,u8 *p_uuid ,attribute_t *p_att)
{
	
	for(int i=0;i<p_att->attNum;i++){
		if(mode == MIBLE_UUID_16){
			if(!memcmp(p_uuid,p_att[i].uuid,2) && p_att[i].uuidLen == 2){
				return i;
			}
		}else if(mode == MIBLE_UUID_128){
			if(!memcmp(p_uuid,p_att[i].uuid,16) && p_att[i].uuidLen == 16){
				return i;
			}
		}
	}
	return 0;
}

u16 find_handle_by_uuid_service(u8 mode,u8 *p_uuid ,attribute_t *p_att)
{
	
	for(int i=0;i<p_att->attNum;i++){
		if(mode == MIBLE_UUID_16){
			if(!memcmp(p_uuid,p_att[i].pAttrValue,2) && p_att[i].attrLen == 2){
				return i;
			}
		}else if(mode == MIBLE_UUID_128){
			if(!memcmp(p_uuid,p_att[i].pAttrValue,16) && p_att[i].attrLen == 16){
				return i;
			}
		}
	}
	return 0;
}

mible_status_t telink_ble_mi_gatts_service_init(mible_gatts_db_t *p_server_db)
{
	
	mible_gatts_srv_db_t *p_srv = p_server_db->p_srv_db ;
	mible_gatts_char_db_t *p_char_db = p_srv->p_char_db ;
	mible_uuid_t *p_uuid_srv_str = &(p_srv->srv_uuid);
	mible_uuid_t *p_uuid_char_str = &(p_char_db->char_uuid);
	u16 handle;
	if(p_srv == NULL || p_char_db == NULL || p_uuid_srv_str == NULL){
		return MI_ERR_INVALID_ADDR;
	}
	extern attribute_t my_Attributes[];
	handle = find_handle_by_uuid_service(p_uuid_srv_str->type,p_uuid_srv_str->uuid128,my_Attributes);
	if(handle == 0){
		return MI_ERR_INVALID_PARAM;
	}
	p_srv->srv_handle = handle;
	for(u8 i=0;i<(p_srv->char_num);i++){
		p_uuid_char_str = &(p_char_db[i].char_uuid);
		handle = find_handle_by_uuid_char(p_uuid_char_str->type,p_uuid_char_str->uuid128,my_Attributes);
		p_char_db[i].char_value_handle = handle;
		// set the permit part 
		if(p_char_db[i].wr_author || p_char_db[i].rd_author){
			*(my_Attributes[handle].p_perm) |=(ATT_PERMISSIONS_AUTHOR);
		}else{
			*(my_Attributes[handle].p_perm) &= (~ATT_PERMISSIONS_AUTHOR);
		}
	}
	return MI_SUCCESS;

}

// user function ,and wil call by the mi api
u8 get_uuid_is_character_or_not(u8 *p)
{
	u16 uuid=0;
	uuid = (p[1]<<8)+p[0];
	if(uuid == GATT_UUID_CHARACTER){
		return TRUE;
	}else{
		return FALSE;
	}
}

mible_status_t telink_ble_mi_gatts_value_set(uint16_t srv_handle, uint16_t value_handle,
    uint8_t offset, uint8_t* p_value,
    uint8_t len)
{
	attribute_t * p_att = gAttributes + value_handle;
	attribute_t * p_att_char = gAttributes + value_handle-1;
	srv_handle =0;
	if(p_value == NULL){
		return MI_ERR_INVALID_ADDR;
	}else if (len ==0 ||len > p_att->attrLen){
		return MI_ERR_INVALID_LENGTH;
	}else if (value_handle > gAttributes->attNum ){
		return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;
	}else if (!get_uuid_is_character_or_not(p_att_char->uuid)){
		return MIBLE_ERR_GATT_INVALID_ATT_TYPE;
	}
	memcpy(p_att->pAttrValue,p_value,len);
	return MI_SUCCESS;
}

// user function ,and will call by the mi api 

mible_status_t telink_ble_mi_gatts_value_get(uint16_t srv_handle, uint16_t value_handle,
    uint8_t* p_value, uint8_t *p_len)
{
	attribute_t * p_att = gAttributes + value_handle;
	attribute_t * p_att_char = gAttributes + value_handle-1;
	srv_handle =0;
	if(p_value == NULL || p_value == NULL){
		return MI_ERR_INVALID_ADDR;
	}else if (value_handle > gAttributes->attNum ){
		return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;
	}else if (!get_uuid_is_character_or_not(p_att_char->uuid)){
		return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;
	}
	*p_len = p_att->attrLen;
	memcpy(p_value,p_att->pAttrValue,*p_len);
	return MI_SUCCESS;
}

// user function ,and will call by the mi api 

mible_status_t telink_ble_mi_gatts_notify_or_indicate(uint16_t conn_handle, uint16_t srv_handle,
    uint16_t char_value_handle, uint8_t offset, uint8_t* p_value,
    uint8_t len, uint8_t type)
{
	u8 notify_ret =0;
	conn_handle =0;
	srv_handle =0;
	offset =0;
	attribute_t * p_att_char = gAttributes + char_value_handle-1;
	offset =0;//??
	if(p_value == NULL){
		return MI_ERR_INVALID_ADDR;
	}else if (blt_state != BLS_LINK_STATE_CONN){
		return MI_ERR_INVALID_STATE;
	}else if (len ==0){
		return MI_ERR_INVALID_LENGTH;
	}else if (char_value_handle > gAttributes->attNum ){
		return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;
	}else if (!get_uuid_is_character_or_not(p_att_char->uuid)){
		return MIBLE_ERR_GATT_INVALID_ATT_TYPE;
	}
	if(type == NOTIFY_CMD_CODE){// notification 
		if(blc_ll_getTxFifoNumber()+ 1 >= (blt_txfifo.num - 2 - 3 )){
			return MI_ERR_BUSY;
		}
		notify_ret = bls_att_pushNotifyData(char_value_handle,p_value,len);
	}else if (type ==INDICATION_CMD_CODE ){
		notify_ret = bls_att_pushIndicateData(char_value_handle,p_value,len);		
	}
	if(notify_ret == BLE_SUCCESS){
		return MI_SUCCESS;
	}else{
		return MI_ERR_BUSY;
	}
	
}

ev_time_event_t ev_evnt[EV_TIMER_MAX_CNT];

ev_time_event_t * find_empty_ev_event()
{
	ev_time_event_t *p_ev = NULL;
	for(int i=0;i<EV_TIMER_MAX_CNT;i++){
		if(ev_evnt[i].cb==NULL){
			p_ev = &(ev_evnt[i]);
			return p_ev;
		}
	}
	return p_ev;
}

ev_time_event_t * find_ev_event_by_cb(mible_timer_handler timeout_handler)
{
	ev_time_event_t *p_ev = NULL;
	for(int i=0;i<EV_TIMER_MAX_CNT;i++){
		if(ev_evnt[i].cb==timeout_handler){
			p_ev = &(ev_evnt[i]);
			return p_ev;
		}
	}
	return p_ev;
}

u8 find_timer_id_pointer(void *p_timer_id)
{
	for(int i=0;i<EV_TIMER_MAX_CNT;i++){
		if(&ev_evnt[i]== p_timer_id){
			return TRUE;
		}
	}
	return FALSE;
}

mible_status_t telink_mi_timer_create(void** p_timer_id,
    mible_timer_handler timeout_handler,
    mible_timer_mode mode)
{
	ev_time_event_t *p_ev = NULL;
	p_ev = find_ev_event_by_cb(timeout_handler);
	if(p_ev){// alredy exist 
		p_ev->interval = 0;
		p_ev->mode = mode;
		*p_timer_id = p_ev;
		return MI_SUCCESS;
	}
	p_ev = find_empty_ev_event();
	if(p_ev != NULL){
			p_ev->cb = timeout_handler;
			p_ev->interval = 0;
			p_ev->mode = mode;
			*p_timer_id = p_ev;
			return MI_SUCCESS;
	}else{
			return MI_ERR_NO_MEM;
		}
}

mible_status_t telink_mi_timer_delete(void *timer_id)
{
	// find the pointer , clear the content part first 
	if(find_timer_id_pointer(timer_id) != TRUE){
		return MI_ERR_INVALID_PARAM;
	}
	ev_unon_timer((ev_time_event_t *) timer_id);
	memset((u8 *)timer_id ,0,sizeof(ev_time_event_t));
	return MI_SUCCESS;
}

mible_status_t telink_mi_timer_start(void* timer_id, uint32_t timeout_value,
    void* p_context)
{
	// find the pointer , clear the content part first 
	if(find_timer_id_pointer(timer_id) != TRUE){
		return MI_ERR_INVALID_PARAM;
	}
	ev_time_event_t *p_ev = (ev_time_event_t *)timer_id;
	p_ev->data = p_context;
	ev_on_timer(p_ev,timeout_value);
	return MI_SUCCESS;
}

mible_status_t telink_mi_timer_stop(void* timer_id)
{
	// delete from the list table 
	if(find_timer_id_pointer(timer_id) != TRUE){
		return MI_ERR_INVALID_PARAM;
	}
	ev_unon_timer((ev_time_event_t *) timer_id);
	return MI_SUCCESS;
}

// record dispatch part 
u32 flash_idx_adr = FLASH_ADR_MI_RECORD;
u32 flash_idx_adr_cpy = FLASH_ADR_MI_RECORD_TMP;
void telink_record_part_init()
{
	
	uint8_t *p_buf = (u8 *)(&telink_record);
	u32 record_adr = FLASH_ADR_MI_RECORD;
	while(1){
		flash_read_page(record_adr,sizeof(telink_record_t),p_buf);
		if(telink_record.rec_id == 0xffff || record_adr >= FLASH_ADR_MI_RECORD_TMP){
			flash_idx_adr = record_adr;
			break;
		}
		record_adr += telink_record.len +3 ;
	}
}
u8 telink_write_flash(u32 *p_adr,u8 *p_buf,u8 len )
{
	u32 tmp_adr = *p_adr;
	flash_write_page(tmp_adr,len,p_buf);
	tmp_adr += len;
	*p_adr = tmp_adr;
	return TRUE;
}

u8 telink_record_clean_cpy()
{
	u32 record_adr = FLASH_ADR_MI_RECORD;
	u32 record_adr_cpy = FLASH_ADR_MI_RECORD_TMP;
	u32 cpy_cnt =0;
	uint8_t *p_buf = (u8 *)(&telink_record);
	flash_erase_sector(FLASH_ADR_MI_RECORD_TMP);
	while(1){//cpy the record to tmp first 
		flash_read_page(record_adr,sizeof(telink_record_t),p_buf);
		if(telink_record.rec_id == 0xffff || record_adr >= FLASH_ADR_MI_RECORD_MAX){
			break;
		}else if(telink_record.rec_id == 0){
			record_adr += telink_record.len +3 ;
		}else{
			// need to cpy
			u8 total_len =0;
			total_len = telink_record.len+3;
			while(total_len){
				if(total_len > sizeof(telink_record_t)){
					telink_write_flash(&record_adr_cpy,p_buf,sizeof(telink_record_t));
					total_len -= sizeof(telink_record_t);
					record_adr += sizeof(telink_record_t);
					flash_read_page(record_adr,sizeof(telink_record_t),p_buf);
				}else{
					telink_write_flash(&record_adr_cpy,p_buf,total_len);
					record_adr += total_len;
					total_len =0;
				}
			}
		}
	}
	flash_erase_sector(FLASH_ADR_MI_RECORD);
	cpy_cnt = record_adr_cpy - FLASH_ADR_MI_RECORD_TMP;
	u32 idx =0;
	flash_idx_adr = FLASH_ADR_MI_RECORD + cpy_cnt;// update the write adr part 
	while(cpy_cnt){
		if(cpy_cnt > sizeof(telink_record_t)){
			flash_read_page(FLASH_ADR_MI_RECORD_TMP+idx,sizeof(telink_record_t),p_buf);
			flash_write_page(FLASH_ADR_MI_RECORD+idx,sizeof(telink_record_t),p_buf);
			idx += sizeof(telink_record_t);
			cpy_cnt -= sizeof(telink_record_t);
		}else{
			flash_read_page(FLASH_ADR_MI_RECORD_TMP+idx,cpy_cnt,p_buf);
			flash_write_page(FLASH_ADR_MI_RECORD+idx,cpy_cnt,p_buf);
			cpy_cnt = 0;
		}
	}	
	return TRUE;
}

mible_status_t telink_record_create(uint16_t record_id, uint8_t len)
{
/*
	uint8_t total_len = len;
	uint8_t buf_idx =0;
	uint8_t *p_buf = (u8 *)(&telink_record);
	if(len > RECORD_MAX_LEN || len == 0){
		return MI_ERR_INVALID_LENGTH;
	}
	mible_status_t err_sts = MI_SUCCESS;
	// if find the record part ,clear the id part ,else return mi_suc;
	u32 record_adr =0;
	if(flash_idx_adr + (len+3)+RECORD_RESERVE_SPACE > FLASH_ADR_MI_RECORD_TMP){
		// need to clean the flash first .
		telink_record_clean_cpy();
		err_sts = MI_ERR_NO_MEM;
	}
	if(find_record_adr(record_id,&record_adr) == TRUE){
		return MI_SUCCESS;
	}else{
		telink_record_delete(record_id);
	}
	memset(p_buf,0,sizeof(telink_record_t));
	telink_record.rec_id = record_id;
	telink_record.len = total_len;
	
	//write the header part 
	if(total_len > sizeof(telink_record.dat)){
		telink_write_flash(&flash_idx_adr,p_buf,sizeof(telink_record_t));
		total_len -= sizeof(telink_record.dat);
		buf_idx += sizeof(telink_record.dat);
	}else{
		telink_write_flash(&flash_idx_adr,p_buf,total_len+3);
		total_len = 0;
	}
	//the continus packet 
	while(total_len >0){
		memset(p_buf,0,sizeof(telink_record_t));
		if(total_len>sizeof(telink_record_t)){
			telink_write_flash(&flash_idx_adr,p_buf,sizeof(telink_record_t));
			total_len -= sizeof(telink_record_t);
			buf_idx += sizeof(telink_record_t);
		}else{
			telink_write_flash(&flash_idx_adr,p_buf,total_len);
			total_len =0;
			buf_idx =0;
		}
	}
	return err_sts;
*/
	return MI_SUCCESS;
}


mible_status_t telink_record_delete(uint16_t record_id)
{
	u32 record_adr =0;
	uint8_t *p_buf = (u8 *)(&telink_record);
	if(!find_record_adr(record_id,&record_adr)){
		telink_record_eve_cb(record_id,MI_ERR_INVALID_PARAM,MIBLE_ARCH_EVT_RECORD_DELETE);
		return MI_ERR_INVALID_PARAM;
	}
	//flash_read_page(record_adr,sizeof(telink_record_t),p_buf);
	telink_record.rec_id =0;
	// set the record id to 0 
	flash_write_page(record_adr,2,p_buf);//just clear the rec id part 
	telink_record_eve_cb(record_id,MI_SUCCESS,MIBLE_ARCH_EVT_RECORD_DELETE);
	return MI_SUCCESS;
}
uint8_t buf_is_empty_or_not(uint8_t* p_data,uint8_t len)
{
	for(int i=0;i<len;i++){
		if(p_data[i] != 0){
			return 0;
		}
	}
	return 1;
}
mible_status_t telink_record_read(uint16_t record_id, uint8_t* p_data,uint8_t len)
{
	u32 record_adr =0;
	uint8_t *p_buf = (u8 *)(&telink_record);
	if(len > RECORD_MAX_LEN ||len ==0  ){
		return  MI_ERR_INVALID_LENGTH;
	}else if(p_data == NULL){
		return MI_ERR_INVALID_ADDR;
	}else if(!find_record_adr(record_id,&record_adr)){
		return MI_ERR_INVALID_PARAM;
	}
	flash_read_page(record_adr,sizeof(telink_record_t),p_buf); 
	if(len>telink_record.len){
		return MI_ERR_INVALID_LENGTH;
	}
	
	if(len<=sizeof(telink_record.dat)){
		memcpy(p_data,telink_record.dat,len);
		/*
		if(buf_is_empty_or_not(p_data,len)){
			return MI_ERR_INVALID_PARAM;
		}*/
		return MI_SUCCESS;
	}else{
		// directly read all the buf part 
		flash_read_page(record_adr+3,len ,p_data);
		// should dispatch whether all the data is zero ,if it's means it have not write already
		/*
		if(buf_is_empty_or_not(p_data,len)){
			return MI_ERR_INVALID_PARAM;
		}*/
		return MI_SUCCESS;
	}
}
void telink_record_eve_cb(uint16_t record_id,mible_status_t sts,mible_arch_event_t eve)
{
	mible_arch_evt_param_t param;
	param.record.status =sts;
	param.record.id = record_id;
	mible_arch_event_callback(eve, &param);
	return ;
}
mible_status_t telink_record_write(uint16_t record_id, uint8_t* p_data,uint8_t len)
{
	uint8_t total_len = len;
	uint8_t buf_idx =0;
	uint8_t *p_buf = (u8 *)(&telink_record);
	if(len > RECORD_MAX_LEN || len == 0){
		telink_record_eve_cb(record_id,MI_ERR_INVALID_LENGTH,MIBLE_ARCH_EVT_RECORD_WRITE);
		return MI_ERR_INVALID_LENGTH;
	}else if((u32)p_data%4 != 0){
		telink_record_eve_cb(record_id,MI_ERR_INVALID_PARAM,MIBLE_ARCH_EVT_RECORD_WRITE);
		return MI_ERR_INVALID_PARAM;
	}
	if(flash_idx_adr + (len+3)+RECORD_RESERVE_SPACE > FLASH_ADR_MI_RECORD_TMP){
		// need to clean the flash first .
		telink_record_clean_cpy();
		
	}
	// check the first record 
	telink_record_delete(record_id);
	// write part 
	memset(p_buf,0,sizeof(telink_record_t));
	telink_record.rec_id = record_id;
	telink_record.len = total_len;
	// write the header part 
	if(total_len > sizeof(telink_record.dat)){
		memcpy(telink_record.dat,p_data,sizeof(telink_record.dat));
		telink_write_flash(&flash_idx_adr,p_buf,sizeof(telink_record_t));
		total_len -= sizeof(telink_record.dat);
		buf_idx += sizeof(telink_record.dat);
	}else{
		memcpy(telink_record.dat,p_data,total_len);
		telink_write_flash(&flash_idx_adr,p_buf,total_len+3);
		total_len = 0;
	}
	// write the continus part 
	while(total_len >0){
		memset(p_buf,0,sizeof(telink_record_t));
		if(total_len>sizeof(telink_record_t)){
			memcpy(p_buf,p_data+buf_idx,sizeof(telink_record_t));
			telink_write_flash(&flash_idx_adr,p_buf,sizeof(telink_record_t));
			total_len -= sizeof(telink_record_t);
			buf_idx += sizeof(telink_record_t);
		}else{
			memcpy(p_buf,p_data+buf_idx,total_len);
			telink_write_flash(&flash_idx_adr,p_buf,total_len);
			total_len =0;
			buf_idx =0;
		}
	}
	telink_record_eve_cb(record_id,MI_SUCCESS,MIBLE_ARCH_EVT_RECORD_WRITE);
	return MI_SUCCESS;
}

uint8_t find_record_adr(uint16_t record_id,u32 *p_adr)
{
	uint32_t record_adr = FLASH_ADR_MI_RECORD;
	uint8_t *p_buf = (u8 *)(&telink_record);
	while(1){
		flash_read_page(record_adr,sizeof(telink_record_t),p_buf);
		if(telink_record.rec_id == 0xffff || record_adr >= FLASH_ADR_MI_RECORD_TMP){
			return FALSE;
		}
		if(telink_record.rec_id == record_id){
			*p_adr = record_adr;
			return TRUE;
		}else{
			record_adr += telink_record.len + 3 ;
		}
	}
	return FALSE;
}

// user function ,and will call by the mi part 
mible_status_t telink_ble_mi_rand_num_generator(uint8_t* p_buf, uint8_t len)
{
	if(len > RAND_NUM_MAX_LEN){
		return MI_ERR_NO_MEM;
	}
	for(int i=0;i<len;i++){
		p_buf[i]=rand()&0xff;
	}
	return MI_SUCCESS;
}

// user function ,and will call by the mi part 
mible_status_t telink_ble_mi_aes128_encrypt(const uint8_t* key,
    const uint8_t* plaintext, uint8_t plen,
    uint8_t* ciphertext)
{
	if(key==NULL || plaintext ==NULL || ciphertext== NULL){
		return MI_ERR_INVALID_ADDR;
	}
	#if 0
	int plen_aes =0;
	plen_aes = plen;
	tn_aes_cmac ( (u8 *)key, (u8 *)plaintext, plen_aes,ciphertext );
	#else
	// raw data aes part 
	u8 text_dat[16];
	memset(text_dat,0,sizeof(text_dat));
	memcpy(text_dat,plaintext,plen);
	tn_aes_128((u8*)key, text_dat, ciphertext);
	#endif 
	return MI_SUCCESS;
}


// task poll part 
MYFIFO_INIT(mi_task_api_func_t, 16, 8);
/*
 * @brief   Post a task to a task quene, which can be executed in a right place(maybe a task in RTOS or while(1) in the main function).
 * @param   [in] handler: a pointer to function 
 *          [in] param: function parameters 
 * @return  MI_SUCCESS              Successfully put the handler to quene.
 *          MI_ERR_NO_MEM           The task quene is full. 
 *          MI_ERR_INVALID_PARAM    Handler is NULL
 * */

mible_status_t telink_mi_task_post(mible_handler_t handler, void *arg)
{
	mi_task_fun_t task_fun;
	task_fun.cb = handler;
	task_fun.arg = arg;
	if(handler == NULL){
		return MI_ERR_INVALID_PARAM;
	}
	if(my_fifo_push(&mi_task_api_func_t,(u8*)(&task_fun),sizeof(mi_task_fun_t))==0){
		return MI_ERR_NO_MEM;
	}else{
		return MI_SUCCESS;
	}
}
mible_status_t telink_mi_task_exec()
{
	// pull the function pointer part 
	mi_task_fun_t task_fun;
	u8 *p_func;
	p_func = my_fifo_get(&mi_task_api_func_t);
	if(p_func!=NULL){
		my_fifo_pop(&mi_task_api_func_t);
		memcpy((u8 *)(&task_fun),p_func+2,sizeof(mi_task_fun_t));
		task_fun.cb(task_fun.arg);
	}
	return MI_SUCCESS;
}

u8 function_para[4]={1,2,3,4};

void telink_timer_test0(u8 *p)
{
	static u8 A_dbg_timer_buf[4];
	static u32 A_dbg_timer_cnt =0;
	A_dbg_timer_cnt ++;
	memcpy(A_dbg_timer_buf,p,sizeof(A_dbg_timer_buf));

}
void test_mi_func_op (u8* arg)
{
	static u8 A_dbg_test_para[4];
	memcpy(A_dbg_test_para,arg,4);
}
u32 tick_start =0;
ev_time_event_t ev_tmp;
ev_time_event_t *p_ev_tmp = &ev_tmp;
ev_time_event_t **p_ev = &p_ev_tmp;
//u8 mi_beacon_event_buf[4]={0};
static u32 mi_event_min = 0 ;
static u8 battery_inter = 59;
void mi_event_beacon_send()
{
	// this code is just for test part .
	if (get_mi_reg_stat()){
	 #if (MI_USER_MODE == MI_HUMI_TEMP_MODE)
		if(mi_event_min % 11 == 0){
			static u16 mi_temp = 200;
			mi_temp+=10;
			if(mi_temp>600){
				mi_temp =200;
			}
			mibeacon_obj_enque(MI_STA_TEMPERATURE, sizeof(mi_temp), &mi_temp,0);
		}
		if((mi_event_min+1)%11==0){
			static u16 mi_humi = 200;
			mi_humi+=10;
			if(mi_humi>600){
				mi_humi =200;	
			}
			mibeacon_obj_enque(MI_STA_HUMIDITY, sizeof(mi_humi), &mi_humi,0);
		}
	 #endif
	}
}

void mi_event_beacon_proc()
{
	static u32 mi_event_beacon_tick =0;

	if(clock_time_exceed(mi_event_beacon_tick,60*1000*1000)){// 1 minute
		mi_event_beacon_tick += 60 * CLOCK_SYS_CLOCK_1S;
		mi_event_min++;
		mi_event_beacon_send();	
		if(mi_event_min%battery_inter == 0){// send power every 59 min,to avoid the battery check 
			static u8  battery = 50;
			battery = battery < 100 ? battery + 1 : 0;
			mibeacon_obj_enque(MI_STA_BATTERY, sizeof(battery), &battery,0);
		}
	}
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(bool solicite_bind)
{
    MI_LOG_INFO("advertising init...\n");
    mibeacon_frame_ctrl_t frame_ctrl = {
        .auth_mode      = 2,
        .version        = 5,
        .solicite       = solicite_bind,
    };

    mibeacon_capability_t cap = {.bondAbility = 1};
    mibeacon_cap_sub_io_t IO = {.in_digits = 1};
    mible_addr_t dev_mac;
    mible_gap_address_get(dev_mac);

    mibeacon_config_t mibeacon_cfg = {
        .frame_ctrl = frame_ctrl,
        .pid = PRODUCT_ID,
        .p_mac = (mible_addr_t*)dev_mac, 
        .p_capability = &cap,
        .p_cap_sub_IO = &IO,
        .p_obj = NULL,
    };

    uint8_t adv_data[31];
    uint8_t adv_len;

    // ADV Struct: Flags: LE General Discoverable Mode + BR/EDR Not supported.
    adv_data[0] = 0x02;
    adv_data[1] = 0x01;
    adv_data[2] = 0x06;
    adv_len     = 3;
    
    uint8_t service_data_len;
    if(MI_SUCCESS != mible_service_data_set(&mibeacon_cfg, adv_data+3, &service_data_len)){
        MI_LOG_ERROR("encode service data failed. \r\n");
        return;
    }
    adv_len += service_data_len;
    
    MI_LOG_HEXDUMP(adv_data, adv_len);

    mible_gap_adv_data_set(adv_data, adv_len, adv_data, 0);

    return;
}

/**@brief Function for starting advertising.
 */
 
void advertising_start(void){
    mible_gap_adv_param_t adv_param =(mible_gap_adv_param_t){
        .adv_type = MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED,
        //.adv_interval_min = 100*1000/625,
        //.adv_interval_max = 200*1000/625,
        .adv_interval_min = 20*1000/625,
        .adv_interval_max = 30*1000/625,
    };
	adv_param.ch_mask.ch_37_off =0;
	adv_param.ch_mask.ch_38_off =0;
	adv_param.ch_mask.ch_39_off =0;
    if(MI_SUCCESS != mible_gap_adv_start(&adv_param)){
        MI_LOG_ERROR("mible_gap_adv_start failed. \r\n");
        return;
    }
}
u8 mi_ceti_busy_flag =0;
void mi_set_encrypt_busy_flag(u8 flag)
{
	mi_ceti_busy_flag = flag ;
	return;
}

u8 mi_get_encrypt_busy_flag()
{
	return mi_ceti_busy_flag;
}

void stdio_rx_handler(uint8_t* p, uint8_t l)
{
	int errno;
	/* RX plain text (It has been decrypted) */
	MI_LOG_INFO("RX raw data\n");
	MI_LOG_HEXDUMP(p, l);
	
	/* TX plain text (It will be encrypted before send out.) */
	errno = stdio_tx(p, l);
	MI_LOG_INFO("errno = %d\n", errno);
	MI_ERR_CHECK(errno);
}
static uint8_t qr_code[16] = {
0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
};
void mi_schd_event_handler(schd_evt_t *p_event)
{
    MI_LOG_INFO("USER CUSTOM CALLBACK RECV EVT ID %d\n", p_event->id);
    switch (p_event->id) {
    case SCHD_EVT_OOB_REQUEST:
        MI_LOG_INFO("App selected IO cap is 0x%04X\n", p_event->data.IO_capability);
        switch (p_event->data.IO_capability) {
        case 0x0080:
            mi_schd_oob_rsp(qr_code, 16);
            MI_LOG_INFO(MI_LOG_COLOR_GREEN "Please scan device QR code.\n");
            break;

        default:
            MI_LOG_ERROR("Selected IO cap is not supported.\n");
            mible_gap_disconnect(0);
        }
        break;

    case SCHD_EVT_KEY_DEL_SUCC:
        // device has been reset, restart adv mibeacon contains IO cap.
        advertising_init(0);
        break;

    default:
        break;
    }
}

void mi_certify_part_init()
{
	#if (MI_USER_MODE == MI_DEVELOP_MODE)
	advertising_init(0);
	#else
	advertising_init(1);
	#endif
	mi_scheduler_init(10, mi_schd_event_handler, NULL);
	mi_scheduler_start(SYS_KEY_RESTORE);
	mi_service_init();
	stdio_service_init(stdio_rx_handler);
	advertising_start();
}

u8 mi_api_loop_run()
{
	telink_mi_task_exec();
	mi_schd_process();
	//test for the beacon part 
	//mi_event_beacon_proc();
#if 0
	if(tick_start && clock_time_exceed(tick_start,1000*1000)){
		tick_start =0;
		//telink_mi_timer_delete(*p_ev);
		telink_mi_timer_stop(*p_ev);
	}
#endif 

#if 0
	static u32 A_dbg_adv_stop_en =0;
	if(A_dbg_adv_stop_en){
		A_dbg_adv_stop_en =0;
		telink_ble_mi_adv_stop();
	}
	
#endif

#if 0
	static u32 A_dbg_conn_en =0;
	mible_gap_conn_param_t conn_params;
	if(A_dbg_conn_en){
		A_dbg_conn_en =0;
		conn_params.slave_latency=0;
		conn_params.conn_sup_timeout =400;
		conn_params.min_conn_interval = 16;
		conn_params.max_conn_interval = 32;
		mible_gap_update_conn_params(0,conn_params);
	}
	
#endif 

#if 0
	static u8 A_dbg_noti_indi_en =0;
	u8 notify_value[4] ={0x11,0x22,0x33,0x44};
	if(A_dbg_noti_indi_en){
		A_dbg_noti_indi_en =0;
		telink_ble_mi_gatts_notify_or_indicate(0, 0,35, 0, notify_value,sizeof(notify_value), NOTIFY_CMD_CODE);
	}
#endif 

#if 0
	static u8 A_dbg_noti_indi_en =0;
	u8 notify_value[4] ={0x11,0x22,0x33,0x44};
	if(A_dbg_noti_indi_en){
		A_dbg_noti_indi_en =0;
		telink_ble_mi_gatts_notify_or_indicate(0, 0,35, 0, notify_value,sizeof(notify_value), INDICATION_CMD_CODE);
	}
#endif 

	return TRUE;
}


u8 test_mi_api_part()
{
#if 0
	mible_gap_adv_data_t data_test;
	u8 adv_data[3]={0x02,0x01,0x06};
	u8 adv_scan[3]={0x02,0x01,0x05};
	data_test.adv_len = sizeof(adv_data);
	data_test.scan_rsp_len = sizeof(adv_scan);
	memcpy(data_test.adv_data,adv_data,sizeof(adv_data));
	memcpy(data_test.scan_rsp_data,adv_scan,sizeof(adv_scan));
	mible_gap_adv_data_set(&data_test);
#endif
#if 0
	telink_record_create(2,20);
	static u8 tmp_buf[20];
	for(int i=0;i<sizeof(tmp_buf);i++){
		tmp_buf[i]=i;
	}
	telink_record_write(3,tmp_buf,sizeof(tmp_buf));
	telink_record_create(3,20);
	telink_record_create(4,20);
	telink_record_clean_cpy();
#endif

#if 0
	telink_mi_task_post((mible_handler_t)&test_mi_func_op,function_para );
	telink_mi_task_exec();
#endif

#if 0
	telink_mi_timer_create((void **)p_ev,(mible_timer_handler)telink_timer_test0,MIBLE_TIMER_REPEATED);
	telink_mi_timer_start(*p_ev,30,function_para);
	tick_start =clock_time()|1;
#endif 

#if 0 
	static u8 A_dbg_set[2]={0x11,0x22};
	static u8 A_dbg_get[2]={0,0};
	static u8 A_dbg_get_len = 0;
	static mible_status_t A_dbg_ret ;
	A_dbg_ret = telink_ble_mi_gatts_value_set(0,17,0,A_dbg_set,sizeof(A_dbg_set));
	telink_ble_mi_gatts_value_get(0,17,A_dbg_get,&A_dbg_get_len);
#endif 

#if 0
	u8 notify_value[4] ={0x11,0x22,0x33,0x44};
	telink_ble_mi_gatts_notify_or_indicate(0, 0,35, 0, notify_value,sizeof(notify_value), NOTIFY_CMD_CODE);
#endif 
#if 0
	static mible_gatts_db_t gatt_db_t;
	static mible_gatts_srv_db_t srv_db_t;
	static mible_gatts_char_db_t char_db_t;
	static mible_uuid_t mi_uuid ;
	mi_uuid.type = MIBLE_UUID_16;
	u16 test_mi_sec_auth_srv_uuid = 0xfe95;
	memcpy(mi_uuid.uuid128,(u8*)&test_mi_sec_auth_srv_uuid,2);
	memcpy(&srv_db_t.srv_uuid,&(mi_uuid),6);
	u16 test_mi_sec_auth_uuid = 0x0016;
	memcpy(mi_uuid.uuid128,(u8 *)&test_mi_sec_auth_uuid,2);
	memcpy(&char_db_t.char_uuid,&(mi_uuid),6);
	gatt_db_t.p_srv_db = &(srv_db_t);
	srv_db_t.p_char_db = &(char_db_t);
	telink_ble_mi_gatts_service_init(&gatt_db_t);
#endif
	return 1;
}


