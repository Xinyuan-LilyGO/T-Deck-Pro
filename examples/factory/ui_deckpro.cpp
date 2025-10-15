
#include "ui_deckpro.h"
#include "src/assets.h"
#include "stdio.h"
#include "ui_deckpro_port.h"
#include "Arduino.h"

#define SETTING_PAGE_MAX_ITEM 7
#define GET_BUFF_LEN(a) sizeof(a)/sizeof(a[0])

#define FONT_BOLD_SIZE_14 &Font_Mono_Bold_14
#define FONT_BOLD_SIZE_15 &Font_Mono_Bold_15
#define FONT_BOLD_SIZE_16 &Font_Mono_Bold_16
#define FONT_BOLD_SIZE_17 &Font_Mono_Bold_17
#define FONT_BOLD_SIZE_18 &Font_Mono_Bold_18
#define FONT_BOLD_SIZE_19 &Font_Mono_Bold_19

#define FONT_BOLD_MONO_SIZE_14 &Font_Mono_Bold_14
#define FONT_BOLD_MONO_SIZE_15 &Font_Mono_Bold_15
#define FONT_BOLD_MONO_SIZE_16 &Font_Mono_Bold_16
#define FONT_BOLD_MONO_SIZE_17 &Font_Mono_Bold_17
#define FONT_BOLD_MONO_SIZE_18 &Font_Mono_Bold_18
#define FONT_BOLD_MONO_SIZE_19 &Font_Mono_Bold_19

#define GLOBAL_BUF_LEN 30
static char global_buf[GLOBAL_BUF_LEN];

static lv_timer_t *touch_chk_timer = NULL;
static lv_timer_t *taskbar_update_timer = NULL;
static lv_obj_t *label_list[10] = {0};
uint16_t taskbar_statue[TASKBAR_ID_MAX] = {0};

//************************************[ Other fun ]******************************************
#if 1
static lv_obj_t *scr_back_btn_create(lv_obj_t *parent, const char *text, lv_event_cb_t cb)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_height(btn, 30);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 3, 3);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, DECKPRO_COLOR_FG, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label2 = lv_label_create(btn);
    lv_obj_align(label2, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_color(label2, DECKPRO_COLOR_FG, LV_PART_MAIN);
    lv_label_set_text(label2, LV_SYMBOL_LEFT);

    lv_obj_t *label = lv_label_create(parent);
    lv_obj_align_to(label, label2, LV_ALIGN_OUT_RIGHT_MID, 5, -1);
    lv_obj_set_style_text_font(label, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, DECKPRO_COLOR_FG, LV_PART_MAIN);
    lv_label_set_text(label, text);
    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_ext_click_area(label, 20);

    return label;
}

static const char *line_full_format(int max_c, const char *str1, const char *str2)
{
    int len1 = 0, len2 = 0;
    int j;

    len1 = strlen(str1);

    strncpy(global_buf, str1, len1);

    len2 = strlen(str2);
    for(j = len1; j < max_c -1 - len2; j++){
        global_buf[j] = ' ';
    }
    strncpy(global_buf + j, str2, len2);
    j = j + len2;
    
    global_buf[j] = '\0'; 

    printf("[%d] buf: %s\n", __LINE__, global_buf);

    return (const char *)global_buf;
}

#endif
//************************************[ screen 0 ]****************************************** menu
#if 1
#define MENU_BTN_NUM (sizeof(menu_btn_list) / sizeof(menu_btn_list[0]))

static ui_indev_read_cb ui_get_gesture_dir = NULL;

static lv_obj_t *menu_screen1;
static lv_obj_t *menu_screen2;
static lv_obj_t *ui_Panel4;

static lv_obj_t * menu_taskbar = NULL;
static lv_obj_t * menu_taskbar_time = NULL;
static lv_obj_t * menu_taskbar_charge = NULL;
static lv_obj_t * menu_taskbar_battery = NULL;
static lv_obj_t * menu_taskbar_battery_percent = NULL;
static lv_obj_t * menu_taskbar_wifi = NULL;

static int page_num = 0;
static int page_curr = 0;

static struct menu_btn menu_btn_list[] = 
{
    {SCREEN1_ID,  &img_lora,    "Lora",     23,     13},  // Page one
    {SCREEN2_ID,  &img_setting, "Setting",  95,     13},
    {SCREEN3_ID,  &img_GPS,     "GPS",      167,    13},
    {SCREEN4_ID,  &img_wifi,    "Wifi",     23,     101},
    {SCREEN5_ID,  &img_test,    "Test",     95,     101},
    {SCREEN6_ID,  &img_batt,    "Battery",  167,    101},
    {SCREEN7_ID,  &img_touch,   "Input",    23,     189},
    {SCREEN8_ID,  &img_A7682E,  "A7682E",   95,     189},
    {SCREEN9_ID,  &img_lora,    "Shutdown", 167,    189},
    {SCREEN10_ID, &img_PCM5102, "PCM5102",  23,     13},  // Page two
    {SCREEN11_ID, &img_PCM5102, "Sleep",    95,     13},  // 
};

static void menu_btn_event_cb(lv_event_t *e)
{
    struct menu_btn *tgr = (struct menu_btn *)e->user_data;
    scr_mgr_push(tgr->idx, false);
}

