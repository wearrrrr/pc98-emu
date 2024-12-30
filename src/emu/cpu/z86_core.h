#pragma once

#ifndef Z86_CORE_H
#define Z86_CORE_H 1

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <type_traits>

struct PortDevice {
    // Receive data from CPU.
    // Returns true if this device handled the port
    virtual bool out(uint16_t port, uint32_t value) = 0;

    // Send data to CPU
    // Returns true if this device handled the port
    virtual bool in(uint32_t& value, uint16_t port) = 0;
};

enum CoreModel {
    z8086,
    z80186
};


#endif