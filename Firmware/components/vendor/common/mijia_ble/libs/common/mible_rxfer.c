//#include <stdint.h>
#include <stdarg.h>
//#include <string.h>

#include "mible_api.h"
#include "mible_port.h"

#undef  MI_LOG_MODULE_NAME
#define MI_LOG_MODULE_NAME "RXFER"
#include "mible_log.h"

#include "mible_rxfer.h"

#define ENABLE_RXFER_VERBOSE            0

#define MODE_CMD                        0
#define MODE_ACK                        1
#define FRAME_CTRL                      0
#define BLE_CONN_HANDLE_INVALID         0xFFFF
#define MAGIC_NUM                       0xA5
#define MAX_CONSECTIVE_INTERVAL         200
#define CEIL_DIV(A, B)           (((A) + (B) - 1) / (B))

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

static uint8_t is_valid_pkt(void *p_in, uint8_t len)
{
    uint8_t magic[len];
    memset(magic, MAGIC_NUM, len);
    if (memcmp(magic, p_in, len) == 0)
        return 0;
    else
        return 1;
}

static uint16_t find_lost_sn(rxfer_cb_t *pxfer)
{
    static uint16_t checked_sn = 1;
    uint8_t (*p_pkt)[18] = (void*)pxfer->pdata;

    p_pkt += checked_sn - 1;
    // <!> vulnerable check : word-read unaligned address may cause mem-hardfault
    // TODO:  refine the big data transfer protocol
    //        or just think about what if we lost the last packet ?
    while ( checked_sn < pxfer->rx_num && is_valid_pkt(p_pkt, 18)) {
        checked_sn++;
        p_pkt++;
    }

    // only check last rx bytes
    if (checked_sn == pxfer->rx_num && pxfer->last_rx_bytes != 0) {
        checked_sn++;
    }

    if (checked_sn > pxfer->rx_num) {
        checked_sn = 1;
        return 0;
    }
    else {
        return checked_sn;
    }
}
extern uint8_t cert[512];
_attribute_ram_code_ static uint8_t rxfer_rx_decode(rxfer_cb_t *pxfer, uint8_t *pdata, uint8_t len)
{
    reliable_xfer_frame_t      *pframe = (void*)pdata;
    int8_t                    data_len = len - sizeof(pframe->sn);

    if (data_len > 0)
        memcpy(pxfer->pdata + (pframe->sn - 1) * 18, pframe->data, data_len);
    else
        MI_LOG_ERROR("rxd data len error. \n");

    return data_len > 0 ? data_len : 0;
}

int format_rx_cb(rxfer_cb_t *pxfer, void *p_rxd, uint16_t rxd_bytes)
{
    uint8_t last_bytes = rxd_bytes % 18;
    pxfer->pdata = p_rxd;
    pxfer->max_rx_num = CEIL_DIV(rxd_bytes, 18);
    pxfer->last_bytes  = last_bytes == 0 ? 18 : last_bytes;

    // padding magic num
    memset(p_rxd, MAGIC_NUM, rxd_bytes);

    return 0;
}

int format_tx_cb(rxfer_cb_t *pxfer, void *p_txd, uint16_t txd_bytes)
{
    uint8_t last_bytes = txd_bytes % 18;
    pxfer->pdata = p_txd;
    pxfer->tx_num = CEIL_DIV(txd_bytes, 18);
    pxfer->last_bytes  = last_bytes == 0 ? 18 : last_bytes;
    return 0;
}

