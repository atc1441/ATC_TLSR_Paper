#include "tlv.h"
#include "mible_log.h"

int tlv8_decode(uint8_t * tlv_buf, int buf_size, uint8_t type, tlv8_t **out)
{
    if (tlv_buf == 0 || out == 0)
        return -1;

    while(tlv_buf[0] != type && buf_size > 2) {
        uint8_t len = tlv_buf[1] + 2;
        tlv_buf += len;
        buf_size -= len;
    }

    if (buf_size <= 2) {
        return -2;
    } else if (buf_size < tlv_buf[1] + 2) {
        return -3;
    } else {
        *out = (tlv8_t*)tlv_buf;
        return 0;
    }
}
