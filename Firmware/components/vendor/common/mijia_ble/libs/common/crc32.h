#ifndef MIJIA_BLE_LIBS_COMMON_CRC32_H_
#define MIJIA_BLE_LIBS_COMMON_CRC32_H_
#include <stdint.h>

uint32_t soft_crc32(uint8_t const * p_data, uint32_t size, uint32_t init_crc);

#endif /* MIJIA_BLE_LIBS_COMMON_CRC32_H_ */
