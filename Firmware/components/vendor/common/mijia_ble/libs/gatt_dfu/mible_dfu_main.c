/* Includes ------------------------------------------------------------------*/
//#include <stddef.h>
//#include <string.h>
#include "mible_api.h"
#include "mible_log.h"
#include "common/mible_rxfer.h"
#include "common/queue.h"
#include "mible_dfu_auth.h"
#include "mible_dfu_flash.h"
#include "mible_dfu_main.h"

/* Private define ------------------------------------------------------------*/

#define MIBLE_DFU_COMMAND_READ_VERSION                         0x0001
#define MIBLE_DFU_COMMAND_READ_SIZE                            0x0002
#define MIBLE_DFU_COMMAND_READ_INDEX                           0x0003
#define MIBLE_DFU_COMMAND_ENABLE_SWITCH                        0x0004

#define MIBLE_DFU_EVENT_CMD_COMPLETE                           0x01
#define MIBLE_DFU_EVENT_TRANS_COMPLETE                         0x02

#define MIBLE_DFU_STATUS_SUCC                                  0x00
#define MIBLE_DFU_STATUS_ERR_NO_CMD                            0x01
#define MIBLE_DFU_STATUS_ERR_AUTH_FAIL                         0x02
#define MIBLE_DFU_STATUS_ERR_INVALID                           0x03
#define MIBLE_DFU_STATUS_ERR_NO_MEM                            0x04
#define MIBLE_DFU_STATUS_ERR_BUSY                              0x05
#define MIBLE_DFU_STATUS_ERR_UNKNOWN                           0x06

#define HI_UINT16(a)                            (((uint16_t)(a) >> 8) & 0xFF)
#define LO_UINT16(a)                            ((uint16_t)(a) & 0xFF)

#define MIBLE_DFU_TIMER_PERIOD                                 10

#define MIBLE_DFU_EVT_SIZE_MAX                                 20
#define MIBLE_DFU_EVT_BUF_MAX                                  2

#define MIBLE_DFU_STORE_RETRY_MAX                              3

#define MIBLE_DFU_CALLBACK_MAX                                 4

/* Private typedef -----------------------------------------------------------*/

typedef struct {
    mible_dfu_init_t init;
    uint16_t current_index;
    uint16_t current_size;
    uint32_t switch_delay;
    uint32_t total_size;
    void *   timer_id;
    rxfer_cb_t rxfer_dfu;
    pt_t     pt_rx_ctrl;
    pt_t     pt_sar_ctrl;
    pt_t     pt_evt_ctrl;
    pt_t     pt_delay_ctrl;
    pt_t     pt_flash_ctrl;
    struct pt_sem sem_evt;
    struct pt_sem sem_delay;
    struct pt_sem sem_flash;
    queue_t  evt_queue;
} mible_dfu_config_t;

#if defined ( __CC_ARM )

__PACKED struct mible_dfu_cmd {
    uint16_t command;
    uint8_t  length;
    uint8_t  param[1] ;
};
typedef struct mible_dfu_cmd mible_dfu_cmd_t;

__PACKED struct mible_dfu_evt {
    uint8_t event;
    uint8_t length;
    uint8_t param[1];
};
typedef struct mible_dfu_evt mible_dfu_evt_t;

__PACKED struct mible_dfu_fragment {
    uint16_t index;
    uint8_t data[MIBLE_DFU_FRAGMENT_SIZE];
};
typedef struct mible_dfu_fragment mible_dfu_fragment_t;

#elif defined   ( __GNUC__ )

struct __PACKED mible_dfu_cmd {
    uint16_t command;
    uint8_t  length;
    uint8_t  param[1] ;
};
typedef struct mible_dfu_cmd mible_dfu_cmd_t;

struct __PACKED mible_dfu_evt {
    uint8_t event;
    uint8_t length;
    uint8_t param[1];
};
typedef struct mible_dfu_evt mible_dfu_evt_t;

struct __PACKED mible_dfu_fragment {
    uint16_t index;
    uint8_t data[MIBLE_DFU_FRAGMENT_SIZE];
};
typedef struct mible_dfu_fragment mible_dfu_fragment_t;

#endif

/* Private function prototypes -----------------------------------------------*/

static mible_status_t mible_dfu_send_evt(uint8_t event, uint8_t * param, uint16_t len);
static void mible_dfu_timer_callback(void * arg);
static int  mible_dfu_fragment_handler(pt_t * pt);
static int  mible_dfu_sar_handler(pt_t * pt);
static int  mible_dfu_evt_handler(pt_t * pt);
static int  mible_dfu_delay_handler(pt_t * pt);
static void mible_dfu_reset(void);

