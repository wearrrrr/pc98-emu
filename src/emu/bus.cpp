#include <cstdint>

#include "bus.h"

void Bus::cpuWrite(uint16_t addr, uint8_t data) {
    memory.write(addr, data);
};

uint16_t Bus::cpuRead(uint16_t addr, bool readOnly) {
    return memory.read(addr);
};