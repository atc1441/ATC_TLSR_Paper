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

#ifndef __STDIO_SERVICE_H__
#define __STDIO_SERVICE_H__
#include <stdint.h>
#include "mible_port.h"

#define BLE_MI_MAX_DATA_LEN (GATT_MTU_SIZE_DEFAULT - 3) /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Xiaomi  service module. */

#ifdef __cplusplus
extern "C" {
#endif

/**@brief stdio Service event handler type. */
typedef void (*stdio_rx_t) (uint8_t* p, uint8_t l);


/**@brief Function for initializing the stdio Service.
 *
 * @param[in] rx_handler receive data callback
 *
 * @retval NRF_SUCCESS If the service was successfully initialized. Otherwise, an error code is returned.
 * @retval NRF_ERROR_NULL If either of the pointers p_mi_s or p_mi_s_init is NULL.
 */
int stdio_service_init(stdio_rx_t rx_handler);


/**@brief Send data.
 *
 * @param[in] vlen : the length of EVENT value.
 * @param[in] value: Pointer to the formated LOCK EVENT value.
 *
 * @retval  0 : successful respond the status.
 *         -1 : BLE_CONN_HANDLE_INVALID
 *         -2 : PEER NOT LOGIN.
 *         -3 : BLE STACK BUSY.
 */
int stdio_tx(void *data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif // __STDIO_SERVICE_H__

/** @} */
