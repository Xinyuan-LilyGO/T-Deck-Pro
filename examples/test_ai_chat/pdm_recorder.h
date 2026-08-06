/**
 * @file      pdm_recorder.h
 * @brief     PDM microphone recorder for T-Deck Pro.
 *            Records audio from the onboard PDM mic and produces WAV data in PSRAM.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Record audio from PDM microphone into a WAV buffer in PSRAM.
 * @param duration_sec Recording duration in seconds.
 * @param sample_rate Sample rate in Hz (e.g., 16000).
 * @param wav_out Output pointer to WAV data (caller must free with free()).
 * @param wav_len Output length of WAV data.
 * @return true if recording succeeded.
 */
bool pdm_record_wav(int duration_sec, int sample_rate, uint8_t **wav_out, size_t *wav_len);

/**
 * @brief Re-initialize the I2S audio player after PDM recording.
 *        Call this after pdm_record_wav to restore audio output.
 */
void pdm_restore_audio(void);
