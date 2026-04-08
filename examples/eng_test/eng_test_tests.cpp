#include "eng_test_app.h"

#include "../factory/wav_hex.h"

static bool test_setup_epd(TestMode mode);
static void test_poll_epd(TestMode mode);
static void test_teardown_epd(TestMode mode);

static bool test_setup_touch(TestMode mode);
static void test_poll_touch(TestMode mode);
static void test_teardown_touch(TestMode mode);

static bool test_setup_keypad(TestMode mode);
static void test_poll_keypad(TestMode mode);
static void test_teardown_keypad(TestMode mode);

static bool test_setup_gps(TestMode mode);
static void test_poll_gps(TestMode mode);
static void test_teardown_gps(TestMode mode);

static bool test_setup_audio(TestMode mode);
static void test_poll_audio(TestMode mode);
static void test_teardown_audio(TestMode mode);

static bool test_setup_battery(TestMode mode);
static void test_poll_battery(TestMode mode);
static void test_teardown_battery(TestMode mode);

static bool test_setup_motor(TestMode mode);
static void test_poll_motor(TestMode mode);
static void test_teardown_motor(TestMode mode);

static bool test_setup_imu(TestMode mode);
static void test_poll_imu(TestMode mode);
static void test_teardown_imu(TestMode mode);

static bool test_setup_wifi(TestMode mode);
static void test_poll_wifi(TestMode mode);
static void test_teardown_wifi(TestMode mode);

static bool test_setup_sd(TestMode mode);
static void test_poll_sd(TestMode mode);
static void test_teardown_sd(TestMode mode);

static bool test_setup_modem(TestMode mode);
static void test_poll_modem(TestMode mode);
static void test_teardown_modem(TestMode mode);

static bool test_setup_lora(TestMode mode);
static void test_poll_lora(TestMode mode);
static void test_teardown_lora(TestMode mode);

static bool test_setup_ble(TestMode mode);
static void test_poll_ble(TestMode mode);
static void test_teardown_ble(TestMode mode);

static bool test_setup_sleep(TestMode mode);
static void test_poll_sleep(TestMode mode);
static void test_teardown_sleep(TestMode mode);

TestResult g_results[TEST_COUNT] = {};

const char *kQuickKeyTargets = "qa2pEU";
const char *kDeepKeyTargets = "qwertyuiopasdfghjkl\b2zxcvbnm$EU0 SU";
const int kMotorEffects[3] = {13, 47, 82};
const int kDeepMenuMap[9] = {
    TEST_KEYPAD, TEST_GPS, TEST_AUDIO, TEST_WIFI, TEST_SD, TEST_MODEM, TEST_LORA, TEST_BLE, DEEP_ITEM_SHUTDOWN,
};

bool is_quick_test_case(int test_id)
{
    return test_id >= 0 && test_id < TEST_COUNT && test_id != TEST_SLEEP;
}

