//#include <string.h>
#include "mible_api.h"
#include "mi_config.h"
#include "common/tlv.h"
#include "cryptography/mi_crypto_backend_msc.h"
#include "cryptography/mja1/mjac_wrapper.h"

#if (HAVE_MSC)
#undef  MI_LOG_MODULE_NAME
#define MI_LOG_MODULE_NAME "MSC"
#include "mible_log.h"

#define MSC_XFER(CMD, INPUT, INPUT_L, OUTPUT, OUTPUT_L)                         \
           (msc_cb_t) {         .cmd      = CMD,                                \
                                .p_para   = INPUT,                              \
                                .para_len = INPUT_L,                            \
                                .p_data   = OUTPUT,                             \
                                .data_len = OUTPUT_L }

#define MJSC               1
#define MJA1               2

#define MSC_ADDR           0x2A
#define MSC_INFO_TAG_VER   0x07
#define MSC_INFO_LEN       34
#define MAX_BUSY_TIME_MS   500
uint16_t REVERSE16(uint16_t x)
{
    u16 tmp;
    tmp = (x & 0x00FF) << 8;
    tmp|= (x & 0xFF00) >> 8;
    return tmp;
}

typedef struct {
    msc_cmd_t     cmd;
    uint16_t para_len;
    uint16_t data_len;
    uint8_t   *p_para;
    uint8_t   *p_data;
    uint8_t    status;
} msc_cb_t;

static pt_t pt_msc_thd;
static pt_t pt_mjac;
static pstimer_t *m_timer;
static uint8_t  twi_buf[515];
static uint16_t twi_tx_len;
static uint16_t twi_rx_len;
static bool m_onoff;
static msc_cb_t msc_control_block;
static volatile char              m_iic_xfer_ack;
static const iic_config_t *       m_p_iic_config;
static int (*m_msc_power_manage)(bool power_stat);
static int msc_thread(msc_cb_t *p_cb);


static void msc_iic_handler(void * arg)
{
    iic_event_t * p_event = arg;
    switch (*p_event) {
    case IIC_EVT_XFER_DONE:
        m_iic_xfer_ack = 1;
        break;
    case IIC_EVT_ADDRESS_NACK:
#if (HAVE_MSC == MJSC)
        MI_LOG_ERROR("IIC ADDR NACK\n");
#endif
        m_iic_xfer_ack = 2;
        break;
    case IIC_EVT_DATA_NACK:
        MI_LOG_ERROR("IIC DATA NACK\n");
        m_iic_xfer_ack = 3;
        break;
    }
    return;
}

#if (HAVE_MSC == MJSC)
static struct {
    uint16_t dev;
    uint16_t manu;
    uint16_t root;
} m_certs_len;

static uint8_t calc_data_xor(uint8_t *pdata, uint16_t len)
{
    uint8_t chk = 0;
    while(len--)
        chk ^= *pdata++;
    return chk;
}
#endif

