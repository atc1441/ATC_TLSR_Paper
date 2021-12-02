#ifndef MIJIA_BLE_LIBS_CRYPTOGRAPHY_MI_MESH_OTP_H_
#define MIJIA_BLE_LIBS_CRYPTOGRAPHY_MI_MESH_OTP_H_
#include <stdbool.h>
#include <stdint.h>

/*  OTP_ITEM_TYPE  */
#define OTP_MARK_DIRTY      0x00
#define OTP_DEV_CERT        0x01
#define OTP_MANU_CERT       0x02
#define OTP_ROOT_CERT       0x03
#define OTP_DEV_CERT_PRI    0x04
#define OTP_BLE_MAC         0x10
#define OTP_META_DATA       0xF1
#define OTP_FREE_ITEM       0xFFFF

typedef int (*uart_sync_tx_t)(uint8_t const * const out, uint16_t olen);
typedef int (*uart_sync_rx_t)(uint8_t * const in, uint16_t max_ilen);
typedef void (*dbg_flash_lock_func_t)(void);

int mi_mesh_otp_seal_tag(uint8_t * base);
bool mi_mesh_otp_is_existed(void);
int mi_mesh_otp_manufacture_init(uart_sync_tx_t tx, uart_sync_rx_t rx,
        dbg_flash_lock_func_t lock_flash_fn, dbg_flash_lock_func_t lock_debug_fn);
int mi_mesh_otp_write(uint16_t item_type, const uint8_t *p_in, uint16_t in_len);
int mi_mesh_otp_read(uint16_t item_type, uint8_t *p_out, uint16_t max_olen);
int mi_mesh_otp_program(void);
int mi_mesh_otp_program_simulation(void);
int mi_mesh_otp_verify(void);

#endif /* MIJIA_BLE_LIBS_CRYPTOGRAPHY_MI_MESH_OTP_H_ */
