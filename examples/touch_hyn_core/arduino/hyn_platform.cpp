#include "hyn_platform.h"

#include "Arduino.h"
#include "board_pins.h"

#include "ExtensionIOXL9555.hpp"

extern ExtensionIOXL9555 xl9555_io;

int hyn_platform_gpio_set_value(uint32_t gpio_id, int value)
{
    if (XL9555_GPIO_IS((int)gpio_id)) {
        const uint8_t pin = XL9555_GPIO_TO_PIN(gpio_id);
        xl9555_io.pinMode(pin, OUTPUT);
        xl9555_io.digitalWrite(pin, value ? HIGH : LOW);
        return 1;
    }
    return 0;
}

int hyn_platform_gpio_get_value(uint32_t gpio_id, int *out_value)
{
    if (!out_value) {
        return 0;
    }
    if (XL9555_GPIO_IS((int)gpio_id)) {
        const uint8_t pin = XL9555_GPIO_TO_PIN(gpio_id);
        *out_value = xl9555_io.digitalRead(pin) ? 1 : 0;
        return 1;
    }
    return 0;
}

