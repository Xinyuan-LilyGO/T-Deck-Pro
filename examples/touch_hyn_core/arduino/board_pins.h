#pragma once

// Minimal board mapping for the T-Deck Pro touch demo.
// Touch reset is wired to XL9555 IO07.

#include <stdint.h>

#define BOARD_I2C_SDA 13
#define BOARD_I2C_SCL 14

#define BOARD_I2C_ADDR_XL9555 0x20
#define BOARD_XL9555_07_TOUCH_RST (7) // LOW: Reset touch

// XL9555 "virtual GPIO" encoding for drivers that expect a GPIO number.
#define XL9555_GPIO_BASE (0x100)
#define XL9555_GPIO(pin) (XL9555_GPIO_BASE + (pin))
#define XL9555_GPIO_IS(id) ((int)(id) >= XL9555_GPIO_BASE && (int)(id) < (XL9555_GPIO_BASE + 16))
#define XL9555_GPIO_TO_PIN(id) ((uint8_t)((id) - XL9555_GPIO_BASE))

#define BOARD_TOUCH_RST XL9555_GPIO(BOARD_XL9555_07_TOUCH_RST)
