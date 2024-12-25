#include <stdio.h>

#include "8255.h"

HW_8255::HW_8255() {
    printf("8255 initialized\n");
}

bool HW_8255::in(uint16_t& value, uint16_t port) {
    printf("data in from port %04X: %04X\n", port, value);
    return true;
}

bool HW_8255::out(uint16_t port, uint16_t value) {
    printf("data out to port %04X: %04X\n", port, value);
    return false;
}