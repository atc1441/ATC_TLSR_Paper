/*
 *  FIPS-180-2 compliant SHA-256 implementation
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */
/*
 *  The SHA-256 Secure Hash Standard was published by NIST in 2002.
 *
 *  http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf
 */

//#include"stddef.h"
#include"stdint.h"
#include "third_party/mbedtls/sha256_hkdf.h"
#include "telink_sdk_mible_api.h"
#include "mible_port.h"
#define MI_LOG_MODULE_NAME "SHA"
#include "mible_log.h"

#define MBEDTLS_SELF_TEST   0
#define mbedtls_printf MI_LOG_INFO

#ifndef MIN
#define MIN(a,b)          ((a) < (b) ? (a) : (b))
#endif

#ifndef CEIL_DIV
#define CEIL_DIV(a, b)    (((a) + (b) - 1) / (b))
#endif

/* Implementation that should never be optimized out by the compiler */
static void mbedtls_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            \
do {                                                    \
    (n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
        | ( (uint32_t) (b)[(i) + 1] << 16 )             \
        | ( (uint32_t) (b)[(i) + 2] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 3]       );            \
} while( 0 )
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                            \
do {                                                    \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
} while( 0 )
#endif

__WEAK void mbedtls_sha256_init( mbedtls_sha256_context *ctx )
{
    memset( ctx, 0, sizeof( mbedtls_sha256_context ) );
}

__WEAK void mbedtls_sha256_free( mbedtls_sha256_context *ctx )
{
    if( ctx == NULL )
        return;

    mbedtls_zeroize( ctx, sizeof( mbedtls_sha256_context ) );
}

__WEAK void mbedtls_sha256_clone( mbedtls_sha256_context *dst,
                           const mbedtls_sha256_context *src )
{
    *dst = *src;
}

/*
 * SHA-256 context setup
 */
__WEAK void mbedtls_sha256_starts( mbedtls_sha256_context *ctx, int is224 )
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    if( is224 == 0 )
    {
        /* SHA-256 */
        ctx->state[0] = 0x6A09E667;
        ctx->state[1] = 0xBB67AE85;
        ctx->state[2] = 0x3C6EF372;
        ctx->state[3] = 0xA54FF53A;
        ctx->state[4] = 0x510E527F;
        ctx->state[5] = 0x9B05688C;
        ctx->state[6] = 0x1F83D9AB;
        ctx->state[7] = 0x5BE0CD19;
    }
    else
    {
        /* SHA-224 */
        ctx->state[0] = 0xC1059ED8;
        ctx->state[1] = 0x367CD507;
        ctx->state[2] = 0x3070DD17;
        ctx->state[3] = 0xF70E5939;
        ctx->state[4] = 0xFFC00B31;
        ctx->state[5] = 0x68581511;
        ctx->state[6] = 0x64F98FA7;
        ctx->state[7] = 0xBEFA4FA4;
    }

    ctx->is224 = is224;
}

static const uint32_t K[] =
{
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
};

#define SHR(x,n) ((x & 0xFFFFFFFF) >> n)
//#define ROTR(x,n) (SHR(x,n) | (x << (32 - n)))
int ROTR(int x,int n){
	volatile int tmp =0;
	tmp = SHR(x,n);
	tmp |=(x << (32 - n));
	return tmp;
}

#define S0(x) (ROTR(x, 7) ^ ROTR(x,18) ^  SHR(x, 3))
#define S1(x) (ROTR(x,17) ^ ROTR(x,19) ^  SHR(x,10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x,13) ^ ROTR(x,22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x,11) ^ ROTR(x,25))

#define F0(x,y,z) ((x & y) | (z & (x | y)))
#define F1(x,y,z) (z ^ (x & (y ^ z)))

#define R(t)                                    \
(                                               \
    W[t] = S1(W[t -  2]) + W[t -  7] +          \
           S0(W[t - 15]) + W[t - 16]            \
)

#define P(a,b,c,d,e,f,g,h,x,K)                  \
{                                               \
    temp1 = h + S3(e) + F1(e,f,g) + K + x;      \
    temp2 = S2(a) + F0(a,b,c);                  \
    d += temp1; h = temp1 + temp2;              \
}