static int rxfer_tx_cmd(rxfer_cb_t *pxfer, fctrl_cmd_t cmd, ...)
{
    reliable_xfer_frame_t       frame = {0};
    uint16_t                 data_len;
    uint32_t                    errno;
    uint16_t                      arg;

    frame.ctrl.mode = MODE_CMD;
    frame.ctrl.type =      cmd;

    va_list ap;
    va_start(ap, cmd);
    arg = va_arg(ap, int);
    if ( arg != 0 ) {
        //*(uint16_t*)frame.ctrl.arg = arg;
        memcpy(frame.ctrl.arg,&arg,sizeof(arg));
    }
    va_end(ap);

    data_len = sizeof(frame.sn) + sizeof(frame.ctrl);

    // TODO : exception handler
    if (pxfer->conn_handle == BLE_CONN_HANDLE_INVALID)
        MI_LOG_ERROR("Exception disconnect in BLE.\n");

    errno = mible_gatts_notify_or_indicate(pxfer->conn_handle, 0, pxfer->value_handle,
                                   0, (void*) &frame, data_len, 1);
#if (ENABLE_RXFER_VERBOSE == 1)
    if (errno != MI_SUCCESS) {
        MI_LOG_ERROR("Cann't send CMD %X : %d\n", cmd, errno);
    } else {
        MI_LOG_ERROR("CMD ");
        MI_LOG_HEXDUMP(&frame, data_len);
    }
#endif

    return errno;
}

static int rxfer_tx_data(rxfer_cb_t *pxfer, uint16_t sn)
{
    reliable_xfer_frame_t       frame = {0};
    uint16_t                 data_len;
    uint32_t                    errno;

    uint8_t      (*pdata)[18] = (void*)pxfer->pdata;

    frame.sn = sn;
    pdata   += sn - 1;

    if (sn == pxfer->tx_num) {
        data_len = pxfer->last_bytes;
    }
    else {
        data_len = sizeof(frame.data);
    }

    memcpy(frame.data, pdata, data_len);

    data_len += sizeof(frame.sn);

    // TODO : exception handler
    if (pxfer->conn_handle == BLE_CONN_HANDLE_INVALID)
        MI_LOG_ERROR("Exception disconnect in BLE.\n");

    errno = mible_gatts_notify_or_indicate(pxfer->conn_handle, 0, pxfer->value_handle,
                                   0, (void*) &frame, data_len, 1);
#if (ENABLE_RXFER_VERBOSE == 1)
    if (errno != MI_SUCCESS) {
        MI_LOG_INFO("Cann't send pkt %d: %X\n", sn, errno);
    } else {
        MI_LOG_HEXDUMP(&frame, data_len);
    }
#endif

    return errno;
}

static int rxfer_rx_ack(rxfer_cb_t *pxfer, fctrl_ack_t ack, ...)
{
    reliable_xfer_frame_t       frame = {0};
    uint16_t                 data_len;
    uint32_t                    errno;

    frame.ctrl.mode = MODE_ACK;
    frame.ctrl.type =      ack;
    data_len = sizeof(frame.sn) + sizeof(frame.ctrl.type) + sizeof(frame.ctrl.mode);

    if (ack == A_LOST) {
        va_list ap;
        va_start(ap, ack);
        uint16_t arg = va_arg(ap, int);
        if ( arg != 0 ) {
            //*(uint16_t*)frame.ctrl.arg = arg;
        	memcpy(frame.ctrl.arg,&arg,sizeof(arg));
            data_len += sizeof(frame.ctrl.arg);
        }
        va_end(ap);
    }

    // TODO : exception handler
    if (pxfer->conn_handle == BLE_CONN_HANDLE_INVALID) {
        MI_LOG_ERROR("Exception disconnect in BLE.\n");
        return 0;
    }
    errno = mible_gatts_notify_or_indicate(pxfer->conn_handle, 0, pxfer->value_handle,
                                   0, (void*) &frame, data_len, 1);
#if (ENABLE_RXFER_VERBOSE == 1)
    if (errno != MI_SUCCESS) {
        MI_LOG_ERROR("Cann't send ACK %x: %X\n", ack, errno);
        // TODO : catch the exception.
    } else {
        MI_LOG_ERROR("ACK ");
        MI_LOG_HEXDUMP(&frame, data_len);
    }
#endif

    return errno;
}

