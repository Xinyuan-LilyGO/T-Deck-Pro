/** Touch test
 * 
 * After downloading the program, open the serial port, touch the 
 * screen with your finger, and the serial port will print the touch coordinates.
 *
 * Note: Touch the RST pin of the chip GT911, which is connected to the IO expansion
 *  chip. When resetting the touch, use the IO expansion chip to drive the reset.
 */

#include <Arduino.h>
#include "utilities.h"

#include <TouchDrvCSTXXX.hpp>
TouchDrvCSTXXX touch;

void setup()
{
    Serial.begin(115200);
    
    touch.setPins(-1, BOARD_TOUCH_INT);
    bool hasTouch = touch.begin(Wire, BOARD_I2C_ADDR_TOUCH, BOARD_TOUCH_SDA, BOARD_TOUCH_SCL);
    if (!hasTouch) {
        Serial.println("Failed to find Capacitive Touch !");
    } else {
        Serial.println("Find Capacitive Touch");
    }

}

void loop()
{
    int16_t read_x, read_y;
    uint8_t touched = touch.getPoint(&read_x, &read_y, 1);
    if(touched) {
        Serial.print("touch_x=");Serial.print(read_x);
        Serial.print(", touch_y=");Serial.println(read_y);
    }
    delay(1);
}

