/**
 * @file      http_utils.h
 * @author    LilyGo
 * @license   MIT
 * @copyright Copyright (c) 2025  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2025-04-01
 * @brief     Shared HTTPS request utilities for network-connected features.
 */
#pragma once

#include <stdint.h>
#include <string>

using namespace std;

typedef struct {
    int status_code;
    string body;
    bool success;
} http_response_t;

/**
 * @brief Check WiFi connectivity and show popup if disconnected.
 * @param feature_name Name of the feature requesting WiFi (shown in popup).
 * @return true if WiFi is connected.
 */
bool http_require_wifi(const char *feature_name);

/**
 * @brief Perform an HTTPS GET request.
 * @param url Full URL to fetch.
 * @param timeout_ms Request timeout in milliseconds (default 10000).
 * @return http_response_t with status_code, body, and success flag.
 */
http_response_t http_get(const char *url, uint32_t timeout_ms = 10000);

/**
 * @brief Perform an HTTPS POST request.
 * @param url Full URL to post to.
 * @param body Request body content.
 * @param content_type Content-Type header value.
 * @param auth_header Optional Authorization header value (empty string to skip).
 * @param timeout_ms Request timeout in milliseconds (default 15000).
 * @return http_response_t with status_code, body, and success flag.
 */
http_response_t http_post(const char *url, const string &body,
                          const char *content_type = "application/json",
                          const char *auth_header = "",
                          uint32_t timeout_ms = 15000);

/**
 * @brief Perform an HTTPS POST with raw binary/large body using streaming write.
 * @param url Full URL to post to.
 * @param data Pointer to raw data.
 * @param data_len Length of data.
 * @param content_type Content-Type header value.
 * @param timeout_ms Request timeout in milliseconds.
 * @return http_response_t with status_code, body, and success flag.
 */
http_response_t http_post_large(const char *url, const uint8_t *data, size_t data_len,
                                const char *content_type = "application/json",
                                uint32_t timeout_ms = 30000);

/**
 * @brief Base64 encode data using mbedTLS (PSRAM-allocated output).
 * @param data Input data pointer.
 * @param len Input data length.
 * @param out_len Output: length of encoded string.
 * @return PSRAM-allocated base64 string (caller must free with free()).
 */
char *base64_encode_psram(const uint8_t *data, size_t len, size_t *out_len);
