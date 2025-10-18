
#include <Adafruit_TCA8418.h>
#include "utilities.h"
#include "peripheral.h"

#define KEYPAD_ROWS 4
#define KEYPAD_COLS 10
#define KEYPAD_PRESS_VAL_MIN   129
#define KEYPAD_PRESS_VAL_MAX   163
#define KEYPAD_RELEASE_VAL_MIN 1
#define KEYPAD_RELEASE_VAL_MAX 35

const char keymap[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
    {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '0'},
    {'2', 'z', 'x', 'c', 'v', 'b', 'n', 'm', '$', 'E'},
    {' ', ' ', ' ', ' ', ' ', '-', '*', 'S', '0', 'U'},
};

Adafruit_TCA8418 keypad; 
keypad_cb keypad_listener = NULL;
char keypad_curr_val = ' ';
int keypad_state = KEYPAD_RELEASE;
bool keypad_update = false;

// TEST STRING
char input_textbox[INPUT_TEXTBOX_LEN] = "";
static int input_textbox_index = 0;

bool keypad_init(int address)
{
    if(!i2cIsInit(0)){
        Wire.begin(BOARD_KEYBOARD_SDA, BOARD_KEYBOARD_SCL);
        Wire.beginTransmission(address);
        Wire.endTransmission(true);
    }

    if (!keypad.begin(address, &Wire)) {
        // Serial.println("keypad not found, check wiring & pullups!");
        log_e("keypad not found, check wiring & pullups!");
        return false;
    }

    // configure the size of the keypad matrix.
    // all other pins will be inputs
    keypad.matrix(KEYPAD_ROWS, KEYPAD_COLS);

    // flush the internal buffer
    keypad.flush();

    return true;
}

int keypad_get_val(char *c)
{
    if(c){
        *c = keypad_curr_val;
    }
    return keypad_update;
} 

int keypad_get_state()
{
    return keypad_state;
}

void keypad_set_flag(void)
{
    keypad_update = false;
}

void keypad_loop(void)
{
    char c = -1;
    int state = -1;
    int row, col;
    int k = keypad.getEvent();
    int v = keypad.available();

    if(k >=KEYPAD_RELEASE_VAL_MIN && k <= KEYPAD_RELEASE_VAL_MAX){ // release event
        k = k - KEYPAD_RELEASE_VAL_MIN;
        state = KEYPAD_RELEASE;
    }   

    if(k >=KEYPAD_PRESS_VAL_MIN && k <= KEYPAD_PRESS_VAL_MAX){ // press event
        k = k - KEYPAD_PRESS_VAL_MIN;
        state = KEYPAD_PRESS;
    }

    if(state != -1){
        row = k / KEYPAD_COLS;
        col = (KEYPAD_COLS-1) - k % KEYPAD_COLS;
        c = keymap[row][col];
        Serial.printf("k=%d, v=%d, press:%d, %d, %c, %d\n",
                      k, v, row, col, c, state);
        // if(keypad_listener)
        //     keypad_listener(state, c);
        
        keypad_curr_val = c;
        keypad_state = state;
        keypad_update = true;

        // TEST STRING
        if (state == KEYPAD_PRESS) {
            if (input_textbox_index + 2 >= INPUT_TEXTBOX_LEN) {
                input_textbox_index = 0;
            }
            if (c == '0') {
                --input_textbox_index;
            } else if (c == 'S') {
                input_textbox[input_textbox_index++] = ' ';
            } else if (c == 'E') {
                input_textbox[input_textbox_index++] = '\n';
            } else {
                input_textbox[input_textbox_index++] = c;
            }
            input_textbox[input_textbox_index] = '|';
            input_textbox[input_textbox_index + 1] = '\0';
        }
    }
}

void keypad_regetser_cb(keypad_cb cb)
{
    keypad_listener = cb;
}
