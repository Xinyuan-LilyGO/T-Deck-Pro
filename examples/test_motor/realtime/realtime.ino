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
    
  // Set Real-Time Playback mode
  drv.setMode(DRV2605_MODE_REALTIME);
}

uint8_t rtp_index = 0;
uint8_t rtp[] = {
  0x30, 100, 0x32, 100, 
  0x34, 100, 0x36, 100, 
  0x38, 100, 0x3A, 100,
  0x00, 100,
  0x40, 200, 0x00, 100, 
  0x40, 200, 0x00, 100, 
  0x40, 200, 0x00, 100
};

void loop() {

  if (rtp_index < sizeof(rtp)/sizeof(rtp[0])) {
    drv.setRealtimeValue(rtp[rtp_index]);
    rtp_index++;
    delay(rtp[rtp_index]);
    rtp_index++;
  } else {
    drv.setRealtimeValue(0x00);
    delay(1000);
    rtp_index = 0;
  }
  
}
