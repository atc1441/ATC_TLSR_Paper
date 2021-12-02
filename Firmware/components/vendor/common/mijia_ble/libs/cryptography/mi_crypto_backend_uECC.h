#ifndef __MIBLE_CRYPTO_BACKEND_UECC_H__
#define __MIBLE_CRYPTO_BACKEND_UECC_H__
#include <stdint.h>
#include "mible_type.h"

void micro_ecc_init(void *p_ctx);
int micro_ecc_keypair_gen(void *p_ctx, uint8_t *p_sk, uint8_t *p_pk);
int micro_ecc_public_key_compute(void *p_ctx, uint8_t const *p_sk, uint8_t *p_pk);
int micro_ecc_shared_secret_compute(void *p_ctx, uint8_t const *p_sk, uint8_t const *p_pk, uint8_t *p_ss);
int micro_ecc_sign(void *p_ctx, uint8_t const *p_sk, uint8_t const * p_hash, uint8_t *p_sig);
int micro_ecc_verify(void *p_ctx, uint8_t const *p_pk, uint8_t const * p_hash, uint8_t const *p_sig);

#endif

