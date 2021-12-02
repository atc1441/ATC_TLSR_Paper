//#include <string.h>
#include "mi_config.h"
#include "mible_type.h"
#include "mible_api.h"
#include "mible_beacon.h"
#include "mible_beacon_internal.h"
#include "common/queue.h"
#include "mijia_profiles/mi_service_server.h"
#include "cryptography/mi_crypto.h"

#undef  MI_LOG_MODULE_NAME
#define MI_LOG_MODULE_NAME "MIBEACON"
#include "mible_log.h"

#define MIBLE_MAX_ADV_LENGTH                        31
#define BLE_UUID_MI_SERVICE                         0xFE95
#define BLE_UUID_COMPANY_ID_XIAOMI                  0x038F
#define REC_ID_SEQNUM                               4

#define LO_BYTE(val) (uint8_t)(val)
#define HI_BYTE(val) (uint8_t)((val)>>8)
#define IS_POWER_OF_TWO(A) ( ((A) != 0) && ((((A) - 1) & (A)) == 0) )
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MS_TO_UNITS(TIME, RESOLUTION) (((TIME) * 1000UL) / (RESOLUTION))
#define CHECK_ADV_LEN(len)                                                     \
    do {                                                                       \
        if ((len) > MIBLE_MAX_ADV_LENGTH)                                      \
        return MI_ERR_DATA_SIZE;                                               \
    } while(0)


static uint16_t m_adv_interval_ms = 2000;
static uint8_t m_is_connected;
static void * mibeacon_timer;
static struct {
    uint8_t is_sending              :1;
    uint8_t is_valid_key            :1;
    uint8_t stop_adv_after_sent_out :1;
} flags;
static uint8_t beacon_key[16];
static queue_t mi_obj_queue;
static mibeacon_obj_t obj_buf[OBJ_QUEUE_SIZE];
static union {
    uint8_t  byte[4];
    uint32_t value;
} seqnum;
static struct {
    uint8_t  mac[6];
    uint16_t pid;
    uint8_t  cnt;
    uint8_t  ext_cnt[3];
} beacon_nonce;
static uint8_t m_user_data[31];
static uint8_t m_user_dlen;
static mible_status_t mibeacon_encode(mibeacon_config_t const * const config, 
        uint8_t *output, uint8_t *output_len);
static mible_status_t mible_manu_data_set(mibeacon_config_t const * const config,
        uint8_t *p_output, uint8_t *p_output_len);

static void event_encode(mibeacon_obj_t *p_obj, uint8_t *output)
{
    output[0] = p_obj->type;
    output[1] = p_obj->type >> 8;
    output[2] = p_obj->len;
    memcpy(output+3, p_obj->val, p_obj->len);
}

static int calc_objs_bytes(mibeacon_obj_t *p_obj, uint8_t num)
{
    uint8_t sum = 0;

    if (p_obj == NULL)
        return 0;

    for (uint8_t i = 0; i < num; i++)
        sum += 3 + p_obj[i].len;

    return sum;
}

static void mibeacon_timer_handler(void * p_context)
{
    uint32_t errno;
    mibeacon_obj_t obj   = {0};
	static u8 adv_reset_flag = 0;

    if (dequeue(&mi_obj_queue, &obj) == MI_ERR_NOT_FOUND) {
        MI_LOG_INFO("no more mibeacon obj.\n");
        flags.is_sending = false;
		if(adv_reset_flag){
			mibeacon_adv_data_set(0, 0, m_user_data, m_user_dlen);
			adv_reset_flag =0;
			if (m_is_connected || flags.stop_adv_after_sent_out) {
            	mibeacon_adv_stop();
            	MI_LOG_INFO("stop advertising.\n");
        	} else {
            	mibeacon_adv_start(m_adv_interval_ms);
        	}
		}
        
    } else {
        MI_LOG_INFO("send mibeacon obj 0x%04X\n", obj.type);
        /* encode ADV packet. */
        /* Flags: LE General Discoverable Mode + BR/EDR Not supported. */
        uint8_t adv_data[31] = {2, 1, 6};
        uint8_t adv_dlen     = 3;

        mibeacon_frame_ctrl_t fctrl = {
            .registered     = HAVE_RESET_BUTTON ? 1 : 0,
            .auth_mode      = MIBLE_AUTH_MODE,
            .version        = 5,
        };
        mible_addr_t dev_mac;
        mible_gap_address_get(dev_mac);
        mibeacon_config_t beacon_cfg = {
            .frame_ctrl     = fctrl,
            .pid            = PRODUCT_ID,
            .p_mac          = obj.len > 3 ? NULL : &dev_mac,
            .p_obj          = &obj,
            .obj_num        = 1,
        };

        /* Service Data: mijia service mibeacon with object. */
        adv_data[4] = 0x16;
        adv_data[5] = LO_BYTE(BLE_UUID_MI_SERVICE);
        adv_data[6] = HI_BYTE(BLE_UUID_MI_SERVICE);
        uint8_t len = 0;
        mibeacon_encode(&beacon_cfg, &adv_data[7], &len);
        adv_data[3] = 3 + len;
        adv_dlen   += 4 + len;

        uint8_t scan_rsp_data[31];
        uint8_t scan_rsp_dlen = 0;
        if (beacon_cfg.p_mac == NULL && !m_is_connected) {
            beacon_cfg.p_mac = &dev_mac;
            beacon_cfg.p_obj = NULL;
            mible_manu_data_set(&beacon_cfg, scan_rsp_data, &scan_rsp_dlen);
        }

        errno = mible_gap_adv_data_set(adv_data, adv_dlen, scan_rsp_data, scan_rsp_dlen);
        MI_ERR_CHECK(errno);

        /* Start adv duration timer, when timeout send the next object */
        mible_timer_start(mibeacon_timer, OBJ_ADV_TIMEOUT_MS, NULL);
		adv_reset_flag =1;
    }
}

