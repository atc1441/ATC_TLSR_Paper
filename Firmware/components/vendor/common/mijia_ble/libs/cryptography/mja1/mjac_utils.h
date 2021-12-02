#ifndef __MJAC_UTILS_H
#define __MJAC_UTILS_H
#include "mjac_defs.h"

struct mjac_lv {
    UINT16 length;
    UINT8 *value;
};
typedef struct {
    struct mjac_lv tbs;
    struct mjac_lv pub;
    struct mjac_lv sn;
    struct mjac_lv sign;
} x509_crt_t;

extern UINT16 crc16_ccitt(const UINT8 *data, UINT16 sz);

extern UINT8 x509_crt_parse_der(const UINT8 *der, UINT16 der_sz, x509_crt_t *crt);

extern UINT16 x509_pubkey_to_mjac(
        const UINT8 *pub, UINT16 pub_sz,
        UINT8 *out, UINT16 out_sz);
extern UINT16 x509_sign_to_mjac(
        const UINT8 *sign, UINT16 sign_sz,
        UINT8 *out, UINT16 out_sz);

#endif
