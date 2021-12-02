/* Copyright (c) 2010-2017 Xiaomi. All Rights Reserved.
 *
 * The information contained herein is property of Xiaomi.
 * Terms and conditions of usage are described in detail in 
 * STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
#include <stdarg.h>
//#include <stdlib.h>

#include "mi_config.h"
#include "mible_api.h"
#include "common/mible_crypto.h"

#define MI_LOG_MODULE_NAME "lock"
#include "mible_port.h"
#include "mible_log.h"
#include "lock_service_server.h"

#define BLE_CONN_HANDLE_INVALID         0xFFFF
#define UNIT_10_MS                      10000

#define MSEC_TO_UNITS(TIME, RESOLUTION) (((TIME) * 1000) / (RESOLUTION))

#define BLE_UUID_LOCK_SRV    {0x6D,0x69,0x2E,0x6D,0x69,0x6F,0x74,0x2E,0x62,0x6C,0x65,0x00,0x00,0x10,0x00,0x00}

#define BLE_UUID_LOCK_OPS    {0x6D,0x69,0x2E,0x6D,0x69,0x6F,0x74,0x2E,0x62,0x6C,0x65,0x00,0x01,0x10,0x00,0x00}
#define BLE_UUID_LOCK_STATE  {0x6D,0x69,0x2E,0x6D,0x69,0x6F,0x74,0x2E,0x62,0x6C,0x65,0x00,0x02,0x10,0x00,0x00}
#define BLE_UUID_LOCK_LOGS   {0x6D,0x69,0x2E,0x6D,0x69,0x6F,0x74,0x2E,0x62,0x6C,0x65,0x00,0x03,0x10,0x00,0x00}


#define CHAR_OPERATION_IDX   0
#define CHAR_STATE_IDX       1
#define CHAR_LOGS_IDX        2
#define CHAR_NUMS            3


typedef struct {
    uint8_t                  uuid_type;               /**< UUID type for Lock Service Base UUID. */
    uint16_t                 conn_handle;             /**< Handle of the current connection (as provided by the BLE Stack). BLE_CONN_HANDLE_INVALID if not in a connection. */
    uint16_t                 service_handle;          /**< Handle of Lock Service (as provided by the BLE Stack). */

    uint16_t                 operation_handle;        /**< Handle related to the characteristic value (as provided by the BLE Stack). */
    uint16_t                 state_handle;            /**< Handle related to the characteristic value (as provided by the BLE Stack). */
    uint16_t                 logs_handle;             /**< Handle related to the characteristic value (as provided by the BLE Stack). */

} lock_service_t;

static lock_service_t m_lock_srv;
static uint8_t lock_operation[7];
static uint8_t lock_state[7];
static uint8_t lock_logs[20];

static lock_handler_t m_opcode_handler;

