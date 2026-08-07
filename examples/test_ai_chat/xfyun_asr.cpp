/**
 * @file      xfyun_asr.cpp
 * @brief     iFlytek RTASR-LLM WebSocket client implementation.
 */
#include "xfyun_asr.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <cJSON.h>
#include <esp_system.h>
#include <mbedtls/base64.h>
#include <mbedtls/md.h>
#include <time.h>

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <vector>

namespace {

static const char *const XFYUN_HOST = "office-api-ast-dx.iflyaisol.com";
static const uint16_t XFYUN_PORT = 443;
static const char *const XFYUN_PATH = "/ast/communicate/v1";

struct query_param_t {
    const char *key;
    string value;
};

struct asr_context_t {
    bool connected = false;
    bool server_ready = false;
    bool disconnected = false;
    bool finished = false;
    bool sent_end = false;
    string text;
    string error;
    string client_uuid;
    string server_session_id;
};

static WebSocketsClient g_websocket;
static asr_context_t *g_context = nullptr;

static string payload_to_string(uint8_t *payload, size_t length)
{
    if (!payload || length == 0) return string();
    return string(reinterpret_cast<const char *>(payload), length);
}

static string url_encode(const string &value)
{
    static const char hex[] = "0123456789ABCDEF";
    string out;
    out.reserve(value.size() * 3);

    for (unsigned char c : value) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' ||
            c == '.' || c == '~') {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(hex[(c >> 4) & 0x0F]);
            out.push_back(hex[c & 0x0F]);
        }
    }
    return out;
}

static bool hmac_sha1_base64(const string &key, const string &message,
                             string *output)
{
    unsigned char digest[20] = {};
    unsigned char encoded[32] = {};
    size_t encoded_len = 0;

    const mbedtls_md_info_t *info =
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (!info) return false;

    int rc = mbedtls_md_hmac(
        info, reinterpret_cast<const unsigned char *>(key.data()), key.size(),
        reinterpret_cast<const unsigned char *>(message.data()), message.size(),
        digest);
    if (rc != 0) return false;

    rc = mbedtls_base64_encode(encoded, sizeof(encoded), &encoded_len,
                               digest, sizeof(digest));
    if (rc != 0) return false;

    output->assign(reinterpret_cast<const char *>(encoded), encoded_len);
    return true;
}

static string make_uuid()
{
    char uuid[40] = {};
    snprintf(uuid, sizeof(uuid), "%08lx-%04lx-%04lx-%04lx-%08lx%04lx",
             static_cast<unsigned long>(esp_random()),
             static_cast<unsigned long>(esp_random() & 0xFFFF),
             static_cast<unsigned long>(esp_random() & 0xFFFF),
             static_cast<unsigned long>(esp_random() & 0xFFFF),
             static_cast<unsigned long>(esp_random()),
             static_cast<unsigned long>(esp_random() & 0xFFFF));
    return uuid;
}

static bool make_china_utc(string *output)
{
    const time_t now = time(nullptr);
    // An unsynchronized ESP32 clock starts close to 1970. Do not sign with it.
    if (now < 1700000000) return false;

    // The RTASR-LLM documentation uses the current China time and +0800.
    const time_t china_time = now + 8 * 60 * 60;
    struct tm tm_info = {};
    if (!gmtime_r(&china_time, &tm_info)) return false;

    char utc[32] = {};
    if (strftime(utc, sizeof(utc), "%Y-%m-%dT%H:%M:%S+0800", &tm_info) == 0)
        return false;
    *output = utc;
    return true;
}

static const char *describe_xfyun_error(const char *code)
{
    if (!code) return "未知错误";
    if (strcmp(code, "35010") == 0) return "APIKey（accessKeyId）不存在，或应用与当前服务不匹配";
    if (strcmp(code, "35017") == 0) return "AccessKeyId与AppID不匹配";
    if (strcmp(code, "35013") == 0) return "时间格式错误";
    if (strcmp(code, "35014") == 0) return "客户端时间偏差过大";
    if (strcmp(code, "35030") == 0) return "签名过期或UUID重复";
    if (strcmp(code, "37005") == 0) return "长时间没有发送音频";
    if (strcmp(code, "37010") == 0) return "发送end后仍继续发送音频";
    if (strcmp(code, "37012") == 0) return "握手后立即发送了end";
    return "讯飞服务端返回错误";
}

