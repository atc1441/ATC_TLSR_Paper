#include "mible_api.h"
#include "mible_port.h"

#undef  MI_LOG_MODULE_NAME
#define MI_LOG_MODULE_NAME "uECC"
#include "mible_log.h"

#if (HAVE_MSC==0)
#include "cryptography/mi_crypto_backend_uECC.h"
#include "third_party/micro-ecc/uECC.h"

typedef uint8_t (ecc256_sk_t)[256 / 8];
typedef uint8_t (ecc256_pk_t)[256 / 8 * 2];
typedef uint8_t (ecc256_ss_t)[256 / 8];

static int swap_endian(const uint8_t *in, uint8_t *out, uint32_t size)
{
    if (out < in + size && in < out + size)
        return -1;

    for(int i = 0; i < size; i++)
        out[i] = in[size-1-i];

    return 0;
}

static int ecc_rng(uint8_t *dest, unsigned size)
{
    mible_rand_num_generator(dest, (uint32_t) size);
    return 1;
}

void micro_ecc_init(void *p_ctx)
{
     uECC_set_rng(ecc_rng);
}

int micro_ecc_keypair_gen(void *p_ctx, uint8_t *p_sk, uint8_t *p_pk)
{
    ecc256_pk_t le_pk;
    ecc256_sk_t le_sk;
    const struct uECC_Curve_t * p_curve;

    p_curve = uECC_secp256r1();

    int ret = uECC_make_key(le_pk, le_sk, p_curve);
    if (!ret) {
        return -3;
    }

    swap_endian(&le_sk[0], &p_sk[0], 32);
    swap_endian(&le_pk[0], &p_pk[0], 32);
    swap_endian(&le_pk[32], &p_pk[32], 32);

    return 0;
}

int micro_ecc_public_key_compute(void *p_ctx, uint8_t const *p_sk, uint8_t *p_pk)
{
    ecc256_pk_t le_pk;
    ecc256_sk_t le_sk;
    const struct uECC_Curve_t * p_curve;

    p_curve = uECC_secp256r1();
    swap_endian(&p_sk[0], &le_sk[0], 32);
    int ret = uECC_compute_public_key(le_sk, le_pk, p_curve);
    if (!ret)
    {
        return -3;
    }

    swap_endian(&le_pk[0], &p_pk[0], 32);
    swap_endian(&le_pk[32], &p_pk[32], 32);
    return 0;
}

int micro_ecc_shared_secret_compute(void *p_ctx, uint8_t const *p_sk, uint8_t const *p_pk, uint8_t *p_ss)
{
    ecc256_sk_t le_sk;
    ecc256_pk_t le_pk;
    ecc256_ss_t le_ss;
    const struct uECC_Curve_t * p_curve;

    p_curve = uECC_secp256r1();
    swap_endian(&p_sk[0], &le_sk[0], 32);
    swap_endian(&p_pk[0], &le_pk[0], 32);
    swap_endian(&p_pk[32], &le_pk[32], 32);
    if (!uECC_valid_public_key(le_pk, p_curve))
        return -4;

    int ret = uECC_shared_secret(le_pk, le_sk, le_ss, p_curve);
    if (!ret)
    {
        return -3;
    }

    swap_endian(&le_ss[0], &p_ss[0], 32);
    return 0;
}

int micro_ecc_sign(void *p_ctx, uint8_t const *p_sk, uint8_t const * p_hash, uint8_t *p_sig)
{
    const struct uECC_Curve_t * p_curve;

    p_curve = uECC_secp256r1();

    ecc256_sk_t le_sk;
    uint8_t le_hash[32];
    uint8_t le_sig[64];

    swap_endian(&p_sk[0], &le_sk[0], 32);
    swap_endian(p_hash, le_hash, 32);

    int ret = uECC_sign((const uint8_t *)le_sk, (const uint8_t *) le_hash, 32, (uint8_t *) le_sig, p_curve);
    if (!ret)
    {
        return -3;
    }

    swap_endian(&le_sig[0], &p_sig[0], 32);
    swap_endian(&le_sig[32], &p_sig[32], 32);
    return 0;
}

int micro_ecc_verify(void *p_ctx, uint8_t const *p_pk, uint8_t const * p_hash, uint8_t const *p_sig)
{
    const struct uECC_Curve_t * p_curve;
    ecc256_pk_t le_pk;
    uint8_t le_hash[32];
    uint8_t le_sig[64];

    p_curve = uECC_secp256r1();

    swap_endian(&p_pk[0], &le_pk[0], 32);
    swap_endian(&p_pk[32], &le_pk[32], 32);
    swap_endian(p_hash, le_hash, 32);
    swap_endian(&p_sig[0], &le_sig[0], 32);
    swap_endian(&p_sig[32], &le_sig[32], 32);
    int ret = uECC_verify((const uint8_t *) le_pk, (const uint8_t *) le_hash, 32, (uint8_t *) le_sig, p_curve);
    if (!ret)
    {
        return -3;
    }

    return 0;
}

#endif // HAVE_MSC==0
