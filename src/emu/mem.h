#include <cstdint>

class Mem {
    public:
        Mem();
        void write(uint16_t address, uint16_t data);
        uint16_t read(uint32_t address);

    private:
        uint16_t memory[0x0FFFFF];
};