/* Private variables ---------------------------------------------------------*/

static int retry_num;
static mible_dfu_config_t dfu_config;
static mible_dfu_fragment_t dfu_fragment;
static uint8_t evt_buffer[MIBLE_DFU_EVT_SIZE_MAX * MIBLE_DFU_EVT_BUF_MAX];
static uint32_t dfu_timer_tick;
static pstimer_t dfu_timer = {
        .p_curr_tick = &dfu_timer_tick,
        .tick_interval_ms = MIBLE_DFU_TIMER_PERIOD,
};
static mible_dfu_callback_t dfu_callback[MIBLE_DFU_CALLBACK_MAX] = {0};
static bool ota_downloading = false;

/* Exported functions --------------------------------------------------------*/

mible_status_t mible_dfu_upgrade(void)
{
    int index;
    mible_status_t ret = MI_SUCCESS;

    if (0 == dfu_config.switch_delay) {
        MI_LOG_DEBUG("upgrade right now\n");
        for (index = 0; index < MIBLE_DFU_CALLBACK_MAX; ++index) {
            if (dfu_callback[index]) {
                dfu_callback[index](MIBLE_DFU_STATE_SWITCH, NULL);
            }
        }
        ret = mible_upgrade_firmware();
    } else if (~0 != dfu_config.switch_delay) {
        mible_timer_stop(dfu_config.timer_id);
        PT_SEM_SIGNAL(0, &dfu_config.sem_delay);
        mible_timer_start(dfu_config.timer_id, 1000, NULL);
    } else {
        mible_timer_delete(dfu_config.timer_id);
        dfu_config.timer_id = NULL;
        if (ota_downloading) {
            MI_LOG_WARNING("no valid firmware\n");
            for (index = 0; index < MIBLE_DFU_CALLBACK_MAX; ++index) {
                if (dfu_callback[index]) {
                    dfu_callback[index](MIBLE_DFU_STATE_CANCEL, NULL);
                }
            }
        }
    }

    ota_downloading = false;

    return ret;
}

mible_status_t mible_dfu_init(mible_dfu_init_t * p_init)
{
    mible_status_t ret;

    if (NULL == p_init) {
        return MI_ERR_INVALID_PARAM;
    }

    /* Init parameters for DFU config */
    dfu_config.init = *p_init;
    mible_dfu_reset();
    ret = mible_dfu_flash_init(p_init->flash_start, p_init->flash_size);
    if (MI_SUCCESS != ret) {
        MI_LOG_ERROR("Fail to init flash (err %d)\n", ret);
        return ret;
    }

    /* Init queue for sending event */
    ret = queue_init(&dfu_config.evt_queue, evt_buffer,
                     MIBLE_DFU_EVT_BUF_MAX, MIBLE_DFU_EVT_SIZE_MAX);
    if (MI_SUCCESS != ret) {
        MI_LOG_ERROR("Fail to init queue for event (err %d)\n", ret);
        return ret;
    }

    /* Init parameters for Segmentation And Reassembly */
    rxfer_init(&dfu_config.rxfer_dfu, &dfu_timer);
    dfu_config.rxfer_dfu.conn_handle  = p_init->conn_handle;
    dfu_config.rxfer_dfu.srv_handle   = p_init->srv_handle;
    dfu_config.rxfer_dfu.value_handle = p_init->data_handle;
    format_rx_cb(&dfu_config.rxfer_dfu, &dfu_fragment, sizeof(dfu_fragment));

    /* Init protothread objects */
    PT_INIT(&dfu_config.pt_sar_ctrl);
    PT_INIT(&dfu_config.pt_evt_ctrl);
    PT_INIT(&dfu_config.pt_delay_ctrl);
    PT_INIT(&dfu_config.pt_flash_ctrl);
    PT_SEM_INIT(&dfu_config.sem_evt, 0);
    PT_SEM_INIT(&dfu_config.sem_delay, 0);
    PT_SEM_INIT(&dfu_config.sem_flash, 0);

    /* Init and enable timer */
    if (NULL != dfu_config.timer_id) {
        mible_timer_delete(dfu_config.timer_id);
        dfu_config.timer_id = NULL;
    }
    ret = mible_timer_create(&dfu_config.timer_id, mible_dfu_timer_callback,
                             MIBLE_TIMER_REPEATED);
    if (MI_SUCCESS != ret) {
        MI_LOG_ERROR("Fail to create timer.\n");
        return ret;
    }

    return MI_SUCCESS;
}

