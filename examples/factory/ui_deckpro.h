/************************************************************************
 * FilePath     : ui_base.h
 * Author       : GX.Duan
 * LastEditors  : ShallowGreen123 2608653986@qq.com
 * Copyright (c): 2022 by GX.Duan, All Rights Reserved.
 * Github       : https://github.com/ShallowGreen123/lvgl_examples.git
 ************************************************************************/
#ifndef __UI_DECKPRO_H__
#define __UI_DECKPRO_H__

#ifdef __cplusplus
extern "C" {
#endif
/*********************************************************************************
 *                                  INCLUDES
 * *******************************************************************************/
#include "lvgl.h"
#include "ui_scr_mrg.h"

/*********************************************************************************
 *                                   DEFINES
 * *******************************************************************************/
// #ifdef __has_include
//     #if __has_include("lv_i18n.h")
//         #ifndef LV_i18N_INCLUDE_SIMPLE
//             #define LV_i18N_INCLUDE_SIMPLE
//         #endif
//     #endif
// #endif

// #if defined(LV_i18N_INCLUDE_SIMPLE)
//     #include "lv_i18n.h"
// #else
//     #define _(text) (text)
//     #define _p(text, num) (text, num)
// #endif
/*********************************************************************************
 *                                   MACROS
 * *******************************************************************************/
#define DECKPRO_COLOR_BG        lv_color_white()
#define DECKPRO_COLOR_FG        lv_color_black()
#define UI_SLIDING_DISTANCE     40
#define UI_WIFI_SCAN_ITEM_MAX   13

/*********************************************************************************
 *                                  TYPEDEFS
 * *******************************************************************************/
enum {
    SCREEN0_ID = 0,
    SCREEN1_ID,
    SCREEN1_1_ID,
    SCREEN1_2_ID,
    SCREEN2_ID,
    SCREEN2_1_ID,
    SCREEN3_ID,
    SCREEN4_ID,
    SCREEN4_1_ID,
    SCREEN4_2_ID,
    SCREEN5_ID,
    SCREEN6_ID,
    SCREEN6_1_ID,
    SCREEN6_2_ID,
    SCREEN7_ID,
    SCREEN8_ID,
    SCREEN8_1_ID,
    SCREEN8_2_ID,
    SCREEN9_ID,
    SCREEN10_ID,
    SCREEN11_ID,
};

typedef void (*ui_indev_read_cb)(int);

enum {
    TASKBAR_ID_CHARGE,
    TASKBAR_ID_CHARGE_FINISH,
    TASKBAR_ID_BATTERY_PERCENT,
    TASKBAR_ID_WIFI,
    TASKBAR_ID_MAX,
};

struct menu_btn {
    uint16_t idx;
    const void *icon;
    const char *name;
    lv_coord_t pos_x;
    lv_coord_t pos_y; 
};

enum{
    UI_SETTING_TYPE_SW,
    UI_SETTING_TYPE_SUB,
};

typedef struct _ui_setting
{
    const char *name;
    int type;
    void (*set_cb)(bool);
    bool (*get_cb)(void);
    int sub_id;
    lv_obj_t *obj;
    lv_obj_t *st;
} ui_setting_handle;

typedef struct _ui_test {
    const char *name;
    int peri_id;
    lv_obj_t *obj;
    lv_obj_t *st;
    bool (*cb)(int);
} ui_test_handle;

typedef struct _ui_a7682 {
    const char *name;
    lv_obj_t *obj;
    lv_obj_t *st;
    bool (*cb)(const char *at_cmd);
} ui_a7682_handle;

typedef struct _ui_pcm5102 {
    const char *name;
    lv_obj_t *obj;
    lv_obj_t *st;
    bool (*cb)(const char *at_cmd);
} ui_pcm5102_handle;


typedef struct {
    char name[16];
    int rssi;
}ui_wifi_scan_info_t;
/*********************************************************************************
 *                              GLOBAL PROTOTYPES
 * *******************************************************************************/
void ui_deckpro_entry(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif /* __UI_EPD47H__ */
