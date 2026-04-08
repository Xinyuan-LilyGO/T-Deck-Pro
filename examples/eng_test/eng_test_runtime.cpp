#include "eng_test_app.h"

#include <Fonts/FreeMonoBold9pt7b.h>
#include <cmath>

RTC_DATA_ATTR PersistedState g_rtc_state = {};

TinyGsm modem(SerialAT);
TaskHandle_t a7682_handle = nullptr;

XPowersPPM PPM;
BQ27220 bq27220;
ExtensionIOXL9555 xl9555_io;
Adafruit_DRV2605 motor_drv;
EspCodec codec;

GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(
    GxEPD2_310_GDEQ031T10(BOARD_EPD_CS, BOARD_EPD_DC, BOARD_EPD_RST, BOARD_EPD_BUSY));

PageKind g_current_page = PAGE_MODULE_SELECT;
TestMode g_current_mode = TEST_MODE_QUICK;
int g_current_test = -1;
int g_focus_count = 0;
int g_focus_index = 0;
int g_selected_count = 0;
uint32_t g_full_refresh_request = 1;
bool g_resume_from_sleep = false;
uint8_t *g_decode_buffer = nullptr;
lv_timer_t *g_app_timer = nullptr;
lv_obj_t *g_focus_widgets[FOCUS_MAX] = {};
lv_obj_t *g_test_info_label = nullptr;
lv_obj_t *g_test_hint_label = nullptr;
lv_obj_t *g_test_status_label = nullptr;
lv_obj_t *g_page_subtitle_label = nullptr;
lv_obj_t *g_module_buttons[TEST_COUNT] = {};
lv_obj_t *g_shutdown_info = nullptr;
uint32_t g_auto_advance_at = 0;

DisplayCtx g_display_ctx = {};
TouchCtx g_touch_ctx = {};
KeypadCtx g_keypad_ctx = {};
GpsCtx g_gps_ctx = {};
AudioCtx g_audio_ctx = {};
BatteryCtx g_battery_ctx = {};
MotorCtx g_motor_ctx = {};
ImuCtx g_imu_ctx = {};
WifiCtx g_wifi_ctx = {};
SdCtx g_sd_ctx = {};
ModemCtx g_modem_ctx = {};
LoRaCtx g_lora_ctx = {};
BleCtx g_ble_ctx = {};
SleepCtx g_sleep_ctx = {};

bool g_touch_ready = false;
bool g_keypad_ready = false;
bool g_display_ready = false;
bool g_spi_ready = false;

static void pulse_touch_reset()
{
    set_power_pin(BOARD_XL9555_07_TOUCH_RST, false);
    delay(20);
    set_power_pin(BOARD_XL9555_07_TOUCH_RST, true);
    delay(60);
}

const char *status_text(TestStatus status)
{
    switch (status) {
        case TEST_STATUS_RUNNING: return "RUNNING";
        case TEST_STATUS_PASS: return "PASS";
        case TEST_STATUS_FAIL: return "FAIL";
        case TEST_STATUS_SKIP: return "SKIP";
        case TEST_STATUS_NOT_RUN:
        default: return "NOT_RUN";
    }
}

const char *status_badge(TestStatus status)
{
    switch (status) {
        case TEST_STATUS_RUNNING: return "[..]";
        case TEST_STATUS_PASS: return "[OK]";
        case TEST_STATUS_FAIL: return "[!!]";
        case TEST_STATUS_SKIP: return "[--]";
        case TEST_STATUS_NOT_RUN:
        default: return "[  ]";
    }
}

void request_full_refresh()
{
    g_full_refresh_request = 1;
}

void shared_spi_deselect_all()
{
    pinMode(BOARD_LORA_CS, OUTPUT);
    digitalWrite(BOARD_LORA_CS, HIGH);
    pinMode(BOARD_SD_CS, OUTPUT);
    digitalWrite(BOARD_SD_CS, HIGH);
    pinMode(BOARD_EPD_CS, OUTPUT);
    digitalWrite(BOARD_EPD_CS, HIGH);
}

