//#include <string.h>
#include "mible_api.h"
#include "mible_port.h"

#include "third_party/pt/pt.h"
#include "third_party/pt/pt_misc.h"

#undef  MI_LOG_MODULE_NAME
#define MI_LOG_MODULE_NAME "mbed"
#include "mible_log.h"

#include "cryptography/mi_crypto_backend_mbedtls.h"

#include "third_party/mbedtls/asn1.h"
#include "third_party/mbedtls/sha256_hkdf.h"
#include "third_party/mbedtls/ccm.h"

#define CHECK(code) if( ( ret = code ) != 0 ){ return( ret ); }
#define CHECK_RANGE(min, max, val) if( val < min || val > max ){ return( ret ); }

static int num_strchr(const char *str, char c)
{
    const char *pindex = strchr(str, c);
    if (NULL == pindex){
        return -1;
    }
    return pindex - str;
}

static int x509_parse_int( unsigned char **p, size_t n, int *res )
{
    *res = 0;

    for( ; n > 0; --n )
    {
        if( ( **p < '0') || ( **p > '9' ) )
            return ( -1 );

        *res *= 10;
        *res += ( *(*p)++ - '0' );
    }

    return( 0 );
}

static int x509_date_is_valid(const msc_crt_time *t)
{
    int ret = -1;

    CHECK_RANGE( 0, 9999, t->year );
    CHECK_RANGE( 0, 23,   t->hour );
    CHECK_RANGE( 0, 59,   t->min  );
    CHECK_RANGE( 0, 59,   t->sec  );

    switch( t->mon )
    {
        case 1: case 3: case 5: case 7: case 8: case 10: case 12:
            CHECK_RANGE( 1, 31, t->day );
            break;
        case 4: case 6: case 9: case 11:
            CHECK_RANGE( 1, 30, t->day );
            break;
        case 2:
            CHECK_RANGE( 1, 28 + (t->year % 4 == 0), t->day );
            break;
        default:
            return( ret );
    }

    return( 0 );
}

/*
 * Parse an ASN1_UTC_TIME (yearlen=2) or ASN1_GENERALIZED_TIME (yearlen=4)
 * field.
 */
static int x509_parse_time( unsigned char **p, size_t len, size_t yearlen,
                            msc_crt_time *tm )
{
    int ret;

    /*
     * Minimum length is 10 or 12 depending on yearlen
     */
    if ( len < yearlen + 8 )
        return ( -1 );
    len -= yearlen + 8;

    /*
     * Parse year, month, day, hour, minute
     */
    CHECK( x509_parse_int( p, yearlen, &tm->year ) );
    if ( 2 == yearlen )
    {
        if ( tm->year < 50 )
            tm->year += 100;

        tm->year += 1900;
    }

    CHECK( x509_parse_int( p, 2, &tm->mon ) );
    CHECK( x509_parse_int( p, 2, &tm->day ) );
    CHECK( x509_parse_int( p, 2, &tm->hour ) );
    CHECK( x509_parse_int( p, 2, &tm->min ) );

    /*
     * Parse seconds if present
     */
    if ( len >= 2 )
    {
        CHECK( x509_parse_int( p, 2, &tm->sec ) );
        len -= 2;
    }
    else
        return ( -2 );

    /*
     * Parse trailing 'Z' if present
     */
    if ( 1 == len && 'Z' == **p )
    {
        (*p)++;
        len--;
    }

    /*
     * We should have parsed all characters at this point
     */
    if ( 0 != len )
        return ( -3 );

    CHECK( x509_date_is_valid( tm ) );

    return ( 0 );
}

/*
 *  Time ::= CHOICE {
 *       utcTime        UTCTime,
 *       generalTime    GeneralizedTime }
 */
static int x509_get_time( unsigned char **p, const unsigned char *end,
                           msc_crt_time *tm )
{
    int ret;
    size_t len, year_len;
    unsigned char tag;

    if( ( end - *p ) < 1 )
        return -1;

    tag = **p;

    if( tag == MBEDTLS_ASN1_UTC_TIME )
        year_len = 2;
    else if( tag == MBEDTLS_ASN1_GENERALIZED_TIME )
        year_len = 4;
    else
        return -2;

    (*p)++;
    ret = mbedtls_asn1_get_len( p, end, &len );

    if( ret != 0 )
        return( -3 );

    return x509_parse_time( p, len, year_len, tm );
}

static int x509_get_notafter( unsigned char *start, const unsigned char *end,
                           msc_crt_time *tm )
{
    unsigned char *p_tmp = start;
    unsigned char **p = &p_tmp;
    
    return (x509_get_time( p, end, tm )|x509_get_time( p, end, tm ));
}

