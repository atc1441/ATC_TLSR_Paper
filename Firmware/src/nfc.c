#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "drivers/8258/flash.h"
#include "nfc.h"
#include "main.h"

void init_nfc()
{
    gpio_set_func(NFC_SDA, AS_GPIO);
    gpio_set_output_en(NFC_SDA, 1);
    gpio_write(NFC_SDA, 1);
    gpio_set_func(NFC_SCL, AS_GPIO);
    gpio_set_output_en(NFC_SCL, 1);
    gpio_write(NFC_SCL, 1);
    gpio_set_func(NFC_CS, AS_GPIO);
    gpio_set_output_en(NFC_CS, 1);
    gpio_write(NFC_CS, 1);
    gpio_set_func(NFC_IRQ, AS_GPIO);
    gpio_set_input_en(NFC_IRQ, 1);
    gpio_set_output_en(NFC_IRQ, 0);
}