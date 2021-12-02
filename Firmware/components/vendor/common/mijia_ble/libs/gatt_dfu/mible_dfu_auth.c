/* Includes ------------------------------------------------------------------*/
//#include <stdlib.h>
//#include <stddef.h>
#include "mible_log.h"
#include "third_party/mbedtls/sha256_hkdf.h"
#include "cryptography/mi_crypto.h"
#include "cryptography/mi_crypto_backend_mbedtls.h"
#include "mible_dfu_auth.h"
#include "mible_dfu_flash.h"

/* Private define ------------------------------------------------------------*/

#define MIBLE_DFU_FLAG_LZMA                                0x01
#define MIBLE_DFU_FLAG_BSDIFF                              0x02

#define MIBLE_DFU_TLV_NEW_VERSION                          0x01
#define MIBLE_DFU_TLV_OLD_VERSION                          0x02
#define MIBLE_DFU_TLV_SIGN_TYPE                            0x03
#define MIBLE_DFU_TLV_MODEL                                0x04
#define MIBLE_DFU_TLV_PID                                  0x05

#define MIBLE_DFU_CERT_MAX_SIZE                            512

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/* Private typedef -----------------------------------------------------------*/
#if defined ( __CC_ARM )

__packed struct mible_dfu_ver {
    uint8_t major;
    uint8_t minor;
    uint8_t revision;
    uint16_t build;
};
typedef struct mible_dfu_ver mible_dfu_ver_t;

__packed struct mible_dfu_tag {
    uint32_t magic[4];
    uint16_t tag_size;
    uint16_t product_id;
    uint32_t firmware_size;
    uint16_t certification_size;
    uint8_t flag;
    uint8_t reserved;
    uint8_t payload[1] ;
};
typedef struct mible_dfu_tag mible_dfu_tag_t;

__packed struct mible_dfu_tlv {
    uint8_t type;
    uint8_t length;
    uint8_t value[1];
};
typedef struct mible_dfu_tlv mible_dfu_tlv_t;

#elif defined   ( __GNUC__ )

struct __PACKED mible_dfu_ver {
    uint8_t major;
    uint8_t minor;
    uint8_t revision;
    uint16_t build;
};
typedef struct mible_dfu_ver mible_dfu_ver_t;

struct __PACKED mible_dfu_tag {
    uint32_t magic[4];
    uint16_t tag_size;
    uint16_t product_id;
    uint32_t firmware_size;
    uint16_t certification_size;
    uint8_t flag;
    uint8_t reserved;
    uint8_t payload[1] ;
};
typedef struct mible_dfu_tag mible_dfu_tag_t;

struct __PACKED mible_dfu_tlv {
    uint8_t type;
    uint8_t length;
    uint8_t value[1];
};
typedef struct mible_dfu_tlv mible_dfu_tlv_t;

#endif

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

static uint8_t get_value(uint32_t offset)
{
    uint8_t tmp;
    mible_dfu_flash_read(&tmp, sizeof tmp, offset);

    return tmp;
}

static uint32_t get_offset(uint32_t total_size);
static mible_status_t parse_tlv(uint8_t type, uint32_t offset, uint16_t max_len,
                                    void * data, uint16_t data_len);
static bool verify_firmware(mible_dfu_tag_t * tag, void * root_cert,
                uint16_t root_len, void * cert, void * sign, uint16_t sign_len);

/* Exported functions --------------------------------------------------------*/

