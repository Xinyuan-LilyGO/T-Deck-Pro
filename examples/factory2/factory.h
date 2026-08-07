#ifndef __TEST_FACTORY_H__
#define __TEST_FACTORY_H__

/*********************************************************************************
 *                                  INCLUDES
 * *******************************************************************************/
#include "peripheral.h"
#include "utilities.h"
#define XPOWERS_CHIP_BQ25896
#include <XPowersLib.h>
#include "bq27220.h"
#include "factory_audio_stub.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "FS.h"
#include "SPIFFS.h"
#include "Adafruit_DRV2605.h"

/*********************************************************************************
 *                                   DEFINES
 * *******************************************************************************/

#define DISP_REFR_MODE_FULL 0
#define DISP_REFR_MODE_PART 1
#define EPD_BITMAP_STRIDE(width) (((width) + 7) / 8)
#define EPD_BITMAP_BUF_SIZE (EPD_BITMAP_STRIDE(LCD_HOR_SIZE) * LCD_VER_SIZE)

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
extern FactoryAudio audio;
extern Adafruit_DRV2605 drv;
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
void ink_screen_prepare_shutdown(void);
void shared_spi_bus_init(void);
void shared_spi_lock(void);
void shared_spi_unlock(void);
void shared_spi_prepare_device(int cs_pin);

int hyn_touch_init(void);
uint8_t hyn_touch_get_point(int16_t *x_array, int16_t *y_array, uint8_t get_point);


#endif
