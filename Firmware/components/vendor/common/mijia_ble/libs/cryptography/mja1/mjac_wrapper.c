/**
 * @file mjac_wrapper.c
 * @brief Build command and parse data from repsponse.
 * @author MIOT
 */
#include "mjac_utils.h"
#include "mjac_wrapper.h"
#include "mible_log.h"

#define PARAMS_VALIDATE 1

#if PARAMS_VALIDATE
#define LV_VALIDATE(L, V, MIN, MAX) (V && (MIN <= L && L <= MAX))
#define CMD_VALIDATE(CMD, CMD_SZ, DATA_SZ) (CMD && CMD_SZ >= DATA_SZ+MJAC_SZ_HEAD_TAIL)
#else
#define LV_VALIDATE(L, V, MIN, MAX) 1
#define CMD_VALIDATE(CMD, CMD_SZ, DATA_SZ) 1
#endif

#define CMD_BUILD_HEAD(OUT, SZ, CODE) \
    OUT[SZ++] = CODE & 0x1F

#define CMD_APPEND_CRC(OUT, SZ, CRC) \
    CRC = crc16_ccitt(OUT, SZ); \
    OUT[SZ++] = (CRC>>8)&0xFF; \
    OUT[SZ++] = (CRC)&0xFF

/**
 * @brief MJAC command codes enumeration
 */
typedef enum
{
    MJAC_ECHO               = 0x00,
    MJAC_GENERATE_RANDOM    = 0x02,
    MJAC_READ               = 0x05,
    MJAC_UPDATE             = 0x06,
    MJAC_HIBERNATE          = 0x0D,
    MJAC_PUT_ATTRIBUTE      = 0x10,
    MJAC_GENERATE_KEY       = 0x11,
    MJAC_QUERY              = 0x14,
    MJAC_GENERATE_SIGNATURE = 0x16,
    MJAC_VERIFY_SIGNATURE   = 0x17,
    MJAC_ESTABLISH_KEY      = 0x18
} mjac_cmd_code_t;

static const UINT8 ecc_curve_nist_p256[] = {
    0x2A, 0X86, 0X48, 0XCE, 0X3D, 0X03, 0X01, 0X07
};

UINT16 mjac_cmd_build_echo(
        UINT8 *cmd, UINT16 cmd_sz,
        const UINT8 *echo_data, UINT16 echo_data_sz)
{
    UINT16 sz = 0;
    UINT16 crc = 0;

    if(!echo_data || !echo_data_sz)
        echo_data_sz = 0;
    else if(!LV_VALIDATE(echo_data_sz, echo_data, 1, MJAC_SZ_RESP_DATA_MAX))
        return 0;

    if(!CMD_VALIDATE(cmd, cmd_sz, echo_data_sz))
        return 0;

    CMD_BUILD_HEAD(cmd, sz, MJAC_ECHO);
    if (echo_data_sz) {
        MEMCPY(&cmd[sz], echo_data, echo_data_sz);
        sz += echo_data_sz;
    }
    CMD_APPEND_CRC(cmd, sz, crc);

    return sz;
}

UINT16 mjac_cmd_build_generate_random(
        UINT8 *cmd, UINT16 cmd_sz,
        UINT8 random_sz)
{
    UINT16 sz = 0;
    UINT16 crc = 0;

    if(!random_sz || !CMD_VALIDATE(cmd, cmd_sz, 2))
        return 0;

    CMD_BUILD_HEAD(cmd, sz, MJAC_GENERATE_RANDOM);
    cmd[sz++] = 0x00;
    cmd[sz++] = random_sz;
    CMD_APPEND_CRC(cmd, sz, crc);

    return sz;
}

UINT16 mjac_cmd_build_hibernate(
        UINT8 *cmd, UINT16 cmd_sz)
{
    UINT16 sz = 0;
    UINT16 crc = 0;

    if(!CMD_VALIDATE(cmd, cmd_sz, 1))
        return 0;

    CMD_BUILD_HEAD(cmd, sz, MJAC_HIBERNATE);
    cmd[sz++] = 0x02;
    CMD_APPEND_CRC(cmd, sz, crc);

    return sz;
}