__WEAK void mbedtls_sha256_process( mbedtls_sha256_context *ctx, const unsigned char data[64] )
{
    uint32_t temp1, temp2, W[64];
    uint32_t A[8];
    unsigned int i;

    for( i = 0; i < 8; i++ )
        A[i] = ctx->state[i];

#if defined(MBEDTLS_SHA256_SMALLER)
    for( i = 0; i < 64; i++ )
    {
        if( i < 16 )
            GET_UINT32_BE( W[i], data, 4 * i );
        else
            R( i );

        P( A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[i], K[i] );

        temp1 = A[7]; A[7] = A[6]; A[6] = A[5]; A[5] = A[4]; A[4] = A[3];
        A[3] = A[2]; A[2] = A[1]; A[1] = A[0]; A[0] = temp1;
    }
#else /* MBEDTLS_SHA256_SMALLER */
    for( i = 0; i < 16; i++ )
        GET_UINT32_BE( W[i], data, 4 * i );

    for( i = 0; i < 16; i += 8 )
    {
        P( A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[i+0], K[i+0] );
        P( A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[i+1], K[i+1] );
        P( A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], W[i+2], K[i+2] );
        P( A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[i+3], K[i+3] );
        P( A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], W[i+4], K[i+4] );
        P( A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], W[i+5], K[i+5] );
        P( A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], W[i+6], K[i+6] );
        P( A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], W[i+7], K[i+7] );
    }

    for( i = 16; i < 64; i += 8 )
    {
        P( A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(i+0), K[i+0] );
        P( A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(i+1), K[i+1] );
        P( A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(i+2), K[i+2] );
        P( A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(i+3), K[i+3] );
        P( A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(i+4), K[i+4] );
        P( A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(i+5), K[i+5] );
        P( A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(i+6), K[i+6] );
        P( A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(i+7), K[i+7] );
    }
#endif /* MBEDTLS_SHA256_SMALLER */

    for( i = 0; i < 8; i++ )
        ctx->state[i] += A[i];
}

/*
 * SHA-256 process buffer
 */
