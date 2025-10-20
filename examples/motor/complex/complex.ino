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
    Serial.println("DRV test");
    drv.begin();

    // I2C trigger by sending 'go' command
    drv.setMode(DRV2605_MODE_INTTRIG); // default, internal trigger when sending GO command

    drv.selectLibrary(1);
    drv.setWaveform(0, 84); // ramp up medium 1, see datasheet part 11.2
    drv.setWaveform(1, 1);  // strong click 100%, see datasheet part 11.2
    drv.setWaveform(2, 0);  // end of waveforms
}

void loop()
{
    drv.go();
    delay(1000);
}
