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
 * @defgroup ble_mi Xiaomi Service
 * @{
 * @ingroup  ble_sdk_srv
 * @brief    Xiaomi Service implementation.
 *
 * @details The Xiaomi Service is a simple GATT-based service with many characteristics.
 *          Data received from the peer is passed to the application, and the data
 *          from the application of this service is sent to the peer as Handle Value
 *          Notifications. This module demonstrates how to implement a custom GATT-based
 *          service and characteristics using the BLE Stack. The service
 *          is used by the application to send and receive pub_key and MSC Cert from the
 *          peer.
 *
 * @note The application must propagate BLE Stack events to the Xiaomi Service module
 *       by calling the ble_mi_on_ble_evt() function from the ble_stack_handler callback.
 */

#ifndef __MI_SERVICE_SECURE_H__
#define __MI_SERVICE_SECURE_H__
#include <stdint.h>

#define BLE_MI_MAX_DATA_LEN (GATT_MTU_SIZE_DEFAULT - 3) /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Xiaomi  service module. */

typedef void (* mi_service_evt_handler_t)(uint8_t is_connected);

/**@brief Function for initializing the Xiaomi Service.
 */
uint32_t mi_service_init(void);

/* internal api */
void mi_service_event_register(mi_service_evt_handler_t h);
uint32_t opcode_send(uint32_t status);
uint32_t opcode_recv(void);

#endif // __MI_SERVICE_SECURE_H__
