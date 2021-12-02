#ifndef __MJAC_WRAPPER_H
#define __MJAC_WRAPPER_H
#include "mjac_defs.h"

/*
 * Data Zone
 * +-------+--------------------------------------+
 * | INDEX | REMARKS                              |
 * +-------+--------------------------------------+
 * |   0   | Leaf certificate in DER.             |
 * |   1   | Intermediate certificate in DER.     |
 * |   2   | Root certificate in DER.             |
 * |   3   | Product Infomation, see table below. |
 * +-------+--------------------------------------+
 *
 * Product Infomation
 * +------+-----------+---------------------------+
 * | ITEM | SIZE/BYTE | REMARKS                   |
 * +------+-----------+---------------------------+
 * | PID  | 1         | Product ID                |
 * | MID  | 1         | Manufacturer ID           |
 * | PID  | 8         | Device ID, big-endian     |
 * +------+-----------+---------------------------+
 */

/*
 * Data Coding
 *
 * ECDSA Signature
 * +--------------+--------+---------+
 * | DATA ELEMENT | FIELD  | LENGTH  |
 * +--------------+--------+---------+
 * | R            | Length | 2 bytes |
 * |              | Value  | -       |
 * +--------------+--------+---------+
 * | S            | Length | 2 bytes |
 * |              | Value  | -       |
 * +--------------+--------+---------+
 *
 * ECC Pubkey Key
 * +---------------------------------+--------+---------+---------------------+
 * | DATA ELEMENT                    | FIELD  | LENGTH  | REMARKS             |
 * +---------------------------------+--------+---------+---------------------+
 * | Point representation identifier | Value  | 1 byte  | Uncompressed: '04h' |
 * +---------------------------------+--------+---------+---------------------+
 * | x-coordinate                    | Length | 2 bytes | -                   |
 * |                                 | Value  | -       | -                   |
 * +---------------------------------+--------+---------+---------------------+
 * | y-coordinate                    | Length | 2 bytes | -                   |
 * |                                 | Value  | -       | -                   |
 * +---------------------------------+--------+---------+---------------------+
 *
 * Hash
 * +--------------+--------+---------+
 * | DATA ELEMENT | FIELD  | LENGTH  |
 * +--------------+--------+---------+
 * | Hash         | Length | 2 bytes |
 * |              | Value  | -       |
 * +--------------+--------+---------+
 *
 * Shared Secret
 * +--------------+--------+---------+
 * | DATA ELEMENT | FIELD  | LENGTH  |
 * +--------------+--------+---------+
 * | Shared secret| Length | 2 bytes |
 * |              | Value  | -       |
 * +--------------+--------+---------+
 */

/* total size of the command/response head-tail */
#define MJAC_SZ_HEAD_TAIL     ((UINT8)3)
/* size of response data(response code || response data), no command length */
#define MJAC_SZ_RESP_DATA_MAX ((UINT16)507)
/* size of command (command header || command data) */
#define MJAC_SZ_CMD_MAX       ((UINT16)510)

/* coded ecc public key size is 1+2+32+2+32=69  */
#define MJAC_SZ_PUBKEY  ((UINT8)69)
/* ecdsa signature size is 2+32+2+32=68 */
#define MJAC_SZ_SIGN    ((UINT8)68)
/* sha digest size is 2+32=64 */
#define MJAC_SZ_HASH    ((UINT8)34)
/* ecdh shared secret size is 2+32=34 */
#define MJAC_SZ_SECRET  ((UINT8)34)

/**
 * @brief MJAC response codes enumeration
 */
typedef enum
{
    MJAC_OK                           = 0x00,
    MJAC_COMMUNICATION_ERROR          = 0x01,
    MJAC_INCONSISTENT_COMMAND_DATA    = 0x02,
    MJAC_VALUE_OUT_OF_RANGE           = 0x03,
    MJAC_UNSUPPORTED_COMMAND_CODE     = 0x04,
    MJAC_UNEXPECTED_ERROR             = 0x05,
    MJAC_BUFFER_LENGTH_EXCEEDED       = 0x06,
    MJAC_KEY_NOT_FOUND                = 0x0A,
    MJAC_ENTRY_NOT_FOUND              = 0x10,
    MJAC_BOUNDARY_EXCEEDED            = 0x14
} mjac_resp_code_t;

/**
 * @brief data zone types enumeration
 */
typedef enum
{
    MJAC_ZONE_DEV_CRT  = 0,
    MJAC_ZONE_MANU_CRT = 1,
    MJAC_ZONE_ROOT_CRT = 2,
    MJAC_ZONE_PRODUCT  = 3,
    MJAC_ZONE_USER = 4
} mjac_zone_t;

