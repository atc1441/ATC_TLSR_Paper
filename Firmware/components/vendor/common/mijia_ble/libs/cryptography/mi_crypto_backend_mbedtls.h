#ifndef __MIBLE_CRYPTO_BACKEND_MBEDTLS_H__
#define __MIBLE_CRYPTO_BACKEND_MBEDTLS_H__
#include <stdint.h>
#include <stddef.h>
#include "mible_type.h"

#define PEM_HEADER                   "-----BEGIN CERTIFICATE-----"
#define PEM_FOOTER                   "-----END CERTIFICATE-----"

/** Container for date and time (precision in seconds). */
typedef struct
{
    int year, mon, day;         /**< Date. */
    int hour, min, sec;         /**< Time. */
} msc_crt_time;

typedef struct {
    unsigned char *p;
    size_t len;
} msc_crt_item_t;

#define MSC_CRT_SIG_SZ 64
typedef struct {
    msc_crt_item_t tbs;                /* to be signed */
    msc_crt_item_t pub;                /* public key */
    msc_crt_item_t sn;                 /* serial number */
    msc_crt_item_t sub_o;              /* subject O */
    unsigned char sig[MSC_CRT_SIG_SZ]; /* signature */
} msc_crt_t;

/**
 * @brief x509 DER crt parse
 *
 * @param raw_crt - input crt in DER format
 * @param raw_crt_sz
 * @param now - current time in GMT
 * @param msc_crt - output parsed crt
 *
 * @return 0-OK
 */
int mbedtls_crt_parse_der(const unsigned char *raw_crt, size_t raw_crt_sz,
        const msc_crt_time *now,
        msc_crt_t *msc_crt
        );
int mbedtls_crt_pem2der(const unsigned char *pem, size_t pem_sz,
                                  unsigned char *der_buf, size_t buf_sz);
int mbedtls_read_signature(const unsigned char *sign_der, uint16_t sign_sz,
                                      uint8_t *out_buf, uint16_t out_sz);

#endif /* __MIBLE_CRYPTO_BACKEND_MSC_H__ END*/