static void set_beacon_key(uint8_t *p_key)
{
    if (p_key == NULL) {
        flags.is_valid_key = 0;
    } else {
        beacon_nonce.pid = PRODUCT_ID;
        mible_gap_address_get(beacon_nonce.mac);
        memcpy(beacon_key, p_key, sizeof(beacon_key));
        flags.is_valid_key = 1;
    }
}


/*
 * @brief   encode mibeacon data.
 * @param   [in] config: mibeacon configure data 
 *          [out] p_output: pointer to mibeacon data  (watch out array out of bounds) 
 *          [out] p_output_len: pointer to mibeacon data length 
 * @return  MI_ERR_INVALID_PARAM:   Invalid pointer supplied or mismatched frmctl.  
 *          MI_ERR_INVALID_LENGTH:  Adv data length exceeds MIBLE_MAX_ADV_LENGTH-7.
 *          MI_ERR_INTERNAL:        Not found rand num used to encrypt data.
 *          MI_SUCCESS:             Set successfully.
 * */
static mible_status_t mibeacon_encode(mibeacon_config_t const * const config, 
        uint8_t *output, uint8_t *output_len)
{
    mibeacon_frame_ctrl_t * const p_frame_ctrl = (void*)output;
    uint32_t errno = 0;
    
    if (config == NULL || output == NULL || output_len == NULL) {
        return MI_ERR_INVALID_PARAM;
    }

    /*  encode frame_ctrl and product_id */
    memcpy(output, (uint8_t*)config, 4);
    output[0]   = 0;
    output     += 4;

    /*  encode frame cnt */
    if (++seqnum.value % 512 == 0)
        mible_record_write(REC_ID_SEQNUM, seqnum.byte, 4);
    *output++ = (uint8_t) seqnum.value;

    /*  encode gap mac */
    if (config->p_mac != NULL) {
        p_frame_ctrl->mac_include = 1;
        memcpy(output, config->p_mac, 6);
        output += 6;
    }

    /*  encode capability */
    if (config->p_capability != NULL) {
        p_frame_ctrl->cap_include = 1;
        mibeacon_capability_t *p_cap = (void*)output;
        output += 1;
        *p_cap = *config->p_capability;

        /*	encode WIFI MAC address */
        if (config->p_wifi_mac != NULL) {
            p_cap->bondAbility = 3;
            memcpy(output, config->p_wifi_mac, 2);
            output += 2;
        }

        /*  encode IO cap */
        if (config->p_cap_sub_IO != NULL) {
            p_cap->IO_capability = 1;
            memcpy(output, config->p_cap_sub_IO, sizeof(mibeacon_cap_sub_io_t));
            output += sizeof(mibeacon_cap_sub_io_t);
        }
    }
    
    /*  encode encrypted objects */
    if (config->p_obj != NULL) {
        if (!flags.is_valid_key)
            return MI_ERR_INVALID_STATE;

        uint8_t *objs_ptr = output;
        uint8_t  objs_len = calc_objs_bytes(config->p_obj, config->obj_num);
        // 7 = 3 bytes ext frame cnt + 4 bytes MIC
        CHECK_ADV_LEN(output - (uint8_t*)p_frame_ctrl + objs_len + 7);

        // append plain objects
        p_frame_ctrl->obj_include = 1;
        for (uint8_t i = 0, max = config->obj_num; i < max; i++) {
            event_encode(config->p_obj + i, output);
            // 3 = 2 bytes object ID + 1 byte length
            output += 3 + config->p_obj[i].len;
        }

        // append ext frame cnt
        beacon_nonce.cnt = seqnum.byte[0];
        memcpy(beacon_nonce.ext_cnt, &seqnum.byte[1], 3);
        memcpy(output, beacon_nonce.ext_cnt, 3);
        output += sizeof(beacon_nonce.ext_cnt);

        // encrypt the objects
        p_frame_ctrl->is_encrypt  = 1;
        uint8_t mic[4];
        uint8_t aad = 0x11;
        errno = mi_crypto_ccm_encrypt(beacon_key,
                    (uint8_t*)&beacon_nonce, sizeof(beacon_nonce),
                                       &aad, sizeof(aad),
                                   objs_ptr, objs_len,
                                   objs_ptr,
                                        mic, 4);
        MI_ERR_CHECK(errno);
        if (errno)
            return MI_ERR_INTERNAL;

        // append MIC
        memcpy(output, mic, sizeof(mic));
        output += sizeof(mic);
    } else {
        p_frame_ctrl->is_encrypt  = 0;
        p_frame_ctrl->obj_include = 0;
    }

    /*  encode mesh info */
    if (config->p_mesh != NULL) {
        CHECK_ADV_LEN(output - (uint8_t*)p_frame_ctrl + sizeof(mibeacon_mesh_t));
        p_frame_ctrl->mesh_include = 1;
        memcpy(output, config->p_mesh, sizeof(mibeacon_mesh_t));
        output += sizeof(mibeacon_mesh_t);
    }

    *output_len = output - (uint8_t*)p_frame_ctrl;

    return MI_SUCCESS;
}

