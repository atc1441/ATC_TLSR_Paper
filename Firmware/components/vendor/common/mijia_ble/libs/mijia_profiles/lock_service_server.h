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

/**@file
 *
 * @defgroup ble_mi Lock Service
 * @{
 * @ingroup  ble_sdk_srv
 * @brief    Lock Service implementation.
 *
 * @details The Lock Service is a simple GATT-based service with many characteristics.
 *          Data received from the peer is passed to the application, and the data received
 *          from the application of this service is sent to the peer as Handle Value
 *          Notifications. This module demonstrates how to implement a custom GATT-based
 *          service and characteristics using the BLE Stack. The service
 *          is used by the application to send and receive pub_key and MSC Cert to and from the
 *          peer.
 *
 * @note The application must propagate BLE Stack events to the Lock Service module
 *       by calling the ble_mi_on_ble_evt() function from the ble_stack_handler callback.
 */

#ifndef __LOCK_SERVICE_SECURE_H__
#define __LOCK_SERVICE_SECURE_H__
#include <stdint.h>
#include "mible_port.h"

#define BLE_MI_MAX_DATA_LEN (GATT_MTU_SIZE_DEFAULT - 3) /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Xiaomi  service module. */

#ifdef __cplusplus
extern "C" {
#endif

#if defined ( __CC_ARM )
__packed struct lock_event_s {
    uint8_t  action : 4;
    uint8_t  method : 4;
    uint32_t user_id;
    uint32_t time;
};
#elif defined ( __GNUC__ )
struct lock_event_s {
    uint8_t  action : 4;
    uint8_t  method : 4;
    uint32_t user_id;
    uint32_t time;
} __PACKED;
#endif
typedef struct lock_event_s lock_event_t;

/**@brief Lock Service event handler type. */
typedef void (*lock_handler_t) (uint8_t opcode);

/**@brief Lock Service initialization structure.
 *
 * @details This structure contains the initialization information for the service. The application
 * must fill this structure and pass it to the service using the @ref ble_mi_init
 *          function.
 */
typedef struct
{
    lock_handler_t opcode_handler; /**< Event handler to be called for handling received data. */
} lock_init_t;


/**@brief Function for initializing the Lock Service.
 *
 * @param[in] config  Information needed to initialize the service.
 *
 * @retval NRF_SUCCESS If the service was successfully initialized. Otherwise, an error code is returned.
 * @retval NRF_ERROR_NULL If either of the pointers p_mi_s or p_mi_s_init is NULL.
 */
int lock_service_init(lock_init_t *p_config);

/**@brief Respond the present lock stat.
 *
 * @param[in] status  0: unlocked
 *                    1: locked
 *                    2: bolted
 *                    
 * @details This function MUST be called in 5 seconds after the lock opcode received.
 * Otherwise, Mijia App will disconnect it.
 *
 * @retval  0 : successful respond the status.
 *         -1 : BLE_CONN_HANDLE_INVALID
 *         -2 : PEER NOT LOGIN.
 *         -3 : BLE STACK BUSY.
 */
int reply_lock_stat(uint8_t status);


/**@brief Respond the lock logs..
 *
 * @param[in] type : LOCK EVENTS (defined in the mibeacon)
 * @param[in] vlen : the length of EVENT value.
 * @param[in] value: Pointer to the formated LOCK EVENT value.
 *
 * @retval  0 : successful respond the status.
 *         -1 : BLE_CONN_HANDLE_INVALID
 *         -2 : PEER NOT LOGIN.
 *         -3 : BLE STACK BUSY.
 */
int send_lock_log(uint16_t type, uint8_t vlen, void *value);

#ifdef __cplusplus
}
#endif

#endif // __LOCK_SERVICE_SECURE_H__

/** @} */
