#ifndef MIJIA_BLE_LIBS_COMMON_TLV_H_
#define MIJIA_BLE_LIBS_COMMON_TLV_H_

#include <stdint.h>

typedef struct {
    uint8_t type;
    uint8_t len;
    uint8_t value[1];
} tlv8_t;

int tlv8_decode(uint8_t * tlv_buf, int buf_size, uint8_t type, tlv8_t **out);

#endif /* MIJIA_BLE_LIBS_COMMON_TLV_H_ */
