/**
 * @file      http_utils.cpp
 * @author    LilyGo
 * @license   MIT
 * @copyright Copyright (c) 2025  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2025-04-01
 * @brief     Shared HTTPS request utilities implementation.
 */
#include "http_utils.h"

#ifdef ARDUINO
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <mbedtls/base64.h>
#include <esp_heap_caps.h>

bool http_require_wifi(const char *feature_name)
{
    return WiFi.status() == WL_CONNECTED;
}

http_response_t http_get(const char *url, uint32_t timeout_ms)
{
    http_response_t resp = {0, "", false};
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    http.setTimeout(timeout_ms);
    if (!http.begin(client, url)) {
        resp.body = "Failed to connect";
        return resp;
    }

    resp.status_code = http.GET();
    if (resp.status_code > 0) {
        resp.body = http.getString().c_str();
        resp.success = (resp.status_code >= 200 && resp.status_code < 300);
    } else {
        resp.body = http.errorToString(resp.status_code).c_str();
    }
    http.end();
    return resp;
}

http_response_t http_post(const char *url, const string &body,
                          const char *content_type,
                          const char *auth_header,
                          uint32_t timeout_ms)
{
    http_response_t resp = {0, "", false};
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    http.setTimeout(timeout_ms);
    if (!http.begin(client, url)) {
        resp.body = "Failed to connect";
        return resp;
    }

    http.addHeader("Content-Type", content_type);
    if (auth_header && auth_header[0] != '\0') {
        http.addHeader("Authorization", auth_header);
    }

    resp.status_code = http.POST(body.c_str());
    if (resp.status_code > 0) {
        resp.body = http.getString().c_str();
        resp.success = (resp.status_code >= 200 && resp.status_code < 300);
    } else {
        resp.body = http.errorToString(resp.status_code).c_str();
    }
    http.end();
    return resp;
}

http_response_t http_post_large(const char *url, const uint8_t *data, size_t data_len,
                                const char *content_type, uint32_t timeout_ms)
{
    http_response_t resp = {0, "", false};
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    http.setTimeout(timeout_ms);
    if (!http.begin(client, url)) {
        resp.body = "Failed to connect";
        return resp;
    }

    http.addHeader("Content-Type", content_type);

    resp.status_code = http.sendRequest("POST", (uint8_t *)data, data_len);
    if (resp.status_code > 0) {
        resp.body = http.getString().c_str();
        resp.success = (resp.status_code >= 200 && resp.status_code < 300);
    } else {
        resp.body = http.errorToString(resp.status_code).c_str();
    }
    http.end();
    return resp;
}

char *base64_encode_psram(const uint8_t *data, size_t len, size_t *out_len)
{
    size_t encoded_len = 0;
    mbedtls_base64_encode(NULL, 0, &encoded_len, data, len);

    char *encoded = (char *)heap_caps_malloc(encoded_len + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!encoded) {
        *out_len = 0;
        return NULL;
    }

    mbedtls_base64_encode((unsigned char *)encoded, encoded_len + 1, &encoded_len, data, len);
    encoded[encoded_len] = '\0';
    *out_len = encoded_len;
    return encoded;
}

#else
// Desktop stubs
bool http_require_wifi(const char *feature_name) { return false; }
http_response_t http_get(const char *url, uint32_t timeout_ms) { return {0, "Not supported on desktop", false}; }
http_response_t http_post(const char *url, const string &body, const char *content_type, const char *auth_header, uint32_t timeout_ms) { return {0, "Not supported on desktop", false}; }
http_response_t http_post_large(const char *url, const uint8_t *data, size_t data_len, const char *content_type, uint32_t timeout_ms) { return {0, "Not supported on desktop", false}; }
char *base64_encode_psram(const uint8_t *data, size_t len, size_t *out_len) { *out_len = 0; return NULL; }
#endif
