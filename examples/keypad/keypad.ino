
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

Adafruit_TCA8418 keypad;

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
        Serial.print(k / 10);
        Serial.print("\tC: ");
        Serial.print(k % 10);
        Serial.println();
    }
}