const TestCaseDescriptor g_tests[TEST_COUNT] = {
    {TEST_EPD, "E-Paper", "EPD", "Show black/white graphics and a partial refresh counter.", false, test_setup_epd, test_poll_epd, test_teardown_epd},
    {TEST_TOUCH, "Touch Panel", "Touch", "Show screen coordinates and test the 3 touch keys.", false, test_setup_touch, test_poll_touch, test_teardown_touch},
    {TEST_KEYPAD, "Keyboard", "Keypad", "Quick: press Q A ALT P ENT UP. Deep: cover every key.", true, test_setup_keypad, test_poll_keypad, test_teardown_keypad},
    {TEST_GPS, "GPS", "GPS", "Quick: check serial activity and time/satellite changes. Deep: check fix.", true, test_setup_gps, test_poll_gps, test_teardown_gps},
    {TEST_AUDIO, "Audio", "ES8311", "Play the built-in WAV and confirm sound manually.", true, test_setup_audio, test_poll_audio, test_teardown_audio},
    {TEST_BATTERY, "Battery", "Battery", "Read BQ25896 and BQ27220 data.", false, test_setup_battery, test_poll_battery, test_teardown_battery},
    {TEST_MOTOR, "Vibration Motor", "Motor", "Play 3 vibration effects in sequence.", false, test_setup_motor, test_poll_motor, test_teardown_motor},
    {TEST_IMU, "IMU", "IMU", "Tilt the device and detect attitude/gyro changes.", false, test_setup_imu, test_poll_imu, test_teardown_imu},
    {TEST_WIFI, "WiFi", "WiFi", "Run one AP scan.", true, test_setup_wifi, test_poll_wifi, test_teardown_wifi},
    {TEST_SD, "SD Card", "SD", "Mount, write, read back, and delete a temp file.", true, test_setup_sd, test_poll_sd, test_teardown_sd},
    {TEST_MODEM, "4G Modem", "A7682E", "Check AT response. Deep: enter USB serial bridge.", true, test_setup_modem, test_poll_modem, test_teardown_modem},
    {TEST_LORA, "LoRa Radio", "SX1262", "Check SX1262 init and basic TX/RX mode.", true, test_setup_lora, test_poll_lora, test_teardown_lora},
    {TEST_BLE, "Bluetooth", "BLE", "Advertise with a fixed name and run one scan.", true, test_setup_ble, test_poll_ble, test_teardown_ble},
    {TEST_SLEEP, "Sleep", "Sleep", "Save progress, enter deep sleep, and wake with BOOT.", false, test_setup_sleep, test_poll_sleep, test_teardown_sleep},
};

static void copy_note(TestCaseId id, const char *note)
{
    g_results[id].note[0] = '\0';
    if (!note) {
        return;
    }
    strncpy(g_results[id].note, note, sizeof(g_results[id].note) - 1);
    g_results[id].note[sizeof(g_results[id].note) - 1] = '\0';
}

static void log_result(const char *phase, TestCaseId id, const char *extra)
{
    Serial.printf("[ENG_TEST] %s %-8s %-10s %s\n",
                  phase,
                  g_tests[id].name_en,
                  status_text(g_results[id].status),
                  extra ? extra : "");
}

void set_test_status(TestCaseId id, TestStatus status, const char *note)
{
    g_results[id].status = status;
    copy_note(id, note);
    log_result("RESULT", id, note);
}

void teardown_current_test()
{
    if (g_current_test < 0 || g_current_test >= TEST_COUNT) {
        return;
    }
    g_tests[g_current_test].teardown(g_current_mode);
}

