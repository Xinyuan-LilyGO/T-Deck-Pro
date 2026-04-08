#include "eng_test_app.h"

static char g_prev_key_event = 0;
static uint32_t g_last_key_ms = 0;

enum ButtonAction : uintptr_t {
    BTN_NONE = 0,
    BTN_SELECT_ALL = 1,
    BTN_START_FROM_SELECT,
    BTN_TOGGLE_FOCUSED,
    BTN_START_QUICK,
    BTN_BACK_TO_SELECT,
    BTN_RESTART_QUICK,
    BTN_OPEN_DEEP,
    BTN_SHOW_SUMMARY,
    BTN_EXECUTE_SHUTDOWN,
    BTN_FOCUS_UP,
    BTN_FOCUS_DOWN,
    BTN_OPEN_FOCUSED,
    BTN_PASS,
    BTN_FAIL,
    BTN_SKIP,
    BTN_RETRY,
    BTN_NEXT_OR_BACK,
};

static void handle_button_action(uintptr_t action_id);

static void handle_button_action_async(void *user_data)
{
    handle_button_action((uintptr_t)user_data);
}

static void mark_quick_disabled(int id)
{
    g_results[id].enabled = false;
    g_results[id].status = TEST_STATUS_SKIP;
    strncpy(g_results[id].note, "not in quick test", sizeof(g_results[id].note) - 1);
    g_results[id].note[sizeof(g_results[id].note) - 1] = '\0';
}

static void select_all_quick_tests()
{
    for (int i = 0; i < TEST_COUNT; ++i) {
        if (!is_quick_test_case(i)) {
            mark_quick_disabled(i);
            continue;
        }
        g_results[i].enabled = true;
        g_results[i].status = TEST_STATUS_NOT_RUN;
        g_results[i].note[0] = '\0';
    }
    update_selected_count();
}

static void reset_selected_quick_tests()
{
    for (int i = 0; i < TEST_COUNT; ++i) {
        if (!is_quick_test_case(i)) {
            mark_quick_disabled(i);
            continue;
        }
        if (g_results[i].enabled) {
            g_results[i].status = TEST_STATUS_NOT_RUN;
            g_results[i].note[0] = '\0';
        } else {
            g_results[i].status = TEST_STATUS_SKIP;
        }
    }
    update_selected_count();
}

static void open_deep_item_async(void *user_data)
{
    int map_id = (int)(uintptr_t)user_data;
    if (map_id == DEEP_ITEM_SHUTDOWN) {
        build_shutdown_page();
        return;
    }
    g_current_test = map_id;
    g_current_mode = TEST_MODE_DEEP;
    build_test_page();
}

static void set_focus_widget(lv_obj_t *obj, bool focused)
{
    if (!obj) {
        return;
    }
    lv_obj_set_style_outline_width(obj, focused ? 2 : 0, LV_PART_MAIN);
    lv_obj_set_style_outline_pad(obj, 1, LV_PART_MAIN);
}

static void focus_reset()
{
    for (int i = 0; i < g_focus_count; ++i) {
        set_focus_widget(g_focus_widgets[i], false);
        g_focus_widgets[i] = nullptr;
    }
    g_focus_count = 0;
    g_focus_index = 0;
}

static void focus_add(lv_obj_t *obj)
{
    if (g_focus_count >= FOCUS_MAX) {
        return;
    }
    g_focus_widgets[g_focus_count++] = obj;
}

static void focus_apply()
{
    for (int i = 0; i < g_focus_count; ++i) {
        set_focus_widget(g_focus_widgets[i], i == g_focus_index);
    }
}

static void focus_move(int delta)
{
    if (!g_focus_count) {
        return;
    }
    g_focus_index += delta;
    if (g_focus_index < 0) {
        g_focus_index = g_focus_count - 1;
    }
    if (g_focus_index >= g_focus_count) {
        g_focus_index = 0;
    }
    focus_apply();
}

static lv_obj_t *create_action_button(lv_obj_t *parent, int x, const char *text, uintptr_t action)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, BUTTON_WIDTH, BUTTON_HEIGHT);
    lv_obj_set_pos(btn, x, 288);
    if (action != BTN_NONE) {
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            uintptr_t action_id = (uintptr_t)lv_event_get_user_data(e);
            lv_async_call(handle_button_action_async, (void *)action_id);
        }, LV_EVENT_CLICKED, (void *)action);
    }
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return btn;
}

