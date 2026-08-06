/**
 * @file      a7682_tts.cpp
 * @brief     A7682E AT+CTTS implementation used by pda2 Voice AI.
 *
 * A7682E owns the audio path, so this module does not use the PCM5102A/I2S
 * Audio library. Chinese text is sent as UCS2 hexadecimal in CTTS mode 1;
 * ASCII-only text uses CTTS mode 2.
 */
#include "a7682_tts.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "utilities.h"

extern TaskHandle_t a7682_handle;

namespace {

constexpr size_t kTtsMaxPayloadBytes = 160;
constexpr size_t kTtsCommandCapacity = 768;
constexpr uint32_t kCommandTimeoutMs = 5000;
constexpr uint32_t kTtsBaseDurationMs = 1200;
constexpr uint32_t kTtsMsPerCharacter = 280;
constexpr uint32_t kTtsMaxDurationMs = 60000;

bool g_initialized = false;
bool g_active = false;
uint32_t g_active_until = 0;

void drain_modem_input()
{
    while (SerialAT.available() > 0) {
        SerialAT.read();
    }
}

bool suspend_serial_bridge()
{
    if (a7682_handle == nullptr) {
        return false;
    }

    const eTaskState state = eTaskGetState(a7682_handle);
    if (state == eSuspended) {
        return false;
    }

    if (state == eRunning || state == eReady || state == eBlocked) {
        vTaskSuspend(a7682_handle);
        return true;
    }
    return false;
}

void restore_serial_bridge(bool resume)
{
    if (resume && a7682_handle != nullptr) {
        vTaskResume(a7682_handle);
    }
}

bool send_at_command(const char *command, uint32_t timeout_ms)
{
    if (command == nullptr || command[0] == '\0') {
        return false;
    }

    const bool resume_bridge = suspend_serial_bridge();
    drain_modem_input();

    Serial.printf("[A7682-TTS] AT: %s\n", command);
    SerialAT.print(command);
    SerialAT.print("\r\n");

    String response;
    response.reserve(256);
    bool accepted = false;
    bool rejected = false;
    const uint32_t started = millis();

    while (millis() - started < timeout_ms) {
        while (SerialAT.available() > 0) {
            const char value = static_cast<char>(SerialAT.read());
            if (response.length() < 512) {
                response += value;
            }
            if (response.indexOf("ERROR") >= 0) {
                rejected = true;
            }
            if (response.indexOf("OK") >= 0) {
                accepted = true;
            }
        }

        if (accepted || rejected) {
            break;
        }
        delay(1);
    }

    restore_serial_bridge(resume_bridge);
    Serial.printf("[A7682-TTS] AT result: %s\n",
                  accepted && !rejected ? "OK" : "ERROR/TIMEOUT");
    return accepted && !rejected;
}

size_t utf8_sequence_length(const String &source, size_t offset)
{
    if (offset >= source.length()) {
        return 0;
    }

    const uint8_t first = static_cast<uint8_t>(source[offset]);
    size_t length = 0;
    if (first <= 0x7F) {
        return 1;
    } else if (first >= 0xC2 && first <= 0xDF) {
        length = 2;
    } else if (first >= 0xE0 && first <= 0xEF) {
        length = 3;
    } else if (first >= 0xF0 && first <= 0xF4) {
        length = 4;
    } else {
        return 0;
    }

    if (offset + length > source.length()) {
        return 0;
    }
    for (size_t i = 1; i < length; ++i) {
        const uint8_t continuation = static_cast<uint8_t>(source[offset + i]);
        if (continuation < 0x80 || continuation > 0xBF) {
            return 0;
        }
    }
    return length;
}

bool is_ascii_space(uint8_t value)
{
    return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

bool is_tts_markup(uint8_t value)
{
    switch (value) {
    case '*':
    case '_':
    case '`':
    case '#':
    case '~':
    case '>':
    case '[':
    case ']':
    case '{':
    case '}':
    case '|':
        return true;
    default:
        return false;
    }
}

String sanitize_tts_text(const String &source)
{
    String result;
    result.reserve(source.length());
    bool pending_space = false;

    for (size_t offset = 0; offset < source.length();) {
        const uint8_t first = static_cast<uint8_t>(source[offset]);
        if (first <= 0x7F) {
            ++offset;
            if (is_ascii_space(first)) {
                pending_space = true;
                continue;
            }
            if (first < 0x20 || is_tts_markup(first)) {
                continue;
            }
            if (pending_space && result.length() > 0) {
                result += ' ';
            }
            pending_space = false;
            result += static_cast<char>(first);
            continue;
        }

        const size_t length = utf8_sequence_length(source, offset);
        if (length == 0) {
            ++offset;
            continue;
        }

        /* A7682E CTTS mode 1 accepts UCS2; drop four-byte emoji/symbols. */
        if (length == 4) {
            offset += length;
            continue;
        }

        if (pending_space && result.length() > 0) {
            result += ' ';
        }
        pending_space = false;
        for (size_t i = 0; i < length; ++i) {
            result += source[offset + i];
        }
        offset += length;
    }

    while (result.length() > 0 && result[result.length() - 1] == ' ') {
        result.remove(result.length() - 1);
    }
    return result;
}

String truncate_utf8(const String &source, size_t max_bytes)
{
    String result;
    result.reserve(max_bytes);

    size_t offset = 0;
    while (offset < source.length()) {
        const size_t length = utf8_sequence_length(source, offset);
        if (length == 0) {
            ++offset;
            continue;
        }
        if (result.length() + length > max_bytes) {
            break;
        }
        for (size_t i = 0; i < length; ++i) {
            result += source[offset + i];
        }
        offset += length;
    }
    return result;
}

char hex_digit(uint8_t value)
{
    return value < 10 ? static_cast<char>('0' + value)
                      : static_cast<char>('A' + value - 10);
}

bool utf8_to_ucs2_hex(const String &source, String &destination)
{
    destination.clear();
    destination.reserve(source.length() * 2 + 1);

    for (size_t offset = 0; offset < source.length();) {
        const size_t length = utf8_sequence_length(source, offset);
        if (length == 0 || length == 4) {
            return false;
        }

        const uint8_t first = static_cast<uint8_t>(source[offset]);
        uint32_t codepoint = 0;
        if (length == 1) {
            codepoint = first;
        } else if (length == 2) {
            codepoint = first & 0x1F;
            codepoint = (codepoint << 6) |
                        (static_cast<uint8_t>(source[offset + 1]) & 0x3F);
        } else {
            codepoint = first & 0x0F;
            codepoint = (codepoint << 6) |
                        (static_cast<uint8_t>(source[offset + 1]) & 0x3F);
            codepoint = (codepoint << 6) |
                        (static_cast<uint8_t>(source[offset + 2]) & 0x3F);
        }

        if (codepoint > 0xFFFF ||
            (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
            return false;
        }

        destination += hex_digit(static_cast<uint8_t>((codepoint >> 12) & 0x0F));
        destination += hex_digit(static_cast<uint8_t>((codepoint >> 8) & 0x0F));
        destination += hex_digit(static_cast<uint8_t>((codepoint >> 4) & 0x0F));
        destination += hex_digit(static_cast<uint8_t>(codepoint & 0x0F));
        offset += length;
    }
    return destination.length() > 0;
}

bool contains_non_ascii(const String &source)
{
    for (size_t i = 0; i < source.length(); ++i) {
        if (static_cast<uint8_t>(source[i]) >= 0x80) {
            return true;
        }
    }
    return false;
}

bool escape_ascii(const String &source, String &destination)
{
    destination.clear();
    destination.reserve(source.length() + 1);
    for (size_t i = 0; i < source.length(); ++i) {
        const char value = source[i];
        if (value == '\r' || value == '\n') {
            destination += ' ';
        } else if (value == '"' || value == '\\') {
            destination += '\\';
            destination += value;
        } else if (static_cast<uint8_t>(value) >= 0x20) {
            destination += value;
        }
    }
    return destination.length() > 0;
}

size_t utf8_character_count(const String &source)
{
    size_t count = 0;
    for (size_t offset = 0; offset < source.length();) {
        const size_t length = utf8_sequence_length(source, offset);
        if (length == 0) {
            ++offset;
            continue;
        }
        ++count;
        offset += length;
    }
    return count;
}

} // namespace

bool a7682_tts_init()
{
    if (g_initialized) {
        return true;
    }

    if (!send_at_command("AT", 1500)) {
        Serial.println("[A7682-TTS] modem is not ready");
        return false;
    }
    if (!send_at_command("AT+CTTSPARAM=1,3,0,1,1", 2500)) {
        Serial.println("[A7682-TTS] AT+CTTSPARAM failed");
        return false;
    }

    send_at_command("AT+VMUTE=0", 1500);
    send_at_command("AT+COUTGAIN=4", 1500);
    g_initialized = true;
    Serial.println("[A7682-TTS] CTTS ready");
    return true;
}

bool a7682_tts_speak(const char *text)
{
    if (text == nullptr || text[0] == '\0') {
        return false;
    }
    if (!a7682_tts_init()) {
        return false;
    }

    if (g_active) {
        a7682_tts_stop();
    }

    String sanitized = sanitize_tts_text(String(text));
    sanitized = truncate_utf8(sanitized, kTtsMaxPayloadBytes);
    if (sanitized.length() == 0) {
        Serial.println("[A7682-TTS] text is empty after sanitizing");
        return false;
    }

    const bool use_ucs2 = contains_non_ascii(sanitized);
    String payload;
    if (use_ucs2) {
        if (!utf8_to_ucs2_hex(sanitized, payload)) {
            Serial.println("[A7682-TTS] UTF-8 to UCS2 conversion failed");
            return false;
        }
    } else if (!escape_ascii(sanitized, payload)) {
        return false;
    }

    String command;
    command.reserve(payload.length() + 32);
    command = use_ucs2 ? "AT+CTTS=1,\"" : "AT+CTTS=2,\"";
    command += payload;
    command += "\"";
    if (command.length() >= kTtsCommandCapacity) {
        Serial.println("[A7682-TTS] AT command is too long");
        return false;
    }

    Serial.printf("[A7682-TTS] mode=%d, text bytes=%u\n",
                  use_ucs2 ? 1 : 2,
                  static_cast<unsigned>(sanitized.length()));
    if (!send_at_command(command.c_str(), kCommandTimeoutMs)) {
        Serial.println("[A7682-TTS] CTTS command rejected");
        return false;
    }

    const size_t characters = utf8_character_count(sanitized);
    uint32_t duration = kTtsBaseDurationMs +
                        static_cast<uint32_t>(characters) * kTtsMsPerCharacter;
    if (duration > kTtsMaxDurationMs) {
        duration = kTtsMaxDurationMs;
    }
    g_active_until = millis() + duration;
    g_active = true;
    return true;
}

void a7682_tts_stop()
{
    if (g_initialized) {
        send_at_command("AT+CTTS=0", 1500);
    }
    g_active = false;
    g_active_until = 0;
}

bool a7682_tts_is_active()
{
    if (g_active && static_cast<int32_t>(millis() - g_active_until) >= 0) {
        g_active = false;
    }
    return g_active;
}

#else

bool a7682_tts_init() { return false; }
bool a7682_tts_speak(const char *) { return false; }
void a7682_tts_stop() {}
bool a7682_tts_is_active() { return false; }

#endif
