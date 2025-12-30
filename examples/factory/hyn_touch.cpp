#include "Arduino.h"
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hyn_core.h"
#include "utilities.h"

#define CONFIG_EXAMPLE_TOUCH_I2C_SDA_PIN BOARD_TOUCH_SDA
#define CONFIG_EXAMPLE_TOUCH_I2C_SCL_PIN BOARD_TOUCH_SCL
#define CONFIG_EXAMPLE_TOUCH_RST_PIN BOARD_TOUCH_RST
#define CONFIG_EXAMPLE_TOUCH_INT_PIN BOARD_TOUCH_INT

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

const static char *TAG = "[HYN]";
static struct hyn_ts_data *hyn_data;
static xQueueHandle gpio_evt_queue;
static bool touch_press_flag = false;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, (BaseType_t *)NULL);
    touch_press_flag = true;
}

static void touch_int_handler(void *arg)
{
    uint32_t io_num;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            int ret;
            hyn_data->hyn_irq_flg = 1;
            if (hyn_data->work_mode < DIFF_MODE)
            {
                ret = hyn_data->hyn_fuc_used->tp_report(); // Read point

                for (u8 i = 0; i < hyn_data->rp_buf.rep_num; i++)
                { // Modify the coordinate origin according to the configuration
                    if (hyn_data->plat_data.swap_xy)
                    {
                        u16 tmp = hyn_data->rp_buf.pos_info[i].pos_x;
                        hyn_data->rp_buf.pos_info[i].pos_x = hyn_data->rp_buf.pos_info[i].pos_y;
                        hyn_data->rp_buf.pos_info[i].pos_y = tmp;
                    }
                    if (hyn_data->plat_data.reverse_x)
                        hyn_data->rp_buf.pos_info[i].pos_x = hyn_data->plat_data.x_resolution - hyn_data->rp_buf.pos_info[i].pos_x;
                    if (hyn_data->plat_data.reverse_y)
                        hyn_data->rp_buf.pos_info[i].pos_y = hyn_data->plat_data.y_resolution - hyn_data->rp_buf.pos_info[i].pos_y;
                }
                printf("ret:%d num:%d xy:", ret, hyn_data->rp_buf.rep_num);
                for (int i = 0; i < hyn_data->rp_buf.rep_num; i++)
                {
                    printf("(%d,%d) ", hyn_data->rp_buf.pos_info[i].pos_x, hyn_data->rp_buf.pos_info[i].pos_y);
                }
                printf("\n");
            }
            hyn_data->rp_buf.report_need = REPORT_NONE;
        }
    }
}

uint8_t hyn_touch_get_point(int16_t *x_array, int16_t *y_array, uint8_t get_point)
{
    uint32_t io_num;

    // if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
    // {
    if(touch_press_flag) {
        touch_press_flag = false;
    } else {
        return 0;
    }

    int ret;
    hyn_data->hyn_irq_flg = 1;
    if (hyn_data->work_mode < DIFF_MODE)
    {
        ret = hyn_data->hyn_fuc_used->tp_report(); // Read point

        for (u8 i = 0; i < hyn_data->rp_buf.rep_num; i++)
        { // Modify the coordinate origin according to the configuration
            if (hyn_data->plat_data.swap_xy)
            {
                u16 tmp = hyn_data->rp_buf.pos_info[i].pos_x;
                hyn_data->rp_buf.pos_info[i].pos_x = hyn_data->rp_buf.pos_info[i].pos_y;
                hyn_data->rp_buf.pos_info[i].pos_y = tmp;
            }
            if (hyn_data->plat_data.reverse_x)
                hyn_data->rp_buf.pos_info[i].pos_x = hyn_data->plat_data.x_resolution - hyn_data->rp_buf.pos_info[i].pos_x;
            if (hyn_data->plat_data.reverse_y)
                hyn_data->rp_buf.pos_info[i].pos_y = hyn_data->plat_data.y_resolution - hyn_data->rp_buf.pos_info[i].pos_y;
        }
        // printf("ret:%d num:%d xy:", ret, hyn_data->rp_buf.rep_num);
        for (int i = 0; i < hyn_data->rp_buf.rep_num; i++)
        {
            if(i < get_point)
            {
                x_array[i] = hyn_data->rp_buf.pos_info[i].pos_x;
                y_array[i] = hyn_data->rp_buf.pos_info[i].pos_y;
            }
            // printf("(%d,%d) ", hyn_data->rp_buf.pos_info[i].pos_x, hyn_data->rp_buf.pos_info[i].pos_y);
        }
        // printf("\n");
    }
    hyn_data->rp_buf.report_need = REPORT_NONE;

    return hyn_data->rp_buf.rep_num;

        
    // }
}

