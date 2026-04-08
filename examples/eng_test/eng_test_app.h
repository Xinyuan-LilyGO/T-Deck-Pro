#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>

#include <BLEAdvertising.h>
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <GxEPD2_BW.h>
#include <RadioLib.h>
#include <TinyGPS++.h>
#include <TinyGsmClient.h>
#include <Adafruit_TCA8418.h>
#include <driver/gpio.h>
#include <esp_bt.h>
#include <esp_sleep.h>
#include <lvgl.h>

#define XPOWERS_CHIP_BQ25896
#include <XPowersLib.h>

#include "Adafruit_DRV2605.h"
#include "ExtensionIOXL9555.hpp"
#include "bq27220.h"
#include "esp_codec.h"

#include "../factory/peripheral.h"
#include "../factory/utilities.h"

enum TestCaseId : uint8_t {
    TEST_EPD = 0,
    TEST_TOUCH,
    TEST_KEYPAD,
    TEST_GPS,
    TEST_AUDIO,
    TEST_BATTERY,
    TEST_MOTOR,
    TEST_IMU,
    TEST_WIFI,
    TEST_SD,
    TEST_MODEM,
    TEST_LORA,
    TEST_BLE,
    TEST_SLEEP,
    TEST_COUNT,
};

enum TestStatus : uint8_t {
    TEST_STATUS_NOT_RUN = 0,
    TEST_STATUS_RUNNING,
    TEST_STATUS_PASS,
    TEST_STATUS_FAIL,
    TEST_STATUS_SKIP,
};

enum TestMode : uint8_t {
    TEST_MODE_QUICK = 0,
    TEST_MODE_DEEP,
};

enum PageKind : uint8_t {
    PAGE_MODULE_SELECT = 0,
    PAGE_PRECHECK,
    PAGE_TEST,
    PAGE_SUMMARY,
    PAGE_DEEP_MENU,
    PAGE_RESUME,
    PAGE_SHUTDOWN,
};

struct TestResult {
    bool enabled;
    TestStatus status;
    char note[96];
};

typedef bool (*TestSetupFn)(TestMode mode);
typedef void (*TestPollFn)(TestMode mode);
typedef void (*TestTeardownFn)(TestMode mode);

struct TestCaseDescriptor {
    TestCaseId id;
    const char *name_cn;
    const char *name_en;
    const char *summary_cn;
    bool supports_deep;
    TestSetupFn setup;
    TestPollFn poll;
    TestTeardownFn teardown;
};

struct PersistedState {
    uint32_t magic;
    uint32_t selected_mask;
    uint8_t status[TEST_COUNT];
    uint8_t sleep_pending;
};

struct DisplayCtx {
    uint8_t phase;
    uint32_t last_tick;
    uint32_t partial_count;
    lv_obj_t *box;
};

struct TouchCtx {
    int16_t last_x;
    int16_t last_y;
    uint8_t point_count;
    bool has_point;
    bool key_down[3];
    bool key_hit[3];
    lv_obj_t *key_blocks[3];
    lv_obj_t *key_labels[3];
};

struct KeypadCtx {
    uint8_t quick_hits;
    uint8_t deep_hits;
    bool quick_seen[6];
    bool deep_seen[64];
};

struct GpsCtx {
    bool initialized;
    bool task_resumed;
    uint32_t start_ms;
};

struct AudioCtx {
    bool initialized;
    bool played;
    uint32_t start_ms;
};

struct BatteryCtx {
    bool charger_ready;
    bool gauge_ready;
    bool tested;
};

struct MotorCtx {
    bool initialized;
    uint8_t effect_index;
    uint32_t last_tick;
};

struct ImuCtx {
    bool initialized;
    bool baseline_ready;
    uint32_t start_ms;
    float base_x;
    float base_y;
    float base_z;
    float max_delta;
};

struct WifiCtx {
    bool scan_started;
    bool scan_done;
    int scan_count;
    uint32_t start_ms;
    String summary;
};

struct SdCtx {
    bool mounted;
    bool tested;
    uint64_t total_mb;
    uint64_t used_mb;
};

struct ModemCtx {
    bool initialized;
    bool bridge_active;
    uint32_t start_ms;
    int signal_quality;
    String summary;
};

struct LoRaCtx {
    bool initialized;
    uint32_t start_ms;
    bool receive_mode;
    String last_recv;
    int last_rssi;
};

struct BleCtx {
    bool initialized;
    bool advertising;
    bool scan_done;
    uint8_t phase;
    uint32_t start_ms;
    int scan_count;
    String summary;
};