mible_status_t mible_dfu_auth(uint32_t total_size, uint32_t product_id)
{
    mible_dfu_tag_t * p_dfu_tag = NULL;
    mible_dfu_ver_t version;
    mible_status_t ret = MI_SUCCESS;
    uint32_t offset;
    uint16_t tag_size, sign_size;
    uint16_t root_size;
    uint8_t dfu_tag[offsetof(mible_dfu_tag_t, payload)];
	uint8_t buffer_for_root[MIBLE_DFU_CERT_MAX_SIZE];
    offset = get_offset(total_size);
    if (0xFFFFFFFF == offset) {
        MI_LOG_WARNING("fail to read magic\n");
        return MI_ERR_NOT_FOUND;
    }
    MI_LOG_DEBUG("find offset in flash -- 0x%x\n", offset);

    ret = mible_dfu_flash_read(&tag_size, sizeof(tag_size), offset + 16); /* 16 bytes magic */
    if (MI_SUCCESS != ret) {
        MI_LOG_WARNING("fail to read tag size (err %d)\n", ret);
        return ret;
    }

    /* CRC and fixed elements */
    MI_LOG_DEBUG("tag length is %d, minimal len is %d\n",
                    tag_size, sizeof(dfu_tag) + 4);
    if (tag_size < sizeof(dfu_tag) + 4) {
        MI_LOG_WARNING("invalid tag size\n");
        return MI_ERR_NOT_FOUND;
    }

    ret = mible_dfu_flash_read(&dfu_tag[0], sizeof(dfu_tag), offset);
    if (MI_SUCCESS != ret) {
        MI_LOG_WARNING("fail to read tag (err %d)\n", ret);
        return ret;
    }

    p_dfu_tag = (mible_dfu_tag_t *)&dfu_tag[0];
    if (product_id != p_dfu_tag->product_id) {
        MI_LOG_WARNING("product ID in firmware is %d\n", p_dfu_tag->product_id);
        return MI_ERR_INTERNAL;
    }
    MI_LOG_DEBUG("flag in firmware is %d\n", p_dfu_tag->flag);

    ret = parse_tlv(MIBLE_DFU_TLV_NEW_VERSION, offset + sizeof(dfu_tag),
                    tag_size - sizeof(dfu_tag) - 4,    /* CRC and fixed elements */
                    &version, sizeof(version));
    if (MI_SUCCESS == ret) {
        MI_LOG_DEBUG("new firmware %u.%u.%u_%04u\n", version.major, version.minor,
                          version.revision, version.build);
    } else {
        MI_LOG_WARNING("Get new firmware version fail (err %d)\n", ret);
    }

    uint8_t certification[p_dfu_tag->certification_size];
    ret = mible_dfu_flash_read(certification, p_dfu_tag->certification_size,
                               p_dfu_tag->firmware_size + tag_size);
    if (MI_SUCCESS != ret) {
        MI_LOG_WARNING("can not read certification from firmware (err %d)\n", ret);
        return ret;
    }

    sign_size = total_size - p_dfu_tag->firmware_size - tag_size - p_dfu_tag->certification_size;
    uint8_t signature[sign_size];
    ret = mible_dfu_flash_read(signature, sign_size,
           p_dfu_tag->firmware_size + tag_size + p_dfu_tag->certification_size);
    if (MI_SUCCESS != ret) {
        MI_LOG_WARNING("can not read signature (err %d)\n", ret);
        return ret;
    }
	root_size = cpy_root_cert(buffer_for_root,(uint16_t) MIBLE_DFU_CERT_MAX_SIZE);
    if (root_size > 512) {
        MI_LOG_ERROR("no invalid root cert\n");
        return MI_ERR_NOT_FOUND;
    }

    if (verify_firmware(p_dfu_tag, buffer_for_root, root_size, certification,
                         signature, sign_size)) {

        MI_LOG_DEBUG("firmware is ready for OTA\n");
        return MI_SUCCESS;
    } else {
        MI_LOG_WARNING("firmware is illegal\n");
        return MI_ERR_INTERNAL;
    }
}

static void get_next(uint8_t ps[], int16_t length, int8_t next[])
{
    int16_t j = 0;
    int8_t k = -1;

    next[0] = -1;
    while (j < length - 1) {
       if (k == -1 || ps[j] == ps[k]) {
           if (ps[++j] == ps[++k]) {
              next[j] = next[k];
           } else {
              next[j] = k;
           }
       } else {
           k = next[k];
       }
    }
}

static uint32_t get_offset(uint32_t total_size)
{
    uint32_t i = 0;
    int8_t next[16], j = 0;
    uint8_t magic[] = {0x47, 0x26, 0x56, 0x82, 0x41, 0x54, 0x4F, 0x46,
                       0x54, 0xEF, 0x49, 0x4D, 0x00, 0x00, 0x00, 0x00};

    get_next(magic, 16, next);
    while (i < total_size && j < 16) {
        if (-1 == j || get_value(total_size - i - 1) == magic[j]) {
            i++;
            j++;
        } else {
            j = next[j];
        }
    }

    return (16 == j) ? total_size - i : 0xFFFFFFFF;
}

static mible_status_t parse_tlv(uint8_t type, uint32_t offset, uint16_t max_len,
                                    void * data, uint16_t data_len)
{
    uint16_t index = 0;
    uint8_t dfu_tlv[offsetof(mible_dfu_tlv_t, value)];
    mible_dfu_tlv_t *p_tlv;
    mible_status_t status;

    if (NULL == data || 0 == data_len) {
        return MI_ERR_INVALID_PARAM;
    }

    MI_LOG_DEBUG("TLV total len is %d\n", max_len);
    p_tlv = (mible_dfu_tlv_t *)&dfu_tlv[0];
    while (index < max_len) {
        if (max_len < (index + sizeof(dfu_tlv))) {
            MI_LOG_WARNING("invalid length to get header\n");
            return MI_ERR_DATA_SIZE;
        }

        status = mible_dfu_flash_read(&dfu_tlv[0], sizeof(dfu_tlv), offset + index);
        if (MI_SUCCESS != status) {
            MI_LOG_WARNING("get TLV header fail (err %d)\n", status);
            return status;
        }

        MI_LOG_DEBUG("type is %d and len is %d\n", p_tlv->type, p_tlv->length);
        if (max_len < (index + sizeof(dfu_tlv) + p_tlv->length)) {
            MI_LOG_WARNING("invalid length to get payload\n");
            return MI_ERR_DATA_SIZE;
        }

        if (p_tlv->type == type && p_tlv->length == data_len) {
            status = mible_dfu_flash_read(data, data_len,
                                            offset + index + sizeof(dfu_tlv));
            if (MI_SUCCESS != status) {
                MI_LOG_WARNING("get TLV payload fail (err %d)\n", status);
                return status;
            }

            return MI_SUCCESS;
        }

        index += p_tlv->length + sizeof(dfu_tlv);
    }

    if (index > max_len) {
        MI_LOG_WARNING("the format of tag might be wrong\n");
    }

    return MI_ERR_NOT_FOUND;
}

