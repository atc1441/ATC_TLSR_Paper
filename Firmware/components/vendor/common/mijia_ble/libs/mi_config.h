#ifndef __MI_CONFIG_H__
#define __MI_CONFIG_H__
#include <stdint.h>

#if defined(CUSTOMIZED_MI_CONFIG_FILE)
#include CUSTOMIZED_MI_CONFIG_FILE
#endif

/**
 * @note MIBLE_LIB_AND_DEVELOPER_FIRMWARE_VERSION
 *
 * The MIBLE SDK major, minor and revision MUST NOT be changed.
 * And DEVELOPER_VERSION will identify developer firmware version.
 * e.g. x.y.z_d
 */
#ifndef DEVELOPER_VERSION
#define DEVELOPER_VERSION       0001
#endif

#define STR_VAL(str)  #str
#define CONCAT_VERSION(x,y,z,u) STR_VAL(x) "." STR_VAL(y) "." STR_VAL(z) "_" STR_VAL(u)
#define MI_BLE_ENABLED	1
#define HAVE_MSC 	0
#if defined(MI_BLE_ENABLED) && (!HAVE_MSC)
#define MIBLE_AUTH_MODE            2
#define MIBLE_LIB_MAJOR            1
#define MIBLE_LIB_MINOR            0
#define MIBLE_LIB_REVISION         1
#elif defined(MI_BLE_ENABLED) && (HAVE_MSC)
#define MIBLE_AUTH_MODE            1
#define MIBLE_LIB_MAJOR            2
#define MIBLE_LIB_MINOR            2
#define MIBLE_LIB_REVISION         6
#elif defined(MI_MESH_ENABLED)
#define MIBLE_AUTH_MODE            1
#define MIBLE_LIB_MAJOR            1
#define MIBLE_LIB_MINOR            3
#define MIBLE_LIB_REVISION         0
#else
#error "No MI_BLE_ENABLED or MI_MESH_ENABLED is defined. Should add one of them in the preprocesser symbols."
#endif

#define MIBLE_LIB_AND_DEVELOPER_VERSION CONCAT_VERSION(MIBLE_LIB_MAJOR, MIBLE_LIB_MINOR, MIBLE_LIB_REVISION, DEVELOPER_VERSION)


/**
 * @note Product identification got from xiaomi IoT developer platform.
 */
#define MI_DEVELOP_MODE		0
#define MI_HUMI_TEMP_MODE 	1
#define MI_USER_MODE	MI_HUMI_TEMP_MODE

#ifndef PRODUCT_ID
#if defined(MI_BLE_ENABLED) && (HAVE_MSC)
#define PRODUCT_ID              463
#elif defined(MI_BLE_ENABLED)
	#if (MI_USER_MODE == MI_DEVELOP_MODE)
#define PRODUCT_ID              156
	#else
#define PRODUCT_ID				426
	#endif
#else
#define PRODUCT_ID              889
#endif
#endif



/**
 * @note mibeacon object advertising configuration
 *
 * The mibeacon object is an adv message contains the status or event. BLE gateway
 * can receive the beacon message (by active scanning) and upload it to server for
 * triggering customized home automation scene.
 *
 * OBJ_QUEUE_SIZE      : max num of objects can be concurrency advertising
 *                       ( actually, it will be sent one by one )
 * OBJ_ADV_INTERVAL_MS : the object adv interval
 * OBJ_ADV_TIMEOUT_MS  : the time one object will be continuously sent.
 */

#ifndef OBJ_QUEUE_SIZE
#define OBJ_QUEUE_SIZE         8
#endif

#ifndef OBJ_ADV_INTERVAL_MS
#define OBJ_ADV_INTERVAL_MS    100
#endif

#ifndef OBJ_ADV_TIMEOUT_MS
#define OBJ_ADV_TIMEOUT_MS     3000
#endif


/**
 * @note To support Secure Auth procedure MUST have mijia secure chip (MSC).
 *      NONE   : 0
 *      MJSC   : 1
 *      MJA1   : 2
 */
#ifndef HAVE_MSC
#define HAVE_MSC               0
#endif


/**
 * @note Device side has RESET button.
 */
#ifndef HAVE_RESET_BUTTON
#define HAVE_RESET_BUTTON      1
#endif

/**
 * @note Which OOB methods device used in authentication procedure.
 *       OOB_USE_NUMPAD_INPUT  (bit 0): device has a numeric keypad to enter the pairing code.
 *       OOB_USE_QR_CODE_OUT   (bit 4): device provided with a QR code label.
 *       OOB_USE_DISPLAT_OUT   (bit 7): device displayed a six digit number.
 */
#ifndef OOB_USE_NUMPAD_INPUT
#define OOB_USE_NUMPAD_INPUT   0
#endif

#ifndef OOB_USE_QR_CODE_OUT
#define OOB_USE_QR_CODE_OUT    0
#endif

#ifndef OOB_USE_DISPLAT_OUT
#define OOB_USE_DISPLAT_OUT    0
#endif

#define OOB_IO_CAP             \
(OOB_USE_NUMPAD_INPUT | (OOB_USE_DISPLAT_OUT)<<4 | (OOB_USE_QR_CODE_OUT)<<7)


/**
 * @note Process mi scheduler in main loop (non-interrupt context).
 */
#ifndef MI_SCHD_PROCESS_IN_MAIN_LOOP
#define MI_SCHD_PROCESS_IN_MAIN_LOOP    1
#endif

/* DEBUG */
#ifndef DEBUG_MIBLE
#define DEBUG_MIBLE            0
#define TIME_PROFILE           0
#endif

#ifndef DFU_NVM_START
#define DFU_NVM_START          0x4A000UL      /**< DFU Buffer Start Address */
#endif

#ifndef DFU_NVM_SIZE
#define DFU_NVM_SIZE           0x34800UL      /**< DFU Buffer Size in bytes */
#endif

typedef struct {
    uint16_t pid;
    uint16_t io;
    uint8_t  have_msc;
    uint8_t  have_reset_button:1;
    uint8_t  schd_in_mainloop :1;
    uint8_t  reserve          :6;
} mi_config_t;

extern const mi_config_t m_config;

#endif  /* __MI_CONFIG_H__ */ 


