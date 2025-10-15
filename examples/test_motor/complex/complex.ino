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

  Serial.println("DRV test");
  drv.begin();
    
  // I2C trigger by sending 'go' command 
  drv.setMode(DRV2605_MODE_INTTRIG); // default, internal trigger when sending GO command

  drv.selectLibrary(1);
  drv.setWaveform(0, 84);  // ramp up medium 1, see datasheet part 11.2
  drv.setWaveform(1, 1);  // strong click 100%, see datasheet part 11.2
  drv.setWaveform(2, 0);  // end of waveforms
}

void loop() {
    drv.go();
    delay(1000);
}
