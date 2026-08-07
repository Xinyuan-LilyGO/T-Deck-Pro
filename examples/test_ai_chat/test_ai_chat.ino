/**
 * T-Deck Pro complete Voice AI demo.
 *
 * Flow: keyboard/UI -> PDM microphone -> iFlytek RTASR-LLM -> DeepSeek
 *       -> Chinese LVGL text -> A7682E AT+CTTS playback.
 *
 * This is deliberately a small standalone application.  It does not open
 * PDA2's launcher, weather, LoRa, GPS, calendar, or other apps.
 */
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <GxEPD2_BW.h>
#include <Adafruit_TCA8418.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <freertos/queue.h>
#include <time.h>
#include "ai_chat_config.h"
#include "utilities.h"

/* The complete Chinese/English UI font is stored in this demo's src folder. */
extern "C" {
#include "src/assets.h"
}
#include "deepseek_api.h"
#include "xfyun_asr.h"
#include "pdm_recorder.h"
#include "a7682_tts.h"

/* Used by the standalone local A7682E TTS implementation. */
TaskHandle_t a7682_handle = nullptr;

namespace {

constexpr int kDisplayWidth = LCD_HOR_SIZE;
constexpr int kDisplayHeight = LCD_VER_SIZE;
constexpr size_t kDisplayPixels = kDisplayWidth * kDisplayHeight;
constexpr size_t kEpdBytes = ((kDisplayWidth + 7) / 8) * kDisplayHeight;
constexpr uint32_t kWifiTimeoutMs = 20000;
constexpr uint8_t kKeyRows = 4;
constexpr uint8_t kKeyCols = 10;

using InkPanel = GxEPD2_310_GDEQ031T10;
using InkDisplay = GxEPD2_BW<InkPanel, InkPanel::HEIGHT>;
InkDisplay epd_v11(InkPanel(BOARD_EPD_CS, BOARD_EPD_DC, BOARD_EPD_RST, BOARD_EPD_BUSY));
InkDisplay epd_v10(InkPanel(BOARD_EPD_CS, BOARD_EPD_DC, -1, BOARD_EPD_BUSY));
InkDisplay *epd = &epd_v11;

lv_obj_t *chat_label = nullptr;
lv_obj_t *status_label = nullptr;
lv_obj_t *input_box = nullptr;
lv_timer_t *ui_timer = nullptr;
QueueHandle_t ui_queue = nullptr;
TaskHandle_t voice_task = nullptr;
uint8_t *epd_bitmap = nullptr;
String last_answer;
bool modem_ready = false;

enum UiMessageType { UI_CHAT, UI_STATUS, UI_TTS };
struct UiMessage {
    UiMessageType type;
    char *text;
};

const char keymap[kKeyRows][kKeyCols] = {
    {'q','w','e','r','t','y','u','i','o','p'},
    {'a','s','d','f','g','h','j','k','l','\b'},
    { 0 ,'z','x','c','v','b','n','m',' ', '\n'},
    { 0 , 0 , 0 , 0 , 0 , 0 , 0 ,' ', 0 , 0 },
};
Adafruit_TCA8418 keypad;

void post_ui(UiMessageType type, const char *text)
{
    if (!ui_queue) return;
    UiMessage message{type, text ? strdup(text) : nullptr};
    if (xQueueSend(ui_queue, &message, pdMS_TO_TICKS(1000)) != pdTRUE) {
        free(message.text);
    }
}

void set_status(const char *text)
{
    if (status_label) lv_label_set_text(status_label, text ? text : "");
}

void append_chat(const char *text)
{
    if (!chat_label || !text) return;
    String all = lv_label_get_text(chat_label);
    if (all.length()) all += '\n';
    all += text;
    /* Bound UI memory without truncating individual responses in normal use. */
    if (all.length() > 2800) all.remove(0, all.length() - 2800);
    lv_label_set_text(chat_label, all.c_str());
    lv_obj_scroll_to_y(chat_label, LV_COORD_MAX, LV_ANIM_OFF);
}

void modem_drain()
{
    while (SerialAT.available()) SerialAT.read();
}

bool modem_at(const char *command, uint32_t timeout_ms)
{
    modem_drain();
    Serial.printf("[VoiceDemo][A7682E] >> %s\n", command);
    SerialAT.print(command);
    SerialAT.print("\r\n");
    String reply;
    const uint32_t started = millis();
    while (millis() - started < timeout_ms) {
        while (SerialAT.available()) reply += static_cast<char>(SerialAT.read());
        if (reply.indexOf("OK") >= 0 || reply.indexOf("ERROR") >= 0) break;
        delay(2);
    }
    reply.replace("\r", " ");
    reply.replace("\n", " ");
    Serial.printf("[VoiceDemo][A7682E] << %s\n", reply.c_str());
    return reply.indexOf("OK") >= 0 && reply.indexOf("ERROR") < 0;
}

void modem_serial_bridge(void *)
{
    vTaskSuspend(nullptr);
    for (;;) {
        while (SerialAT.available()) Serial.write(SerialAT.read());
        delay(2);
    }
}

bool modem_init()
{
    pinMode(BOARD_6609_EN, OUTPUT);
    pinMode(BOARD_A7682E_PWRKEY, OUTPUT);
    digitalWrite(BOARD_6609_EN, HIGH);
    digitalWrite(BOARD_A7682E_PWRKEY, HIGH);
    SerialAT.begin(115200, SERIAL_8N1, BOARD_A7682E_TXD, BOARD_A7682E_RXD);
    delay(300);

    if (!modem_at("AT", 1000)) {
        Serial.println("[VoiceDemo][A7682E] PWRKEY pulse");
        digitalWrite(BOARD_A7682E_PWRKEY, LOW); delay(10);
        digitalWrite(BOARD_A7682E_PWRKEY, HIGH); delay(50);
        digitalWrite(BOARD_A7682E_PWRKEY, LOW); delay(10);
        for (int retry = 0; retry < 5 && !modem_at("AT", 1000); ++retry) delay(300);
    }
    modem_ready = modem_at("AT", 1000);
    if (modem_ready && a7682_handle == nullptr) {
        xTaskCreate(modem_serial_bridge, "a7682_bridge", 3072, nullptr, 2, &a7682_handle);
    }
    Serial.printf("[VoiceDemo][A7682E] %s\n", modem_ready ? "ready" : "not ready");
    return modem_ready;
}

bool wifi_connect()
{
    if (WiFi.status() == WL_CONNECTED) return true;
    if (!WIFI_SSID[0] || !WIFI_PASSWORD[0]) {
        Serial.println("[VoiceDemo][WiFi] Set WIFI_SSID/WIFI_PASSWORD in config_keys.h");
        return false;
    }
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    const uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < kWifiTimeoutMs) delay(250);
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[VoiceDemo][WiFi] Connected: %s, RSSI=%d\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        configTime(8 * 3600, 0, "ntp.aliyun.com", "pool.ntp.org");
        return true;
    }
    Serial.printf("[VoiceDemo][WiFi] Failed, status=%d\n", WiFi.status());
    return false;
}