mible_status_t mible_service_data_set(mibeacon_config_t const * const config,
        uint8_t *p_output, uint8_t *p_output_len)
{
    uint32_t errno;
    uint8_t data_len = 0;

    if(config == NULL || p_output == NULL || p_output_len == NULL){
        return MI_ERR_INVALID_PARAM;
    }

    p_output[1] = 0x16;
    p_output[2] = LO_BYTE(BLE_UUID_MI_SERVICE);
    p_output[3] = HI_BYTE(BLE_UUID_MI_SERVICE);
    errno = mibeacon_encode(config, &p_output[4], &data_len);
    p_output[0] = 3 + data_len;

    if (errno != MI_SUCCESS) {
        MI_ERR_CHECK(errno);
        return MI_ERR_DATA_SIZE;
    }

    *p_output_len = 4 + data_len;

    return MI_SUCCESS;
}

static mible_status_t mible_manu_data_set(mibeacon_config_t const * const config,
        uint8_t *p_output, uint8_t *p_output_len)
{
    uint32_t errno;
    uint8_t data_len = 0;

    if(config == NULL || p_output == NULL || p_output_len == NULL){
        return MI_ERR_INVALID_PARAM;
    }

    p_output[1] = 0xFF;
    p_output[2] = LO_BYTE(BLE_UUID_COMPANY_ID_XIAOMI);
    p_output[3] = HI_BYTE(BLE_UUID_COMPANY_ID_XIAOMI);
    errno = mibeacon_encode(config, &p_output[4], &data_len);
    p_output[0] = 3 + data_len;

    if (errno != MI_SUCCESS) {
        MI_ERR_CHECK(errno);
        return MI_ERR_DATA_SIZE;
    }

    *p_output_len = 4 + data_len;

    return MI_SUCCESS;
}

static mible_status_t adv_start(uint16_t interval_ms, uint8_t is_connected)
{
    mible_gap_adv_stop();
    mible_gap_adv_param_t param = {
        .adv_interval_min = MS_TO_UNITS(interval_ms, 625),
        .adv_interval_max = MS_TO_UNITS(interval_ms+50, 625),
        .adv_type = is_connected ? MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED : MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED,
    };
	if(interval_ms>500){
		param.adv_interval_max = MS_TO_UNITS(interval_ms+(interval_ms/10), 625);
	}

    return mible_gap_adv_start(&param);
}

static void mi_service_handler(uint8_t is_connected)
{
    m_is_connected = is_connected;
    if (flags.is_sending) {
        // if there is object need to send : send (non) connectable adv
        adv_start(OBJ_ADV_INTERVAL_MS, m_is_connected);
    } else if (!m_is_connected) {
        // if disconnect: send connectable adv
        adv_start(m_adv_interval_ms, m_is_connected);
    }
}