/**@brief Function for handling the @ref BLE_GAP_EVT_CONNECTED event from the BLE Stack.
 *
 * @param[in] p_mi_s    Lock Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_connect(mible_gap_evt_param_t * p_ble_evt)
{
    m_lock_srv.conn_handle = p_ble_evt->conn_handle;
}

/**@brief Function for handling the @ref BLE_GAP_EVT_DISCONNECTED event from the BLE Stack.
 *
 * @param[in] p_mi_s    Lock Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_disconnect(mible_gap_evt_param_t * p_ble_evt)
{
    m_lock_srv.conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**@brief Function for handling the @ref BLE_GATTS_EVT_READ event from the BLE Stack.
 *
 * @param[in] p_mi_s    Lock Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_read(mible_gatts_evt_param_t * p_ble_evt)
{
    uint16_t handle = p_ble_evt->read.value_handle;

#if DEBUG_MIBLE
    MI_LOG_DEBUG("read handle %d: \n", handle);
#endif

    if (handle == m_lock_srv.state_handle) {
        if (get_mi_authorization() == UNAUTHORIZATION) {
            mible_gatts_rw_auth_reply(m_lock_srv.conn_handle, 0, handle, 0, NULL, 0, 1);
        } else {
            mible_gatts_rw_auth_reply(m_lock_srv.conn_handle, 1, handle, 0, lock_state, sizeof(lock_state), 1);
        }
    }
    else if (handle == m_lock_srv.logs_handle) {
        if (get_mi_authorization() == UNAUTHORIZATION) {
            mible_gatts_rw_auth_reply(m_lock_srv.conn_handle, 0, handle, 0, NULL, 0, 1);
        } else {
            mible_gatts_rw_auth_reply(m_lock_srv.conn_handle, 1, handle, 0, lock_logs, sizeof(lock_logs), 1);
        }
    }
    else {
        // Do Nothing. This event is not relevant for this service.
    }
}

/**@brief Function for handling the @ref BLE_GATTS_EVT_WRITE_PERMIT event from the BLE Stack.
 *
 * @param[in] p_mi_s    Lock Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_write_permit(mible_gatts_evt_param_t * p_ble_evt)
{
    uint16_t handle = p_ble_evt->write.value_handle;
    uint16_t   len = p_ble_evt->write.len;
    uint8_t *pdata = p_ble_evt->write.data;

    if (handle == m_lock_srv.operation_handle) {
#if DEBUG_MIBLE
        MI_LOG_DEBUG("write permit handle %d: \n", handle);
        MI_LOG_HEXDUMP(pdata, len);
#endif
        if (get_mi_authorization() == UNAUTHORIZATION) {
            mible_gatts_rw_auth_reply(m_lock_srv.conn_handle, 0, handle, 0, NULL, 0, 2);
        } else {
            mible_gatts_rw_auth_reply(m_lock_srv.conn_handle, 1, handle, 0, pdata, len, 2);
            uint8_t opcode = 0;
            uint8_t errno = mi_session_decrypt(pdata, len, &opcode);
            if(errno == MI_SUCCESS && m_opcode_handler != NULL)
                m_opcode_handler(opcode);
            else
                mible_gap_disconnect(m_lock_srv.conn_handle);
        }
    }
}

static void service_init_succ(mible_arch_gatts_srv_init_cmp_t *param)
{
    mible_gatts_srv_db_t *p_srv_db = param->p_gatts_db->p_srv_db;
    mible_gatts_char_db_t *p_char_db = p_srv_db->p_char_db;
    
    MI_LOG_INFO("lock service init succ.\n");

    if (param->status == MI_SUCCESS) {
        m_lock_srv.service_handle       = p_srv_db->srv_handle;
        m_lock_srv.operation_handle     = p_char_db[CHAR_OPERATION_IDX].char_value_handle;
        m_lock_srv.state_handle         = p_char_db[CHAR_STATE_IDX].char_value_handle;
        m_lock_srv.logs_handle          = p_char_db[CHAR_LOGS_IDX].char_value_handle;

        MI_LOG_INFO("char value handle list\n operation : %d\n state : %d\n logs : %d\n",
            m_lock_srv.operation_handle, m_lock_srv.state_handle, m_lock_srv.logs_handle);
    }
    
    //free(p_char_db);
    //free(p_srv_db);
}

static void gap_event_handler(mible_gap_evt_t evt,
    mible_gap_evt_param_t* param)
{
    
    switch (evt) {
    case MIBLE_GAP_EVT_CONNECTED:
        on_connect(param);
        break;
    case MIBLE_GAP_EVT_DISCONNECT:
        on_disconnect(param);
        break;
    case MIBLE_GAP_EVT_CONN_PARAM_UPDATED:
        break;
    case MIBLE_GAP_EVT_ADV_REPORT:
        break;
    }
}

static void gatts_event_handler(mible_gatts_evt_t evt,
    mible_gatts_evt_param_t* param)
{
    switch (evt) {
    case MIBLE_GATTS_EVT_WRITE:

        break;
    case MIBLE_GATTS_EVT_READ_PERMIT_REQ:
        on_read(param);
        break;
    case MIBLE_GATTS_EVT_WRITE_PERMIT_REQ:
        on_write_permit(param);
        break;
    case MIBLE_GATTS_EVT_IND_CONFIRM: break;
    }
}

static void arch_event_handler(mible_arch_event_t evt, 
        mible_arch_evt_param_t* param)
{
    switch (evt) {
    case MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP:
        if (memcmp(param->srv_init_cmp.p_gatts_db->p_srv_db->srv_uuid.uuid128,
                   (uint8_t [])BLE_UUID_LOCK_SRV, 16) == 0)
            service_init_succ(&param->srv_init_cmp);
        break;

    default:
        break;
    }
}

mible_gatts_srv_db_t lock_service_server={0};
mible_gatts_char_db_t lock_char_server[CHAR_NUMS];

int lock_service_init(lock_init_t *p_config)
{
    uint32_t      err_code;
    mible_uuid_t  uuid;

    if (p_config != NULL && p_config->opcode_handler != NULL) {
        m_opcode_handler = p_config->opcode_handler;
    } else {
        MI_LOG_ERROR(" opcode handler is NULL\n");
        return -1;
    }

    m_lock_srv.conn_handle             = BLE_CONN_HANDLE_INVALID;

    err_code = mible_gap_register(gap_event_handler);
    MI_ERR_CHECK(err_code);

    err_code = mible_gatts_register(gatts_event_handler);
    MI_ERR_CHECK(err_code);

    err_code = mible_arch_register(arch_event_handler);
    MI_ERR_CHECK(err_code);

    // Initialize the service structure.
    MI_LOG_DEBUG("lock service init ... srv %d, char %d\n", sizeof(mible_gatts_srv_db_t), sizeof(mible_gatts_char_db_t) );
    mible_gatts_db_t mi_gatts_db;
    mible_gatts_srv_db_t *p_lock_service = (mible_gatts_srv_db_t*)&lock_service_server; 
    mible_gatts_char_db_t *p_lock_char = (mible_gatts_char_db_t *)lock_char_server;

    mi_gatts_db = (mible_gatts_db_t){
        .p_srv_db = p_lock_service,
        .srv_num = 1,
    };

    // add lock service
    uuid.type = 1;
    memcpy(uuid.uuid128, (uint8_t [])BLE_UUID_LOCK_SRV, 16);
    *p_lock_service = (mible_gatts_srv_db_t){
        .srv_type = MIBLE_PRIMARY_SERVICE,
        .srv_uuid = uuid,
        .char_num = CHAR_NUMS,
        .p_char_db = p_lock_char,
    };
    
    // add lock operation char  
    memcpy(uuid.uuid128, (uint8_t [])BLE_UUID_LOCK_OPS, 16);
    p_lock_char[CHAR_OPERATION_IDX] = (mible_gatts_char_db_t){
        .char_uuid = uuid,
        .char_property = MIBLE_WRITE,
        .char_value_len = sizeof(lock_operation),
        .wr_author = 1
    };

    // add lock state char
    memcpy(uuid.uuid128, (uint8_t [])BLE_UUID_LOCK_STATE, 16);
    p_lock_char[CHAR_STATE_IDX] = (mible_gatts_char_db_t){
        .char_uuid = uuid,
        .char_property = MIBLE_READ | MIBLE_NOTIFY,
        .char_value_len = sizeof(lock_state),
        .rd_author = 1
    };

    // add lock logs char
    memcpy(uuid.uuid128, (uint8_t [])BLE_UUID_LOCK_LOGS, 16);
    p_lock_char[CHAR_LOGS_IDX] = (mible_gatts_char_db_t){
        .char_uuid = uuid, 
        .char_property = MIBLE_READ | MIBLE_NOTIFY,
        .char_value_len = sizeof(lock_logs),
        .is_variable_len = true,
        .rd_author = 1
    };

    err_code = mible_gatts_service_init(&mi_gatts_db);
    MI_ERR_CHECK(err_code);

    return err_code;
}


int reply_lock_stat(uint8_t status)
{
    uint32_t errno;
    uint8_t  tx_buf[20] = {0};
    uint8_t  tx_len     = 0;

    if (m_lock_srv.conn_handle == BLE_CONN_HANDLE_INVALID)
        return -1;
    
    memcpy(tx_buf, &status, sizeof(status));
    tx_len += sizeof(status);

    errno = mi_session_encrypt(tx_buf, tx_len, tx_buf);
    if (errno != MI_SUCCESS)
        return -2;
    else
        tx_len += 6;                      /* 6 = 2 bytes SEQNUM + 4 bytes MIC */

    errno = mible_gatts_notify_or_indicate(m_lock_srv.conn_handle, 0, m_lock_srv.state_handle,
                                    0, tx_buf, tx_len, 1);
    if (errno != MI_SUCCESS)
        return -3;

    return errno;
}


int send_lock_log(uint16_t type, uint8_t vlen, void *value)
{
    uint32_t errno;
    uint8_t  tx_buf[20] = {0};
    uint8_t  tx_len     = 0;

    if (m_lock_srv.conn_handle == BLE_CONN_HANDLE_INVALID)
        return -1;
    
    memcpy(tx_buf, &type, sizeof(type));
    tx_len += sizeof(type);

    memcpy(tx_buf + 2, value, vlen); 
    tx_len += vlen;

    errno = mi_session_encrypt(tx_buf, tx_len, tx_buf);
    if (errno != MI_SUCCESS)
        return -2;
    else
        tx_len += 6;                      /* 6 = 2 bytes SEQNUM + 4 bytes MIC */

    errno = mible_gatts_notify_or_indicate(m_lock_srv.conn_handle, 0, m_lock_srv.logs_handle,
                                    0, tx_buf, tx_len, 1);
    if (errno != MI_SUCCESS)
        return -3;

    return errno;
}

