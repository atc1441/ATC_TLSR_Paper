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
_attribute_ram_code_ void user_init_deepRetn(void);
void deinit_gpio();
void main_loop();