mible_status_t mible_dfu_ctrl(uint8_t * buffer, uint16_t size)
{
    mible_dfu_cmd_t * p_cmd = (mible_dfu_cmd_t *)buffer;
    mible_status_t ret = MI_ERR_INVALID_PARAM;
    mible_dfu_param_t dfu_param;
    int index;

    if (NULL == p_cmd ||
        size < offsetof(mible_dfu_cmd_t, param) ||
        size != offsetof(mible_dfu_cmd_t, param) + p_cmd->length) {

        return ret;
    }

    switch (p_cmd->command) {
        case MIBLE_DFU_COMMAND_READ_VERSION: {
            uint8_t param[5];
            param[0] = LO_UINT16(p_cmd->command);
            param[1] = HI_UINT16(p_cmd->command);
            param[2] = MIBLE_DFU_STATUS_SUCC;
            param[3] = LO_UINT16(MIBLE_DFU_PROTOCAL_VERSION);
            param[4] = HI_UINT16(MIBLE_DFU_PROTOCAL_VERSION);
            ret = mible_dfu_send_evt(MIBLE_DFU_EVENT_CMD_COMPLETE, param, sizeof(param));
            break;
        }
        case MIBLE_DFU_COMMAND_READ_SIZE: {
            uint8_t param[5];
            param[0] = LO_UINT16(p_cmd->command);
            param[1] = HI_UINT16(p_cmd->command);
            param[2] = MIBLE_DFU_STATUS_SUCC;
            param[3] = LO_UINT16(MIBLE_DFU_FRAGMENT_SIZE);
            param[4] = HI_UINT16(MIBLE_DFU_FRAGMENT_SIZE);
            mible_dfu_reset();
            mible_timer_start(dfu_config.timer_id, MIBLE_DFU_TIMER_PERIOD, NULL);
            ret = mible_dfu_send_evt(MIBLE_DFU_EVENT_CMD_COMPLETE, param, sizeof(param));
            dfu_param.start.fragment_size = MIBLE_DFU_FRAGMENT_SIZE;
            ota_downloading = true;
            for (index = 0; index < MIBLE_DFU_CALLBACK_MAX; ++index) {
                if (dfu_callback[index]) {
                    dfu_callback[index](MIBLE_DFU_STATE_START, &dfu_param);
                }
            }
            break;
        }
        case MIBLE_DFU_COMMAND_READ_INDEX: {
            uint8_t param[5];
            param[0] = LO_UINT16(p_cmd->command);
            param[1] = HI_UINT16(p_cmd->command);
            param[2] = MIBLE_DFU_STATUS_SUCC;
            param[3] = LO_UINT16(dfu_config.current_index);
            param[4] = HI_UINT16(dfu_config.current_index);
            ret = mible_dfu_send_evt(MIBLE_DFU_EVENT_CMD_COMPLETE, param, sizeof(param));
            break;
        }
        case MIBLE_DFU_COMMAND_ENABLE_SWITCH: {
            uint8_t param[3];
            param[0] = LO_UINT16(p_cmd->command);
            param[1] = HI_UINT16(p_cmd->command);
            dfu_param.verify.value = MIBLE_DFU_VERIFY_FAIL;
            if (sizeof(dfu_config.switch_delay) == p_cmd->length) {
                MI_LOG_DEBUG("begin verify, fw_size: 0x%X, pid: %d\n",
                		dfu_config.total_size, dfu_config.init.product_id);
                ret = mible_dfu_auth(dfu_config.total_size, dfu_config.init.product_id);
                if (MI_SUCCESS == ret) {
                    dfu_param.verify.value = MIBLE_DFU_VERIFY_SUCC;
                    param[2] = MIBLE_DFU_STATUS_SUCC;
                    memcpy(&dfu_config.switch_delay, p_cmd->param, p_cmd->length);
                    MI_LOG_DEBUG("delay %d seconds\n", dfu_config.switch_delay);
                } else if (MI_ERR_INTERNAL == ret) {
                    param[2] = MIBLE_DFU_STATUS_ERR_AUTH_FAIL;
                } else {
                    param[2] = MIBLE_DFU_STATUS_ERR_UNKNOWN;
                }
            } else {
                param[2] = MIBLE_DFU_STATUS_ERR_INVALID;
            }
            ret = mible_dfu_send_evt(MIBLE_DFU_EVENT_CMD_COMPLETE, param, sizeof(param));
            for (index = 0; index < MIBLE_DFU_CALLBACK_MAX; ++index) {
                if (dfu_callback[index]) {
                    dfu_callback[index](MIBLE_DFU_STATE_VERIFY, &dfu_param);
                }
            }
            break;
        }
        default: {
            uint8_t param[3];
            param[0] = LO_UINT16(p_cmd->command);
            param[1] = HI_UINT16(p_cmd->command);
            param[2] = MIBLE_DFU_STATUS_ERR_NO_CMD;
            ret = mible_dfu_send_evt(MIBLE_DFU_EVENT_CMD_COMPLETE, param, sizeof(param));
            break;
        }
    }

    return ret;
}

