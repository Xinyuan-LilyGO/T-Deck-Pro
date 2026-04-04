#include "eng_test_app.h"

void setup()
{
    setCpuFrequencyMhz(240);
    Serial.begin(115200);
    delay(50);

    shared_spi_deselect_all();
    analogWrite(BOARD_EPD_BL, 24);
    analogWrite(BOARD_KEYBOARD_LED, 0);

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    init_spi_if_needed();

    if (!xl9555_io.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, XL9555_SLAVE_ADDRESS0)) {
        while (true) {
            Serial.println("[ENG_TEST] XL9555 init failed");
            delay(1000);
        }
    }

    const uint8_t output_pins[] = {
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
    for (uint8_t pin : output_pins) {
        xl9555_io.pinMode(pin, OUTPUT);
    }

    set_power_pin(BOARD_XL9555_00_6609_EN, false);
    set_power_pin(BOARD_XL9555_01_LORA_EN, false);
    set_power_pin(BOARD_XL9555_02_GPS_EN, false);
    set_power_pin(BOARD_XL9555_03_1V8_EN, false);
    set_power_pin(BOARD_XL9555_04_LORA_SEL, true);
    set_power_pin(BOARD_XL9555_05_MOTOR_EN, false);
    set_power_pin(BOARD_XL9555_06_AMPLIFIER, false);
    set_power_pin(BOARD_XL9555_10_PWEKEY_EN, false);
    set_power_pin(BOARD_XL9555_11_KEY_RST, true);
    route_audio_to_es8311(false);
    init_display_stack();
    init_touch_if_needed();
    init_keypad_if_needed();
    init_power_manager();

    boot_resume_if_needed();

    if (!g_display_ready) {
        while (true) {
            Serial.println("[ENG_TEST] display init failed");
            delay(1000);
        }
    }

    if (!g_app_timer) {
        g_app_timer = lv_timer_create([](lv_timer_t *timer) {
            LV_UNUSED(timer);
            if (g_current_page == PAGE_TEST && g_current_test >= 0) {
                g_tests[g_current_test].poll(g_current_mode);
                refresh_test_page_text();
            }
            if (g_auto_advance_at && millis() >= g_auto_advance_at) {
                g_auto_advance_at = 0;
                show_next_quick_test(true);
            }
        }, 150, nullptr);
    }

    if (g_resume_from_sleep) {
        build_resume_page();
    } else {
        build_module_select_page();
    }

    analogWrite(BOARD_EPD_BL, 0);
}

void loop()
{
    lv_timer_handler();
    poll_keypad_shortcuts();
    delay(5);
}
