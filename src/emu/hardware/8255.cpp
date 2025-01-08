#include <stdio.h>

#include "8255.h"

HW_8255::HW_8255() {
    printf("8255 initialized\n");
}

bool HW_8255::out_byte(uint32_t port, uint8_t value) {
    printf("data out to port %04X: %04X\n", port, value);
    return false;
}

bool HW_8255::in_byte(uint8_t& value, uint32_t port) {
    printf("data in from port %04X: %04X\n", port, value);
    return true;
}