static int msc_encode_twi_buf(msc_cb_t *p_cb, uint16_t *p_tx_len, uint16_t *p_rx_len)
{
#if (HAVE_MSC == MJA1)
    uint8_t hash_buf[MJAC_SZ_HASH]  = {0};
    uint8_t pub_buf[MJAC_SZ_PUBKEY] = {0};
    uint8_t sig_buf[MJAC_SZ_SIGN]   = {0};
    uint8_t buf[500] = {0};

    switch (p_cb->cmd) {
    case MSC_ROOT_CERT_LEN:
        *p_tx_len = mjac_cmd_build_read(twi_buf, MJAC_SZ_CMD_MAX, MJAC_ZONE_ROOT_CRT, 2, p_cb->data_len);
        *p_rx_len = p_cb->data_len;
        break;
        
    case MSC_MANU_CERT_LEN:
        *p_tx_len = mjac_cmd_build_read(twi_buf, MJAC_SZ_CMD_MAX, MJAC_ZONE_MANU_CRT, 2, p_cb->data_len);
        *p_rx_len = p_cb->data_len;
        break;
        
    case MSC_DEV_CERT_LEN:
        *p_tx_len = mjac_cmd_build_read(twi_buf, MJAC_SZ_CMD_MAX, MJAC_ZONE_DEV_CRT, 2, p_cb->data_len);
        *p_rx_len = p_cb->data_len;
        break;
        
    case MSC_DEV_CERT:
        *p_tx_len= mjac_cmd_build_read(twi_buf, MJAC_SZ_CMD_MAX, MJAC_ZONE_DEV_CRT, 0, p_cb->data_len);
        *p_rx_len = p_cb->data_len;
        break;
        
    case MSC_MANU_CERT:
        *p_tx_len = mjac_cmd_build_read(twi_buf, MJAC_SZ_CMD_MAX, MJAC_ZONE_MANU_CRT, 0, p_cb->data_len);
        *p_rx_len = p_cb->data_len;
        break;
        
    case MSC_ROOT_CERT:
        *p_tx_len = mjac_cmd_build_read(twi_buf, MJAC_SZ_CMD_MAX, MJAC_ZONE_ROOT_CRT, 0, p_cb->data_len);
        *p_rx_len = p_cb->data_len;
        break;
        
    case MSC_ID:
        *p_tx_len = mjac_cmd_build_read(twi_buf, MJAC_SZ_CMD_MAX, MJAC_ZONE_PRODUCT, 2, p_cb->data_len);
        *p_rx_len = p_cb->data_len;
        break;
        
    case MSC_INFO:
        *p_tx_len = mjac_cmd_build_query(twi_buf, MJAC_SZ_CMD_MAX);
        *p_rx_len = p_cb->data_len;
        break;
        
    case MSC_SIGN:
        hash_buf[0] = 0x00;
        hash_buf[1] = 0x20;
        memcpy(hash_buf + 2, p_cb->p_para, 32);
        *p_tx_len = mjac_cmd_build_generate_signature(twi_buf, MJAC_SZ_CMD_MAX, hash_buf, MJAC_SZ_HASH);
        *p_rx_len = MJAC_SZ_SIGN;
        break;
        
    case MSC_VERIFY:
        pub_buf[0] = 0x04;
        pub_buf[1] = 0x00;
        pub_buf[2] = 0x20;
        memcpy(pub_buf + 3, p_cb->p_para, 32);
        pub_buf[35] = 0x00;
        pub_buf[36] = 0x20;
        memcpy(pub_buf + 37, p_cb->p_para + 32, 32);

        hash_buf[0] = 0x00;
        hash_buf[1] = 0x20;
        memcpy(hash_buf + 2, p_cb->p_para + 64, 32);

        sig_buf[0] = 0x00;
        sig_buf[1] = 0x20;
        memcpy(sig_buf + 2, p_cb->p_para + 64 + 32, 32);
        sig_buf[34] = 0x00;
        sig_buf[35] = 0x20;
        memcpy(sig_buf + 36, p_cb->p_para + 64 + 32 + 32, 32);
        
        *p_tx_len = mjac_cmd_build_verify_signature(twi_buf, MJAC_SZ_CMD_MAX, pub_buf, MJAC_SZ_PUBKEY, 
                         sig_buf, MJAC_SZ_SIGN, hash_buf, MJAC_SZ_HASH); 
        *p_rx_len = 1;
        break;
        
    case MSC_ECDHE:
        pub_buf[0] = 0x04;
        pub_buf[1] = 0x00;
        pub_buf[2] = 0x20;
        memcpy(pub_buf + 3 , p_cb->p_para, 32);
        pub_buf[35] = 0x00;
        pub_buf[36] = 0x20;
        memcpy(pub_buf + 37, p_cb->p_para + 32, 32);
        *p_tx_len = mjac_cmd_build_establish_key(twi_buf, MJAC_SZ_CMD_MAX, pub_buf, MJAC_SZ_PUBKEY);
        *p_rx_len = MJAC_SZ_SECRET;
        break;
        
    case MSC_PUBKEY:
        *p_tx_len = mjac_cmd_build_generate_key(twi_buf, MJAC_SZ_CMD_MAX);
        *p_rx_len = MJAC_SZ_PUBKEY;
        break;
        
    case MSC_WR_MKPK:
        *p_tx_len = mjac_cmd_build_update(twi_buf, MJAC_SZ_CMD_MAX, MJAC_ZONE_USER, (p_cb->p_para[0])*512, &(p_cb->p_para[1]), (p_cb->para_len)-1);
        *p_rx_len = 0;
        break;
        
    case MSC_RD_MKPK:
        *p_tx_len = mjac_cmd_build_read(twi_buf, MJAC_SZ_CMD_MAX, MJAC_ZONE_USER, (p_cb->p_para[0])*512, p_cb->data_len);
        *p_rx_len = p_cb->data_len;
        break;
        
    case MSC_ERASE:
        *p_tx_len = mjac_cmd_build_update(twi_buf, MJAC_SZ_CMD_MAX, MJAC_ZONE_USER, p_cb->p_para[0]*512,
                         buf, 500);
        *p_rx_len = 0;
        break;

   case MSC_HIBERNATE:
         *p_tx_len = mjac_cmd_build_hibernate(twi_buf, MJAC_SZ_CMD_MAX);
         *p_rx_len = 0;
         break;

    default:
        break;
    }
    
    if (*p_tx_len ==  0) {
        MI_LOG_ERROR("mjac para len error.\n");
        return 1;
    }
    /* 5 = 2bytes lengh + 1byte cmd + 2byte crc */
    *p_rx_len += 5;
    return 0;
        
#else
    uint16_t para_len = p_cb->p_para == NULL ? 0 : p_cb->para_len;
    uint16_t cmd_len  = para_len + 1;

    if (cmd_len > 512) {
        MI_LOG_ERROR("MJSC para len error.\n");
        return 1;
    }
    
    twi_buf[0] = cmd_len >> 8;
    twi_buf[1] = cmd_len & 0xFF;
    twi_buf[2] = p_cb->cmd;
    memcpy(twi_buf+3, p_cb->p_para, para_len);
    twi_buf[3+para_len] = calc_data_xor(twi_buf, para_len + 3);
    
    /* 4 = 2bytes lengh + 1byte status + 1byte chk  */
    *p_tx_len = p_cb->para_len + 4;
    *p_rx_len = p_cb->data_len + 4;

    return 0;
#endif   
} 

