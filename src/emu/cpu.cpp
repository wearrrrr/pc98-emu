#include <cstdint>
#include <stdio.h>

#include "cpu.h"

uint8_t ModRM(uint8_t byte) {
    uint8_t mod = (byte & 0b11000000) >> 6;
    uint8_t reg = (byte & 0b00111000) >> 3;
    uint8_t rm = byte & 0b00000111;

    return mod;
}

CPU::CPU() {
    reset();
}

void CPU::load(uint8_t program[], uint16_t size) {
    for (uint16_t i = 0; i < size; i++) {
        bus.cpuWrite(IP + i, program[i]);
    }
}

void CPU::loadBIOS(const std::vector<uint8_t> &biosData) {
    uint32_t biosStartAddress = 0x0E8000; // System ROM range start
    size_t biosSize = biosData.size();

    if (biosSize > 0x18000) {
        biosSize = 0x18000;
    }

    for (size_t i = 0; i < biosSize; ++i) {
        bus.cpuWrite(biosStartAddress + i, biosData[i]);
    }

    printf("BIOS loaded into memory from 0x%08X to 0x%08X\n", biosStartAddress, biosStartAddress + biosSize - 1);
}

void CPU::Push(uint16_t value) {
    SP -= 2;
    bus.cpuWrite(SS * 16 + SP, value & 0xFF);
    bus.cpuWrite(SS * 16 + SP + 1, (value >> 8) & 0xFF);
}

uint16_t CPU::Pop() {
    uint16_t low = bus.cpuRead(SS * 16 + SP);
    uint16_t high = bus.cpuRead(SS * 16 + SP + 1);
    SP += 2;
    return (high << 8) | low;
}

