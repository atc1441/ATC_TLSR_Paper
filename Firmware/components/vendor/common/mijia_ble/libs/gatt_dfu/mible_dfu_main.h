/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __MI_DFU_MAIN_H__
#define __MI_DFU_MAIN_H__

/* Includes ------------------------------------------------------------------*/

#include "mible_type.h"

/* Exported define -----------------------------------------------------------*/

#define MIBLE_DFU_PROTOCAL_VERSION                              0x0001
#define MIBLE_DFU_FRAGMENT_SIZE                                 512

/* Exported types ------------------------------------------------------------*/

typedef struct {
    uint16_t conn_handle;
    uint16_t srv_handle;
    uint16_t ctrl_handle;
    uint16_t data_handle;
    uint16_t product_id;
    uint32_t flash_start;
    uint32_t flash_size;
} mible_dfu_init_t;

typedef enum {
    MIBLE_DFU_STATE_START,
    MIBLE_DFU_STATE_VERIFY,
    MIBLE_DFU_STATE_SWITCH,
    MIBLE_DFU_STATE_CANCEL,
} mible_dfu_state_t;

typedef enum {
    MIBLE_DFU_VERIFY_SUCC,
    MIBLE_DFU_VERIFY_FAIL,
} mible_dfu_verify_t;

typedef struct {
    uint16_t fragment_size;
} mible_dfu_start_param_t;

typedef struct {
    mible_dfu_verify_t value;
} mible_dfu_verify_param_t;

typedef union {
    mible_dfu_start_param_t start;
    mible_dfu_verify_param_t verify;
} mible_dfu_param_t;

typedef void (* mible_dfu_callback_t) (mible_dfu_state_t state, mible_dfu_param_t *param);

/* Exported constants --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

mible_status_t mible_dfu_init(mible_dfu_init_t * p_init);
mible_status_t mible_dfu_ctrl(uint8_t * buffer, uint16_t size);
mible_status_t mible_dfu_data(uint8_t * buffer, uint16_t size);
mible_status_t mible_dfu_upgrade(void);
mible_status_t mible_dfu_callback_register(mible_dfu_callback_t cb);
mible_status_t mible_dfu_callback_unregister(mible_dfu_callback_t cb);

#endif
