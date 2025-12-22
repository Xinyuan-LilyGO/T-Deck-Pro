#include "hyn_core.h"



#define DATA_LENGTH 100

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          200000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

esp_err_t hyn_i2c_init(u8 pin_sda ,u8 pin_scl)
{
   int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = pin_sda,
        .scl_io_num = pin_scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

int hyn_write_data(struct hyn_ts_data *ts_data, u8 *buf, u8 reg_len, u16 len)
{
    int ret = 0;
    // ret = i2c_master_send(ts_data->salve_addr, buf, len);
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, ts_data->salve_addr, buf, len, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
    return ret < 0 ? -1:0;
}

int hyn_read_data(struct hyn_ts_data *ts_data,u8 *buf, u16 len)
{
    int ret = 0;
    //ret = i2c_master_recv(ts_data->salve_addr, buf, len);
    ret = i2c_master_read_from_device(I2C_MASTER_NUM, ts_data->salve_addr, buf, len, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
    return ret < 0 ? -1:0;
}

int hyn_wr_reg(struct hyn_ts_data *ts_data, u32 reg_addr, u8 reg_len, u8 *rbuf, u16 rlen)
{
    int ret = 0,i=0;
    u8 wbuf[4];
    reg_len = reg_len&0x0F;
    memset(wbuf,0,sizeof(wbuf));
    i = reg_len;
    while(i){
        i--;
        wbuf[i] = reg_addr;
        reg_addr >>= 8;
    }
    // ret = i2c_master_send(ts_data->salve_addr, wbuf, reg_len);
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, ts_data->salve_addr, wbuf, reg_len, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
    if(rlen){
    //     ret |= i2c_master_recv(ts_data->salve_addr, rbuf, rlen);
        ret |= i2c_master_read_from_device(I2C_MASTER_NUM, ts_data->salve_addr, rbuf, rlen, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
    }
    return ret < 0 ? -1:0;
}

int gpio_set_value(uint32_t gpio_id,bool vlue)
{
    gpio_set_level(gpio_id,vlue);
    return 0;
}   
bool gpio_get_value(uint32_t gpio_id) 
{
 return gpio_get_level(gpio_id);
}
void hyn_delay_ms(int cnt)
{
    vTaskDelay(cnt/portTICK_PERIOD_MS);
}