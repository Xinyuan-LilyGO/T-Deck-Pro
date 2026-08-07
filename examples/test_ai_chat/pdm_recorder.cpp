/**
 * @file      pdm_recorder.cpp
 * @brief     PDM microphone recorder using ESP-IDF I2S driver.
 */
#include "pdm_recorder.h"
#include "utilities.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <driver/i2s.h>
#include <esp_heap_caps.h>

#define I2S_PORT I2S_NUM_0

static bool i2s_installed = false;

static bool pdm_init(int sample_rate)
{
    if (i2s_installed) {
        i2s_driver_uninstall(I2S_PORT);
        i2s_installed = false;
    }

    /* I2S_NUM_0 may be in use by the audio player — uninstall it first */
    i2s_driver_uninstall(I2S_PORT);

    i2s_config_t i2s_config = {};
    i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    i2s_config.sample_rate = sample_rate;
    i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    i2s_config.dma_buf_count = 8;
    i2s_config.dma_buf_len = 1024;
    i2s_config.use_apll = false;

    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[PDM] i2s_driver_install failed: %d\n", err);
        return false;
    }

    i2s_pin_config_t pin_config = {};
    pin_config.bck_io_num = I2S_PIN_NO_CHANGE;
    pin_config.ws_io_num = BOARD_MIC_CLOCK;
    pin_config.data_out_num = I2S_PIN_NO_CHANGE;
    pin_config.data_in_num = BOARD_MIC_DATA;

    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[PDM] i2s_set_pin failed: %d\n", err);
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }

    i2s_installed = true;
    return true;
}

static void pdm_deinit()
{
    if (i2s_installed) {
        i2s_driver_uninstall(I2S_PORT);
        i2s_installed = false;
    }
}

static void write_wav_header(uint8_t *buf, int sample_rate, int num_samples)
{
    int data_size = num_samples * 2;
    int file_size = 44 + data_size - 8;

    memcpy(buf, "RIFF", 4);
    *(uint32_t *)(buf + 4) = file_size;
    memcpy(buf + 8, "WAVE", 4);
    memcpy(buf + 12, "fmt ", 4);
    *(uint32_t *)(buf + 16) = 16;
    *(uint16_t *)(buf + 20) = 1;         // PCM
    *(uint16_t *)(buf + 22) = 1;         // mono
    *(uint32_t *)(buf + 24) = sample_rate;
    *(uint32_t *)(buf + 28) = sample_rate * 2;  // byte rate
    *(uint16_t *)(buf + 32) = 2;         // block align
    *(uint16_t *)(buf + 34) = 16;        // bits per sample
    memcpy(buf + 36, "data", 4);
    *(uint32_t *)(buf + 40) = data_size;
}

bool pdm_record_wav(int duration_sec, int sample_rate, uint8_t **wav_out, size_t *wav_len)
{
    *wav_out = NULL;
    *wav_len = 0;

    if (!pdm_init(sample_rate)) return false;

    int total_samples = sample_rate * duration_sec;
    size_t total_bytes = total_samples * 2;
    size_t wav_size = 44 + total_bytes;

    uint8_t *wav = (uint8_t *)heap_caps_malloc(wav_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!wav) {
        Serial.printf("[PDM] Failed to allocate %zu bytes in PSRAM\n", wav_size);
        pdm_deinit();
        return false;
    }

    write_wav_header(wav, sample_rate, total_samples);

    /* Flush initial noisy samples */
    uint8_t flush_buf[2048];
    size_t flush_read = 0;
    i2s_read(I2S_PORT, flush_buf, sizeof(flush_buf), &flush_read, portMAX_DELAY);

    /* Record */
    Serial.printf("[PDM] Recording %d sec at %d Hz...\n", duration_sec, sample_rate);
    size_t offset = 44;
    size_t remaining = total_bytes;
    while (remaining > 0) {
        size_t to_read = remaining > 4096 ? 4096 : remaining;
        size_t bytes_read = 0;
        esp_err_t err = i2s_read(I2S_PORT, wav + offset, to_read, &bytes_read, pdMS_TO_TICKS(1000));
        if (err != ESP_OK || bytes_read == 0) break;
        offset += bytes_read;
        remaining -= bytes_read;
    }

    pdm_deinit();

    if (remaining > 0) {
        Serial.printf("[PDM] Recording incomplete: got %zu / %zu bytes\n", offset - 44, total_bytes);
    }

    /* Update WAV header with actual size */
    int actual_samples = (offset - 44) / 2;
    write_wav_header(wav, sample_rate, actual_samples);

    *wav_out = wav;
    *wav_len = offset;
    Serial.printf("[PDM] Recorded %zu bytes (%d samples)\n", offset, actual_samples);
    return true;
}

void pdm_restore_audio(void)
{
    /* Re-init I2S for audio output is handled by the Audio library
     * when audio.connecttoFS or similar is called next.
     * Nothing needed here — the Audio library re-installs I2S on demand. */
    Serial.println("[PDM] Audio restored (I2S freed for audio player)");
}

#else
bool pdm_record_wav(int duration_sec, int sample_rate, uint8_t **wav_out, size_t *wav_len)
{
    *wav_out = NULL; *wav_len = 0;
    return false;
}
void pdm_restore_audio(void) {}
#endif
