#include "mi_config.h"

const mi_config_t m_config = {
        .pid               = PRODUCT_ID,
        .io                = OOB_IO_CAP,
        .have_msc          = HAVE_MSC,
        .have_reset_button = HAVE_RESET_BUTTON,
        .schd_in_mainloop  = MI_SCHD_PROCESS_IN_MAIN_LOOP,
};
