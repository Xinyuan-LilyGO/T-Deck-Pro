# T-Deck Pro 语音 AI 示例

[English documentation](README.md)

`test_ai_chat` 是一个面向 T-Deck Pro 的独立语音和文字对话应用。示例整合了板载 PDM 麦克风、讯飞 RTASR-LLM 语音识别、DeepSeek Chat Completions、240 x 320 墨水屏显示，以及 A7682E 模块的文本转语音输出。

## 数据流程

```text
PDM 麦克风：5 秒、16 kHz、16 bit、单声道 PCM
    -> 通过安全 WebSocket 连接讯飞 RTASR-LLM
    -> DeepSeek 对话补全
    -> LVGL 在墨水屏上显示文本
    -> A7682E 通过 AT+CTTS 朗读
```

示例内置 Noto Sans SC 16 px GB2312 字库，用于显示中英文界面。USB 串口监视器会输出录音、Wi-Fi、NTP、讯飞、DeepSeek 和 A7682E 的状态信息，但不会输出凭据或完整的讯飞签名。

## 使用条件

- T-Deck Pro，板载 PDM 麦克风和 A7682E 模块可用。
- 已安装 PlatformIO 和 ESP32 Arduino framework。
- 一个 2.4 GHz Wi-Fi 网络。
- 一个 DeepSeek API 密钥。
- 一个已开通 RTASR-LLM 的讯飞应用，以及对应的 `AppID`、`APIKey` 和 `APISecret`。

A7682E 的音频通路需要连接电池。没有电池供电时，模块可能不响应 AT 指令或无法播放语音。

## 配置凭据

将本目录中的 `config_keys.h.example` 复制为 `config_keys.h`，然后填写私密配置：

| 宏 | 用途 |
| --- | --- |
| `WIFI_SSID` | 2.4 GHz Wi-Fi 名称。 |
| `WIFI_PASSWORD` | Wi-Fi 密码。 |
| `DEEPSEEK_API_KEY` | DeepSeek API 密钥，可在 [platform.deepseek.com](https://platform.deepseek.com) 创建。 |
| `DEEPSEEK_MODEL` | DeepSeek 模型名称，默认值为 `deepseek-chat`。 |
| `XFYUN_APP_ID` | 讯飞 RTASR-LLM 应用的 AppID。 |
| `XFYUN_API_KEY` | 讯飞 RTASR-LLM 的 `APIKey`，对应 `accessKeyId`。 |
| `XFYUN_API_SECRET` | 讯飞 RTASR-LLM 的 `APISecret`，对应 `accessKeySecret`。 |

可在 [console.xfyun.cn/services/new_rta](https://console.xfyun.cn/services/new_rta) 创建或管理讯飞应用。三个讯飞配置必须来自同一个 RTASR-LLM 应用。

`config_keys.h` 已被仓库忽略，不要提交该文件，也不要在 issue 中粘贴其内容。不创建该文件也可以编译示例，但运行时网络功能会提示缺少凭据。

## 编译和上传

请在仓库根目录运行 PlatformIO，并确认根目录 `platformio.ini` 中启用的 `src_dir` 指向本示例：

```ini
src_dir = examples/test_ai_chat
```

然后构建、上传，并以 115200 波特率打开串口监视器：

```bash
pio run -e T-Deck-Pro
pio run -e T-Deck-Pro -t upload
pio device monitor -b 115200
```

同一时间只能启用一个示例的 `src_dir`。T-Deck Pro 的自定义开发板定义和 PSRAM 编译选项已经由仓库配置提供。

## 操作按键

输入框为空时，`V` 和 `R` 才会执行特殊操作。

| 按键 | 操作 |
| --- | --- |
| `V` | 必要时连接 Wi-Fi，等待 NTP 时间同步，录音 5 秒，将录音发送给讯飞识别，再把识别文本发送给 DeepSeek，显示回答并自动朗读。 |
| `R` | 重新朗读上一条 DeepSeek 回答。 |
| `Enter` | 将输入框中的英文或符号文本发送给 DeepSeek，显示回答并自动朗读。 |
| `Backspace` | 删除输入框中的一个字符。 |

当前键盘映射提供小写英文字母、空格和符号。语音或文字请求执行期间，键盘输入会被忽略。

## 时间和日志

Wi-Fi 刚连接后，第一次语音请求最多可能等待 10 秒进行 NTP 时间同步。讯飞 RTASR-LLM 鉴权依赖正确的设备时间。识别和服务端错误会同时显示在墨水屏和串口监视器中。

串口监视器波特率为 115200。日志包括 Wi-Fi 和 NTP 状态、PDM 录音、讯飞 WebSocket/鉴权过程，以及 A7682E AT 指令结果。虽然日志会主动隐藏 API 密钥，但识别文本可能出现在日志中，请将串口输出视为敏感信息。

## 常见问题

- **Wi-Fi 无法连接：** 确认使用 2.4 GHz 网络，并检查 `WIFI_SSID` 和 `WIFI_PASSWORD`。
- **讯飞鉴权失败：** 等待 NTP 完成，并确认 `XFYUN_APP_ID`、`XFYUN_API_KEY`、`XFYUN_API_SECRET` 来自同一个 RTASR-LLM 应用；同时查看串口中的错误码，例如 `35010`、`35013`、`35014` 或 `35017`。
- **录音失败：** 检查板载 PDM 麦克风，并确认设备有足够的 PSRAM 可用。
- **无法朗读：** 连接电池，确认 A7682E 已上电，并查看串口监视器中的 AT 返回结果。
- **DeepSeek 请求失败：** 检查 `DEEPSEEK_API_KEY`、`DEEPSEEK_MODEL` 和 Wi-Fi 连接，并查看屏幕或串口显示的 HTTP 错误。
