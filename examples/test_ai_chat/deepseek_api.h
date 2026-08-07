/**
 * @file      deepseek_api.h
 * @brief     DeepSeek Chat Completions client for the PDA AI page.
 */
#pragma once

#include <stddef.h>
#include <string>

using namespace std;

typedef struct {
    string text;
    bool success;
    string error;
} deepseek_response_t;

deepseek_response_t deepseek_send_text(const char *prompt,
                                       const char *api_key,
                                       const char *model = "deepseek-chat");

