/**
 * @brief We just set up the codec and the i2c pins, so that the wire is
 * automatically initialized with the indicated pins.
 * After this you can set up and use i2s
 * @author phil schatzmann
 */

#include "AudioBoard.h"
#include "ExtensionIOXL9555.hpp"
#include "Audio.h"
#include "FS.h"
#include "SPIFFS.h"

#define BOARD_EPD_CS   34
#define BOARD_LORA_CS   3
#define BOARD_LORA_RST  4
// SPI
#define BOARD_SPI_SCK  36
#define BOARD_SPI_MOSI 33
#define BOARD_SPI_MISO 47

// XL9555
#define BOARD_I2C_SDA 13
#define BOARD_I2C_SCL 14

#define BOARD_XL9555_00_6609_EN     (0)
#define BOARD_XL9555_10_PWEKEY_EN   (8)

// Connected to XL9555 IO06, enable power amplifier,
// increase the volume of the speaker
#define BOARD_XL9555_06_SHUTDOWN (6) // Connected to XL9555 IO06
/* Module A7682E and ES8311 share the output for headphones and speakers.
/  Select the audio output through AUDIO_SEL. When AUDIO_SEL is
/  HIGH : the headphones and speakers output the sound from A7682E.
/  LOW :  the headphones and speakers output the sound from ES8311.
*/
#define BOARD_XL9555_12_AUDIO_SEL (10) // Connected to XL9555 IO12
ExtensionIOXL9555 io;

// SD card
#define BOARD_SD_CS   48
#define BOARD_SD_SCK  BOARD_SPI_SCK
#define BOARD_SD_MOSI BOARD_SPI_MOSI
#define BOARD_SD_MISO BOARD_SPI_MISO

// ES8311
#define BOARD_I2C_ADDR_ES8311   0x18
#define BOARD_ES8311_SCL        BOARD_I2C_SCL
#define BOARD_ES8311_SDA        BOARD_I2C_SDA
#define BOARD_ES8311_MCLK       38
#define BOARD_ES8311_SCLK       39
#define BOARD_ES8311_ASDOUT     17
#define BOARD_ES8311_LRCK       18
#define BOARD_ES8311_DSDIN      40

Audio audio;
DriverPins my_pins;
AudioBoard board(AudioDriverES8311, my_pins);

void scan_i2c_device(TwoWire &i2c)  //I2C Module address scanning function
{
  Serial.println("Scanning for I2C devices ...");
  Serial.print("      ");
  for (int i = 0; i < 0x10; i++) {
    Serial.printf("0x%02X|", i);
  }
  uint8_t error;
  for (int j = 0; j < 0x80; j += 0x10) {
    Serial.println();
    Serial.printf("0x%02X |", j);
    for (int i = 0; i < 0x10; i++) {
      Wire.beginTransmission(i | j);
      error = Wire.endTransmission();
      if (error == 0)
        Serial.printf("0x%02X|", i | j);
      else
        Serial.print(" -- |");
    }
  }
  Serial.println();
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.path(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void sd_card_init(void)
{
    if (!SD.begin(BOARD_SD_CS))
    {
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC)
    {
        Serial.println("MMC");
    }
    else if (cardType == CARD_SD)
    {
        Serial.println("SDSC");
    }
    else if (cardType == CARD_SDHC)
    {
        Serial.println("SDHC");
    }
    else
    {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    listDir(SD, "/", 0);
}

void setup()
{
    // Setup logging
    Serial.begin(115200);

    // LORA、SD、EPD use the same SPI, in order to avoid mutual influence;
    // before powering on, all CS signals should be pulled high and in an unselected state;
    pinMode(BOARD_LORA_CS, OUTPUT); 
    digitalWrite(BOARD_LORA_CS, HIGH);
    pinMode(BOARD_LORA_RST, OUTPUT); 
    digitalWrite(BOARD_LORA_RST, HIGH);
    pinMode(BOARD_EPD_CS, OUTPUT); 
    digitalWrite(BOARD_EPD_CS, HIGH);

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
    io.digitalWrite(BOARD_XL9555_06_SHUTDOWN, HIGH); // Enable power amplifier
    io.digitalWrite(BOARD_XL9555_12_AUDIO_SEL, LOW); // Audio output selection: A7682E module

    io.digitalWrite(BOARD_XL9555_00_6609_EN, HIGH);     // enable A7682 module power
    io.digitalWrite(BOARD_XL9555_10_PWEKEY_EN, HIGH);   // power on

    // 
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
    sd_card_init();
    
    // ES8311
    // add i2c codec pins: scl, sda, port
    my_pins.addI2C(PinFunction::CODEC, BOARD_ES8311_SCL, BOARD_ES8311_SDA, BOARD_I2C_ADDR_ES8311);

    // configure codec 
    CodecConfig cfg;
    cfg.input_device = ADC_INPUT_ALL;//ADC_INPUT_LINE1; ADC_INPUT_ALL
    cfg.output_device = DAC_OUTPUT_ALL; 
    cfg.i2s.bits = BIT_LENGTH_16BITS;
    cfg.i2s.rate = RATE_44K;
    cfg.i2s.fmt = I2S_NORMAL;  

    if(!board.begin(cfg))
    {
        Serial.println("ES8311 Init Failed");
        return;
    }

    audio.setPinout(BOARD_ES8311_SCLK, BOARD_ES8311_LRCK, BOARD_ES8311_ASDOUT, BOARD_ES8311_DSDIN);
    audio.setVolume(21); // 0...21
    audio.connecttoFS(SD, "BBIBBI.mp3");
}

void loop() 
{
    audio.loop();
}

// optional
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info){  //id3 metadata
    Serial.print("id3data     ");Serial.println(info);
}
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
}
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);
}
void audio_showstreamtitle(const char *info){
    Serial.print("streamtitle ");Serial.println(info);
}
void audio_bitrate(const char *info){
    Serial.print("bitrate     ");Serial.println(info);
}
void audio_commercial(const char *info){  //duration in sec
    Serial.print("commercial  ");Serial.println(info);
}
void audio_icyurl(const char *info){  //homepage
    Serial.print("icyurl      ");Serial.println(info);
}
void audio_lasthost(const char *info){  //stream URL played
    Serial.print("lasthost    ");Serial.println(info);
}