static bool reason_has_code(const string &reason, const char *code)
{
    return code && reason.find(code) != string::npos;
}

static void set_handshake_error(asr_context_t *ctx, const string &reason)
{
    if (!ctx) return;

    if (reason_has_code(reason, "35010")) {
        ctx->error =
            "讯飞握手失败 HTTP35010：APIKey（accessKeyId）不存在，或AppID与服务不匹配。请确认三项凭据属于同一应用且已开通RTASR服务";
    } else if (reason_has_code(reason, "35017")) {
        ctx->error =
            "讯飞握手失败 HTTP35017：AccessKeyId与AppID不匹配，请检查RTASR-LLM三项配置";
    } else if (reason_has_code(reason, "35013")) {
        ctx->error = "讯飞握手失败 HTTP35013：时间格式错误，请检查设备NTP时间";
    } else if (reason_has_code(reason, "35014")) {
        ctx->error = "讯飞握手失败 HTTP35014：设备时间偏差过大，请先同步NTP";
    } else if (reason_has_code(reason, "35030")) {
        ctx->error = "讯飞握手失败 HTTP35030：签名过期或UUID重复，请重试并检查时间";
    }
}

static string make_signed_path(const char *app_id, const char *api_key,
                               const char *api_secret, const string &uuid,
                               string *error)
{
    string utc;
    if (!make_china_utc(&utc)) {
        *error = "设备时间尚未同步，无法生成讯飞鉴权参数";
        Serial.println("[讯飞ASR] 鉴权失败：设备时间尚未同步，请确认WiFi后已完成NTP");
        return string();
    }

    // Never print the key itself. Lengths are enough to detect empty/wrong
    // config files while keeping credentials out of the serial log.
    Serial.printf("[讯飞ASR] 鉴权参数长度：AppID=%u，AccessKeyId=%u，AccessKeySecret=%u\n",
                  static_cast<unsigned>(strlen(app_id)),
                  static_cast<unsigned>(strlen(api_key)),
                  static_cast<unsigned>(strlen(api_secret)));
    Serial.printf("[讯飞ASR] 鉴权时间=%s，客户端UUID=%s\n",
                  utc.c_str(), uuid.c_str());

    vector<query_param_t> params = {
        {"accessKeyId", api_key},
        {"appId", app_id},
        {"audio_encode", "pcm_s16le"},
        {"lang", "autodialect"},
        {"samplerate", "16000"},
        {"utc", utc},
        {"uuid", uuid},
    };

    sort(params.begin(), params.end(), [](const query_param_t &a,
                                          const query_param_t &b) {
        return strcmp(a.key, b.key) < 0;
    });

    string base_string;
    string query_string;
    for (size_t i = 0; i < params.size(); ++i) {
        if (i != 0) {
            base_string += '&';
            query_string += '&';
        }

        const string encoded_key = url_encode(params[i].key);
        const string encoded_value = url_encode(params[i].value);
        base_string += encoded_key;
        base_string += '=';
        base_string += encoded_value;
        query_string += encoded_key;
        query_string += '=';
        query_string += encoded_value;
    }

    string signature;
    if (!hmac_sha1_base64(api_secret, base_string, &signature)) {
        *error = "HMAC-SHA1签名计算失败";
        Serial.println("[讯飞ASR] 鉴权失败：HMAC-SHA1签名计算失败");
        return string();
    }

    query_string += "&signature=";
    query_string += url_encode(signature);
    Serial.printf("[讯飞ASR] 签名完成：参数数=%u，baseString长度=%u，签名长度=%u，URL长度=%u\n",
                  static_cast<unsigned>(params.size()),
                  static_cast<unsigned>(base_string.length()),
                  static_cast<unsigned>(signature.length()),
                  static_cast<unsigned>(query_string.length()));
    return string(XFYUN_PATH) + "?" + query_string;
}

