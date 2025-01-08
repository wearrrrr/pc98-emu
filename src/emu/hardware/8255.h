#include "../cpu/8086_cpu.h"

class HW_8255 : public PortByteDevice {
    public:
        HW_8255();
        bool out_byte(uint32_t port, uint8_t value);
        bool in_byte(uint8_t& value, uint32_t port);

        enum {
            port_a = 0x0,
            port_b = 0x1,
            port_c = 0x2 
        };
};