void CPU::clock() {
    // Fetch instruction
    uint8_t opcode = bus.cpuRead(CS * 16 + IP);

    printf("Opcode: %x\n", opcode);

    // Decode instruction
    switch (opcode) {
        // ADD <reg>, <reg> (8-bit)
        case 0x00: {
            IP++;

            uint8_t modrm = bus.cpuRead(CS * 16 + IP);

            uint8_t mod = (modrm & 0b11000000) >> 6;
            uint8_t reg = (modrm & 0b00111000) >> 3;
            uint8_t rm = modrm & 0b00000111;

            uint8_t *reg_ptr = GetRegisterPointer_8Bit(reg);
            uint8_t *rm_ptr = GetRegisterPointer_8Bit(rm);

            if (reg_ptr == nullptr || rm_ptr == nullptr) {
                printf("Invalid ModR/M encoding\n");
                return;
            }

            uint16_t result = *rm_ptr + *reg_ptr;

            *rm_ptr = (uint8_t)result;

            UpdateFlags(result, *rm_ptr);

            break;
        }

        // ADD <rm>, <reg> (16-bit)
        case 0x01: {
            IP++;

            uint8_t modrm = bus.cpuRead(CS * 16 + IP);

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
        // ADD <reg>, <rm> (8-bit)
        case 0x02: {
            IP++;

            uint8_t modrm = bus.cpuRead(CS * 16 + IP);

            uint8_t mod = (modrm & 0b11000000) >> 6;
            uint8_t reg = (modrm & 0b00111000) >> 3;
            uint8_t rm = modrm & 0b00000111;

            uint8_t *reg_ptr = GetRegisterPointer_8Bit(reg);
            uint8_t *rm_ptr = GetRegisterPointer_8Bit(rm);

            if (reg_ptr == nullptr || rm_ptr == nullptr) {
                printf("Invalid ModR/M encoding\n");
                return;
            }

            uint16_t result = *reg_ptr + *rm_ptr;

            *reg_ptr = (uint8_t)result;

            UpdateFlags(result, *reg_ptr);

            break;
        }
        // ADD <reg>, <rm> (16-bit)
        case 0x03: {
            IP++;

            uint8_t modrm = bus.cpuRead(CS * 16 + IP);

            uint8_t mod = (modrm & 0b11000000) >> 6;
            uint8_t reg = (modrm & 0b00111000) >> 3;
            uint8_t rm = modrm & 0b00000111;

            uint16_t *reg_ptr = GetRegisterPointer(reg);
            uint16_t *rm_ptr = ResolveRM(mod, rm);

            if (reg_ptr == nullptr || rm_ptr == nullptr) {
                printf("Invalid ModR/M encoding\n");
                return;
            }

            uint32_t result = *reg_ptr + *rm_ptr;

            *reg_ptr = (uint16_t)result;

            UpdateFlags(result, *reg_ptr);

            break;
        }
        // ADD AL, kk
        case 0x04: {
            IP++;
            AL += bus.cpuRead(CS * 16 + IP);
            break;
        }
        // ADD AX, jjkk
        case 0x05: {
            IP++;
            uint8_t lo = bus.cpuRead(CS * 16 + IP);
            uint8_t hi = bus.cpuRead(CS * 16 + IP + 1);
            AX += lo | (hi << 8);
            IP++;
            break;
        }
        case 0x06: {
            Push(ES);
            break;
        }
        case 0x07: {
            ES = Pop();
            break;
        }
        // OR rm, reg (8-bit)
        case 0x08: {
            IP++;

            uint8_t modrm = bus.cpuRead(CS * 16 + IP);

            uint8_t mod = (modrm & 0b11000000) >> 6;
            uint8_t reg = (modrm & 0b00111000) >> 3;
            uint8_t rm = modrm & 0b00000111;

            uint8_t *reg_ptr = GetRegisterPointer_8Bit(reg);
            uint8_t *rm_ptr = GetRegisterPointer_8Bit(rm);

            if (reg_ptr == nullptr || rm_ptr == nullptr) {
                printf("Invalid ModR/M encoding\n");
                return;
            }

            uint16_t result = *rm_ptr | *reg_ptr;
            *rm_ptr = (uint8_t)result;
            UpdateFlags(result, *rm_ptr);

            break;
        }

        // INC, DEC, and PUSH/POP instructions.
        case 0x40: {
            AX++;
            break;
        }
        case 0x41: {
            CX++;
            break;
        }
        case 0x42: {
            DX++;
            break;
        }
        case 0x43: {
            BX++;
            break;
        }
        case 0x44: {
            SP++;
            break;
        }
        case 0x45: {
            BP++;
            break;
        }
        case 0x46: {
            SI++;
            break;
        }
        case 0x47: {
            DI++;
            break;
        }
        case 0x48: {
            AX--;
            break;
        }
        case 0x49: {
            CX--;
            break;
        }
        case 0x4A: {
            DX--;
            break;
        }
        case 0x4B: {
            BX--;
            break;
        }
        case 0x4C: {
            SP--;
            break;
        }
        case 0x4D: {
            BP--;
            break;
        }
        case 0x4E: {
            SI--;
            break;
        }
        case 0x4F: {
            DI--;
            break;
        }
        case 0x50: {
            Push(AX);
            break;
        }
        case 0x51: {
            Push(CX);
            break;
        }
        case 0x52: {
            Push(DX);
            break;
        }
        case 0x53: {
            Push(BX);
            break;
        }
        case 0x54: {
            Push(SP);
            break;
        }
        case 0x55: {
            Push(BP);
            break;
        }
        case 0x56: {
            Push(SI);
            break;
        }
        case 0x57: {
            Push(DI);
            break;
        }
        case 0x58: {
            AX = Pop();
            break;
        }
        case 0x59: {
            CX = Pop();
            break;
        }
        case 0x5A: {
            DX = Pop();
            break;
        }
        case 0x5B: {
            BX = Pop();
            break;
        }
        case 0x5C: {
            SP = Pop();
            break;
        }
        case 0x5D: {
            BP = Pop();
            break;
        }
        case 0x5E: {
            SI = Pop();
            break;
        }
        case 0x5F: {
            DI = Pop();
            break;
        }
        // JNE disp
        case 0x75: {
            IP++;
            int8_t disp = static_cast<int8_t>(bus.cpuRead(CS * 16 + IP));
            if (!zero_flag) {
                IP += disp;
            }
            break;
        }
        // MOV reg, rm (16-bit)
        case 0x89: {
            IP++;

            uint8_t modrm = bus.cpuRead(CS * 16 + IP);

            uint8_t mod = (modrm & 0b11000000) >> 6;
            uint8_t reg = (modrm & 0b00111000) >> 3;
            uint8_t rm = modrm & 0b00000111;

            uint16_t *reg_ptr = GetRegisterPointer(reg);
            uint16_t *rm_ptr = ResolveRM(mod, rm);

            if (reg_ptr == nullptr || rm_ptr == nullptr) {
                printf("Invalid ModR/M encoding\n");
                return;
            }

            *rm_ptr = *reg_ptr;

            break;
        }
        
        // MOV reg, reg (16-bit)
        case 0x8B: {
            IP++;

            uint8_t modrm = bus.cpuRead(CS * 16 + IP);

            uint8_t mod = (modrm & 0b11000000) >> 6;
            uint8_t reg = (modrm & 0b00111000) >> 3;
            uint8_t rm = modrm & 0b00000111;

            uint16_t *reg_ptr = GetRegisterPointer(reg);
            uint16_t *rm_ptr = ResolveRM(mod, rm);

            if (reg_ptr == nullptr || rm_ptr == nullptr) {
                printf("Invalid ModR/M encoding\n");
                return;
            }

            *reg_ptr = *rm_ptr;

            break;
        }
        
        case 0x90:
            // NOP
            break;

        // MOV AX, addr
        case 0xA1: {
            uint8_t lo = bus.cpuRead(CS * 16 + IP + 1);
            uint8_t hi = bus.cpuRead(CS * 16 + IP + 2);
            AX = lo | (hi << 8);
            IP += 2;
            break;
        }
        
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

            uint8_t imm8 = bus.cpuRead(CS * 16 + IP + 1);

            *reg_ptr = imm8;

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

            uint8_t imm_lo = bus.cpuRead(CS * 16 + IP + 1);
            uint8_t imm_hi = bus.cpuRead(CS * 16 + IP + 2);
            uint16_t imm16 = imm_lo | (imm_hi << 8);

            
            *reg_ptr = imm16;

            IP += 2;
            break;
        }

        // HLT
        case 0xF4:
            // Clear terminal
            // printf("\x1B[2J\x1B[H");
            printf("CPU Halted!\n");
            HLT = true;
            break;

        // TODO: How is this implemented?
        case 0xEF:
            // OUT DX, AL
            printf("OUT DX, AL\n");
            break;

        default:
            printf("Unknown opcode: %x! Halting...\n", opcode);
            // Dump bytes around the current instruction
            std::ostringstream oss;
            oss << std::hex << std::uppercase;
            oss << "Bytes around the current instruction:\n";

            for (int i = 0; i < 8; i++) {
                oss << std::setw(2) << std::setfill('0') << bus.cpuRead(CS * 16 + IP - 3 + i) << " ";
            }

            printf("%s\n", oss.str().c_str());

            HLT = true;
            break;
    }
    IP++;
    return;
}