mible_status_t mible_dfu_data(uint8_t * buffer, uint16_t size)
{
    rxfer_process(&dfu_config.rxfer_dfu, buffer, size);
    return MI_SUCCESS;
}

mible_status_t mible_dfu_callback_register(mible_dfu_callback_t cb)
{
    int index;
    mible_status_t ret;

    if (NULL == cb) {
        return MI_ERR_INVALID_PARAM;
    }

//    CRITICAL_SECTION_ENTER();
    for (index = 0; index < MIBLE_DFU_CALLBACK_MAX; ++index) {
        if (cb == dfu_callback[index]) {
            ret = MI_SUCCESS;
            goto end;
        }
    }

    for (index = 0; index < MIBLE_DFU_CALLBACK_MAX; ++index) {
        if (NULL == dfu_callback[index]) {
            dfu_callback[index] = cb;
            ret = MI_SUCCESS;
            goto end;
        }
    }

    ret = MI_ERR_RESOURCES;

end:
 //   CRITICAL_SECTION_EXIT();

    return ret;
}

mible_status_t mible_dfu_callback_unregister(mible_dfu_callback_t cb)
{
    int index;

    if (NULL == cb) {
        return MI_ERR_INVALID_PARAM;
    }

//    CRITICAL_SECTION_ENTER();
    for (index = 0; index < MIBLE_DFU_CALLBACK_MAX; ++index) {
        if (cb == dfu_callback[index]) {
            dfu_callback[index] = NULL;
            break;
        }
    }
//    CRITICAL_SECTION_EXIT();

    return MI_SUCCESS;
}

static void mible_dfu_reset(void)
{
    dfu_config.total_size = 0;
    dfu_config.switch_delay = ~0;
    dfu_config.current_index = 0;
}

static mible_status_t mible_dfu_send_evt(uint8_t event, uint8_t * param, uint16_t len)
{
    uint8_t tmp[MIBLE_DFU_EVT_SIZE_MAX] = {0};
    mible_dfu_evt_t * p_evt;

    if (NULL == param || len > MIBLE_DFU_EVT_SIZE_MAX - offsetof(mible_dfu_evt_t, param)) {
        return MI_ERR_INVALID_PARAM;
    }

    p_evt = (mible_dfu_evt_t *)tmp;
    p_evt->event = event;
    p_evt->length = len;
    memcpy(p_evt->param, param, len);
    if (MI_SUCCESS != enqueue(&dfu_config.evt_queue, tmp)) {
        MI_LOG_ERROR("Fail to save event into queue.\n");
        return MI_ERR_NO_MEM;
    }

    PT_SEM_SIGNAL(0, &dfu_config.sem_evt);
    return MI_SUCCESS;
}

static void mible_dfu_timer_callback(void * arg)
{
    dfu_timer_tick++;
    if (0 == PT_SCHEDULE(mible_dfu_sar_handler(&dfu_config.pt_sar_ctrl))) {
        if (dfu_fragment.index > 0 && dfu_fragment.index >= dfu_config.current_index) {
            MI_LOG_DEBUG("current index = %3x\n", dfu_fragment.index);
            retry_num = MIBLE_DFU_STORE_RETRY_MAX;
            PT_SEM_SIGNAL(0, &dfu_config.sem_flash);
        }
    }

    mible_dfu_fragment_handler(&dfu_config.pt_flash_ctrl);
    mible_dfu_evt_handler(&dfu_config.pt_evt_ctrl);
    mible_dfu_delay_handler(&dfu_config.pt_delay_ctrl);
}