void set_power_pin(uint8_t pin, bool high)
{
    xl9555_io.pinMode(pin, OUTPUT);
    xl9555_io.digitalWrite(pin, high ? HIGH : LOW);
}

void route_audio_to_es8311(bool enable_amp)
{
    set_power_pin(BOARD_XL9555_12_AUDIO_SEL, LOW);
    set_power_pin(BOARD_XL9555_06_AMPLIFIER, enable_amp);
}

void route_audio_to_modem(bool enable_amp)
{
    set_power_pin(BOARD_XL9555_12_AUDIO_SEL, HIGH);
    set_power_pin(BOARD_XL9555_06_AMPLIFIER, enable_amp);
}

static uint32_t pack_lvgl_area_to_epd(const lv_color_t *color_p, uint32_t w, uint32_t h)
{
    uint32_t idx = 0;
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; x += 8U) {
            uint8_t byte = 0;
            for (uint32_t bit = 0; bit < 8U; ++bit) {
                uint32_t src_x = x + bit;
                if (src_x < w && color_p[(y * w) + src_x].full) {
                    byte |= (0x80U >> bit);
                }
            }
            g_decode_buffer[idx++] = byte;
        }
    }
    return idx;
}

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    const uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    const uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
    (void)pack_lvgl_area_to_epd(color_p, w, h);

    shared_spi_deselect_all();
    if (g_full_refresh_request) {
        display.setFullWindow();
    } else {
        display.setPartialWindow(area->x1, area->y1, w, h);
    }

    display.firstPage();
    do {
        display.drawInvertedBitmap(area->x1, area->y1, g_decode_buffer, w, h, GxEPD_BLACK);
    } while (display.nextPage());

    g_full_refresh_request = 0;
    lv_disp_flush_ready(disp_drv);
}

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    LV_UNUSED(indev_drv);
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;
    int16_t x = 0;
    int16_t y = 0;
    uint8_t touched = g_touch_ready ? hyn_touch_get_point(&x, &y, 1) : 0;
    if (touched) {
        last_x = x;
        last_y = y;
        g_touch_ctx.last_x = x;
        g_touch_ctx.last_y = y;
        g_touch_ctx.point_count = touched;
        g_touch_ctx.has_point = true;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
    data->point.x = last_x;
    data->point.y = last_y;
}

static bool init_epd_panel()
{
    shared_spi_deselect_all();
    display.init(115200, true, 2, false);
    display.setRotation(0);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(18, 40);
        display.print("T-Deck-Pro MAX");
        display.setCursor(18, 65);
        display.print("Engineering Test");
    } while (display.nextPage());
    return true;
}

static void init_lvgl_runtime()
{
    lv_init();

    lv_color_t *buf1 = (lv_color_t *)ps_calloc(sizeof(lv_color_t), DISP_BUF_SIZE);
    lv_color_t *buf2 = (lv_color_t *)ps_calloc(sizeof(lv_color_t), DISP_BUF_SIZE);
    g_decode_buffer = (uint8_t *)ps_calloc(sizeof(uint8_t), DISP_BUF_SIZE);

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_HOR_SIZE * LCD_VER_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_HOR_SIZE;
    disp_drv.ver_res = LCD_VER_SIZE;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 1;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    lv_theme_t *theme = lv_theme_mono_init(disp, false, LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, theme);
}

bool init_spi_if_needed()
{
    if (g_spi_ready) {
        return true;
    }
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
    g_spi_ready = true;
    return true;
}

bool init_display_stack()
{
    if (g_display_ready) {
        return true;
    }
    g_display_ready = init_epd_panel();
    init_lvgl_runtime();
    return g_display_ready;
}

