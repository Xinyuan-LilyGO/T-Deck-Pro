
/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "Arduino.h"

#include "bq27220.h"
#include "Wire.h"

BQ27220 bq;
BQ27220BatteryStatus batt;

#define BOARD_I2C_SDA  13
#define BOARD_I2C_SCL  14


void setup(void)
{
    Serial.begin(115200);
    while (!Serial)
        ;

    byte error, address;
    int nDevices = 0;
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Serial.println("Scanning for I2C devices ...");
    for (address = 0x01; address < 0x7f; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0)
        {
            Serial.printf("I2C device found at address 0x%02X\n", address);
            nDevices++;
        }
        else if (error != 2)
        {
            Serial.printf("Error %d at address 0x%02X\n", error, address);
        }
    }
    if (nDevices == 0)
    {
        Serial.println("No I2C devices found");
    }

    Serial.println("Initialization success!");

    // 
    Serial.printf("full=%d, design=%d\n", bq.getFullChargeCapacity(), bq.getDesignCapacity());

    bq.setDefaultCapacity(3000);
    bq.init();

    Serial.printf("full=%d, design=%d\n", bq.getFullChargeCapacity(), bq.getDesignCapacity());

    uint16_t data = bq.getDeviceNumber();
    Serial.printf("(%d) Device Number 0x%04x\n", __LINE__, data);
    
}

void loop()
{
    Serial.printf("-------------------------------------------\n");
    bq.getBatteryStatus(&batt);
    Serial.printf("Status = %x\n", batt.full);
    Serial.printf("Status = %s\n", bq.getIsCharging() ? "Charging" : "Discharging");
    Serial.printf("Volt = %d mV\n", bq.getVoltage());
    Serial.printf("Curr = %d mA\n", bq.getCurrent());
    Serial.printf("Temp = %.2f K\n", (float)(bq.getTemperature() / 10.0));
    Serial.printf("full cap= %d mAh\n", bq.getFullChargeCapacity());
    Serial.printf("remain cap = %d mAh\n", bq.getRemainingCapacity());
    Serial.printf("state of charge = %d\n", bq.getStateOfCharge());
    Serial.printf("state of health = %d\n", bq.getStateOfHealth());
    delay(3000);
}