static int mible_dfu_fragment_handler(pt_t * pt)
{
    uint8_t buffer[MIBLE_DFU_FRAGMENT_SIZE] = {0};

    PT_BEGIN(pt);

    /* Wait until the fragment is received */
    PT_SEM_WAIT(pt, &dfu_config.sem_flash);

    /* Try to write the fragment into flash */
    PT_WAIT_UNTIL(pt, (MI_SUCCESS == mible_dfu_flash_write(dfu_fragment.data, MIBLE_DFU_FRAGMENT_SIZE,
        (dfu_fragment.index - 1) * MIBLE_DFU_FRAGMENT_SIZE)) || ((--retry_num) <= 0));

    /* Read the copy of fragment to RAM */
    PT_WAIT_UNTIL(pt, (MI_SUCCESS == mible_dfu_flash_read(buffer, MIBLE_DFU_FRAGMENT_SIZE,
        (dfu_fragment.index - 1) * MIBLE_DFU_FRAGMENT_SIZE)) || ((--retry_num) <= 0));

    /* Check the integrity of fragment */
    if (0 == memcmp(dfu_fragment.data, buffer, MIBLE_DFU_FRAGMENT_SIZE)) {
        uint8_t param[3];
        if (1 == dfu_fragment.index) {
            dfu_config.total_size = dfu_config.current_size - sizeof(dfu_fragment.index);
        } else if (dfu_fragment.index > dfu_config.current_index) {
            dfu_config.total_size += dfu_config.current_size - sizeof(dfu_fragment.index);
        }
        dfu_config.current_index = dfu_fragment.index;
        MI_LOG_DEBUG("total length = %5x\n", dfu_config.total_size);
        format_rx_cb(&dfu_config.rxfer_dfu, &dfu_fragment, sizeof(dfu_fragment));
        param[0] = MIBLE_DFU_STATUS_SUCC;
        param[1] = LO_UINT16(dfu_fragment.index);
        param[2] = HI_UINT16(dfu_fragment.index);
        mible_dfu_send_evt(MIBLE_DFU_EVENT_TRANS_COMPLETE, param, sizeof(param));
    } else if (--retry_num > 0) {
        MI_LOG_DEBUG("Fail to save fragment, try again\n");
        PT_SEM_SIGNAL(0, &dfu_config.sem_flash);
    } else {
        uint8_t param[3];
        param[0] = MIBLE_DFU_STATUS_ERR_BUSY;
        param[1] = LO_UINT16(dfu_fragment.index);
        param[2] = HI_UINT16(dfu_fragment.index);
        MI_LOG_DEBUG("Fail to save fragment, abort\n");
        mible_dfu_send_evt(MIBLE_DFU_EVENT_TRANS_COMPLETE, param, sizeof(param));
    }

    PT_END(pt);
}

static int mible_dfu_sar_handler(pt_t * pt)
{
    PT_BEGIN(pt);

    PT_SPAWN(pt, &dfu_config.pt_rx_ctrl, rxfer_rx_thd(&dfu_config.pt_rx_ctrl,
                                                      &dfu_config.rxfer_dfu, 
                                                      PASS_THROUGH,
                                                      &dfu_config.current_size));

    PT_END(pt);
}

static int mible_dfu_evt_handler(pt_t * pt)
{
    PT_BEGIN(pt);

    PT_SEM_WAIT(pt, &dfu_config.sem_evt);
    uint8_t tmp[MIBLE_DFU_EVT_SIZE_MAX];
    if (MI_SUCCESS == dequeue(&dfu_config.evt_queue, tmp)){
        mible_dfu_evt_t * p_evt = (mible_dfu_evt_t *)tmp;
        if (MI_SUCCESS == mible_gatts_notify_or_indicate(dfu_config.init.conn_handle,
                                         dfu_config.init.srv_handle,
                                         dfu_config.init.ctrl_handle,
                                         0, tmp,
                                         p_evt->length + offsetof(mible_dfu_evt_t, param),
                                         1)) {

            //MI_LOG_DEBUG("Succeed in sending event %d\n", p_evt->event);
        } else {
            MI_LOG_DEBUG("Fail to send %d, retry later\n", p_evt->event);
            if (MI_SUCCESS == enqueue(&dfu_config.evt_queue, tmp)) {
                PT_SEM_SIGNAL(pt, &dfu_config.sem_evt);
            } else {
                MI_LOG_WARNING("Can not restore event.\n");
            }
        }
    } else {
        MI_LOG_WARNING("Can not find event.\n");
    }

    PT_END(pt);
}

static int mible_dfu_delay_handler(pt_t * pt)
{
    PT_BEGIN(pt);

    PT_SEM_WAIT(pt, &dfu_config.sem_delay);
    if (dfu_config.switch_delay > 0 && ~0 != dfu_config.switch_delay) {
        dfu_config.switch_delay--;
        if (0 == dfu_config.switch_delay) {
            MI_LOG_DEBUG("start to upgrade\n");
            for (int index = 0; index < MIBLE_DFU_CALLBACK_MAX; ++index) {
                if (dfu_callback[index]) {
                    dfu_callback[index](MIBLE_DFU_STATE_SWITCH, NULL);
                }
            }
            mible_upgrade_firmware();
        } else {
            PT_SEM_SIGNAL(pt, &dfu_config.sem_delay);
        }
    }

    PT_END(pt);
}

