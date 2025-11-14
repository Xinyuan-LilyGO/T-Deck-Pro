#ifndef __TEST_FACTORY_H__
#define __TEST_FACTORY_H__

/*********************************************************************************
 *                                  INCLUDES
 * *******************************************************************************/
#include "peripheral.h"
#define XPOWERS_CHIP_BQ25896
#include <XPowersLib.h>
#include "bq27220.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "FS.h"
#include "SPIFFS.h"
#include "ExtensionIOXL9555.hpp"
#include "Adafruit_DRV2605.h"
#include "esp_codec.h"

/*********************************************************************************
 *                                   DEFINES
 * *******************************************************************************/

#define DISP_REFR_MODE_FULL 0
#define DISP_REFR_MODE_PART 1

#define TINY_GSM_MODEM_SIM7672
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define MODEM_GPS_ENABLE_GPIO               (-1)

#include <TinyGsmClient.h>

extern TinyGsm modem;
extern TaskHandle_t a7682_handle;
/*********************************************************************************
 *                                   MACROS
 * *******************************************************************************/
extern bool peri_init_st[E_PERI_NUM_MAX];
extern XPowersPPM PPM;
extern BQ27220 bq27220;
extern ExtensionIOXL9555 xl9555_io;
extern Adafruit_DRV2605 motor_drv;
extern EspCodec codec;
/*********************************************************************************
 *                                  TYPEDEFS
 * *******************************************************************************/
union flush_buf_pixel
{
    struct _byte {
        uint8_t b1 : 1;
        uint8_t b2 : 1;
        uint8_t b3 : 1;
        uint8_t b4 : 1;
        uint8_t b5 : 1;
        uint8_t b6 : 1;
        uint8_t b7 : 1;
        uint8_t b8 : 1;
    }bit;
    uint8_t full;
};

/*********************************************************************************
 *                              GLOBAL PROTOTYPES
 * *******************************************************************************/
void disp_full_refr(void); // Next global refresh

#endif