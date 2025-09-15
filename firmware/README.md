## :one: Firmware Description

|            Firmware                |          Remarks            |
| :--------------------------------: | :-------------------------: |
|   H693_factory_v1.2_20250412.bin   |      Factory firmware       |
| H693_factory_v1.2_20250412_fix.bin |        Fix firmware         |
|           test_EPD.bin             |        Display test         |
|        test_pcm5102a.bin           | Sound test（non-4G version） |

Fix firmware：To download the firmware using `flash_download_tools`, you need to first ERASE by clicking `ERASE`. After the download is complete, you need to plug and unplug the usb once.

## :two: Download Instructions

More documentation about flash_download_tools: [link](https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html);


How to use the official software download program;

1、Download the `Flash Download Tools` , [Flash Download Tools](https://dl.espressif.com/public/flash_download_tool.zip);

2、Plug in USB. To enter DFU mode:
- :one: Locate the rocker button on the right side of the device. The top half is BOOT, the bottom half is RST.

- :two: Hold down the BOOT button without releasing it

- :three: Click the RST button on the back and release

- :four: Finally, release the BOOT button

3、Open the `Flash Download Tools` tool and select from the following figure;

![alt text](../docs/image.png)

4、Select the program you want to download and click `Start` key to download it as shown in the image below;

![alt text](../docs/image-2.png)

- :one: Click `...` and locate the *.bin firmware file you wish to load

- :two: Enter `0` in this box

- :three: Select 115200 BAUD and the correct COM port for your device
  - hint: if you are unsure, switch to the ChipInfoDump tab and click `Chip Info`. When you have the right port, you'll see something like the following:
```
start detect chip...please wait
chip sync ...
Chip is ESP32-S3 (QFN56) (revision v0.2)
Features: Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240MHz
Crystal is 40MHz
MAC: 2837ffffffff
Manufacturer: ef
Device: 6018
Status value: 0x600200
Detected flash size: 16MB
```

- :four: Finally, press the start button and wait patiently until the load and verification process is complete.

5、When the download is complete, click the `RST` button to restart the device;
