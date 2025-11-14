

#include "Arduino.h"
#include "ExtensionIOXL9555.hpp"

// XL9555
#define BOARD_I2C_SDA  13
#define BOARD_I2C_SCL  14
#define BOARD_XL9555_00_6609_EN     (0)     // Connected to XL9555 IO00, enable A7682 module
#define BOARD_XL9555_06_AMPLIFIER    (6)     // Connected to XL9555 IO06, enable power amplifier, increase the volume of the speaker
#define BOARD_XL9555_10_PWEKEY_EN   (8)     // Connected to XL9555 IO10, power on/off ctrl

/* Module A7682E and ES8311 share the output for headphones and speakers.
/  Select the audio output through AUDIO_SEL. When AUDIO_SEL is
/  HIGH : the headphones and speakers output the sound from A7682E.
/  LOW :  the headphones and speakers output the sound from ES8311.
*/
#define BOARD_XL9555_12_AUDIO_SEL   (10)    // Connected to XL9555 IO12

// A7682E Modem
#define SerialMon Serial
#define SerialAT Serial1

// A7682E Pin
#define BOARD_A7682E_RI 7
#define BOARD_A7682E_DTR 8
#define BOARD_A7682E_RST 9
#define BOARD_A7682E_RXD 10
#define BOARD_A7682E_TXD 11

ExtensionIOXL9555 io;
bool reply = false;

void setup(void)
{
    
    SerialMon.begin(115200);
    SerialAT.begin(115200, SERIAL_8N1, BOARD_A7682E_TXD, BOARD_A7682E_RXD);

    // XL9555 Init
    if (!io.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, XL9555_SLAVE_ADDRESS0)) {
        while (1) {
            Serial.println("Failed to find XL9555 - check your wiring!");
            delay(1000);
        }
    }
    io.configPort(ExtensionIOXL9555::PORT0, 0x00);      // Set PORT0 as output ,mask = 0x00 = all pin output
    io.configPort(ExtensionIOXL9555::PORT1, 0x00);      // Set PORT1 as output ,mask = 0x00 = all pin output
    io.digitalWrite(BOARD_XL9555_00_6609_EN, HIGH);     // enable A7682 module power
    io.digitalWrite(BOARD_XL9555_10_PWEKEY_EN, HIGH);   // power on
    io.digitalWrite(BOARD_XL9555_06_AMPLIFIER, HIGH);    // Enable power amplifier
    io.digitalWrite(BOARD_XL9555_12_AUDIO_SEL, HIGH);   // Audio output selection: A7682E module
    
    // A7682  Power on
    Serial.println("Power on the modem PWRKEY.");
    // A7682E_power_on();

    int i = 10;
    Serial.println("\nTesting Modem Response...\n");
    Serial.println("****");
    while (i)
    {
        SerialAT.println("AT");
        delay(500);
        if (SerialAT.available())
        {
            String r = SerialAT.readString();
            SerialAT.println(r);
            if (r.indexOf("OK") >= 0)
            {
                reply = true;
                break;
            }
        }
        delay(500);
        i--;
    }
    Serial.println("****\n");

    if (reply)
    {
        Serial.println(F("***********************************************************"));
        Serial.println(F(" You can now send AT commands"));
        Serial.println(F(" Enter \"AT\" (without quotes), and you should see \"OK\""));
        Serial.println(F(" If it doesn't work, select \"Both NL & CR\" in Serial Monitor"));
        Serial.println(F(" DISCLAIMER: Entering AT commands without knowing what they do"));
        Serial.println(F(" can have undesired consiquinces..."));
        Serial.println(F("***********************************************************\n"));
    }
    else
    {
        Serial.println(F("***********************************************************"));
        Serial.println(F(" Failed to connect to the modem! Check the baud and try again."));
        Serial.println(F("***********************************************************\n"));
    }
}

void loop(void)
{
    while (SerialAT.available())
    {
        SerialMon.write(SerialAT.read());
    }
    while (SerialMon.available())
    {
        SerialAT.write(SerialMon.read());
    }
}