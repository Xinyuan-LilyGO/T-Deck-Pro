
#include "Arduino.h"
#include "ui_deckpro_port.h"
#include "factory.h"
#include "utilities.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <TinyGPS++.h>
#include "peripheral.h"
#include "WiFi.h"
#include <ctype.h>
#include <TouchDrvCSTXXX.hpp>


// extern 
extern TouchDrvCSTXXX touch;


volatile int default_language = DEFAULT_LANGUAGE_EN;
volatile bool default_keypad_light = false;
volatile bool default_motor_status = false;
volatile bool default_gps_status = true;
volatile bool default_lora_status = true;
volatile bool default_gyro_status = true;
volatile bool default_a7682_status = true;
// ----

void ui_disp_full_refr(void)
{
    disp_full_refr();
}
//************************************[ screen 0 ]****************************************** menu

void ui_xl9555_power_on(uint8_t io)      { xl9555_io.digitalWrite(io, HIGH); }
void ui_xl9555_power_off(uint8_t io)     { xl9555_io.digitalWrite(io, LOW); }
void ui_xl9555_amplifier(bool en)           { xl9555_io.digitalWrite(BOARD_XL9555_06_AMPLIFIER, en); }
void ui_xl9555_lora_antenna_sel(bool ante)  { xl9555_io.digitalWrite(BOARD_XL9555_04_LORA_SEL, ante); }
void ui_xl9555_audio_sel(bool sel)          { xl9555_io.digitalWrite(BOARD_XL9555_12_AUDIO_SEL, sel); }

//************************************[ screen 1 ]****************************************** lora

static float lora_default_freq = 850.0;
static int lora_default_band = 125;
static int lora_default_power = 22;

float ui_lora_get_freq(void) { return lora_default_freq; }
void ui_lora_set_freq(float freq) { lora_default_freq = freq; }
int ui_lora_get_bandwidth(void) { return lora_default_band; }
void ui_lora_set_bandwidth(float bd) { lora_default_band = bd; }
int ui_lora_get_power(void) { return lora_default_power; }
void ui_lora_set_power(float po) { lora_default_power = po; }

void ui_lora_param_set(void)
{
    lora_param_set();
}

int ui_lora_get_mode(void)
{
    return lora_get_mode();
}
void ui_lora_set_mode(int mode)
{
    lora_set_mode(mode);
}
void ui_lora_send(const char *str)
{
    lora_transmit(str);
}
void ui_lora_recv_loop(void)
{
    lora_receive_loop();
}
bool ui_lora_get_recv(const char **str, int *rssi)
{
    return lora_get_recv(str, rssi);
}
void ui_lora_set_recv_flag(void)
{
    lora_set_recv_flag();
}
//************************************[ screen 2 ]****************************************** setting
#if 1
// set function
// DEFAULT_LANGUAGE_CN、DEFAULT_LANGUAGE_EN
void ui_setting_set_language(int language)
{
    default_language = language;
}
void ui_setting_set_keypad_light(bool on)
{
    // digitalWrite(BOARD_KEYBOARD_LED, on);
    // default_keypad_light = on;
}
void ui_setting_set_motor_status(bool on)
{
    // digitalWrite(BOARD_MOTOR_PIN, on);
    // default_motor_status = on;
}
void ui_setting_set_gps_status(bool on)
{
    // enable GPS module power
    // digitalWrite(BOARD_GPS_EN, on);
    // default_gps_status = on;
}
void ui_setting_set_lora_status(bool on)
{
    // enable LORA module power
    // digitalWrite(BOARD_LORA_EN, on);
    // default_lora_status = on;
}
void ui_setting_set_gyro_status(bool on)
{
    // enable gyroscope module power
    // digitalWrite(BOARD_1V8_EN, on);
    // default_gyro_status = on;
}
void ui_setting_set_a7682_status(bool on)
{
    // enable 7682 module power
    // digitalWrite(BOARD_6609_EN, on);
    // digitalWrite(BOARD_A7682E_PWRKEY, on);
    // default_a7682_status = on;
}

bool ui_setting_get_keypad_light(void)
{
    return default_keypad_light;
}
bool ui_setting_get_motor_status(void)
{
    return default_motor_status;
}
bool ui_setting_get_gps_status(void)
{
    return default_gps_status;
}
bool ui_setting_get_lora_status(void)
{
    return default_lora_status;
}
bool ui_setting_get_gyro_status(void)
{
    return default_gyro_status;
}
bool ui_setting_get_a7682_status(void)
{
    return default_a7682_status;
}

