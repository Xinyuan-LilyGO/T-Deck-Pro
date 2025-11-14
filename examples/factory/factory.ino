/**
 * @file      test_touchpad.h
 * @author    ShallowGreen123
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2024-05-27
 *
 */


#include <Arduino.h>
#include "utilities.h"
#include <GxEPD2_BW.h>
#include <TouchDrvCSTXXX.hpp>
#include <TinyGPS++.h>
#include "lvgl.h"
#include "ui_deckpro.h"
#include <Fonts/FreeMonoBold9pt7b.h>
#include "factory.h"
#include "peripheral.h"
#include <SensorWireHelper.h>

TinyGsm modem(SerialAT);
TaskHandle_t a7682_handle;

XPowersPPM PPM;
BQ27220 bq27220;

TouchDrvCSTXXX touch;
GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(GxEPD2_310_GDEQ031T10(BOARD_EPD_CS, BOARD_EPD_DC, BOARD_EPD_RST, BOARD_EPD_BUSY)); // GDEQ031T10 240x320, UC8253, (no inking, backside mark KEGMO 3100)
ExtensionIOXL9555 xl9555_io;
Adafruit_DRV2605 motor_drv;
EspCodec codec;

uint8_t *decodebuffer = NULL;
int disp_refr_mode = DISP_REFR_MODE_PART;
const char HelloWorld[] = "T-Deck-Pro MAX v0.1";

bool peri_init_st[E_PERI_NUM_MAX] = {0};

/*********************************************************************************
 *                              STATIC PROTOTYPES
 * *******************************************************************************/
static bool ink_screen_init()
{
    // SPI.begin(BOARD_SPI_SCK, -1, BOARD_SPI_MOSI, BOARD_EPD_CS);
    display.init(115200, true, 2, false);
    //Serial.println("helloWorld");
    display.setRotation(0);
    display.setFont(&FreeMonoBold9pt7b);
    if (display.epd2.WIDTH < 104) display.setFont(0);
    display.setTextColor(GxEPD_BLACK);
    int16_t tbx, tby; uint16_t tbw, tbh;
    display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
    // center bounding box by transposition of origin:
    uint16_t x = ((display.width() - tbw) / 2) - tbx;
    uint16_t y = ((display.height() - tbh) / 2) - tby;
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(x, y);
        display.print(HelloWorld);

        display.setCursor(x+35, y+30);
        display.print(UI_T_DECK_PRO_VERSION);
    }
    while (display.nextPage());
    display.hibernate();
    return true;
}

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    uint32_t w = (area->x2 - area->x1);
    uint32_t h = (area->y2 - area->y1);

    uint16_t epd_idx = 0;

    union flush_buf_pixel pixel;

    for(int i = 0; i < w * h; i += 8) {
        pixel.bit.b1 = (color_p + i + 7)->full;
        pixel.bit.b2 = (color_p + i + 6)->full;
        pixel.bit.b3 = (color_p + i + 5)->full;
        pixel.bit.b4 = (color_p + i + 4)->full;
        pixel.bit.b5 = (color_p + i + 3)->full;
        pixel.bit.b6 = (color_p + i + 2)->full;
        pixel.bit.b7 = (color_p + i + 1)->full;
        pixel.bit.b8 = (color_p + i + 0)->full;
        decodebuffer[epd_idx] = pixel.full;
        epd_idx++;
    }

    static int idx = 0;
    if(disp_refr_mode == DISP_REFR_MODE_PART) {
        display.setPartialWindow(0, 0, w, h);
    } else if(disp_refr_mode == DISP_REFR_MODE_FULL){
        display.setFullWindow();
    }

    display.firstPage();
    do {
        display.drawInvertedBitmap(0, 0, decodebuffer, w, h - 3, GxEPD_BLACK);
    }
    while (display.nextPage());
    // display.hibernate();
    
    Serial.printf("flush_timer_cb:%d, %s\n", idx++, (disp_refr_mode == 0 ?"full":"part"));

    disp_refr_mode = DISP_REFR_MODE_PART;

    // Serial.printf("x1=%d, y1=%d, x2=%d, y2=%d\n", area->x1, area->y1, area->x2, area->y2);

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(disp_drv);
}

static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;

    uint8_t touched = touch.getPoint(&last_x, &last_y, 1);
    if(touched) {
        data->state = LV_INDEV_STATE_PR;

        Serial.printf("x = %d, y = %d\n", last_x, last_y);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;
}