static void append_result_words(cJSON *data, asr_context_t *ctx)
{
    if (!data || !ctx) return;

    cJSON *cn = cJSON_GetObjectItem(data, "cn");
    cJSON *st = cn ? cJSON_GetObjectItem(cn, "st") : nullptr;
    if (!st) return;

    bool final_segment = false;
    cJSON *type = cJSON_GetObjectItem(st, "type");
    if (cJSON_IsString(type) && type->valuestring)
        final_segment = strcmp(type->valuestring, "0") == 0;
    else if (cJSON_IsNumber(type))
        final_segment = type->valueint == 0;

    cJSON *ls = cJSON_GetObjectItem(st, "ls");
    if (!type && cJSON_IsTrue(ls)) final_segment = true;
    if (!final_segment) return;

    cJSON *rt = cJSON_GetObjectItem(st, "rt");
    if (!cJSON_IsArray(rt)) return;

    cJSON *rt_item = nullptr;
    cJSON_ArrayForEach(rt_item, rt) {
        cJSON *ws = cJSON_GetObjectItem(rt_item, "ws");
        if (!cJSON_IsArray(ws)) continue;

        cJSON *ws_item = nullptr;
        cJSON_ArrayForEach(ws_item, ws) {
            cJSON *cw = cJSON_GetObjectItem(ws_item, "cw");
            if (!cJSON_IsArray(cw)) continue;

            cJSON *cw_item = nullptr;
            cJSON_ArrayForEach(cw_item, cw) {
                cJSON *word = cJSON_GetObjectItem(cw_item, "w");
                if (cJSON_IsString(word) && word->valuestring)
                    ctx->text += word->valuestring;
            }
        }
    }
}

static cJSON *json_data_object(cJSON *root, cJSON **owned_data)
{
    *owned_data = nullptr;
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (cJSON_IsString(data) && data->valuestring) {
        *owned_data = cJSON_Parse(data->valuestring);
        if (*owned_data) return *owned_data;
    }
    return cJSON_IsObject(data) ? data : root;
}

static cJSON *json_code(cJSON *root, cJSON *data)
{
    cJSON *code = cJSON_GetObjectItem(root, "code");
    if (!code && data != root) code = cJSON_GetObjectItem(data, "code");
    return code;
}

static const char *json_text(cJSON *item)
{
    return cJSON_IsString(item) && item->valuestring ? item->valuestring : nullptr;
}

static bool json_code_is_error(cJSON *code)
{
    if (cJSON_IsString(code) && code->valuestring)
        return strcmp(code->valuestring, "0") != 0;
    if (cJSON_IsNumber(code)) return code->valueint != 0;
    return false;
}

