#include "Arduino.h"

#include "../factory/ui_deckpro_port.h"
#include "../factory/ui_scr_mrg.h"

static float g_lora_freq = 850.0f;
static int g_lora_bandwidth = 125;
static int g_lora_power = 22;

extern "C" bool scr_mgr_pop(bool anim)
{
    LV_UNUSED(anim);
    return false;
}

extern "C" float ui_lora_get_freq(void)
{
    return g_lora_freq;
}

extern "C" void ui_lora_set_freq(float freq)
{
    g_lora_freq = freq;
}

extern "C" int ui_lora_get_bandwidth(void)
{
    return g_lora_bandwidth;
}

extern "C" void ui_lora_set_bandwidth(float bd)
{
    g_lora_bandwidth = (int)bd;
}

extern "C" int ui_lora_get_power(void)
{
    return g_lora_power;
}

extern "C" void ui_lora_set_power(float po)
{
    g_lora_power = (int)po;
}
