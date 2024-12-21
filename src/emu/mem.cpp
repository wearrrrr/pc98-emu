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
    if (address >= 0xFFF00000) {
        // printf("Warning: Attempted to read from 0x%08X, repeating memory from 0x%08X\n", address, address & 0x000FFFFF);
        address &= 0x000FFFFF;
    }

    return memory[address];
}
