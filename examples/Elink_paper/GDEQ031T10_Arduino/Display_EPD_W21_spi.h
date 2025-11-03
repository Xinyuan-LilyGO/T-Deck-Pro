#ifndef _DISPLAY_EPD_W21_SPI_
#define _DISPLAY_EPD_W21_SPI_
#include "Arduino.h"

#define BOARD_EPD_DC   35
#define BOARD_EPD_CS   34
#define BOARD_EPD_BUSY 37
#define BOARD_EPD_RST  9


#define  Arduino_UNO
//#define  ESP8266

#ifdef ESP8266
//IO settings
//HSCLK---D5
//HMOSI--D7
#define isEPD_W21_BUSY digitalRead(D0) 
#define EPD_W21_RST_0 digitalWrite(D1,LOW)
#define EPD_W21_RST_1 digitalWrite(D1,HIGH)
#define EPD_W21_DC_0  digitalWrite(D2,LOW)
#define EPD_W21_DC_1  digitalWrite(D2,HIGH)
#define EPD_W21_CS_0 digitalWrite(D4,LOW)
#define EPD_W21_CS_1 digitalWrite(D4,HIGH)

void Sys_run(void);
void LED_run(void);    
#endif 

#ifdef Arduino_UNO
//IO settings
//HSCLK---13
//HMOSI--11
#define isEPD_W21_BUSY digitalRead(BOARD_EPD_BUSY) 
#define EPD_W21_RST_0 digitalWrite(BOARD_EPD_RST,LOW)
#define EPD_W21_RST_1 digitalWrite(BOARD_EPD_RST,HIGH)
#define EPD_W21_DC_0  digitalWrite(BOARD_EPD_DC,LOW)
#define EPD_W21_DC_1  digitalWrite(BOARD_EPD_DC,HIGH)
#define EPD_W21_CS_0 digitalWrite(BOARD_EPD_CS,LOW)
#define EPD_W21_CS_1 digitalWrite(BOARD_EPD_CS,HIGH)
#endif 

void SPI_Write(unsigned char value);
void EPD_W21_WriteDATA(unsigned char datas);
void EPD_W21_WriteCMD(unsigned char command);

#endif 