// About System
const char *ui_setting_get_sf_ver(void)
{
    return UI_T_DECK_PRO_VERSION;
}
const char *ui_setting_get_hd_ver(void)
{
    return BOARD_T_DECK_PRO_VERSION;
}

void ui_setting_get_sd_capacity(uint64_t *total, uint64_t *used)
{
    if(ui_test_sd_card())
    {
        if(total)
            *total = SD.totalBytes() / (1024 * 1024);
        if(used)
            *used = SD.usedBytes() / (1024 * 1024);

        printf("total=%lluMB, used=%lluMB\n", *total, *used);

        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        Serial.printf("SD Card Size: %lluMB\n", cardSize);

        uint64_t totalSize = SD.totalBytes() / (1024 * 1024);
        Serial.printf("SD Card Total: %lluMB\n", totalSize);

        uint64_t usedSize = SD.usedBytes() / (1024 * 1024);
        Serial.printf("SD Card Used: %lluMB\n", usedSize);
    }
}

#endif
//************************************[ screen 3 ]****************************************** GPS
void ui_gps_task_suspend(void)
{
    gps_task_suspend();
}
void ui_gps_task_resume(void)
{
    gps_task_resume();
}
void ui_gps_get_coord(double *lat, double *lng)
{
    gps_get_coord(lat, lng);
}
void ui_gps_get_data(uint16_t *year, uint8_t *month, uint8_t *day)
{
    gps_get_data(year, month, day);
}
void ui_gps_get_time(uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    gps_get_time(hour, minute, second);
}

void ui_gps_get_satellites(uint32_t *vsat)
{
    gps_get_satellites(vsat);
}
void ui_gps_get_speed(double *speed)
{
    gps_get_speed(speed);
}
//************************************[ screen 4 ]****************************************** Wifi Scan
int is_chinese_utf8(const char *str) {
    unsigned char c = (unsigned char)str[0];
    return (c >= 0xE0 && c <= 0xEF);  // 检查第一个字节是否在 UTF-8 的中文字符范围内
}

void ui_wifi_get_scan_info(ui_wifi_scan_info_t *list, int list_len)
{
    int n = WiFi.scanNetworks();
    if(n > list_len)
        n = list_len;
    
    memset(list, 0, (sizeof(*list) * list_len));
    for(int i = 0; i < n; i++)
    {
        const char *str = WiFi.SSID(i).c_str();
        if(is_chinese_utf8(str))
            continue;
        strncpy(list[i].name, WiFi.SSID(i).c_str(), 16);
        list[i].rssi = WiFi.RSSI(i);
    }
}
//************************************[ screen 5 ]****************************************** Test
bool ui_test_get(int peri_id)
{
    return peri_init_st[peri_id];
}
bool ui_test_sd_card(void) 
{
    return peri_init_st[E_PERI_SD];
}
bool ui_test_a7682e(void) 
{
    return peri_init_st[E_PERI_A7682E];
}
bool ui_test_pcm5102a(void)
{
    return peri_init_st[E_PERI_ES8311];
}

//************************************[ screen 6 ]****************************************** Battery
#if 1

// BQ25896
bool ui_battery_25896_is_vbus_in(void) { return PPM.isVbusIn(); }
bool ui_batt_25896_is_chg(void)
{
    if(PPM.isCharging() == false) {
        return false;
    } else {
        return true;
    }
}
float ui_batt_25896_get_vbus(void)           { return (PPM.getVbusVoltage() *1.0 / 1000.0 ); }
float ui_batt_25896_get_vsys(void)           { return (PPM.getSystemVoltage() * 1.0 / 1000.0); }
float ui_batt_25896_get_vbat(void)           { return (PPM.getBattVoltage() * 1.0 / 1000.0); }
float ui_batt_25896_get_volt_targ(void)      { return (PPM.getChargeTargetVoltage() * 1.0 / 1000.0); }
float ui_batt_25896_get_chg_curr(void)       { return (PPM.getChargeCurrent()); }
float ui_batt_25896_get_pre_curr(void)       { return (PPM.getPrechargeCurr()); }
const char * ui_batt_25896_get_chg_st(void)  { return PPM.getChargeStatusString(); }
const char * ui_batt_25896_get_vbus_st(void) { return PPM.getBusStatusString(); }
const char * ui_batt_25896_get_ntc_st(void)  { return PPM.getNTCStatusString(); }

