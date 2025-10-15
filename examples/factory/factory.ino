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
#include <Preferences.h>

Preferences preferences;

TinyGsm modem(SerialAT);
TaskHandle_t a7682_handle;

XPowersPPM PPM;
BQ27220 bq27220;
Audio audio;

TouchDrvCSTXXX touch;
GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(GxEPD2_310_GDEQ031T10(BOARD_EPD_CS, BOARD_EPD_DC, BOARD_EPD_RST, BOARD_EPD_BUSY)); // GDEQ031T10 240x320, UC8253, (no inking, backside mark KEGMO 3100)

uint8_t *decodebuffer = NULL;
lv_timer_t *flush_timer = NULL;
int disp_refr_mode = DISP_REFR_MODE_PART;

uint8_t isT_Deck_Pro_v1_1 = 0;
const char Version_str1[] = "T-Deck-Pro V1.0";
const char Version_str2[] = "T-Deck-Pro V1.1";

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
    if(isT_Deck_Pro_v1_1) {
        display.getTextBounds(Version_str2, 0, 0, &tbx, &tby, &tbw, &tbh);
    }
    else {
        display.getTextBounds(Version_str1, 0, 0, &tbx, &tby, &tbw, &tbh);
    }
    // center bounding box by transposition of origin:
    uint16_t x = ((display.width() - tbw) / 2) - tbx;
    uint16_t y = ((display.height() - tbh) / 2) - tby;
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(x, y);
        if(isT_Deck_Pro_v1_1) {
            display.print(Version_str2);
        }
        else {
            display.print(Version_str1);
        }

        display.setCursor(x+20, y+20);
        display.print(UI_T_DECK_PRO_VERSION);
    }
    while (display.nextPage());
    display.hibernate();
    return true;
}

static void flush_timer_cb(lv_timer_t *t)
{
    static int idx = 0;
    lv_disp_t *disp = lv_disp_get_default();
    if(disp->rendering_in_progress == false) {
        lv_coord_t w = LV_HOR_RES;
        lv_coord_t h = LV_VER_RES;

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
        display.hibernate();
        
        Serial.printf("flush_timer_cb:%d, %s\n", idx++, (disp_refr_mode == 0 ?"full":"part"));

        disp_refr_mode = DISP_REFR_MODE_PART;
        lv_timer_pause(flush_timer);
    }
}

