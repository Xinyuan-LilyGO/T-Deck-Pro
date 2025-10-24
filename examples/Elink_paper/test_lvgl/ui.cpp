
#include "ui.h"
#include "src/assets.h"
#include "stdio.h"
#include "Arduino.h"

//************************************[ UI ENTRY ]******************************************
#define MENU_BTN_NUM (sizeof(menu_btn_list) / sizeof(menu_btn_list[0]))
ui_indev_read_cb ui_get_gesture_dir = NULL;

static lv_obj_t *label;
static lv_obj_t *screen1;
static lv_obj_t *screen2;
static lv_obj_t *ui_Panel4;

struct menu_btn menu_btn_list[] = 
{
    {0, &img_lora, "icon0", 23, 23},
    {1, &img_lora, "icon1", 95, 23},
    {2, &img_lora, "icon2", 167, 23},
    {3, &img_lora, "icon3", 23, 111},
    {4, &img_lora, "icon4", 95, 111},
    {5, &img_lora, "icon5", 167, 111},
    {6, &img_lora, "icon6", 23, 199},
    {7, &img_lora, "icon7", 95, 199},
    {8, &img_lora, "icon8", 167, 199},

    // Page two
    {9, &img_GPS, "Lora9", 23, 23},
};

static void menu_btn_event_cb(lv_event_t *e)
{
    // struct menu_btn *tgr = (const char *)e->user_data;
    // lv_label_set_text(label, tgr->name);
}
static void menu_get_gesture_dir(int dir)
{
    static int page_curr = 0;
    int page_num = MENU_BTN_NUM;
    if(page_num < 9) return;

    if(dir == LV_DIR_LEFT) {
        Serial.printf("LV_DIR_LEFT\n");
        lv_obj_clear_flag(screen2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(screen1, LV_OBJ_FLAG_HIDDEN);

        lv_obj_set_style_bg_color(lv_obj_get_child(ui_Panel4, 0), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(lv_obj_get_child(ui_Panel4, 1), lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else if(dir == LV_DIR_RIGHT) {
        Serial.printf("LV_DIR_RIGHT\n");
        lv_obj_clear_flag(screen1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(screen2, LV_OBJ_FLAG_HIDDEN);

        lv_obj_set_style_bg_color(lv_obj_get_child(ui_Panel4, 0), lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(lv_obj_get_child(ui_Panel4, 1), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void menu_btn_create(lv_obj_t *parent, struct menu_btn *info)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_set_width(btn, 50);
    lv_obj_set_height(btn, 50);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_OVERFLOW_VISIBLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(btn, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_outline_width(btn, 3, LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_t *label = lv_label_create(btn);
    lv_obj_set_width(label, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(label, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(label, 0);
    lv_obj_set_y(label, 20);
    lv_obj_set_align(label, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_x(btn, info->pos_x);
    lv_obj_set_y(btn, info->pos_y);
    lv_obj_set_style_bg_img_src(btn, info->icon, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(label, info->name);
    lv_obj_set_style_border_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn, menu_btn_event_cb, LV_EVENT_CLICKED, (void *)info);
}

static void indev_get_gesture_dir(lv_timer_t *t)
{
    lv_indev_data_t data;
    lv_indev_t * indev_pointer = lv_indev_get_next(NULL);
    lv_coord_t diff_x = 0;
    lv_coord_t diff_y = 0;

    static lv_point_t last_point;
    static bool is_press = false;

    _lv_indev_read(indev_pointer, &data);

    if(data.state == LV_INDEV_STATE_PR){

        if(is_press == false) {
            is_press = true;
            last_point = data.point;
        }

        diff_x = last_point.x - data.point.x;
        diff_y = last_point.x - data.point.y;

        if(diff_x > 60) { // right
            if(ui_get_gesture_dir) {
                ui_get_gesture_dir(LV_DIR_LEFT);
            }
            last_point.x = 0;
        } else if(diff_x < -60) { // left
            if(ui_get_gesture_dir) {
                ui_get_gesture_dir(LV_DIR_RIGHT);
            }
            last_point.x = 0;
        }
        Serial.printf("x=%d, y=%d\n", data.point.x, data.point.y);
    }else{
        is_press = false;
        last_point.x = 0;
        last_point.y = 0;
    }
}

void ui_deckpro_entry(void)
{
    lv_obj_t * ui_Screen1;

    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    screen1 = lv_obj_create(ui_Screen1);
    lv_obj_set_size(screen1, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(screen1, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(screen1, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(screen1, 0, LV_PART_MAIN);
    lv_obj_set_style_border_color(screen1, DECKPRO_COLOR_FG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen1, 0, LV_PART_MAIN);
    // lv_obj_add_flag(screen2, LV_OBJ_FLAG_HIDDEN);

    screen2 = lv_obj_create(ui_Screen1);
    lv_obj_set_size(screen2, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(screen2, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(screen2, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(screen2, 0, LV_PART_MAIN);
    lv_obj_set_style_border_color(screen2, DECKPRO_COLOR_FG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen2, 0, LV_PART_MAIN);
    lv_obj_add_flag(screen2, LV_OBJ_FLAG_HIDDEN);

    for(int i = 0; i < MENU_BTN_NUM; i++) {
        if(i < 9) {
            menu_btn_create(screen1, &menu_btn_list[i]);
        } else {
            menu_btn_create(screen2, &menu_btn_list[i]);
        }
    }

    ui_Panel4 = lv_obj_create(ui_Screen1);
    lv_obj_set_width(ui_Panel4, 240);
    lv_obj_set_height(ui_Panel4, 42);
    lv_obj_set_align(ui_Panel4, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_flex_flow(ui_Panel4, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_Panel4, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(ui_Panel4, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_Panel4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Panel4, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Panel4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Panel4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_Panel4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_Panel4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Panel4, 0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_Panel4, 0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_Panel4, 0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);


    lv_obj_t *ui_Button11 = lv_btn_create(ui_Panel4);
    lv_obj_set_width(ui_Button11, 10);
    lv_obj_set_height(ui_Button11, 10);
    lv_obj_add_flag(ui_Button11, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_Button11, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Button11, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Button11, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Button11, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_Button11, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button11, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_Button11, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_Button11, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *ui_Button12 = lv_btn_create(ui_Panel4);
    lv_obj_set_width(ui_Button12, 10);
    lv_obj_set_height(ui_Button12, 10);
    lv_obj_add_flag(ui_Button12, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_Button12, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Button12, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Button12, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Button12, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_Button12, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button12, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_Button12, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_Button12, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // label = lv_label_create(ui_Screen1);
    // lv_obj_set_width(label, LV_SIZE_CONTENT);   /// 1
    // lv_obj_set_height(label, LV_SIZE_CONTENT);    /// 1
    // lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
    // lv_label_set_text(label, "---");

    lv_timer_create(indev_get_gesture_dir, LV_INDEV_DEF_READ_PERIOD, NULL);

    ui_get_gesture_dir = menu_get_gesture_dir;

    lv_scr_load(ui_Screen1);
}