bool init_power_manager()
{
    Wire.beginTransmission(BQ25896_SLAVE_ADDRESS);
    int ret = Wire.endTransmission();
    if (ret == 0) {
        g_battery_ctx.charger_ready = PPM.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, BQ25896_SLAVE_ADDRESS);
        if (g_battery_ctx.charger_ready) {
            PPM.setChargeTargetVoltage(4288);
            PPM.setChargerConstantCurr(1024);
            PPM.enableMeasure();
        }
    } else {
        g_battery_ctx.charger_ready = false;
    }

    g_battery_ctx.gauge_ready = bq27220.init();
    return g_battery_ctx.charger_ready || g_battery_ctx.gauge_ready;
}

bool init_keypad_if_needed()
{
    if (g_keypad_ready) {
        return true;
    }
    set_power_pin(BOARD_XL9555_11_KEY_RST, false);
    delay(10);
    set_power_pin(BOARD_XL9555_11_KEY_RST, true);
    delay(30);
    g_keypad_ready = keypad_init(BOARD_I2C_ADDR_KEYBOARD);
    return g_keypad_ready;
}

bool init_touch_if_needed()
{
    if (g_touch_ready) {
        return true;
    }
    pulse_touch_reset();
    g_touch_ready = hyn_touch_init();
    return g_touch_ready;
}

bool init_gps_if_needed()
{
    if (g_gps_ctx.initialized) {
        return true;
    }
    set_power_pin(BOARD_XL9555_02_GPS_EN, true);
    delay(10);
    g_gps_ctx.initialized = gps_init();
    return g_gps_ctx.initialized;
}

bool init_codec_if_needed()
{
    if (g_audio_ctx.initialized) {
        return true;
    }
    route_audio_to_es8311(true);
    set_power_pin(BOARD_XL9555_00_6609_EN, true);
    set_power_pin(BOARD_XL9555_10_PWEKEY_EN, true);
    codec.setPins(BOARD_ES8311_MCLK, BOARD_ES8311_SCLK, BOARD_ES8311_LRCK, BOARD_ES8311_ASDOUT, BOARD_ES8311_DSDIN);
    g_audio_ctx.initialized = codec.begin(Wire, BOARD_I2C_ADDR_ES8311, CODEC_TYPE_ES8311);
    if (g_audio_ctx.initialized) {
        codec.setVolume(50);
    }
    return g_audio_ctx.initialized;
}

bool init_motor_if_needed()
{
    if (g_motor_ctx.initialized) {
        return true;
    }
    set_power_pin(BOARD_XL9555_05_MOTOR_EN, true);
    delay(10);
    g_motor_ctx.initialized = motor_drv.begin();
    if (g_motor_ctx.initialized) {
        motor_drv.selectLibrary(1);
        motor_drv.setMode(DRV2605_MODE_INTTRIG);
    }
    return g_motor_ctx.initialized;
}

bool init_imu_if_needed()
{
    if (g_imu_ctx.initialized) {
        return true;
    }
    set_power_pin(BOARD_XL9555_03_1V8_EN, true);
    delay(10);
    g_imu_ctx.initialized = BHI260AP_init();
    return g_imu_ctx.initialized;
}

bool init_sd_if_needed()
{
    if (g_sd_ctx.mounted) {
        return true;
    }
    init_spi_if_needed();
    shared_spi_deselect_all();
    g_sd_ctx.mounted = SD.begin(BOARD_SD_CS);
    if (g_sd_ctx.mounted) {
        g_sd_ctx.total_mb = SD.totalBytes() / (1024ULL * 1024ULL);
        g_sd_ctx.used_mb = SD.usedBytes() / (1024ULL * 1024ULL);
    }
    return g_sd_ctx.mounted;
}

static void modem_bridge_task(void *param)
{
    LV_UNUSED(param);
    vTaskSuspend(a7682_handle);
    while (true) {
        while (SerialAT.available()) {
            Serial.write(SerialAT.read());
        }
        while (Serial.available()) {
            SerialAT.write(Serial.read());
        }
        delay(1);
    }
}