static void handle_button_action(uintptr_t action_id)
{
    switch (action_id) {
        case BTN_SELECT_ALL:
            select_all_quick_tests();
            build_module_select_page();
            break;
        case BTN_START_FROM_SELECT:
            if (selected_test_count()) {
                build_precheck_page();
            }
            break;
        case BTN_TOGGLE_FOCUSED:
            if (g_current_page == PAGE_MODULE_SELECT && g_focus_count && g_focus_index < g_focus_count) {
                lv_event_send(g_focus_widgets[g_focus_index], LV_EVENT_CLICKED, nullptr);
            }
            break;
        case BTN_START_QUICK:
            show_next_quick_test(false);
            break;
        case BTN_BACK_TO_SELECT:
            build_module_select_page();
            break;
        case BTN_RESTART_QUICK:
            reset_selected_quick_tests();
            g_current_test = -1;
            show_next_quick_test(false);
            break;
        case BTN_OPEN_DEEP:
            build_deep_menu_page();
            break;
        case BTN_SHOW_SUMMARY:
            build_summary_page();
            break;
        case BTN_EXECUTE_SHUTDOWN:
            if (g_shutdown_info) {
                lv_label_set_text(g_shutdown_info, "Preparing BQ25896 shutdown...");
            }
            delay(200);
            PPM.shutdown();
            break;
        case BTN_FOCUS_UP:
            focus_move(-1);
            break;
        case BTN_FOCUS_DOWN:
            focus_move(1);
            break;
        case BTN_OPEN_FOCUSED:
            if (g_focus_count && g_focus_index < g_focus_count) {
                lv_event_send(g_focus_widgets[g_focus_index], LV_EVENT_CLICKED, nullptr);
            }
            break;
        case BTN_PASS:
            set_test_status((TestCaseId)g_current_test, TEST_STATUS_PASS, "manual pass");
            if (g_current_mode == TEST_MODE_QUICK) quick_auto_advance();
            break;
        case BTN_FAIL:
            set_test_status((TestCaseId)g_current_test, TEST_STATUS_FAIL, "manual fail");
            if (g_current_mode == TEST_MODE_QUICK) quick_auto_advance();
            break;
        case BTN_SKIP:
            set_test_status((TestCaseId)g_current_test, TEST_STATUS_SKIP, "manual skip");
            if (g_current_mode == TEST_MODE_QUICK) quick_auto_advance();
            break;
        case BTN_RETRY:
            teardown_current_test();
            build_test_page();
            break;
        case BTN_NEXT_OR_BACK:
            if (g_current_mode == TEST_MODE_DEEP) {
                teardown_current_test();
                build_deep_menu_page();
            } else {
                show_next_quick_test(false);
            }
            break;
        default:
            break;
    }
}

