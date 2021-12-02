#ifndef __MI_CRYPTO_H__
#define __MI_CRYPTO_H__
#include <stdint.h>

#include "cryptography/mi_crypto_backend_msc.h"
#include "cryptography/mi_crypto_backend_mbedtls.h"

typedef enum {
    P256R1,
    ED25519
} ecc_curve_t;

typedef enum {
    SHA256,
    SHA512
} hash_alg_t;

typedef enum {
    MI_DEV_CERT    = 0x01,
    MI_MANU_CERT   = 0x02,
    MI_ROOT_CERT   = 0x03
} mi_cert_t;

typedef uint8_t (ecc256_sk_t)[256 / 8];
typedef uint8_t (ecc256_pk_t)[256 / 8 * 2];
typedef uint8_t (ecc256_ss_t)[256 / 8];

typedef uint8_t (aes128_t)[128 / 8];

typedef struct {
    union {
        ecc_curve_t curve;
        hash_alg_t  hash;
    };
    void * p_ctx;
} mi_crypto_ctx_t;

/**@brief Initialize the mi crypto module.
 * */
int mi_crypto_init(void);

/**@brief Create a public/private key pair.
 *
 * @param[out]  pk   Public key. Pointer must be aligned to a 4-byte boundary.
 *
 * @retval      3:  Key pair successfuly created.
 * @retval     -1:  NULL pointer provided.
 * @retval     -2:  Unaligned pointer provided.
 */
int mi_crypto_ecc_keypair_gen(ecc_curve_t curve, ecc256_pk_t pk);

/**@brief Create a public key from a provided private key.
 *
 * @param[in]   p_sk   Private key. Pointer must be aligned to a 4-byte boundary.
 * @param[out]  p_pk   Public key. Pointer must be aligned to a 4-byte boundary.
 *
 * @retval      3:  Public key successfuly created.
 * @retval     -1:  NULL pointer provided.
 * @retval     -2:  Unaligned pointer provided.
 */
int mi_crypto_ecc_pubkey_compute(ecc_curve_t curve, const ecc256_sk_t sk, ecc256_pk_t pk);

/**@brief Create a shared secret from a provided public/private key pair.
 *
 * @param[in]   p_pk   Public key. Pointer must be aligned to a 4-byte boundary.
 * @param[out]  p_ss   Shared secret. Pointer must be aligned to a 4-byte boundary.
 *
 * @retval      3:  Shared secret successfuly created.
 * @retval     -1:  NULL pointer provided.
 * @retval     -2:  Unaligned pointer provided.
 */
int mi_crypto_ecc_shared_secret_compute(ecc_curve_t curve, const ecc256_pk_t pk, ecc256_ss_t ss);

/**@brief Sign a hash or digest using a private key.
 *
 * @param[in]   p_hash Hash. Pointer must be aligned to a 4-byte boundary.
 * @param[out]  p_sig  Signature. Pointer must be aligned to a 4-byte boundary.
 *
 * @retval      3:  Signature successfuly created.
 * @retval     -1:  NULL pointer provided.
 * @retval     -2:   Unaligned pointer provided.
 */
int mi_crypto_ecc_sign(ecc_curve_t curve, const uint8_t *p_hash, uint8_t *p_sig);

/**@brief Verify a signature using a public key.
 *
 * @param[in]   p_pk   Public key. Pointer must be aligned to a 4-byte boundary.
 * @param[in]   p_hash Hash. Pointer must be aligned to a 4-byte boundary.
 * @param[in]   p_sig  Signature. Pointer must be aligned to a 4-byte boundary.
 *
 * @retval      3:  Signature verified.
 * @retval      2:  Signature failed verification.
 * @retval     -1:  NULL pointer provided.
 * @retval     -2:  Unaligned pointer provided.
 */
int mi_crypto_ecc_verify(ecc_curve_t curve, const ecc256_pk_t pk, const uint8_t *p_hash, const uint8_t *p_sig);


int mi_crypto_get_crt_len(uint16_t *p_dev_len, uint16_t *p_manu_len, uint16_t *p_root_len);

int mi_crypto_get_crt_der(mi_cert_t type, uint8_t *p_der, uint16_t der_len);

int mi_crypto_crt_parse_der(const unsigned char *crt, size_t crt_sz,
        const msc_crt_time *now, msc_crt_t *msc_crt);


int mi_crypto_hash_init(void * p_ctx);

int mi_crypto_hash_update(void *p_ctx, uint8_t *p_data, uint16_t data_len);

int mi_crypto_hash_finish(void *p_ctx, uint8_t *p_digest, uint8_t *p_digest_len);

int mi_crypto_sha256(uint8_t *p_in, uint32_t in_len, uint8_t *p_out);

int mi_crypto_hkdf_sha256(const uint8_t *key, uint8_t key_len, const uint8_t *salt, uint8_t salt_len,
                        const uint8_t *info, uint16_t info_len, uint8_t *out, uint8_t out_len);

int mi_crypto_hmac_init(void *p_ctx, uint8_t *p_key, uint8_t key_len);

int mi_crypto_hmac_update(void *p_ctx, uint8_t *p_data, uint16_t data_len);

int mi_crypto_hmac_finish(void *p_ctx, uint8_t *p_digest, uint8_t *p_digest_len);

int mi_crypto_hmac_sha256(const uint8_t *k, uint32_t key_len, const uint8_t *p_in, uint32_t in_len, uint8_t *p_out);

int mi_crypto_ccm_encrypt(const uint8_t *key,
                         const uint8_t *iv, size_t iv_len,
                         const uint8_t *add, size_t add_len,
                         const uint8_t *in, size_t in_len,
                         uint8_t *out,
                         uint8_t *tag, size_t tag_len );

int mi_crypto_ccm_decrypt( const uint8_t *key,
                        const uint8_t *iv, size_t iv_len,
                        const uint8_t *add, size_t add_len,
                        const uint8_t *in, size_t in_len,
                        uint8_t *out,
                        const uint8_t *tag, size_t tag_len );

int mi_crypto_record_write(uint8_t record_id, const uint8_t *p_data, uint8_t len);

int mi_crypto_record_read(uint8_t record_id, uint8_t *p_data, uint8_t len);

int mi_crypto_record_delete(uint8_t record_id);
void set_mi_mac_address(unsigned char *p_mac);

#endif  /* __MI_CRYPTO_H__ */ 