String current_test_status_text()
{
    String text;
    switch (g_current_test) {
        case TEST_EPD:
            text = "Phase: ";
            text += g_display_ctx.phase + 1;
            break;
        case TEST_TOUCH:
            if (g_touch_ctx.has_point) {
                text = "Touch: x=";
                text += g_touch_ctx.last_x;
                text += " y=";
                text += g_touch_ctx.last_y;
                text += " pts=";
                text += g_touch_ctx.point_count;
            } else {
                text = "Touch: waiting for coordinates";
            }
            text += "\nKeys: ";
            for (int i = 0; i < 3; ++i) {
                if (i) {
                    text += " ";
                }
                text += "K";
                text += i + 1;
                text += "=";
                text += g_touch_ctx.key_down[i] ? "DOWN" : "REL";
            }
            break;
        case TEST_KEYPAD: {
            int target = g_current_mode == TEST_MODE_QUICK ? strlen(kQuickKeyTargets) : strlen(kDeepKeyTargets);
            int hit = g_current_mode == TEST_MODE_QUICK ? g_keypad_ctx.quick_hits : g_keypad_ctx.deep_hits;
            text = "Keys detected: ";
            text += hit;
            text += "/";
            text += target;
            break;
        }
        case TEST_GPS: {
            double lat = 0;
            double lng = 0;
            double speed = 0;
            uint16_t year = 0;
            uint8_t month = 0, day = 0, hour = 0, minute = 0, second = 0;
            uint32_t sats = 0;
            gps_get_coord(&lat, &lng);
            gps_get_data(&year, &month, &day);
            gps_get_time(&hour, &minute, &second);
            gps_get_satellites(&sats);
            gps_get_speed(&speed);
            text = "sat=";
            text += sats;
            text += "  ";
            text += year;
            text += "/";
            text += month;
            text += "/";
            text += day;
            text += "\nlat=";
            text += String(lat, 6);
            text += " lng=";
            text += String(lng, 6);
            text += "\nspeed=";
            text += String(speed, 1);
            text += " km/h";
            break;
        }
        case TEST_AUDIO:
            text = g_audio_ctx.initialized ? "ES8311 initialized. Built-in WAV playback started." : "ES8311 init failed.";
            break;
        case TEST_BATTERY:
            text = "BQ25896: ";
            text += g_battery_ctx.charger_ready ? "OK" : "FAIL";
            text += "\nBQ27220: ";
            text += g_battery_ctx.gauge_ready ? "OK" : "FAIL";
            if (g_battery_ctx.charger_ready) {
                text += "\nVBUS=";
                text += String(PPM.getVbusVoltage() / 1000.0f, 2);
                text += "V  VBAT=";
                text += String(PPM.getBattVoltage() / 1000.0f, 2);
                text += "V";
            }
            if (g_battery_ctx.gauge_ready) {
                text += "\nSOC=";
                text += bq27220.getStateOfCharge();
                text += "%  Temp=";
                text += bq27220.getTemperature();
            }
            break;
        case TEST_MOTOR:
            text = "Cycling vibration effect: ";
            text += kMotorEffects[g_motor_ctx.effect_index % 3];
            text += "\nConfirm clear vibration manually.";
            break;
        case TEST_IMU: {
            float x = 0, y = 0, z = 0;
            if (g_imu_ctx.initialized) {
                BHI260AP_get_val(2, &x, &y, &z);
            }
            text = "gyro=(";
            text += String(x, 1);
            text += ", ";
            text += String(y, 1);
            text += ", ";
            text += String(z, 1);
            text += ")";
            text += "\nmax delta=";
            text += String(g_imu_ctx.max_delta, 1);
            break;
        }
        case TEST_WIFI:
            text = g_wifi_ctx.summary;
            break;
        case TEST_SD:
            text = g_sd_ctx.mounted ? "SD mounted." : "SD mount failed.";
            if (g_sd_ctx.mounted) {
                text += "\nTotal=";
                text += g_sd_ctx.total_mb;
                text += "MB  Used=";
                text += g_sd_ctx.used_mb;
                text += "MB";
            }
            break;
        case TEST_MODEM:
            text = g_modem_ctx.summary;
            break;
        case TEST_LORA:
            text = g_lora_ctx.initialized ? "SX1262 init succeeded." : "SX1262 init failed.";
            if (g_current_mode == TEST_MODE_DEEP) {
                text += "\nmode=";
                text += (lora_get_mode() == LORA_MODE_RECV ? "recv" : "send");
                text += "\nrssi=";
                text += g_lora_ctx.last_rssi;
                text += "\nmsg=";
                text += g_lora_ctx.last_recv;
            }
            break;
        case TEST_BLE:
            text = g_ble_ctx.summary;
            break;
        case TEST_SLEEP:
            text = "Entering deep sleep in 1 second.\nPress BOOT to wake the device.";
            break;
        default:
            text = g_tests[g_current_test].summary_cn;
            break;
    }
    return text;
}

void refresh_test_page_text()
{
    if (g_current_page != PAGE_TEST || g_current_test < 0) {
        return;
    }
    String status_line = "Status: ";
    status_line += status_text(g_results[g_current_test].status);
    if (g_results[g_current_test].note[0]) {
        status_line += " / ";
        status_line += g_results[g_current_test].note;
    }
    if (g_test_status_label) {
        lv_label_set_text(g_test_status_label, current_test_status_text().c_str());
    }
    if (g_test_hint_label) {
        if (g_current_test == TEST_EPD) {
            lv_label_set_text_fmt(g_test_hint_label, "Partial refresh count: %lu", (unsigned long)g_display_ctx.partial_count);
        } else if (g_current_test == TEST_TOUCH) {
            int key_hit_count = 0;
            for (int i = 0; i < 3; ++i) {
                if (g_touch_ctx.key_hit[i]) {
                    ++key_hit_count;
                }
            }
            lv_label_set_text_fmt(g_test_hint_label, "Seen: screen %s, keys %d/3", g_touch_ctx.has_point ? "OK" : "--", key_hit_count);
        } else {
            lv_label_set_text(g_test_hint_label, status_line.c_str());
        }
    }
}