static lv_obj_t *create_header(const char *title, const char *subtitle)
{
    lv_obj_t *scr = lv_scr_act();
    focus_reset();
    lv_obj_clean(scr);
    request_full_refresh();

    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_color(scr, lv_color_black(), LV_PART_MAIN);

    lv_obj_t *title_label = lv_label_create(scr);
    lv_label_set_text(title_label, title);
    lv_obj_set_pos(title_label, 6, 4);

    g_page_subtitle_label = lv_label_create(scr);
    lv_label_set_text(g_page_subtitle_label, subtitle ? subtitle : "");
    lv_obj_set_pos(g_page_subtitle_label, 6, 24);

    lv_obj_t *body = lv_obj_create(scr);
    lv_obj_set_pos(body, 2, 44);
    lv_obj_set_size(body, 236, 238);
    lv_obj_set_style_radius(body, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(body, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(body, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(body, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    return body;
}

static String summary_line(TestCaseId id)
{
    String line = status_badge(g_results[id].status);
    line += " ";
    line += g_tests[id].name_cn;
    if (g_results[id].note[0]) {
        line += " - ";
        line += g_results[id].note;
    }
    return line;
}

static void refresh_module_button(TestCaseId id)
{
    if (!g_module_buttons[id]) {
        return;
    }
    String text = g_results[id].enabled ? "[x] " : "[ ] ";
    text += g_tests[id].name_cn;
    text += " / ";
    text += g_tests[id].name_en;
    lv_obj_t *label = lv_obj_get_child(g_module_buttons[id], 0);
    lv_label_set_text(label, text.c_str());
}

static void handle_module_toggle(TestCaseId id)
{
    if (!is_quick_test_case(id)) {
        mark_quick_disabled(id);
        return;
    }
    g_results[id].enabled = !g_results[id].enabled;
    if (!g_results[id].enabled) {
        g_results[id].status = TEST_STATUS_SKIP;
        strncpy(g_results[id].note, "disabled before run", sizeof(g_results[id].note) - 1);
        g_results[id].note[sizeof(g_results[id].note) - 1] = '\0';
    } else {
        g_results[id].status = TEST_STATUS_NOT_RUN;
        g_results[id].note[0] = '\0';
    }
    refresh_module_button(id);
    update_selected_count();
    if (g_page_subtitle_label) {
        String sub = "Selected modules: ";
        sub += selected_test_count();
        lv_label_set_text(g_page_subtitle_label, sub.c_str());
    }
}

static void on_module_button(lv_event_t *e)
{
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    g_focus_index = id;
    focus_apply();
    handle_module_toggle((TestCaseId)id);
}

static void on_deep_item(lv_event_t *e)
{
    int map_id = (int)(uintptr_t)lv_event_get_user_data(e);
    lv_async_call(open_deep_item_async, (void *)(uintptr_t)map_id);
}

static void start_current_test()
{
    g_auto_advance_at = 0;
    g_results[g_current_test].status = TEST_STATUS_RUNNING;
    g_results[g_current_test].note[0] = '\0';
    Serial.printf("[ENG_TEST] START %-8s %s\n",
                  g_tests[g_current_test].name_en,
                  g_current_mode == TEST_MODE_QUICK ? "quick" : "deep");
    bool ok = g_tests[g_current_test].setup(g_current_mode);
    if (!ok) {
        set_test_status((TestCaseId)g_current_test, TEST_STATUS_FAIL, "setup failed");
    }
}

static void build_test_action_bar(bool deep_mode)
{
    lv_obj_t *scr = lv_scr_act();
    create_action_button(scr, 4, "Pass", BTN_PASS);
    create_action_button(scr, 50, "Fail", BTN_FAIL);
    create_action_button(scr, 96, "Skip", BTN_SKIP);
    create_action_button(scr, 142, "Retry", BTN_RETRY);
    create_action_button(scr, 188, deep_mode ? "Back" : "Next", BTN_NEXT_OR_BACK);
}

static void build_test_body_for_epd(lv_obj_t *body)
{
    g_test_info_label = lv_label_create(body);
    lv_obj_set_width(g_test_info_label, 220);
    lv_label_set_text(g_test_info_label, "Watch the black/white block and counter change.");

    g_display_ctx.box = lv_obj_create(body);
    lv_obj_set_pos(g_display_ctx.box, 20, 48);
    lv_obj_set_size(g_display_ctx.box, 180, 96);
    lv_obj_set_style_bg_color(g_display_ctx.box, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_display_ctx.box, 1, LV_PART_MAIN);

    g_test_hint_label = lv_label_create(body);
    lv_obj_set_pos(g_test_hint_label, 0, 160);
    lv_obj_set_width(g_test_hint_label, 220);
    lv_label_set_text(g_test_hint_label, "Partial refresh count: 0");
}

static void build_test_body_for_touch(lv_obj_t *body)
{
    memset(&g_touch_ctx, 0, sizeof(g_touch_ctx));
    g_test_info_label = lv_label_create(body);
    lv_obj_set_width(g_test_info_label, 220);
    lv_label_set_long_mode(g_test_info_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(g_test_info_label, "Touch the screen and press/release the 3 touch keys.");

    g_test_status_label = lv_label_create(body);
    lv_obj_set_pos(g_test_status_label, 0, 48);
    lv_obj_set_width(g_test_status_label, 220);
    lv_label_set_long_mode(g_test_status_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(g_test_status_label, "Touch: waiting for coordinates");

    for (int i = 0; i < 3; ++i) {
        lv_obj_t *block = lv_obj_create(body);
        g_touch_ctx.key_blocks[i] = block;
        lv_obj_set_size(block, 64, 44);
        lv_obj_set_pos(block, i * 76, 116);
        lv_obj_set_style_radius(block, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(block, 1, LV_PART_MAIN);
        lv_obj_set_style_bg_color(block, lv_color_white(), LV_PART_MAIN);
        lv_obj_clear_flag(block, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *label = lv_label_create(block);
        g_touch_ctx.key_labels[i] = label;
        lv_label_set_text_fmt(label, "KEY%d", i + 1);
        lv_obj_center(label);
    }

    g_test_hint_label = lv_label_create(body);
    lv_obj_set_pos(g_test_hint_label, 0, 176);
    lv_obj_set_width(g_test_hint_label, 220);
    lv_label_set_long_mode(g_test_hint_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(g_test_hint_label, "Seen: screen --, keys 0/3");
}

static void build_test_body_for_keypad(lv_obj_t *body)
{
    g_test_info_label = lv_label_create(body);
    lv_obj_set_width(g_test_info_label, 220);
    lv_label_set_long_mode(g_test_info_label, LV_LABEL_LONG_WRAP);
    if (g_current_mode == TEST_MODE_QUICK) {
        lv_label_set_text(g_test_info_label, "Quick keys: Q  A  ALT  P  ENT  UP");
    } else {
        lv_label_set_text(g_test_info_label, "Deep mode: press every key. Passes when all are counted.");
    }

    g_test_status_label = lv_label_create(body);
    lv_obj_set_pos(g_test_status_label, 0, 60);
    lv_obj_set_width(g_test_status_label, 220);
    lv_label_set_long_mode(g_test_status_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(g_test_status_label, "");

    g_test_hint_label = lv_label_create(body);
    lv_obj_set_pos(g_test_hint_label, 0, 170);
    lv_obj_set_width(g_test_hint_label, 220);
    lv_label_set_text(g_test_hint_label, "This page counts keypad input for the test and ignores shortcuts.");
}

static void build_generic_info_body(lv_obj_t *body)
{
    g_test_info_label = lv_label_create(body);
    lv_obj_set_width(g_test_info_label, 220);
    lv_label_set_long_mode(g_test_info_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(g_test_info_label, g_tests[g_current_test].summary_cn);

    g_test_status_label = lv_label_create(body);
    lv_obj_set_pos(g_test_status_label, 0, 46);
    lv_obj_set_width(g_test_status_label, 220);
    lv_label_set_long_mode(g_test_status_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(g_test_status_label, "");

    g_test_hint_label = lv_label_create(body);
    lv_obj_set_pos(g_test_hint_label, 0, 182);
    lv_obj_set_width(g_test_hint_label, 220);
    lv_label_set_long_mode(g_test_hint_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(g_test_hint_label, "Shortcuts: E=Pass  S=Fail  -=Skip  *=Retry  U=Next");
}

void build_module_select_page()
{
    g_current_page = PAGE_MODULE_SELECT;
    lv_obj_t *body = create_header("ENG Test / Modules", "Selected modules: 0");
    update_selected_count();
    String sub = "Selected modules: ";
    sub += selected_test_count();
    lv_label_set_text(g_page_subtitle_label, sub.c_str());

    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(body, 4, LV_PART_MAIN);
    lv_obj_set_scroll_dir(body, LV_DIR_VER);

    for (int i = 0; i < TEST_COUNT; ++i) {
        g_module_buttons[i] = nullptr;
        if (!is_quick_test_case(i)) {
            mark_quick_disabled(i);
            continue;
        }
        lv_obj_t *btn = lv_btn_create(body);
        g_module_buttons[i] = btn;
        lv_obj_set_width(btn, 220);
        lv_obj_set_height(btn, 30);
        lv_obj_add_event_cb(btn, on_module_button, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_t *label = lv_label_create(btn);
        lv_obj_center(label);
        refresh_module_button((TestCaseId)i);
        focus_add(btn);
    }
    focus_apply();

    create_action_button(lv_scr_act(), 4, "All", BTN_SELECT_ALL);
    create_action_button(lv_scr_act(), 50, "Sel", BTN_TOGGLE_FOCUSED);
    create_action_button(lv_scr_act(), 96, "Up", BTN_FOCUS_UP);
    create_action_button(lv_scr_act(), 142, "Down", BTN_FOCUS_DOWN);
    create_action_button(lv_scr_act(), 188, "Start", BTN_START_FROM_SELECT);
}

void build_precheck_page()
{
    g_current_page = PAGE_PRECHECK;
    lv_obj_t *body = create_header("ENG Test / Precheck", "Ready for quick test");

    String info;
    info += "Board: ";
    info += BOARD_NAME;
    info += "\nSoftware: ";
    info += UI_T_DECK_PRO_VERSION;
    info += "\nHardware: ";
    info += BOARD_T_DECK_PRO_VERSION;
    info += "\nSelected modules: ";
    info += selected_test_count();
    info += "/";
    info += quick_test_count();
    info += "\n\nNotes:\n1. 4G quick test normally needs battery power.\n2. GPS/LoRa/BLE deep tests depend on external conditions.\n3. Sleep is not part of quick test.\n4. Missing modules will be marked SKIP.";

    lv_obj_t *label = lv_label_create(body);
    lv_obj_set_width(label, 220);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, info.c_str());

    create_action_button(lv_scr_act(), 4, "Back", BTN_BACK_TO_SELECT);
    create_action_button(lv_scr_act(), 50, "All", BTN_SELECT_ALL);
    create_action_button(lv_scr_act(), 96, " ", 0);
    create_action_button(lv_scr_act(), 142, " ", 0);
    create_action_button(lv_scr_act(), 188, "Start", BTN_START_QUICK);
}

void build_test_page()
{
    g_current_page = PAGE_TEST;
    String subtitle;
    if (g_current_mode == TEST_MODE_QUICK) {
        subtitle = "Quick test ";
        subtitle += selected_progress_of(g_current_test);
        subtitle += "/";
        subtitle += selected_test_count();
    } else {
        subtitle = "Deep diagnostics / ";
        subtitle += g_tests[g_current_test].name_en;
    }

    lv_obj_t *body = create_header(g_tests[g_current_test].name_cn, subtitle.c_str());
    g_test_info_label = nullptr;
    g_test_status_label = nullptr;
    g_test_hint_label = nullptr;
    g_display_ctx.box = nullptr;

    switch (g_current_test) {
        case TEST_EPD: build_test_body_for_epd(body); break;
        case TEST_TOUCH: build_test_body_for_touch(body); break;
        case TEST_KEYPAD: build_test_body_for_keypad(body); break;
        default: build_generic_info_body(body); break;
    }

    build_test_action_bar(g_current_mode == TEST_MODE_DEEP);
    start_current_test();
    refresh_test_page_text();
}

void build_summary_page()
{
    g_current_page = PAGE_SUMMARY;
    teardown_current_test();
    g_current_test = -1;
    g_current_mode = TEST_MODE_QUICK;
    g_auto_advance_at = 0;

    int pass_cnt = 0;
    int fail_cnt = 0;
    int skip_cnt = 0;
    for (int i = 0; i < TEST_COUNT; ++i) {
        if (!is_quick_test_case(i) && !g_results[i].enabled) {
            continue;
        }
        if (g_results[i].status == TEST_STATUS_PASS) ++pass_cnt;
        if (g_results[i].status == TEST_STATUS_FAIL) ++fail_cnt;
        if (g_results[i].status == TEST_STATUS_SKIP) ++skip_cnt;
    }

    String subtitle = "PASS ";
    subtitle += pass_cnt;
    subtitle += "  FAIL ";
    subtitle += fail_cnt;
    subtitle += "  SKIP ";
    subtitle += skip_cnt;

    lv_obj_t *body = create_header("ENG Test / Summary", subtitle.c_str());
    lv_obj_set_scroll_dir(body, LV_DIR_VER);

    String info;
    for (int i = 0; i < TEST_COUNT; ++i) {
        if (!is_quick_test_case(i) && !g_results[i].enabled) {
            continue;
        }
        info += summary_line((TestCaseId)i);
        info += "\n";
    }

    lv_obj_t *label = lv_label_create(body);
    lv_obj_set_width(label, 220);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, info.c_str());

    create_action_button(lv_scr_act(), 4, "Cfg", BTN_BACK_TO_SELECT);
    create_action_button(lv_scr_act(), 50, "Retry", BTN_RESTART_QUICK);
    create_action_button(lv_scr_act(), 96, " ", 0);
    create_action_button(lv_scr_act(), 142, "Deep", BTN_OPEN_DEEP);
    create_action_button(lv_scr_act(), 188, " ", 0);

    Serial.println("[ENG_TEST] -------- SUMMARY --------");
    for (int i = 0; i < TEST_COUNT; ++i) {
        if (!is_quick_test_case(i) && !g_results[i].enabled) {
            continue;
        }
        Serial.printf("[ENG_TEST] %-8s %-10s %s\n",
                      g_tests[i].name_en,
                      status_text(g_results[i].status),
                      g_results[i].note);
    }
}

void build_resume_page()
{
    g_current_page = PAGE_RESUME;
    lv_obj_t *body = create_header("ENG Test / Sleep Resume", "Woke from deep sleep");
    lv_obj_t *label = lv_label_create(body);
    lv_obj_set_width(label, 220);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, "Last session ran the sleep test.\nSleep was marked PASS automatically.\nTap Summary to view all results.");

    create_action_button(lv_scr_act(), 4, "Cfg", BTN_BACK_TO_SELECT);
    create_action_button(lv_scr_act(), 50, "Sum", BTN_SHOW_SUMMARY);
    create_action_button(lv_scr_act(), 96, " ", 0);
    create_action_button(lv_scr_act(), 142, "Deep", BTN_OPEN_DEEP);
    create_action_button(lv_scr_act(), 188, " ", 0);
}

void build_deep_menu_page()
{
    g_current_page = PAGE_DEEP_MENU;
    teardown_current_test();
    g_current_test = -1;
    g_current_mode = TEST_MODE_DEEP;

    lv_obj_t *body = create_header("ENG Test / Deep", "w/s move  U open");
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(body, 4, LV_PART_MAIN);
    lv_obj_set_scroll_dir(body, LV_DIR_VER);

    focus_reset();
    for (size_t i = 0; i < sizeof(kDeepMenuMap) / sizeof(kDeepMenuMap[0]); ++i) {
        int map_id = kDeepMenuMap[i];
        lv_obj_t *btn = lv_btn_create(body);
        lv_obj_set_width(btn, 220);
        lv_obj_set_height(btn, 30);
        lv_obj_add_event_cb(btn, on_deep_item, LV_EVENT_CLICKED, (void *)(uintptr_t)map_id);

        String text;
        if (map_id == DEEP_ITEM_SHUTDOWN) {
            text = "Shutdown Test / Shutdown";
        } else {
            text = g_tests[map_id].name_cn;
            text += " / ";
            text += g_tests[map_id].name_en;
        }
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, text.c_str());
        lv_obj_center(label);
        focus_add(btn);
    }
    focus_apply();

    create_action_button(lv_scr_act(), 4, "Back", BTN_SHOW_SUMMARY);
    create_action_button(lv_scr_act(), 50, "Up", BTN_FOCUS_UP);
    create_action_button(lv_scr_act(), 96, "Down", BTN_FOCUS_DOWN);
    create_action_button(lv_scr_act(), 142, "Open", BTN_OPEN_FOCUSED);
    create_action_button(lv_scr_act(), 188, "Sum", BTN_SHOW_SUMMARY);
}

void build_shutdown_page()
{
    g_current_page = PAGE_SHUTDOWN;
    lv_obj_t *body = create_header("ENG Test / Shutdown", "This will cut system power immediately");
    g_shutdown_info = lv_label_create(body);
    lv_obj_set_width(g_shutdown_info, 220);
    lv_label_set_long_mode(g_shutdown_info, LV_LABEL_LONG_WRAP);
    lv_label_set_text(g_shutdown_info, "Run only when powered by battery alone.\nConfirm to call BQ25896 shutdown. Not included in default quick test.");

    create_action_button(lv_scr_act(), 4, "Back", BTN_OPEN_DEEP);
    create_action_button(lv_scr_act(), 50, " ", 0);
    create_action_button(lv_scr_act(), 96, " ", 0);
    create_action_button(lv_scr_act(), 142, "OK", BTN_EXECUTE_SHUTDOWN);
    create_action_button(lv_scr_act(), 188, " ", 0);
}

void show_next_quick_test(bool keep_current_status)
{
    if (g_current_mode != TEST_MODE_QUICK) {
        return;
    }
    if (g_current_test >= 0 && g_current_test < TEST_COUNT) {
        if (!keep_current_status && g_results[g_current_test].status == TEST_STATUS_RUNNING) {
            set_test_status((TestCaseId)g_current_test, TEST_STATUS_SKIP, "next without verdict");
        }
        teardown_current_test();
    }

    int next = g_current_test;
    while (++next < TEST_COUNT) {
        if (!is_quick_test_case(next)) {
            mark_quick_disabled(next);
            continue;
        }
        if (!g_results[next].enabled) {
            g_results[next].status = TEST_STATUS_SKIP;
            continue;
        }
        if (g_results[next].status == TEST_STATUS_PASS ||
            g_results[next].status == TEST_STATUS_FAIL ||
            g_results[next].status == TEST_STATUS_SKIP) {
            continue;
        }
        g_current_test = next;
        g_current_mode = TEST_MODE_QUICK;
        build_test_page();
        return;
    }
    build_summary_page();
}

void handle_key_event(char key)
{
    if (!key) {
        return;
    }
    if (handle_keypad_test_key(key)) {
        return;
    }

    switch (g_current_page) {
        case PAGE_MODULE_SELECT:
            if (key == 'w') focus_move(-1);
            else if (key == 's') focus_move(1);
            else if ((key == 'e' || key == 'E') && g_focus_count && g_focus_index < g_focus_count) {
                lv_event_send(g_focus_widgets[g_focus_index], LV_EVENT_CLICKED, nullptr);
            }
            else if (key == 'U') build_precheck_page();
            else if (key == '*') {
                select_all_quick_tests();
                build_module_select_page();
            }
            break;
        case PAGE_PRECHECK:
            if (key == 'S') build_module_select_page();
            else if (key == 'U' || key == 'E') show_next_quick_test(false);
            break;
        case PAGE_TEST:
            if (key == 'E') {
                set_test_status((TestCaseId)g_current_test, TEST_STATUS_PASS, "manual pass");
                if (g_current_mode == TEST_MODE_QUICK) quick_auto_advance();
            } else if (key == 'S') {
                set_test_status((TestCaseId)g_current_test, TEST_STATUS_FAIL, "manual fail");
                if (g_current_mode == TEST_MODE_QUICK) quick_auto_advance();
            } else if (key == '-') {
                set_test_status((TestCaseId)g_current_test, TEST_STATUS_SKIP, "manual skip");
                if (g_current_mode == TEST_MODE_QUICK) quick_auto_advance();
            } else if (key == '*') {
                teardown_current_test();
                build_test_page();
            } else if (key == 'U') {
                if (g_current_mode == TEST_MODE_DEEP) {
                    teardown_current_test();
                    build_deep_menu_page();
                } else {
                    show_next_quick_test(false);
                }
            }
            break;
        case PAGE_SUMMARY:
            if (key == 'S') build_module_select_page();
            else if (key == '*') {
                reset_selected_quick_tests();
                g_current_test = -1;
                show_next_quick_test(false);
            } else if (key == 'U' || key == 'E') {
                build_deep_menu_page();
            }
            break;
        case PAGE_DEEP_MENU:
            if (key == 'w') focus_move(-1);
            else if (key == 's') focus_move(1);
            else if (key == 'S') build_summary_page();
            else if ((key == 'U' || key == 'E') && g_focus_count && g_focus_index < g_focus_count) {
                lv_event_send(g_focus_widgets[g_focus_index], LV_EVENT_CLICKED, nullptr);
            }
            break;
        case PAGE_RESUME:
            if (key == 'U' || key == 'E') build_summary_page();
            else if (key == 'S') build_module_select_page();
            break;
        case PAGE_SHUTDOWN:
            if (key == 'S') build_deep_menu_page();
            else if (key == 'U' || key == 'E') PPM.shutdown();
            break;
        default:
            break;
    }
}

void poll_keypad_shortcuts()
{
    keypad_loop();
    char value = 0;
    if (!keypad_get_val(&value)) {
        return;
    }
    keypad_set_flag();

    if ((value == g_prev_key_event) && (millis() - g_last_key_ms < 120)) {
        return;
    }
    g_prev_key_event = value;
    g_last_key_ms = millis();
    handle_key_event(value);
}

