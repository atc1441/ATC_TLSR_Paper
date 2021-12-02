#ifndef MIBLE_BEACON_INTERNAL_H__
#define MIBLE_BEACON_INTERNAL_H__

#include "mible_type.h"
#include "mible_port.h"

typedef struct {
    uint8_t         reserved0           :1;
    uint8_t         reserved1           :1;
    uint8_t         reserved2           :1;
    uint8_t         is_encrypt          :1;
    uint8_t         mac_include         :1;
    uint8_t         cap_include         :1;
    uint8_t         obj_include         :1;
    uint8_t         mesh_include        :1;

    uint8_t         registered          :1;
    uint8_t         solicite            :1;
    uint8_t         auth_mode           :2;
    uint8_t         version             :4;
} mibeacon_frame_ctrl_t;

typedef struct {
    uint16_t        type;
    uint8_t         len;
    uint8_t         need_encrypt;
    uint8_t         val[12];
 } mibeacon_obj_t;

typedef struct {
    uint8_t         connectable  :1;
    uint8_t         centralable  :1;
    uint8_t         encryptable  :1;
    uint8_t         bondAbility  :2;
    uint8_t         IO_capability:1;
    uint8_t         reserved     :2;
} mibeacon_capability_t;

typedef struct {
    uint8_t in_digits            :1;
    uint8_t RFU0                 :1;
    uint8_t RFU1                 :1;
    uint8_t RFU2                 :1;
    uint8_t out_digits           :1;
    uint8_t RFU3                 :1;
    uint8_t RFU4                 :1;
    uint8_t out_image            :1;

    uint8_t reserved             :8;
} mibeacon_cap_sub_io_t;

typedef struct {
    uint8_t         pb_adv       :1;
    uint8_t         pb_gatt      :1;
    uint8_t         state        :2;
    uint8_t         version      :4;

    uint8_t         reserved     :8;
} mibeacon_mesh_t;

typedef struct {
    mibeacon_frame_ctrl_t   frame_ctrl;
    uint16_t                pid;
    mible_addr_t           *p_mac;
    mibeacon_capability_t  *p_capability;
    mibeacon_cap_sub_io_t  *p_cap_sub_IO;
    uint8_t 			   *p_wifi_mac;
    mibeacon_obj_t         *p_obj;
    uint8_t                 obj_num;
    mibeacon_mesh_t        *p_mesh;
} mibeacon_config_t;

mible_status_t mibeacon_init(uint8_t key[16], uint8_t newone);

#endif //MIBLE_BEACON_INTERNAL_H__
