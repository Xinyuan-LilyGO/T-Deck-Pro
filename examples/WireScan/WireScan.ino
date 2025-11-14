#include "Wire.h"
#include <SensorWireHelper.h>

// IIC
#define BOARD_I2C_SDA 13
#define BOARD_I2C_SCL 14

#define BOARD_I2C_ADDR_ES8311 0x18
#define BOARD_I2C_ADDR_TOUCH 0x1A
#define BOARD_I2C_ADDR_XL9555 0x20
#define BOARD_I2C_ADDR_GYROSCOPDE 0x28
#define BOARD_I2C_ADDR_KEYBOARD 0x34
#define BOARD_I2C_ADDR_BQ27220 0x55
#define BOARD_I2C_ADDR_MOTOR 0x5A
#define BOARD_I2C_ADDR_BQ25896 0x6B

int dev_mark = 0;

void setup()
{
	Serial.begin(115200);
	Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
}

void loop()
{
	byte error, address;
	int nDevices = 0;

	delay(2000);

	Serial.printf(" -------------------------- I2C -------------------------- \n");
	SensorWireHelper::dumpDevices(Wire, Serial);

	for (address = 0x01; address < 0x7f; address++)
	{
		Wire.beginTransmission(address);
		error = Wire.endTransmission();
		if (error == 0)
		{
			// Serial.printf("I2C device found at address 0x%02X\n", address);
			nDevices++;
			if ((address == BOARD_I2C_ADDR_MOTOR) || (address == BOARD_I2C_ADDR_XL9555))
			{
				dev_mark++;
			}
		}
		else if (error != 2)
		{
			Serial.printf("Error %d at address 0x%02X\n", error, address);
		}
	}
	if (nDevices == 0)
	{
		Serial.println("No I2C devices found");
	}

	switch (dev_mark)
	{
	case 0: Serial.printf("\nThis is: T-Deck-Pro V1.0\n\n"); break;
	case 1: Serial.printf("\nThis is: T-Deck-Pro V1.1\n\n"); break;
	case 2: Serial.printf("\nThis is: T-Deck-Pro Max V0.1\n\n"); break;
	default:
		break;
	}

	dev_mark = 0;
	Serial.printf("Detected %d I2C devices\n\n", nDevices);
}
