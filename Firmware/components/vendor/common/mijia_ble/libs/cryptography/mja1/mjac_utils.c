#include "mjac_utils.h"
#include "third_party/mbedtls/asn1.h"

/* --------------------------------------------------------------------- crc */
/* CRC CCITT computed with POLY set to 0x1189. */
#if defined(CRC_CCCITT_IN_RAM)
static UINT16 crc_ccitt_16[256];
static void crc_ccitt_16_table_init(void);
static UINT8 crc_ccitt_16_table_init_done = 0;

#define POLY ((UINT16)0x1189)
static void crc_ccitt_16_table_init(void)
{
    UINT16 crc, i, j;

    for (i = 0; i < 256; ++i)
    {
        UINT16 c;
        crc = 0;
        c = i << 8;

        for (j = 0; j < 8; ++j)
        {
            if (((crc ^ c) & 0x8000) != 0)
            {
                crc = (UINT16)((crc << 1) ^ POLY);
            }
            else
            {
                crc <<= 1;
            }

            c <<= 1;
        }

        crc_ccitt_16[i] = crc;
    }
}

#else
/* Table is mirrored to use a right shift CRC generation to avoid needing to
 * mirror all the bit endianess of the incoming data*/
static const UINT16 crc_ccitt_16[256] =
{
    0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
    0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
    0x1081, 0x0108, 0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E,
    0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876,
    0x2102, 0x308B, 0x0210, 0x1399, 0x6726, 0x76AF, 0x4434, 0x55BD,
    0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5,
    0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E, 0x54B5, 0x453C,
    0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD, 0xC974,
    0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB,
    0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3,
    0x5285, 0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A,
    0xDECD, 0xCF44, 0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72,
    0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9,
    0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5, 0xA96A, 0xB8E3, 0x8A78, 0x9BF1,
    0x7387, 0x620E, 0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738,
    0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862, 0x9AF9, 0x8B70,
    0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E, 0xF0B7,
    0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
    0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036,
    0x18C1, 0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E,
    0xA50A, 0xB483, 0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5,
    0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD,
    0xB58B, 0xA402, 0x9699, 0x8710, 0xF3AF, 0xE226, 0xD0BD, 0xC134,
    0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C,
    0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1, 0xA33A, 0xB2B3,
    0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72, 0x3EFB,
    0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232,
    0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A,
    0xE70E, 0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1,
    0x6B46, 0x7ACF, 0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9,
    0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330,
    0x7BC7, 0x6A4E, 0x58D5, 0x495C, 0x3DE3, 0x2C6A, 0x1EF1, 0x0F78
};
#endif /* CRC_CCCITT_IN_RAM */

UINT16 crc16_ccitt(const UINT8 *data, UINT16 sz)
{
    UINT16 crc = 0xFFFF;
    UINT8 ndx;
    UINT16 i;

    if (data && sz)
    {
#if defined(CRC_CCCITT_IN_RAM)
        if (crc_ccitt_16_table_init_done == 0)
        {
            crc_ccitt_16_table_init();
            crc_ccitt_16_table_init_done = 1;
        }
#endif /* CRC_CCCITT_IN_RAM */

        for (i = 0; i < sz; i++)
        {
            ndx = 0xFF & (crc ^ data[i]);
            crc = (crc_ccitt_16[ndx] ^ (crc >> 8 ));
        }
    }

    return crc^0xFFFF;
}

