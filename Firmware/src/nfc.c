#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "drivers/8258/flash.h"
#include "nfc.h"
#include "main.h"

_attribute_ram_code_ void init_nfc()
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

_attribute_ram_code_ void deinit_nfc()
{
    // NFC_SDA
    gpio_set_func(NFC_SDA, AS_GPIO);
    gpio_set_output_en(NFC_SDA, 0);
    gpio_set_input_en(NFC_SDA, 1);
    gpio_setup_up_down_resistor(NFC_SDA, PM_PIN_PULLDOWN_100K);
    // NFC_SCL
    gpio_set_func(NFC_SCL, AS_GPIO);
    gpio_set_output_en(NFC_SCL, 0);
    gpio_set_input_en(NFC_SCL, 1);
    gpio_setup_up_down_resistor(NFC_SCL, PM_PIN_PULLDOWN_100K);
    // NFC_CS
    gpio_set_func(NFC_CS, AS_GPIO);
    gpio_set_output_en(NFC_CS, 0);
    gpio_set_input_en(NFC_CS, 1);
    gpio_setup_up_down_resistor(NFC_CS, PM_PIN_PULLUP_1M);
    // NFC_IRQ
    gpio_set_func(NFC_IRQ, AS_GPIO);
    gpio_set_output_en(NFC_IRQ, 0);
    gpio_set_input_en(NFC_IRQ, 1);
    gpio_setup_up_down_resistor(NFC_IRQ, PM_PIN_PULLDOWN_100K); // turn off pull-up resistor
}