/**
 * @brief command for connection check.
 *        response data will be same as what you sent.
 *
 * @param cmd - out buffer.
 * @param echo_data - data to echo
 *                    NULL for no data.
 *
 * @return length of the built cmd.
 */
extern UINT16 mjac_cmd_build_echo(
        UINT8 *cmd, UINT16 cmd_sz,
        const UINT8 *echo_data, UINT16 echo_data_sz);

/**
 * @brief generate cryptographically secure random numbers.
 *        response random size will be same as the size you specified.
 *
 * @param cmd - out buffer.
 * @param random_sz - size = [1,255].
 *
 * @return length of the built cmd.
 */
extern UINT16 mjac_cmd_build_generate_random(
        UINT8 *cmd, UINT16 cmd_sz,
        UINT8 random_sz);

/**
 * @brief hibernate to low power consumption.
 *        cause chip will hibernate immediatly, you SHOULD just read MJAC_SZ_HEAD_TAIL once.
 *
 * @param cmd - out buffer.
 *
 * @return length of the built cmd.
 */
extern UINT16 mjac_cmd_build_hibernate(
        UINT8 *cmd, UINT16 cmd_sz);

/**
 * @brief read data from data zone.
 *
 * @param cmd - out buffer
 * @param idx - index of the zone to read, see "Table Data Zone" above.
 * @param offset - offset start to read.
 * @param length - length of data to read.
 *
 * @return length of the built cmd.
 */
extern UINT16 mjac_cmd_build_read(
        UINT8 *cmd, UINT16 cmd_sz,
        mjac_zone_t idx, UINT16 offset, UINT16 length);

/**
 * @brief generate signature over the message digest.
 *
 * @param cmd - out buffer
 * @param hash - message digest, coding as "Table Hash" above.
 *
 * @return length of the built cmd.
 */
extern UINT16 mjac_cmd_build_generate_signature(
        UINT8 *cmd, UINT16 cmd_sz,
        const UINT8 *hash, UINT16 hash_sz);

/**
 * @brief verify signature over the message digest.
 *        response data will be 1 byte, and 1 on success, 0 for fail.
 *
 * @param cmd - out buffer
 * @param pubkey - public key, coding as "Table ECC Public Key" above.
 * @param signature - signature, coding as "Table ECDSA Signature" above.
 * @param hash - message digest, coding as "Table Hash" above.
 *
 * @return length of the built cmd.
 */
extern UINT16 mjac_cmd_build_verify_signature(
        UINT8 *cmd, UINT16 cmd_sz,
        const UINT8 *pubkey, UINT16 pubkey_sz,
        const UINT8 *signature, UINT16 signature_sz,
        const UINT8 *hash, UINT16 hash_sz);

/**
 * @brief establish shared secret key(coding as Table "Shared Secret" above).
 *        aka ECDH, you should call mjac_cmd_build_generate_key first.
 *        response will be shared secret key.
 *
 * @param cmd - out buffer
 * @param pubkey - peer pubkey key, coding as Table "ECC Public key" above.
 *
 * @return length of the built cmd.
 */
extern UINT16 mjac_cmd_build_establish_key(
        UINT8 *cmd, UINT16 cmd_sz,
        const UINT8 *pubkey, UINT16 pubkey_sz);

/**
 * @brief generate ephemeral ecc keypair for ECDH.
 *        response will be public key to exchange.
 *
 * @param cmd - out buffer
 *
 * @return length of the built cmd.
 */
extern UINT16 mjac_cmd_build_generate_key(
        UINT8 *cmd, UINT16 cmd_sz);

/**
 * @brief query chip infomation.
 *        format as TLV, Length 1 byte, Tag as below:
 *        +--------------------------------+
 *        | Serial Number            | 02h |
 *        | Test date                | 06h |
 *        | Internal product version | 07h |
 *        | Module date              | 08h |
 *        +--------------------------+-----+
 *
 * @param cmd - out buffer
 *
 * @return length of the built cmd.
 */
extern UINT16 mjac_cmd_build_query(
        UINT8 *cmd, UINT16 cmd_sz);

/**
* @brief parse data from response.
*
* @param resp - in whole response, out parsed data.
* @param resp_sz - length of parsed data.
*
 * @return verify response code.
*/
extern mjac_resp_code_t mjac_resp_parse_data(
        UINT8 *resp, UINT16 *data_sz);

extern UINT16 mjac_cmd_build_update(
        UINT8 *cmd, UINT16 cmd_sz,
        mjac_zone_t idx, UINT16 offset, const UINT8 *data, UINT16 data_sz);


#endif
