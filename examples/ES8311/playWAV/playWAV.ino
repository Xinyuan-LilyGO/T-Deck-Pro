#include "Arduino.h"
#include "wav_hex.h"
#include "esp_codec.h"
#include "ExtensionIOXL9555.hpp"

// XL9555
#define BOARD_I2C_SDA 13
#define BOARD_I2C_SCL 14

#define BOARD_XL9555_00_6609_EN     (0)
#define BOARD_XL9555_10_PWEKEY_EN   (8)
// Connected to XL9555 IO06, enable power amplifier,
// increase the volume of the speaker
#define BOARD_XL9555_06_AMPLIFIER (6) // Connected to XL9555 IO06
/* Module A7682E and ES8311 share the output for headphones and speakers.
/  Select the audio output through AUDIO_SEL. When AUDIO_SEL is
/  HIGH : the headphones and speakers output the sound from A7682E.
/  LOW :  the headphones and speakers output the sound from ES8311.
*/
#define BOARD_XL9555_12_AUDIO_SEL (10) // Connected to XL9555 IO12

// ES8311
#define BOARD_I2C_ADDR_ES8311   0x18
#define BOARD_ES8311_SCL        BOARD_I2C_SCL
#define BOARD_ES8311_SDA        BOARD_I2C_SDA
#define BOARD_ES8311_MCLK       38
#define BOARD_ES8311_SCLK       39
#define BOARD_ES8311_ASDOUT     40
#define BOARD_ES8311_LRCK       18
#define BOARD_ES8311_DSDIN      17

EspCodec codec;
ExtensionIOXL9555 io;

void setup()
{
    Serial.begin(115200);

    // XL9555 Init
    if (!io.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, XL9555_SLAVE_ADDRESS0))
    {
        while (1)
        {
            Serial.println("Failed to find XL9555 - check your wiring!");
            delay(1000);
        }
    }
    io.configPort(ExtensionIOXL9555::PORT0, 0x00);   // Set PORT0 as output ,mask = 0x00 = all pin output
    io.configPort(ExtensionIOXL9555::PORT1, 0x00);   // Set PORT1 as output ,mask = 0x00 = all pin output
    io.digitalWrite(BOARD_XL9555_06_AMPLIFIER, HIGH); // Enable power amplifier
    io.digitalWrite(BOARD_XL9555_12_AUDIO_SEL, LOW); // Audio output selection: ES8311 module
    io.digitalWrite(BOARD_XL9555_00_6609_EN, HIGH);     // enable A7682 module power
    io.digitalWrite(BOARD_XL9555_10_PWEKEY_EN, HIGH);   // power on

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    codec.setPins(BOARD_ES8311_MCLK, BOARD_ES8311_SCLK, BOARD_ES8311_LRCK, BOARD_ES8311_ASDOUT, BOARD_ES8311_DSDIN);
    if (codec.begin(Wire, BOARD_I2C_ADDR_ES8311, CODEC_TYPE_ES8311)) {
        Serial.println("Codec init succeeded");
    } else {
        Serial.println("Warning: Failed to find Codec");
    }
    codec.setVolume(50); // range: [0, 100]
    codec.playWAV((uint8_t*)wav_hex, wav_hex_len);
}

void loop()
{
    delay(1000);
}
