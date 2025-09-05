

#include <RadioLib.h>
#include "utilities.h"
#include "peripheral.h"
#include "ui_deckpro_port.h"

static SX1262 radio = new Module(BOARD_LORA_CS, BOARD_LORA_INT, BOARD_LORA_RST, BOARD_LORA_BUSY);
static int lora_mode = LORA_MODE_SEND;
static String lora_recv_data;
static bool lora_recv_success = false;
static int lora_recv_rssi = 0;

// transmit 
static int transmissionState = RADIOLIB_ERR_NONE;
static volatile bool transmittedFlag = false;

static void set_transmit_flag(void){
    transmittedFlag = true;
}

// receive
static int receivedState = RADIOLIB_ERR_NONE;
static volatile bool receivedFlag = false;

static void set_receive_flag(void){
    receivedFlag = true;
}

bool lora_init(void)
{
    Serial.print(F("[SX1262] Initializing ... "));
    int state = radio.begin(ui_lora_get_freq());
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        return false;
    }

    radio.setPacketSentAction(set_transmit_flag);

    // set carrier frequency to 433.5 MHz
    if (radio.setFrequency(ui_lora_get_freq()) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));
        while (true);
    }

    // set bandwidth to 250 kHz
    if (radio.setBandwidth(ui_lora_get_bandwidth()) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("Selected bandwidth is invalid for this module!"));
        while (true);
    }

    // set spreading factor to 10
    if (radio.setSpreadingFactor(10) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println(F("Selected spreading factor is invalid for this module!"));
        while (true);
    }

    // set coding rate to 6
    if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Selected coding rate is invalid for this module!"));
        while (true);
    }

    // set LoRa sync word to 0xAB
    if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
        Serial.println(F("Unable to set sync word!"));
        while (true);
    }

    // set output power to 10 dBm (accepted range is -17 - 22 dBm)
    if (radio.setOutputPower(ui_lora_get_power()) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));
        while (true);
    }

    // set over current protection limit to 80 mA (accepted range is 45 - 240 mA)
    // NOTE: set value to 0 to disable overcurrent protection
    if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        Serial.println(F("Selected current limit is invalid for this module!"));
        while (true);
    }

    // set LoRa preamble length to 15 symbols (accepted range is 0 - 65535)
    if (radio.setPreambleLength(15) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
        Serial.println(F("Selected preamble length is invalid for this module!"));
        while (true);
    }

    // disable CRC
    if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Serial.println(F("Selected CRC is invalid for this module!"));
        while (true);
    }

    // Some SX126x modules have TCXO (temperature compensated crystal
    // oscillator). To configure TCXO reference voltage,
    // the following method can be used.
    if (radio.setTCXO(2.4) == RADIOLIB_ERR_INVALID_TCXO_VOLTAGE) {
        Serial.println(F("Selected TCXO voltage is invalid for this module!"));
        while (true);
    }

    // Some SX126x modules use DIO2 as RF switch. To enable
    // this feature, the following method can be used.
    // NOTE: As long as DIO2 is configured to control RF switch,
    //       it can't be used as interrupt pin!
    if (radio.setDio2AsRfSwitch() != RADIOLIB_ERR_NONE) {
        Serial.println(F("Failed to set DIO2 as RF switch!"));
        while (true);
    }

    Serial.println(F("All settings succesfully changed!"));

    Serial.println(F("[SX1262] Sending first packet ... "));
    transmissionState = radio.startTransmit("Hello World!");
    // radio.sleep();

    return true;
}

void lora_set_mode(int mode) 
{
    if(mode == LORA_MODE_SEND){
        radio.setPacketSentAction(set_transmit_flag);
        Serial.println(F("[LORA] Sending first packet ... "));
        transmissionState = radio.startTransmit("Hello World!");
    } else if(mode == LORA_MODE_RECV){
        radio.setPacketReceivedAction(set_receive_flag);
        Serial.println(F("[LORA] Starting to listen ... "));
        receivedState = radio.startReceive();
        if (receivedState == RADIOLIB_ERR_NONE) {
            Serial.println(F("success!"));
        } else {
            Serial.print(F("failed, code "));
            Serial.println(receivedState);
        }
    }
    lora_mode = mode;
}

int lora_get_mode(void)
{
    return lora_mode;
}

void lora_receive_loop(void)
{
    if(receivedFlag){
        receivedFlag = false;

        lora_recv_success = true;

        // String str;
        receivedState = radio.readData(lora_recv_data);
        if(receivedState == RADIOLIB_ERR_NONE){
            Serial.println(F("[SX1262] Received packet!"));

            Serial.print(F("[SX1262] Data:\t\t"));
            Serial.println(lora_recv_data.c_str());
            // Serial.println("%d", (int)lora_recv_data.toInt());

            Serial.print(F("[SX1262] RSSI:\t\t"));
            Serial.print(radio.getRSSI());
            lora_recv_rssi = (int) radio.getRSSI();
            Serial.println(F(" dBm"));
        }else{
            Serial.print(F("failed, code "));
            Serial.println(receivedState);
        }
    }
}

void lora_transmit(const char *str)
{
    if(transmittedFlag){
        transmittedFlag = false;
        if(transmissionState == RADIOLIB_ERR_NONE){
            Serial.println(F("transmission finished!"));
        } else {
            Serial.print(F("failed, code "));
            Serial.println(transmissionState);
        }

        radio.finishTransmit();
        Serial.print(F("[Lora] Sending another packet ... "));
        transmissionState = radio.startTransmit(str);
    }
}

bool lora_get_recv(const char **str, int *rssi)
{
    *str = lora_recv_data.c_str();
    *rssi = lora_recv_rssi;
    return lora_recv_success;
}

void lora_set_recv_flag(void)
{
    lora_recv_success = false;
}

void lora_sleep(void)
{
    radio.sleep();
}

void lora_param_set(void)
{
    // set carrier frequency to 433.5 MHz
    if (radio.setFrequency(ui_lora_get_freq()) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));
        while (true);
    }

    // set bandwidth to 250 kHz
    if (radio.setBandwidth(ui_lora_get_bandwidth()) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("Selected bandwidth is invalid for this module!"));
        while (true);
    }

    // set output power to 10 dBm (accepted range is -17 - 22 dBm)
    if (radio.setOutputPower(ui_lora_get_power()) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));
        while (true);
    }
}