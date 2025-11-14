#include "Arduino.h"
#include "esp_codec.h"
#include "ExtensionIOXL9555.hpp"

#include <AudioFileSourceSD.h>
#include <AudioGeneratorWAV.h>
#include <AudioFileSourceFunction.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>


#define BOARD_EPD_CS   34
#define BOARD_LORA_CS   3
#define BOARD_LORA_RST  4

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

// SPI
#define BOARD_SPI_SCK  36
#define BOARD_SPI_MOSI 33
#define BOARD_SPI_MISO 47

// ES8311
#define BOARD_I2C_ADDR_ES8311   0x18
#define BOARD_ES8311_SCL        BOARD_I2C_SCL
#define BOARD_ES8311_SDA        BOARD_I2C_SDA
#define BOARD_ES8311_MCLK       38
#define BOARD_ES8311_SCLK       39
#define BOARD_ES8311_ASDOUT     40
#define BOARD_ES8311_LRCK       18
#define BOARD_ES8311_DSDIN      17

// SD card
#define BOARD_SD_CS   48
#define BOARD_SD_SCK  BOARD_SPI_SCK
#define BOARD_SD_MOSI BOARD_SPI_MOSI
#define BOARD_SD_MISO BOARD_SPI_MISO

class EspAudioOutput : public AudioOutput
{
public:
    EspAudioOutput(EspCodec & codec)
    {
        _codec = &codec;
    }
    bool begin()
    {
        Serial.printf("bps:%d channels:%u hertz:%u\n", bps, channels, hertz);
        if (hertz != 16000 || hertz != 32000 || hertz != 24000 || hertz != 41000 || hertz == 0) {
            hertz = 16000;
        }
        channels = 2;
        alreadyBeing = _codec->open(bps, channels, hertz ) != -1;
        return alreadyBeing;
    };

    bool SetBitsPerSample(int bits)
    {
        Serial.printf("Set bps : %u\n", bps);
        bps = bits;
        return true;
    }
    bool SetChannels(int chan)
    {
        Serial.printf("Set channels : %u\n", channels);
        channels = chan;
        return true;
    }
    bool SetRate(int hz)
    {
        hertz = hz;
        Serial.printf("Set Rate : %u\n", hertz);
        if (alreadyBeing) {
            _codec->close();
        }
        alreadyBeing = _codec->open(bps, channels, hertz ) != -1;
        return true;
    }

    bool ConsumeSample(int16_t sample[2])
    {
        return _codec->write((uint8_t*)sample, 4) != -1;
    }

    uint16_t ConsumeSamples(int16_t *samples, uint16_t count)
    {
        Serial.println("ConsumeSamples....");
        _codec->write((uint8_t*)samples, count);
        return count;
    }
    bool stop()
    {
        Serial.println("Closed\n\n\n");
        alreadyBeing = false;
        _codec->close();
        return true;
    }
private:
    bool alreadyBeing = false;
    EspCodec *_codec;
};

EspCodec codec;
ExtensionIOXL9555 io;

EspAudioOutput          *out = NULL;
AudioGeneratorWAV       *wav = NULL;
AudioFileSourceSD       *file = NULL;
AudioGeneratorMP3       *mp3 = NULL;
AudioFileSourceID3      *id3 = NULL;
std::vector<String>     file_list;


void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to open directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory() && levels) {
            Serial.print("  DIR : ");
            Serial.print(file.name());
            Serial.println();

            listDir(fs, file.path(), levels - 1);
        } else {
            String filename = String(file.name());
            if (filename.endsWith(".wav") || filename.endsWith(".mp3")) {
                Serial.print("  FILE: ");
                Serial.print(file.name());
                Serial.print("  SIZE: ");
                Serial.println(file.size());

                // Save full path
                file_list.push_back("/" + filename);
            }
        }
        file = root.openNextFile();
    }
}


void setup()
{
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
    io.digitalWrite(BOARD_XL9555_06_AMPLIFIER, HIGH); // Enable power amplifier
    io.digitalWrite(BOARD_XL9555_12_AUDIO_SEL, LOW); // Audio output selection: A7682E module
    io.digitalWrite(BOARD_XL9555_00_6609_EN, HIGH);     // enable A7682 module power
    io.digitalWrite(BOARD_XL9555_10_PWEKEY_EN, HIGH);   // power on

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    codec.setPins(BOARD_ES8311_MCLK, BOARD_ES8311_SCLK, BOARD_ES8311_LRCK, BOARD_ES8311_ASDOUT, BOARD_ES8311_DSDIN);
    if (codec.begin(Wire, BOARD_I2C_ADDR_ES8311, CODEC_TYPE_ES8311)) {  
        Serial.println("Codec init succeeded");
    } else {
        Serial.println("Warning: Failed to find Codec");
    }
    codec.setVolume(100); // range: [0, 100]
    out = new EspAudioOutput(codec);
    file = new AudioFileSourceSD();
    wav = new AudioGeneratorWAV();
    mp3 = new AudioGeneratorMP3();

    SPI.begin(BOARD_SD_SCK, BOARD_SD_MISO, BOARD_SD_MOSI, BOARD_SD_CS);
    if(!SD.begin(BOARD_SD_CS)){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    // Traverse files in SD card
    listDir(SD, "/", 2); // Change the root directory as needed
    Serial.print("\n\n\nTotal music file found: ");
    Serial.println(file_list.size());

    for (size_t i = 0; i < file_list.size(); i++) {
        Serial.print("File: ");
        Serial.print(i + 1);
        Serial.print(" path: ");
        Serial.println(file_list[i]);
    }

    for (size_t i = 0; i < file_list.size(); i++) {
        Serial.print("File: ");
        Serial.print(i + 1);
        Serial.print(" path: ");
        Serial.println(file_list[i]);

        if (file_list[i].endsWith(".mp3")) {
            file->open(file_list[i].c_str());
            id3 = new AudioFileSourceID3(file);
            mp3->begin(id3, out);
        } else if (file_list[i].endsWith(".wav")) {
            file->open(file_list[i].c_str());
            wav->begin(file, out);
        }

        while (mp3->isRunning()) {
            if (!mp3->loop()) {
                mp3->stop();
                delete id3;
            }
        }

        while (wav->isRunning()) {
            if (!wav->loop()) {
                wav->stop();
            }
        }
    }

    Serial.println("Done.....");
}

void loop()
{
    delay(10000);
}
