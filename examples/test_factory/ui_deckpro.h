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
    SCREEN2_ID,
    SCREEN3_ID,
    SCREEN4_ID,
    SCREEN5_ID,
    SCREEN6_ID,
    SCREEN7_ID,
    SCREEN8_ID,
    SCREEN9_ID,
    SCREEN10_ID,
};

typedef void (*ui_indev_read_cb)(int);

struct menu_btn {
    uint16_t idx;
    const void *icon;
    const char *name;
    lv_coord_t pos_x;
    lv_coord_t pos_y; 
};

typedef struct _ui_setting
{
    const char *name;
    lv_obj_t *obj;
    lv_obj_t *st;
    void (*set_cb)(bool);
    bool (*get_cb)(void);
}_ui_setting_handle;

typedef struct _ui_test {
    const char *name;
    bool (*cb)(void);
} ui_test_handle;

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