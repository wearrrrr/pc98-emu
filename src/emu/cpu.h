#include <cstdint>
#include <string>
#include <iomanip>

#include "bus.h"

class CPU {
    public:
        CPU();
        void load(uint8_t program[], uint16_t size);
        void clock();
        void reset();
        uint8_t* GetRegisterPointer_8Bit(uint8_t reg);
        uint16_t* GetRegisterPointer(uint8_t reg);
        uint16_t GetEffectiveAddress(uint8_t rm);
        std::string GetRegisterState();

        Bus bus;

    private:
        void Push(uint16_t value);
        uint16_t Pop();
        void UpdateFlags(uint32_t result, uint16_t finalValue);
        uint16_t* ResolveRM(uint8_t mod, uint8_t rm);

        union {
            uint16_t AX;
            struct {
                uint8_t AL;
                uint8_t AH;
            };
        };

        union {
            uint16_t BX;
            struct {
                uint8_t BL;
                uint8_t BH;
            };
        };

        union {
            uint16_t CX;
            struct {
                uint8_t CL;
                uint8_t CH;
            };
        };

        union {
            uint16_t DX;
            struct {
                uint8_t DL;
                uint8_t DH;
            };
        };

        bool HLT = false;

        uint16_t SI, DI, BP, SP;
        uint16_t DS, ES, SS, CS;
        // Instruction Pointer (Program Counter)
        uint16_t IP;

        // Flags
        bool carry_flag, parity_flag, aux_carry_flag, zero_flag, sign_flag, trap_flag, interrupt_flag, direction_flag, overflow_flag;
};