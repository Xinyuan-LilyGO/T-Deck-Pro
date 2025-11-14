/**
 * @file      utilities.h
 * @author    ShallowGreen123
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2024-05-27
 */
#pragma once

#define UI_T_DECK_PRO_VERSION    "v3.0-250910"  // Software version
#define BOARD_T_DECK_PRO_VERSION "v3.0-250911"  // Hardware version

// Serial
#define SerialMon   Serial      // 
#define SerialAT    Serial1     // 
#define SerialGPS   Serial2     // 

// IIC Addr
#define BOARD_I2C_ADDR_ES8311      0x18
#define BOARD_I2C_ADDR_TOUCH       0x1A
#define BOARD_I2C_ADDR_XL9555      0x20
#define BOARD_I2C_ADDR_GYROSCOPDE  0x28
#define BOARD_I2C_ADDR_KEYBOARD    0x34
#define BOARD_I2C_ADDR_BQ27220     0x55
#define BOARD_I2C_ADDR_MOTOR       0x5A
#define BOARD_I2C_ADDR_BQ25896     0x6B

// IIC
#define BOARD_I2C_SDA  13
#define BOARD_I2C_SCL  14

// XL9555 IO expansion 
#define BOARD_XL9555_INT            (-1)
#define BOARD_XL9555_SDA            BOARD_I2C_SDA
#define BOARD_XL9555_SCL            BOARD_I2C_SCL
#define BOARD_XL9555_00_6609_EN     (0)     // HIGH: Enable the A7682E power supply
#define BOARD_XL9555_01_LORA_EN     (1)     // HIGH: Enable the SX1262 power supply
#define BOARD_XL9555_02_GPS_EN      (2)     // HIGH: Enable the GPS power supply
#define BOARD_XL9555_03_1V8_EN      (3)     // HIGH: Enable the BHI260AP power supply
/* LORA_SEL determines whether to use the internal antenna 
/  or the external antenna; Connected to XL9555 IO04
/   HIGH --- external antenna
/   LOW --- internal antenna  */
#define BOARD_XL9555_04_LORA_SEL    (4)
#define BOARD_XL9555_05_MOTOR_EN    (5)     // HIGH: Enable the DRV2605 power supply
// Connected to XL9555 IO06, enable power amplifier,
// increase the volume of the speaker
#define BOARD_XL9555_06_AMPLIFIER   (6)     // HIGH: Enable power amplifier
#define BOARD_XL9555_07_TOUCH_RST   (7)     // LOW: Reset touch
#define BOARD_XL9555_10_PWEKEY_EN   (8)     // HIGH: Enable A7682E POWERKEY
#define BOARD_XL9555_11_KEY_RST     (9)     // LOW: Reset keyboard
/* Module A7682E and ES8311 share the output for headphones and speakers.
/  Select the audio output through AUDIO_SEL. When AUDIO_SEL is
/  HIGH : the headphones and speakers output the sound from A7682E.
/  LOW :  the headphones and speakers output the sound from ES8311. */
#define BOARD_XL9555_12_AUDIO_SEL   (10)
#define BOARD_XL9555_13             (11)    // Reserve
#define BOARD_XL9555_14             (12)    // Reserve
#define BOARD_XL9555_15             (13)    // Reserve
#define BOARD_XL9555_16             (14)    // Reserve
#define BOARD_XL9555_17             (15)    // Reserve

// Keyboard
#define BOARD_KEYBOARD_SCL BOARD_I2C_SCL
#define BOARD_KEYBOARD_SDA BOARD_I2C_SDA
#define BOARD_KEYBOARD_INT 15
#define BOARD_KEYBOARD_LED 42
#define BOARD_KEYBOARD_RST BOARD_XL9555_11_KEY_RST  // Connect the IO11 of the chip XL9555

// Touch
#define BOARD_TOUCH_SCL BOARD_I2C_SCL
#define BOARD_TOUCH_SDA BOARD_I2C_SDA
#define BOARD_TOUCH_INT 12
#define BOARD_TOUCH_RST BOARD_XL9555_07_TOUCH_RST   // Connect the IO07 of the chip XL9555

// Gyroscope
#define BOARD_GYROSCOPDE_SCL BOARD_I2C_SCL
#define BOARD_GYROSCOPDE_SDA BOARD_I2C_SDA
#define BOARD_GYROSCOPDE_INT 21
#define BOARD_GYROSCOPDE_RST -1

// Motor
#define BOARD_MOTOR_SCL BOARD_I2C_SCL
#define BOARD_MOTOR_SDA BOARD_I2C_SDA
#define BOARD_MOTOR_EN BOARD_XL9555_05_MOTOR_EN     // Connect the IO05 of the chip XL9555

// ES8311
#define BOARD_ES8311_SCL BOARD_I2C_SCL
#define BOARD_ES8311_SDA BOARD_I2C_SDA
#define BOARD_ES8311_MCLK 38
#define BOARD_ES8311_SCLK 39
#define BOARD_ES8311_ASDOUT 40
#define BOARD_ES8311_LRCK 18
#define BOARD_ES8311_DSDIN 17

// SPI
#define BOARD_SPI_SCK  36
#define BOARD_SPI_MOSI 33
#define BOARD_SPI_MISO 47

// Display

#define LCD_HOR_SIZE 240
#define LCD_VER_SIZE 320
#define DISP_BUF_SIZE (LCD_HOR_SIZE * LCD_VER_SIZE)

#define BOARD_EPD_BL   41
#define BOARD_EPD_SCK  BOARD_SPI_SCK
#define BOARD_EPD_MOSI BOARD_SPI_MOSI
#define BOARD_EPD_DC   35
#define BOARD_EPD_CS   34
#define BOARD_EPD_BUSY 37
#define BOARD_EPD_RST  9

// SD card
#define BOARD_SD_CS   48
#define BOARD_SD_SCK  BOARD_SPI_SCK
#define BOARD_SD_MOSI BOARD_SPI_MOSI
#define BOARD_SD_MISO BOARD_SPI_MISO

// Lora
#define BOARD_LORA_SCK  BOARD_SPI_SCK
#define BOARD_LORA_MOSI BOARD_SPI_MOSI
#define BOARD_LORA_MISO BOARD_SPI_MISO
#define BOARD_LORA_CS   3
#define BOARD_LORA_BUSY 6
#define BOARD_LORA_RST  4
#define BOARD_LORA_INT  5

// GPS
#define BOARD_GPS_RXD 2
#define BOARD_GPS_TXD 16
#define BOARD_GPS_PPS 1

// A7682E Modem
#define BOARD_A7682E_RI     7
#define BOARD_A7682E_ITR    8
#define BOARD_A7682E_RXD    10
#define BOARD_A7682E_TXD    11
#define BOARD_A7682E_PWRKEY BOARD_XL9555_10_PWEKEY_EN   // Connect the IO10 of the expansion chip XL9555

// Boot pin
#define BOARD_BOOT_PIN  0
// -------------------------------------------------