void CPU::reset() {

    AX = 0;
    BX = 0;
    CX = 0;
    DX = 0;

    SI = 0x0000;
    DI = 0x0000;
    BP = 0x0000;
    SP = 0xFFFE;

    DS = 0;
    ES = 0;
    SS = 0;
    CS = 0xF000;

    IP = 0x8000;

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

void CPU::UpdateFlags(uint32_t result, uint16_t finalValue) {
    carry_flag = result > 0xFFFF;
    zero_flag = finalValue == 0;
    sign_flag = finalValue & 0x8000;
    overflow_flag = ((result ^ finalValue) & 0x8000) != 0;
}

// TODO: Test this function to make sure it works
uint16_t* CPU::ResolveRM(uint8_t mod, uint8_t rm) {
    uint16_t effective_address = 0;

    switch (mod) {
        case 0b00: // Memory addressing, no displacement
            if (rm == 0b110) {
                // Direct addressing: Use the next word as the effective address
                effective_address = bus.cpuRead(CS * 16 + IP + 1) |
                                    (bus.cpuRead(CS * 16 + IP + 2) << 8);
                IP += 2;
            } else {
                // Based on [BX+SI], [BX+DI], etc.
                effective_address = GetEffectiveAddress(rm);
            }
            break;

        case 0b01: { // Memory addressing with 8-bit displacement
            int8_t disp8 = static_cast<int8_t>(bus.cpuRead(CS * 16 + IP + 1));
            effective_address = GetEffectiveAddress(rm) + disp8;
            IP += 1; // Move IP past the displacement
            break;
        }

        case 0b10: { // Memory addressing with 16-bit displacement
            uint16_t disp16 = bus.cpuRead(CS * 16 + IP + 1) |
                              (bus.cpuRead(CS * 16 + IP + 2) << 8);
            effective_address = GetEffectiveAddress(rm) + disp16;
            IP += 2;
            break;
        }

        case 0b11: // Register addressing
            return GetRegisterPointer(rm);

        default:
            printf("Invalid addressing mode\n");
            return nullptr;
    }

    
    uint16_t read_value = bus.cpuRead(effective_address);
    uint16_t *read = &read_value;
    return read;
}


std::string CPU::GetRegisterState() {
    printf("\033[1;32mDumped Register state:\033[0m\n");

    std::ostringstream oss;

    oss << std::hex << std::uppercase;

    // Registers
    oss << "\033[1;36m"
        << "AX: " << std::setw(4) << std::setfill('0') << AX << "\t"
        << "BX: " << std::setw(4) << std::setfill('0') << BX << "\n"
        << "CX: " << std::setw(4) << std::setfill('0') << CX << "\t"
        << "DX: " << std::setw(4) << std::setfill('0') << DX << "\n"
        << "SI: " << std::setw(4) << std::setfill('0') << SI << "\t"
        << "DI: " << std::setw(4) << std::setfill('0') << DI << "\n"
        << "BP: " << std::setw(4) << std::setfill('0') << BP << "\t"
        << "SP: " << std::setw(4) << std::setfill('0') << SP << "\n"
        << "DS: " << std::setw(4) << std::setfill('0') << DS << "\t"
        << "ES: " << std::setw(4) << std::setfill('0') << ES << "\n"
        << "SS: " << std::setw(4) << std::setfill('0') << SS << "\t"
        << "CS: " << std::setw(4) << std::setfill('0') << CS << "\n"
        << "IP: " << std::setw(4) << std::setfill('0') << IP << "\n"

        << "\033[1;33m"
        // Flags
        << "Carry flag: " << carry_flag << "\t\t"
        << "Parity flag: " << parity_flag << "\n"
        << "Aux Carry flag: " << aux_carry_flag << "\t"
        << "Zero flag: " << zero_flag << "\n"
        << "Sign flag: " << sign_flag << "\t\t"
        << "Trap flag: " << trap_flag << "\n"
        << "Interrupt flag: " << interrupt_flag << "\t"
        << "Direction flag: " << direction_flag << "\n"
        << "Overflow flag: " << overflow_flag << "\t"
        << "[0m";

    return oss.str();
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

uint16_t CPU::GetEffectiveAddress(uint8_t rm) {
    switch (rm) {
        case 0b000: return BX + SI; // [BX+SI]
        case 0b001: return BX + DI; // [BX+DI]
        case 0b010: return BP + SI; // [BP+SI]
        case 0b011: return BP + DI; // [BP+DI]
        case 0b100: return SI;      // [SI]
        case 0b101: return DI;      // [DI]
        case 0b110: return BP;      // [BP] (or direct address for mod=00)
        case 0b111: return BX;      // [BX]
        default: return 0;
    }
}
