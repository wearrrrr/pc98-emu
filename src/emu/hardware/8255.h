#include "../cpu/8086_cpu.h"

class HW_8255 : public PortDevice {
    public:
        HW_8255();
        bool in(uint16_t& value, uint16_t port);
        bool out(uint16_t port, uint16_t value);
};