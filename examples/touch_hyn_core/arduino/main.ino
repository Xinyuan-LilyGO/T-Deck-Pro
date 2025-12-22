
#include <Arduino.h>
#include "hyn_core.h"

#define CONFIG_EXAMPLE_TOUCH_I2C_SDA_PIN 13
#define CONFIG_EXAMPLE_TOUCH_I2C_SCL_PIN 14
#define CONFIG_EXAMPLE_TOUCH_RST_PIN 38
#define CONFIG_EXAMPLE_TOUCH_INT_PIN 12


const static char *TAG = "[HYN]";
static struct hyn_ts_data *hyn_data;


static xQueueHandle gpio_evt_queue;
static BaseType_t xHigherPriorityTaskWoken = pdFALSE;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, &xHigherPriorityTaskWoken);
}

static void touch_int_handler(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            int ret;
            hyn_data->hyn_irq_flg = 1;
            if(hyn_data->work_mode < DIFF_MODE){
                ret = hyn_data->hyn_fuc_used->tp_report(); //读点

                for(u8 i=0; i< hyn_data->rp_buf.rep_num; i++){   //根据配置修改坐标原点
                    if(hyn_data->plat_data.swap_xy){
                        u16 tmp = hyn_data->rp_buf.pos_info[i].pos_x;
                        hyn_data->rp_buf.pos_info[i].pos_x = hyn_data->rp_buf.pos_info[i].pos_y;
                        hyn_data->rp_buf.pos_info[i].pos_y = tmp;
                    }
                    if(hyn_data->plat_data.reverse_x)
                        hyn_data->rp_buf.pos_info[i].pos_x = hyn_data->plat_data.x_resolution-hyn_data->rp_buf.pos_info[i].pos_x;
                    if(hyn_data->plat_data.reverse_y)
                        hyn_data->rp_buf.pos_info[i].pos_y = hyn_data->plat_data.y_resolution-hyn_data->rp_buf.pos_info[i].pos_y;
                }
                printf("ret:%d num:%d xy:",ret,hyn_data->rp_buf.rep_num);
                for(int i = 0; i < hyn_data->rp_buf.rep_num; i++) {
                    printf("(%d,%d) ", hyn_data->rp_buf.pos_info[i].pos_x,hyn_data->rp_buf.pos_info[i].pos_y);
                }
                printf("\n");
            }
            hyn_data->rp_buf.report_need = REPORT_NONE;

        }
    }
}

void setup()
{
    int ret = 0;
    static struct hyn_ts_data ts_data;
    memset((void*)&ts_data,0,sizeof(ts_data));
    hyn_data = &ts_data;
    ESP_LOGI(TAG, HYN_DRIVER_VERSION);
/*************************************************************/
//    handle            chip types
// &cst66xx_fuc   /*suport 36xx 35xx 66xx 68xx 148E*/
// &cst36xxes_fuc /*suport 154es 3654es 3640es*/
// &cst3240_fuc   /*suport 3240 */
// &cst92xx_fuc   /*suport 9217、9220 */
// &cst3xx_fuc    /*suport 328 128 140 148 340 348*/
// &cst7xx_fuc    /*suport 726 826 830 836u*/
// &cst8xxT_fuc   /*suport 816t 816d 820 08C*/
// &cst226se_fuc  /*suport 226se 8922*/
/*************************************************************/
    hyn_data->hyn_fuc_used = &cst66xx_fuc;  //根据芯片型号赋值
    hyn_data->plat_data.max_touch_num = MAX_POINTS_REPORT;   //最大手指数
    hyn_data->plat_data.irq_gpio = CONFIG_EXAMPLE_TOUCH_INT_PIN;    //中断脚配置
    hyn_data->plat_data.reset_gpio = CONFIG_EXAMPLE_TOUCH_RST_PIN;  //rest脚配置

    //配置rst 脚为push-pullp输出模式，输出1
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<hyn_data->plat_data.reset_gpio);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    //初始化I2c master ,配置速率、master addr
    hyn_i2c_init(CONFIG_EXAMPLE_TOUCH_I2C_SDA_PIN, CONFIG_EXAMPLE_TOUCH_I2C_SCL_PIN);

    //触摸芯片初始化
    ret = hyn_data->hyn_fuc_used->tp_chip_init(hyn_data); 
    if(ret){
        printf("I2c NAk");
        //return;
    }
    printf("IC_info fw_project_id:%04lx ictype:%04lx fw_ver:%lx", hyn_data->hw_info.fw_project_id,hyn_data->hw_info.fw_chip_type,hyn_data->hw_info.fw_ver);

    //配置 int脚为 输入pull up，开启gpio 下降沿中断
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL << hyn_data->plat_data.irq_gpio);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(touch_int_handler, "touch_int_handler", 2048, NULL, 10, (TaskHandle_t *)NULL);
    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add((gpio_num_t )hyn_data->plat_data.irq_gpio, gpio_isr_handler, (void*) hyn_data->plat_data.irq_gpio);
}

void loop()
{
    delay(1000);
}

