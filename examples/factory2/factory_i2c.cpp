#include <Arduino.h>
#include <Wire.h>
#include <esp32-hal-i2c.h>

extern "C" {

esp_err_t factory_i2c_begin(uint8_t pin_sda, uint8_t pin_scl)
{
    if (i2cIsInit(0)) {
        return ESP_OK;
    }
    return Wire.begin(pin_sda, pin_scl, 200000) ? ESP_OK : ESP_FAIL;
}

int factory_i2c_write(uint8_t address, const uint8_t *buf, uint16_t len)
{
    if (len && !buf) return -1;

    Wire.beginTransmission(address);
    if (len && Wire.write(buf, len) != len) {
        Wire.endTransmission(true);
        return -1;
    }
    return Wire.endTransmission(true) == 0 ? 0 : -1;
}

int factory_i2c_read(uint8_t address, uint8_t *buf, uint16_t len)
{
    if (len && !buf) return -1;

    const size_t received = Wire.requestFrom((int)address, (int)len, (int)true);
    size_t copied = 0;
    while (Wire.available() && copied < len) {
        buf[copied++] = (uint8_t)Wire.read();
    }
    while (Wire.available()) Wire.read();
    return received == len && copied == len ? 0 : -1;
}

int factory_i2c_write_read(uint8_t address, const uint8_t *wbuf, uint16_t wlen,
                           uint8_t *rbuf, uint16_t rlen)
{
    if ((wlen && !wbuf) || (rlen && !rbuf)) return -1;

    Wire.beginTransmission(address);
    if (wlen && Wire.write(wbuf, wlen) != wlen) {
        Wire.endTransmission(true);
        return -1;
    }
    if (Wire.endTransmission(false) != 0) return -1;

    const size_t received = Wire.requestFrom((int)address, (int)rlen, (int)true);
    size_t copied = 0;
    while (Wire.available() && copied < rlen) {
        rbuf[copied++] = (uint8_t)Wire.read();
    }
    while (Wire.available()) Wire.read();
    return received == rlen && copied == rlen ? 0 : -1;
}

}
