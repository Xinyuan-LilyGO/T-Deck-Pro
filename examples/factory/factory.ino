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
#include <freertos/queue.h>
// #include "wav_hex.h"

#ifndef FACTORY_RUNTIME_LOG
#define FACTORY_RUNTIME_LOG 0
#endif

#define FACTORY_BATTERY_DESIGN_CAPACITY_MAH 1400

TinyGsm modem(SerialAT);
TaskHandle_t a7682_handle;

XPowersPPM PPM;
BQ27220 bq27220;

// TouchDrvCSTXXX touch;
GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(GxEPD2_310_GDEQ031T10(BOARD_EPD_CS, BOARD_EPD_DC, BOARD_EPD_RST, BOARD_EPD_BUSY)); // GDEQ031T10 240x320, UC8253, (no inking, backside mark KEGMO 3100)
ExtensionIOXL9555 xl9555_io;
Adafruit_DRV2605 motor_drv;
EspCodec codec;

uint8_t *decodebuffer = NULL;
int disp_refr_mode = DISP_REFR_MODE_PART;
static lv_disp_t *factory_disp = NULL;
static uint32_t disp_partial_refr_until_ms = 0;
const char HelloWorld[] = BOARD_NAME;

bool peri_init_st[E_PERI_NUM_MAX] = {0};

typedef enum {
    PHONE_CMD_DIAL = 0,
    PHONE_CMD_ANSWER,
    PHONE_CMD_HANGUP,
    PHONE_CMD_TEST_DIGITS,
    PHONE_CMD_DEBUG,
} phone_cmd_type_t;

typedef struct {
    phone_cmd_type_t type;
    char number[UI_PHONE_NUMBER_LEN];
    bool enabled;
} phone_cmd_t;

static QueueHandle_t phone_cmd_queue = NULL;
static portMUX_TYPE phone_snapshot_mux = portMUX_INITIALIZER_UNLOCKED;
static ui_phone_snapshot_t phone_snapshot = {};
static bool phone_audio_restore_valid = false;
static int phone_audio_restore_sel = HIGH;
static int phone_audio_restore_amp = HIGH;
static bool phone_call_connected = false;
static bool phone_call_incoming = false;
static bool phone_call_user_hangup = false;
static bool phone_clcc_query_pending = false;
static bool phone_clcc_lines_seen = false;
static volatile bool phone_test_audio_active = false;
static uint8_t phone_test_audio_ok_count = 0;
static uint32_t phone_call_start_ms = 0;
static uint32_t phone_state_started_ms = 0;
static uint32_t phone_test_audio_min_done_ms = 0;
static uint32_t phone_test_audio_restore_ms = 0;
static uint32_t phone_history_sequence = 0;
static char phone_end_reason[UI_PHONE_STATUS_LEN] = {0};

/*********************************************************************************
 *                              STATIC PROTOTYPES
 * *******************************************************************************/
static void epd_prepare_bus()
{
    // EPD, LoRa and SD share the SPI bus. Keep the other devices deselected
    // before sending commands to the panel to avoid command corruption.
    pinMode(BOARD_LORA_CS, OUTPUT);
    digitalWrite(BOARD_LORA_CS, HIGH);
    pinMode(BOARD_SD_CS, OUTPUT);
    digitalWrite(BOARD_SD_CS, HIGH);
    pinMode(BOARD_EPD_CS, OUTPUT);
    digitalWrite(BOARD_EPD_CS, HIGH);
}

static bool epd_area_is_full_screen(const lv_area_t *area)
{
    if (area == NULL) {
        return false;
    }

    return area->x1 <= 0 &&
           area->y1 <= 0 &&
           area->x2 >= (LCD_HOR_SIZE - 1) &&
           area->y2 >= (LCD_VER_SIZE - 1);
}

static uint32_t pack_lvgl_area_to_epd(const lv_color_t *color_p, uint32_t w, uint32_t h)
{
    const uint32_t bytes_per_row = (w + 7U) / 8U;
    uint32_t epd_idx = 0;

    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; x += 8U) {
            uint8_t byte = 0;

            for (uint32_t bit = 0; bit < 8U; ++bit) {
                const uint32_t src_x = x + bit;
                if ((src_x < w) && color_p[(y * w) + src_x].full) {
                    byte |= (0x80U >> bit);
                }
            }

            decodebuffer[epd_idx++] = byte;
        }
    }

    return bytes_per_row * h;
}