static bool handle_keypad_hit(char key, const char *targets, bool *seen, uint8_t *hit_count)
{
    for (size_t i = 0; i < strlen(targets); ++i) {
        if (targets[i] == key && !seen[i]) {
            seen[i] = true;
            ++(*hit_count);
            return true;
        }
    }
    return false;
}

static bool test_setup_epd(TestMode mode)
{
    LV_UNUSED(mode);
    memset(&g_display_ctx, 0, sizeof(g_display_ctx));
    g_display_ctx.last_tick = millis();
    return true;
}

static void test_poll_epd(TestMode mode)
{
    LV_UNUSED(mode);
    if (millis() - g_display_ctx.last_tick < 700) {
        return;
    }
    g_display_ctx.last_tick = millis();
    g_display_ctx.phase = (g_display_ctx.phase + 1) % 3;
    ++g_display_ctx.partial_count;
    if (g_display_ctx.box) {
        lv_obj_set_style_bg_color(
            g_display_ctx.box,
            g_display_ctx.phase == 1 ? lv_color_white() : lv_color_black(),
            LV_PART_MAIN);
    }
}

static void test_teardown_epd(TestMode mode) { LV_UNUSED(mode); }

static bool test_setup_touch(TestMode mode)
{
    LV_UNUSED(mode);
    memset(g_touch_ctx.key_down, 0, sizeof(g_touch_ctx.key_down));
    memset(g_touch_ctx.key_hit, 0, sizeof(g_touch_ctx.key_hit));
    g_touch_ctx.last_x = 0;
    g_touch_ctx.last_y = 0;
    g_touch_ctx.point_count = 0;
    g_touch_ctx.has_point = false;
    hyn_touch_clear_key_seen();
    return g_touch_ready;
}

static void test_poll_touch(TestMode mode)
{
    LV_UNUSED(mode);
    int key_hit_count = 0;
    for (int i = 0; i < 3; ++i) {
        bool down = hyn_touch_get_key_state(i);
        g_touch_ctx.key_down[i] = down;
        if (down || hyn_touch_get_key_seen(i)) {
            g_touch_ctx.key_hit[i] = true;
        }
        if (g_touch_ctx.key_hit[i]) {
            ++key_hit_count;
        }
        if (g_touch_ctx.key_blocks[i]) {
            lv_color_t color = down ? lv_color_black() : lv_color_white();
            lv_color_t text_color = down ? lv_color_white() : lv_color_black();
            lv_obj_set_style_bg_color(g_touch_ctx.key_blocks[i], color, LV_PART_MAIN);
            lv_obj_set_style_text_color(g_touch_ctx.key_blocks[i], text_color, LV_PART_MAIN);
            if (g_touch_ctx.key_labels[i]) {
                lv_obj_set_style_text_color(g_touch_ctx.key_labels[i], text_color, LV_PART_MAIN);
            }
        }
    }
    if (g_touch_ctx.has_point && key_hit_count == 3 && g_results[TEST_TOUCH].status == TEST_STATUS_RUNNING) {
        set_test_status(TEST_TOUCH, TEST_STATUS_PASS, "screen touch and 3 keys detected");
        if (g_current_mode == TEST_MODE_QUICK) {
            quick_auto_advance();
        }
    }
}
static void test_teardown_touch(TestMode mode) { LV_UNUSED(mode); }