bool init_modem_if_needed()
{
    if (g_modem_ctx.initialized) {
        return true;
    }

    set_power_pin(BOARD_XL9555_00_6609_EN, true);
    route_audio_to_modem(false);
    SerialAT.begin(115200, SERIAL_8N1, BOARD_A7682E_TXD, BOARD_A7682E_RXD);

    set_power_pin(BOARD_XL9555_10_PWEKEY_EN, false);
    delay(10);
    set_power_pin(BOARD_XL9555_10_PWEKEY_EN, true);
    delay(50);
    set_power_pin(BOARD_XL9555_10_PWEKEY_EN, false);
    delay(20);

    for (int retry = 0; retry < 5; ++retry) {
        if (modem.testAT(1200)) {
            g_modem_ctx.initialized = true;
            break;
        }
        delay(300);
    }

    if (g_modem_ctx.initialized) {
        g_modem_ctx.signal_quality = modem.getSignalQuality();
        if (!a7682_handle) {
            xTaskCreate(modem_bridge_task, "a7682_bridge", 1024 * 3, nullptr, A7682E_PRIORITY, &a7682_handle);
        }
    } else {
        SerialAT.end();
    }
    return g_modem_ctx.initialized;
}

bool init_lora_if_needed()
{
    if (g_lora_ctx.initialized) {
        return true;
    }
    init_spi_if_needed();
    set_power_pin(BOARD_XL9555_01_LORA_EN, true);
    set_power_pin(BOARD_XL9555_04_LORA_SEL, true);
    shared_spi_deselect_all();
    g_lora_ctx.initialized = lora_init();
    return g_lora_ctx.initialized;
}

void stop_ble_activity()
{
    if (!BLEDevice::getInitialized()) {
        return;
    }
    BLEDevice::stopAdvertising();
    BLEScan *scan = BLEDevice::getScan();
    if (scan) {
        scan->clearResults();
    }
    g_ble_ctx.advertising = false;
}

bool init_ble_if_needed()
{
    if (g_ble_ctx.initialized) {
        return true;
    }
    if (!BLEDevice::getInitialized()) {
        BLEDevice::init("TDeckPro-ENG");
    } else {
        stop_ble_activity();
    }
    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    BLEAdvertisementData adv_data;
    adv_data.setName("TDeckPro-ENG");
    adv_data.setManufacturerData("ENG");
    advertising->setScanResponse(false);
    advertising->setAdvertisementData(adv_data);
    advertising->start();
    g_ble_ctx.initialized = true;
    g_ble_ctx.advertising = true;
    return true;
}

void persist_for_sleep()
{
    g_rtc_state.magic = RTC_MAGIC;
    g_rtc_state.selected_mask = 0;
    for (int i = 0; i < TEST_COUNT; ++i) {
        if (g_results[i].enabled) {
            g_rtc_state.selected_mask |= (1UL << i);
        }
        g_rtc_state.status[i] = (uint8_t)g_results[i].status;
    }
    g_rtc_state.sleep_pending = 1;
}

