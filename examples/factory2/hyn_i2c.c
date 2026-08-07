#include "hyn_core.h"

/* The touch driver is a C module; keep the Arduino Wire bridge C-linkable. */
esp_err_t factory_i2c_begin(u8 pin_sda, u8 pin_scl);
int factory_i2c_write(u8 address, const u8 *buf, u16 len);
int factory_i2c_read(u8 address, u8 *buf, u16 len);
int factory_i2c_write_read(u8 address, const u8 *wbuf, u16 wlen, u8 *rbuf, u16 rlen);

esp_err_t hyn_i2c_init(u8 pin_sda, u8 pin_scl)
{
    return factory_i2c_begin(pin_sda, pin_scl);
}

int hyn_write_data(struct hyn_ts_data *ts_data, u8 *buf, u8 reg_len, u16 len)
{
    (void)reg_len;
    return factory_i2c_write(ts_data->salve_addr, buf, len);
}

int hyn_read_data(struct hyn_ts_data *ts_data, u8 *buf, u16 len)
{
    return factory_i2c_read(ts_data->salve_addr, buf, len);
}

int hyn_wr_reg(struct hyn_ts_data *ts_data, u32 reg_addr, u8 reg_len, u8 *rbuf, u16 rlen)
{
    u8 wbuf[4] = {0};
    reg_len &= 0x0F;
    if (reg_len > sizeof(wbuf)) return -1;

    for (int i = reg_len - 1; i >= 0; --i) {
        wbuf[i] = (u8)reg_addr;
        reg_addr >>= 8;
    }

    if (rlen) {
        return factory_i2c_write_read(ts_data->salve_addr, wbuf, reg_len, rbuf, rlen);
    }
    return factory_i2c_write(ts_data->salve_addr, wbuf, reg_len);
}

void hyn_delay_ms(int cnt)
{
    vTaskDelay(cnt / portTICK_PERIOD_MS);
}

/** gpio control */
int gpio_set_value(uint32_t gpio_id, bool value)
{
    gpio_set_level((gpio_num_t)gpio_id, value);
    return 0;
}

bool gpio_get_value(uint32_t gpio_id)
{
    return gpio_get_level((gpio_num_t)gpio_id);
}