static bool test_setup_keypad(TestMode mode)
{
    LV_UNUSED(mode);
    memset(&g_keypad_ctx, 0, sizeof(g_keypad_ctx));
    return init_keypad_if_needed();
}

static void test_poll_keypad(TestMode mode) { LV_UNUSED(mode); }
static void test_teardown_keypad(TestMode mode) { LV_UNUSED(mode); }

static bool test_setup_gps(TestMode mode)
{
    LV_UNUSED(mode);
    g_gps_ctx.start_ms = millis();
    bool ok = init_gps_if_needed();
    if (ok) {
        gps_task_resume();
        g_gps_ctx.task_resumed = true;
    }
    return ok;
}

static void test_poll_gps(TestMode mode)
{
    LV_UNUSED(mode);
    double lat = 0, lng = 0;
    uint16_t year = 0;
    uint8_t month = 0, day = 0;
    uint32_t sats = 0;
    gps_get_coord(&lat, &lng);
    gps_get_data(&year, &month, &day);
    gps_get_satellites(&sats);
    if (g_current_mode == TEST_MODE_QUICK && g_results[TEST_GPS].status == TEST_STATUS_RUNNING) {
        if (sats > 0 || year > 2023 || fabs(lat) > 0.0001 || fabs(lng) > 0.0001) {
            set_test_status(TEST_GPS, TEST_STATUS_PASS, "gps activity detected");
            quick_auto_advance();
        } else if (millis() - g_gps_ctx.start_ms > 6000) {
            copy_note(TEST_GPS, "no fix yet, manual verdict");
        }
    }
}

static void test_teardown_gps(TestMode mode)
{
    LV_UNUSED(mode);
    if (g_gps_ctx.task_resumed) {
        gps_task_suspend();
        g_gps_ctx.task_resumed = false;
    }
    set_power_pin(BOARD_XL9555_02_GPS_EN, false);
}

static bool test_setup_audio(TestMode mode)
{
    LV_UNUSED(mode);
    g_audio_ctx.start_ms = millis();
    bool ok = init_codec_if_needed();
    if (ok) {
        route_audio_to_es8311(true);
        codec.playWAV((uint8_t *)wav_hex, wav_hex_len);
        g_audio_ctx.played = true;
        copy_note(TEST_AUDIO, "listening for waveform");
    }
    return ok;
}

static void test_poll_audio(TestMode mode)
{
    if (mode == TEST_MODE_DEEP && g_audio_ctx.initialized && millis() - g_audio_ctx.start_ms > 4000) {
        codec.playWAV((uint8_t *)wav_hex, wav_hex_len);
        g_audio_ctx.start_ms = millis();
    }
}

static void test_teardown_audio(TestMode mode)
{
    LV_UNUSED(mode);
    route_audio_to_es8311(false);
}

static bool test_setup_battery(TestMode mode)
{
    LV_UNUSED(mode);
    if (!g_battery_ctx.charger_ready && !g_battery_ctx.gauge_ready) {
        init_power_manager();
    }
    if (g_battery_ctx.charger_ready || g_battery_ctx.gauge_ready) {
        set_test_status(TEST_BATTERY, TEST_STATUS_PASS, "battery telemetry ready");
        if (g_current_mode == TEST_MODE_QUICK) {
            quick_auto_advance();
        }
        return true;
    }
    copy_note(TEST_BATTERY, "BQ25896/BQ27220 not detected");
    return false;
}

static void test_poll_battery(TestMode mode) { LV_UNUSED(mode); }
static void test_teardown_battery(TestMode mode) { LV_UNUSED(mode); }

static bool test_setup_motor(TestMode mode)
{
    LV_UNUSED(mode);
    bool ok = init_motor_if_needed();
    g_motor_ctx.effect_index = 0;
    g_motor_ctx.last_tick = 0;
    return ok;
}