UINT16 mjac_cmd_build_read(
        UINT8 *cmd, UINT16 cmd_sz,
        mjac_zone_t idx, UINT16 offset, UINT16 length)
{
    UINT16 sz = 0;
    UINT16 crc = 0;

    if(!CMD_VALIDATE(cmd, cmd_sz, 6) ||
            !(length > 0 && length <= MJAC_SZ_RESP_DATA_MAX))
        return 0;

    CMD_BUILD_HEAD(cmd, sz, MJAC_READ);
    cmd[sz++] = 0x00;
    cmd[sz++] = idx;
    cmd[sz++] = (offset>>8)&0xFF;
    cmd[sz++] = (offset)&0xFF;
    cmd[sz++] = (length>>8)&0xFF;
    cmd[sz++] = (length)&0xFF;
    CMD_APPEND_CRC(cmd, sz, crc);

    return sz;
}

UINT16 mjac_cmd_build_generate_signature(
        UINT8 *cmd, UINT16 cmd_sz,
        const UINT8 *hash, UINT16 hash_sz)
{
    UINT16 sz = 0;
    UINT16 crc = 0;

    if(!LV_VALIDATE(hash_sz, hash, MJAC_SZ_HASH, MJAC_SZ_HASH)
            || !CMD_VALIDATE(cmd, cmd_sz, 3+hash_sz))
        return 0;

    CMD_BUILD_HEAD(cmd, sz, MJAC_GENERATE_SIGNATURE);
    cmd[sz++] = 0x00;
    MEMCPY(&cmd[sz], hash, hash_sz);
    sz += hash_sz;
    CMD_APPEND_CRC(cmd, sz, crc);

    return sz;
}

UINT16 mjac_cmd_build_verify_signature(
        UINT8 *cmd, UINT16 cmd_sz,
        const UINT8 *pubkey, UINT16 pubkey_sz,
        const UINT8 *signature, UINT16 signature_sz,
        const UINT8 *hash, UINT16 hash_sz)
{
    UINT16 sz = 0;
    UINT16 crc = 0;

    if(!LV_VALIDATE(pubkey_sz, pubkey, MJAC_SZ_PUBKEY, MJAC_SZ_PUBKEY) 
            || !LV_VALIDATE(signature_sz, signature, MJAC_SZ_SIGN, MJAC_SZ_SIGN)
            || !LV_VALIDATE(hash_sz, hash, MJAC_SZ_HASH, MJAC_SZ_HASH)
            || !CMD_VALIDATE(cmd, cmd_sz, 1+sizeof(ecc_curve_nist_p256)+pubkey_sz
                +signature_sz+hash_sz))
        return 0;

    CMD_BUILD_HEAD(cmd, sz, MJAC_VERIFY_SIGNATURE);
    cmd[sz++] = 0x00;
    cmd[sz++] = (sizeof(ecc_curve_nist_p256)>>8)&0xFF;
    cmd[sz++] = sizeof(ecc_curve_nist_p256)&0xFF;
    MEMCPY(&cmd[sz], ecc_curve_nist_p256, sizeof(ecc_curve_nist_p256));
    sz += sizeof(ecc_curve_nist_p256);
    MEMCPY(&cmd[sz], pubkey, pubkey_sz);
    sz += pubkey_sz;
    MEMCPY(&cmd[sz], signature, signature_sz);
    sz += signature_sz;
    MEMCPY(&cmd[sz], hash, hash_sz);
    sz += hash_sz;
    CMD_APPEND_CRC(cmd, sz, crc);

    return sz;
}

