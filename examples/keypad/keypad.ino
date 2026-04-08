
#include <Adafruit_TCA8418.h>
#define BOARD_I2C_ADDR_KEYBOARD 0x34
#define BOARD_KEYBOARD_SDA 13
#define BOARD_KEYBOARD_SCL 14
#define BOARD_KEYBOARD_INT 15
#define BOARD_KEYBOARD_LED 42

// Connect the IO11 of the chip XL9555
#define BOARD_XL9555_11_KEY_RST     (9)
#define BOARD_KEYBOARD_RST BOARD_XL9555_11_KEY_RST  
//
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 10
#define KEYPAD_KEY_NONE  '\0'
#define KEYPAD_KEY_DEL   '\b'
#define KEYPAD_KEY_SPACE ' '
#define KEYPAD_KEY_ALT   '2'
#define KEYPAD_KEY_ENT   'E'
#define KEYPAD_KEY_UP    'U'
#define KEYPAD_KEY_SYM   'S'

Adafruit_TCA8418 keypad;

const char keymap[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
    {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', KEYPAD_KEY_DEL},
    {KEYPAD_KEY_ALT, 'z', 'x', 'c', 'v', 'b', 'n', 'm', '$', KEYPAD_KEY_ENT},
    {KEYPAD_KEY_NONE, KEYPAD_KEY_NONE, KEYPAD_KEY_NONE, KEYPAD_KEY_NONE, KEYPAD_KEY_NONE, KEYPAD_KEY_UP, '0', KEYPAD_KEY_SPACE, KEYPAD_KEY_SYM, KEYPAD_KEY_UP},
};

const char *key_name(char c)
{
    static char label[2] = {0};
    switch (c) {
        case KEYPAD_KEY_DEL: return "DEL";
        case KEYPAD_KEY_SPACE: return "SPACE";
        case KEYPAD_KEY_ALT: return "ALT";
        case KEYPAD_KEY_ENT: return "ENT";
        case KEYPAD_KEY_UP: return "UP";
        case KEYPAD_KEY_SYM: return "sym";
        case KEYPAD_KEY_NONE: return "NONE";
        default:
            label[0] = c;
            label[1] = '\0';
            return label;
    }
}

void setup(void)
{
    Serial.begin(115200);
    // while (!Serial)
    // {
    //     delay(10);
    // }
    // Backlight test
    for (int i = 0; i < 255; i++)
    {
        analogWrite(BOARD_KEYBOARD_LED, i);
        delay(10);
    }
    for (int i = 255; i > 0; i--)
    {
        analogWrite(BOARD_KEYBOARD_LED, i);
        delay(10);
    }
    analogWrite(BOARD_KEYBOARD_LED, 0);

    Serial.println(__FILE__);

    Wire.begin(BOARD_KEYBOARD_SDA, BOARD_KEYBOARD_SCL);

    if (!keypad.begin(BOARD_I2C_ADDR_KEYBOARD, &Wire))
    {
        Serial.println("keypad not found, check wiring & pullups!");
        while (1)
            ;
    }
    // configure the size of the keypad matrix.
    // all other pins will be inputs
    keypad.matrix(KEYPAD_ROWS, KEYPAD_COLS);

    // flush the internal buffer
    keypad.flush();
}

void loop(void)
{
    if (keypad.available() > 0)
    {
        //  datasheet page 15 - Table 1
        int k = keypad.getEvent();
        if (k & 0x80)
            Serial.print("PRESS\tR: ");
        else
            Serial.print("RELEASE\tR: ");
        k &= 0x7F;
        k--;
        int row = k / KEYPAD_COLS;
        int col = (KEYPAD_COLS - 1) - (k % KEYPAD_COLS);
        char c = keymap[row][col];
        Serial.print(row);
        Serial.print("\tC: ");
        Serial.print(col);
        Serial.print("\tKEY: ");
        Serial.print(key_name(c));
        Serial.println();
    }
}