static int x509_check_time( const msc_crt_time *before, const msc_crt_time *after )
{
    if( before->year  > after->year )
        return( 1 );

    if( before->year == after->year &&
        before->mon   > after->mon )
        return( 1 );

    if( before->year == after->year &&
        before->mon  == after->mon  &&
        before->day   > after->day )
        return( 1 );

    if( before->year == after->year &&
        before->mon  == after->mon  &&
        before->day  == after->day  &&
        before->hour  > after->hour )
        return( 1 );

    if( before->year == after->year &&
        before->mon  == after->mon  &&
        before->day  == after->day  &&
        before->hour == after->hour &&
        before->min   > after->min  )
        return( 1 );

    if( before->year == after->year &&
        before->mon  == after->mon  &&
        before->day  == after->day  &&
        before->hour == after->hour &&
        before->min  == after->min  &&
        before->sec   > after->sec  )
        return( 1 );

    return( 0 );
}
/*
 *  Version  ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
 */
static int x509_get_version( unsigned char **p,
                             const unsigned char *end,
                             int *ver )
{
    int ret;
    size_t len;

    if( ( ret = mbedtls_asn1_get_tag( p, end, &len,
            MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0 ) ) != 0 )
    {
        if( ret == MBEDTLS_ERR_ASN1_UNEXPECTED_TAG )
        {
            *ver = 0;
            return( 0 );
        }

        return( ret );
    }

    end = *p + len;

    if( ( ret = mbedtls_asn1_get_int( p, end, ver ) ) != 0 )
        return( ret );

    if( *p != end )
        return( -1 );

    return( 0 );
}
/*
 *  CertificateSerialNumber  ::=  INTEGER
 */
static int x509_get_serial( unsigned char **p, const unsigned char *end,
                     mbedtls_asn1_buf *serial )
{
    int ret;

    if( ( end - *p ) < 1 )
        return -1;

    if( **p != ( MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_PRIMITIVE | 2 ) &&
        **p !=   MBEDTLS_ASN1_INTEGER )
        return -2;

    serial->tag = *(*p)++;

    ret = mbedtls_asn1_get_len( p, end, &serial->len );
    if( ret != 0 )
        return -3;

    serial->p = *p;
    *p += serial->len;

    return( 0 );
}

/*
 *  AttributeTypeAndValue ::= SEQUENCE {
 *    type     AttributeType,
 *    value    AttributeValue }
 *
 *  AttributeType ::= OBJECT IDENTIFIER
 *
 *  AttributeValue ::= ANY DEFINED BY AttributeType
 */