UINT16 mjac_cmd_build_establish_key(
        UINT8 *cmd, UINT16 cmd_sz,
        const UINT8 *pubkey, UINT16 pubkey_sz)
{
    UINT16 sz = 0;
    UINT16 crc = 0;

    if(!LV_VALIDATE(pubkey_sz, pubkey, MJAC_SZ_PUBKEY, MJAC_SZ_PUBKEY)
            || !CMD_VALIDATE(cmd, cmd_sz, 1+pubkey_sz))
        return 0;

    CMD_BUILD_HEAD(cmd, sz, MJAC_ESTABLISH_KEY);
    cmd[sz++] = 0xFF;
    MEMCPY(&cmd[sz], pubkey, pubkey_sz);
    sz += pubkey_sz;
    CMD_APPEND_CRC(cmd, sz, crc);

    return sz;
}

UINT16 mjac_cmd_build_generate_key(
        UINT8 *cmd, UINT16 cmd_sz)
{
    UINT16 sz = 0;
    UINT16 crc = 0;

    if(!CMD_VALIDATE(cmd, cmd_sz, 6+sizeof(ecc_curve_nist_p256)))
        return 0;

    CMD_BUILD_HEAD(cmd, sz, MJAC_GENERATE_KEY);
    cmd[sz++] = 0x13;
    cmd[sz++] = 0xFF;
    cmd[sz++] = 0x00;
    cmd[sz++] = 0x01;
    cmd[sz++] = 0x00;
    cmd[sz++] = 0x00;
    cmd[sz++] = (sizeof(ecc_curve_nist_p256)>>8)&0xFF;
    cmd[sz++] = sizeof(ecc_curve_nist_p256)&0xFF;
    MEMCPY(&cmd[sz], ecc_curve_nist_p256, sizeof(ecc_curve_nist_p256));
    sz += sizeof(ecc_curve_nist_p256);
    CMD_APPEND_CRC(cmd, sz, crc);

    return sz;
}

UINT16 mjac_cmd_build_query(
        UINT8 *cmd, UINT16 cmd_sz)
{
    UINT16 sz = 0;
    UINT16 crc = 0;

    if(!CMD_VALIDATE(cmd, cmd_sz, 1))
        return 0;

    CMD_BUILD_HEAD(cmd, sz, MJAC_QUERY);
    cmd[sz++] = 0x11;
    CMD_APPEND_CRC(cmd, sz, crc);

    return sz;
}

mjac_resp_code_t mjac_resp_parse_data(
        UINT8 *resp, UINT16 *data_sz)
{
    mjac_resp_code_t code = MJAC_OK;
    UINT8 *pdata = NULL;
    UINT16 length = 0;
    UINT16 crc = 0;

    if(!resp || !data_sz)
        return MJAC_UNEXPECTED_ERROR;

    length = (resp[1]<<8 | resp[2])-2;
    if(length > MJAC_SZ_RESP_DATA_MAX)
        return MJAC_COMMUNICATION_ERROR;

    code = (mjac_resp_code_t)resp[0];
    pdata = &resp[3];

    *(pdata-1) = code;
    crc = pdata[length]<<8 | pdata[length+1];
    if(crc != crc16_ccitt(pdata-1, 1+length))
        return MJAC_COMMUNICATION_ERROR;

    MEMCPY(resp, pdata, length);
    *data_sz = length;

    return code;
}


UINT16 mjac_cmd_build_update(
        UINT8 *cmd, UINT16 cmd_sz,
        mjac_zone_t idx, UINT16 offset, const UINT8 *data, UINT16 data_sz)
{
    UINT16 sz = 0;
    UINT16 crc = 0;

    if(idx != MJAC_ZONE_USER || offset >= 4096
            || !(data && data_sz) || !(cmd && cmd_sz >= 4+data_sz))
    {
    	MI_LOG_ERROR("para error\n");
        return 0;
    }

    cmd[sz++] = 0x06;
    cmd[sz++] = 0x00;
    cmd[sz++] = idx;
    cmd[sz++] = (offset>>8)&0xFF;
    cmd[sz++] = (offset)&0xFF;
    MEMCPY(&cmd[sz], data, data_sz);
    sz += data_sz;
    crc = crc16_ccitt(cmd, sz);
    cmd[sz++] = (crc>>8)&0xFF;
    cmd[sz++] = (crc)&0xFF;

    return sz;
}