bool wait_for_ntp_time(uint32_t timeout_ms)
{
    const uint32_t started = millis();
    while (time(nullptr) < 1700000000 && millis() - started < timeout_ms) delay(100);
    const bool ready = time(nullptr) >= 1700000000;
    Serial.printf("[VoiceDemo][NTP] %s\n", ready ? "time ready" : "time not synchronized");
    return ready;
}

void convert_to_epd_bitmap(const lv_color_t *pixels, lv_coord_t width, lv_coord_t height)
{
    const size_t stride = (width + 7) / 8;
    memset(epd_bitmap, 0xFF, stride * height);
    for (lv_coord_t y = 0; y < height; ++y) {
        for (lv_coord_t x = 0; x < width; ++x) {
            if (lv_color_brightness(pixels[y * width + x]) < 128) {
                epd_bitmap[y * stride + x / 8] &= ~(0x80 >> (x & 7));
            }
        }
    }
}

void display_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *pixels)
{
    const lv_coord_t width = lv_area_get_width(area);
    const lv_coord_t height = lv_area_get_height(area);
    if (width > 0 && height > 0 && epd_bitmap) {
        convert_to_epd_bitmap(pixels, width, height);
        epd->setPartialWindow(area->x1, area->y1, width, height);
        epd->firstPage();
        do {
            epd->drawInvertedBitmap(area->x1, area->y1, epd_bitmap, width, height, GxEPD_BLACK);
        } while (epd->nextPage());
        epd->powerOff();
    }
    lv_disp_flush_ready(drv);
}