static int x509_get_attr_type_value( unsigned char **p,
                                     const unsigned char *end,
                                     mbedtls_asn1_named_data *cur )
{
    int ret;
    size_t len;
    mbedtls_asn1_buf *oid;
    mbedtls_asn1_buf *val;

    if( ( ret = mbedtls_asn1_get_tag( p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
        return( -91 + ret );

    if( ( end - *p ) < 1 )
        return( -91 +
                MBEDTLS_ERR_ASN1_OUT_OF_DATA );

    oid = &cur->oid;
    oid->tag = **p;

    if( ( ret = mbedtls_asn1_get_tag( p, end, &oid->len, MBEDTLS_ASN1_OID ) ) != 0 )
        return( -91 + ret );

    oid->p = *p;
    *p += oid->len;

    if( ( end - *p ) < 1 )
        return( -91 +
                MBEDTLS_ERR_ASN1_OUT_OF_DATA );

    if( **p != MBEDTLS_ASN1_BMP_STRING && **p != MBEDTLS_ASN1_UTF8_STRING      &&
        **p != MBEDTLS_ASN1_T61_STRING && **p != MBEDTLS_ASN1_PRINTABLE_STRING &&
        **p != MBEDTLS_ASN1_IA5_STRING && **p != MBEDTLS_ASN1_UNIVERSAL_STRING &&
        **p != MBEDTLS_ASN1_BIT_STRING )
        return( -91 +
                MBEDTLS_ERR_ASN1_UNEXPECTED_TAG );

    val = &cur->val;
    val->tag = *(*p)++;

    if( ( ret = mbedtls_asn1_get_len( p, end, &val->len ) ) != 0 )
        return( -91 + ret );

    val->p = *p;
    *p += val->len;

    cur->next = NULL;

    return( 0 );
}
static int x509_get_name( unsigned char **p, const unsigned char *end,
                   uint8_t id, mbedtls_asn1_buf *val )
{
    int ret;
    size_t set_len;
    const unsigned char *end_set;

    mbedtls_asn1_named_data cur_buf;
    mbedtls_asn1_named_data *cur = &cur_buf;
    while( 1 )
    {
        /*
         * parse SET
         */
        if( ( ret = mbedtls_asn1_get_tag( p, end, &set_len,
                MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET ) ) != 0 )
            return( -91 + ret );

        end_set  = *p + set_len;

        while( 1 )
        {
            if( ( ret = x509_get_attr_type_value( p, end_set, cur ) ) != 0 )
                return( ret );

            if (cur->oid.p[cur->oid.len-1] == id) { /* found the specified oid */
                memcpy(val, &cur->val, sizeof(mbedtls_asn1_buf));
            }

            if( *p == end_set )
                break;
        }

        /*
         * continue until end of SEQUENCE is reached
         */
        if( *p == end )
            return( 0 );
    }
}

int mbedtls_crt_parse_der(const unsigned char *crt, size_t crt_sz,
        const msc_crt_time *now, msc_crt_t *msc_crt)
{
    int ret = 0;
    unsigned char *p, *end, *p_tmp, *crt_end;
    size_t len;

    if (!crt || !crt_sz) {
        return -1;
    }

    p = (unsigned char *)crt;
    end = p + crt_sz;

    /*
     * Certificate  ::=  SEQUENCE  {
     *      tbsCertificate       TBSCertificate,
     *      signatureAlgorithm   AlgorithmIdentifier,
     *      signatureValue       BIT STRING  }
     */
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 
            || len > (size_t)(end - p))
    {
        ret = -2;
        goto done;
    }

    /* got cert raw and save start and end */
    p_tmp = p;
    crt_end = p+len;

    /*
     * TBSCertificate  ::=  SEQUENCE  {
     */
    end = crt_end;
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        ret = -3;
        goto done;
    }

    /* got tbs */
    msc_crt->tbs.p = p_tmp;
    msc_crt->tbs.len = p + len - p_tmp;

    /* save tbs end since when don't parse whole tbs now */
    p_tmp = p + len;
    end = p_tmp;
    /*
     * Version  ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
     */
    int ver;
    if( ( ret = x509_get_version( &p, end, &ver ) ) != 0
            || (ver < 0 || ver > 2)
            )
    {
            ret = -4;
            goto done;
    }

    /*
     * CertificateSerialNumber  ::=  INTEGER
     */
    mbedtls_asn1_buf sn_buf;
    if( (ret = x509_get_serial( &p, end, &sn_buf ) ) != 0)
    {
        ret = -5;
        goto done;
    }
    /* got sn */
    msc_crt->sn.p = sn_buf.p;
    msc_crt->sn.len = sn_buf.len;

    /*
     * signature            AlgorithmIdentifier
     */
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        ret = -6;
        goto done;
    }

    p += len;
    /*
     * issuer               Name
     */
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        ret = -7;
        goto done;
    }

    p += len;
    /*
     * Validity ::= SEQUENCE {
     *      notBefore      Time,
     *      notAfter       Time }
     *
     */
    {
        msc_crt_time time;
        if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
                MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 ||
                x509_get_notafter( p, p+len, &time ))
        {
            ret = -8;
            goto done;
        }

        if (now != NULL && x509_check_time(now, &time))
        {
            ret = -81;
            goto done;
        }
    }
    p += len;
    /*
     * subject              Name
     */
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        ret = -9;
        goto done;
    }

    {
        mbedtls_asn1_buf sub_o = {0};
        if( len && ( ret = x509_get_name( &p, p + len, 0x0A, &sub_o ) ) != 0 )
        {
            return( -91 );
        }
        /* got sub_o */
        msc_crt->sub_o.p = sub_o.p;
        msc_crt->sub_o.len = sub_o.len;
    }

    /*
     * SubjectPublicKeyInfo
     */
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        ret = -10;
        goto done;
    }

    /* Public Key Algorithm */
    if( ( ret = mbedtls_asn1_get_tag( &p, p+len, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        ret = -11;
        goto done;
    }

    p += len;
    /* Subject Public Key */
    if( ( ret = mbedtls_asn1_get_bitstring_null( &p, end, &len ) ) != 0 )
    {
        ret = -12;
        goto done;
    }

    msc_crt->pub.p = p+1;
    msc_crt->pub.len = len - 1;

    /*
     *  }
     *  -- end of TBSCertificate
     *
     *  signatureAlgorithm   AlgorithmIdentifier,
     *  signatureValue       BIT STRING
     */
    p = p_tmp;
    end = crt_end;
    /* parse signatureAlgorithm */
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        ret = -13;
        goto done;
    }

    /* parse signatureValue */
    p += len;
    if( ( ret = mbedtls_asn1_get_bitstring_null( &p, end, &len ) ) != 0 )
    {
        ret = -14;
        goto done;
    }

    end = p+len;
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        ret = -15;
        goto done;
    }

    end = p+len;
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_INTEGER ) ) != 0 )
    {
        ret = -16;
        goto done;
    }

    /* got first bignum which <= 32 bytes */
    memset(msc_crt->sig, 0, MSC_CRT_SIG_SZ);
    /* when the first byte > 0x7f, the ASN.1 will prepend 0x00, so remove it */
    if (len >= 0x20)
        memcpy(msc_crt->sig, p+len-0x20, 0x20);
    else /* if the bignum less than 32 bytes, prepend 0x00(the bignum is big endian) */
        memcpy(msc_crt->sig+0x20-len, p, len);

    p += len;
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_INTEGER ) ) != 0 )
    {
        ret = -17;
        goto done;
    }

    if (len >= 0x20)
        memcpy(&msc_crt->sig[0x20], p+len-0x20, 0x20);
    else
        memcpy(&msc_crt->sig[0x40-len], p, len);