static int msc_decode_twi_buf(msc_cb_t *p_cb, uint16_t rx_len)
{
#if (HAVE_MSC == MJA1)
    uint16_t length;
    uint16_t tlvalue;
    char *p_value;
    tlv8_t *p_tlv;
    mjac_resp_code_t rsp_code;
    
    rsp_code = mjac_resp_parse_data(twi_buf, &length);
    p_cb->status = rsp_code;
    
    if(rsp_code != 0) {
        MI_LOG_ERROR("MJA1 return code error.\n");
        return 1;
    }

    /* 5 = 2bytes lengh + 1byte cmd + 2byte crc */
    if (length != rx_len - 5) {
        MI_LOG_ERROR("MJA1 data len error.\n");
        p_cb->status = 1;
        return 1;
    }
    
    switch (p_cb->cmd) {
    case MSC_DEV_CERT:
    case MSC_MANU_CERT:  
    case MSC_ROOT_CERT:
    case MSC_ID:
    case MSC_RD_MKPK:
        memcpy(p_cb->p_data, twi_buf, rx_len - 5);
        break;
        
    case MSC_SIGN:
        memcpy(p_cb->p_data, twi_buf + 2, 32);
        memcpy(p_cb->p_data + 32, twi_buf + 2 + 32 + 2, 32);
        break;
        
    case MSC_VERIFY:
        if (twi_buf[0] != 1) {
            p_cb->status = 1;
            return 1;
        }
        break;

    case MSC_ECDHE:
        memcpy(p_cb->p_data, twi_buf + 2, 32);
        break;

    case MSC_PUBKEY:
        memcpy(p_cb->p_data, twi_buf + 3, 32);
        memcpy(p_cb->p_data + 32, twi_buf + 37, 32);
        break;
        
    case MSC_INFO:
        if (tlv8_decode(twi_buf, MSC_INFO_LEN, MSC_INFO_TAG_VER, &p_tlv) != 0){
            MI_LOG_WARNING("sw ver decoder error\n");
            break;
        }
        if (p_tlv->len != sizeof(uint16_t)){
             MI_LOG_WARNING("sw ver size error\n");
             break;
        }
        
        p_value = (char*)(p_tlv->value);
        tlvalue = *p_value;
        tlvalue = (tlvalue << 8) | (*(p_value + 1));
        ((msc_info_t*)(p_cb->p_data))->sw_ver = tlvalue;
        break;
        
    default:
        break;
    }
    
    return 0;
#else
    uint16_t len = (twi_buf[0]<<8) | twi_buf[1];        // contain data + status
    uint16_t data_len = len - sizeof(p_cb->status);
    
    
    if(data_len != p_cb->data_len) {
        MI_LOG_ERROR("MJSC data len error.\n");
        return 1;
    }

    uint8_t  chk = calc_data_xor(twi_buf, 2+len);

    if (chk != twi_buf[2+len]) {
        p_cb->status = 255;
        return 2;
    }

    if (p_cb->status == 0x0f) {
        MI_LOG_ERROR("MJSC received invaild packet.\n");
        return 3;
    }

    p_cb->status = twi_buf[2+data_len];

    if (p_cb->p_data != NULL)
        memcpy(p_cb->p_data, twi_buf+2, data_len);
    
    return 0;
#endif
}