static void lvgl_init(void)
{
    lv_init();

    static lv_disp_draw_buf_t draw_buf_dsc_1;
    lv_color_t *buf_1 = (lv_color_t *)ps_calloc(sizeof(lv_color_t), DISP_BUF_SIZE);
    lv_color_t *buf_2 = (lv_color_t *)ps_calloc(sizeof(lv_color_t), DISP_BUF_SIZE);
    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, buf_2, LCD_HOR_SIZE * LCD_VER_SIZE);
    decodebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), DISP_BUF_SIZE);
    // lv_disp_draw_buf_init(&draw_buf, lv_disp_buf_p, NULL, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_HOR_SIZE;
    disp_drv.ver_res = LCD_VER_SIZE;
    disp_drv.flush_cb = disp_flush;
    // disp_drv.render_start_cb = dips_render_start_cb;
    disp_drv.draw_buf = &draw_buf_dsc_1;
    // disp_drv.rounder_cb = display_driver_rounder_cb;
    disp_drv.full_refresh = 1;

    lv_disp_drv_register(&disp_drv);

    /*------------------
     * Touchpad
     * -----------------*/
    /*Register a touchpad input device*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);
}

static bool bq25896_init(void)
{
    // BQ25896 --- 0x6B
    Wire.beginTransmission(BOARD_I2C_ADDR_BQ25896);
    if (Wire.endTransmission() == 0)
    {
        // battery_25896.begin();
        PPM.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, BOARD_I2C_ADDR_BQ25896);
        // set battery charge voltage
        PPM.setChargeTargetVoltage(4288);

        // Set charge current
        PPM.setChargerConstantCurr(1024);

        // Enable measure
        PPM.enableMeasure();

        return true;
    }
    return false;
}

static bool bq27220_init(void)
{
    bool ret = bq27220.init();
    // if(ret) 
    //     bq27220.reset();
    return ret;
}

static bool sd_care_init(void)
{
    if(!SD.begin(BOARD_SD_CS)){
        Serial.println("[SD CARD] Card Mount Failed");
        return false;
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    uint64_t totalSize = SD.totalBytes() / (1024 * 1024);
    Serial.printf("SD Card Total: %lluMB\n", totalSize);

    uint64_t usedSize = SD.usedBytes() / (1024 * 1024);
    Serial.printf("SD Card Used: %lluMB\n", usedSize);
    return true;
}

static void a7682_task(void *param)
{
    vTaskSuspend(a7682_handle);
    while (1)
    {
        while (SerialAT.available())
        {
            SerialMon.write(SerialAT.read());
        }
        while (SerialMon.available())
        {
            SerialAT.write(SerialMon.read());
        }
        delay(1);
    }
}

static bool A7682E_init(void)
{
    Serial.println("Place your board outside to catch satelite signal");

    // Set module baud rate and UART pins
    SerialAT.begin(115200, SERIAL_8N1, BOARD_A7682E_TXD, BOARD_A7682E_RXD);

    Serial.println("Start modem...");

    // power on
    digitalWrite(BOARD_A7682E_PWRKEY, LOW);
    delay(10);
    digitalWrite(BOARD_A7682E_PWRKEY, HIGH);
    delay(50);
    digitalWrite(BOARD_A7682E_PWRKEY, LOW);
    delay(10);

    int retry_cnt = 5;
    int retry = 0;
    while (!modem.testAT(1000)) {
        Serial.println(".");
        if (retry++ > retry_cnt) {
            digitalWrite(BOARD_A7682E_PWRKEY, LOW);
            delay(100);
            digitalWrite(BOARD_A7682E_PWRKEY, HIGH);
            delay(1000);
            digitalWrite(BOARD_A7682E_PWRKEY, LOW);

            Serial.println("[A7682E] Init Fail");
            break;
        }
    }
    
    Serial.println();
    delay(200);

    xTaskCreate(a7682_task, "a7682_handle", 1024 * 3, NULL, A7682E_PRIORITY, &a7682_handle);

    return (retry < retry_cnt);
}

static void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing spiffs directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void setup()
{
    setCpuFrequencyMhz(240);
    Serial.begin(115200);

    // LORA、SD、EPD use the same SPI, in order to avoid mutual influence;
    // before powering on, all CS signals should be pulled high and in an unselected state;
    pinMode(BOARD_LORA_CS, OUTPUT); 
    digitalWrite(BOARD_LORA_CS, HIGH);
    pinMode(BOARD_LORA_RST, OUTPUT); 
    digitalWrite(BOARD_LORA_RST, HIGH);
    pinMode(BOARD_SD_CS, OUTPUT); 
    digitalWrite(BOARD_SD_CS, HIGH);
    pinMode(BOARD_EPD_CS, OUTPUT); 
    digitalWrite(BOARD_EPD_CS, HIGH);

    // i2c devices
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Serial.printf(" -------------------------- I2C -------------------------- \n");
    SensorWireHelper::dumpDevices(Wire, Serial);

    // XL9555 Init
    Serial.printf(" -------------------------- XL9555 -------------------------- \n");
    if (xl9555_io.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, XL9555_SLAVE_ADDRESS0)) {
        const uint8_t expands[] = {
            BOARD_XL9555_00_6609_EN,
            BOARD_XL9555_01_LORA_EN,
            BOARD_XL9555_02_GPS_EN,
            BOARD_XL9555_03_1V8_EN,
            // BOARD_XL9555_04_LORA_SEL,
            BOARD_XL9555_05_MOTOR_EN,
            BOARD_XL9555_06_AMPLIFIER,
            BOARD_XL9555_07_TOUCH_RST,
            BOARD_XL9555_10_PWEKEY_EN,
            BOARD_XL9555_11_KEY_RST,
            BOARD_XL9555_12_AUDIO_SEL,
        };
        for (auto pin : expands) {
            xl9555_io.pinMode(pin, OUTPUT);
            xl9555_io.digitalWrite(pin, HIGH);
            delay(1);
        }
        // HIGH --- external antenna
        // LOW --- internal antenna (default)
        xl9555_io.pinMode(BOARD_XL9555_04_LORA_SEL, OUTPUT);
        xl9555_io.digitalWrite(BOARD_XL9555_04_LORA_SEL, LOW);

        // xl9555_io.digitalWrite(BOARD_XL9555_07_TOUCH_RST, LOW);
        // delay(100);
        // xl9555_io.digitalWrite(BOARD_XL9555_07_TOUCH_RST, HIGH);
    } else {
        while (1) {
            Serial.println("Failed to find XL9555 - check your wiring!");
            delay(1000);
        }
    }

    Serial.print("PORT0:0b");
    Serial.print(xl9555_io.readPort(ExtensionIOXL9555::PORT0), BIN);
    Serial.print("\tPORT1:0b");
    Serial.println(xl9555_io.readPort(ExtensionIOXL9555::PORT1), BIN);

    // open backlight
    analogWrite(BOARD_EPD_BL, 50);
    analogWrite(BOARD_KEYBOARD_LED, 255);

    // init motor
    Serial.printf(" -------------------------- DRV2605 -------------------------- \n");
    if(!motor_drv.begin()) {
        while (1) {
            Serial.println("Failed to find DRV2605 - Motor drive");
            delay(1000);
        }
    }
    motor_drv.selectLibrary(1);
    motor_drv.setMode(DRV2605_MODE_INTTRIG);
    motor_drv.setWaveform(0, 13); // play effect
    motor_drv.setWaveform(1, 0);      // end waveform
    motor_drv.go();

    Serial.printf(" -------------------------- ES8311 -------------------------- \n");
    codec.setPins(BOARD_ES8311_MCLK, BOARD_ES8311_SCLK, BOARD_ES8311_LRCK, BOARD_ES8311_ASDOUT, BOARD_ES8311_DSDIN);
    if (codec.begin(Wire, BOARD_I2C_ADDR_ES8311, CODEC_TYPE_ES8311)) {
        Serial.println("Codec init succeeded");
        peri_init_st[E_PERI_ES8311] = true;
    } else {
        Serial.println("Warning: Failed to find Codec");
    }
    codec.setPaPinCallback([](bool enable, void *user_data) {
        //  HIGH : the headphones and speakers output the sound from A7682E.
        //  LOW :  the headphones and speakers output the sound from ES8311.
        ((ExtensionIOXL9555 *)user_data)->digitalWrite(BOARD_XL9555_12_AUDIO_SEL, LOW);
        ((ExtensionIOXL9555 *)user_data)->digitalWrite(BOARD_XL9555_06_AMPLIFIER, HIGH);
    }, &xl9555_io);
    codec.setVolume(50); // range: [0, 100]
    // codec.playWAV((uint8_t*)wav_hex, wav_hex_len);

    // Serial.printf(" ------------- SPIFFS ------------- \n");

    // if(!SPIFFS.begin(true)){
    //     Serial.println("SPIFFS Mount Failed");
    //     return;
    // }

    // listDir(SPIFFS, "/", 0);
    // Serial.println(" ------------- PERI ------------- ");

    // SPI
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

    // init peripheral
    touch.setPins(-1, BOARD_TOUCH_INT); // touch_rst connect the IO07 of the chip XL9555
    peri_init_st[E_PERI_INK_SCREEN] = ink_screen_init();
    peri_init_st[E_PERI_LORA]       = lora_init();
    peri_init_st[E_PERI_TOUCH]      = touch.begin(Wire, BOARD_I2C_ADDR_TOUCH, BOARD_TOUCH_SDA, BOARD_TOUCH_SCL);
    peri_init_st[E_PERI_KYEPAD]     = keypad_init(BOARD_I2C_ADDR_KEYBOARD);
    peri_init_st[E_PERI_BQ25896]    = bq25896_init();
    peri_init_st[E_PERI_BQ27220]    = bq27220_init();
    peri_init_st[E_PERI_SD]         = sd_care_init();
    peri_init_st[E_PERI_GPS]        = gps_init();
    peri_init_st[E_PERI_BHI260AP]   = BHI260AP_init();
    peri_init_st[E_PERI_A7682E]     = A7682E_init();

    lvgl_init();

    ui_deckpro_entry();

    disp_full_refr();

    // close backlight
    analogWrite(BOARD_EPD_BL, 0);
    analogWrite(BOARD_KEYBOARD_LED, 0);
}



void loop()
{
    lv_task_handler();
    keypad_loop();

    
    delay(1);
}

/*********************************************************************************
 *                              GLOBAL PROTOTYPES
 * *******************************************************************************/
void disp_full_refr(void)
{
    disp_refr_mode = DISP_REFR_MODE_FULL;
}


