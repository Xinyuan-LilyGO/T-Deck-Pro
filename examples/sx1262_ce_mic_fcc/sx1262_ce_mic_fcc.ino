#include <RadioLib.h>

#define BOARD_LORA_EN 46
#define BOARD_LORA_CS 3
#define BOARD_LORA_BUSY 6
#define BOARD_LORA_RST 4
#define BOARD_LORA_INT 5
#define BOARD_SPI_SCK 36
#define BOARD_SPI_MOSI 33
#define BOARD_SPI_MISO 47

// #define Certification_MIC
#define Certification_CE
// #define Certification_FCC

// MIC
#if defined(Certification_MIC)
#define RADIO_TX_POWER 5.0
#define RADIO_BANDWIDTH 125.0
#define RADIO_FREQ 923.0
#define TX_INTERVAL 500
// CE
#elif defined(Certification_CE)
#define RADIO_TX_POWER 22.0
#define RADIO_BANDWIDTH 125.0
#define RADIO_FREQ 915.0
#define TX_INTERVAL 500
// FCC
#elif defined(Certification_FCC)
#define RADIO_TX_POWER 7.0
#define RADIO_BANDWIDTH 500.0
#define RADIO_FREQ 915.0
#define TX_INTERVAL 500
#endif

SX1262 radio = new Module(BOARD_LORA_CS, BOARD_LORA_INT, BOARD_LORA_RST, BOARD_LORA_BUSY);

// transmit
int transmissionState = RADIOLIB_ERR_NONE;
volatile bool transmittedFlag = true;

bool enableInterrupt = true;
int count = 0;
uint32_t runningMillis = 0;

void set_transmit_flag(void)
{
    // check if the interrupt is enabled
    if (!enableInterrupt)
    {
        return;
    }
    // we got a packet, set the flag
    transmittedFlag = true;
}

void setup()
{
    Serial.begin(115200);
    // while(!Serial) { delay (10); }

    //
    pinMode(BOARD_LORA_EN, OUTPUT);
    digitalWrite(BOARD_LORA_EN, HIGH);

    SPI.end();
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI, BOARD_LORA_CS);

    Serial.printf("[SX1262] %.1fM Initializing ... ", RADIO_FREQ);
    int state = radio.begin(RADIO_FREQ);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("success!"));
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true)
            ;
    }

    if (radio.setFrequency(RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY)
    {
        Serial.println(F("Selected frequency is invalid for this module!"));
        while (true)
            ;
    }

    // set bandwidth to 250 kHz
    if (radio.setBandwidth(RADIO_BANDWIDTH) == RADIOLIB_ERR_INVALID_BANDWIDTH)
    {
        Serial.println(F("Selected bandwidth is invalid for this module!"));
        while (true)
            ;
    }

    // set spreading factor to 10
    if (radio.setSpreadingFactor(10) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR)
    {
        Serial.println(F("Selected spreading factor is invalid for this module!"));
        while (true)
            ;
    }

    // set coding rate to 6
    if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE)
    {
        Serial.println(F("Selected coding rate is invalid for this module!"));
        while (true)
            ;
    }

    // set LoRa sync word to 0xAB
    if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE)
    {
        Serial.println(F("Unable to set sync word!"));
        while (true)
            ;
    }

    // set output power to 10 dBm (accepted range is -17 - 22 dBm)
    if (radio.setOutputPower(RADIO_TX_POWER) == RADIOLIB_ERR_INVALID_OUTPUT_POWER)
    {
        Serial.println(F("Selected output power is invalid for this module!"));
        while (true)
            ;
    }

    // set over current protection limit to 80 mA (accepted range is 45 - 240 mA)
    // NOTE: set value to 0 to disable overcurrent protection
    if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT)
    {
        Serial.println(F("Selected current limit is invalid for this module!"));
        while (true)
            ;
    }

    // set LoRa preamble length to 15 symbols (accepted range is 0 - 65535)
    if (radio.setPreambleLength(15) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH)
    {
        Serial.println(F("Selected preamble length is invalid for this module!"));
        while (true)
            ;
    }

    // disable CRC
    if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION)
    {
        Serial.println(F("Selected CRC is invalid for this module!"));
        while (true)
            ;
    }

    // Some SX126x modules have TCXO (temperature compensated crystal
    // oscillator). To configure TCXO reference voltage,
    // the following method can be used.
    if (radio.setTCXO(2.4) == RADIOLIB_ERR_INVALID_TCXO_VOLTAGE)
    {
        Serial.println(F("Selected TCXO voltage is invalid for this module!"));
        while (true)
            ;
    }

    // Some SX126x modules use DIO2 as RF switch. To enable
    // this feature, the following method can be used.
    // NOTE: As long as DIO2 is configured to control RF switch,
    //       it can't be used as interrupt pin!
    if (radio.setDio2AsRfSwitch() != RADIOLIB_ERR_NONE)
    {
        Serial.println(F("Failed to set DIO2 as RF switch!"));
        while (true)
            ;
    }

    Serial.println(F("All settings succesfully changed!"));

    radio.setDio1Action(set_transmit_flag);
    // transmissionState = radio.startTransmit("Hello World!");
}

char buf[256];

void loop()
{
    if (millis() - runningMillis > TX_INTERVAL)
    {
        // check if the previous transmission finished
        if (transmittedFlag)
        {
            // processing the data
            enableInterrupt = false;
            // reset flag
            transmittedFlag = false;

            if (transmissionState == RADIOLIB_ERR_NONE)
            {
                // packet was successfully sent
                Serial.println(F("transmission finished!"));

                // NOTE: when using interrupt-driven transmit method,
                //       it is not possible to automatically measure
                //       transmission data rate using getDataRate()
            }
            else
            {
                Serial.print(F("failed, code "));
                Serial.println(transmissionState);
            }
            // clean up after transmission is finished
            // this will ensure transmitter is disabled,
            // RF switch is powered down etc.
            // radio.finishTransmit();

            // send another one
            snprintf(buf, 256, "[ %u ]TX %u finished\n", millis() / 1000, count);

            Serial.println(buf);

            // you can transmit C-string or Arduino string up to
            // 256 characters long
            transmissionState = radio.startTransmit("Hello");

            // we're ready to send more packets,
            // enable interrupt service routine
            enableInterrupt = true;
        }
        runningMillis = millis();
    }
}