static void power_down_before_sleep()
{
    if (g_ble_ctx.initialized && BLEDevice::getInitialized()) {
        stop_ble_activity();
    }
    WiFi.mode(WIFI_OFF);

    if (g_lora_ctx.initialized) {
        lora_sleep();
    }
    if (g_touch_ready) {
        hyn_sleep();
    }
    motor_drv.stop();

    SerialAT.end();
    SerialGPS.end();
    SPI.end();
    Wire.end();

    const uint8_t off_pins[] = {
        BOARD_XL9555_00_6609_EN,
        BOARD_XL9555_01_LORA_EN,
        BOARD_XL9555_02_GPS_EN,
        BOARD_XL9555_03_1V8_EN,
        BOARD_XL9555_05_MOTOR_EN,
        BOARD_XL9555_06_AMPLIFIER,
        BOARD_XL9555_10_PWEKEY_EN,
    };
    for (uint8_t pin : off_pins) {
        set_power_pin(pin, false);
        delay(1);
    }

    const uint8_t input_pins[] = {
        BOARD_XL9555_04_LORA_SEL,
        BOARD_XL9555_07_TOUCH_RST,
        BOARD_XL9555_11_KEY_RST,
        BOARD_XL9555_12_AUDIO_SEL,
    };
    for (uint8_t pin : input_pins) {
        xl9555_io.pinMode(pin, INPUT);
    }

    const uint8_t gpio_inputs[] = {
        BOARD_KEYBOARD_INT, BOARD_KEYBOARD_LED, BOARD_TOUCH_INT, BOARD_GYROSCOPDE_INT,
        BOARD_EPD_BL, BOARD_EPD_DC, BOARD_EPD_CS, BOARD_EPD_BUSY, BOARD_EPD_RST,
        BOARD_SD_CS, BOARD_LORA_CS, BOARD_LORA_BUSY, BOARD_LORA_RST, BOARD_LORA_INT,
        BOARD_GPS_RXD, BOARD_GPS_TXD, BOARD_GPS_PPS,
        BOARD_A7682E_RI, BOARD_A7682E_ITR, BOARD_A7682E_RXD, BOARD_A7682E_TXD,
        BOARD_BOOT_PIN, BOARD_I2C_SCL, BOARD_I2C_SDA, BOARD_SPI_SCK, BOARD_SPI_MOSI, BOARD_SPI_MISO,
    };
    for (uint8_t pin : gpio_inputs) {
        gpio_reset_pin((gpio_num_t)pin);
        pinMode(pin, INPUT);
    }

    const uint8_t gpio_low[] = {
        BOARD_ES8311_MCLK, BOARD_ES8311_SCLK, BOARD_ES8311_ASDOUT, BOARD_ES8311_LRCK, BOARD_ES8311_DSDIN,
    };
    for (uint8_t pin : gpio_low) {
        gpio_reset_pin((gpio_num_t)pin);
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
}

void enter_deep_sleep_now()
{
    persist_for_sleep();
    power_down_before_sleep();
    Serial.flush();
    esp_sleep_enable_ext1_wakeup((1ULL << BOARD_BOOT_PIN), ESP_EXT1_WAKEUP_ANY_LOW);
    esp_deep_sleep_start();
}

void boot_resume_if_needed()
{
    if (g_rtc_state.magic != RTC_MAGIC || !g_rtc_state.sleep_pending) {
        return;
    }
    g_resume_from_sleep = true;
    for (int i = 0; i < TEST_COUNT; ++i) {
        g_results[i].enabled = ((g_rtc_state.selected_mask >> i) & 0x1U) != 0U;
        g_results[i].status = (TestStatus)g_rtc_state.status[i];
        g_results[i].note[0] = '\0';
    }
    g_results[TEST_SLEEP].enabled = true;
    g_results[TEST_SLEEP].status = TEST_STATUS_PASS;
    strncpy(g_results[TEST_SLEEP].note, "woke from deep sleep", sizeof(g_results[TEST_SLEEP].note) - 1);
    g_results[TEST_SLEEP].note[sizeof(g_results[TEST_SLEEP].note) - 1] = '\0';
    update_selected_count();

    g_rtc_state.magic = 0;
    g_rtc_state.sleep_pending = 0;
}

void quick_auto_advance()
{
    if (g_current_mode == TEST_MODE_QUICK) {
        g_auto_advance_at = millis() + 700;
    }
}

void update_selected_count()
{
    g_selected_count = 0;
    for (int i = 0; i < TEST_COUNT; ++i) {
        if (is_quick_test_case(i) && g_results[i].enabled) {
            ++g_selected_count;
        }
    }
}

int quick_test_count()
{
    int count = 0;
    for (int i = 0; i < TEST_COUNT; ++i) {
        if (is_quick_test_case(i)) {
            ++count;
        }
    }
    return count;
}

int selected_test_count()
{
    return g_selected_count;
}

int selected_progress_of(int test_id)
{
    int progress = 0;
    for (int i = 0; i <= test_id; ++i) {
        if (is_quick_test_case(i) && g_results[i].enabled) {
            ++progress;
        }
    }
    return progress;
}