int hyn_touch_init(void)
{
    int ret = 0;
    static struct hyn_ts_data ts_data;
    memset((void *)&ts_data, 0, sizeof(ts_data));
    hyn_data = &ts_data;
    ESP_LOGI(TAG, HYN_DRIVER_VERSION);

    /*************************************************************/
    //    handle            chip types
    // &cst66xx_fuc   /*suport 36xx 35xx 66xx 68xx 148E*/
    // &cst3xx_fuc    /*suport 328 128 140 148 340 348*/
    // &cst226se_fuc  /*suport 226se 8922*/
    /*************************************************************/

    struct hyn_ts_fuc *support_touch_list[] = {
        (struct hyn_ts_fuc *)&cst66xx_fuc,
        (struct hyn_ts_fuc *)&cst3xx_fuc,
        (struct hyn_ts_fuc *)&cst226se_fuc};

    hyn_data->hyn_fuc_used = &cst66xx_fuc;                         // 根据芯片型号赋值
    hyn_data->plat_data.max_touch_num = MAX_POINTS_REPORT;         // 最大手指数
    hyn_data->plat_data.irq_gpio = CONFIG_EXAMPLE_TOUCH_INT_PIN;   // 中断脚配置
    hyn_data->plat_data.reset_gpio = CONFIG_EXAMPLE_TOUCH_RST_PIN; // rest脚配置

    // 配置rst 脚为push-pullp输出模式，输出1
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << hyn_data->plat_data.reset_gpio);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // 初始化I2c master ,配置速率、master addr
    hyn_i2c_init(CONFIG_EXAMPLE_TOUCH_I2C_SDA_PIN, CONFIG_EXAMPLE_TOUCH_I2C_SCL_PIN);

    // Touch chip initialization
    for (int i = 0; i < ARRAY_SIZE(support_touch_list); i++)
    {
        hyn_data->hyn_fuc_used = support_touch_list[i];
        ret = hyn_data->hyn_fuc_used->tp_chip_init(hyn_data);
        if (!ret)
        {
            ESP_LOGI(TAG, "Touch init SUCCEED");
            ESP_LOGI(TAG, "IC_info fw_project_id:%lx", hyn_data->hw_info.fw_project_id);
            ESP_LOGI(TAG, "ictype:[%lx]", hyn_data->hw_info.fw_chip_type);
            ESP_LOGI(TAG, "fw_ver:%lx", hyn_data->hw_info.fw_ver);
            break;
        }
    }
    if (ret)
    {
        ESP_LOGE(TAG, "I2c NAk");
        // return;
    }

    // 配置 int脚为 输入pull up，开启gpio 下降沿中断
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL << hyn_data->plat_data.irq_gpio);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // start gpio task
    // xTaskCreate(touch_int_handler, "touch_int_handler", 2048, NULL, 10, (TaskHandle_t *)NULL);
    // install gpio isr service
    gpio_install_isr_service(0);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add((gpio_num_t)hyn_data->plat_data.irq_gpio, gpio_isr_handler, (void *)hyn_data->plat_data.irq_gpio);

    return !ret;
}