static int msc_thread(msc_cb_t *p_cb)
{
    uint32_t err_code;
    static uint8_t retry_times = 0;

    PT_BEGIN(&pt_msc_thd);
    
    err_code = msc_encode_twi_buf(p_cb, &twi_tx_len, &twi_rx_len);
    if (err_code != 0) {
        MI_LOG_ERROR("Cann't encode CMD 0x%02X\n", p_cb->cmd);
        p_cb->cmd = MSC_NOP;
        PT_EXIT(&pt_msc_thd);  
    }
    
    MI_LOG_INFO("Initiate CMD 0x%02X @ schd_ticks %d\n", p_cb->cmd, m_timer->p_curr_tick[0]);

    m_iic_xfer_ack = 0;
    err_code = mible_iic_tx(MSC_ADDR, twi_buf, twi_tx_len, 0);
    MI_ERR_CHECK(err_code);
    PT_WAIT_UNTIL(&pt_msc_thd, m_iic_xfer_ack == 1);

    MI_LOG_INFO("Waiting...  @ schd_ticks %d\n", m_timer->p_curr_tick[0]);

#if (HAVE_MSC == MJA1)
    static uint8_t poll_times;
    poll_times = 0;
    while(1) {
        m_iic_xfer_ack  = 0;
        mible_iic_rx(MSC_ADDR, twi_buf, twi_rx_len);
        PT_WAIT_UNTIL(&pt_msc_thd, m_iic_xfer_ack);

        if (m_iic_xfer_ack == 1) {
            break;
        } else {
            if (poll_times * 10 <= MAX_BUSY_TIME_MS)
                poll_times++;
            else {
                p_cb->cmd = MSC_NOP;
                PT_EXIT(&pt_msc_thd);
            }

            /* delay 10 ms */
            pstimer_set(m_timer, 10);
            PT_WAIT_UNTIL(&pt_msc_thd, pstimer_expired(m_timer, NULL));
        }
    }
#else
    PT_WAIT_UNTIL(&pt_msc_thd, mible_iic_scl_pin_read(m_p_iic_config->scl_port, m_p_iic_config->scl_pin));
    m_iic_xfer_ack = 0;
    err_code = mible_iic_rx(MSC_ADDR, twi_buf, twi_rx_len);
    MI_ERR_CHECK(err_code);
    PT_WAIT_UNTIL(&pt_msc_thd, m_iic_xfer_ack);
#endif

    MI_LOG_INFO("Ready now.  @ schd_ticks %d\n", m_timer->p_curr_tick[0]);

    msc_decode_twi_buf(p_cb, twi_rx_len);
    if (p_cb->status != 0 ) {
        if (retry_times < 3) {
            retry_times++;
            MI_LOG_ERROR("CMD 0x%02X Error 0x%02X\nRETRY...\n", p_cb->cmd, p_cb->status);
            PT_RESTART(&pt_msc_thd);
        } else {
            retry_times = 0;
            MI_LOG_ERROR("Cann't finish MSC CMD 0x%02X\n", p_cb->cmd);
            p_cb->cmd = MSC_NOP;
            PT_EXIT(&pt_msc_thd);
        }
    } else {
        retry_times = 0;
    }

    MI_LOG_INFO("Finish CMD 0x%02X @ schd_ticks %d\n\n",p_cb->cmd, m_timer->p_curr_tick[0]);
    p_cb->cmd = MSC_NOP;

    PT_END(&pt_msc_thd);
}

int msc_hibernate()
{ 
    msc_control_block = MSC_XFER(MSC_HIBERNATE, NULL, 0, NULL, 0);
    return msc_thread(&msc_control_block);
}