static void get_hash(uint8_t sha[32], uint32_t firmware_size)
{
    mbedtls_sha256_context ctx;
    uint32_t size = firmware_size, copy_size;
    uint8_t buffer[32];

    mbedtls_sha256_init( &ctx );
    mbedtls_sha256_starts( &ctx, 0 );

    while (size > 0) {
        copy_size = MIN(size, sizeof(buffer));
        mible_dfu_flash_read(buffer, copy_size, firmware_size - size);
        mbedtls_sha256_update( &ctx, buffer, copy_size );
        size -= copy_size;
    }

    mbedtls_sha256_finish( &ctx, sha );
    mbedtls_sha256_free( &ctx );
}

static bool verify_firmware(mible_dfu_tag_t * tag, void * root_cert,
                            uint16_t root_len, void * cert, void * sign, uint16_t sign_len)
{
    msc_crt_t crt;
    ecc256_pk_t parent_pub;
    int ret;
    uint8_t child_sig[64], child_sha[32], buffer_for_cert[MIBLE_DFU_CERT_MAX_SIZE];
    uint16_t cert_len;
    const unsigned char * cert_develop;

    ret = mi_crypto_crt_parse_der(root_cert, root_len, NULL, &crt);
    if (ret < 0) {
        MI_LOG_WARNING("can not parse root certificate\n");
        return false;
    }

    memcpy(parent_pub, crt.pub.p, 64);    /* get root public key */
    MI_LOG_DEBUG("root public key:\n");
    MI_LOG_HEXDUMP(parent_pub, 64);

    /* get certificate of server */
    cert_len = mbedtls_crt_pem2der(cert, tag->certification_size,
                                   buffer_for_cert, MIBLE_DFU_CERT_MAX_SIZE);
    if (0 == cert_len) {
        MI_LOG_WARNING("can not transfer server certificate\n");
        return false;
    }

    ret = mi_crypto_crt_parse_der(buffer_for_cert, cert_len, NULL, &crt);
    if (ret < 0) {
        MI_LOG_WARNING("can not parse server certificate\n");
        return false;
    }

    /* verify certificate of server */
    memcpy(child_sig, crt.sig, 64);
    mi_crypto_sha256(crt.tbs.p, crt.tbs.len, child_sha);
    ret = mi_crypto_ecc_verify(P256R1, parent_pub, child_sha, child_sig);
    if (ret != 3) {
        MI_LOG_WARNING("fail to verify server certificate\n");
        return false;
    }
    MI_LOG_DEBUG("server certificate is verified\n");

    memcpy(parent_pub, crt.pub.p, 64);     /* get server public key */
    MI_LOG_DEBUG("mi server public key:\n");
    MI_LOG_HEXDUMP(parent_pub, 64);

    /* get certificate of developer */
    cert_develop = (unsigned char *)strstr( (const char *) cert, PEM_FOOTER );
    if (NULL == cert_develop) {
        MI_LOG_WARNING("can not find developer certificate\n");
        return false;
    }

    cert_len = mbedtls_crt_pem2der(cert_develop + strlen(PEM_FOOTER),
                                    tag->certification_size,
                                    buffer_for_cert,
                                    MIBLE_DFU_CERT_MAX_SIZE);
    if (0 == cert_len) {
        MI_LOG_WARNING("can not transfer developer certificate\n");
        return false;
    }

    ret = mi_crypto_crt_parse_der(buffer_for_cert, cert_len, NULL, &crt);
    if (ret < 0) {
        MI_LOG_WARNING("can not parse developer certificate\n");
        return false;
    }

    /* verify certificate of developer */
    memcpy(child_sig, crt.sig, 64);
    mi_crypto_sha256(crt.tbs.p, crt.tbs.len, child_sha);
    ret = mi_crypto_ecc_verify(P256R1, parent_pub, child_sha, child_sig);
    if (ret != 3) {
        MI_LOG_WARNING("fail to verify developer certificate\n");
        return false;
    }
    MI_LOG_DEBUG("developer certificate is verified\n");

    memcpy(parent_pub, crt.pub.p, 64);    /* get developer public key */
    MI_LOG_DEBUG("developer public key:\n");
    MI_LOG_HEXDUMP(parent_pub, 64);

    /* verify signature */
    ret = mbedtls_read_signature(sign, sign_len, child_sig, 64);
    if (ret < 0) {
        MI_LOG_WARNING("fail to get signature from firmware\n");
        return false;
    }

    get_hash(child_sha, tag->firmware_size + tag->tag_size);
    ret = mi_crypto_ecc_verify(P256R1, parent_pub, child_sha, child_sig);
    if (ret < 0) {
        MI_LOG_WARNING("fail to verify signature\n");
        return false;
    }
    MI_LOG_DEBUG("firmware signature is verified\n");

    return true;
}