static bool ink_screen_init()
{
    epd_prepare_bus();
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
    display.powerOff();
    return true;
}

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    const uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    const uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
    (void)pack_lvgl_area_to_epd(color_p, w, h);

    static int idx = 0;
    const bool full_window = epd_area_is_full_screen(area);

    epd_prepare_bus();
    // if(full_window) {
    //     display.setFullWindow();
    // } else {
    //     display.setPartialWindow(area->x1, area->y1, w, h);
    // }
    
    if(disp_refr_mode == DISP_REFR_MODE_PART) {
        display.setPartialWindow(area->x1, area->y1, w, h);
    } else if(disp_refr_mode == DISP_REFR_MODE_FULL){
        display.setFullWindow();
    }

    display.firstPage();
    do {
        display.drawInvertedBitmap(area->x1, area->y1, decodebuffer, w, h, GxEPD_BLACK);
    }
    while (display.nextPage());
    // display.hibernate();
    
#if FACTORY_RUNTIME_LOG
    Serial.printf("flush_timer_cb:%d, %s, x=%d, y=%d, w=%lu, h=%lu\n",
                  idx++,
                  (full_window ? "full" : "part"),
                  area->x1, area->y1,
                  (unsigned long)w, (unsigned long)h);
#else
    (void)idx;
#endif

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

    // uint8_t touched = touch.getPoint(&last_x, &last_y, 1);
    uint8_t touched = hyn_touch_get_point(&last_x, &last_y, 1);
    if(touched) {
        data->state = LV_INDEV_STATE_PR;

#if FACTORY_RUNTIME_LOG
        Serial.printf("x = %d, y = %d\n", last_x, last_y);
#endif
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

    factory_disp = lv_disp_drv_register(&disp_drv);

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
    int ret = 0;
    // BQ25896 --- 0x6B
    // ret = Wire.beginTransmission(BOARD_I2C_ADDR_BQ25896);
    // ret = Wire.endTransmission();
    // if (ret == 0)
    // {
    //     // battery_25896.begin();
    //     PPM.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, BOARD_I2C_ADDR_BQ25896);
    //     // set battery charge voltage
    //     PPM.setChargeTargetVoltage(4288);

    //     // Set charge current
    //     PPM.setChargerConstantCurr(1024);

    //     // Enable measure
    //     // PPM.enableMeasure();
    //     PPM.disableMeasure();

    //     return true;
    // }

    // SY6970 --- 0x6A
    Wire.beginTransmission(SY6970_SLAVE_ADDRESS);
    ret = Wire.endTransmission();
    if(ret == 0) {
        PPM.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, SY6970_SLAVE_ADDRESS);
        // set battery charge voltage
        PPM.setChargeTargetVoltage(4288);

        // Set charge current
        PPM.setChargerConstantCurr(1024);

        // Enable measure
        PPM.enableMeasure();
        // PPM.disableMeasure();

        return true;
    }
    return false;
}

