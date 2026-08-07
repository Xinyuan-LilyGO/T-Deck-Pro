/**
 * @file      deepseek_api.cpp
 * @brief     DeepSeek Chat Completions client implementation.
 */
#include "deepseek_api.h"
#include "http_utils.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <cJSON.h>

static const char kSpeechSystemPrompt[] =
    "Return concise plain text suitable for direct speech. Do not use "
    "Markdown, emojis, code blocks, lists, or line breaks. Prefer a short "
    "answer of no more than about 50 Chinese characters when possible.";

static deepseek_response_t parse_deepseek_response(const char *json)
{
    deepseek_response_t response = {"", false, ""};
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        response.error = "Invalid JSON from DeepSeek";
        return response;
    }

    cJSON *error = cJSON_GetObjectItem(root, "error");
    if (error) {
        cJSON *message = cJSON_GetObjectItem(error, "message");
        response.error = message && cJSON_IsString(message) ? message->valuestring : "DeepSeek API error";
        cJSON_Delete(root);
        return response;
    }

    cJSON *choices = cJSON_GetObjectItem(root, "choices");
    cJSON *choice = cJSON_IsArray(choices) ? cJSON_GetArrayItem(choices, 0) : nullptr;
    cJSON *message = choice ? cJSON_GetObjectItem(choice, "message") : nullptr;
    cJSON *content = message ? cJSON_GetObjectItem(message, "content") : nullptr;
    if (cJSON_IsString(content) && content->valuestring) {
        response.text = content->valuestring;
        response.success = true;
    } else {
        response.error = "DeepSeek response contains no text";
    }

    cJSON_Delete(root);
    return response;
}

deepseek_response_t deepseek_send_text(const char *prompt,
                                       const char *api_key,
                                       const char *model)
{
    deepseek_response_t response = {"", false, ""};
    if (!prompt || !prompt[0]) {
        response.error = "Empty prompt";
        return response;
    }
    if (!api_key || !api_key[0]) {
        response.error = "No DeepSeek API key";
        return response;
    }
    if (!model || !model[0]) model = "deepseek-chat";

    cJSON *root = cJSON_CreateObject();
    cJSON *messages = cJSON_CreateArray();
    cJSON *system_message = cJSON_CreateObject();
    cJSON *message = cJSON_CreateObject();
    if (!root || !messages || !system_message || !message) {
        if (root) cJSON_Delete(root);
        if (messages) cJSON_Delete(messages);
        if (system_message) cJSON_Delete(system_message);
        if (message) cJSON_Delete(message);
        response.error = "Out of memory while building DeepSeek request";
        return response;
    }

    cJSON_AddStringToObject(root, "model", model);
    cJSON_AddBoolToObject(root, "stream", false);
    cJSON_AddStringToObject(system_message, "role", "system");
    cJSON_AddStringToObject(system_message, "content", kSpeechSystemPrompt);
    cJSON_AddStringToObject(message, "role", "user");
    cJSON_AddStringToObject(message, "content", prompt);
    cJSON_AddItemToArray(messages, system_message);
    cJSON_AddItemToArray(messages, message);
    cJSON_AddItemToObject(root, "messages", messages);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) {
        response.error = "Failed to build DeepSeek JSON";
        return response;
    }

    string body = json;
    free(json);
    string auth = "Bearer ";
    auth += api_key;

    http_response_t http_response = http_post(
        "https://api.deepseek.com/chat/completions", body,
        "application/json", auth.c_str(), 30000);
    if (!http_response.success) {
        response.error = "DeepSeek HTTP " + to_string(http_response.status_code) + ": " + http_response.body;
        return response;
    }
    return parse_deepseek_response(http_response.body.c_str());
}

#else

deepseek_response_t deepseek_send_text(const char *, const char *, const char *)
{
    return {"", false, "Not supported on desktop"};
}

#endif