/* 27220 */
bool ui_battery_27220_is_vaild(void)            {return peri_init_st[E_PERI_BQ27220]; }
bool ui_battery_27220_get_input(void)           { return bq27220.getIsCharging();}
bool ui_battery_27220_get_charge_finish(void)   { return bq27220.getCharingFinish();}
uint16_t ui_battery_27220_get_status(void) 
{
    BQ27220BatteryStatus batt;
    bq27220.getBatteryStatus(&batt);
    return batt.full;
}
uint16_t ui_battery_27220_get_voltage(void)         { return bq27220.getVoltage(); }
int16_t ui_battery_27220_get_current(void)          { return bq27220.getCurrent(); }
uint16_t ui_battery_27220_get_temperature(void)     { return bq27220.getTemperature(); }
uint16_t ui_battery_27220_get_full_capacity(void)   { return bq27220.getFullChargeCapacity(); }
uint16_t ui_battery_27220_get_design_capacity(void) { return bq27220.getDesignCapacity(); }
uint16_t ui_battery_27220_get_remain_capacity(void) { return bq27220.getRemainingCapacity(); }
uint16_t ui_battery_27220_get_percent(void)         { return bq27220.getStateOfCharge(); }
uint16_t ui_battery_27220_get_health(void)          { return bq27220.getStateOfHealth(); }
const char * ui_battert_27220_get_percent_level(void)
{
    int percent = bq27220.getStateOfCharge();
    const char * str = NULL;
    if(percent < 20)      str =  LV_SYMBOL_BATTERY_EMPTY;
    else if(percent < 40) str =  LV_SYMBOL_BATTERY_1;
    else if(percent < 65) str =  LV_SYMBOL_BATTERY_2;
    else if(percent < 90) str =  LV_SYMBOL_BATTERY_3;
    else                  str =  LV_SYMBOL_BATTERY_FULL;
    return str;
}
#endif
//************************************[ screen 7 ]****************************************** Input
int ui_input_get_touch_coord(int *x, int *y)
{
    int16_t last_x = 0;
    int16_t last_y = 0;
    int ret = touch.getPoint(&last_x, &last_y);
    *x = last_x;
    *y = last_y;
    return ret;
}

int ui_input_get_keypay_val(char *v)
{
    return keypad_get_val(v);
}

void ui_input_set_keypay_flag(void)
{
    keypad_set_flag();
}

int ui_other_get_gyro(float *gyro_x, float *gyro_y, float *gyro_z)
{
    // if(gyro_x != NULL) *gyro_x = lv_rand(0, LCD_VER_SIZE);
    // if(gyro_y != NULL) *gyro_y = lv_rand(0, LCD_VER_SIZE);
    // if(gyro_z != NULL) *gyro_z = lv_rand(0, LCD_VER_SIZE);

    if((gyro_x != NULL) && (gyro_x != NULL) && (gyro_x != NULL))
    {
        BHI260AP_get_val(2, gyro_x, gyro_y, gyro_z);
    }
    else
    {
        Serial.printf("[%d] %s : Argument cannot be empty", __LINE__, __FILE__);
    }
    return 1;
}

//************************************[ screen 8 ]****************************************** A7682E
bool ui_a7682_at_cb(const char *at_cmd)
{
    printf("[A7682E] at cmd: %s\n", at_cmd);

    modem.sendAT("+CTTSPARAM=1,3,0,1,1");

    delay(100);

    modem.sendAT("+CTTS=2,\"1234567890\"");

    return false;
}

void ui_a7682_call(const char *number)
{
    char buf[32];
    lv_snprintf(buf, 32, "D%s;", number);
    printf("[A7682E] at cmd: %s\n", buf);

    modem.sendAT(buf);
    delay(100);
}

void ui_a7682_hang_up(void)
{
    modem.sendAT("+CHUP");
    delay(100);
}

void ui_a7682_loop_resume(void)
{
    vTaskResume(a7682_handle);
}

void ui_a7682_loop_suspend(void)
{
    vTaskSuspend(a7682_handle);
}

//************************************[ screen 9 ]****************************************** Input

void ui_shutdown_on(void)
{
    PPM.shutdown();
    Serial.println("Shutdown .....");
}

//************************************[ screen 10 ]****************************************** PCM5102

#include "wav_hex.h"
bool ui_pcm5102_cb(const char *at_cmd)
{
    codec.playWAV((uint8_t*)wav_hex, wav_hex_len);
    return true;
}

