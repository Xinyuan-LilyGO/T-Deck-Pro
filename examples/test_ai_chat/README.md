# T-Deck Pro Voice AI Example

[中文文档](README_zh.md)

`test_ai_chat` is a standalone voice and text chat application for the T-Deck Pro. It combines the onboard PDM microphone, iFlytek RTASR-LLM speech recognition, DeepSeek Chat Completions, the 240 x 320 e-paper display, and the A7682E modem's text-to-speech output.

## Data Flow

```text
PDM microphone: 5 seconds, 16 kHz, 16-bit, mono PCM
    -> iFlytek RTASR-LLM over a secure WebSocket
    -> DeepSeek chat completion
    -> LVGL text on the e-paper display
    -> A7682E AT+CTTS speech output
```

The example includes a Noto Sans SC 16 px GB2312 font for the Chinese and English UI. The USB serial monitor reports recording, Wi-Fi, NTP, iFlytek, DeepSeek, and A7682E status information. Credentials and the complete iFlytek signature are not printed.

## Requirements

- T-Deck Pro with its onboard PDM microphone and A7682E modem.
- PlatformIO with the ESP32 Arduino framework.
- A 2.4 GHz Wi-Fi network.
- A DeepSeek API key.
- An iFlytek RTASR-LLM application with its `AppID`, `APIKey`, and `APISecret`.

The A7682E audio path requires the board battery to be connected. Without battery power, the modem may not respond to AT commands or play speech.

## Configure Credentials

Copy `config_keys.h.example` to `config_keys.h` in this directory, then fill in the private values:

| Define | Purpose |
| --- | --- |
| `WIFI_SSID` | 2.4 GHz Wi-Fi network name. |
| `WIFI_PASSWORD` | Wi-Fi password. |
| `DEEPSEEK_API_KEY` | DeepSeek API key. Create one at [platform.deepseek.com](https://platform.deepseek.com). |
| `DEEPSEEK_MODEL` | DeepSeek model name. The default is `deepseek-chat`. |
| `XFYUN_APP_ID` | AppID of the iFlytek RTASR-LLM application. |
| `XFYUN_API_KEY` | iFlytek RTASR-LLM `APIKey`, used as `accessKeyId`. |
| `XFYUN_API_SECRET` | iFlytek RTASR-LLM `APISecret`, used as `accessKeySecret`. |

Create or manage the iFlytek application at [console.xfyun.cn/services/new_rta](https://console.xfyun.cn/services/new_rta). The three iFlytek values must belong to the same RTASR-LLM application. They are not the credentials for the Spark IAT service.

`config_keys.h` is ignored by the repository. Do not commit it or paste its contents into issue reports. The example can still compile without this file, but network features will report missing credentials at runtime.

## Build and Upload

Run PlatformIO from the repository root. Make sure the active `src_dir` in the root `platformio.ini` points to this example:

```ini
src_dir = examples/test_ai_chat
```

Then build, upload, and open the serial monitor at 115200 baud:

```bash
pio run -e T-Deck-Pro
pio run -e T-Deck-Pro -t upload
pio device monitor -b 115200
```

Only one example `src_dir` should be active at a time. The custom T-Deck Pro board definition and the PSRAM build flag are already provided by the repository configuration.

## Controls

The input field must be empty for `V` and `R` to trigger their special actions.

| Key | Action |
| --- | --- |
| `V` | Connect Wi-Fi if needed, wait for NTP time, record 5 seconds of audio, send it to iFlytek, send the recognized text to DeepSeek, display the answer, and read it aloud. |
| `R` | Read the last DeepSeek answer aloud again. |
| `Enter` | Send the typed English or symbol input to DeepSeek, display the answer, and read it aloud. |
| `Backspace` | Delete one character from the input field. |

The keyboard map currently provides lowercase English letters, spaces, and symbols. Keyboard input is ignored while a voice or text request is running.

## Timing and Logs

The first voice request after Wi-Fi connects may wait up to 10 seconds for NTP synchronization. iFlytek RTASR-LLM authentication depends on the device clock being correct. Recognition and server errors are shown on the display and in the serial monitor.

The serial monitor uses 115200 baud. Logs include Wi-Fi and NTP state, PDM recording, iFlytek WebSocket/authentication progress, and A7682E AT command results. Recognition text may appear in logs, so treat serial output as sensitive even though API keys are intentionally omitted.

## Troubleshooting

- **Wi-Fi does not connect:** confirm that the network is 2.4 GHz and check `WIFI_SSID` and `WIFI_PASSWORD`.
- **iFlytek authentication fails:** wait for NTP to complete, then verify that `XFYUN_APP_ID`, `XFYUN_API_KEY`, and `XFYUN_API_SECRET` come from the same RTASR-LLM application. Check the serial error code, such as `35010`, `35013`, `35014`, or `35017`.
- **Recording fails:** check the onboard PDM microphone and make sure the board has enough PSRAM available.
- **Speech output fails:** connect the battery, confirm that the A7682E modem is powered, and inspect the AT command responses in the serial monitor.
- **DeepSeek requests fail:** verify `DEEPSEEK_API_KEY`, `DEEPSEEK_MODEL`, Wi-Fi connectivity, and the HTTP error shown on the display or serial monitor.
