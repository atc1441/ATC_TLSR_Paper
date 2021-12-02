//#include <string.h>

#include "mible_api.h"
#include "mible_log.h"
#include "cryptography/mi_crypto.h"
#include "mible_crypto.h"

#define SESSION_KEY_VALID_PERIOD_SEC   30

typedef struct { 
    uint32_t  iv;
    uint8_t   reserve[4];
    uint32_t  counter;
} session_nonce_t;

static struct {
    uint8_t initialized :1;
    uint8_t processing  :1;
    uint8_t expired     :1;
} m_flags;

static uint32_t  session_dev_cnt;
static uint32_t  session_app_cnt;
static session_ctx_t session_ctx;
static void * session_timer;

static int update_cnt(uint32_t* p_cnt, uint16_t cnt_low)
{
    uint16_t last_cnt_low = *p_cnt;
    
    if ((last_cnt_low > 0x7FFF) && (cnt_low < 0x8000))
        *p_cnt += 0x10000UL;
    
    *(uint16_t*)p_cnt = cnt_low;

    return 0;
}

static void session_timeout(void * arg)
{
//    MI_LOG_ERROR("Session key has expired. \n");
//    m_flags.expired = 1;
// TODO: mandatory update device counter.
}

int mi_session_init(session_ctx_t *p_ctx)
{
    if (p_ctx == NULL)
        return 1;

    session_ctx = *p_ctx;
    session_app_cnt = 0;
    session_dev_cnt = 0;

    m_flags.initialized = 1;
    m_flags.processing  = 0;
    m_flags.expired = 0;

    uint8_t errno = mible_timer_create(&session_timer, session_timeout, MIBLE_TIMER_SINGLE_SHOT);
    MI_ERR_CHECK(errno);
    mible_timer_start(session_timer, SESSION_KEY_VALID_PERIOD_SEC * 1000, NULL);

    return 0;
}

int mi_session_uninit(void)
{
    if (m_flags.initialized == 0)
        return 0;

    m_flags.initialized = 0;
    mible_timer_stop(session_timer);
    mible_timer_delete(session_timer);
    session_timer = NULL;
    return 0;
}

int mi_session_encrypt(const uint8_t *input, uint8_t ilen, uint8_t *output)
{
    int ret = 0;
    if (m_flags.initialized != 1)
        return 1;

    if (m_flags.processing == 1)
        return 2;

    if (m_flags.expired == 1)
        return 3;

    CRITICAL_SECTION_ENTER();
    m_flags.processing = 1;
    CRITICAL_SECTION_EXIT();

    uint8_t tmp[ilen];
    memcpy(tmp, input, ilen);
    
    session_nonce_t nonce = {0};
    nonce.iv = session_ctx.dev_iv;
    uint16_t cnt_low = (uint16_t)session_dev_cnt;
    update_cnt(&session_dev_cnt, ++cnt_low);

    nonce.counter = session_dev_cnt;
    output[0] = (uint8_t) session_dev_cnt;
    output[1] = (uint8_t)(session_dev_cnt >> 8);
    ret = mi_crypto_ccm_encrypt(session_ctx.dev_key,
                                (void*)&nonce, sizeof(nonce),
                                         NULL, 0,
                                          tmp, ilen,
                                     2+output,
                                2+output+ilen, 4);

    m_flags.processing = 0;

    mible_timer_stop(session_timer);
    mible_timer_start(session_timer, SESSION_KEY_VALID_PERIOD_SEC * 1000, NULL);

    return ret;
}

int mi_session_decrypt(const uint8_t *input, uint8_t ilen, uint8_t *output)
{
    uint32_t ret = 0;
    uint32_t curr_cnt = session_app_cnt;

    if (m_flags.initialized != 1)
        return 1;

    if (m_flags.processing == 1)
        return 2;

    if (m_flags.expired == 1)
        return 3;

    CRITICAL_SECTION_ENTER();
    m_flags.processing = 1;
    CRITICAL_SECTION_EXIT();

    session_nonce_t nonce = {0};
    nonce.iv = session_ctx.app_iv;
    uint16_t cnt_low = input[1]<<8 | input[0];
    update_cnt(&curr_cnt, cnt_low);

    if (curr_cnt <= session_app_cnt && curr_cnt != 0) {
        m_flags.processing = 0;
        return -1;
    } else {
        session_app_cnt = curr_cnt;
    }

    nonce.counter = session_app_cnt;
    ret = mi_crypto_ccm_decrypt(session_ctx.app_key,
                                (void*)&nonce, sizeof(nonce),
                                         NULL, 0,
                                      2+input, ilen-2-4,
                                       output,
                             2+input+ilen-2-4, 4);

    m_flags.processing = 0;

    if (ret == MI_SUCCESS) {
        mible_timer_stop(session_timer);
        mible_timer_start(session_timer, SESSION_KEY_VALID_PERIOD_SEC * 1000, NULL);
    }
    return ret;
}
