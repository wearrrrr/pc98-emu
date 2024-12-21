#include <cstdint>
#include <string.h>

#include "mem.h"
#include <stdio.h>

Mem::Mem()
{
    memset(memory, 0, sizeof(memory));
}

void Mem::write(uint16_t address, uint16_t data)
{
    memory[address] = data;
}

uint16_t Mem::read(uint32_t address)
{
    address &= 0xFFFFF; // Ensure 20-bit addressing

    if (address >= 0x0E8000 && address < 0x100000) {
        printf("Reading BIOS at 0x%08X\n", address);
    }

    return memory[address];
}