void ui_pcm5102_stop(void)
{
}

// optional
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
// void audio_id3data(const char *info){  //id3 metadata
//     Serial.print("id3data     ");Serial.println(info);
// }
// void audio_eof_mp3(const char *info){  //end of file
//     Serial.print("eof_mp3     ");Serial.println(info);
// }
// void audio_showstation(const char *info){
//     Serial.print("station     ");Serial.println(info);
// }
// void audio_showstreamtitle(const char *info){
//     Serial.print("streamtitle ");Serial.println(info);
// }
// void audio_bitrate(const char *info){
//     Serial.print("bitrate     ");Serial.println(info);
// }
// void audio_commercial(const char *info){  //duration in sec
//     Serial.print("commercial  ");Serial.println(info);
// }
// void audio_icyurl(const char *info){  //homepage
//     Serial.print("icyurl      ");Serial.println(info);
// }
// void audio_lasthost(const char *info){  //stream URL played
//     Serial.print("lasthost    ");Serial.println(info);
// }

//************************************[ screen 12 ]****************************************** Motor
void ui_motor_loop(int i)
{
    if(i > 1 && i < 123) {
        // set the effect to play
        motor_drv.setWaveform(0, i);  // play effect 
        motor_drv.setWaveform(1, 0);       // end waveform

        // play the effect!
        motor_drv.go();
    }
}

void ui_motor_stop(void)
{
    motor_drv.stop();
}

void ui_system_sleep(void)
{
    extern TouchDrvCSTXXX touch;
    extern ExtensionIOXL9555 xl9555_io;
    touch.sleep();

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
        xl9555_io.digitalWrite(pin, LOW);
        delay(1);
    }

    const uint8_t pins[] = {
        BOARD_KEYBOARD_INT,
        BOARD_KEYBOARD_LED,

        BOARD_TOUCH_INT,

        BOARD_GYROSCOPDE_INT,

        BOARD_ES8311_MCLK,
        BOARD_ES8311_SCLK,
        BOARD_ES8311_ASDOUT,
        BOARD_ES8311_LRCK,
        BOARD_ES8311_DSDIN,

        BOARD_EPD_BL,
        BOARD_EPD_DC,
        BOARD_EPD_CS,
        BOARD_EPD_BUSY,
        BOARD_EPD_RST,

        BOARD_SD_CS,

        BOARD_LORA_CS,
        BOARD_LORA_BUSY,
        BOARD_LORA_RST,
        BOARD_LORA_INT,

        BOARD_GPS_RXD,
        BOARD_GPS_TXD,
        BOARD_GPS_PPS,

        BOARD_A7682E_RI,
        BOARD_A7682E_ITR,
        BOARD_A7682E_RXD,
        BOARD_A7682E_TXD,

        BOARD_BOOT_PIN,

        BOARD_I2C_SCL,
        BOARD_I2C_SDA,

        BOARD_SPI_SCK,
        BOARD_SPI_MOSI,
        BOARD_SPI_MISO,
    };

    for (auto pin : pins) {
        log_d("Set pin %d to open drain\n", pin);
        gpio_reset_pin((gpio_num_t )pin);
        pinMode(pin, OPEN_DRAIN);
    }

    lora_sleep();

    SerialMon.end();
    SerialAT.end();
    SerialGPS.end();

    Serial1.end();

    SPI.end();

    Wire.end();
    
    // gpio_reset_pin((gpio_num_t)BOARD_GPS_PPS);
    // gpio_reset_pin((gpio_num_t)BOARD_GPS_RXD);
    // gpio_reset_pin((gpio_num_t)BOARD_GPS_TXD);
    // gpio_reset_pin((gpio_num_t)BOARD_LORA_RST);
    // gpio_reset_pin((gpio_num_t)BOARD_TOUCH_RST);
    // gpio_reset_pin((gpio_num_t)BOARD_LORA_BUSY);

    // digitalWrite(BOARD_A7682E_PWRKEY, LOW);
    // gpio_hold_en((gpio_num_t)BOARD_A7682E_PWRKEY);
    // gpio_deep_sleep_hold_en();

    // esp_sleep_enable_ext0_wakeup((gpio_num_t)ENCODER_KEY, 0);                            
    esp_sleep_enable_ext1_wakeup((1UL << BOARD_BOOT_PIN), ESP_EXT1_WAKEUP_ANY_LOW);   // Hibernate using user keys
    esp_deep_sleep_start();
}