bool init_display()
{
    pinMode(BOARD_EPD_BL, OUTPUT);
    digitalWrite(BOARD_EPD_BL, HIGH);
    pinMode(BOARD_EPD_CS, OUTPUT); digitalWrite(BOARD_EPD_CS, HIGH);
    pinMode(BOARD_SD_CS, OUTPUT); digitalWrite(BOARD_SD_CS, HIGH);
    pinMode(BOARD_LORA_CS, OUTPUT); digitalWrite(BOARD_LORA_CS, HIGH);
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
    /* Match PDA2's panel-reset compatibility selection. */
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Wire.beginTransmission(BOARD_I2C_ADDR_DRV2605);
    const bool has_drv2605 = Wire.endTransmission() == 0;
    epd = has_drv2605 ? &epd_v11 : &epd_v10;
    Serial.printf("[VoiceDemo][EPD] reset mode: %s\n", has_drv2605 ? "hardware" : "soft-only");
    epd->epd2.selectSPI(SPI, SPISettings(2000000, MSBFIRST, SPI_MODE0));
    epd->init(115200, true, 2, false);
    epd->setRotation(0);

    lv_init();
    static lv_disp_draw_buf_t draw_buffer;
    lv_color_t *buf1 = static_cast<lv_color_t *>(heap_caps_calloc(kDisplayPixels, sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    lv_color_t *buf2 = static_cast<lv_color_t *>(heap_caps_calloc(kDisplayPixels, sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    epd_bitmap = static_cast<uint8_t *>(heap_caps_calloc(kEpdBytes, 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buf1 || !buf2 || !epd_bitmap) {
        Serial.println("[VoiceDemo][UI] PSRAM allocation failed");
        return false;
    }
    lv_disp_draw_buf_init(&draw_buffer, buf1, buf2, kDisplayPixels);
    static lv_disp_drv_t driver;
    lv_disp_drv_init(&driver);
    driver.hor_res = kDisplayWidth;
    driver.ver_res = kDisplayHeight;
    driver.flush_cb = display_flush;
    driver.draw_buf = &draw_buffer;
    driver.full_refresh = 1;
    lv_disp_drv_register(&driver);
    return true;
}

bool init_keyboard()
{
    Wire.begin(BOARD_KEYBOARD_SDA, BOARD_KEYBOARD_SCL);
    if (!keypad.begin(BOARD_I2C_ADDR_KEYBOARD, &Wire)) {
        Serial.println("[VoiceDemo][KBD] TCA8418 not found");
        return false;
    }
    keypad.matrix(kKeyRows, kKeyCols);
    keypad.flush();
    return true;
}

void start_tts()
{
    if (!last_answer.length()) return;
    if (!modem_ready && !modem_init()) {
        set_status("A7682E TTS unavailable");
        return;
    }
    set_status("正在朗读 AI 回复...");
    const bool ok = a7682_tts_speak(last_answer.c_str());
    Serial.printf("[VoiceDemo][TTS] AT+CTTS=%d\n", ok);
    if (!ok) set_status("A7682E 朗读失败，请看串口");
    else set_status("V:语音  R:重读  Enter:发送");
}

void voice_worker(void *)
{
    post_ui(UI_STATUS, "正在同步网络时间...");
    if (!wait_for_ntp_time(10000)) {
        post_ui(UI_CHAT, "NTP 时间同步失败，讯飞鉴权需要正确时间。");
        post_ui(UI_STATUS, "V:语音  R:重读  Enter:发送");
        voice_task = nullptr;
        vTaskDelete(nullptr);
        return;
    }
    post_ui(UI_STATUS, "录音中，持续 5 秒...");
    post_ui(UI_CHAT, "> [正在录音]");
    uint8_t *wav = nullptr;
    size_t wav_len = 0;
    const bool recorded = pdm_record_wav(5, 16000, &wav, &wav_len);
    pdm_restore_audio();
    if (!recorded || !wav || wav_len <= 44) {
        free(wav);
        post_ui(UI_CHAT, "录音失败，请检查 PDM 麦克风。");
        post_ui(UI_STATUS, "V:语音  R:重读  Enter:发送");
        voice_task = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    post_ui(UI_STATUS, "讯飞 RTASR-LLM 识别中...");
    xfyun_asr_response_t asr = xfyun_asr_transcribe_pcm(
        wav + 44, wav_len - 44, XFYUN_APP_ID, XFYUN_API_KEY, XFYUN_API_SECRET);
    free(wav);
    if (!asr.success) {
        String error = "ASR: " + String(asr.error.c_str());
        post_ui(UI_CHAT, error.c_str());
        post_ui(UI_STATUS, "V:语音  R:重读  Enter:发送");
        voice_task = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    String heard = "> " + String(asr.text.c_str());
    post_ui(UI_CHAT, heard.c_str());
    post_ui(UI_STATUS, "DeepSeek 思考中...");
    deepseek_response_t answer = deepseek_send_text(asr.text.c_str(), DEEPSEEK_API_KEY, DEEPSEEK_MODEL);
    if (answer.success) {
        last_answer = answer.text.c_str();
        post_ui(UI_CHAT, last_answer.c_str());
        post_ui(UI_TTS, nullptr);
    } else {
        String error = "DeepSeek: " + String(answer.error.c_str());
        post_ui(UI_CHAT, error.c_str());
        post_ui(UI_STATUS, "V:语音  R:重读  Enter:发送");
    }
    voice_task = nullptr;
    vTaskDelete(nullptr);
}

void text_worker(void *parameter)
{
    char *prompt = static_cast<char *>(parameter);
    post_ui(UI_STATUS, "DeepSeek 思考中...");
    deepseek_response_t answer = deepseek_send_text(prompt, DEEPSEEK_API_KEY, DEEPSEEK_MODEL);
    free(prompt);
    if (answer.success) {
        last_answer = answer.text.c_str();
        post_ui(UI_CHAT, last_answer.c_str());
        post_ui(UI_TTS, nullptr);
    } else {
        String error = "DeepSeek: " + String(answer.error.c_str());
        post_ui(UI_CHAT, error.c_str());
        post_ui(UI_STATUS, "V:语音  R:重读  Enter:发送");
    }
    voice_task = nullptr;
    vTaskDelete(nullptr);
}

void start_voice()
{
    if (voice_task) return;
    if (!wifi_connect()) { append_chat("WiFi 未连接，请检查 config_keys.h。"); return; }
    if (!DEEPSEEK_API_KEY[0] || !XFYUN_APP_ID[0] || !XFYUN_API_KEY[0] || !XFYUN_API_SECRET[0]) {
        append_chat("请先填写 DeepSeek 和讯飞 RTASR-LLM 凭据。");
        return;
    }
    xTaskCreatePinnedToCore(voice_worker, "voice_ai", 16384, nullptr, 5, &voice_task, 0);
}

void send_typed_text()
{
    if (voice_task || !input_box) return;
    const char *input = lv_textarea_get_text(input_box);
    if (!input || !input[0]) return;
    if (!wifi_connect()) { append_chat("WiFi 未连接，请检查 config_keys.h。"); return; }
    if (!DEEPSEEK_API_KEY[0]) { append_chat("请先填写 DEEPSEEK_API_KEY。"); return; }
    String line = "> " + String(input);
    append_chat(line.c_str());
    char *prompt = strdup(input);
    lv_textarea_set_text(input_box, "");
    if (prompt) xTaskCreatePinnedToCore(text_worker, "text_ai", 12288, prompt, 5, &voice_task, 0);
}

void poll_keyboard()
{
    const int event = keypad.getEvent();
    if (event < 129 || event > 163 || voice_task) return;
    const int index = event - 129;
    const int row = index / kKeyCols;
    const int col = (kKeyCols - 1) - (index % kKeyCols);
    if (row < 0 || row >= kKeyRows || col < 0 || col >= kKeyCols) return;
    const char key = keymap[row][col];
    if (key == '\n') { send_typed_text(); return; }
    if (key == '\b') { lv_textarea_del_char(input_box); return; }
    if (key == 'v' && lv_textarea_get_text(input_box)[0] == '\0') { start_voice(); return; }
    if (key == 'r' && lv_textarea_get_text(input_box)[0] == '\0') { start_tts(); return; }
    if (key >= ' ') lv_textarea_add_char(input_box, key);
}

void ui_tick(lv_timer_t *)
{
    UiMessage message;
    while (xQueueReceive(ui_queue, &message, 0) == pdTRUE) {
        if (message.type == UI_CHAT) append_chat(message.text);
        else if (message.type == UI_STATUS) set_status(message.text);
        else if (message.type == UI_TTS) start_tts();
        free(message.text);
    }
}

void create_ui()
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "T-Deck Pro 语音 AI");
    lv_obj_set_style_text_font(title, &Font_Chinese_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    chat_label = lv_label_create(screen);
    lv_obj_set_pos(chat_label, 7, 32);
    lv_obj_set_size(chat_label, 226, 210);
    lv_obj_set_style_text_font(chat_label, &Font_Chinese_16, LV_PART_MAIN);
    lv_label_set_long_mode(chat_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_pad_all(chat_label, 2, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(chat_label, LV_SCROLLBAR_MODE_AUTO);
    lv_label_set_text(chat_label, "V：开始 5 秒语音对话\nR：朗读上一条回复\nEnter：发送键盘输入");

    status_label = lv_label_create(screen);
    lv_obj_set_pos(status_label, 7, 245);
    lv_obj_set_width(status_label, 226);
    lv_obj_set_style_text_font(status_label, &Font_Chinese_16, LV_PART_MAIN);
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
    set_status("正在初始化 WiFi、麦克风和 A7682E...");

    input_box = lv_textarea_create(screen);
    lv_obj_set_pos(input_box, 7, 282);
    lv_obj_set_size(input_box, 226, 31);
    lv_textarea_set_one_line(input_box, true);
    lv_textarea_set_max_length(input_box, 160);
    lv_textarea_set_placeholder_text(input_box, "键盘文字提问");
    lv_obj_set_style_text_font(input_box, &Font_Chinese_16, LV_PART_MAIN);
}

} // namespace

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[VoiceDemo] Full Voice AI: PDM -> iFlytek -> DeepSeek -> A7682E");
    pinMode(BOARD_KEYBOARD_LED, OUTPUT);
    digitalWrite(BOARD_KEYBOARD_LED, LOW);
    pinMode(BOARD_GPS_EN, OUTPUT); digitalWrite(BOARD_GPS_EN, HIGH);
    pinMode(BOARD_LORA_EN, OUTPUT); digitalWrite(BOARD_LORA_EN, HIGH);
    if (!init_display()) return;
    create_ui();
    ui_queue = xQueueCreate(10, sizeof(UiMessage));
    ui_timer = lv_timer_create(ui_tick, 100, nullptr);
    init_keyboard();
    wifi_connect();
    modem_init();
    set_status("V:语音  R:重读  Enter:发送");
}

void loop()
{
    lv_timer_handler();
    poll_keyboard();
    delay(5);
}