static void menu_get_gesture_dir(int dir)
{
    if(MENU_BTN_NUM <= 9) return;

    if(dir == LV_DIR_LEFT) {
        if(page_curr < page_num){
            page_curr++;
            // ui_disp_full_refr();
        }
        else{
            return ;
        }
    } else if(dir == LV_DIR_RIGHT) {
        if(page_curr > 0){
            page_curr--;
        }
        else{
            return ;
        }
    }   

    if(page_curr == 1) {
        lv_obj_clear_flag(menu_screen2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(menu_screen1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(lv_obj_get_child(ui_Panel4, 0), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(lv_obj_get_child(ui_Panel4, 1), lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    } else if(page_curr == 0) {
        lv_obj_clear_flag(menu_screen1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(menu_screen2, LV_OBJ_FLAG_HIDDEN);
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
    lv_obj_set_style_text_font(label, FONT_BOLD_MONO_SIZE_14, LV_PART_MAIN);
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
    lv_label_set_text(label, (info->name));
    lv_obj_set_style_border_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn, menu_btn_event_cb, LV_EVENT_CLICKED, (void *)info);
}

static void create0(lv_obj_t *parent) 
{
    int status_bar_height = 25;

    menu_taskbar = lv_obj_create(parent);
    lv_obj_set_size(menu_taskbar, LV_HOR_RES, status_bar_height);
    lv_obj_set_style_pad_all(menu_taskbar, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(menu_taskbar, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(menu_taskbar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(menu_taskbar, LV_OBJ_FLAG_SCROLLABLE);
    
    menu_taskbar_time = lv_label_create(menu_taskbar);
    lv_obj_set_style_border_width(menu_taskbar_time, 0, 0);
    lv_label_set_text_fmt(menu_taskbar_time, "%02d:%02d", 10, 19);
    lv_obj_set_style_text_font(menu_taskbar_time, &Font_Mono_Bold_14, LV_PART_MAIN);
    lv_obj_align(menu_taskbar_time, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t *status_parent = lv_obj_create(menu_taskbar);
    lv_obj_set_size(status_parent, lv_pct(80)-2, status_bar_height-2);
    lv_obj_set_style_pad_all(status_parent, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_parent, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(status_parent, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_parent, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(status_parent, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(status_parent, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(status_parent, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(status_parent, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(status_parent, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(status_parent, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(status_parent, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(status_parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(status_parent, LV_ALIGN_RIGHT_MID, 0, 0);

    menu_taskbar_wifi = lv_label_create(status_parent);
    lv_label_set_text_fmt(menu_taskbar_wifi, "%s", LV_SYMBOL_WIFI);
    lv_obj_add_flag(menu_taskbar_wifi, LV_OBJ_FLAG_HIDDEN);

    menu_taskbar_charge = lv_label_create(status_parent);
    lv_label_set_text_fmt(menu_taskbar_charge, "%s", LV_SYMBOL_CHARGE);
    lv_obj_add_flag(menu_taskbar_charge, LV_OBJ_FLAG_HIDDEN);

    if(taskbar_statue[TASKBAR_ID_WIFI])
        lv_obj_clear_flag(menu_taskbar_wifi, LV_OBJ_FLAG_HIDDEN);

    if(taskbar_statue[TASKBAR_ID_CHARGE])
        lv_obj_clear_flag(menu_taskbar_charge, LV_OBJ_FLAG_HIDDEN);

    menu_taskbar_battery = lv_label_create(status_parent);
    
    menu_taskbar_battery_percent = lv_label_create(status_parent);
    lv_obj_set_style_text_font(menu_taskbar_battery_percent, &Font_Mono_Bold_14, LV_PART_MAIN);

    //
    page_num = MENU_BTN_NUM / 9;

    menu_screen1 = lv_obj_create(parent);
    lv_obj_set_size(menu_screen1, lv_pct(100), LV_VER_RES - status_bar_height);
    lv_obj_set_style_bg_color(menu_screen1, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(menu_screen1, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(menu_screen1, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(menu_screen1, DECKPRO_COLOR_FG, LV_PART_MAIN);
    lv_obj_set_style_border_side(menu_screen1, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(menu_screen1, 0, LV_PART_MAIN);
    lv_obj_align(menu_screen1, LV_ALIGN_BOTTOM_MID, 0, 0);
    // lv_obj_add_flag(menu_screen1, LV_OBJ_FLAG_HIDDEN);

    menu_screen2 = lv_obj_create(parent);
    lv_obj_set_size(menu_screen2, lv_pct(100), LV_VER_RES - status_bar_height);
    lv_obj_set_style_bg_color(menu_screen2, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(menu_screen2, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(menu_screen2, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(menu_screen2, DECKPRO_COLOR_FG, LV_PART_MAIN);
    lv_obj_set_style_border_side(menu_screen2, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(menu_screen2, 0, LV_PART_MAIN);
    lv_obj_align(menu_screen2, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(menu_screen2, LV_OBJ_FLAG_HIDDEN);

    if(ui_test_a7682e() == false)
    {
        for(int i = 0; i < GET_BUFF_LEN(menu_btn_list); i++)
        {
            if(menu_btn_list[i].idx == SCREEN8_ID)
            {
                menu_btn_list[i].idx = SCREEN10_ID;
                menu_btn_list[i].name = "PCM5012";
                menu_btn_list[i].icon = &img_PCM5102;
            }
        }
    }

    for(int i = 0; i < MENU_BTN_NUM; i++) {
        if(i < 9) {
            menu_btn_create(menu_screen1, &menu_btn_list[i]);
        } else {
            menu_btn_create(menu_screen2, &menu_btn_list[i]);
        }
    }

    if(MENU_BTN_NUM > 9) {
        ui_Panel4 = lv_obj_create(parent);
        lv_obj_set_width(ui_Panel4, 240);
        lv_obj_set_height(ui_Panel4, 25);
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
        lv_obj_clear_flag(ui_Button11, LV_OBJ_FLAG_CHECKABLE);      /// Flags
        lv_obj_set_style_radius(ui_Button11, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_Button11, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_Button11, DECKPRO_COLOR_FG, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *ui_Button12 = lv_btn_create(ui_Panel4);
        lv_obj_set_width(ui_Button12, 10);
        lv_obj_set_height(ui_Button12, 10);
        lv_obj_add_flag(ui_Button12, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
        lv_obj_clear_flag(ui_Button12, LV_OBJ_FLAG_CHECKABLE);      /// Flags
        lv_obj_set_style_radius(ui_Button12, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_Button12, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void entry0(void) {
    ui_get_gesture_dir = menu_get_gesture_dir;
    lv_timer_resume(touch_chk_timer);
    lv_timer_resume(taskbar_update_timer);

    lv_label_set_text_fmt(menu_taskbar_battery, "%s", ui_battert_27220_get_percent_level());

    lv_label_set_text_fmt(menu_taskbar_battery_percent, "%d", ui_battery_27220_get_percent());
}
static void exit0(void) {
    ui_get_gesture_dir = NULL;
    lv_timer_pause(touch_chk_timer);
    lv_timer_pause(taskbar_update_timer);
}
static void destroy0(void) {
    if(menu_taskbar) {
        lv_obj_del(menu_taskbar);
        menu_taskbar = NULL;
    }
}

static scr_lifecycle_t screen0 = {
    .create = create0,
    .entry = entry0,
    .exit  = exit0,
    .destroy = destroy0,
};
#endif
//************************************[ screen 1 ]****************************************** lora
// --------------------- screen 1 --------------------- lora
#if 1
lv_obj_t * scr1_list;
static lv_obj_t *scr1_lab_buf[20];

static void scr1_list_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    for(int i = 0; i < lv_obj_get_child_cnt(obj); i++) 
    {
        lv_obj_t * child = lv_obj_get_child(obj, i);
        if(lv_obj_check_type(child, &lv_label_class)) {
            char *str = lv_label_get_text(child);

            if(strcmp("- Auto Test", str) == 0)
            {
                scr_mgr_push(SCREEN1_1_ID, false);
            }
            if(strcmp("- Lora Setting", str) == 0)
            {
                scr_mgr_push(SCREEN1_2_ID, false);
            }
            printf("%s\n", str);
        }
    }
}

static void scr1_item_create(const char *name, lv_event_cb_t cb)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&lv_list_btn_class, scr1_list);
    lv_obj_class_init_obj(obj);
    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, name);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_set_height(obj, LV_VER_RES / 6);
    lv_obj_set_style_text_font(obj, FONT_BOLD_SIZE_15, LV_PART_MAIN);
    // lv_obj_set_style_bg_color(obj, lv_color_hex(EPD_COLOR_BG), LV_PART_MAIN);
    // lv_obj_set_style_text_color(obj, lv_color_hex(EPD_COLOR_FG), LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_outline_width(obj, 1, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(obj, cb, LV_EVENT_CLICKED, NULL); 
}

static void scr1_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // ui_full_refresh();
        scr_mgr_pop(false);
    }
}

static void create1(lv_obj_t *parent) 
{
    scr1_list = lv_list_create(parent);
    lv_obj_set_size(scr1_list, lv_pct(93), lv_pct(91));
    lv_obj_align(scr1_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    // lv_obj_set_style_bg_color(scr1_list, lv_color_hex(EPD_COLOR_BG), LV_PART_MAIN);
    lv_obj_set_style_pad_top(scr1_list, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_row(scr1_list, 15, LV_PART_MAIN);
    lv_obj_set_style_radius(scr1_list, 0, LV_PART_MAIN);
    // lv_obj_set_style_outline_pad(scr1_list, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr1_list, 0, LV_PART_MAIN);
    // lv_obj_set_style_border_color(scr1_list, lv_color_hex(EPD_COLOR_FG), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(scr1_list, 0, LV_PART_MAIN);

    scr1_item_create("- Auto Test", scr1_list_event);
    scr1_item_create("- Lora Setting", scr1_list_event);

    // back
    scr_back_btn_create(parent, "Lora", scr1_btn_event_cb);
}

static void entry1(void) 
{
    ui_disp_full_refr();
}
static void exit1(void) {
    ui_disp_full_refr();
}
static void destroy1(void) { }

static scr_lifecycle_t screen1 = {
    .create = create1,
    .entry = entry1,
    .exit  = exit1,
    .destroy = destroy1,
};
#endif
// --------------------- screen 1.1 --------------------- Auto Send
#if 1
static lv_obj_t *scr1_1_cont;
static lv_obj_t *lora_lab_buf[11] = {0};
static lv_obj_t *lora_sw_btn;
static lv_obj_t *lora_sw_btn_info;
static lv_timer_t *lora_RT_timer = NULL;
static lv_timer_t *lora_recv_timer = NULL;
static int lora_cnt = 0;

static void scr1_1_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void lora_mode_sw_event(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        if(ui_lora_get_mode() == LORA_MODE_SEND) {
            ui_lora_set_mode(LORA_MODE_RECV);
            lv_label_set_text(lora_sw_btn_info, "Recv");
            for(int i = 0; i < GET_BUFF_LEN(lora_lab_buf); i++){
                lv_label_set_text_fmt(lora_lab_buf[i], " ", i);
            }
            lora_cnt = 0;
        } else if(ui_lora_get_mode() == LORA_MODE_RECV) {
            ui_lora_set_mode(LORA_MODE_SEND);
            lv_label_set_text(lora_sw_btn_info, "Send");
            for(int i = 0; i < GET_BUFF_LEN(lora_lab_buf); i++){
                lv_label_set_text_fmt(lora_lab_buf[i], " ", i);
            }
            lora_cnt = 0;
        }
    }
}

static void lora_recv_loop_event(lv_timer_t *t)
{
    ui_lora_recv_loop();
}

static void lora_RT_timer_event(lv_timer_t *t)
{
    static int data = 0;
    char buf[32];
    const char *recv_info = NULL;
    int recv_rssi = 0;
    
    if(ui_lora_get_mode() == LORA_MODE_SEND) 

    {
        lv_snprintf(buf, 32, "DeckPro #%d", data++);
        lv_label_set_text_fmt(lora_lab_buf[lora_cnt], "send-> %s", buf);
        ui_lora_send(buf);

        lora_cnt++;
        if(lora_cnt >= GET_BUFF_LEN(lora_lab_buf)) {
            lora_cnt = 0;
        }
    }
    else if(ui_lora_get_mode() == LORA_MODE_RECV)
    {
        if(ui_lora_get_recv(&recv_info, &recv_rssi))
        {
            ui_lora_set_recv_flag();
            lv_label_set_text_fmt(lora_lab_buf[lora_cnt], "recv-> %s [%d]", recv_info, recv_rssi);

            lora_cnt++;
            if(lora_cnt >= GET_BUFF_LEN(lora_lab_buf)) {
                lora_cnt = 0;
            }
        }
    }
}

static lv_obj_t * scr2_create_label(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_width(label, LV_HOR_RES - 26);
    lv_obj_set_style_text_font(label, FONT_BOLD_SIZE_15, LV_PART_MAIN);   
    lv_obj_set_style_border_width(label, 0, LV_PART_MAIN);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    return label;
}
static void create1_1(lv_obj_t *parent) 
{
    scr1_1_cont = lv_obj_create(parent);
    lv_obj_set_size(scr1_1_cont, lv_pct(100), lv_pct(85));
    lv_obj_set_style_bg_color(scr1_1_cont, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(scr1_1_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr1_1_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(scr1_1_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr1_1_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(scr1_1_cont, 13, LV_PART_MAIN);
    lv_obj_set_flex_flow(scr1_1_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(scr1_1_cont, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_column(scr1_1_cont, 5, LV_PART_MAIN);
    lv_obj_set_align(scr1_1_cont, LV_ALIGN_BOTTOM_MID);

    for(int i = 0; i < GET_BUFF_LEN(lora_lab_buf); i++){
        lora_lab_buf[i] = scr2_create_label(scr1_1_cont);
        lv_label_set_text_fmt(lora_lab_buf[i], " ", i);
    }

    lora_sw_btn = lv_btn_create(parent);
    lv_obj_set_size(lora_sw_btn, 70, 25);
    lv_obj_set_style_radius(lora_sw_btn, 5, LV_PART_MAIN);
    lv_obj_set_style_border_width(lora_sw_btn, 2, LV_PART_MAIN);
    lora_sw_btn_info = lv_label_create(lora_sw_btn);
    lv_obj_set_style_text_font(lora_sw_btn_info, FONT_BOLD_SIZE_15, LV_PART_MAIN);
    lv_obj_set_style_text_align(lora_sw_btn_info, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lora_sw_btn_info, "Send");
    lv_obj_center(lora_sw_btn_info);
    lv_obj_align(lora_sw_btn, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_add_event_cb(lora_sw_btn, lora_mode_sw_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lab = lv_label_create(parent);
    lv_obj_set_style_text_font(lab, FONT_BOLD_SIZE_15, LV_PART_MAIN);
    lv_label_set_text_fmt(lab, "%.1fM", ui_lora_get_freq());
    lv_obj_align(lab, LV_ALIGN_TOP_RIGHT, -10, 10);

    ui_lora_set_mode(LORA_MODE_SEND);
    lora_cnt = 0;

    // back
    scr_back_btn_create(parent, ("Lora"), scr1_1_btn_event_cb);
}
static void entry1_1(void) 
{
    ui_disp_full_refr();
    lora_RT_timer = lv_timer_create(lora_RT_timer_event, 2000, NULL);
    lora_recv_timer = lv_timer_create(lora_recv_loop_event, 400, NULL);
}
static void exit1_1(void) {
    ui_disp_full_refr();
    if(lora_RT_timer) {
        lv_timer_del(lora_RT_timer);
        lora_RT_timer = NULL;
    }
    if(lora_recv_timer) {
        lv_timer_del(lora_recv_timer);
        lora_recv_timer = NULL;
    }
}
static void destroy1_1(void) { }

static scr_lifecycle_t screen1_1 = {
    .create = create1_1,
    .entry = entry1_1,
    .exit  = exit1_1,
    .destroy = destroy1_1,
};
#endif
// --------------------- screen 1.2 --------------------- Lora Setting
#if 1

#define RADIO_FREQUENCY_LIST "433MHz\n 850MHz\n 868MHz\n 915MHz\n 920MHz"
#define RADIO_BANDWIDTH "125KHz\n 250KHz\n 500KHz"
#define RADIO_TX_POWER "10dBm\n 22dBm"

static float lora_freq_list[] = {433.0, 850.0, 868.0, 915.0, 920.0};
static int lora_band_list[] = {125, 250, 500};
static int lora_power_list[] = {10, 22};

static lv_obj_t *scr1_2_cont;
static lv_obj_t *dropdown_freq;
static lv_obj_t *dropdown_band;
static lv_obj_t *dropdown_power;

static void scr1_2_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void lora_setting_event_handler(lv_event_t * e)
{
    char buf[32]={0};
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    const char *flag = ( const char *)lv_event_get_user_data(e);
    int select = lv_dropdown_get_selected(obj);

    lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
    switch (*flag)
    {
    case 'f': 
        for(int i = 0; i < GET_BUFF_LEN(lora_freq_list); i++) {
            if(lora_freq_list[select] == lora_freq_list[i]) {
                printf("set freq %.1fMHz\n", lora_freq_list[i]);
                ui_lora_set_freq(lora_freq_list[i]);
            }
        }
        break;
    case 'b': 
        for(int i = 0; i < GET_BUFF_LEN(lora_band_list); i++) {
            if(lora_band_list[select] == lora_band_list[i]) {
                printf("set bandwidth %dKhz\n", lora_band_list[i]);
                ui_lora_set_bandwidth(lora_band_list[i]);
            }
        }
        break;
    case 'p': 
        for(int i = 0; i < GET_BUFF_LEN(lora_power_list); i++) {
            if(lora_power_list[select] == lora_power_list[i]) {
                printf("set power %ddBm\n", lora_power_list[i]);
                ui_lora_set_power(lora_power_list[i]);
            }
        }
        break;
    
    default:
        break;
    }
}

static lv_obj_t * scr1_2_lora_setting_create(lv_obj_t *parent, const char *text)
{
    lv_obj_t *ui_Container1 = lv_obj_create(parent);
    lv_obj_remove_style_all(ui_Container1);
    lv_obj_set_height(ui_Container1, 42);
    lv_obj_set_width(ui_Container1, lv_pct(100));
    lv_obj_set_x(ui_Container1, 35);
    lv_obj_set_y(ui_Container1, -16);
    lv_obj_set_align(ui_Container1, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_Container1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_Container1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(ui_Container1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_pad_row(ui_Container1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(ui_Container1, 20, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *ui_Label14 = lv_label_create(ui_Container1);
    lv_obj_set_width(ui_Label14, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label14, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label14, -60);
    lv_obj_set_y(ui_Label14, -42);
    lv_obj_set_align(ui_Label14, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label14, text);
    lv_obj_set_style_text_font(ui_Label14, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);   

    lv_obj_t *ui_Dropdown1 = lv_dropdown_create(ui_Container1);
    lv_obj_set_width(ui_Dropdown1, lv_pct(60));
    lv_obj_set_height(ui_Dropdown1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Dropdown1, 19);
    lv_obj_set_y(ui_Dropdown1, -1);
    lv_obj_add_flag(ui_Dropdown1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags

    // lv_obj_set_style_bg_opa(ui_Dropdown1, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(ui_Dropdown1, 1, LV_PART_MAIN | LV_STATE_PRESSED);
    // lv_obj_set_style_shadow_width(ui_Dropdown1, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);

    return ui_Dropdown1;
}

static void create1_2(lv_obj_t *parent) 
{
    scr1_2_cont = lv_obj_create(parent);
    lv_obj_remove_style_all(scr1_2_cont);
    lv_obj_set_width(scr1_2_cont, lv_pct(100));
    lv_obj_set_height(scr1_2_cont, lv_pct(85));
    lv_obj_set_align(scr1_2_cont, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(scr1_2_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr1_2_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(scr1_2_cont, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_pad_row(scr1_2_cont, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(scr1_2_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_border_width(scr1_2_cont, 3, LV_PART_MAIN);
    lv_obj_set_align(scr1_2_cont, LV_ALIGN_BOTTOM_MID);

    dropdown_freq = scr1_2_lora_setting_create(scr1_2_cont, "Freq: ");
    lv_dropdown_set_options(dropdown_freq, RADIO_FREQUENCY_LIST);
    for(int i = 0; i < GET_BUFF_LEN(lora_freq_list); i++) {
        if(ui_lora_get_freq() == lora_freq_list[i]) {
            lv_dropdown_set_selected(dropdown_freq, i);
        }
    }

    dropdown_band = scr1_2_lora_setting_create(scr1_2_cont, "Band: ");
    lv_dropdown_set_options(dropdown_band, RADIO_BANDWIDTH);
    for(int i = 0; i < GET_BUFF_LEN(lora_band_list); i++) {
        if(ui_lora_get_bandwidth() == lora_band_list[i]) {
            lv_dropdown_set_selected(dropdown_band, i);
        }
    }

    dropdown_power = scr1_2_lora_setting_create(scr1_2_cont, "Power:");
    lv_dropdown_set_options(dropdown_power, RADIO_TX_POWER);
    for(int i = 0; i < GET_BUFF_LEN(lora_power_list); i++) {
        if(ui_lora_get_power() == lora_power_list[i]) {
            lv_dropdown_set_selected(dropdown_power, i);
        }
    }
    static const char freq_flag = 'f';
    static const char band_flag = 'b';
    static const char power_flag = 'p';
    lv_obj_add_event_cb(dropdown_freq, lora_setting_event_handler, LV_EVENT_VALUE_CHANGED, (void *)&freq_flag);
    lv_obj_add_event_cb(dropdown_band, lora_setting_event_handler, LV_EVENT_VALUE_CHANGED, (void *)&band_flag);
    lv_obj_add_event_cb(dropdown_power,   lora_setting_event_handler, LV_EVENT_VALUE_CHANGED, (void *)&power_flag);
    // back
    scr_back_btn_create(parent, ("Lora Setting"), scr1_2_btn_event_cb);
}
static void entry1_2(void) 
{
    ui_disp_full_refr();
}
static void exit1_2(void) {
    ui_disp_full_refr();
    ui_lora_param_set();
}
static void destroy1_2(void) { }

static scr_lifecycle_t screen1_2 = {
    .create = create1_2,
    .entry = entry1_2,
    .exit  = exit1_2,
    .destroy = destroy1_2,
};
#endif
//************************************[ screen 2 ]****************************************** Setting
// --------------------- screen 2.1 --------------------- About System
#if 1
static lv_obj_t *scr2_1_cont;

static void scr2_1_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void create2_1(lv_obj_t *parent) 
{
    lv_obj_t *info = lv_label_create(parent);
    lv_obj_set_width(info, LV_HOR_RES * 0.9);
    lv_obj_set_style_text_color(info, DECKPRO_COLOR_FG, LV_PART_MAIN);
    lv_obj_set_style_text_font(info, &Font_Mono_Bold_14, LV_PART_MAIN);
    lv_obj_set_style_text_align(info, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(info, LV_LABEL_LONG_WRAP);

    String str = "";

    str += "                           \n";
    str += line_full_format(28, "SF Version:", ui_setting_get_sf_ver());
    str += "\n                           \n";

    str += line_full_format(28, "HD Version:", ui_setting_get_hd_ver());
    str += "\n                           \n";

    char buf[30];
    uint64_t total=0, used=0;
    ui_setting_get_sd_capacity(&total, &used);
    lv_snprintf(buf, 30, "%lluMB", total);
    str += line_full_format(28, "SD total:", (const char *)buf);
    str += "\n                           \n";

    lv_snprintf(buf, 30, "%lluMB", used);
    str += line_full_format(28, "SD used:", (const char *)buf);
    str += "\n                           \n";


    lv_label_set_text_fmt(info, "%s", str.c_str());
    
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 35);
    
    lv_obj_t *back2_1_label = scr_back_btn_create(parent, ("About System"), scr2_1_btn_event_cb);
}
static void entry2_1(void) 
{
    ui_disp_full_refr();
}
static void exit2_1(void) {
    ui_disp_full_refr();
}
static void destroy2_1(void) { }

static scr_lifecycle_t screen2_1 = {
    .create = create2_1,
    .entry = entry2_1,
    .exit  = exit2_1,
    .destroy = destroy2_1,
};
#endif
// --------------------- screen 2 --------------------- Setting
#if 1
static lv_obj_t *setting_list;
static lv_obj_t *setting_page;
static int setting_num = 0;
static int setting_page_num = 0;
static int setting_curr_page = 0;
static ui_setting_handle setting_handle_list[] = {
    {.name = "Keypad Backlight", .type=UI_SETTING_TYPE_SW,  .set_cb = ui_setting_set_keypad_light, .get_cb = ui_setting_get_keypad_light},
    {.name = "Motor Status",     .type=UI_SETTING_TYPE_SW,  .set_cb = ui_setting_set_motor_status, .get_cb = ui_setting_get_motor_status},
    {.name = "Power GPS",        .type=UI_SETTING_TYPE_SW,  .set_cb = ui_setting_set_gps_status,   .get_cb = ui_setting_get_gps_status},
    {.name = "Power Lora",       .type=UI_SETTING_TYPE_SW,  .set_cb = ui_setting_set_lora_status,  .get_cb = ui_setting_get_lora_status},
    {.name = "Power Gyro",       .type=UI_SETTING_TYPE_SW,  .set_cb = ui_setting_set_gyro_status,  .get_cb = ui_setting_get_gyro_status},
    {.name = "Power A7682",      .type=UI_SETTING_TYPE_SW,  .set_cb = ui_setting_set_a7682_status, .get_cb = ui_setting_get_a7682_status},
    {.name = "- About System",   .type=UI_SETTING_TYPE_SUB, .sub_id = SCREEN2_1_ID},
};

static void setting_item_create(int curr_apge);

static void scr2_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void setting_scr_event(lv_event_t *e)
{
    lv_obj_t *tgt = (lv_obj_t *)e->target;
    ui_setting_handle *h = (ui_setting_handle *)e->user_data;

    if(e->code == LV_EVENT_CLICKED) {
        switch (h->type)
        {
        case UI_SETTING_TYPE_SW:
            h->set_cb(!h->get_cb());
            lv_label_set_text_fmt(h->st, "%s", (h->get_cb() ? "ON" : "OFF"));
            break;
        case UI_SETTING_TYPE_SUB:
            scr_mgr_push(h->sub_id, false);
            break;
        default:
            break;
        }
    }
}

static void setting_page_switch_cb(lv_event_t *e)
{
    char opt = (int)e->user_data;
    
    if(setting_num < SETTING_PAGE_MAX_ITEM) return;

    int child_cnt = lv_obj_get_child_cnt(setting_list);
    
    for(int i = 0; i < child_cnt; i++)
    {
        lv_obj_t *child = lv_obj_get_child(setting_list, 0);
        if(child)
            lv_obj_del(child);
    }

    if(opt == 'p')
    {
        setting_curr_page = (setting_curr_page < setting_page_num) ? setting_curr_page + 1 : 0;
    }
    else if(opt == 'n')
    {
        setting_curr_page = (setting_curr_page > 0) ? setting_curr_page - 1 : setting_page_num;
    }

    setting_item_create(setting_curr_page);
    lv_label_set_text_fmt(setting_page, "%d / %d", setting_curr_page, setting_page_num);
}

static void setting_item_create(int curr_apge)
{
    printf("setting_curr_page = %d\n", setting_curr_page);
    int start = (curr_apge * SETTING_PAGE_MAX_ITEM);
    int end = start + SETTING_PAGE_MAX_ITEM;
    if(end > setting_num) end = setting_num;

    printf("start=%d, end=%d\n", start, end);

    for(int i = start; i < end; i++) {
        ui_setting_handle *h = &setting_handle_list[i];
        

        switch (h->type)
        {
        case UI_SETTING_TYPE_SW:
            h->obj = lv_list_add_btn(setting_list, NULL, h->name);
            h->st = lv_label_create(h->obj);
            lv_obj_set_style_text_font(h->st, FONT_BOLD_SIZE_15, LV_PART_MAIN);
            lv_obj_align(h->st, LV_ALIGN_RIGHT_MID, 0, 0);
            lv_label_set_text_fmt(h->st, "%s", (h->get_cb() ? "ON" : "OFF"));
            break;
        case UI_SETTING_TYPE_SUB:
            h->obj = lv_list_add_btn(setting_list, NULL, h->name);
            break;
        default:
            break;
        }

        // style
        lv_obj_set_height(h->obj, 28);
        lv_obj_set_style_text_font(h->obj, FONT_BOLD_SIZE_14, LV_PART_MAIN);
        lv_obj_set_style_bg_color(h->obj, DECKPRO_COLOR_BG, LV_PART_MAIN);
        lv_obj_set_style_text_color(h->obj, DECKPRO_COLOR_FG, LV_PART_MAIN);
        lv_obj_set_style_border_width(h->obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(h->obj, 1, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_outline_width(h->obj, 3, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_radius(h->obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(h->obj, setting_scr_event, LV_EVENT_CLICKED, (void *)h);
    }
}

static void create2(lv_obj_t *parent) 
{
    setting_list = lv_list_create(parent);
    lv_obj_set_size(setting_list, LV_HOR_RES, lv_pct(88));
    lv_obj_align(setting_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(setting_list, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_top(setting_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(setting_list, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(setting_list, 0, LV_PART_MAIN);
    // lv_obj_set_style_outline_pad(setting_list, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(setting_list, 0, LV_PART_MAIN);
    lv_obj_set_style_border_color(setting_list, DECKPRO_COLOR_FG, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(setting_list, 0, LV_PART_MAIN);

    setting_num = sizeof(setting_handle_list) / sizeof(setting_handle_list[0]);
    setting_page_num = setting_num / SETTING_PAGE_MAX_ITEM;

    setting_item_create(setting_curr_page);

    lv_obj_t * ui_Button2 = lv_btn_create(parent);
    lv_obj_set_width(ui_Button2, 71);
    lv_obj_set_height(ui_Button2, 40);
    lv_obj_set_x(ui_Button2, -70);
    lv_obj_set_y(ui_Button2, 130);
    lv_obj_set_align(ui_Button2, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Button2, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_Button2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Button2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Button2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button2, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_Button2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_Button2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button2, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui_Button2, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_spread(ui_Button2, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_radius(ui_Button2, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Label1 = lv_label_create(ui_Button2);
    lv_obj_set_width(ui_Label1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Label1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label1, "Back");
    lv_obj_set_style_text_color(ui_Label1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Button14 = lv_btn_create(parent);
    lv_obj_set_width(ui_Button14, 71);
    lv_obj_set_height(ui_Button14, 40);
    lv_obj_set_x(ui_Button14, 70);
    lv_obj_set_y(ui_Button14, 130);
    lv_obj_set_align(ui_Button14, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Button14, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_Button14, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Button14, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Button14, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button14, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_Button14, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_Button14, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button14, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui_Button14, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_spread(ui_Button14, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_radius(ui_Button14, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Label15 = lv_label_create(ui_Button14);
    lv_obj_set_width(ui_Label15, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label15, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Label15, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label15, "Next");
    lv_obj_set_style_text_color(ui_Label15, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label15, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_Button2, setting_page_switch_cb, LV_EVENT_CLICKED, (void*)'n');
    lv_obj_add_event_cb(ui_Button14, setting_page_switch_cb, LV_EVENT_CLICKED, (void*)'p');

    setting_page = lv_label_create(parent);
    lv_obj_set_width(setting_page, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(setting_page, LV_SIZE_CONTENT);    /// 1
    lv_obj_align(setting_page, LV_ALIGN_BOTTOM_MID, 0, -23);
    lv_label_set_text_fmt(setting_page, "%d / %d", setting_curr_page, setting_page_num);
    lv_obj_set_style_text_color(setting_page, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(setting_page, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    scr_back_btn_create(parent, ("Setting"), scr2_btn_event_cb);
}
static void entry2(void) {
    ui_disp_full_refr();
}
static void exit2(void) {
    ui_disp_full_refr();
}
static void destroy2(void) { }

static scr_lifecycle_t screen2 = {
    .create = create2,
    .entry = entry2,
    .exit  = exit2,
    .destroy = destroy2,
};
#endif
//************************************[ screen 3 ]****************************************** GPS
#if 1
#define line_max 23
static lv_obj_t *scr3_cont;
static lv_obj_t *scr3_cnt_lab;
static lv_timer_t *GPS_loop_timer = NULL;

static void gps_set_line(lv_obj_t *label, const char *str1, const char *str2)
{
    int w2 = strlen(str2);
    int w1 = line_max - w2;
    lv_label_set_text_fmt(label, "%-*s%-*s", w1, str1, w2, str2);
}

static lv_obj_t * scr3_create_label(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_width(label, lv_pct(90));
    lv_obj_set_style_text_font(label, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);   
    lv_obj_set_style_border_width(label, 1, LV_PART_MAIN);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_border_side(label, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    return label;
}

static void scr3_GPS_updata(void)
{
    double lat      = 0; // Latitude
    double lon      = 0; // Longitude
    double speed    = 0; // Speed over ground
    float alt      = 0; // Altitude
    float accuracy = 0; // Accuracy
    uint32_t   vsat     = 0; // Visible Satellites
    int   usat     = 0; // Used Satellites
    uint16_t   year     = 0; // 
    uint8_t   month    = 0; // 
    uint8_t   day      = 0; // 
    uint8_t   hour     = 0; // 
    uint8_t   min      = 0; // 
    uint8_t   sec      = 0; // 

    static int cnt = 0;

    lv_label_set_text_fmt(scr3_cnt_lab, " %05d ", ++cnt);

    ui_gps_get_coord(&lat, &lon);
    ui_gps_get_data(&year, &month, &day);
    ui_gps_get_time(&hour, &min, &sec);
    ui_gps_get_satellites(&vsat);
    ui_gps_get_speed(&speed);

    char buf[32];

    lv_snprintf(buf, 16, "%0.1f", lat);
    gps_set_line(label_list[0], "Latitude:", buf);

    lv_snprintf(buf, 16, "%0.1f", lon);
    gps_set_line(label_list[1], "Longitude:", buf);

    lv_snprintf(buf, 16, "%0.3fkmph", speed);
    gps_set_line(label_list[2], "Speed:", buf);

    lv_snprintf(buf, 16, "%d", vsat);
    gps_set_line(label_list[3], "vsat:", buf);
    
    lv_snprintf(buf, 16, "%d", year);
    gps_set_line(label_list[4], "year:", buf);

    lv_snprintf(buf, 16, "%d", month);
    gps_set_line(label_list[5], "month:", buf);

    lv_snprintf(buf, 16, "%d", day);
    gps_set_line(label_list[6], "day:", buf);

    lv_snprintf(buf, 16, "%02d:%02d:%02d", hour, min, sec);
    gps_set_line(label_list[7], "time:", buf);

    // lv_snprintf(buf, 16, "%0.1f", alt);
    // gps_set_line(label_list[3], "alt:", buf);

    // lv_snprintf(buf, 16, "%d", usat);
    // gps_set_line(label_list[5], "usat:", buf);

}

static void GPS_loop_timer_event(lv_timer_t * t)
{
    scr3_GPS_updata();
}

static void scr3_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void create3(lv_obj_t *parent) 
{   
    scr3_cont = lv_obj_create(parent);
    lv_obj_set_size(scr3_cont, lv_pct(100), lv_pct(88));
    lv_obj_set_style_bg_color(scr3_cont, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(scr3_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr3_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(scr3_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr3_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(scr3_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(scr3_cont, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_column(scr3_cont, 0, LV_PART_MAIN);
    lv_obj_set_align(scr3_cont, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_flex_flow(scr3_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr3_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    for(int i = 0; i < sizeof(label_list) / sizeof(label_list[0]); i++) {
        label_list[i] = scr3_create_label(scr3_cont);
        lv_label_set_text(label_list[i], " ");
    }

    scr3_cnt_lab = lv_label_create(parent);
    lv_obj_set_style_text_font(scr3_cnt_lab, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);
    lv_obj_set_style_radius(scr3_cnt_lab, 5, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr3_cnt_lab, 2, LV_PART_MAIN);
    lv_obj_set_style_text_align(scr3_cnt_lab, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(scr3_cnt_lab, " %05d ", 0);
    lv_obj_center(scr3_cnt_lab);
    lv_obj_align(scr3_cnt_lab, LV_ALIGN_TOP_RIGHT, -10, 10);

    lv_obj_t *back3_label = scr_back_btn_create(parent, ("GPS"), scr3_btn_event_cb);
}
static void entry3(void) 
{
    scr3_GPS_updata();

    ui_gps_task_resume();

    GPS_loop_timer = lv_timer_create(GPS_loop_timer_event, 3000, NULL);
    ui_disp_full_refr();
}
static void exit3(void) {
    ui_gps_task_suspend();
    if(GPS_loop_timer) {
        lv_timer_del(GPS_loop_timer);
        GPS_loop_timer = NULL;
    }
    ui_disp_full_refr();
}
static void destroy3(void) { }

static scr_lifecycle_t screen3 = {
    .create = create3,
    .entry = entry3,
    .exit  = exit3,
    .destroy = destroy3,
};

#undef line_max

#endif
//************************************[ screen 4 ]****************************************** Wifi Scan
// --------------------- screen 4 --------------------- WIFI
#if 1
lv_obj_t * scr4_list;
static lv_obj_t *scr4_lab_buf[20];

static void scr4_list_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    for(int i = 0; i < lv_obj_get_child_cnt(obj); i++) 
    {
        lv_obj_t * child = lv_obj_get_child(obj, i);
        if(lv_obj_check_type(child, &lv_label_class)) {
            char *str = lv_label_get_text(child);

            if(strcmp("- WIFI Config", str) == 0)
            {
                scr_mgr_push(SCREEN4_1_ID, false);
            }
            if(strcmp("- WIFI Scan", str) == 0)
            {
                scr_mgr_push(SCREEN4_2_ID, false);
            }
            printf("%s\n", str);
        }
    }
}

static void scr4_item_create(const char *name, lv_event_cb_t cb)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&lv_list_btn_class, scr4_list);
    lv_obj_class_init_obj(obj);
    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, name);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_set_height(obj, LV_VER_RES / 6);
    lv_obj_set_style_text_font(obj, FONT_BOLD_SIZE_15, LV_PART_MAIN);
    // lv_obj_set_style_bg_color(obj, lv_color_hex(EPD_COLOR_BG), LV_PART_MAIN);
    // lv_obj_set_style_text_color(obj, lv_color_hex(EPD_COLOR_FG), LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_outline_width(obj, 1, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(obj, cb, LV_EVENT_CLICKED, NULL); 
}

static void scr4_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // ui_full_refresh();
        scr_mgr_pop(false);
    }
}

static void create4(lv_obj_t *parent) 
{
    scr4_list = lv_list_create(parent);
    lv_obj_set_size(scr4_list, lv_pct(93), lv_pct(91));
    lv_obj_align(scr4_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    // lv_obj_set_style_bg_color(scr4_list, lv_color_hex(EPD_COLOR_BG), LV_PART_MAIN);
    lv_obj_set_style_pad_top(scr4_list, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_row(scr4_list, 15, LV_PART_MAIN);
    lv_obj_set_style_radius(scr4_list, 0, LV_PART_MAIN);
    // lv_obj_set_style_outline_pad(scr4_list, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr4_list, 0, LV_PART_MAIN);
    // lv_obj_set_style_border_color(scr4_list, lv_color_hex(EPD_COLOR_FG), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(scr4_list, 0, LV_PART_MAIN);

    scr4_item_create("- WIFI Config", scr4_list_event);
    scr4_item_create("- WIFI Scan", scr4_list_event);

    // back
    scr_back_btn_create(parent, "WIFI", scr4_btn_event_cb);
}

static void entry4(void) 
{
    ui_disp_full_refr();
}
static void exit4(void) {
    ui_disp_full_refr();
}
static void destroy4(void) { }

static scr_lifecycle_t screen4 = {
    .create = create4,
    .entry = entry4,
    .exit  = exit4,
    .destroy = destroy4,
};
#endif
// --------------------- screen 4.2 --------------------- Wifi Config
#if 1
static void scr4_1_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void create4_1(lv_obj_t *parent) 
{


    // back
    scr_back_btn_create(parent, "Wifi Config", scr4_1_btn_event_cb);
}

static void entry4_1(void) 
{
    ui_disp_full_refr();
}
static void exit4_1(void) {
    ui_disp_full_refr();
}
static void destroy4_1(void) { }

static scr_lifecycle_t screen4_1 = {
    .create = create4_1,
    .entry = entry4_1,
    .exit  = exit4_1,
    .destroy = destroy4_1,
};
#endif
// --------------------- screen 4.2 --------------------- Wifi Scan
#if 1
static lv_obj_t *scr4_2_cont;
static lv_obj_t *wifi_scan_lab;
static lv_timer_t *wifi_scan_timer = NULL;

static ui_wifi_scan_info_t wifi_info_list[UI_WIFI_SCAN_ITEM_MAX];

static void scr4_2_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void show_wifi_scan(void)
{
#define BUFF_LEN 400
    char buf[BUFF_LEN];
    int ret = 0, offs = 0;

    ret = lv_snprintf(buf + offs, BUFF_LEN, "       NAME      | RSSI\n");
    offs = offs + ret;
    ret = lv_snprintf(buf + offs, BUFF_LEN, "-----------------------\n");
    offs = offs + ret;

    for(int i = 0; i < UI_WIFI_SCAN_ITEM_MAX; i++) {
        if(strcmp(wifi_info_list[i].name, "") == 0 && wifi_info_list[i].rssi == 0)
        {
            break;
        }
        if(i == UI_WIFI_SCAN_ITEM_MAX - 1) {
            ret = lv_snprintf(buf + offs, BUFF_LEN, "%-16.16s | %4d", wifi_info_list[i].name, wifi_info_list[i].rssi);
            break;
        }

        ret = lv_snprintf(buf + offs, BUFF_LEN, "%-16.16s | %4d\n", wifi_info_list[i].name, wifi_info_list[i].rssi);
        offs = offs + ret;
    }
    lv_label_set_text(wifi_scan_lab, buf);
#undef BUFF_LEN
}

static void wifi_scan_timer_event(lv_timer_t *t)
{
    ui_wifi_get_scan_info(wifi_info_list, UI_WIFI_SCAN_ITEM_MAX);
    show_wifi_scan();
}

static void create4_2(lv_obj_t *parent) 
{
    scr4_2_cont = lv_obj_create(parent);
    lv_obj_set_size(scr4_2_cont, lv_pct(100), lv_pct(90));
    lv_obj_set_style_bg_color(scr4_2_cont, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(scr4_2_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr4_2_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(scr4_2_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr4_2_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(scr4_2_cont, 13, LV_PART_MAIN);
    lv_obj_set_flex_flow(scr4_2_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(scr4_2_cont, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_column(scr4_2_cont, 5, LV_PART_MAIN);
    lv_obj_set_align(scr4_2_cont, LV_ALIGN_BOTTOM_MID);

    wifi_scan_lab = lv_label_create(scr4_2_cont);
    lv_obj_set_width(wifi_scan_lab, lv_pct(95));
    lv_obj_set_style_pad_all(wifi_scan_lab, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(wifi_scan_lab, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);   
    lv_obj_set_style_border_width(wifi_scan_lab, 0, LV_PART_MAIN);
    lv_label_set_long_mode(wifi_scan_lab, LV_LABEL_LONG_WRAP);

    lv_obj_t *back4_label = scr_back_btn_create(parent, ("Wifi"), scr4_2_btn_event_cb);
}
static void entry4_2(void) 
{
    ui_disp_full_refr();
    wifi_scan_timer = lv_timer_create(wifi_scan_timer_event, 10000, NULL);
    lv_timer_ready(wifi_scan_timer);
}
static void exit4_2(void) {
    ui_disp_full_refr();
    if(wifi_scan_timer) {
        lv_timer_del(wifi_scan_timer);
        wifi_scan_timer = NULL;
    }
}

static void destroy4_2(void) { }

static scr_lifecycle_t screen4_2 = {
    .create = create4_2,
    .entry = entry4_2,
    .exit  = exit4_2,
    .destroy = destroy4_2,
};
#endif
//************************************[ screen 5 ]****************************************** Test
#if 1
static lv_obj_t *test_list;
static lv_obj_t *test_page;
static int test_num = 0;
static int test_page_num = 0;
static int test_curr_page = 0;

static ui_test_handle test_handle_list[] = {
    { .name="Lora",       .peri_id=E_PERI_LORA       , .cb=ui_test_get },
    { .name="Touch",      .peri_id=E_PERI_TOUCH      , .cb=ui_test_get },
    { .name="BQ25896",    .peri_id=E_PERI_BQ25896    , .cb=ui_test_get },
    { .name="BQ27220",    .peri_id=E_PERI_BQ27220    , .cb=ui_test_get },
    { .name="SD Card",    .peri_id=E_PERI_SD         , .cb=ui_test_get },
    { .name="A7682E",     .peri_id=E_PERI_A7682E     , .cb=ui_test_get },
    { .name="PCM5102A",   .peri_id=E_PERI_PCM5102A   , .cb=ui_test_get },
    { .name="Keypad",     .peri_id=E_PERI_KYEPAD     , .cb=ui_test_get },
    { .name="GPS",        .peri_id=E_PERI_GPS        , .cb=ui_test_get },
    { .name="BHI260AP",   .peri_id=E_PERI_BHI260AP   , .cb=ui_test_get },
    { .name="INK_SCREEN", .peri_id=E_PERI_INK_SCREEN , .cb=ui_test_get },
};

static void test_item_create(int curr_apge);

static void scr5_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void test_page_switch_cb(lv_event_t *e)
{
    char opt = (int)e->user_data;
    
    if(test_num < SETTING_PAGE_MAX_ITEM) return;

    int child_cnt = lv_obj_get_child_cnt(test_list);
    
    for(int i = 0; i < child_cnt; i++)
    {
        lv_obj_t *child = lv_obj_get_child(test_list, 0);
        if(child)
            lv_obj_del(child);
    }

    if(opt == 'p')
    {
        test_curr_page = (test_curr_page < test_page_num) ? test_curr_page + 1 : 0;
    }
    else if(opt == 'n')
    {
        test_curr_page = (test_curr_page > 0) ? test_curr_page - 1 : test_page_num;
    }

    test_item_create(test_curr_page);
    lv_label_set_text_fmt(test_page, "%d / %d", test_curr_page, test_page_num);
}

static void test_item_create(int curr_apge)
{
    printf("test_curr_page = %d\n", test_curr_page);
    int start = (curr_apge * SETTING_PAGE_MAX_ITEM);
    int end = start + SETTING_PAGE_MAX_ITEM;
    if(end > test_num) end = test_num;

    printf("start=%d, end=%d\n", start, end);

    for(int i = start; i < end; i++) {
        ui_test_handle *h = &test_handle_list[i];
        h->obj = lv_list_add_btn(test_list, NULL, h->name);
        h->st = lv_label_create(h->obj);
        lv_obj_set_style_text_font(h->st, FONT_BOLD_SIZE_15, LV_PART_MAIN);
        lv_obj_align(h->st, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_label_set_text_fmt(h->st, "%s", (h->cb(h->peri_id) ? "PASS" : "----"));
        // style
        lv_obj_set_style_text_font(h->obj, FONT_BOLD_SIZE_15, LV_PART_MAIN);
        lv_obj_set_style_bg_color(h->obj, DECKPRO_COLOR_BG, LV_PART_MAIN);
        lv_obj_set_style_text_color(h->obj, DECKPRO_COLOR_FG, LV_PART_MAIN);
        lv_obj_set_style_border_width(h->obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(h->obj, 1, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_outline_width(h->obj, 3, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_radius(h->obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
        // lv_obj_add_event_cb(h->obj, test_scr_event, LV_EVENT_CLICKED, (void *)h);
    }
}

static void create5(lv_obj_t *parent) 
{
    test_list = lv_list_create(parent);
    lv_obj_set_size(test_list, LV_HOR_RES, lv_pct(88));
    lv_obj_align(test_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(test_list, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_top(test_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(test_list, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(test_list, 0, LV_PART_MAIN);
    // lv_obj_set_style_outline_pad(test_list, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(test_list, 0, LV_PART_MAIN);
    lv_obj_set_style_border_color(test_list, DECKPRO_COLOR_FG, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(test_list, 0, LV_PART_MAIN);

    test_num = sizeof(test_handle_list) / sizeof(test_handle_list[0]);
    test_page_num = test_num / SETTING_PAGE_MAX_ITEM;
    test_item_create(test_curr_page);

    lv_obj_t * ui_Button2 = lv_btn_create(parent);
    lv_obj_set_width(ui_Button2, 71);
    lv_obj_set_height(ui_Button2, 40);
    lv_obj_set_x(ui_Button2, -70);
    lv_obj_set_y(ui_Button2, 130);
    lv_obj_set_align(ui_Button2, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Button2, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_Button2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Button2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Button2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button2, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_Button2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_Button2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button2, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui_Button2, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_spread(ui_Button2, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_radius(ui_Button2, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Label1 = lv_label_create(ui_Button2);
    lv_obj_set_width(ui_Label1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Label1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label1, "Back");
    lv_obj_set_style_text_color(ui_Label1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Button14 = lv_btn_create(parent);
    lv_obj_set_width(ui_Button14, 71);
    lv_obj_set_height(ui_Button14, 40);
    lv_obj_set_x(ui_Button14, 70);
    lv_obj_set_y(ui_Button14, 130);
    lv_obj_set_align(ui_Button14, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Button14, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_Button14, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Button14, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Button14, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button14, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_Button14, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_Button14, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button14, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui_Button14, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_spread(ui_Button14, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_radius(ui_Button14, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Label15 = lv_label_create(ui_Button14);
    lv_obj_set_width(ui_Label15, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label15, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Label15, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label15, "Next");
    lv_obj_set_style_text_color(ui_Label15, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label15, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_Button2, test_page_switch_cb, LV_EVENT_CLICKED, (void*)'n');
    lv_obj_add_event_cb(ui_Button14, test_page_switch_cb, LV_EVENT_CLICKED, (void*)'p');

    test_page = lv_label_create(parent);
    lv_obj_set_width(test_page, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(test_page, LV_SIZE_CONTENT);    /// 1
    lv_obj_align(test_page, LV_ALIGN_BOTTOM_MID, 0, -23);
    lv_label_set_text_fmt(test_page, "%d / %d", test_curr_page, test_page_num);
    lv_obj_set_style_text_color(test_page, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(test_page, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back5_label = scr_back_btn_create(parent, ("Test"), scr5_btn_event_cb);
}
static void entry5(void) 
{
    ui_disp_full_refr();
}
static void exit5(void) {
    ui_disp_full_refr();
}
static void destroy5(void) { }

static scr_lifecycle_t screen5 = {
    .create = create5,
    .entry = entry5,
    .exit  = exit5,
    .destroy = destroy5,
};
#endif
//************************************[ screen 6 ]****************************************** Battery
// --------------------- screen 6 --------------------- Battery
#if 1
lv_obj_t * scr6_list;
static lv_obj_t *scr6_lab_buf[20];

static void scr6_list_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    for(int i = 0; i < lv_obj_get_child_cnt(obj); i++) 
    {
        lv_obj_t * child = lv_obj_get_child(obj, i);
        if(lv_obj_check_type(child, &lv_label_class)) {
            char *str = lv_label_get_text(child);

            if(strcmp("- BQ25896", str) == 0)
            {
                scr_mgr_push(SCREEN6_1_ID, false);
            }
            if(strcmp("- BQ27220", str) == 0)
            {
                scr_mgr_push(SCREEN6_2_ID, false);
            }
            printf("%s\n", str);
        }
    }
}

static void scr6_item_create(const char *name, lv_event_cb_t cb)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&lv_list_btn_class, scr6_list);
    lv_obj_class_init_obj(obj);
    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, name);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_set_height(obj, LV_VER_RES / 6);
    lv_obj_set_style_text_font(obj, FONT_BOLD_SIZE_15, LV_PART_MAIN);
    // lv_obj_set_style_bg_color(obj, lv_color_hex(EPD_COLOR_BG), LV_PART_MAIN);
    // lv_obj_set_style_text_color(obj, lv_color_hex(EPD_COLOR_FG), LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_outline_width(obj, 1, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(obj, cb, LV_EVENT_CLICKED, NULL); 
}

static void scr6_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // ui_full_refresh();
        scr_mgr_pop(false);
    }
}

static void create6(lv_obj_t *parent) 
{
    scr6_list = lv_list_create(parent);
    lv_obj_set_size(scr6_list, lv_pct(93), lv_pct(91));
    lv_obj_align(scr6_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    // lv_obj_set_style_bg_color(scr6_list, lv_color_hex(EPD_COLOR_BG), LV_PART_MAIN);
    lv_obj_set_style_pad_top(scr6_list, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_row(scr6_list, 15, LV_PART_MAIN);
    lv_obj_set_style_radius(scr6_list, 0, LV_PART_MAIN);
    // lv_obj_set_style_outline_pad(scr6_list, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr6_list, 0, LV_PART_MAIN);
    // lv_obj_set_style_border_color(scr6_list, lv_color_hex(EPD_COLOR_FG), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(scr6_list, 0, LV_PART_MAIN);

    scr6_item_create("- BQ25896", scr6_list_event);
    scr6_item_create("- BQ27220", scr6_list_event);

    // back
    scr_back_btn_create(parent, "Battery", scr6_btn_event_cb);
}

static void entry6(void) 
{
    ui_disp_full_refr();
}
static void exit6(void) {
    ui_disp_full_refr();
}
static void destroy6(void) { }

static scr_lifecycle_t screen6 = {
    .create = create6,
    .entry = entry6,
    .exit  = exit6,
    .destroy = destroy6,
};
#endif
// --------------------- screen 6.1 --------------------- BQ25896
#if 1
#define line_max 23

static lv_timer_t *batt_6_1_timer = NULL;

static void battery_set_line(lv_obj_t *label, const char *str1, const char *str2)
{
    int w2 = strlen(str2);
    int w1 = line_max - w2;
    lv_label_set_text_fmt(label, "%-*s%-*s", w1, str1, w2, str2);
}

static lv_obj_t * scr6_1_create_label(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_width(label, lv_pct(90));
    lv_obj_set_style_text_font(label, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);   
    lv_obj_set_style_border_width(label, 1, LV_PART_MAIN);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_border_side(label, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    return label;
}

static void scr6_1_battert_updata(void)
{
    char buf[line_max];

    battery_set_line(label_list[0], "Charging:", (ui_batt_25896_is_chg() == true ? "Charging" : "Not charged"));

    lv_snprintf(buf, line_max, "%.2fV", ui_batt_25896_get_vbus());
    battery_set_line(label_list[1], "VBUS:", buf);

    lv_snprintf(buf, line_max, "%.2fV", ui_batt_25896_get_vsys());
    battery_set_line(label_list[2], "VSYS:", buf);

    lv_snprintf(buf, line_max, "%.2fV", ui_batt_25896_get_vbat());
    battery_set_line(label_list[3], "VBAT:", buf);

    lv_snprintf(buf, line_max, "%.2fv", ui_batt_25896_get_volt_targ());
    battery_set_line(label_list[4], "VOLT Target:", buf);

    lv_snprintf(buf, line_max, "%.2fmA", ui_batt_25896_get_chg_curr());
    battery_set_line(label_list[5], "Charge Curr:", buf);

    lv_snprintf(buf, line_max, "%.2fmA", ui_batt_25896_get_pre_curr());
    battery_set_line(label_list[6], "Prechg Curr:", buf);

    lv_snprintf(buf, line_max, "%s", ui_batt_25896_get_chg_st());
    battery_set_line(label_list[7], "CHG ST:", buf);

    lv_snprintf(buf, line_max, "%s", ui_batt_25896_get_vbus_st());
    battery_set_line(label_list[8], "VBUS Status:", buf);

    lv_snprintf(buf, line_max, "%s", ui_batt_25896_get_ntc_st());
    battery_set_line(label_list[9], " ", buf);
}

static void batt_6_1_updata_timer_event(lv_timer_t *t) 
{
    scr6_1_battert_updata();
}

static void scr6_1_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void create6_1(lv_obj_t *parent) 
{
    lv_obj_t *scr6_1_cont = lv_obj_create(parent);
    lv_obj_set_size(scr6_1_cont, lv_pct(100), lv_pct(88));
    lv_obj_set_style_bg_color(scr6_1_cont, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(scr6_1_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr6_1_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(scr6_1_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr6_1_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(scr6_1_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(scr6_1_cont, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_column(scr6_1_cont, 0, LV_PART_MAIN);
    lv_obj_set_align(scr6_1_cont, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_flex_flow(scr6_1_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr6_1_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    for(int i = 0; i < sizeof(label_list) / sizeof(label_list[0]); i++) {
        label_list[i] = scr6_1_create_label(scr6_1_cont);
    }

    scr_back_btn_create(parent, ("BQ25896"), scr6_1_btn_event_cb);
}
static void entry6_1(void) 
{
    scr6_1_battert_updata();
    ui_disp_full_refr();
    batt_6_1_timer = lv_timer_create(batt_6_1_updata_timer_event, 5000, NULL);
}
static void exit6_1(void) {
    if(batt_6_1_timer) {
        lv_timer_del(batt_6_1_timer);
        batt_6_1_timer = NULL;
    }
    ui_disp_full_refr();
}
static void destroy6_1(void) { }

static scr_lifecycle_t screen6_1 = {
    .create = create6_1,
    .entry = entry6_1,
    .exit  = exit6_1,
    .destroy = destroy6_1,
};
#undef line_max

#endif
// --------------------- screen 6.2 --------------------- BQ27220
#if 1

#define line_max 23

static lv_timer_t *batt_6_2_timer = NULL;

static lv_obj_t * scr6_2_create_label(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_width(label, lv_pct(90));
    lv_obj_set_style_text_font(label, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);   
    lv_obj_set_style_border_width(label, 1, LV_PART_MAIN);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_border_side(label, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    return label;
}

static void scr6_2_battert_updata(void)
{
    char buf[line_max];

    battery_set_line(label_list[0],"VBUS ST::", (ui_battery_27220_get_input() == true ? "Connected" : "Disonnected"));

    if(ui_battery_27220_get_input() == true ){
        lv_snprintf(buf, line_max, "%s", (ui_battery_27220_get_charge_finish()? "Finsish":"Charging"));
    } else {
        lv_snprintf(buf, line_max, "%s", "Discharge");
    }
    battery_set_line(label_list[1],"Charing ST:", buf);

    lv_snprintf(buf, line_max, "0x%x", ui_battery_27220_get_status());
    battery_set_line(label_list[2],"Battery ST:", buf);

    lv_snprintf(buf, line_max, "%dmV", ui_battery_27220_get_voltage());
    battery_set_line(label_list[3], "Voltage:", buf);

    lv_snprintf(buf, line_max, "%dmA", ui_battery_27220_get_current());
    battery_set_line(label_list[4], "Current:", buf);

    lv_snprintf(buf, line_max, "%.2fC", (float)(ui_battery_27220_get_temperature() / 10.0 - 273.0));
    battery_set_line(label_list[5], "Temperature:", buf);

    lv_snprintf(buf, line_max, "%dmAh", ui_battery_27220_get_remain_capacity());
    battery_set_line(label_list[6], "Cap Remain:", buf);

    lv_snprintf(buf, line_max, "%dmAh", ui_battery_27220_get_full_capacity());
    battery_set_line(label_list[7], "Cap Full:", buf);

    lv_snprintf(buf, line_max, "%d%%", ui_battery_27220_get_percent());
    battery_set_line(label_list[8], "Cap Percent:", buf);

    lv_snprintf(buf, line_max, "%d%%", ui_battery_27220_get_health());
    battery_set_line(label_list[9], "CapHealth:", buf);
}

static void batt_6_2_updata_timer_event(lv_timer_t *t) 
{
    scr6_2_battert_updata();
}

static void scr6_2_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void create6_2(lv_obj_t *parent) 
{   
    lv_obj_t *scr6_2_cont = lv_obj_create(parent);
    lv_obj_set_size(scr6_2_cont, lv_pct(100), lv_pct(88));
    lv_obj_set_style_bg_color(scr6_2_cont, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(scr6_2_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr6_2_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(scr6_2_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr6_2_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(scr6_2_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(scr6_2_cont, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_column(scr6_2_cont, 0, LV_PART_MAIN);
    lv_obj_set_align(scr6_2_cont, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_flex_flow(scr6_2_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr6_2_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    for(int i = 0; i < sizeof(label_list) / sizeof(label_list[0]); i++) {
        label_list[i] = scr6_2_create_label(scr6_2_cont);
    }
    // back
    scr_back_btn_create(parent, ("BQ27220"), scr6_btn_event_cb);
}

static void entry6_2(void) 
{
    scr6_2_battert_updata();
    ui_disp_full_refr();
    batt_6_2_timer = lv_timer_create(batt_6_2_updata_timer_event, 5000, NULL);
}
static void exit6_2(void) {
    if(batt_6_2_timer) {
        lv_timer_del(batt_6_2_timer);
        batt_6_2_timer = NULL;
    }
    ui_disp_full_refr();
}

static void destroy6_2(void) { }

static scr_lifecycle_t screen6_2 = {
    .create = create6_2,
    .entry = entry6_2,
    .exit  = exit6_2,
    .destroy = destroy6_2,
};
#undef line_max
#endif
//************************************[ screen 7 ]****************************************** Other
#if 1
static lv_obj_t *scr7_cont;
static lv_obj_t *input_touch;
static lv_obj_t *input_keypad;
static lv_obj_t *light_sensor;
static lv_obj_t *gyroscope;
static lv_timer_t *input_timer;

static void scr7_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void input_timer_event(lv_timer_t *t)
{
    int touch_x, touch_y;

    int ret = ui_input_get_touch_coord(&touch_x, &touch_y);

    if(ret > 0)
    {
        lv_label_set_text_fmt(input_touch,  "Touch: x: %03d | y: %03d", touch_x, touch_y);
    }

    char keypay_v;
    ret = ui_input_get_keypay_val(&keypay_v);
    if(ret > 0)
    {
        ui_input_set_keypay_flag();
        lv_label_set_text_fmt(input_keypad, "Keypad: %c", keypay_v);
    }

    static int sec = 0;
    int ch0, ch1 ,ps;
    float gyro_x, gyro_y, gyro_z;

    sec++;
    if(sec > 20) // 2s
    {
        sec = 0;
        lv_label_set_text_fmt(light_sensor, "   c0: %d\n"
                                            "   c1: %d\n"
                                            "   ps: %d", ch0, ch1 ,ps);

        ui_other_get_gyro(&gyro_x, &gyro_y, &gyro_z);
        lv_label_set_text_fmt(gyroscope,    "   gyros_x: %.3f\n"
                                            "   gyros_y: %.3f\n"
                                            "   gyros_z: %.3f", gyro_x, gyro_y, gyro_z);
    }
}

static void create7(lv_obj_t *parent) 
{
    scr7_cont = lv_obj_create(parent);
    lv_obj_set_size(scr7_cont, lv_pct(100), lv_pct(88));
    lv_obj_set_style_bg_color(scr7_cont, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(scr7_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr7_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(scr7_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr7_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(scr7_cont, 13, LV_PART_MAIN);
    lv_obj_set_flex_flow(scr7_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(scr7_cont, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_column(scr7_cont, 5, LV_PART_MAIN);
    lv_obj_set_align(scr7_cont, LV_ALIGN_BOTTOM_MID);

    input_touch = lv_label_create(scr7_cont);
    // lv_obj_set_height(input_touch, 90);
    lv_obj_set_width(input_touch, lv_pct(95));
    lv_obj_set_style_pad_all(input_touch, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(input_touch, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);
    // lv_obj_set_style_border_width(input_touch, 1, LV_PART_MAIN);
    lv_label_set_long_mode(input_touch, LV_LABEL_LONG_WRAP);
    lv_label_set_text(input_touch,  "Touch: x:     | y:    ");

    input_keypad = lv_label_create(scr7_cont);
    // lv_obj_set_height(input_keypad, 100);
    lv_obj_set_width(input_keypad, lv_pct(95));
    lv_obj_set_style_pad_all(input_keypad, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(input_keypad, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);
    // lv_obj_set_style_border_width(input_keypad, 1, LV_PART_MAIN);
    lv_label_set_long_mode(input_keypad, LV_LABEL_LONG_WRAP);
    lv_label_set_text_fmt(input_keypad, "Keypad: ");

    lv_obj_t *lab1 = lv_label_create(scr7_cont);
    lv_obj_set_style_text_font(lab1, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);
    lv_label_set_text(lab1, "light sensor");

    light_sensor = lv_label_create(scr7_cont);
    // lv_obj_set_height(input_keypad, 100);
    lv_obj_set_width(light_sensor, lv_pct(95));
    lv_obj_set_style_pad_all(light_sensor, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(light_sensor, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);
    // lv_obj_set_style_border_width(light_sensor, 1, LV_PART_MAIN);
    lv_label_set_long_mode(light_sensor, LV_LABEL_LONG_WRAP);
    lv_label_set_text_fmt(light_sensor, "   c0: 000\n"
                                        "   c1: 000\n"
                                        "   ps: 000");

    lv_obj_t *lab2 = lv_label_create(scr7_cont);
    lv_obj_set_style_text_font(lab2, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);
    lv_label_set_text(lab2, "gyroscope");

    gyroscope = lv_label_create(scr7_cont);
    // lv_obj_set_height(input_keypad, 100);
    lv_obj_set_width(gyroscope, lv_pct(95));
    lv_obj_set_style_pad_all(gyroscope, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(gyroscope, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);
    // lv_obj_set_style_border_width(gyroscope, 1, LV_PART_MAIN);
    lv_label_set_long_mode(gyroscope, LV_LABEL_LONG_WRAP);
    lv_label_set_text_fmt(gyroscope,    "   gyros_x: 000\n"
                                        "   gyros_y: 000\n"
                                        "   gyros_z: 000");

    lv_obj_t *back7_label = scr_back_btn_create(parent, ("Other"), scr7_btn_event_cb);
}
static void entry7(void) 
{
    ui_disp_full_refr();
    input_timer = lv_timer_create(input_timer_event, 100, NULL);
}
static void exit7(void) {
    if(input_timer)
    {
        lv_timer_del(input_timer);
        input_timer = NULL;
    }
    ui_disp_full_refr();
}
static void destroy7(void) { }

static scr_lifecycle_t screen7 = {
    .create = create7,
    .entry = entry7,
    .exit  = exit7,
    .destroy = destroy7,
};
#endif
//************************************[ screen 8 ]****************************************** A7682E
// --------------------- screen 8 --------------------- A7682E
#if 1
static lv_obj_t *a7682_list;
static lv_obj_t *a7682_page;
static int a7682_num = 0;
static int a7682_page_num = 0;
static int a7682_curr_page = 0;

bool ui_a7682_call_test(const char *param)
{
    scr_mgr_push(SCREEN8_1_ID, false);
    return true;
}

bool ui_a7682_at_test(const char *param)
{
    scr_mgr_push(SCREEN8_2_ID, false);
    return true;
}

static ui_a7682_handle a7682_handle_list[] = 
{
    {"A7682 Audio", NULL, NULL, ui_a7682_at_cb},
    {"Call test", NULL, NULL, ui_a7682_call_test},
    {"AT test", NULL, NULL, ui_a7682_at_test},
};

static void a7682_item_create(int curr_apge);

static void a7682_scr_event(lv_event_t *e)
{
    lv_obj_t *tgt = (lv_obj_t *)e->target;
    ui_a7682_handle *h = (ui_a7682_handle *)e->user_data;

    if(e->code == LV_EVENT_CLICKED) {
        if(h->cb)
            h->cb(h->name);
    }
}

static void a7682_item_create(int curr_apge)
{
    printf("a7682_curr_page = %d\n", a7682_curr_page);
    int start = (curr_apge * SETTING_PAGE_MAX_ITEM);
    int end = start + SETTING_PAGE_MAX_ITEM;
    if(end > a7682_num) end = a7682_num;

    printf("start=%d, end=%d\n", start, end);

    for(int i = start; i < end; i++) {
        ui_a7682_handle *h = &a7682_handle_list[i];
        h->obj = lv_list_add_btn(a7682_list, NULL, h->name);
        lv_obj_set_height(h->obj, 28);
        // h->st = lv_label_create(h->obj);
        // lv_obj_set_style_text_font(h->st, FONT_BOLD_SIZE_15, LV_PART_MAIN);
        // lv_obj_align(h->st, LV_ALIGN_RIGHT_MID, 0, 0);
        // lv_label_set_text_fmt(h->st, "%s", (h->get_cb() ? "ON" : "OFF"));
        // style
        lv_obj_set_style_text_font(h->obj, FONT_BOLD_SIZE_14, LV_PART_MAIN);
        lv_obj_set_style_bg_color(h->obj, DECKPRO_COLOR_BG, LV_PART_MAIN);
        lv_obj_set_style_text_color(h->obj, DECKPRO_COLOR_FG, LV_PART_MAIN);
        lv_obj_set_style_border_width(h->obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(h->obj, 1, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_outline_width(h->obj, 3, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_radius(h->obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(h->obj, a7682_scr_event, LV_EVENT_CLICKED, (void *)h);
    }
}

static void a7682_page_switch_cb(lv_event_t *e)
{
    char opt = (int)e->user_data;
    
    if(a7682_num < SETTING_PAGE_MAX_ITEM) return;

    int child_cnt = lv_obj_get_child_cnt(a7682_list);
    
    for(int i = 0; i < child_cnt; i++)
    {
        lv_obj_t *child = lv_obj_get_child(a7682_list, 0);
        if(child)
            lv_obj_del(child);
    }

    if(opt == 'p')
    {
        a7682_curr_page = (a7682_curr_page < a7682_page_num) ? a7682_curr_page + 1 : 0;
    }
    else if(opt == 'n')
    {
        a7682_curr_page = (a7682_curr_page > 0) ? a7682_curr_page - 1 : a7682_page_num;
    }

    a7682_item_create(a7682_curr_page);
    lv_label_set_text_fmt(a7682_page, "%d / %d", a7682_curr_page, a7682_page_num);
}

static void scr8_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void create8(lv_obj_t *parent) 
{
    a7682_list = lv_list_create(parent);
    lv_obj_set_size(a7682_list, LV_HOR_RES, lv_pct(88));
    lv_obj_align(a7682_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(a7682_list, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_top(a7682_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(a7682_list, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(a7682_list, 0, LV_PART_MAIN);
    // lv_obj_set_style_outline_pad(a7682_list, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(a7682_list, 0, LV_PART_MAIN);
    lv_obj_set_style_border_color(a7682_list, DECKPRO_COLOR_FG, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(a7682_list, 0, LV_PART_MAIN);

    a7682_num = sizeof(a7682_handle_list) / sizeof(a7682_handle_list[0]);
    a7682_page_num = a7682_num / SETTING_PAGE_MAX_ITEM;
    a7682_item_create(a7682_curr_page);

    lv_obj_t * ui_Button2 = lv_btn_create(parent);
    lv_obj_set_width(ui_Button2, 71);
    lv_obj_set_height(ui_Button2, 40);
    lv_obj_set_x(ui_Button2, -70);
    lv_obj_set_y(ui_Button2, 130);
    lv_obj_set_align(ui_Button2, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Button2, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_Button2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Button2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Button2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button2, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_Button2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_Button2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button2, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui_Button2, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_spread(ui_Button2, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_radius(ui_Button2, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Label1 = lv_label_create(ui_Button2);
    lv_obj_set_width(ui_Label1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Label1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label1, "Back");
    lv_obj_set_style_text_color(ui_Label1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Button14 = lv_btn_create(parent);
    lv_obj_set_width(ui_Button14, 71);
    lv_obj_set_height(ui_Button14, 40);
    lv_obj_set_x(ui_Button14, 70);
    lv_obj_set_y(ui_Button14, 130);
    lv_obj_set_align(ui_Button14, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Button14, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_Button14, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Button14, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Button14, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button14, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_Button14, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_Button14, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button14, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui_Button14, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_spread(ui_Button14, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_radius(ui_Button14, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Label15 = lv_label_create(ui_Button14);
    lv_obj_set_width(ui_Label15, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label15, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Label15, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label15, "Next");
    lv_obj_set_style_text_color(ui_Label15, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label15, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_Button2, a7682_page_switch_cb, LV_EVENT_CLICKED, (void*)'n');
    lv_obj_add_event_cb(ui_Button14, a7682_page_switch_cb, LV_EVENT_CLICKED, (void*)'p');

    a7682_page = lv_label_create(parent);
    lv_obj_set_width(a7682_page, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(a7682_page, LV_SIZE_CONTENT);    /// 1
    lv_obj_align(a7682_page, LV_ALIGN_BOTTOM_MID, 0, -23);
    lv_label_set_text_fmt(a7682_page, "%d / %d", a7682_curr_page, a7682_page_num);
    lv_obj_set_style_text_color(a7682_page, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(a7682_page, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back8_label = scr_back_btn_create(parent, ("A7682E"), scr8_btn_event_cb);
}
static void entry8(void) 
{
    ui_disp_full_refr();
}
static void exit8(void) {
    ui_disp_full_refr();
}
static void destroy8(void) { }

static scr_lifecycle_t screen8 = {
    .create = create8,
    .entry = entry8,
    .exit  = exit8,
    .destroy = destroy8,
};
#endif
// --------------------- screen 8.1 --------------------- Call test
#if 1
static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t * ta =  (lv_obj_t *)lv_event_get_user_data(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_btnmatrix_get_selected_btn(obj);
        const char * txt = lv_btnmatrix_get_btn_text(obj, id);
        int len = strlen(txt);
 
        if(!strcmp(txt, LV_SYMBOL_CALL)) {
            ui_a7682_call(lv_textarea_get_text(ta));
        } else if(!strcmp(txt, "Hang up"))
        {
            ui_a7682_hang_up();
        } else if(!strcmp(txt, LV_SYMBOL_BACKSPACE))
        {
            lv_textarea_del_char(ta);
        }else{
            lv_textarea_add_text(ta, txt);
        }
    }
}

static const char * btnm_map[] = {  "1", "2", "3", "\n",
                                    "4", "5", "6", "\n",
                                    "7", "8", "9", "\n",
                                    "*", "0", "#", "\n",
                                    LV_SYMBOL_CALL, "Hang up", LV_SYMBOL_BACKSPACE,""
                                 };


static void scr8_1_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void create8_1(lv_obj_t *parent) 
{
    lv_obj_t * ta = lv_textarea_create(parent);
    lv_textarea_set_one_line(ta, true);
    lv_obj_set_width(ta, lv_pct(98));
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, lv_pct(20));
    lv_obj_set_style_text_font(ta, &Font_Mono_Bold_20, LV_PART_MAIN);
    // lv_obj_add_state(ta, LV_STATE_FOCUSED); /*To be sure the cursor is visible*/
    lv_obj_clear_flag(ta, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_letter_space(ta, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ta, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * btnm1 = lv_btnmatrix_create(parent);
    lv_btnmatrix_set_map(btnm1, btnm_map);
    lv_obj_set_size(btnm1, lv_pct(100)-2, lv_pct(60));
    lv_obj_set_style_border_width(btnm1, 0, 0);
    // lv_btnmatrix_set_btn_width(btnm1, 10, 2);        /*Make "Action1" twice as wide as "Action2"*/
    // lv_btnmatrix_set_btn_ctrl(btnm1, 10, LV_BTNMATRIX_CTRL_CHECKABLE);
    // lv_btnmatrix_set_btn_ctrl(btnm1, 11, LV_BTNMATRIX_CTRL_CHECKED);
    lv_obj_align(btnm1, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(btnm1, event_handler, LV_EVENT_VALUE_CHANGED, ta);
    
    lv_obj_t *back8_1_label = scr_back_btn_create(parent, ("Call"), scr8_1_btn_event_cb);
}
static void entry8_1(void) 
{
    ui_a7682_loop_resume();
    ui_disp_full_refr();
}
static void exit8_1(void) {
    ui_a7682_loop_suspend();
    ui_disp_full_refr();
}
static void destroy8_1(void) { }

static scr_lifecycle_t screen8_1 = {
    .create = create8_1,
    .entry = entry8_1,
    .exit  = exit8_1,
    .destroy = destroy8_1,
};
#endif
// --------------------- screen 8.2 --------------------- AT test
#if 1
static void scr8_2_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void create8_2(lv_obj_t *parent) 
{
    lv_obj_t *lab = lv_label_create(parent);
    lv_obj_set_width(lab, lv_pct(95));
    lv_obj_set_style_text_font(lab, FONT_BOLD_SIZE_17, LV_PART_MAIN);
    lv_label_set_text(lab, "Open the serial port, set the baud rate to 115200, "
                            "and send the AT command of A7682E to test the function.");
    lv_obj_center(lab);
    
    lv_obj_t *back8_2_label = scr_back_btn_create(parent, ("AT test"), scr8_2_btn_event_cb);
}
static void entry8_2(void) 
{
    ui_a7682_loop_resume();
    ui_disp_full_refr();
}
static void exit8_2(void) {
    ui_a7682_loop_suspend();
    ui_disp_full_refr();
}
static void destroy8_2(void) { }

static scr_lifecycle_t screen8_2 = {
    .create = create8_2,
    .entry = entry8_2,
    .exit  = exit8_2,
    .destroy = destroy8_2,
};
#endif
//************************************[ screen 9 ]****************************************** Shutdown
#if 1
static lv_timer_t *shutdown_timer = NULL;

static void scr9_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void shutdown_timer_event(lv_timer_t* t)
{
    ui_shutdown_on();
    lv_timer_del(t);
}

static void create9(lv_obj_t *parent)
{
    if(ui_battery_25896_is_vbus_in()) 
    {
        lv_obj_t * label = lv_label_create(parent);
        lv_obj_set_width(label, lv_pct(95));
        lv_obj_set_style_text_font(label, FONT_BOLD_SIZE_15, LV_PART_MAIN);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_label_set_text(label, "The shutdown function can only be used when the "
                            "battery is connected alone, and cannot be shut down when connected to USB.");
        lv_obj_center(label);

        // back 
        scr_back_btn_create(parent, "Shoutdown", scr8_btn_event_cb);
    } 
    else 
    {
        lv_obj_t * img = lv_img_create(parent);
        lv_img_set_src(img, &img_start);
        lv_obj_center(img);

        lv_timer_create(shutdown_timer_event, 2000, (void *)parent);
    }
}
static void entry9(void) 
{
    ui_disp_full_refr();
}
static void exit9(void) {
    ui_disp_full_refr();
}
static void destroy9(void) { }

static scr_lifecycle_t screen9 = {
    .create = create9,
    .entry = entry9,
    .exit  = exit9,
    .destroy = destroy9,
};
#endif
//************************************[ screen 10 ]***************************************** pcm5102
#if 1
static lv_obj_t *pcm5102_list;
static lv_obj_t *pcm5102_page;
static int pcm5102_num = 0;
static int pcm5102_page_num = 0;
static int pcm5102_curr_page = 0;

static ui_pcm5102_handle pcm5102_handle_list[] = 
{
    {"PCM5102 Audio", NULL, NULL, ui_pcm5102_cb},
};

static void pcm5102_item_create(int curr_apge);

static void pcm5102_scr_event(lv_event_t *e)
{
    lv_obj_t *tgt = (lv_obj_t *)e->target;
    ui_pcm5102_handle *h = (ui_pcm5102_handle *)e->user_data;

    if(e->code == LV_EVENT_CLICKED) {
        if(h->cb)
            h->cb(h->name);
    }
}

static void pcm5102_item_create(int curr_apge)
{
    printf("pcm5102_curr_page = %d\n", pcm5102_curr_page);
    int start = (curr_apge * SETTING_PAGE_MAX_ITEM);
    int end = start + SETTING_PAGE_MAX_ITEM;
    if(end > pcm5102_num) end = pcm5102_num;

    printf("start=%d, end=%d\n", start, end);

    for(int i = start; i < end; i++) {
        ui_pcm5102_handle *h = &pcm5102_handle_list[i];
        h->obj = lv_list_add_btn(pcm5102_list, NULL, h->name);
        lv_obj_set_height(h->obj, 28);
        // h->st = lv_label_create(h->obj);
        // lv_obj_set_style_text_font(h->st, FONT_BOLD_SIZE_15, LV_PART_MAIN);
        // lv_obj_align(h->st, LV_ALIGN_RIGHT_MID, 0, 0);
        // lv_label_set_text_fmt(h->st, "%s", (h->get_cb() ? "ON" : "OFF"));
        // style
        lv_obj_set_style_text_font(h->obj, FONT_BOLD_SIZE_14, LV_PART_MAIN);
        lv_obj_set_style_bg_color(h->obj, DECKPRO_COLOR_BG, LV_PART_MAIN);
        lv_obj_set_style_text_color(h->obj, DECKPRO_COLOR_FG, LV_PART_MAIN);
        lv_obj_set_style_border_width(h->obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(h->obj, 1, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_outline_width(h->obj, 3, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_radius(h->obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(h->obj, pcm5102_scr_event, LV_EVENT_CLICKED, (void *)h);
    }
}

static void pcm5102_page_switch_cb(lv_event_t *e)
{
    char opt = (int)e->user_data;
    
    if(pcm5102_num < SETTING_PAGE_MAX_ITEM) return;

    int child_cnt = lv_obj_get_child_cnt(pcm5102_list);
    
    for(int i = 0; i < child_cnt; i++)
    {
        lv_obj_t *child = lv_obj_get_child(pcm5102_list, 0);
        if(child)
            lv_obj_del(child);
    }

    if(opt == 'p')
    {
        pcm5102_curr_page = (pcm5102_curr_page < pcm5102_page_num) ? pcm5102_curr_page + 1 : 0;
    }
    else if(opt == 'n')
    {
        pcm5102_curr_page = (pcm5102_curr_page > 0) ? pcm5102_curr_page - 1 : pcm5102_page_num;
    }

    pcm5102_item_create(pcm5102_curr_page);
    lv_label_set_text_fmt(pcm5102_page, "%d / %d", pcm5102_curr_page, pcm5102_page_num);
}


static void scr10_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void create10(lv_obj_t *parent) 
{
    pcm5102_list = lv_list_create(parent);
    lv_obj_set_size(pcm5102_list, LV_HOR_RES, lv_pct(88));
    lv_obj_align(pcm5102_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(pcm5102_list, DECKPRO_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_top(pcm5102_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(pcm5102_list, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(pcm5102_list, 0, LV_PART_MAIN);
    // lv_obj_set_style_outline_pad(pcm5102_list, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(pcm5102_list, 0, LV_PART_MAIN);
    lv_obj_set_style_border_color(pcm5102_list, DECKPRO_COLOR_FG, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(pcm5102_list, 0, LV_PART_MAIN);

    pcm5102_num = sizeof(pcm5102_handle_list) / sizeof(pcm5102_handle_list[0]);
    pcm5102_page_num = pcm5102_num / SETTING_PAGE_MAX_ITEM;
    pcm5102_item_create(pcm5102_curr_page);

    lv_obj_t * ui_Button2 = lv_btn_create(parent);
    lv_obj_set_width(ui_Button2, 71);
    lv_obj_set_height(ui_Button2, 40);
    lv_obj_set_x(ui_Button2, -70);
    lv_obj_set_y(ui_Button2, 130);
    lv_obj_set_align(ui_Button2, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Button2, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_Button2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Button2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Button2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button2, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_Button2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_Button2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button2, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui_Button2, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_spread(ui_Button2, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_radius(ui_Button2, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Label1 = lv_label_create(ui_Button2);
    lv_obj_set_width(ui_Label1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Label1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label1, "Back");
    lv_obj_set_style_text_color(ui_Label1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Button14 = lv_btn_create(parent);
    lv_obj_set_width(ui_Button14, 71);
    lv_obj_set_height(ui_Button14, 40);
    lv_obj_set_x(ui_Button14, 70);
    lv_obj_set_y(ui_Button14, 130);
    lv_obj_set_align(ui_Button14, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Button14, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_Button14, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Button14, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Button14, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button14, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_Button14, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_Button14, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Button14, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui_Button14, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_spread(ui_Button14, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_radius(ui_Button14, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Label15 = lv_label_create(ui_Button14);
    lv_obj_set_width(ui_Label15, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label15, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Label15, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label15, "Next");
    lv_obj_set_style_text_color(ui_Label15, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label15, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_Button2, pcm5102_page_switch_cb, LV_EVENT_CLICKED, (void*)'n');
    lv_obj_add_event_cb(ui_Button14, pcm5102_page_switch_cb, LV_EVENT_CLICKED, (void*)'p');

    pcm5102_page = lv_label_create(parent);
    lv_obj_set_width(pcm5102_page, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(pcm5102_page, LV_SIZE_CONTENT);    /// 1
    lv_obj_align(pcm5102_page, LV_ALIGN_BOTTOM_MID, 0, -23);
    lv_label_set_text_fmt(pcm5102_page, "%d / %d", pcm5102_curr_page, pcm5102_page_num);
    lv_obj_set_style_text_color(pcm5102_page, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(pcm5102_page, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back10_label = scr_back_btn_create(parent, ("PCM5102"), scr10_btn_event_cb);
}
static void entry10(void) 
{
    ui_disp_full_refr();
}
static void exit10(void) 
{
    ui_pcm5102_stop();
    ui_disp_full_refr();
}
static void destroy10(void) { }

static scr_lifecycle_t screen10 = {
    .create = create10,
    .entry = entry10,
    .exit  = exit10,
    .destroy = destroy10,
};
#endif
//************************************[ screen 11 ]****************************************** Sleep
#if 1
#include <TouchDrvCSTXXX.hpp>
static void scr11_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_pop(false);
    }
}

static void create11(lv_obj_t *parent)
{
    extern TouchDrvCSTXXX touch;

    touch.sleep();

    lora_sleep();

    SerialGPS.end();
    
    // pinMode(BOARD_GPS_PPS, OUTPUT);
    // pinMode(BOARD_GPS_RXD, OUTPUT);
    // pinMode(BOARD_GPS_TXD, OUTPUT);
    // pinMode(BOARD_LORA_RST, OUTPUT);
    // pinMode(BOARD_TOUCH_RST, OUTPUT);
    // pinMode(BOARD_LORA_BUSY, OUTPUT);

    // digitalWrite(BOARD_GPS_PPS, LOW);
    // digitalWrite(BOARD_GPS_RXD, LOW);
    // digitalWrite(BOARD_GPS_TXD, LOW);
    // digitalWrite(BOARD_LORA_RST, LOW);
    // digitalWrite(BOARD_TOUCH_RST, LOW);
    // digitalWrite(BOARD_LORA_BUSY, LOW);

    gpio_reset_pin((gpio_num_t)BOARD_GPS_PPS);
    gpio_reset_pin((gpio_num_t)BOARD_GPS_RXD);
    gpio_reset_pin((gpio_num_t)BOARD_GPS_TXD);
    gpio_reset_pin((gpio_num_t)BOARD_LORA_RST);
    gpio_reset_pin((gpio_num_t)BOARD_TOUCH_RST);
    gpio_reset_pin((gpio_num_t)BOARD_LORA_BUSY);

    digitalWrite(BOARD_6609_EN, LOW);
    digitalWrite(BOARD_LORA_EN, LOW);
    digitalWrite(BOARD_GPS_EN, LOW);
    
    digitalWrite(BOARD_A7682E_PWRKEY, LOW);

    // gpio_hold_en((gpio_num_t)BOARD_GPS_PPS);
    // gpio_hold_en((gpio_num_t)BOARD_TOUCH_RST);
    // gpio_hold_en((gpio_num_t)BOARD_GPS_RXD);
    // gpio_hold_en((gpio_num_t)BOARD_GPS_TXD);
    // gpio_hold_en((gpio_num_t)BOARD_LORA_RST);
    // gpio_hold_en((gpio_num_t)BOARD_LORA_BUSY);
    gpio_hold_en((gpio_num_t)BOARD_6609_EN);
    gpio_hold_en((gpio_num_t)BOARD_LORA_EN);
    gpio_hold_en((gpio_num_t)BOARD_GPS_EN);
    gpio_hold_en((gpio_num_t)BOARD_A7682E_PWRKEY);
    gpio_deep_sleep_hold_en();

    
    // esp_sleep_enable_ext0_wakeup((gpio_num_t)ENCODER_KEY, 0);                            
    esp_sleep_enable_ext1_wakeup((1UL << BOARD_BOOT_PIN), ESP_EXT1_WAKEUP_ANY_LOW);   // Hibernate using user keys
    esp_deep_sleep_start();

    // back 
    scr_back_btn_create(parent, "Sleep", scr8_btn_event_cb);
}
static void entry11(void) 
{
    ui_disp_full_refr();
}
static void exit11(void) {
    ui_disp_full_refr();
}
static void destroy11(void) { }

static scr_lifecycle_t screen11 = {
    .create = create11,
    .entry = entry11,
    .exit  = exit11,
    .destroy = destroy11,
};
#endif
//************************************[ UI ENTRY ]******************************************
static lv_obj_t *menu_keypad;
static lv_timer_t *menu_timer = NULL;

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

        if(diff_x > UI_SLIDING_DISTANCE) { // right
            if(ui_get_gesture_dir) {
                ui_get_gesture_dir(LV_DIR_LEFT);
            }
            last_point.x = 0;
        } else if(diff_x < -UI_SLIDING_DISTANCE) { // left
            if(ui_get_gesture_dir) {
                ui_get_gesture_dir(LV_DIR_RIGHT);
            }
            last_point.x = 0;
        }
        // Serial.printf("x=%d, y=%d\n", data.point.x, data.point.y);
    }else{
        is_press = false;
        last_point.x = 0;
        last_point.y = 0;
    }
}

static void menu_keypay_get_event(lv_timer_t *t)
{
    static int sec = 0;
    static int press = false;
    char keypay_v;
    int ret = ui_input_get_keypay_val(&keypay_v);

    if(ret > 0)
    {
        sec = 0;
        press = true;
        ui_input_set_keypay_flag();
        lv_label_set_text_fmt(menu_keypad, "%c", keypay_v);
    }

    if(press){
        sec++;
        if(sec > 20) {
            sec = 0;
            press = false;
            lv_label_set_text(menu_keypad, " ");
        }
    }
}

static void menu_taskbar_update_timer_cb(lv_timer_t *t)
{
    static int sec = 0;
    sec++;

    bool charge = 0;
    bool finish = 0;
    bool wifi = 0;
    int percent = 0;

    if(sec % 10 == 0)
    {
        finish = ui_battery_27220_get_charge_finish();
        percent = ui_battery_27220_get_percent();

        if(taskbar_statue[TASKBAR_ID_CHARGE_FINISH] != finish) 
        {
            if(finish){
                lv_label_set_text_fmt(menu_taskbar_charge, "%s", LV_SYMBOL_OK);
            } else {
                lv_label_set_text_fmt(menu_taskbar_charge, "%s", LV_SYMBOL_CHARGE);
            }
            taskbar_statue[TASKBAR_ID_CHARGE_FINISH] = finish;
        }

        if(taskbar_statue[TASKBAR_ID_BATTERY_PERCENT] != percent) 
        {
            lv_label_set_text_fmt(menu_taskbar_battery_percent, "%d", percent);
            lv_label_set_text_fmt(menu_taskbar_battery, "%s", ui_battert_27220_get_percent_level());
            taskbar_statue[TASKBAR_ID_BATTERY_PERCENT] = percent;
        }
    }

    charge = ui_battery_27220_get_input();
    if(taskbar_statue[TASKBAR_ID_CHARGE] != charge) 
    {
        if(charge) {
            lv_obj_clear_flag(menu_taskbar_charge, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(menu_taskbar_charge, LV_OBJ_FLAG_HIDDEN);
        }
        taskbar_statue[TASKBAR_ID_CHARGE] = charge;
    }

}

void ui_deckpro_entry(void)
{
    lv_disp_t *disp = lv_disp_get_default();
    disp->theme = lv_theme_mono_init(disp, false, LV_FONT_DEFAULT);

    touch_chk_timer = lv_timer_create(indev_get_gesture_dir, LV_INDEV_DEF_READ_PERIOD, NULL);
    lv_timer_pause(touch_chk_timer);

    taskbar_update_timer = lv_timer_create(menu_taskbar_update_timer_cb, 1000, NULL);
    lv_timer_pause(taskbar_update_timer);

    scr_mgr_init();

    scr_mgr_register(SCREEN0_ID,    &screen0);      // menu
    scr_mgr_register(SCREEN1_ID,    &screen1);      // Lora
    scr_mgr_register(SCREEN1_1_ID,  &screen1_1);    // - Auto send
    scr_mgr_register(SCREEN1_2_ID,  &screen1_2);    // - Lora Setting
    scr_mgr_register(SCREEN2_ID,    &screen2);      // Setting
    scr_mgr_register(SCREEN2_1_ID,  &screen2_1);    //  - About System
    scr_mgr_register(SCREEN3_ID,    &screen3);      // 
    scr_mgr_register(SCREEN4_ID,    &screen4);      // WIFI
    scr_mgr_register(SCREEN4_1_ID,  &screen4_1);    //  - WIFI Config
    scr_mgr_register(SCREEN4_2_ID,  &screen4_2);    //  - WIFI Scan
    scr_mgr_register(SCREEN5_ID,    &screen5);      // 
    scr_mgr_register(SCREEN6_ID,    &screen6);      // Battery
    scr_mgr_register(SCREEN6_1_ID,  &screen6_1);    //  - BQ25896
    scr_mgr_register(SCREEN6_2_ID,  &screen6_2);    //  - BQ27220
    scr_mgr_register(SCREEN7_ID,    &screen7);      // 
    scr_mgr_register(SCREEN8_ID,    &screen8);      // A7682E
    scr_mgr_register(SCREEN8_1_ID,  &screen8_1);    //  - Call test
    scr_mgr_register(SCREEN8_2_ID,  &screen8_2);    //  - AT test
    scr_mgr_register(SCREEN9_ID,    &screen9);      // Shutdown
    scr_mgr_register(SCREEN10_ID,   &screen10);     // PCM5102
    scr_mgr_register(SCREEN11_ID,   &screen11);
    

    scr_mgr_switch(SCREEN0_ID, false); // set root screen
    scr_mgr_set_anim(LV_SCR_LOAD_ANIM_OVER_LEFT, LV_SCR_LOAD_ANIM_OVER_LEFT, LV_SCR_LOAD_ANIM_OVER_LEFT);

    // menu_keypad = lv_label_create(lv_layer_top());
    // lv_obj_set_style_text_font(menu_keypad, FONT_BOLD_MONO_SIZE_15, LV_PART_MAIN);
    // lv_label_set_text(menu_keypad, " ");
    // lv_obj_align(menu_keypad, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    // menu_timer = lv_timer_create(menu_keypay_get_event, 40, NULL);
}