static void test_poll_motor(TestMode mode)
{
    LV_UNUSED(mode);
    if (!g_motor_ctx.initialized) {
        return;
    }
    if (millis() - g_motor_ctx.last_tick < 1000) {
        return;
    }
    g_motor_ctx.last_tick = millis();
    int effect = kMotorEffects[g_motor_ctx.effect_index % 3];
    motor_drv.setWaveform(0, effect);
    motor_drv.setWaveform(1, 0);
    motor_drv.go();
    g_motor_ctx.effect_index = (g_motor_ctx.effect_index + 1) % 3;
}

static void test_teardown_motor(TestMode mode)
{
    LV_UNUSED(mode);
    if (g_motor_ctx.initialized) {
        motor_drv.stop();
    }
    set_power_pin(BOARD_XL9555_05_MOTOR_EN, false);
}

static bool test_setup_imu(TestMode mode)
{
    LV_UNUSED(mode);
    g_imu_ctx.start_ms = millis();
    g_imu_ctx.max_delta = 0;
    g_imu_ctx.baseline_ready = false;
    return init_imu_if_needed();
}

static void test_poll_imu(TestMode mode)
{
    LV_UNUSED(mode);
    if (!g_imu_ctx.initialized) {
        return;
    }
    float x = 0, y = 0, z = 0;
    BHI260AP_get_val(2, &x, &y, &z);
    if (!g_imu_ctx.baseline_ready) {
        g_imu_ctx.base_x = x;
        g_imu_ctx.base_y = y;
        g_imu_ctx.base_z = z;
        g_imu_ctx.baseline_ready = true;
        return;
    }
    float delta = fabs(x - g_imu_ctx.base_x) + fabs(y - g_imu_ctx.base_y) + fabs(z - g_imu_ctx.base_z);
    if (delta > g_imu_ctx.max_delta) {
        g_imu_ctx.max_delta = delta;
    }
    if (g_current_mode == TEST_MODE_QUICK && g_results[TEST_IMU].status == TEST_STATUS_RUNNING && g_imu_ctx.max_delta > 18.0f) {
        set_test_status(TEST_IMU, TEST_STATUS_PASS, "imu movement detected");
        quick_auto_advance();
    }
}

static void test_teardown_imu(TestMode mode)
{
    LV_UNUSED(mode);
    set_power_pin(BOARD_XL9555_03_1V8_EN, false);
}

static bool test_setup_wifi(TestMode mode)
{
    LV_UNUSED(mode);
    g_wifi_ctx = {};
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, true);
    delay(20);
    g_wifi_ctx.scan_started = true;
    g_wifi_ctx.start_ms = millis();
    WiFi.scanDelete();
    WiFi.scanNetworks(true, false);
    g_wifi_ctx.summary = "Scanning...";
    return true;
}

static void test_poll_wifi(TestMode mode)
{
    LV_UNUSED(mode);
    if (!g_wifi_ctx.scan_started || g_wifi_ctx.scan_done) {
        return;
    }
    int state = WiFi.scanComplete();
    if (state == WIFI_SCAN_RUNNING) {
        return;
    }
    if (state >= 0) {
        g_wifi_ctx.scan_done = true;
        g_wifi_ctx.scan_count = state;
        g_wifi_ctx.summary = "Scan done, AP count = ";
        g_wifi_ctx.summary += state;
        int show = state > 6 ? 6 : state;
        for (int i = 0; i < show; ++i) {
            g_wifi_ctx.summary += "\n";
            g_wifi_ctx.summary += WiFi.SSID(i).c_str();
            g_wifi_ctx.summary += " (";
            g_wifi_ctx.summary += WiFi.RSSI(i);
            g_wifi_ctx.summary += ")";
        }
        set_test_status(TEST_WIFI, TEST_STATUS_PASS, state > 0 ? "scan finished" : "scan finished with 0 AP");
        if (g_current_mode == TEST_MODE_QUICK) {
            quick_auto_advance();
        }
        WiFi.scanDelete();
    } else if (millis() - g_wifi_ctx.start_ms > 10000) {
        set_test_status(TEST_WIFI, TEST_STATUS_FAIL, "scan timeout");
        WiFi.scanDelete();
    }
}

static void test_teardown_wifi(TestMode mode)
{
    LV_UNUSED(mode);
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);
}

