#include <Wire.h>
#include "Adafruit_DRV2605.h"

// IIC
#define BOARD_I2C_SDA  13
#define BOARD_I2C_SCL  14

// Motor pin
#define BOARD_MOTOR_PIN 2

Adafruit_DRV2605 drv;

void setup() {
  Serial.begin(115200);

  pinMode(BOARD_MOTOR_PIN, OUTPUT); 
  digitalWrite(BOARD_MOTOR_PIN, HIGH);

  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
  
  Serial.println("DRV2605 Audio responsive test");
  drv.begin();


  drv.setMode(DRV2605_MODE_AUDIOVIBE);

  // ac coupled input, puts in 0.9V bias
  drv.writeRegister8(DRV2605_REG_CONTROL1, 0x20);  
 
  // analog input
  drv.writeRegister8(DRV2605_REG_CONTROL3, 0xA3);  
}


void loop() {
}