static pt_t pt_msc_on, pt_msc_off;
int msc_power_on(void)
{
    if (m_onoff == 0) {
        m_onoff = 1;
        PT_INIT(&pt_msc_thd);
        PT_INIT(&pt_mjac);
        PT_INIT(&pt_msc_on);
        PT_INIT(&pt_msc_off);
        mible_iic_init(m_p_iic_config, msc_iic_handler);
    }

    PT_BEGIN(&pt_msc_on);

    if (m_msc_power_manage != NULL) {
        m_msc_power_manage(1);
        pstimer_set(m_timer, 10);
        PT_WAIT_UNTIL(&pt_msc_on, pstimer_expired(m_timer, NULL));
#if (HAVE_MSC == MJA1)
        m_msc_power_manage(0);
        pstimer_set(m_timer, 10);
        PT_WAIT_UNTIL(&pt_msc_on, pstimer_expired(m_timer, NULL));

        m_msc_power_manage(1);
        pstimer_set(m_timer, 10);
        PT_WAIT_UNTIL(&pt_msc_on, pstimer_expired(m_timer, NULL));
#endif
        pstimer_set(m_timer, 40);
        PT_WAIT_UNTIL(&pt_msc_on, pstimer_expired(m_timer, NULL));
    }
    msc_control_block.cmd = MSC_NOP;

    PT_END(&pt_msc_on);
}

int msc_power_off(void)
{
    if (m_onoff == 0)
        return PT_ENDED;

#if (HAVE_MSC == MJA1)
    PT_BEGIN(&pt_msc_off);

    PT_WAIT_UNTIL(&pt_msc_off, msc_control_block.cmd == MSC_NOP);
    
    msc_control_block = MSC_XFER(MSC_HIBERNATE, NULL, 0, NULL, 0);
    PT_SPAWN(&pt_msc_off, &pt_msc_thd, msc_thread(&msc_control_block));
    
    mible_iic_uninit();
    m_onoff = 0;
    PT_END(&pt_msc_off);
#else
    if (m_msc_power_manage != NULL)
        m_msc_power_manage(0);

    mible_iic_uninit();
    m_onoff = 0;
    return PT_ENDED;
#endif
}

int mi_msc_config(void * pwr_manage, const iic_config_t * p_iic_config, pstimer_t *p_timer)
{
    if (p_iic_config == NULL || p_timer == NULL)
        return -1;

    if (pwr_manage != NULL) {
        m_msc_power_manage = (int(*)(bool))pwr_manage;
        m_msc_power_manage(0);
    }

    m_p_iic_config = p_iic_config;
    m_timer        = p_timer;
    return 0;
}

int msc_get_chip_info(msc_info_t *p_info)
{
#if (HAVE_MSC == MJA1)
    msc_control_block = MSC_XFER(MSC_INFO, NULL, 0, (void*)p_info, MSC_INFO_LEN);
    return msc_thread(&msc_control_block);

#else
    msc_control_block = MSC_XFER(MSC_INFO, NULL, 0, (void*)p_info, 26);
    return msc_thread(&msc_control_block);
#endif
}

int msc_get_device_id(uint8_t did_big_endian[8])
{
    msc_control_block = MSC_XFER(MSC_ID, NULL, 0, (void*)did_big_endian, 8);
    return msc_thread(&msc_control_block);
}


