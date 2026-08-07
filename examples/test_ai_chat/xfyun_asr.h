/**
 * @file      xfyun_asr.h
 * @brief     iFlytek RTASR client for the T-Deck Pro microphone.
 *
 * The RTASR service accepts raw 16 kHz, 16-bit, mono PCM over a secure
 * WebSocket connection.  The caller owns the input buffer and receives a
 * normalised text result.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string>

using namespace std;

typedef struct {
    string text;
    bool success;
    string error;
} xfyun_asr_response_t;

/**
 * @brief Transcribe a raw PCM buffer with iFlytek RTASR.
 * @param pcm_data 16-bit, mono PCM samples (no WAV header).
 * @param pcm_len Length of pcm_data in bytes.
 * @param app_id iFlytek AppID.
 * @param api_key iFlytek APIKey/accessKeyId.
 * @param api_secret iFlytek APISecret/accessKeySecret.
 */
xfyun_asr_response_t xfyun_asr_transcribe_pcm(const uint8_t *pcm_data,
                                              size_t pcm_len,
                                              const char *app_id,
                                              const char *api_key,
                                              const char *api_secret);

