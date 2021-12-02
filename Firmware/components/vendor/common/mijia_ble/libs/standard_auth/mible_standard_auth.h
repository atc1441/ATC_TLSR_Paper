#ifndef __MIBLE_STD_AUTH_H__
#define __MIBLE_STD_AUTH_H__
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCHD_IDLE                      0

#define REG_TYPE                       0x10UL
#define REG_START                      (REG_TYPE)
#define REG_SUCCESS                    (REG_TYPE+1)
#define REG_FAILED                     (REG_TYPE+2)
#define REG_VERIFY_SUCC                (REG_TYPE+3)
#define REG_VERIFY_FAIL                (REG_TYPE+4)
#define REG_START_WO_PKI               (REG_TYPE+5)

#define LOG_TYPE                       0x20UL
#define LOG_START                      (LOG_TYPE)
#define LOG_SUCCESS                    (LOG_TYPE+1)
#define LOG_INVALID_LTMK               (LOG_TYPE+2)
#define LOG_FAILED                     (LOG_TYPE+3)
#define LOG_START_W_RANDOM             (LOG_TYPE+4)

#define SYS_TYPE                       0xA0UL
#define SYS_KEY_RESTORE                (SYS_TYPE)
#define SYS_KEY_DELETE                 (SYS_TYPE+1)
#define SYS_DEV_INFO_GET               (SYS_TYPE+2)

#define ERR_TYPE                       0xE0UL
#define ERR_NOT_REGISTERED             (ERR_TYPE)
#define ERR_REGISTERED                 (ERR_TYPE+1)
#define ERR_REPEAT_LOGIN               (ERR_TYPE+2)
#define ERR_INVALID_OOB                (ERR_TYPE+3)

typedef enum {
    UNAUTHORIZATION = 0,
    ADMIN_AUTHORIZATION,
    SHARE_AUTHORIZATION
} mi_author_stat_t;

typedef enum {
    SCHD_EVT_REG_SUCCESS                    = 0x01,
    SCHD_EVT_REG_FAILED                     = 0x02,
    SCHD_EVT_ADMIN_LOGIN_SUCCESS            = 0x03,
    SCHD_EVT_ADMIN_LOGIN_FAILED             = 0x04,
    SCHD_EVT_SHARE_LOGIN_SUCCESS            = 0x05,
    SCHD_EVT_SHARE_LOGIN_FAILED             = 0x06,
    SCHD_EVT_TIMEOUT                        = 0x07,
    SCHD_EVT_KEY_NOT_FOUND                  = 0x08,
    SCHD_EVT_KEY_FOUND                      = 0x09,
    SCHD_EVT_KEY_DEL_FAIL                   = 0x0A,
    SCHD_EVT_KEY_DEL_SUCC                   = 0x0B,
    SCHD_EVT_MESH_REG_SUCCESS               = 0x0C,
    SCHD_EVT_MESH_REG_FAILED                = 0x0D,
    SCHD_EVT_OOB_REQUEST                    = 0x0E,
    SCHD_EVT_MSC_SELF_TEST_PASS             = 0x0F,
    SCHD_EVT_MSC_SELF_TEST_FAIL             = 0x10,
    SCHD_EVT_INTERNAL                       = 0x11
} schd_evt_id_t;

typedef struct {
    schd_evt_id_t id;
    union {
        uint16_t IO_capability;
    } data;
} schd_evt_t;

typedef void (*mi_schd_event_handler_t)(schd_evt_t * p_event);

typedef int (*mi_msc_power_manage_t)(bool power_stat);

typedef struct {
    mi_msc_power_manage_t msc_onoff;
    void *p_msc_iic_config;
} mible_libs_config_t;

extern uint32_t schd_ticks;

int get_mi_device_id(uint8_t did[20]);
uint8_t get_mi_reg_stat(void);
uint8_t get_mi_authorization(void);
void set_mi_unauthorization(void);

uint32_t mi_scheduler_init(uint32_t interval, mi_schd_event_handler_t handler,
    mible_libs_config_t const * p_config);

uint32_t mi_scheduler_start(uint32_t procedure);

uint32_t mi_scheduler_stop(void);

int mi_schd_oob_rsp(uint8_t const * const p_oob, uint8_t oob_len);

void mi_schd_process(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif  /* __MIBLE_STD_AUTH_H__ */
