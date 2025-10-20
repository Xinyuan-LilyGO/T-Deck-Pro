#include <Wire.h>
#include "Adafruit_DRV2605.h"
#include "ExtensionIOXL9555.hpp"

// XL9555
#define BOARD_I2C_SDA 13
#define BOARD_I2C_SCL 14
#define BOARD_XL9555_05_MOTOR_EN (5) // Connected to XL9555 IO05

Adafruit_DRV2605 drv;
ExtensionIOXL9555 io;

void setup()
{
    Serial.begin(115200);

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);

    // XL9555 Init
    if (!io.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, XL9555_SLAVE_ADDRESS0)) {
        while (1) {
            Serial.println("Failed to find XL9555 - check your wiring!");
            delay(1000);
        }
    }
    io.configPort(ExtensionIOXL9555::PORT0, 0x00);   // Set PORT0 as output ,mask = 0x00 = all pin output
    io.configPort(ExtensionIOXL9555::PORT1, 0x00);   // Set PORT1 as output ,mask = 0x00 = all pin output
    io.digitalWrite(BOARD_XL9555_05_MOTOR_EN, HIGH); // Enable LoRa power

    // DRV2605 Init
    Serial.println("DRV2605 Audio responsive test");
    drv.begin();

    drv.setMode(DRV2605_MODE_AUDIOVIBE);

    // ac coupled input, puts in 0.9V bias
    drv.writeRegister8(DRV2605_REG_CONTROL1, 0x20);

    // analog input
    drv.writeRegister8(DRV2605_REG_CONTROL3, 0xA3);
}

void loop()
{
}