static void dips_render_start_cb(struct _lv_disp_drv_t * disp_drv)
{
    if(flush_timer == NULL) {
        flush_timer = lv_timer_create(flush_timer_cb, 10, NULL);
    } else {
        lv_timer_resume(flush_timer);
    }
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

    // lv_coord_t w = LV_HOR_RES;
    // lv_coord_t h = LV_VER_RES;

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
    display.hibernate();
    
    static int idx = 0;
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

        // Serial.printf("x = %d, y = %d\n", last_x, last_y);
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

static bool pcm5102a_init(void)
{
    bool ret = audio.setPinout(BOARD_I2S_BCLK, BOARD_I2S_LRC, BOARD_I2S_DOUT);

    if (ret == false) 
        Serial.printf("[%d] Execution error\n", __LINE__);

    audio.setVolume(21); // 0...21

    pinMode(BOARD_6609_EN, OUTPUT);
    digitalWrite(BOARD_6609_EN, HIGH);

    // audio_paly_flag = audio.connecttoFS(SD, "/voice_time/BBIBBI.mp3");

    return true;
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
    gpio_hold_dis((gpio_num_t)BOARD_6609_EN);
    gpio_hold_dis((gpio_num_t)BOARD_LORA_EN);
    gpio_hold_dis((gpio_num_t)BOARD_GPS_EN);
    gpio_hold_dis((gpio_num_t)BOARD_A7682E_PWRKEY);

    gpio_deep_sleep_hold_dis();

    Serial.begin(115200);

    // IO
    pinMode(BOARD_KEYBOARD_LED, OUTPUT);
    pinMode(BOARD_MOTOR_PIN, OUTPUT);
    pinMode(BOARD_6609_EN, OUTPUT);         // enable 7682 module
    pinMode(BOARD_LORA_EN, OUTPUT);         // enable LORA module
    pinMode(BOARD_GPS_EN, OUTPUT);          // enable GPS module
    pinMode(BOARD_A7682E_PWRKEY, OUTPUT); 
    digitalWrite(BOARD_KEYBOARD_LED, LOW);
    digitalWrite(BOARD_MOTOR_PIN, LOW);
    digitalWrite(BOARD_6609_EN, HIGH);
    digitalWrite(BOARD_LORA_EN, HIGH);
    digitalWrite(BOARD_GPS_EN, HIGH);
    digitalWrite(BOARD_A7682E_PWRKEY, HIGH);

    pinMode(BOARD_EPD_BL, OUTPUT); 
    digitalWrite(BOARD_EPD_BL, HIGH);

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
    byte error, address;
    int nDevices = 0;
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Serial.printf(" ------------- I2C ------------- \n");
    for(address = 0x01; address < 0x7F; address++){
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if(error == 0){ // 0: success.
            nDevices++;
            if(address == BOARD_I2C_ADDR_TOUCH){
                // flag_Touch_init = true;
                Serial.printf("[0x%x] TOUCH find!\n", address);
            } else if (address == BOARD_I2C_ADDR_GYROSCOPDE) {
                Serial.printf("[0x%x] GYROSCOPDE find!\n", address);
            } else if (address == BOARD_I2C_ADDR_KEYBOARD) {
                Serial.printf("[0x%x] KEYBOARD find!\n", address);
            } else if (address == BOARD_I2C_ADDR_BQ27220) {
                Serial.printf("[0x%x] BQ27220 find!\n", address);
            } else if (address == BOARD_I2C_ADDR_BQ25896) {
                Serial.printf("[0x%x] BQ25896 find!\n", address);
            } else if (address == BOARD_I2C_ADDR_DRV2605) {
                Serial.printf("[0x%x] DRV2605 find!\n", address);
            }
        }
    }

    for(int i = 0; i < 3; i++) {
        Wire.beginTransmission(BOARD_I2C_ADDR_DRV2605);
        error = Wire.endTransmission();
        if(error == 0) {
            isT_Deck_Pro_v1_1 = 1;
        } else {
            isT_Deck_Pro_v1_1 = 0;
        }
    }

#ifdef T_DECK_PRO_V1_1
    if(!isT_Deck_Pro_v1_1){
        Serial.printf(" ------------- ERROR ------------- \n");
        Serial.printf("Firmware mismatch\n");
        Serial.printf("Your hardware might be the T-Deck-Pro V1.0, but please download the H693_factory_v2.x.bin firmware.\n");
        Serial.printf("T-Deck-Pro V1.0   ---   H693_factory_v1.x.bin \n");
        Serial.printf("T-Deck-Pro V1.1   ---   H693_factory_v2.x.bin\n");
    }
#endif

    Serial.printf(" ------------- SPIFFS ------------- \n");

    if(!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    listDir(SPIFFS, "/", 0);
    Serial.println(" ------------- PERI ------------- ");

    // SPI
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

    // init peripheral
    touch.setPins(BOARD_TOUCH_RST, BOARD_TOUCH_INT);
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

    if(peri_init_st[E_PERI_A7682E] == false)
    {
        peri_init_st[E_PERI_PCM5102A] = pcm5102a_init();
    }

    lvgl_init();

    ui_deckpro_entry();

    disp_full_refr();

    digitalWrite(BOARD_KEYBOARD_LED, LOW);
    digitalWrite(BOARD_MOTOR_PIN, LOW);
    digitalWrite(BOARD_6609_EN, HIGH);
    digitalWrite(BOARD_LORA_EN, HIGH);
    digitalWrite(BOARD_GPS_EN, HIGH);
    digitalWrite(BOARD_A7682E_PWRKEY, HIGH);
}


uint32_t tick = 0;

void loop()
{
    lv_task_handler();
    keypad_loop();

    if(peri_init_st[E_PERI_PCM5102A] == true) 
    {
        audio.loop();
    }
    
    delay(1);


    if(millis() - tick > 3000) {
        tick = millis();
        // printf("BOARD_LORA_CS=%d\n", digitalRead(BOARD_LORA_CS));
        // printf("BOARD_LORA_RST=%d\n", digitalRead(BOARD_LORA_RST));
        // printf("BOARD_LORA_BUSY=%d\n", digitalRead(BOARD_LORA_BUSY));
        // printf("BOARD_LORA_EN=%d\n", digitalRead(BOARD_LORA_EN));

        // printf("BOARD_EPD_CS=%d\n", digitalRead(BOARD_EPD_CS));
    }
}

/*********************************************************************************
 *                              GLOBAL PROTOTYPES
 * *******************************************************************************/
void disp_full_refr(void)
{
    disp_refr_mode = DISP_REFR_MODE_FULL;
}


