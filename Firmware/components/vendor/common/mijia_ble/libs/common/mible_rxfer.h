#ifndef __MIBLE_RXFER_H__
#define __MIBLE_RXFER_H__
#include <stdint.h>
#include "third_party/pt/pt.h"
#include "third_party/pt/pt_misc.h"
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ATT_MTU         20

typedef enum {
    PASS_THROUGH          = 0x00,
    DEV_CERT              = 0x01,
    DEV_MANU_CERT         = 0x02,
    ECC_PUBKEY            = 0x03,
    DEV_SIGNATURE         = 0x04,
    DEV_LOGIN_INFO        = 0x05,
    DEV_SHARE_INFO        = 0x06,
    SERVER_CERT           = 0x07,
    SERVER_SIGN           = 0x08,
    MESH_CONFIG           = 0x09,
    APP_CONFIRMATION      = 0x0A,
    APP_RANDOM            = 0x0B,
    DEV_CONFIRMATION      = 0x0C,
    DEV_RANDOM            = 0x0D,
    BIND_KEY              = 0x0E,
} fctrl_cmd_t;

typedef enum {
    A_SUCCESS             = 0x00,
    A_READY,
    A_BUSY,
    A_TIMEOUT,
    A_CANCEL,
    A_LOST
} fctrl_ack_t;

typedef struct {
    uint8_t mode;
    uint8_t type;
    uint8_t arg[2];
} reliable_fctrl_t;

typedef struct {
    uint16_t sn;
    union {
        uint8_t              data[MAX_ATT_MTU-2];
        reliable_fctrl_t     ctrl;
    };
} reliable_xfer_frame_t;

typedef enum {
    RXFER_WAIT_CMD,
    RXFER_WAIT_ACK,

    RXFER_TXD,
    RXFER_RXD,

    RXFER_ERROR = 0xFF
} rxfer_stat_t;

typedef struct {
    uint16_t       curr_sn;
    uint16_t        tx_num;
    uint16_t    max_rx_num;
    uint16_t        rx_num;

    uint16_t   conn_handle;
    uint16_t    srv_handle;
    uint16_t  value_handle;

    rxfer_stat_t     state;
    uint8_t           mode;
    uint8_t            cmd;
    uint8_t            ack;

    uint8_t  last_rx_bytes;
    uint8_t     last_bytes;

    uint8_t         *pdata;
    pstimer_t     *p_timer;

} rxfer_cb_t;


int format_rx_cb(rxfer_cb_t *pxfer, void *p_rxd, uint16_t rxd_bytes);
int format_tx_cb(rxfer_cb_t *pxfer, void *p_txd, uint16_t txd_bytes);

int rxfer_init(rxfer_cb_t *pxfer, pstimer_t *p_timer);
void rxfer_process(rxfer_cb_t *pxfer, uint8_t *pdata, uint16_t len);
int rxfer_rx_thd(pt_t *pt, rxfer_cb_t *pxfer, uint8_t data_type, ...);
int rxfer_tx_thd(pt_t *pt, rxfer_cb_t *pxfer, uint8_t data_type);

#ifdef __cplusplus
}
#endif

/** @} */

#endif  /* __MIBLE_RXFER_H__ */ 
