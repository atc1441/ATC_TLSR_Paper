#ifndef __MIBLE_CRYPTO_BACKEND_MSC_H__
#define __MIBLE_CRYPTO_BACKEND_MSC_H__
#include <stdint.h>
#include "mible_type.h"
#include "third_party/pt/pt.h"
#include "third_party/pt/pt_misc.h"

typedef enum {
    // Info
    MSC_INFO       = 0x01,
    MSC_ID,
    MSC_HIBERNATE,

    // Sign
    MSC_SIGN       = 0x10,
    MSC_VERIFY     = 0x11,

    MSC_ECDHE      = 0x14,

    MSC_DEV_CERT   = 0x20,
    MSC_MANU_CERT,
    MSC_ROOT_CERT,
    MSC_CERTS_LEN  = 0x28,
    MSC_ROOT_CERT_LEN,
    MSC_MANU_CERT_LEN,
    MSC_DEV_CERT_LEN,

    MSC_PUBKEY     = 0x3B,
    
    MSC_WR_MKPK    = 0x40,
    MSC_RD_MKPK,
    MSC_ERASE,
    
    MSC_RANDOM     = 0x50,
    MSC_STATUS     = 0x52,

    MSC_AESCCM_ENC = 0x60,
    MSC_AESCCM_DEC,

    MSC_NOP        = 0xFF
} msc_cmd_t;

typedef struct {
    uint8_t  vid;
    uint16_t hw_ver;
    uint16_t sw_ver;
    uint16_t protocol_ver;
    uint8_t  cfg;
    uint8_t  reserve[2];
    uint16_t sn[16];
    uint8_t  pad[2];
} msc_info_t;

int msc_power_on(void);
int msc_power_off(void);
int mi_msc_config(void * pwr_manage, const iic_config_t * p_iic_config, pstimer_t *p_timer);
int msc_get_chip_info(msc_info_t *p_info);
int msc_get_device_id(uint8_t did[8]);
int msc_get_cert_len(uint16_t *p_dev_len, uint16_t *p_manu_len, uint16_t *p_root_len);
int msc_get_cert_der(uint8_t cert_type, uint8_t *p_der, uint16_t der_len);
int msc_ecc_keypair_gen(const uint8_t *p_curve, uint8_t *p_pk);
int msc_ecdh(const uint8_t *p_curve, const uint8_t *p_le_pk, uint8_t *p_shared);
int msc_ecdsa_verify(const uint8_t *p_curve, const uint8_t *p_le_pk,
                   uint8_t const *p_hash, uint8_t const *p_sig);
int msc_ecdsa_sign(const uint8_t *p_curve, const uint8_t *p_hash, uint8_t *p_sig);
int msc_record_write(uint8_t record_id, const uint8_t *p_data, uint8_t len);
int msc_record_read(uint8_t record_id, uint8_t *p_data, uint8_t len);
int msc_record_delete(uint8_t record_id);
#endif

