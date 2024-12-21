#include <cstdint>

#include "cpu.h"
#include <stdio.h>

uint8_t ModRM(uint8_t byte) {
    uint8_t mod = (byte & 0b11000000) >> 6;
    uint8_t reg = (byte & 0b00111000) >> 3;
    uint8_t rm = byte & 0b00000111;

    return mod;
}

CPU::CPU() : memory(memory) {
    reset();
}

void CPU::load(uint8_t program[], uint16_t size) {
    for (uint16_t i = 0; i < size; i++) {
        memory.write(IP + i, program[i]);
    }
}

void CPU::clock() {
    if (HLT) {
        // TODO: Send signal that makes sure the cpu is stopped and the program is done
        return;
    }
    // Fetch instruction
    uint8_t opcode = memory.read(CS * 16 + IP);

    // Decode instruction
    switch (opcode) {
        // MOV reg, imm8
        case 0xB0:
        case 0xB1:
        case 0xB2:
        case 0xB3:
        case 0xB4:
        case 0xB5:
        case 0xB6:
        case 0xB7: {
            uint8_t reg = opcode - 0xB0;
            uint8_t *reg_ptr = GetRegisterPointer_8Bit(reg);

            if (reg_ptr == nullptr) {
                printf("Invalid 8-bit register\n");
                return;
            }

            uint8_t imm8 = memory.read(CS * 16 + IP + 1);

            *reg_ptr = imm8;

            // Print value of reg_ptr
            printf("reg #%u: %x\n", reg, *reg_ptr);

            IP++;

            break;
        }
        // MOV reg, imm16
        case 0xB8:
        case 0xB9:
        case 0xBA:
        case 0xBB:
        case 0xBC:
        case 0xBD:
        case 0xBE:
        case 0xBF: {
            uint8_t reg = opcode - 0xB8;
            uint16_t *reg_ptr = GetRegisterPointer(reg);

            if (reg_ptr == nullptr) {
                printf("Invalid register\n");
                return;
            }

            uint8_t imm_lo = memory.read(CS * 16 + IP + 1);
            uint8_t imm_hi = memory.read(CS * 16 + IP + 2);
            uint16_t imm16 = imm_lo | (imm_hi << 8);

            
            *reg_ptr = imm16;

            // Print value of reg_ptr
            printf("reg #%u: %x\n", reg, *reg_ptr);

            IP += 2;
            break;
        }
        // add <rm>, <reg>
        case 0x01: {
            IP++;

            uint8_t modrm = memory.read(CS * 16 + IP);

            uint8_t mod = (modrm & 0b11000000) >> 6;
            uint8_t reg = (modrm & 0b00111000) >> 3;
            uint8_t rm = modrm & 0b00000111;

            uint16_t *reg_ptr = GetRegisterPointer(reg);
            uint16_t *rm_ptr = ResolveRM(mod, rm);

            if (reg_ptr == nullptr || rm_ptr == nullptr) {
                printf("Invalid ModR/M encoding\n");
                return;
            }

            uint32_t result = *rm_ptr + *reg_ptr;

            *rm_ptr = (uint16_t)result;

            UpdateFlags(result, *rm_ptr);

            break;
        }

        case 0x90:
            // NOP
            break;
        case 0xF4:
            // HLT
            printf("HLT!\n");
            HLT = true;
            break;
        default:
            printf("Unknown opcode: %x\n", opcode);
            break;
    }
    IP++;
    return;
}

void CPU::reset() {

    AX = 10;
    BX = 0;
    CX = 0;
    DX = 0;

    SI = 0;
    DI = 0;
    BP = 0;
    SP = 0;

    DS = 0;
    ES = 0;
    SS = 0;
    CS = 0;

    IP = 0;

    carry_flag = false;
    parity_flag = false;
    aux_carry_flag = false;
    zero_flag = false;
    sign_flag = false;
    trap_flag = false;
    interrupt_flag = false;
    direction_flag = false;
    overflow_flag = false;
}

uint16_t CPU::read(uint16_t address) {
    return memory.read(address);
}

void CPU::UpdateFlags(uint32_t result, uint16_t finalValue) {
    carry_flag = result > 0xFFFF;
    zero_flag = finalValue == 0;
    sign_flag = finalValue & 0x8000;
    overflow_flag = ((result ^ finalValue) & 0x8000) != 0;
}

uint16_t* CPU::ResolveRM(uint8_t mod, uint8_t rm) {
    switch (mod) {
        case 0b11: // Register addressing
            return GetRegisterPointer(rm);
        case 0b00: // Memory addressing with no displacement
        case 0b01: // Memory addressing with 8-bit displacement
        case 0b10: // Memory addressing with 16-bit displacement
            printf("Memory addressing not implemented yet\n");
            return nullptr;
        default:
            return nullptr;
    }
}

uint8_t* CPU::GetRegisterPointer_8Bit(uint8_t reg) {
    switch (reg) {
        case 0: return &AL;
        case 1: return &CL;
        case 2: return &DL;
        case 3: return &BL;
        case 4: return &AH;
        case 5: return &CH;
        case 6: return &DH;
        case 7: return &BH;
        default: return nullptr;
    }
}

uint16_t* CPU::GetRegisterPointer(uint8_t reg) {
    switch (reg) {
        case 0: return &AX;
        case 1: return &CX;
        case 2: return &DX;
        case 3: return &BX;
        case 4: return &SP;
        case 5: return &BP;
        case 6: return &SI;
        case 7: return &DI;
        default: return nullptr;
    }
}