int msc_get_cert_len(uint16_t *p_dev_len, uint16_t *p_manu_len, uint16_t *p_root_len)
{
#if (HAVE_MSC == MJA1)
    int ret;
    PT_BEGIN(&pt_mjac);

    if (p_dev_len != NULL) {
        msc_control_block = MSC_XFER(MSC_DEV_CERT_LEN, NULL, 0, (void*)p_dev_len, 2);
        PT_WAIT_THREAD(&pt_mjac, ret = msc_thread(&msc_control_block));
        if(ret == PT_ENDED) {
            *p_dev_len = 4 + (twi_buf[0] << 8 | twi_buf[1]);
        } else {   
            MI_LOG_ERROR("get dev len error\n");
            return ret;
        }
    }

    if (p_manu_len != NULL) {
        msc_control_block = MSC_XFER(MSC_MANU_CERT_LEN, NULL, 0, (void*)p_manu_len, 2);
        PT_WAIT_THREAD(&pt_mjac, ret = msc_thread(&msc_control_block));
        if(ret == PT_ENDED) {
            *p_manu_len = 4 + (twi_buf[0] << 8 | twi_buf[1]);
        } else {   
            MI_LOG_ERROR("get manu len error\n");
            return ret;
        }
    }

    if (p_root_len != NULL) {
        msc_control_block = MSC_XFER(MSC_ROOT_CERT_LEN, NULL, 0, (void*)p_root_len, 2);
        PT_WAIT_THREAD(&pt_mjac, ret = msc_thread(&msc_control_block));
        if(ret == PT_ENDED) {
            *p_root_len = 4 + (twi_buf[0] << 8 | twi_buf[1]);
        } else {
            MI_LOG_ERROR("get root len error\n");
            return ret;
        }
    }

    PT_END(&pt_mjac);

#else
    int ret;

    msc_control_block = MSC_XFER(MSC_CERTS_LEN, NULL, 0, (void*)&m_certs_len, sizeof(m_certs_len));
    ret = msc_thread(&msc_control_block);
    
    if (ret == PT_ENDED) {
        p_dev_len  ? *p_dev_len  = REVERSE16(m_certs_len.dev)  : 0;
        p_manu_len ? *p_manu_len = REVERSE16(m_certs_len.manu) : 0;
        p_root_len ? *p_root_len = REVERSE16(m_certs_len.root) : 0;
    }
    return ret;
#endif
}

int msc_get_cert_der(uint8_t cert_type, uint8_t *p_der, uint16_t der_len)
{
    msc_control_block = MSC_XFER((msc_cmd_t)cert_type, NULL, 0, p_der, der_len);
    return msc_thread(&msc_control_block);
}

int msc_ecc_keypair_gen(const uint8_t *p_curve, uint8_t *p_le_pk)
{
    msc_control_block = MSC_XFER(MSC_PUBKEY, NULL, 0, p_le_pk, 64);
    return msc_thread(&msc_control_block);
}

int msc_ecdh(const uint8_t *p_curve, const uint8_t *p_le_pk, uint8_t *p_shared)
{
    msc_control_block = MSC_XFER(MSC_ECDHE, (uint8_t*)p_le_pk, 64, p_shared, 32);
    return msc_thread(&msc_control_block);

}

int msc_ecdsa_verify(const uint8_t *p_curve, const uint8_t *p_le_pk,
                   uint8_t const *p_hash, uint8_t const *p_sig)
{
#if (HAVE_MSC == MJA1)
    uint8_t buf[64+32+64];

    memcpy(buf, p_le_pk, 64);
    memcpy(buf+64, p_hash, 32);
    memcpy(buf+64+32, p_sig, 64);
#else
    uint8_t buf[1+64+1+32+1+64];

    buf[0] = 64;
    memcpy(buf+1, p_le_pk, 64);
    buf[1+64] = 32;
    memcpy(buf+1+64+1, p_hash, 32);
    buf[1+64+1+32] = 64;
    memcpy(buf+1+64+1+32+1, p_sig, 64);
#endif

    msc_control_block = MSC_XFER(MSC_VERIFY, buf, sizeof(buf), NULL, 0);
    return msc_thread(&msc_control_block);
}

int msc_ecdsa_sign(const uint8_t *p_curve, const uint8_t *p_hash, uint8_t *p_sig)
{
    msc_control_block = MSC_XFER(MSC_SIGN, (uint8_t*)p_hash, 32, p_sig, 64);
    return msc_thread(&msc_control_block);
}

int msc_record_write(uint8_t record_id, const uint8_t *p_data, uint8_t len)
{
    struct {
        uint8_t id;
        uint8_t data[47];
    } buf;
    
    buf.id = record_id;
    memcpy(buf.data, p_data, len);

    msc_control_block = MSC_XFER(MSC_WR_MKPK, (void*)&buf, 1+len, NULL, 0);
    return msc_thread(&msc_control_block);
}

int msc_record_read(uint8_t record_id, uint8_t *p_data, uint8_t len)
{
    msc_control_block = MSC_XFER(MSC_RD_MKPK, (void*)&record_id, 1, p_data, len);
    return msc_thread(&msc_control_block);

}

int msc_record_delete(uint8_t record_id)
{
    msc_control_block = MSC_XFER(MSC_ERASE, (void*)&record_id, 1, NULL, 0);
    return msc_thread(&msc_control_block);
}
#endif