mible_status_t mibeacon_init(uint8_t key[16], uint8_t is_newone)
{
    queue_init(&mi_obj_queue, (void*) obj_buf, ARRAY_SIZE(obj_buf), sizeof(obj_buf[0]));

    mi_service_event_register(mi_service_handler);

    if (mibeacon_timer == NULL)
        mible_timer_create(&mibeacon_timer, mibeacon_timer_handler, MIBLE_TIMER_SINGLE_SHOT);

    if (key == NULL || memcmp(key, beacon_key, 16) != 0)
        set_beacon_key(key);

    if (is_newone) {
        seqnum.value = 0;
    } else if (key != NULL && mible_record_read(REC_ID_SEQNUM, seqnum.byte, 4) == MI_SUCCESS) {
        seqnum.value += 512;
    } else {
        return MI_ERR_INTERNAL;
    }

    MI_LOG_DEBUG("Init mibeacon SEQNUM: %d\n", seqnum.value);

    return mible_record_write(REC_ID_SEQNUM, seqnum.byte, 4);
}

mible_status_t mibeacon_adv_data_set(uint8_t is_solicited, uint8_t mesh_stat, uint8_t *p_user_data, uint8_t user_dlen)
{
    mibeacon_frame_ctrl_t fctrl = {
        .registered     = HAVE_RESET_BUTTON ? flags.is_valid_key : 0,
        .solicite       = is_solicited,
        .auth_mode      = MIBLE_AUTH_MODE,
        .version        = 5,
    };

    mibeacon_capability_t cap = {
        .bondAbility = 1
    };

    mibeacon_cap_sub_io_t IO = {
        .in_digits   = OOB_USE_NUMPAD_INPUT,
        .out_digits  = OOB_USE_DISPLAT_OUT,
        .out_image   = OOB_USE_QR_CODE_OUT,
    };

    mible_addr_t dev_mac;
    mible_gap_address_get(dev_mac);

    mibeacon_config_t config = {
        .frame_ctrl = fctrl,
        .pid   = PRODUCT_ID,
        .p_mac = (mible_addr_t*)dev_mac,
    };

    if (!fctrl.registered) {
        config.p_capability = &cap;
        config.p_cap_sub_IO = OOB_IO_CAP ? &IO : NULL;
    }

#if MI_MESH_ENABLED
    mibeacon_mesh_t mesh = {
            .pb_gatt = 1,
            .state   = mesh_stat,
    };
    config.p_mesh = &mesh;
#endif

    // flags
    uint8_t adv_data[31] = {2, 1, 6};
    uint8_t adv_dlen = 3;

    // service data
    uint8_t dlen = 0;
    mible_service_data_set(&config, &adv_data[3], &dlen);
    adv_dlen += dlen;

    if (adv_dlen > MIBLE_MAX_ADV_LENGTH || user_dlen > MIBLE_MAX_ADV_LENGTH) {
        return MI_ERR_DATA_SIZE;
    }

    // try to append user advdata
    if (p_user_data != NULL) {
        // cache the user data for future object mibeacon adv.
        memcpy(m_user_data, p_user_data, user_dlen);
        m_user_dlen = user_dlen;

        // break some piece of user data into adv packet.
        while (user_dlen > 0 && adv_dlen + p_user_data[0] < MIBLE_MAX_ADV_LENGTH) {
            uint8_t len = p_user_data[0] + 1;
            if (len > user_dlen)
                return MI_ERR_DATA_SIZE;
            memcpy(adv_data+adv_dlen, p_user_data, len);
            adv_dlen    += len;
            p_user_data += len;
            user_dlen   -= len;
        }
    }

    return mible_gap_adv_data_set(adv_data, adv_dlen, p_user_data, user_dlen);
}

mible_status_t mibeacon_adv_start(uint16_t adv_interval_ms)
{
    m_adv_interval_ms = adv_interval_ms;
    return adv_start(adv_interval_ms, m_is_connected);
}

mible_status_t mibeacon_adv_stop(void)
{
    return mible_gap_adv_stop();
}

int mibeacon_obj_enque(mibeacon_obj_name_t nm, uint8_t len, void *val, uint8_t stop_adv)
{
    uint32_t errno;
    mibeacon_obj_t obj;

    if (!flags.is_valid_key)
        return MI_ERR_INVALID_STATE;

    if (len > sizeof(obj.val))
        return MI_ERR_DATA_SIZE;

    obj.type = nm;
    obj.len  = len;
    obj.need_encrypt = 1;
    memcpy(obj.val, (uint8_t*)val, len);

    if(enqueue(&mi_obj_queue, &obj) == MI_ERR_NO_MEM) {
        MI_LOG_ERROR("mibeacon object queue full.\n");
        return MI_ERR_RESOURCES;
    }

    flags.stop_adv_after_sent_out = stop_adv;

    if (flags.is_sending != true ) {
        adv_start(OBJ_ADV_INTERVAL_MS, m_is_connected);
        /* All event will be processed in mibeacon_timer_handler() */
        errno = mible_timer_start(mibeacon_timer, 10, NULL);
        MI_ERR_CHECK(errno);
        if (errno != MI_SUCCESS)
            return MI_ERR_INTERNAL;
        else
            flags.is_sending = true;
    }

    return MI_SUCCESS;
}
