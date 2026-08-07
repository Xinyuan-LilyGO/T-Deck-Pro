#ifndef FACTORY_AUDIO_STUB_H
#define FACTORY_AUDIO_STUB_H

#include <stdint.h>
#include "FS.h"

/* Keep factory2 independent from the incompatible external Audio decoder. */
class FactoryAudio {
public:
    bool setPinout(uint8_t, uint8_t, uint8_t, int8_t = -1) { return false; }
    void setVolume(uint8_t) {}
    bool connecttoFS(fs::FS &, const char *, uint32_t = 0) { return false; }
    uint32_t stopSong() { return 0; }
    bool isRunning() const { return false; }
    void loop() {}
};

#endif
