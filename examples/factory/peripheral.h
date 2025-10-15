#ifndef __PERIPHERAL_H__
#define __PERIPHERAL_H__

#define GPS_PRIORITY     (configMAX_PRIORITIES - 1)
#define LORA_PRIORITY    (configMAX_PRIORITIES - 2)
#define WS2812_PRIORITY  (configMAX_PRIORITIES - 3)
#define BATTERY_PRIORITY (configMAX_PRIORITIES - 4)
#define A7682E_PRIORITY  (configMAX_PRIORITIES - 5)

enum {
    E_PERI_LORA = 0,
    E_PERI_TOUCH,
    E_PERI_KYEPAD,
    E_PERI_BQ25896,
    E_PERI_BQ27220,
    E_PERI_SD,
    E_PERI_GPS,
    E_PERI_BHI260AP,
    E_PERI_A7682E,
    E_PERI_PCM5102A,
    E_PERI_INK_SCREEN,
    E_PERI_MIC,
    E_PERI_NUM_MAX,
};

// lora sx1262
// #define LORA_FREQ      850.0
#define LORA_MODE_SEND 0
#define LORA_MODE_RECV 1

bool lora_init(void);
void lora_set_mode(int mode);
int lora_get_mode(void);
void lora_receive_loop(void);
void lora_transmit(const char *str);
bool lora_get_recv(const char **str, int *rssi);
void lora_set_recv_flag(void);
void lora_sleep(void);
void lora_param_set(void);
// keypad
#define KEYPAD_PRESS   1
#define KEYPAD_RELEASE 0

typedef void (*keypad_cb)(int state, char val);

bool keypad_init(int address);
int keypad_get_val(char *c);
void keypad_loop(void);
void keypad_regetser_cb(keypad_cb cb);
void keypad_set_flag(void);

// gyro
bool BHI260AP_init(void);
void BHI260AP_get_val(int val_type, float *x, float *y, float *z);

// gps u-blox m10q
bool gps_init(void);
void gps_task_create(void);
void gps_task_suspend(void);
void gps_task_resume(void);
void gps_get_coord(double *lat, double *lng);
void gps_get_data(uint16_t *year, uint8_t *month, uint8_t *day);
void gps_get_time(uint8_t *hour, uint8_t *minute, uint8_t *second);
void gps_get_satellites(uint32_t *vsat);
void gps_get_speed(double *speed);

#endif