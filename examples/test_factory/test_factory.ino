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
#include "test_factory.h"
#include "peripheral.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"


TouchDrvCSTXXX touch;
GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(GxEPD2_310_GDEQ031T10(BOARD_EPD_CS, BOARD_EPD_DC, BOARD_EPD_RST, BOARD_EPD_BUSY)); // GDEQ031T10 240x320, UC8253, (no inking, backside mark KEGMO 3100)

uint8_t *decodebuffer = NULL;
lv_timer_t *flush_timer = NULL;
int disp_refr_mode = DISP_REFR_MODE_PART;
const char HelloWorld[] = "Hello World!";

bool flag_sd_init = false;
bool flag_lora_init = false;
bool flag_Touch_init = false;
bool flag_Gyroscope_init = false;
bool flag_Keypad_init = false;
bool flag_BQ25896_init = false;
bool flag_BQ27220_init = false;
bool flag_LTR553ALS_init = false;
/*********************************************************************************
 *                              STATIC PROTOTYPES
 * *******************************************************************************/
static void helloWorld()
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
    }
    while (display.nextPage());
    display.hibernate();
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
        
        // Serial.printf("flush_timer_cb:%d, %s\n", idx++, (disp_refr_mode == 0 ?"full":"part"));

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
        pixel.bit.b1 = (color_p + i + 7)->ch.red;
        pixel.bit.b2 = (color_p + i + 6)->ch.red;
        pixel.bit.b3 = (color_p + i + 5)->ch.red;
        pixel.bit.b4 = (color_p + i + 4)->ch.red;
        pixel.bit.b5 = (color_p + i + 3)->ch.red;
        pixel.bit.b6 = (color_p + i + 2)->ch.red;
        pixel.bit.b7 = (color_p + i + 1)->ch.red;
        pixel.bit.b8 = (color_p + i + 0)->ch.red;
        decodebuffer[epd_idx] = pixel.full;
        epd_idx++;
    }

    Serial.printf("x1=%d, y1=%d, x2=%d, y2=%d\n", area->x1, area->y1, area->x2, area->y2);

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
    disp_drv.render_start_cb = dips_render_start_cb;
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

void setup()
{
    Serial.begin(115200);

    // IO
    pinMode(BOARD_KEYBOARD_LED, OUTPUT);
    pinMode(BOARD_MOTOR_PIN, OUTPUT);
    pinMode(BOARD_6609_EN, OUTPUT);         // enable 7682 module
    pinMode(BOARD_LORA_EN, OUTPUT);         // enable LORA module
    pinMode(BOARD_GPS_EN, OUTPUT);          // enable GPS module
    pinMode(BOARD_1V8_EN, OUTPUT);          // enable gyroscope module
    pinMode(BOARD_A7682E_PWRKEY, OUTPUT); 
    digitalWrite(BOARD_KEYBOARD_LED, LOW);
    digitalWrite(BOARD_MOTOR_PIN, LOW);
    digitalWrite(BOARD_6609_EN, HIGH);
    digitalWrite(BOARD_LORA_EN, HIGH);
    digitalWrite(BOARD_GPS_EN, HIGH);
    digitalWrite(BOARD_1V8_EN, HIGH);
    digitalWrite(BOARD_A7682E_PWRKEY, HIGH);

    // i2c devices
    byte error, address;
    int nDevices = 0;
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Serial.printf("------------- I2C ------------- \n");
    for(address = 0x01; address < 0x7F; address++){
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if(error == 0){ // 0: success.
            nDevices++;
            if(address == BOARD_I2C_ADDR_TOUCH){
                // flag_Touch_init = true;
                Serial.printf("[0x%x] TOUCH find!\n", address);
            } else if (address == BOARD_I2C_ADDR_LTR_553ALS) {
                flag_LTR553ALS_init = true;
                Serial.printf("[0x%x] LTR_553ALS find!\n", address);
            } else if (address == BOARD_I2C_ADDR_GYROSCOPDE) {
                flag_Gyroscope_init = true;
                Serial.printf("[0x%x] GYROSCOPDE find!\n", address);
            } else if (address == BOARD_I2C_ADDR_KEYBOARD) {
                // flag_Keypad_init = true;
                Serial.printf("[0x%x] KEYBOARD find!\n", address);
            } else if (address == BOARD_I2C_ADDR_BQ27220) {
                flag_BQ27220_init = true;
                Serial.printf("[0x%x] BQ27220 find!\n", address);
            } else if (address == BOARD_I2C_ADDR_BQ25896) {
                flag_BQ25896_init = true;
                Serial.printf("[0x%x] BQ25896 find!\n", address);
            }
        }
    }
    Serial.printf("------------------------------ \n");

    touch.setPins(BOARD_TOUCH_RST, BOARD_TOUCH_INT);
    flag_Touch_init = touch.begin(Wire, BOARD_I2C_ADDR_TOUCH, BOARD_TOUCH_SDA, BOARD_TOUCH_SCL);
    if (!flag_Touch_init) {
        Serial.println("Failed to find Capacitive Touch !");
    } else {
        Serial.println("Find Capacitive Touch");
    }

    flag_Keypad_init = keypad_init(BOARD_I2C_ADDR_KEYBOARD);

    // SPI
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

    helloWorld();
    // delay(1000);

    if(SD.begin(BOARD_SD_CS)) {
        flag_sd_init = true;
        Serial.println("sd success");
    } else {
        Serial.println("sd faild");
    }

    // lora
    flag_lora_init = lora_init();


    // GPS AT init
    SerialGPS.begin(38400, SERIAL_8N1, BOARD_GPS_RXD, BOARD_GPS_TXD);


    lvgl_init();

    ui_deckpro_entry();

    disp_full_refr();
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

