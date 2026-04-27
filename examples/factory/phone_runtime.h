#ifndef __PHONE_RUNTIME_H__
#define __PHONE_RUNTIME_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UI_PHONE_NUMBER_LEN 32
#define UI_PHONE_STATUS_LEN 16
#define UI_PHONE_HISTORY_MAX 3

typedef enum {
    UI_PHONE_STATE_UNAVAILABLE = 0,
    UI_PHONE_STATE_IDLE,
    UI_PHONE_STATE_OUTGOING,
    UI_PHONE_STATE_INCOMING,
    UI_PHONE_STATE_ACTIVE,
} ui_phone_state_t;

typedef struct {
    char number[UI_PHONE_NUMBER_LEN];
    char direction[UI_PHONE_STATUS_LEN];
    char result[UI_PHONE_STATUS_LEN];
    uint32_t sequence;
} ui_phone_history_item_t;

typedef struct {
    ui_phone_state_t state;
    char current_number[UI_PHONE_NUMBER_LEN];
    char status[UI_PHONE_STATUS_LEN];
    uint32_t call_duration_sec;
    ui_phone_history_item_t recent_calls[UI_PHONE_HISTORY_MAX];
    bool debug_passthrough;
} ui_phone_snapshot_t;

bool phone_runtime_dial(const char *number);
bool phone_runtime_answer(void);
bool phone_runtime_hang_up(void);
bool phone_runtime_play_test_digits(void);
void phone_runtime_set_debug_passthrough(bool enabled);
bool phone_runtime_get_snapshot(ui_phone_snapshot_t *snapshot);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __PHONE_RUNTIME_H__ */