static void handle_text_message(const string &json)
{
    if (!g_context) return;

    cJSON *root = cJSON_ParseWithLength(json.c_str(), json.length());
    if (!root) {
        g_context->error = "讯飞返回了无法解析的JSON";
        Serial.printf("[讯飞ASR] 收到非法JSON，长度=%u\n",
                      static_cast<unsigned>(json.length()));
        return;
    }

    cJSON *action_item = cJSON_GetObjectItem(root, "action");
    cJSON *msg_type_item = cJSON_GetObjectItem(root, "msg_type");
    const char *action = json_text(action_item);
    const char *msg_type = json_text(msg_type_item);
    Serial.printf("[讯飞ASR] 收到服务端JSON：长度=%u，action=%s，msg_type=%s\n",
                  static_cast<unsigned>(json.length()),
                  action ? action : "无",
                  msg_type ? msg_type : "无");

    cJSON *owned_data = nullptr;
    cJSON *data = json_data_object(root, &owned_data);

    cJSON *code = json_code(root, data);
    const char *code_text = json_text(code);
    char numeric_code[24] = {};
    if (!code_text && cJSON_IsNumber(code)) {
        snprintf(numeric_code, sizeof(numeric_code), "%d", code->valueint);
        code_text = numeric_code;
    }

    if ((action && strcmp(action, "started") == 0) ||
        (msg_type && strcmp(msg_type, "action") == 0 &&
         (!code || !json_code_is_error(code)))) {
        if (!g_context->server_ready) {
            Serial.println("[讯飞ASR] 服务端已就绪，可以发送PCM音频");
        }
        g_context->server_ready = true;
    }

    cJSON *session = cJSON_GetObjectItem(root, "sessionId");
    if (!cJSON_IsString(session)) session = cJSON_GetObjectItem(root, "sid");
    if (cJSON_IsString(session) && session->valuestring) {
        g_context->server_session_id = session->valuestring;
        Serial.printf("[讯飞ASR] 收到服务端sessionId：%s\n",
                      g_context->server_session_id.c_str());
    }
    if (data != root) {
        session = cJSON_GetObjectItem(data, "sessionId");
        if (cJSON_IsString(session) && session->valuestring) {
            g_context->server_session_id = session->valuestring;
            Serial.printf("[讯飞ASR] 收到服务端sessionId：%s\n",
                          g_context->server_session_id.c_str());
        }
    }

    if (action && strcmp(action, "error") == 0) {
        cJSON *desc = cJSON_GetObjectItem(root, "desc");
        if (!desc && data != root) desc = cJSON_GetObjectItem(data, "desc");
        const char *desc_text = json_text(desc);
        g_context->error = desc_text ? desc_text : "讯飞返回action错误";
        if (code_text) {
            const char *meaning = describe_xfyun_error(code_text);
            g_context->error += "（错误码";
            g_context->error += code_text;
            g_context->error += "：";
            g_context->error += meaning;
            g_context->error += "）";
        }
        Serial.printf("[讯飞ASR] 服务端action错误：%s\n",
                      g_context->error.c_str());
        if (owned_data) cJSON_Delete(owned_data);
        cJSON_Delete(root);
        return;
    }

    if (json_code_is_error(code)) {
        cJSON *desc = cJSON_GetObjectItem(root, "desc");
        if (!desc && data != root) desc = cJSON_GetObjectItem(data, "desc");
        const char *desc_text = json_text(desc);
        g_context->error = desc_text ? desc_text : "讯飞服务端返回错误";
        if (code_text) {
            g_context->error += "（错误码";
            g_context->error += code_text;
            g_context->error += "：";
            g_context->error += describe_xfyun_error(code_text);
            g_context->error += "）";
        }
        Serial.printf("[讯飞ASR] 服务端错误码=%s，说明=%s\n",
                      code_text ? code_text : "未知",
                      g_context->error.c_str());
        if (owned_data) cJSON_Delete(owned_data);
        cJSON_Delete(root);
        return;
    }

    cJSON *res_type = cJSON_GetObjectItem(root, "res_type");
    const char *res_type_text = json_text(res_type);
    if (res_type_text && strcmp(res_type_text, "asr") != 0) {
        g_context->error = "讯飞返回了非ASR结果";
        Serial.printf("[讯飞ASR] 非ASR响应：res_type=%s\n", res_type_text);
        if (owned_data) cJSON_Delete(owned_data);
        cJSON_Delete(root);
        return;
    }

    append_result_words(data, g_context);
    cJSON *last = cJSON_GetObjectItem(data, "ls");
    if (cJSON_IsTrue(last)) g_context->finished = true;
    if (action && strcmp(action, "result") == 0 && g_context->sent_end)
        g_context->finished = true;

    if (g_context->finished) {
        Serial.printf("[讯飞ASR] 收到最终识别结果，当前文本长度=%u\n",
                      static_cast<unsigned>(g_context->text.length()));
    }

    if (owned_data) cJSON_Delete(owned_data);
    cJSON_Delete(root);
}