done:
    return ret;
}

static size_t base64_decode(const unsigned char * base64, size_t size,
                                unsigned char * buf, size_t buf_sz)
{
    int i = 0, j = 0, trans[4] = {0};
    const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    while (i < size && j < buf_sz) {
        if (' ' == base64[i] || '\r' == base64[i] || '\n' == base64[i]) {
            ++i;
            continue;
        }
        trans[0] = num_strchr(base64char, base64[i]);
        trans[1] = num_strchr(base64char, base64[i + 1]);
        buf[j++] = ((trans[0] << 2) & 0xfc) | ((trans[1] >> 4) & 0x03);
        if ('=' == base64[i + 2]) {
            continue;
        } else {
            trans[2] = num_strchr(base64char, base64[i + 2]);
        }
        buf[j++] = ((trans[1] << 4) & 0xf0) | ((trans[2] >> 2) & 0x0f);
        if ('=' == base64[i + 3]) {
            continue;
        } else {
            trans[3] = num_strchr(base64char, base64[i + 3]);
        }
        buf[j++] = ((trans[2] << 6) & 0xc0) | (trans[3] & 0x3f);
        i += 4;
    }

    return j;
}

int mbedtls_crt_pem2der(const unsigned char *pem, size_t pem_sz,
                                  unsigned char *der_buf, size_t buf_sz)
{
    const unsigned char *s1, *s2;
    size_t der_sz = 0;

    if (!pem || !der_buf) {
        goto done;
    }

    s1 = (unsigned char *) strstr( (const char *) pem, PEM_HEADER );
    if (s1 == NULL) {
        goto done;
    }

    s2 = (unsigned char *) strstr( (const char *) pem, PEM_FOOTER );
    if( s2 == NULL || s2 <= s1 || s2 - s1 > pem_sz ) {
        goto done;
    }

    s1 += strlen(PEM_HEADER);
    der_sz = base64_decode( s1, s2 - s1, der_buf, buf_sz );

done:
    return der_sz;
}

int mbedtls_read_signature(const unsigned char *sign_der, uint16_t sign_sz,
                                      uint8_t *out_buf, uint16_t out_sz)
{
    int ret = 0;
    unsigned char *p, *end;
    size_t len;

    if (!sign_der || !out_buf || out_sz < MSC_CRT_SIG_SZ) {
        return -1;
    }

    p = (unsigned char *)sign_der;
    end = p + sign_sz;

    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        ret = -15;
        goto done;
    }

    end = p+len;
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_INTEGER ) ) != 0 )
    {
        ret = -16;
        goto done;
    }

    /* got first bignum which <= 32 bytes */
    memset(out_buf, 0, MSC_CRT_SIG_SZ);
    /* when the first byte > 0x7f, the ASN.1 will prepend 0x00, so remove it */
    if (len >= 0x20) 
        memcpy(out_buf, p+len-0x20, 0x20);
    else /* if the bignum less than 32 bytes, prepend 0x00(the bignum is big endian) */
        memcpy(out_buf+0x20-len, p, len);

    p += len;
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_INTEGER ) ) != 0 )
    {
        ret = -17;
        goto done;
    }

    if (len >= 0x20)
        memcpy(&out_buf[0x20], p+len-0x20, 0x20);
    else
        memcpy(&out_buf[0x40-len], p, len);

done:
    return ret;
}