static bool bq27220_init(void)
{
    bq27220.setDefaultCapacity(FACTORY_BATTERY_DESIGN_CAPACITY_MAH);
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

static void phone_copy_text(char *dst, size_t dst_len, const char *src)
{
    if (dst == NULL || dst_len == 0U) {
        return;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    strncpy(dst, src, dst_len - 1U);
    dst[dst_len - 1U] = '\0';
}

static void phone_snapshot_reset(bool available)
{
    portENTER_CRITICAL(&phone_snapshot_mux);
    memset(&phone_snapshot, 0, sizeof(phone_snapshot));
    phone_snapshot.state = available ? UI_PHONE_STATE_IDLE : UI_PHONE_STATE_UNAVAILABLE;
    phone_copy_text(phone_snapshot.status, sizeof(phone_snapshot.status), available ? "Idle" : "A7682E Failed");
    phone_snapshot.debug_passthrough = false;
    portEXIT_CRITICAL(&phone_snapshot_mux);

    phone_audio_restore_valid = false;
    phone_call_connected = false;
    phone_call_incoming = false;
    phone_call_user_hangup = false;
    phone_clcc_query_pending = false;
    phone_clcc_lines_seen = false;
    phone_test_audio_active = false;
    phone_test_audio_ok_count = 0;
    phone_call_start_ms = 0;
    phone_state_started_ms = 0;
    phone_test_audio_min_done_ms = 0;
    phone_test_audio_restore_ms = 0;
    phone_history_sequence = 0;
    memset(phone_end_reason, 0, sizeof(phone_end_reason));
}

static ui_phone_state_t phone_snapshot_state(void)
{
    ui_phone_state_t state;
    portENTER_CRITICAL(&phone_snapshot_mux);
    state = phone_snapshot.state;
    portEXIT_CRITICAL(&phone_snapshot_mux);
    return state;
}

static void phone_snapshot_set_debug(bool enabled)
{
    portENTER_CRITICAL(&phone_snapshot_mux);
    phone_snapshot.debug_passthrough = enabled;
    if (phone_snapshot.state == UI_PHONE_STATE_IDLE) {
        phone_copy_text(phone_snapshot.status, sizeof(phone_snapshot.status), enabled ? "AT Debug" : "Idle");
    }
    portEXIT_CRITICAL(&phone_snapshot_mux);
}

static void phone_snapshot_set_state(ui_phone_state_t state, const char *number, const char *status)
{
    portENTER_CRITICAL(&phone_snapshot_mux);
    phone_snapshot.state = state;
    phone_snapshot.debug_passthrough = false;
    if (number != NULL) {
        phone_copy_text(phone_snapshot.current_number, sizeof(phone_snapshot.current_number), number);
    }
    if (status != NULL) {
        phone_copy_text(phone_snapshot.status, sizeof(phone_snapshot.status), status);
    }
    portEXIT_CRITICAL(&phone_snapshot_mux);
}

static void phone_snapshot_set_number(const char *number)
{
    portENTER_CRITICAL(&phone_snapshot_mux);
    phone_copy_text(phone_snapshot.current_number, sizeof(phone_snapshot.current_number), number);
    portEXIT_CRITICAL(&phone_snapshot_mux);
}

static void phone_snapshot_add_history(const char *direction, const char *result, const char *number)
{
    if (number == NULL || number[0] == '\0') {
        return;
    }

    portENTER_CRITICAL(&phone_snapshot_mux);
    for (int i = UI_PHONE_HISTORY_MAX - 1; i > 0; --i) {
        phone_snapshot.recent_calls[i] = phone_snapshot.recent_calls[i - 1];
    }

    memset(&phone_snapshot.recent_calls[0], 0, sizeof(phone_snapshot.recent_calls[0]));
    phone_copy_text(phone_snapshot.recent_calls[0].number, sizeof(phone_snapshot.recent_calls[0].number), number);
    phone_copy_text(phone_snapshot.recent_calls[0].direction, sizeof(phone_snapshot.recent_calls[0].direction), direction);
    phone_copy_text(phone_snapshot.recent_calls[0].result, sizeof(phone_snapshot.recent_calls[0].result), result);
    phone_snapshot.recent_calls[0].sequence = ++phone_history_sequence;
    portEXIT_CRITICAL(&phone_snapshot_mux);
}

static void phone_capture_audio_state(void)
{
    if (phone_audio_restore_valid) {
        return;
    }

    phone_audio_restore_sel = xl9555_io.digitalRead(BOARD_XL9555_12_AUDIO_SEL);
    phone_audio_restore_amp = xl9555_io.digitalRead(BOARD_XL9555_06_AMPLIFIER);
    phone_audio_restore_valid = true;
}

static void phone_enable_call_audio(void)
{
    phone_capture_audio_state();
    xl9555_io.digitalWrite(BOARD_XL9555_12_AUDIO_SEL, HIGH);
    xl9555_io.digitalWrite(BOARD_XL9555_06_AMPLIFIER, HIGH);
}

static void phone_restore_audio(void)
{
    if (!phone_audio_restore_valid) {
        return;
    }

    xl9555_io.digitalWrite(BOARD_XL9555_12_AUDIO_SEL, phone_audio_restore_sel);
    xl9555_io.digitalWrite(BOARD_XL9555_06_AMPLIFIER, phone_audio_restore_amp);
    phone_audio_restore_valid = false;
}

static void phone_reset_call_session(void)
{
    phone_call_connected = false;
    phone_call_incoming = false;
    phone_call_user_hangup = false;
    phone_clcc_query_pending = false;
    phone_clcc_lines_seen = false;
    phone_call_start_ms = 0;
    phone_state_started_ms = 0;
    memset(phone_end_reason, 0, sizeof(phone_end_reason));
}

static void phone_set_end_reason(const char *reason)
{
    phone_copy_text(phone_end_reason, sizeof(phone_end_reason), reason);
}

static const char *phone_choose_end_reason(void)
{
    if (phone_end_reason[0] != '\0') {
        return phone_end_reason;
    }

    if (phone_call_incoming && !phone_call_connected) {
        return phone_call_user_hangup ? "Rejected" : "Missed";
    }

    if (!phone_call_incoming && !phone_call_connected) {
        return phone_call_user_hangup ? "Canceled" : "Ended";
    }

    return "Ended";
}

static void phone_begin_outgoing_call(const char *number)
{
    phone_reset_call_session();
    phone_enable_call_audio();
    phone_call_incoming = false;
    phone_state_started_ms = millis();
    phone_snapshot_set_state(UI_PHONE_STATE_OUTGOING, number, "Dialing");
}

static void phone_begin_incoming_call(const char *number)
{
    if (phone_snapshot_state() == UI_PHONE_STATE_ACTIVE) {
        return;
    }

    phone_reset_call_session();
    phone_enable_call_audio();
    phone_call_incoming = true;
    phone_state_started_ms = millis();
    phone_snapshot_set_state(UI_PHONE_STATE_INCOMING, number, "Incoming");
}

static void phone_mark_active_call(const char *number)
{
    if (!phone_call_connected) {
        phone_call_connected = true;
        phone_call_start_ms = millis();
    }

    phone_state_started_ms = millis();
    phone_enable_call_audio();
    phone_snapshot_set_state(UI_PHONE_STATE_ACTIVE, number, "In Call");
}

static void phone_finalize_call(const char *result)
{
    char number[UI_PHONE_NUMBER_LEN] = {0};
    const char *direction = phone_call_incoming ? "Incoming" : "Outgoing";

    portENTER_CRITICAL(&phone_snapshot_mux);
    phone_copy_text(number, sizeof(number), phone_snapshot.current_number);
    portEXIT_CRITICAL(&phone_snapshot_mux);

    if (number[0] != '\0') {
        phone_snapshot_add_history(direction, result, number);
    }

    phone_restore_audio();
    phone_reset_call_session();
    phone_snapshot_set_state(UI_PHONE_STATE_IDLE, "", "Idle");
}

static void phone_finish_test_audio(void)
{
    if (!phone_test_audio_active) {
        return;
    }

    phone_restore_audio();
    phone_test_audio_active = false;
    phone_test_audio_ok_count = 0;
    phone_test_audio_min_done_ms = 0;
    phone_test_audio_restore_ms = 0;
    phone_snapshot_set_state(UI_PHONE_STATE_IDLE, NULL, "Idle");
}

static bool phone_queue_command(phone_cmd_type_t type, const char *number, bool enabled)
{
    if (phone_cmd_queue == NULL) {
        return false;
    }

    phone_cmd_t cmd = {};
    cmd.type = type;
    cmd.enabled = enabled;
    if (number != NULL) {
        phone_copy_text(cmd.number, sizeof(cmd.number), number);
    }
    return xQueueSend(phone_cmd_queue, &cmd, 0) == pdPASS;
}

bool phone_runtime_dial(const char *number)
{
    if (number == NULL || number[0] == '\0') {
        return false;
    }

    if (phone_snapshot_state() != UI_PHONE_STATE_IDLE || phone_test_audio_active) {
        return false;
    }

    return phone_queue_command(PHONE_CMD_DIAL, number, false);
}

bool phone_runtime_answer(void)
{
    return phone_snapshot_state() == UI_PHONE_STATE_INCOMING &&
           phone_queue_command(PHONE_CMD_ANSWER, NULL, false);
}

bool phone_runtime_hang_up(void)
{
    ui_phone_state_t state = phone_snapshot_state();
    if (state != UI_PHONE_STATE_INCOMING &&
        state != UI_PHONE_STATE_OUTGOING &&
        state != UI_PHONE_STATE_ACTIVE) {
        return false;
    }

    return phone_queue_command(PHONE_CMD_HANGUP, NULL, false);
}

bool phone_runtime_play_test_digits(void)
{
    if (phone_snapshot_state() != UI_PHONE_STATE_IDLE || phone_test_audio_active) {
        return false;
    }

    phone_test_audio_active = true;
    phone_test_audio_ok_count = 0;
    phone_snapshot_set_state(UI_PHONE_STATE_IDLE, NULL, "Testing 0-9");
    if (!phone_queue_command(PHONE_CMD_TEST_DIGITS, NULL, false)) {
        phone_snapshot_set_state(UI_PHONE_STATE_IDLE, NULL, "Idle");
        phone_test_audio_active = false;
        phone_test_audio_ok_count = 0;
        return false;
    }
    return true;
}

void phone_runtime_set_debug_passthrough(bool enabled)
{
    if (!enabled) {
        phone_snapshot_set_debug(false);
    }

    if (phone_cmd_queue == NULL) {
        return;
    }

    if (!enabled || (phone_snapshot_state() == UI_PHONE_STATE_IDLE && !phone_test_audio_active)) {
        (void)phone_queue_command(PHONE_CMD_DEBUG, NULL, enabled);
    }
}

bool phone_runtime_get_snapshot(ui_phone_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        return false;
    }

    portENTER_CRITICAL(&phone_snapshot_mux);
    *snapshot = phone_snapshot;
    if (phone_snapshot.state == UI_PHONE_STATE_ACTIVE && phone_call_start_ms > 0U) {
        snapshot->call_duration_sec = (millis() - phone_call_start_ms) / 1000U;
    } else {
        snapshot->call_duration_sec = 0;
    }
    portEXIT_CRITICAL(&phone_snapshot_mux);

    if (snapshot->status[0] == '\0') {
        phone_copy_text(snapshot->status, sizeof(snapshot->status),
                        snapshot->state == UI_PHONE_STATE_UNAVAILABLE ? "A7682E Failed" : "Idle");
    }
    return true;
}

static void phone_send_command_line(const char *line)
{
    if (line == NULL || line[0] == '\0') {
        return;
    }

    SerialAT.print(line);
    SerialAT.print("\r\n");
}

static void phone_send_dial_command(const char *number)
{
    SerialAT.print("ATD");
    SerialAT.print(number);
    SerialAT.print(";\r\n");
}

static void phone_handle_clip_line(const char *line)
{
    const char *first_quote = strchr(line, '"');
    if (first_quote == NULL) {
        return;
    }

    const char *second_quote = strchr(first_quote + 1, '"');
    if (second_quote == NULL || second_quote <= first_quote + 1) {
        return;
    }

    char number[UI_PHONE_NUMBER_LEN] = {0};
    size_t copy_len = (size_t)(second_quote - (first_quote + 1));
    if (copy_len >= sizeof(number)) {
        copy_len = sizeof(number) - 1U;
    }
    memcpy(number, first_quote + 1, copy_len);
    number[copy_len] = '\0';

    if (number[0] == '\0') {
        return;
    }

    phone_call_incoming = true;
    phone_snapshot_set_number(number);
    if (phone_snapshot_state() != UI_PHONE_STATE_ACTIVE) {
        phone_snapshot_set_state(UI_PHONE_STATE_INCOMING, number, "Incoming");
    }
}

static void phone_handle_clcc_line(const char *line)
{
    int idx = 0;
    int dir = 0;
    int stat = 0;
    int mode = 0;
    int mpty = 0;
    int type = 0;
    char number[UI_PHONE_NUMBER_LEN] = {0};
    int matched = sscanf(line, "+CLCC: %d,%d,%d,%d,%d,\"%31[^\"]\",%d",
                         &idx, &dir, &stat, &mode, &mpty, number, &type);

    if (matched < 5) {
        return;
    }

    phone_clcc_lines_seen = true;
    if (matched >= 6 && number[0] != '\0') {
        phone_snapshot_set_number(number);
    }

    switch (stat) {
    case 0:
        phone_call_incoming = (dir == 1);
        phone_mark_active_call(number[0] != '\0' ? number : NULL);
        break;
    case 2:
    case 3:
        phone_call_incoming = false;
        phone_state_started_ms = millis();
        phone_snapshot_set_state(UI_PHONE_STATE_OUTGOING,
                                 number[0] != '\0' ? number : NULL,
                                 stat == 2 ? "Dialing" : "Calling");
        break;
    case 4:
    case 5:
        phone_call_incoming = true;
        phone_state_started_ms = millis();
        phone_snapshot_set_state(UI_PHONE_STATE_INCOMING,
                                 number[0] != '\0' ? number : NULL,
                                 "Incoming");
        break;
    default:
        break;
    }
}

static void phone_handle_command(const phone_cmd_t *cmd)
{
    if (cmd == NULL) {
        return;
    }

    switch (cmd->type) {
    case PHONE_CMD_DIAL:
        phone_begin_outgoing_call(cmd->number);
        phone_send_dial_command(cmd->number);
        break;
    case PHONE_CMD_ANSWER:
        phone_enable_call_audio();
        phone_state_started_ms = millis();
        phone_snapshot_set_state(UI_PHONE_STATE_INCOMING, NULL, "Answering");
        phone_send_command_line("ATA");
        break;
    case PHONE_CMD_HANGUP:
        phone_call_user_hangup = true;
        phone_send_command_line("AT+CHUP");
        break;
    case PHONE_CMD_TEST_DIGITS:
        phone_enable_call_audio();
        phone_test_audio_ok_count = 0;
        phone_test_audio_min_done_ms = millis() + 1800U;
        phone_test_audio_restore_ms = millis() + 3500U;
        phone_snapshot_set_state(UI_PHONE_STATE_IDLE, NULL, "Testing 0-9");
        phone_send_command_line("AT+CTTSPARAM=1,3,0,1,1,0");
        vTaskDelay(pdMS_TO_TICKS(100));
        phone_send_command_line("AT+CTTS=2,\"0 1 2 3 4 5 6 7 8 9\"");
        break;
    case PHONE_CMD_DEBUG:
        phone_snapshot_set_debug(cmd->enabled);
        break;
    default:
        break;
    }
}

static void phone_handle_no_carrier(void)
{
    phone_finalize_call(phone_choose_end_reason());
}

static void phone_handle_serial_line(const char *line)
{
    if (line == NULL || line[0] == '\0') {
        return;
    }

    if (strcmp(line, "RING") == 0) {
        phone_begin_incoming_call(NULL);
        return;
    }

    if (strncmp(line, "+CLIP:", 6) == 0) {
        phone_handle_clip_line(line);
        return;
    }

    if (strncmp(line, "+CLCC:", 6) == 0) {
        phone_handle_clcc_line(line);
        return;
    }

    if (strcmp(line, "BUSY") == 0) {
        phone_set_end_reason("Busy");
        phone_finalize_call("Busy");
        return;
    }

    if (strcmp(line, "NO ANSWER") == 0) {
        phone_set_end_reason("No Answer");
        phone_finalize_call("No Answer");
        return;
    }

    if (strcmp(line, "NO CARRIER") == 0) {
        phone_handle_no_carrier();
        return;
    }

    if (phone_test_audio_active) {
        if (strncmp(line, "+CTTS:", 6) == 0) {
            phone_finish_test_audio();
            return;
        }

        if (strcmp(line, "OK") == 0) {
            if (phone_test_audio_ok_count < UINT8_MAX) {
                phone_test_audio_ok_count++;
            }
            return;
        }

        if (strstr(line, "ERROR") != NULL) {
            phone_finish_test_audio();
            return;
        }
    }

    if (strcmp(line, "OK") == 0 && phone_clcc_query_pending) {
        phone_clcc_query_pending = false;

        if (!phone_clcc_lines_seen &&
            phone_snapshot_state() != UI_PHONE_STATE_IDLE &&
            phone_state_started_ms > 0U &&
            (millis() - phone_state_started_ms) > 3000U) {
            phone_finalize_call(phone_choose_end_reason());
        }

        phone_clcc_lines_seen = false;
        return;
    }
}

static void a7682_task(void *param)
{
    (void)param;

    char line_buf[128] = {0};
    size_t line_len = 0;
    uint32_t last_clcc_ms = 0;

    while (1) {
        phone_cmd_t cmd = {};
        if (xQueueReceive(phone_cmd_queue, &cmd, pdMS_TO_TICKS(20)) == pdPASS) {
            phone_handle_command(&cmd);
        }

        bool debug_mode = false;
        portENTER_CRITICAL(&phone_snapshot_mux);
        debug_mode = phone_snapshot.debug_passthrough;
        portEXIT_CRITICAL(&phone_snapshot_mux);

        if (debug_mode) {
            while (SerialAT.available()) {
                SerialMon.write(SerialAT.read());
            }
            while (SerialMon.available()) {
                SerialAT.write(SerialMon.read());
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        while (SerialAT.available()) {
            char ch = (char)SerialAT.read();
            if (ch == '\r') {
                continue;
            }
            if (ch == '\n') {
                if (line_len > 0U) {
                    line_buf[line_len] = '\0';
                    phone_handle_serial_line(line_buf);
                    line_len = 0;
                }
                continue;
            }

            if (line_len < (sizeof(line_buf) - 1U)) {
                line_buf[line_len++] = ch;
            } else {
                line_len = 0;
            }
        }

        ui_phone_state_t state = phone_snapshot_state();
        if ((state == UI_PHONE_STATE_OUTGOING ||
             state == UI_PHONE_STATE_INCOMING ||
             state == UI_PHONE_STATE_ACTIVE) &&
            (millis() - last_clcc_ms) > 1000U &&
            !phone_clcc_query_pending) {
            phone_clcc_query_pending = true;
            phone_clcc_lines_seen = false;
            phone_send_command_line("AT+CLCC");
            last_clcc_ms = millis();
        }

        if (phone_test_audio_active &&
            phone_test_audio_restore_ms > 0U &&
            state == UI_PHONE_STATE_IDLE &&
            ((phone_test_audio_ok_count >= 2U && millis() >= phone_test_audio_min_done_ms) ||
             millis() >= phone_test_audio_restore_ms)) {
            phone_finish_test_audio();
        }
    }
}

static bool A7682E_init(void)
{
    Serial.println("Place your board outside to catch satelite signal");
    phone_snapshot_reset(false);

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

    if (retry >= retry_cnt) {
        phone_snapshot_reset(false);
        return false;
    }

    modem.sendAT("+CLIP=1");
    modem.waitResponse(1000);

    phone_cmd_queue = xQueueCreate(6, sizeof(phone_cmd_t));
    if (phone_cmd_queue == NULL) {
        Serial.println("[A7682E] Command queue init failed");
        phone_snapshot_reset(false);
        return false;
    }

    phone_snapshot_reset(true);
    if (xTaskCreate(a7682_task, "a7682_handle", 1024 * 4, NULL, A7682E_PRIORITY, &a7682_handle) != pdPASS) {
        Serial.println("[A7682E] Runtime task init failed");
        vQueueDelete(phone_cmd_queue);
        phone_cmd_queue = NULL;
        phone_snapshot_reset(false);
        return false;
    }

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
    setCpuFrequencyMhz(240);
    Serial.begin(115200);
#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
    // Keep the UI responsive after the USB serial monitor is closed.
    Serial.setTxTimeoutMs(0);
#endif

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
            BOARD_XL9555_04_LORA_SEL,
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
        // LOW --- external antenna 
        // HIGH --- internal antenna (default)
        // xl9555_io.pinMode(BOARD_XL9555_04_LORA_SEL, OUTPUT);
        // xl9555_io.digitalWrite(BOARD_XL9555_04_LORA_SEL, LOW);

        // Ensure touch controller exits any "half-powered" state after a power cut:
        // provide a deterministic reset pulse via XL9555 IO07 before touch init.
        xl9555_io.pinMode(BOARD_XL9555_07_TOUCH_RST, OUTPUT);
        xl9555_io.digitalWrite(BOARD_XL9555_07_TOUCH_RST, LOW);
        delay(20);
        xl9555_io.digitalWrite(BOARD_XL9555_07_TOUCH_RST, HIGH);
        delay(60);
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
        // while (1) {
            Serial.println("Failed to find DRV2605 - Motor drive");
            // delay(1000);
            Serial.println("Failed to find DRV2605 - Motor drive");
            Serial.println("Failed to find DRV2605 - Motor drive");
        // }
    } else {
        motor_drv.selectLibrary(1);
        motor_drv.setMode(DRV2605_MODE_INTTRIG);
        motor_drv.setWaveform(0, 13); // play effect
        motor_drv.setWaveform(1, 0);      // end waveform
        motor_drv.go();
    }

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
    // touch.setPins(-1, BOARD_TOUCH_INT); // touch_rst connect the IO07 of the chip XL9555
    peri_init_st[E_PERI_INK_SCREEN] = ink_screen_init();
    peri_init_st[E_PERI_LORA]       = lora_init();
    // peri_init_st[E_PERI_TOUCH]      = touch.begin(Wire, BOARD_I2C_ADDR_TOUCH, BOARD_TOUCH_SDA, BOARD_TOUCH_SCL);
    peri_init_st[E_PERI_KYEPAD]     = keypad_init(BOARD_I2C_ADDR_KEYBOARD);
    peri_init_st[E_PERI_BQ25896]    = bq25896_init();
    peri_init_st[E_PERI_BQ27220]    = bq27220_init();
    peri_init_st[E_PERI_SD]         = sd_care_init();
    peri_init_st[E_PERI_GPS]        = gps_init();
    peri_init_st[E_PERI_BHI260AP]   = BHI260AP_init();
    peri_init_st[E_PERI_A7682E]     = A7682E_init();

    peri_init_st[E_PERI_TOUCH] = hyn_touch_init();

    lvgl_init();

    ui_deckpro_entry();

    disp_full_refr();

    // close backlight
    analogWrite(BOARD_EPD_BL, 0);
    analogWrite(BOARD_KEYBOARD_LED, 0);

}

uint32_t tick = 0;


void loop()
{
    lv_task_handler();
    keypad_loop();

    if (disp_partial_refr_until_ms > 0U &&
        millis() >= disp_partial_refr_until_ms) {
        lv_disp_t *disp = factory_disp ? factory_disp : lv_disp_get_default();
        if (disp != NULL && disp->driver != NULL) {
            disp->driver->full_refresh = 1;
        }
        disp_partial_refr_until_ms = 0U;
    }

    delay(1);
}

/*********************************************************************************
 *                              GLOBAL PROTOTYPES
 * *******************************************************************************/
void disp_full_refr(void)
{
    lv_disp_t *disp = factory_disp ? factory_disp : lv_disp_get_default();
    if (disp != NULL && disp->driver != NULL) {
        disp->driver->full_refresh = 1;
    }
    disp_partial_refr_until_ms = 0U;
    disp_refr_mode = DISP_REFR_MODE_FULL;
}

void disp_partial_refr_for(uint32_t duration_ms)
{
    lv_disp_t *disp = factory_disp ? factory_disp : lv_disp_get_default();
    if (disp != NULL && disp->driver != NULL) {
        disp->driver->full_refresh = 0;
    }
    disp_refr_mode = DISP_REFR_MODE_PART;
    disp_partial_refr_until_ms = millis() + duration_ms;
}


