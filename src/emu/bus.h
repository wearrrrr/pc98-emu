#include <cstdint>

#include "mem.h"

class Bus {
    public:
        void cpuWrite(uint16_t addr, uint8_t data);
        uint16_t cpuRead(uint16_t addr, bool readOnly = false);

    private:
        Mem memory;
};