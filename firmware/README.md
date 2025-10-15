## :one: Firmware Description

|            Firmware                |          Remarks            |
| :--------------------------------: | :-------------------------: |
|   H693_factory_v1.2_20250412.bin   |      Factory firmware       |
| H693_factory_v1.2_20250412_fix.bin |        Fix firmware         |
|           test_EPD.bin             |        Display test         |
|        test_pcm5102a.bin           | Sound test（non-4G version） |

Fix firmware：To download the firmware using `flash_download_tools`, you need to first ERASE by clicking `ERASE`. After the download is complete, you need to plug and unplug the usb once.

Notice:

After the first download of Factory firmware or after the flash is checked, the spiffs.bin file system needs to be downloaded.

![alt text](image.png)

## :two: Download Instructions

More documentation about flash_download_tools: [link](https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html);


How to use the official software download program;

1、Download the `Flash Download Tools` , [Flash Download Tools](https://dl.espressif.com/public/flash_download_tool.zip);

2、Plug in USB. Enters download mode;
- :one: Hold down the BOOT key of the device without releasing it

- :two: Click the RST button on the back and release

- :three: Finally, release the BOOT key

3、Open the `Flash Download Tools` tool and select from the following figure;

![alt text](../docs/image.png)

4、Select the program you want to download and click `Start` key to download it as shown in the image below;

![alt text](../docs/image-2.png)

5、When the download is complete, click the `RST` button to restart the device;