static bool test_setup_sd(TestMode mode)
{
    LV_UNUSED(mode);
    bool ok = init_sd_if_needed();
    if (!ok) {
        return false;
    }
    File file = SD.open("/eng_test.tmp", FILE_WRITE);
    if (!file) {
        return false;
    }
    file.print("ENG_TEST_OK");
    file.close();

    file = SD.open("/eng_test.tmp", FILE_READ);
    if (!file) {
        return false;
    }
    String payload = file.readString();
    file.close();
    SD.remove("/eng_test.tmp");
    g_sd_ctx.tested = payload.indexOf("ENG_TEST_OK") >= 0;
    if (g_sd_ctx.tested) {
        set_test_status(TEST_SD, TEST_STATUS_PASS, "rw check passed");
        if (g_current_mode == TEST_MODE_QUICK) {
            quick_auto_advance();
        }
    }
    return g_sd_ctx.tested;
}

static void test_poll_sd(TestMode mode) { LV_UNUSED(mode); }
static void test_teardown_sd(TestMode mode) { LV_UNUSED(mode); }

static bool test_setup_modem(TestMode mode)
{
    g_modem_ctx.summary = "Initializing...";
    g_modem_ctx.start_ms = millis();
    bool ok = init_modem_if_needed();
    if (!ok) {
        g_modem_ctx.summary = "A7682E AT no response.\nHint: this test normally requires battery power.";
        return false;
    }
    g_modem_ctx.signal_quality = modem.getSignalQuality();
    g_modem_ctx.summary = "AT OK";
    g_modem_ctx.summary += "\nCSQ=";
    g_modem_ctx.summary += g_modem_ctx.signal_quality;
    g_modem_ctx.summary += "\n";
    g_modem_ctx.summary += (mode == TEST_MODE_DEEP)
        ? "USB serial bridge is enabled. Send AT directly."
        : "Quick test only verifies AT response.";

    if (mode == TEST_MODE_DEEP && a7682_handle) {
        vTaskResume(a7682_handle);
        g_modem_ctx.bridge_active = true;
    }
    if (mode == TEST_MODE_QUICK) {
        set_test_status(TEST_MODEM, TEST_STATUS_PASS, "AT responded");
        quick_auto_advance();
    }
    return true;
}

static void test_poll_modem(TestMode mode) { LV_UNUSED(mode); }

static void test_teardown_modem(TestMode mode)
{
    LV_UNUSED(mode);
    if (g_modem_ctx.bridge_active && a7682_handle) {
        vTaskSuspend(a7682_handle);
        g_modem_ctx.bridge_active = false;
    }
}

static bool test_setup_lora(TestMode mode)
{
    g_lora_ctx.start_ms = millis();
    g_lora_ctx.receive_mode = mode == TEST_MODE_DEEP;
    g_lora_ctx.last_recv = "";
    g_lora_ctx.last_rssi = 0;
    bool ok = init_lora_if_needed();
    if (!ok) {
        return false;
    }
    if (mode == TEST_MODE_DEEP) {
        lora_set_mode(LORA_MODE_RECV);
        copy_note(TEST_LORA, "recv mode, wait peer packet");
    } else {
        lora_set_mode(LORA_MODE_SEND);
    }
    return true;
}

static void test_poll_lora(TestMode mode)
{
    if (!g_lora_ctx.initialized) {
        return;
    }
    if (mode == TEST_MODE_QUICK && g_results[TEST_LORA].status == TEST_STATUS_RUNNING && millis() - g_lora_ctx.start_ms > 1200) {
        set_test_status(TEST_LORA, TEST_STATUS_PASS, "radio init completed");
        quick_auto_advance();
        return;
    }
    if (mode == TEST_MODE_DEEP) {
        lora_receive_loop();
        const char *str = nullptr;
        int rssi = 0;
        if (lora_get_recv(&str, &rssi)) {
            g_lora_ctx.last_recv = str ? str : "";
            g_lora_ctx.last_rssi = rssi;
            copy_note(TEST_LORA, "packet received");
        }
    }
}