__WEAK void mbedtls_sha256_update( mbedtls_sha256_context *ctx, const unsigned char *input,
                    size_t ilen )
{
    size_t fill;
    uint32_t left;

    if( ilen == 0 )
        return;

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += (uint32_t) ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if( ctx->total[0] < (uint32_t) ilen )
        ctx->total[1]++;

    if( left && ilen >= fill )
    {
        memcpy( (void *) (ctx->buffer + left), input, fill );
        mbedtls_sha256_process( ctx, ctx->buffer );
        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while( ilen >= 64 )
    {
        mbedtls_sha256_process( ctx, input );
        input += 64;
        ilen  -= 64;
    }

    if( ilen > 0 )
        memcpy( (void *) (ctx->buffer + left), input, ilen );
}

static const unsigned char sha256_padding[64] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*
 * SHA-256 final digest
 */
__WEAK void mbedtls_sha256_finish( mbedtls_sha256_context *ctx, unsigned char output[32] )
{
    uint32_t last, padn;
    uint32_t high, low;
    unsigned char msglen[8];

    high = ( ctx->total[0] >> 29 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_UINT32_BE( high, msglen, 0 );
    PUT_UINT32_BE( low,  msglen, 4 );

    last = ctx->total[0] & 0x3F;
    padn = ( last < 56 ) ? ( 56 - last ) : ( 120 - last );

    mbedtls_sha256_update( ctx, sha256_padding, padn );
    mbedtls_sha256_update( ctx, msglen, 8 );

    PUT_UINT32_BE( ctx->state[0], output,  0 );
    PUT_UINT32_BE( ctx->state[1], output,  4 );
    PUT_UINT32_BE( ctx->state[2], output,  8 );
    PUT_UINT32_BE( ctx->state[3], output, 12 );
    PUT_UINT32_BE( ctx->state[4], output, 16 );
    PUT_UINT32_BE( ctx->state[5], output, 20 );
    PUT_UINT32_BE( ctx->state[6], output, 24 );

    if( ctx->is224 == 0 )
        PUT_UINT32_BE( ctx->state[7], output, 28 );
}

/*
 * output = SHA-256( input buffer )
 */
__WEAK void mbedtls_sha256( const unsigned char *input, size_t ilen,
             unsigned char output[32], int is224 )
{
    mbedtls_sha256_context ctx;

    mbedtls_sha256_init( &ctx );
    mbedtls_sha256_starts( &ctx, is224 );
    mbedtls_sha256_update( &ctx, input, ilen );
    mbedtls_sha256_finish( &ctx, output );
    mbedtls_sha256_free( &ctx );
}



#define SHA256_DIGEST_SIZE ( 256 / 8)
#define SHA256_BLOCK_SIZE  ( 512 / 8)

typedef struct {
    /** Digest-specific context */
    mbedtls_sha256_context *md_ctx;

    /** HMAC part of the context */
    unsigned char ipad[64];
    unsigned char opad[64];
} mbedtls_md_context_t;



void mbedtls_md_init( mbedtls_md_context_t *ctx )
{
    memset( ctx, 0, sizeof( mbedtls_md_context_t ) );
}

void mbedtls_md_free( mbedtls_md_context_t *ctx )
{
    mbedtls_sha256_free((mbedtls_sha256_context *)ctx->md_ctx);
//  free((mbedtls_sha256_context *)ctx->md_ctx);
}


int mbedtls_md_setup( mbedtls_md_context_t *ctx, int hmac )
{
//    if( ( ctx->md_ctx = malloc(sizeof(mbedtls_sha256_context)) ) == NULL )
//        return( -1 );
    return 0;
}

int mbedtls_md_hmac_starts( mbedtls_md_context_t *ctx, const unsigned char *key, size_t keylen )
{

    unsigned char sum[SHA256_DIGEST_SIZE];
    size_t i;

    if( keylen > (size_t) SHA256_BLOCK_SIZE )
    {
        mbedtls_sha256_starts( (mbedtls_sha256_context *) ctx->md_ctx, 0);
        mbedtls_sha256_update( (mbedtls_sha256_context *) ctx->md_ctx, key, keylen );
        mbedtls_sha256_finish( (mbedtls_sha256_context *) ctx->md_ctx, sum );

        keylen = SHA256_DIGEST_SIZE;
        key = sum;
    }


    memset( ctx->ipad, 0x36, SHA256_BLOCK_SIZE );
    memset( ctx->opad, 0x5C, SHA256_BLOCK_SIZE );

    for( i = 0; i < keylen; i++ )
    {
        ctx->ipad[i] = (unsigned char)( ctx->ipad[i] ^ key[i] );
        ctx->opad[i] = (unsigned char)( ctx->opad[i] ^ key[i] );
    }

    mbedtls_zeroize( sum, sizeof( sum ) );

    mbedtls_sha256_starts( (mbedtls_sha256_context *) ctx->md_ctx, 0);
    mbedtls_sha256_update( (mbedtls_sha256_context *) ctx->md_ctx, ctx->ipad, SHA256_BLOCK_SIZE );

    return 0;

}

int mbedtls_md_hmac_update( mbedtls_md_context_t *ctx, const unsigned char *input, size_t ilen )
{
    mbedtls_sha256_update( (mbedtls_sha256_context *) ctx->md_ctx, input, ilen );
    return 0;
}

int mbedtls_md_hmac_finish( mbedtls_md_context_t *ctx, unsigned char *output )
{
    unsigned char tmp[SHA256_DIGEST_SIZE];

    mbedtls_sha256_finish( (mbedtls_sha256_context *) ctx->md_ctx, tmp );
    mbedtls_sha256_starts( (mbedtls_sha256_context *) ctx->md_ctx , 0);
    mbedtls_sha256_update( (mbedtls_sha256_context *) ctx->md_ctx, ctx->opad, SHA256_BLOCK_SIZE );
    mbedtls_sha256_update( (mbedtls_sha256_context *) ctx->md_ctx, tmp, SHA256_DIGEST_SIZE );
    mbedtls_sha256_finish( (mbedtls_sha256_context *) ctx->md_ctx, output );

    return 0;
}

int mbedtls_md_hmac_reset( mbedtls_md_context_t *ctx )
{

    mbedtls_sha256_starts( (mbedtls_sha256_context *) ctx->md_ctx , 0);
    mbedtls_sha256_update( (mbedtls_sha256_context *) ctx->md_ctx, ctx->ipad, SHA256_BLOCK_SIZE );

    return 0 ;
}

int mbedtls_md_hmac( const unsigned char *key, size_t keylen, const unsigned char *input, size_t ilen,
                unsigned char *output )
{
    mbedtls_md_context_t ctx;
    mbedtls_sha256_context sha_ctx;
    int ret;

    mbedtls_md_init( &ctx );
    memset(&sha_ctx, 0, sizeof(sha_ctx));
    ctx.md_ctx = &sha_ctx;
    if( ( ret = mbedtls_md_setup( &ctx, 1 ) ) != 0 )
        return( ret );

    mbedtls_md_hmac_starts( &ctx, key, keylen );
    mbedtls_md_hmac_update( &ctx, input, ilen );
    mbedtls_md_hmac_finish( &ctx, output );

//    mbedtls_md_free( &ctx );

    return( 0 );
}

unsigned int mbedtls_sha256_hkdf(const unsigned char *key, unsigned int key_len,
                                const unsigned char *salt, unsigned int salt_len,
                                const unsigned char *info, unsigned int info_len,
                                unsigned char *out, unsigned int out_len)
{
    const unsigned char null_salt[32] = {0};
    unsigned char PRK[32];
    unsigned char T_n[32];
    unsigned int loop;
    unsigned int temp_len;

    // Step 1: HKDF-Extract(salt, IKM) -> PRK
    if (salt == NULL)
        mbedtls_md_hmac(null_salt, 32, key, key_len, PRK);
    else
        mbedtls_md_hmac(salt, salt_len, key, key_len, PRK);

    // Step 2: HKDF-Expand(PRK, info, L) -> OKM
    //T(0) = empty string (zero length)
    //T(1) = HMAC-Hash(PRK, T(0) | info | 0x01)
    //T(2) = HMAC-Hash(PRK, T(1) | info | 0x02)
    //T(3) = HMAC-Hash(PRK, T(2) | info | 0x03)

    unsigned char temp[32 + info_len + 1];
    memset(temp, 0, 32 + info_len +1);
    loop = CEIL_DIV(out_len, 32);

    for (int i = 0; i < loop ; i++) {
        if (i == 0) {
            temp_len = 0;
        } else {
            memcpy(temp, T_n, 32);
            temp_len = 32;
        }

        memcpy(temp + temp_len, info, info_len);
        temp_len += info_len;

        temp[temp_len] = i + 1;
        temp_len += 1;

        mbedtls_md_hmac(PRK, 32, temp, temp_len, T_n);

        memcpy(out + 32 * i, T_n, MIN(32, out_len));
        out_len -= 32;
    }

    return 0;
}



/***************************************************this is for test************************************************/
#if (MBEDTLS_SELF_TEST == 1)

///////////////////////////////////////////
//Hash = SHA-256
//IKM  = 0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b (22 octets)
//salt = 0x000102030405060708090a0b0c (13 octets)
//info = 0xf0f1f2f3f4f5f6f7f8f9 (10 octets)
//L    = 42
//
//PRK  = 0x077709362c2e32df0ddc3f0dc47bba63
//       90b6c73bb50f9c3122ec844ad7c2b3e5 (32 octets)
//OKM  = 0x3cb25f25faacd57a90434f64d0362f2a
//       2d2d0a90cf1a5a4c5db02d56ecc4c5bf
//       34007208d5b887185865 (42 octets)

#if 0
unsigned char salt[13]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c};
unsigned char info[10]={0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9};
unsigned char key_material[22]={0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b};
#endif


void hkdf_test(void)
{
    unsigned char key_material[80]={0x00,
            0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,
            0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,
            0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,
            0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,
            0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f};

    unsigned char salt[80]={0x60,
            0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,
            0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0x80,
            0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,
            0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,
            0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf};

    unsigned char info[80]={0xb0,
            0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,
            0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,
            0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0xe0,
            0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,
            0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff};

    unsigned char expect[82]={
            0xb1,0x1e,0x39,0x8d,0xc8,0x03,0x27,0xa1,0xc8,0xe7,0xf7,0x8c,0x59,0x6a,0x49,0x34,
            0x4f,0x01,0x2e,0xda,0x2d,0x4e,0xfa,0xd8,0xa0,0x50,0xcc,0x4c,0x19,0xaf,0xa9,0x7c,
            0x59,0x04,0x5a,0x99,0xca,0xc7,0x82,0x72,0x71,0xcb,0x41,0xc6,0x5e,0x59,0x0e,0x09,
            0xda,0x32,0x75,0x60,0x0c,0x2f,0x09,0xb8,0x36,0x77,0x93,0xa9,0xac,0xa3,0xdb,0x71,
            0xcc,0x30,0xc5,0x81,0x79,0xec,0x3e,0x87,0xc1,0x4c,0x01,0xd5,0xc1,0xf3,0x43,0x4f,
            0x1d,0x87
    };
    unsigned char out_key[82];

    sha256_hkdf(key_material,sizeof(key_material),salt,sizeof(salt),info,sizeof(info),out_key,82);

    MI_LOG_INFO("SHA256-HKDF TEST: ");
    if (memcmp(expect, out_key, 82) == 0) {
        MI_LOG_PRINTF(" PASS\n");
    }
    else {
        MI_LOG_PRINTF(" FAIL\n");
    }

}


/*
 * FIPS-180-2 test vectors
 */
static const unsigned char sha256_test_buf[3][57] =
{
    { "abc" },
    { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" },
    { "" }
};

static const int sha256_test_buflen[3] =
{
    3, 56, 1000
};

static const unsigned char sha256_test_sum[6][32] =
{
    /*
     * SHA-224 test vectors
     */
    { 0x23, 0x09, 0x7D, 0x22, 0x34, 0x05, 0xD8, 0x22,
      0x86, 0x42, 0xA4, 0x77, 0xBD, 0xA2, 0x55, 0xB3,
      0x2A, 0xAD, 0xBC, 0xE4, 0xBD, 0xA0, 0xB3, 0xF7,
      0xE3, 0x6C, 0x9D, 0xA7 },
    { 0x75, 0x38, 0x8B, 0x16, 0x51, 0x27, 0x76, 0xCC,
      0x5D, 0xBA, 0x5D, 0xA1, 0xFD, 0x89, 0x01, 0x50,
      0xB0, 0xC6, 0x45, 0x5C, 0xB4, 0xF5, 0x8B, 0x19,
      0x52, 0x52, 0x25, 0x25 },
    { 0x20, 0x79, 0x46, 0x55, 0x98, 0x0C, 0x91, 0xD8,
      0xBB, 0xB4, 0xC1, 0xEA, 0x97, 0x61, 0x8A, 0x4B,
      0xF0, 0x3F, 0x42, 0x58, 0x19, 0x48, 0xB2, 0xEE,
      0x4E, 0xE7, 0xAD, 0x67 },

    /*
     * SHA-256 test vectors
     */
    { 0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
      0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
      0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
      0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD },
    { 0x24, 0x8D, 0x6A, 0x61, 0xD2, 0x06, 0x38, 0xB8,
      0xE5, 0xC0, 0x26, 0x93, 0x0C, 0x3E, 0x60, 0x39,
      0xA3, 0x3C, 0xE4, 0x59, 0x64, 0xFF, 0x21, 0x67,
      0xF6, 0xEC, 0xED, 0xD4, 0x19, 0xDB, 0x06, 0xC1 },
    { 0xCD, 0xC7, 0x6E, 0x5C, 0x99, 0x14, 0xFB, 0x92,
      0x81, 0xA1, 0xC7, 0xE2, 0x84, 0xD7, 0x3E, 0x67,
      0xF1, 0x80, 0x9A, 0x48, 0xA4, 0x97, 0x20, 0x0E,
      0x04, 0x6D, 0x39, 0xCC, 0xC7, 0x11, 0x2C, 0xD0 }
};

/*
 * Checkup routine
 */
int mbedtls_sha256_self_test( int verbose )
{
    int i, j, k, buflen, ret = 0;
    unsigned char buf[1024];
    unsigned char sha256sum[32];
    mbedtls_sha256_context ctx;

    mbedtls_sha256_init( &ctx );

    for( i = 0; i < 6; i++ )
    {
        j = i % 3;
        k = i < 3;

        if( verbose != 0 )
            mbedtls_printf( "  SHA-%d test #%d: ", 256 - k * 32, j + 1 );

        mbedtls_sha256_starts( &ctx, k );

        if( j == 2 )
        {
            memset( buf, 'a', buflen = 1000 );

            for( j = 0; j < 1000; j++ )
                mbedtls_sha256_update( &ctx, buf, buflen );
        }
        else
            mbedtls_sha256_update( &ctx, sha256_test_buf[j],
                                 sha256_test_buflen[j] );

        mbedtls_sha256_finish( &ctx, sha256sum );

        if( memcmp( sha256sum, sha256_test_sum[i], 32 - k * 4 ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed\n" );

            ret = 1;
            goto exit;
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );

exit:
    mbedtls_sha256_free( &ctx );

    return( ret );
}


#endif /* MBEDTLS_SELF_TEST */