/* --------------------------------------------------------------------- x509 */
UINT8 x509_crt_parse_der(const UINT8 *der, UINT16 der_sz, x509_crt_t *crt)
{
    UINT8 *p, *end, *tbs_data;
    size_t len;

    if (!der || !der_sz || !crt)
        return 0x01;

    p = (UINT8 *)der;
    end = p+der_sz;

    /*
     * Certificate  ::=  SEQUENCE  {
     *      tbsCertificate       TBSCertificate,
     *      signatureAlgorithm   AlgorithmIdentifier,
     *      signatureValue       BIT STRING  }
     */
    if(mbedtls_asn1_get_tag(&p, end, &len,
                MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))
        return 0x02;

    /* got raw cert raw */
    end = p+len;

    /* got tbs */
    crt->tbs.value = p;

    if(mbedtls_asn1_get_tag(&p, end, &len,
                MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))
        return 0x10;

    crt->tbs.length = p+len-crt->tbs.value;
    tbs_data = p;

    /* locate signature alg */
    p += len;
    if(mbedtls_asn1_get_tag(&p, end, &len,
                MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))
        return 0x20;

    /* locate signature value */
    p += len;
    if(mbedtls_asn1_get_bitstring_null(&p, end, &len))
        return 0x30;

    crt->sign.value = p;
    crt->sign.length = len;

    /*
     * TBSCertificate  ::=  SEQUENCE  {
     */
    p = tbs_data;
    end = crt->tbs.value+crt->tbs.length;

    /*
     *      Version  ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
     */
    if(mbedtls_asn1_get_tag(&p, end, &len,
            MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED))
        return 0x11;

    /*
     *      CertificateSerialNumber  ::=  INTEGER
     */
    p += len;
    if(mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_INTEGER))
        return 0x12;

    /* got sn */
    crt->sn.value = p;
    crt->sn.length = len;

    /*
     *      signature            AlgorithmIdentifier
     */
    p += len;
    if(mbedtls_asn1_get_tag(&p, end, &len,
                MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))
        return 0x13;

    /*
     *      issuer               Name
     */
    p += len;
    if(mbedtls_asn1_get_tag(&p, end, &len,
                MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))
        return 0x14;

    /*
     *      Validity ::= SEQUENCE {
     *              notBefore      Time,
     *              notAfter       Time }
     *
     */
    p += len;
    if(mbedtls_asn1_get_tag(&p, end, &len,
                MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))
        return 0x15;

    /*
     *      subject              Name
     */
    p += len;
    if(mbedtls_asn1_get_tag(&p, end, &len,
                MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))
        return 0x16;

    /*
     * SubjectPublicKeyInfo
     */
    p += len;
    if(mbedtls_asn1_get_tag(&p, end, &len,
                MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))
        return 0x17;

    /* Public Key Algorithm */
    if(mbedtls_asn1_get_tag(&p, p+len, &len,
                MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))
        return 0x18;

    /* Subject Public Key */
    p += len;
    if(mbedtls_asn1_get_bitstring_null(&p, end, &len))
        return 0x19;

    crt->pub.value = p;
    crt->pub.length = len;
    /*
     *  }
     *  -- end of TBSCertificate
     */

    return 0;
}

extern UINT16 x509_pubkey_to_mjac(
        const UINT8 *pub, UINT16 pub_sz,
        UINT8 *out, UINT16 out_sz)
{
    UINT16 length = 0;

    if(!pub || pub_sz != 65 || !out || out_sz < 69)
        return 0;

    out[length++] = pub[0];
    out[length++] = 0x00;
    out[length++] = 0x20;
    MEMCPY(&out[length], &pub[1], 32);
    length += 32;

    out[length++] = 0x00;
    out[length++] = 0x20;
    MEMCPY(&out[length], &pub[33], 32);
    length += 32;

    return length;
}

extern UINT16 x509_sign_to_mjac(
        const UINT8 *sign, UINT16 sign_sz,
        UINT8 *out, UINT16 out_sz)
{
    UINT16 length = 0;
    size_t len = 0;
    UINT8 *p = (UINT8*)sign, *end = p+sign_sz;

    if(!sign || !out || out_sz < 68)
        return 0;

    if(mbedtls_asn1_get_tag(&p, end, &len,
                MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))
        return 0;
    end = p+len;

    /* got signature */
    /* r */
    if(mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_INTEGER))
        return 0;

    out[length++] = 0x00;
    out[length++] = 0x20;
    MEMCPY(out+length, p+len-32, 32);
    length += 32;

    /* s */
    p += len;
    if(mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_INTEGER))
        return 0;
    out[length++] = 0x00;
    out[length++] = 0x20;
    MEMCPY(out+length, p+len-32, 32);
    length += 32;

    return length;
}