int rxfer_init(rxfer_cb_t *pxfer, pstimer_t *p_timer)
{
    memset(pxfer, 0, sizeof(rxfer_cb_t));
    pxfer->state = RXFER_WAIT_CMD;
    pxfer->p_timer = p_timer;
    return 0;
}

void rxfer_process(rxfer_cb_t *pxfer, uint8_t *pdata, uint16_t len)
{
    reliable_xfer_frame_t *pframe = (void*)pdata;
    uint16_t  curr_sn = pframe->sn;

    if (curr_sn == FRAME_CTRL ) {
        if (pxfer->state == RXFER_WAIT_CMD && pframe->ctrl.mode == MODE_CMD) {
            fctrl_cmd_t cmd = (fctrl_cmd_t)pframe->ctrl.type;
            pxfer->mode = MODE_CMD;
            pxfer->cmd = cmd;
            switch (cmd) {
            case PASS_THROUGH:
            case ECC_PUBKEY:
            case DEV_LOGIN_INFO:
            case DEV_SHARE_INFO:
            case SERVER_CERT:
            case SERVER_SIGN:
            case MESH_CONFIG:
            case APP_CONFIRMATION:
            case APP_RANDOM:
            case DEV_CONFIRMATION:
            case DEV_RANDOM:
            case BIND_KEY:
                pxfer->rx_num = *(uint16_t*)(pframe->ctrl.arg);
                break;
            default:
                MI_LOG_ERROR("Unknow rxfer CMD.\n");
            }
        } else if (pxfer->state == RXFER_WAIT_ACK && pframe->ctrl.mode == MODE_ACK) {
            fctrl_ack_t ack = (fctrl_ack_t)pframe->ctrl.type;
            pxfer->mode = MODE_ACK;
            pxfer->ack = ack;
            switch (ack) {
            case A_SUCCESS:
                pxfer->curr_sn = 0;
                pxfer->state = RXFER_WAIT_CMD;
                break;
            case A_READY:
                pxfer->curr_sn = 0;
                pxfer->state = RXFER_TXD;
                break;
            case A_LOST:
                pxfer->curr_sn = *(uint16_t*)pframe->ctrl.arg;
                break;
            default:
                MI_LOG_ERROR("Unknown rxfer ACK.\n");
            }
        } else {
            MI_LOG_ERROR("recv malformed packet !\n");
            // malware
            // TODO: handle this exception...
        }
    }
    else if (pxfer->state == RXFER_RXD) {
        pxfer->curr_sn = curr_sn;
        if (curr_sn < pxfer->rx_num && len == 20) {
            rxfer_rx_decode(pxfer, pdata, 20);
        } else if (curr_sn == pxfer->rx_num) {
            if (pxfer->rx_num == pxfer->max_rx_num) {
                pxfer->last_rx_bytes =
                rxfer_rx_decode(pxfer, pdata, MIN(len, pxfer->last_bytes+2));
            } else {
                pxfer->last_rx_bytes =
                rxfer_rx_decode(pxfer, pdata, len);
            }
        } else {
            MI_LOG_ERROR("recv illegal rxfer data. SN:%d %d\n", curr_sn, len);
            pxfer->curr_sn = 0;
            // TODO: handle this exception...
        }
    }
}