static void test_teardown_lora(TestMode mode)
{
    LV_UNUSED(mode);
    if (g_lora_ctx.initialized) {
        lora_sleep();
    }
    set_power_pin(BOARD_XL9555_01_LORA_EN, false);
}

static bool test_setup_ble(TestMode mode)
{
    g_ble_ctx = {};
    bool ok = init_ble_if_needed();
    if (!ok) {
        return false;
    }
    g_ble_ctx.start_ms = millis();
    g_ble_ctx.phase = 0;
    g_ble_ctx.summary = "Advertising started.";
    if (mode == TEST_MODE_DEEP) {
        g_ble_ctx.summary += "\nSearch for name on your phone: TDeckPro-ENG";
    }
    return true;
}

static void test_poll_ble(TestMode mode)
{
    if (!g_ble_ctx.initialized) {
        return;
    }
    if (!g_ble_ctx.scan_done && millis() - g_ble_ctx.start_ms > 900) {
        BLEDevice::stopAdvertising();
        g_ble_ctx.advertising = false;
        BLEScan *scan = BLEDevice::getScan();
        scan->setActiveScan(false);
        BLEScanResults results = scan->start(mode == TEST_MODE_DEEP ? 4 : 2, false);
        g_ble_ctx.scan_done = true;
        g_ble_ctx.scan_count = results.getCount();
        g_ble_ctx.summary = "Advertising + scan done\nDevices found: ";
        g_ble_ctx.summary += g_ble_ctx.scan_count;
        if (mode == TEST_MODE_DEEP && g_ble_ctx.scan_count > 0) {
            BLEAdvertisedDevice device = results.getDevice(0);
            g_ble_ctx.summary += "\nFirst device: ";
            g_ble_ctx.summary += device.getName().c_str();
        }
        scan->clearResults();
        if (mode == TEST_MODE_QUICK && g_results[TEST_BLE].status == TEST_STATUS_RUNNING) {
            set_test_status(TEST_BLE, TEST_STATUS_PASS, "ble init+scan done");
            quick_auto_advance();
        }
    }
}

static void test_teardown_ble(TestMode mode)
{
    LV_UNUSED(mode);
    stop_ble_activity();
    g_ble_ctx.initialized = false;
}

static bool test_setup_sleep(TestMode mode)
{
    LV_UNUSED(mode);
    g_sleep_ctx.armed = true;
    g_sleep_ctx.deadline_ms = millis() + 1200;
    return true;
}

static void test_poll_sleep(TestMode mode)
{
    LV_UNUSED(mode);
    if (!g_sleep_ctx.armed) {
        return;
    }
    if (millis() >= g_sleep_ctx.deadline_ms) {
        g_sleep_ctx.armed = false;
        enter_deep_sleep_now();
    }
}

static void test_teardown_sleep(TestMode mode) { LV_UNUSED(mode); }

bool handle_keypad_test_key(char key)
{
    if (g_current_test != TEST_KEYPAD) {
        return false;
    }
    if (g_current_mode == TEST_MODE_QUICK) {
        handle_keypad_hit(key, kQuickKeyTargets, g_keypad_ctx.quick_seen, &g_keypad_ctx.quick_hits);
        if (g_keypad_ctx.quick_hits == strlen(kQuickKeyTargets) && g_results[TEST_KEYPAD].status == TEST_STATUS_RUNNING) {
            set_test_status(TEST_KEYPAD, TEST_STATUS_PASS, "quick keys covered");
            quick_auto_advance();
        }
    } else {
        handle_keypad_hit(key, kDeepKeyTargets, g_keypad_ctx.deep_seen, &g_keypad_ctx.deep_hits);
        if (g_keypad_ctx.deep_hits == strlen(kDeepKeyTargets) && g_results[TEST_KEYPAD].status == TEST_STATUS_RUNNING) {
            set_test_status(TEST_KEYPAD, TEST_STATUS_PASS, "deep key coverage done");
        }
    }
    return true;
}