static void websocket_event(WStype_t type, uint8_t *payload, size_t length)
{
    if (!g_context) return;

    if (type == WStype_CONNECTED) {
        g_context->connected = true;
        Serial.println("[讯飞ASR] WebSocket握手成功，等待服务端started消息");
        return;
    }

    if (type == WStype_ERROR) {
        const string reason = payload_to_string(payload, length);
        Serial.printf("[讯飞ASR] WebSocket底层错误：%s\n",
                      reason.empty() ? "未提供错误内容" : reason.c_str());
        set_handshake_error(g_context, reason);
        if (g_context->error.empty()) {
            g_context->error = reason.empty() ? "WebSocket底层错误" : reason;
        }
        return;
    }

    if (type == WStype_DISCONNECTED) {
        g_context->disconnected = true;
        const string reason = payload_to_string(payload, length);
        Serial.printf("[讯飞ASR] WebSocket已断开：%s\n",
                      reason.empty() ? "服务端未提供原因" : reason.c_str());

        // WebSocketsClient reports this as "HTTP 35010" (with a space),
        // while some firmware versions display "HTTP35010". Match both.
        set_handshake_error(g_context, reason);
        if (!g_context->error.empty()) return;

        if (g_context->sent_end && !g_context->text.empty()) {
            // The service may close immediately after delivering the final
            // result. Treat that close as normal.
            g_context->finished = true;
            Serial.println("[讯飞ASR] 服务端已在最终结果后正常关闭连接");
            return;
        }

        g_context->error = "讯飞WebSocket意外断开";
        if (!reason.empty()) {
            g_context->error += "：";
            g_context->error += reason;
        }
        return;
    }

    if (type != WStype_TEXT || !payload || length == 0) return;
    handle_text_message(payload_to_string(payload, length));
}

} // namespace