struct SleepCtx {
    bool armed;
    uint32_t deadline_ms;
};

static constexpr uint32_t RTC_MAGIC = 0x454E4754UL;
static constexpr int BUTTON_WIDTH = 44;
static constexpr int BUTTON_HEIGHT = 28;
static constexpr int FOCUS_MAX = 20;
static constexpr int DEEP_ITEM_SHUTDOWN = 1000;

extern RTC_DATA_ATTR PersistedState g_rtc_state;

extern TinyGsm modem;
extern TaskHandle_t a7682_handle;
extern XPowersPPM PPM;
extern BQ27220 bq27220;
extern ExtensionIOXL9555 xl9555_io;
extern Adafruit_DRV2605 motor_drv;
extern EspCodec codec;
extern GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display;

extern PageKind g_current_page;
extern TestMode g_current_mode;
extern int g_current_test;
extern int g_focus_count;
extern int g_focus_index;
extern int g_selected_count;
extern uint32_t g_full_refresh_request;
extern bool g_resume_from_sleep;
extern uint8_t *g_decode_buffer;
extern lv_timer_t *g_app_timer;
extern lv_obj_t *g_focus_widgets[FOCUS_MAX];
extern lv_obj_t *g_test_info_label;
extern lv_obj_t *g_test_hint_label;
extern lv_obj_t *g_test_status_label;
extern lv_obj_t *g_page_subtitle_label;
extern lv_obj_t *g_module_buttons[TEST_COUNT];
extern lv_obj_t *g_shutdown_info;
extern uint32_t g_auto_advance_at;

extern DisplayCtx g_display_ctx;
extern TouchCtx g_touch_ctx;
extern KeypadCtx g_keypad_ctx;
extern GpsCtx g_gps_ctx;
extern AudioCtx g_audio_ctx;
extern BatteryCtx g_battery_ctx;
extern MotorCtx g_motor_ctx;
extern ImuCtx g_imu_ctx;
extern WifiCtx g_wifi_ctx;
extern SdCtx g_sd_ctx;
extern ModemCtx g_modem_ctx;
extern LoRaCtx g_lora_ctx;
extern BleCtx g_ble_ctx;
extern SleepCtx g_sleep_ctx;

extern bool g_touch_ready;
extern bool g_keypad_ready;
extern bool g_display_ready;
extern bool g_spi_ready;

extern TestResult g_results[TEST_COUNT];
extern const TestCaseDescriptor g_tests[TEST_COUNT];
extern const char *kQuickKeyTargets;
extern const char *kDeepKeyTargets;
extern const int kMotorEffects[3];
extern const int kDeepMenuMap[9];

int hyn_touch_init(void);
uint8_t hyn_touch_get_point(int16_t *x_array, int16_t *y_array, uint8_t get_point);
bool hyn_touch_get_key_state(uint8_t key_id);
bool hyn_touch_get_key_seen(uint8_t key_id);
void hyn_touch_clear_key_seen(void);
void hyn_sleep(void);

const char *status_text(TestStatus status);
const char *status_badge(TestStatus status);
bool is_quick_test_case(int test_id);
void request_full_refresh();
void shared_spi_deselect_all();
bool init_spi_if_needed();
bool init_display_stack();
bool init_power_manager();
bool init_keypad_if_needed();
bool init_touch_if_needed();
bool init_gps_if_needed();
bool init_codec_if_needed();
bool init_motor_if_needed();
bool init_imu_if_needed();
bool init_sd_if_needed();
bool init_modem_if_needed();
bool init_lora_if_needed();
bool init_ble_if_needed();
void stop_ble_activity();
void route_audio_to_es8311(bool enable_amp);
void route_audio_to_modem(bool enable_amp);
void set_power_pin(uint8_t pin, bool high);
void persist_for_sleep();
void enter_deep_sleep_now();
void boot_resume_if_needed();
void set_test_status(TestCaseId id, TestStatus status, const char *note);
void quick_auto_advance();
void update_selected_count();
int quick_test_count();
int selected_test_count();
int selected_progress_of(int test_id);
String current_test_status_text();
void refresh_test_page_text();
void teardown_current_test();
void build_module_select_page();
void build_precheck_page();
void build_test_page();
void build_summary_page();
void build_deep_menu_page();
void build_resume_page();
void build_shutdown_page();
void show_next_quick_test(bool keep_current_status);
bool handle_keypad_test_key(char key);
void handle_key_event(char key);
void poll_keypad_shortcuts();