_attribute_ram_code_ int rxfer_rx_thd(pt_t *pt, rxfer_cb_t *pxfer, uint8_t data_type, ...)
{
    static uint16_t sn;
    PT_BEGIN(pt);

    sn = 0;
    /* Recive data */
    PT_WAIT_UNTIL(pt, pxfer->rx_num != 0 && pxfer->cmd == data_type);
    if (pxfer->rx_num <= pxfer->max_rx_num && pxfer->pdata != NULL) {
        PT_WAIT_UNTIL(pt, rxfer_rx_ack(pxfer, A_READY) == MI_SUCCESS);
/*
        static uint8_t retries_num;
        for (retries_num = 0; retries_num < 3; retries_num++) {
            if (reliable_xfer_ack(A_READY) == MI_SUCCESS)
                break;
            else
                PT_YIELD(pt);
        }
        if (retries_num == 3) PT_EXIT(pt);
 */ 
        pxfer->state = RXFER_RXD;

    } else {
        PT_WAIT_UNTIL(pt, rxfer_rx_ack(pxfer, A_CANCEL) == MI_SUCCESS);
        pxfer->rx_num = 0;
        PT_RESTART(pt);
    }

    pstimer_set(pxfer->p_timer, MAX_CONSECTIVE_INTERVAL);
    while (sn < pxfer->rx_num && !pstimer_expired(pxfer->p_timer, NULL)) {
        if (pxfer->curr_sn > sn ) {
            sn = pxfer->curr_sn;
            pstimer_set(pxfer->p_timer, MAX_CONSECTIVE_INTERVAL);
        }
        PT_YIELD(pt);
    }

    while(1) {
        sn = find_lost_sn(pxfer);
        if (sn == 0) {
            PT_WAIT_UNTIL(pt, rxfer_rx_ack(pxfer, A_SUCCESS) == MI_SUCCESS);
            break;
        } else {
            MI_LOG_ERROR("lost packet %d.\n", sn);
            PT_WAIT_UNTIL(pt, rxfer_rx_ack(pxfer, A_LOST, sn) == MI_SUCCESS);
            PT_WAIT_UNTIL(pt, pxfer->curr_sn == sn);
        }
    }

    if (data_type == SERVER_CERT ||
        data_type == PASS_THROUGH ||
        data_type == MESH_CONFIG) {
        va_list ap;
        va_start(ap, data_type);
        uint16_t * arg = (uint16_t*)(va_arg(ap, int));
        if ( arg != NULL ) {
            *arg = 18 * (pxfer->rx_num - 1) + pxfer->last_rx_bytes;
        }
        va_end(ap);
    }

    pxfer->state         = RXFER_WAIT_CMD;
    pxfer->cmd           = 0;
    pxfer->last_rx_bytes = 0;
    pxfer->rx_num        = 0;
    pxfer->pdata         = 0;
    pxfer->curr_sn       = 0;
    PT_END(pt);
}

_attribute_ram_code_ int rxfer_tx_thd(pt_t *pt, rxfer_cb_t *pxfer, uint8_t data_type)
{
    static uint16_t sn;
    PT_BEGIN(pt);

    /* Send data */
    PT_WAIT_UNTIL(pt, rxfer_tx_cmd(pxfer, (fctrl_cmd_t)data_type, pxfer->tx_num) == MI_SUCCESS);

    pxfer->state = RXFER_WAIT_ACK;
    pxfer->mode  = 0xFF;
    PT_WAIT_UNTIL(pt, pxfer->mode == MODE_ACK);

    if (pxfer->ack != A_READY)
        PT_RESTART(pt);

    sn = 1;

    while(sn <= pxfer->tx_num) {
        PT_WAIT_UNTIL(pt, rxfer_tx_data(pxfer, sn) == MI_SUCCESS);
        sn++;
    }

    pxfer->state = RXFER_WAIT_ACK;

    while(pxfer->mode == MODE_ACK && pxfer->ack != A_SUCCESS) {
        PT_WAIT_UNTIL(pt, pxfer->curr_sn != 0 || pxfer->ack == A_SUCCESS);
        if (pxfer->ack == A_SUCCESS) {
            break;
        }
        else if(pxfer->curr_sn <= pxfer->tx_num) {
            PT_WAIT_UNTIL(pt, rxfer_tx_data(pxfer, pxfer->curr_sn) == MI_SUCCESS);
        }
        pxfer->curr_sn = 0;
    }

    pxfer->state = RXFER_WAIT_CMD;
    pxfer->tx_num = 0;
    PT_END(pt);
}