xfyun_asr_response_t xfyun_asr_transcribe_pcm(const uint8_t *pcm_data,
                                              size_t pcm_len,
                                              const char *app_id,
                                              const char *api_key,
                                              const char *api_secret)
{
    xfyun_asr_response_t response = {"", false, ""};
    if (!pcm_data || pcm_len == 0) {
        response.error = "没有可识别的PCM音频";
        Serial.println("[讯飞ASR] 识别停止：PCM音频为空");
        return response;
    }

    if (!app_id || !api_key || !api_secret || !app_id[0] || !api_key[0] ||
        !api_secret[0]) {
        response.error = "讯飞AppID/APIKey/APISecret配置不完整";
        Serial.println("[讯飞ASR] 识别停止：AppID、APIKey、APISecret配置不完整");
        return response;
    }

    Serial.printf("[讯飞ASR] 开始识别：PCM=%u字节，16000Hz，16bit，单声道\n",
                  static_cast<unsigned>(pcm_len));
    Serial.println("[讯飞ASR] 鉴权映射：讯飞控制台APIKey作为accessKeyId，APISecret作为签名密钥");

    asr_context_t context;
    context.client_uuid = make_uuid();
    const string path = make_signed_path(app_id, api_key, api_secret,
                                         context.client_uuid, &response.error);
    if (path.empty()) {
        Serial.printf("[讯飞ASR] 生成鉴权URL失败：%s\n", response.error.c_str());
        return response;
    }

    g_context = &context;
    g_websocket.disconnect();
    g_websocket.onEvent(websocket_event);
    g_websocket.setReconnectInterval(0);
    Serial.printf("[讯飞ASR] 正在连接：wss://%s:%u%s?签名参数\n",
                  XFYUN_HOST, static_cast<unsigned>(XFYUN_PORT), XFYUN_PATH);
    Serial.printf("[讯飞ASR] 鉴权URL长度=%u，不输出密钥和完整签名\n",
                  static_cast<unsigned>(path.length()));
    g_websocket.beginSSL(XFYUN_HOST, XFYUN_PORT, path.c_str());

    uint32_t wait_start = millis();
    while (!context.connected && !context.disconnected && context.error.empty() &&
           millis() - wait_start < 12000) {
        g_websocket.loop();
        delay(10);
    }

    if (!context.connected) {
        response.error = context.error.empty() ? "讯飞WebSocket连接超时" : context.error;
        Serial.printf("[讯飞ASR] WebSocket连接失败：%s\n", response.error.c_str());
        g_websocket.disconnect();
        g_context = nullptr;
        return response;
    }

    // RTASR-LLM needs a short initialization period after the WebSocket 101.
    // Keep servicing the socket so the started/sessionId message is received.
    Serial.println("[讯飞ASR] 等待服务端就绪（最多1500ms）");
    const uint32_t ready_start = millis();
    while (!context.server_ready && !context.disconnected && context.error.empty() &&
           millis() - ready_start < 1500) {
        g_websocket.loop();
        delay(10);
    }

    if (context.disconnected || !context.error.empty()) {
        response.error = context.error.empty() ? "等待讯飞服务端就绪时连接断开" : context.error;
        Serial.printf("[讯飞ASR] 等待服务端就绪失败：%s\n", response.error.c_str());
        g_websocket.disconnect();
        g_context = nullptr;
        return response;
    }
    if (!context.server_ready) {
        Serial.println("[讯飞ASR] 警告：未收到started消息，仍按协议继续发送PCM");
    }
    if (!context.server_session_id.empty()) {
        Serial.printf("[讯飞ASR] 使用服务端sessionId：%s\n",
                      context.server_session_id.c_str());
    }

    const size_t frame_size = 1280; // 40 ms at 16 kHz, 16-bit mono.
    size_t offset = 0;
    size_t frame_index = 0;
    Serial.println("[讯飞ASR] 开始发送PCM：每40ms发送1280字节");
    while (offset < pcm_len && context.error.empty() && !context.disconnected) {
        size_t frame_len = pcm_len - offset;
        if (frame_len > frame_size) frame_len = frame_size;

        if (!g_websocket.sendBIN(pcm_data + offset, frame_len)) {
            context.error = "PCM发送失败，WebSocket可能已断开";
            Serial.printf("[讯飞ASR] PCM发送失败：第%u帧，长度=%u\n",
                          static_cast<unsigned>(frame_index),
                          static_cast<unsigned>(frame_len));
            break;
        }

        offset += frame_len;
        ++frame_index;
        if ((frame_index % 25) == 0 || offset >= pcm_len) {
            Serial.printf("[讯飞ASR] PCM发送进度：第%u帧，%u/%u字节\n",
                          static_cast<unsigned>(frame_index),
                          static_cast<unsigned>(offset),
                          static_cast<unsigned>(pcm_len));
        }

        const uint32_t frame_start = millis();
        while (millis() - frame_start < 40 && context.error.empty() &&
               !context.disconnected) {
            g_websocket.loop();
            delay(2);
        }
    }

    if (context.error.empty() && !context.disconnected) {
        char end_message[160] = {};
        if (!context.server_session_id.empty()) {
            snprintf(end_message, sizeof(end_message),
                     "{\"end\":true,\"sessionId\":\"%s\"}",
                     context.server_session_id.c_str());
        } else {
            snprintf(end_message, sizeof(end_message), "{\"end\":true}");
        }

        Serial.printf("[讯飞ASR] 发送结束帧：sessionId=%s\n",
                      context.server_session_id.empty() ? "未收到，使用兼容格式" : "已收到");
        context.sent_end = g_websocket.sendTXT(end_message);
        if (!context.sent_end) {
            context.error = "讯飞结束帧发送失败";
            Serial.println("[讯飞ASR] 结束帧发送失败");
        } else {
            Serial.printf("[讯飞ASR] 结束帧已发送：%s\n", end_message);
        }
    }

    if (context.error.empty() && !context.disconnected) {
        Serial.println("[讯飞ASR] 等待最终识别结果（最多12秒）");
        wait_start = millis();
        while (!context.finished && context.error.empty() &&
               !context.disconnected && millis() - wait_start < 12000) {
            g_websocket.loop();
            delay(10);
        }
        if (!context.finished && context.error.empty() && !context.disconnected) {
            context.error = "等待讯飞最终识别结果超时";
            Serial.println("[讯飞ASR] 等待最终识别结果超时");
        }
    }

    g_websocket.disconnect();
    g_context = nullptr;

    if (!context.error.empty()) {
        response.error = context.error;
        Serial.printf("[讯飞ASR] 识别失败：%s\n", response.error.c_str());
        return response;
    }
    if (context.text.empty()) {
        response.error = "讯飞服务端没有返回识别文本";
        Serial.println("[讯飞ASR] 识别结束，但服务端没有返回文本");
        return response;
    }

    response.text = context.text;
    response.success = true;
    Serial.printf("[讯飞ASR] 识别成功：文本长度=%u，内容=%s\n",
                  static_cast<unsigned>(response.text.length()),
                  response.text.c_str());
    return response;
}

#else

xfyun_asr_response_t xfyun_asr_transcribe_pcm(const uint8_t *, size_t,
                                              const char *, const char *,
                                              const char *)
{
    return {"", false, "Not supported on desktop"};
}

#endif
