#include <stdint.h>
#include "tl_common.h"
#include "main.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "vendor/common/blt_common.h"

#include "battery.h"
#include "ble.h"
#include "flash.h"

void user_init_normal(void);
void user_init_deepRetn(void);
void set_time(uint32_t time_now);
void main_loop(void);