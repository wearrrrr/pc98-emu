#pragma once

#ifndef Z86_CORE_INTERNAL_POST_H
#define Z86_CORE_INTERNAL_POST_H 1

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <limits>
#include <algorithm>
#include <utility>
#include <type_traits>

#include "../zero/util.h"

#if __INTELLISENSE__
// This just helps when intellisense derps out sometimes
#if !Z86_CORE_INTERNAL_PRE_H
#include "z86_core_internal_pre.h"
#endif
#endif

template <size_t max_bits>
struct z86AddrESImpl<max_bits, true> {
    using type = z86AddrFixedImpl<max_bits, ES>;
};
template <size_t max_bits>
struct z86AddrCSImpl<max_bits, true> {
    using type = z86AddrFixedImpl<max_bits, CS>;
};
template <size_t max_bits>
struct z86AddrSSImpl<max_bits, true> {
    using type = z86AddrFixedImpl<max_bits, SS>;
};

using z86Addr = z86AddrImpl<ctx.max_bits, ctx.PROTECTED_MODE>;

using z86AddrES = z86AddrESImpl<ctx.max_bits, ctx.PROTECTED_MODE>::type;
using z86AddrCS = z86AddrCSImpl<ctx.max_bits, ctx.PROTECTED_MODE>::type;
using z86AddrSS = z86AddrSSImpl<ctx.max_bits, ctx.PROTECTED_MODE>::type;

template <typename P>
uint32_t ModRM::extra_length(const P& pc) const {
    uint8_t mod = this->Mod();
    if (mod == 3) {
        return 0;
    }
    if constexpr (ctx.max_bits > 16) {
        if (!ctx.addr_size_16()) {
            uint32_t length = 0;
            switch (this->M()) {
                case 4:
                    ++length;
                    if (mod == 0) {
                        return length + (pc.read<SIB>(1).B() == 5) * 4;
                    }
                    break;
                case 5:
                    if (mod == 0) {
                        return 4;
                    }
            }
            switch (mod) {
                case 0:
                    return length;
                case 1:
                    return length + 1;
                case 2:
                    return length + 4;
                default:
                    unreachable;
            }
        }
    }
    switch (mod) {
        case 0:
            return (this->M() == 6) * 2;
        case 1:
            return 1;
        case 2:
            return 2;
        default:
            unreachable;
    }
}

template <typename P>
auto ModRM::parse_memM(P& pc) const {
    uint32_t segment_mask;
    size_t offset = 0;
    uint8_t m = this->M();
    assume(m < 8);
    uint8_t mod = this->Mod();
    if constexpr (ctx.max_bits > 16) {
        if (!ctx.addr_size_16()) {
            if constexpr (ctx.max_bits == 64) {
                if (ctx.addr_size_64()) {
                    switch (m) {
                        default: unreachable;
                        case RSP: {
                            SIB sib = pc.read_advance<SIB>();
                            uint8_t i = sib.I();
                            if (i != RSP) {
                                offset = ctx.index_qword_regI(i) * (1 << sib.S());
                            }
                            m = sib.B();
                            if (m == RBP && mod == 0) {
                                // Not RIP relative
                                // Label overrides segment to DS
                                goto add_const32;
                            }
                            break;
                        }
                        case RBP:
                            if (mod == 0) {
                                // TODO: Properly offset for
                                // the end of the instruction
                                offset = ctx.rip;
                                goto add_const32;
                            }
                        case RAX: case RCX: case RDX: case RBX: case RSI: case RDI:
                            break;
                    }
                    // Initialize the full M so that
                    // later code can bit test for 
                    // default segment
                    m = ctx.full_indexMB(m);
                    offset += ctx.index_qword_reg_raw(m);
                }
                goto add_const;
            }
            switch (m) {
                default: unreachable;
                case ESP: {
                    SIB sib = pc.read_advance<SIB>();
                    uint8_t i = sib.I();
                    if (i != ESP) {
                        offset = ctx.index_dword_regI(i) * (1 << sib.S());
                    }
                    m = sib.B();
                    if (m == EBP && mod == 0) {
                        // Not EIP relative
                        // Label overrides segment to DS
                        goto add_const32;
                    }
                    break;
                }
                case EBP:
                    if (mod == 0) {
                        if constexpr (ctx.max_bits == 64) {
                            if (expect(ctx.is_long_mode(), false)) {
                                // TODO: Properly offset for
                                // the end of the instruction
                                offset = ctx.eip;
                            }
                        }
                        goto add_const32;
                    }
                case EAX: case ECX: case EDX: case EBX: case ESI: case EDI:
                    break;
            }
            // Initialize the full M so that
            // later code can bit test for 
            // default segment
            m = ctx.full_indexMB(m);
            offset += ctx.index_dword_reg_raw(m);
        add_const:
            switch (mod) {
                default: unreachable;
                case 1:
                    offset += pc.read_advance<int8_t>();
                    break;
                add_const32:
                    // Override the segment to always be DS
                    m = 0;
                case 2:
                    offset += pc.read_advance<int32_t>();
                case 0:;
            }
            // Set bits are for DS
            if constexpr (ctx.max_bits == 64) {
                segment_mask = 0b11111111111111111111111111001111;
            }
            else {
                segment_mask = 0b11001111;
            }
            goto ret;
        }
    }
    
    static constexpr uint32_t first_reg16[] = { BX, BX, BP, BP, SI, DI, BP, BX };
    offset = ctx.index_word_regMB<true>(first_reg16[m]);
    if (m < 4) {
        offset += ctx.index_word_regI<true>(SI | m);
    }
    switch (mod) {
        default: unreachable;
        case 1:
            // TODO:
            // Merge this with 32 bit byte offset somehow?
            offset += pc.read_advance<int8_t>();
            break;
        case 0:
            if (m != 6) {
                break;
            }
            m = 0;
            offset = 0;
        case 2:
            offset += pc.read_advance<int16_t>();
    }
    // Set bits are for DS
    segment_mask = 0b10110011;
ret:
    return ctx.addr(SS + (bool)(segment_mask & 1 << m), offset);
}

template <size_t max_bits>
inline constexpr SEG_DESCRIPTOR<max_bits>* z86DescriptorCache<max_bits>::load_selector(uint16_t selector) const {
    //uint8_t rpl = selector & 3;
    BT offset = selector & 0xFFF8;
    if (offset <= this->limit) {
        return mem.ptr<SEG_DESCRIPTOR<max_bits>>(offset + this->base);
    }
    return NULL;
}

template <size_t bits, bool protected_mode>
inline constexpr size_t z86AddrImpl<bits, protected_mode>::seg() const {
    if constexpr (protected_mode) {
        return ctx.descriptors[this->segment].base;
    }
    else {
        return (size_t)this->segment << 4;
    }
}

template <size_t max_bits, uint8_t descriptor_index>
inline constexpr size_t z86AddrFixedImpl<max_bits, descriptor_index>::seg() const {
    return ctx.descriptors[descriptor_index].base;
}

template <typename T>
inline constexpr bool z86AddrSharedFuncs::addr_fits_on_bus(size_t addr) {
    return (addr & align_mask<ctx.bus_bytes>) <= ctx.bus_bytes - z86DataProperites<T>::size;
}

template <typename T>
inline constexpr bool z86AddrSharedFuncs::addr_crosses_page(size_t addr) {
    if constexpr (ctx.PAGING) {
        // No paging simulated yet
    }
    return false;
}

inline constexpr size_t regcall z86AddrSharedFuncs::virt_to_phys(size_t addr) {
    // No paging yet
    return addr;
}

template <typename T, typename P>
inline void regcall z86AddrSharedFuncs::write(P* self, const T& value, ssize_t offset) {

    if constexpr (!ctx.SINGLE_MEM_WRAPS) {
        // TODO: Check segment limits
        return mem.write<T>(self->addr(offset), value);
    }
    else {
        // 8086 compatiblity
        size_t virt_seg_base = self->seg();
        size_t virt_addr_base = virt_seg_base + self->ptr(offset);
        size_t wrap = self->offset_wrap<T>(offset);
        if (expect(!wrap, true)) {
            mem.write<T>(virt_addr_base, value);
        }
        else {
            //mem.write(virt_addr_base, &value, wrap);
            //mem.write(virt_seg_base + self->ptr(offset), &((uint8_t*)&value)[wrap], z86DataProperites<T>::size - wrap);
            if constexpr (sizeof(T) != sizeof(uint16_t)) {
                mem.write_movsb(
                    virt_addr_base - self->offset_wrap_sub<T>(wrap),
                    mem.write_movsb(virt_addr_base, &value, wrap),
                    z86DataProperites<T>::size - wrap
                );
            }
            else {
                uint16_t raw = std::bit_cast<uint16_t>(value);
                mem.write<uint8_t>(virt_addr_base, raw);
                mem.write<uint8_t>(virt_addr_base - self->offset_wrap_sub<T>(wrap), raw >> 8);
            }
        }
    }

    /*
    if constexpr (sizeof(T) == sizeof(uint8_t)) {
        uint8_t raw = std::bit_cast<uint8_t>(value);
        return mem.write<uint8_t>(self->addr(offset), raw);
    }
    else if constexpr (sizeof(T) == sizeof(uint16_t)) {
        size_t virt_seg_base = this->seg();
        size_t virt_addr_base = virt_seg_base + self->ptr(offset);
        uint16_t raw = std::bit_cast<uint16_t>(value);
        if constexpr (ctx.bus_width >= 16) {
            if (is_aligned<uint16_t>(virt_addr_base)) {
                return mem.write<uint16_t>(virt_addr_base, raw);
            }
        }
        mem.write<uint8_t>(virt_addr_base, raw);
        mem.write<uint8_t>(virt_seg_base + self->ptr(offset + 1), raw >> 8);
        return;
    }
    else if constexpr (sizeof(T) == sizeof(uint32_t)) {
        size_t virt_seg_base = this->seg();
        size_t virt_addr_base = virt_seg_base + self->ptr(offset);
        uint32_t raw = std::bit_cast<uint32_t>(value);
        if constexpr (ctx.bus_width >= 16) {
            if constexpr (ctx.bus_width >= 32) {
                if (is_aligned<uint32_t>(virt_addr_base)) {
                    return mem.write<uint32_t>(virt_addr_base, raw);
                }
            }
            if (is_aligned<uint16_t>(virt_addr_base)) {
                mem.write<uint16_t>(virt_addr_base, raw);
                mem.write<uint16_t>(virt_seg_base + self->ptr(offset + 2), raw >> 16);
                return;
            }
        }
        uint32_t raw = *(uint32_t*)&value;
        mem.write<uint8_t>(virt_addr_base, raw);
        mem.write<uint8_t>(virt_seg_base + self->ptr(offset + 1), raw >> 8);
        mem.write<uint8_t>(virt_seg_base + self->ptr(offset + 2), raw >> 16);
        mem.write<uint8_t>(virt_seg_base + self->ptr(offset + 3), raw >> 24);
        return;
    }
    else if constexpr (sizeof(T) == sizeof(uint64_t)) {
        size_t virt_seg_base = this->seg();
        size_t virt_addr_base = virt_seg_base + self->ptr(offset);
        uint64_t raw = std::bit_cast<uint64_t>(value);
        if constexpr (ctx.bus_width >= 16) {
            if constexpr (ctx.bus_width >= 32) {
                if constexpr (ctx.bus_width >= 64) {
                    if (is_aligned<uint64_t>(self->offset + offset)) {
                        return mem.write<T>(self->addr(offset), value);
                    }
                }
                if (is_aligned<uint32_t>(self->offset + offset)) {
                    uint64_t raw = *(uint64_t*)&value;
                    mem.write<uint32_t>(self->addr(offset), raw);
                    mem.write<uint32_t>(self->addr(offset + 4), raw >> 32);
                    return;
                }
            }
            if (is_aligned<uint16_t>(self->offset + offset)) {
                uint64_t raw = *(uint64_t*)&value;
                mem.write<uint16_t>(self->addr(offset), raw);
                mem.write<uint16_t>(self->addr(offset + 2), raw >> 16);
                mem.write<uint16_t>(self->addr(offset + 4), raw >> 32);
                mem.write<uint16_t>(self->addr(offset + 6), raw >> 48);
                return;
            }
        }
        uint64_t raw = *(uint64_t*)&value;
        mem.write<uint8_t>(self->addr(offset), raw);
        mem.write<uint8_t>(self->addr(offset + 1), raw >> 8);
        mem.write<uint8_t>(self->addr(offset + 2), raw >> 16);
        mem.write<uint8_t>(self->addr(offset + 3), raw >> 24);
        mem.write<uint8_t>(self->addr(offset + 4), raw >> 32);
        mem.write<uint8_t>(self->addr(offset + 5), raw >> 40);
        mem.write<uint8_t>(self->addr(offset + 6), raw >> 48);
        mem.write<uint8_t>(self->addr(offset + 7), raw >> 56);
        return;
    }
    else {
        size_t virt_seg_base = this->seg();
        size_t virt_addr_base = virt_seg_base + self->ptr(offset);

    }
    */
}

template <typename T, typename V, typename P>
inline V z86AddrSharedFuncs::read(const P* self, ssize_t offset) {
    if constexpr (!ctx.SINGLE_MEM_WRAPS) {
        // TODO: Check segment limits
        return mem.read<V>(self->addr(offset));
    }
    else {
        // 8086 compatibility
        size_t virt_seg_base = self->seg();
        size_t virt_addr_base = virt_seg_base + self->ptr(offset);
        size_t wrap = self->offset_wrap<V>(offset);
        if (expect(!wrap, true)) {
            return mem.read<V>(virt_addr_base);
        }
        else {
            //union {
                //V ret;
            //};
            if constexpr (sizeof(V) != sizeof(uint16_t)) {
                unsigned char raw[z86DataProperites<V>::size];
                mem.read_movsb(
                    mem.read_movsb(raw, virt_addr_base, wrap),
                    virt_addr_base - self->offset_wrap_sub<V>(wrap),
                    z86DataProperites<V>::size - wrap
                );
                return *(V*)&raw;
            }
            else {
                uint16_t raw;
                raw = mem.read<uint8_t>(virt_addr_base);
                raw |= (uint16_t)mem.read<uint8_t>(virt_addr_base - self->offset_wrap_sub<V>(wrap)) << 8;
                return std::bit_cast<V>(raw);
            }
            //UByteIntTypeEx<z86DataProperites<V>::size> raw = {};

        }
    }

    /*
    if constexpr (sizeof(V) == sizeof(uint8_t)) {
        return mem.read<V>(self->addr(offset));
    }
    else if constexpr (sizeof(V) == sizeof(uint16_t)) {
        if (is_aligned<uint16_t>(self->offset + offset)) {
            return mem.read<V>(self->addr(offset));
        }
        else {
            union {
                V ret;
                uint16_t raw;
            };
            raw = mem.read<uint8_t>(self->addr(offset));
            raw |= mem.read<uint8_t>(self->addr(offset + 1)) << 8;
            return ret;
        }
    }
    else if constexpr (sizeof(V) == sizeof(uint32_t)) {
        return mem.read<V>(self->addr(offset));
    }
    else if constexpr (sizeof(V) == sizeof(uint64_t)) {
        return mem.read<V>(self->addr(offset));
    }
    */
}

template <typename P>
inline uint32_t z86AddrSharedFuncs::read_Iz(const P* self, ssize_t index) {
    if constexpr (ctx.max_bits > 16) {
        if (!ctx.data_size_16()) {
            return self->read<uint32_t>(index);
        }
    }
    return self->read<uint16_t>(index);
}

template <typename P>
inline uint32_t z86AddrSharedFuncs::read_advance_Iz(P* self) {
    if constexpr (ctx.max_bits > 16) {
        if (!ctx.data_size_16()) {
            return self->read_advance<uint32_t>();
        }
    }
    return self->read_advance<uint16_t>();
}

template <typename P>
inline int32_t z86AddrSharedFuncs::read_Is(const P* self, ssize_t index) {
    if constexpr (ctx.max_bits > 16) {
        if (!ctx.data_size_16()) {
            return self->read<int32_t>(index);
        }
    }
    return self->read<int16_t>(index);
}

template <typename P>
inline int32_t z86AddrSharedFuncs::read_advance_Is(P* self) {
    if constexpr (ctx.max_bits > 16) {
        if (!ctx.data_size_16()) {
            return self->read_advance<int32_t>();
        }
    }
    return self->read_advance<int16_t>();
}

template <typename P>
inline uint64_t z86AddrSharedFuncs::read_Iv(const P* self, ssize_t index) {
    if constexpr (ctx.max_bits > 16) {
        if (!ctx.data_size_16()) {
            if constexpr (ctx.max_bits > 32) {
                if (ctx.data_size_64()) {
                    return self->read<uint64_t>(index);
                }
            }
            return self->read<uint32_t>(index);
        }
    }
    return self->read<uint16_t>(index);
}

template <typename P>
inline uint64_t z86AddrSharedFuncs::read_advance_Iv(P* self) {
    if constexpr (ctx.max_bits > 16) {
        if (!ctx.data_size_16()) {
            if constexpr (ctx.max_bits > 32) {
                if (ctx.data_size_64()) {
                    return self->read_advance<uint64_t>();
                }
            }
            return self->read_advance<uint32_t>();
        }
    }
    return self->read_advance<uint16_t>();
}


template <typename P>
inline uint64_t z86AddrSharedFuncs::read_O(const P* self, ssize_t index) {
    if constexpr (ctx.max_bits > 16) {
        if (!ctx.addr_size_16()) {
            if constexpr (ctx.max_bits > 32) {
                if (ctx.addr_size_64()) {
                    return self->read<uint64_t>(index);
                }
            }
            return self->read<uint32_t>(index);
        }
    }
    return self->read<uint16_t>(index);
}

template <typename P>
inline uint64_t z86AddrSharedFuncs::read_advance_O(P* self) {
    if constexpr (ctx.max_bits > 16) {
        if (!ctx.addr_size_16()) {
            if constexpr (ctx.max_bits > 32) {
                if (ctx.addr_size_64()) {
                    return self->read_advance<uint64_t>();
                }
            }
            return self->read_advance<uint32_t>();
        }
    }
    return self->read_advance<uint16_t>();
}

template <z86BaseTemplate>
template <typename T1, typename T2, typename P, typename L>
inline bool regcall z86BaseDefault::binopMR_impl(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    T2& rval = this->index_regR<T2>(modrm.R());
    if (modrm.is_mem()) {
        P data_addr = modrm.parse_memM(pc);
        T1 mval = data_addr.read<T1>();
        if (lambda(mval, rval)) {
            data_addr.write<T1>(mval);
        }
    }
    else {
        lambda(this->index_regMB<T1>(modrm.M()), rval);
    }
    return false;
}

template <z86BaseTemplate>
template <typename T1, typename T2, typename P, typename L>
inline bool regcall z86BaseDefault::binopRM_impl(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    T2 mval;
    if (modrm.is_mem()) {
        P data_addr = modrm.parse_memM(pc);
        mval = data_addr.read<T2>();
    }
    else {
        mval = this->index_regMB<T2>(modrm.M());
    }
    lambda(this->index_regR<T1>(modrm.R()), mval);
    return false;
}

// Bit test memory operand
template <z86BaseTemplate>
template <typename T, typename P, typename L>
inline bool regcall z86BaseDefault::binopMRB_impl(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    T rval = this->index_regR<T>(modrm.R());
    T mask = rval & bitsof(T) - 1;
    if (modrm.is_mem()) {
        P data_addr = modrm.parse_memM(pc);
        data_addr.offset += sizeof(T) * (rval >> std::bit_width(bitsof(T) - 1));
        T mval = data_addr.read<T>();
        if (lambda(mval, mask)) {
            data_addr.write<T>(mval);
        }
    }
    else {
        lambda(this->index_regMB<T>(modrm.M()), mask);
    }
    return false;
}

// Far width memory operand, special for LDS/LES
template <z86BaseTemplate>
template <typename T, typename P, typename L>
inline bool regcall z86BaseDefault::binopRMF_impl(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    T& rval = this->index_regR<T>(modrm.R());
    if (modrm.is_mem()) {
        using DT = dbl_int_t<T>;
        P data_addr = modrm.parse_memM(pc);
        DT temp = data_addr.read<T>();
        temp |= (DT)data_addr.read<uint16_t>(sizeof(T)) << bitsof(T);
        lambda(rval, temp);
    }
    else {
        if (!this->NO_UD) {
            // TODO: jank
        }
        else {
            this->set_fault(IntUD);
            return true;
        }
    }
    return false;
}

// Double width memory operand, special for BOUND
template <z86BaseTemplate>
template <typename T, typename P, typename L>
inline bool regcall z86BaseDefault::binopRM2_impl(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    T& rval = this->index_regR<T>(modrm.R());
    if (modrm.is_mem()) {
        P data_addr = modrm.parse_memM(pc);
        return lambda(rval, data_addr.read<T>(), data_addr.read<T>(sizeof(T)));
    }
    else {
        if constexpr (!this->NO_UD) {
            // TODO: jank
        }
        else {
            this->set_fault(IntUD);
            return true;
        }
    }
    return false;
}

template <z86BaseTemplate>
template <typename T, typename P, typename L>
inline bool regcall z86BaseDefault::binopMS_impl(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    uint16_t rval = this->get_seg(modrm.R());
    if (modrm.is_mem()) {
        P data_addr = modrm.parse_memM(pc);
        T mval = data_addr.read<T>();
        if (lambda(mval, rval)) {
            data_addr.write<T>(mval);
        }
    }
    else {
        lambda(this->index_regMB<T>(modrm.M()), rval);
    }
    return false;
}

template <z86BaseTemplate>
template <typename T, typename P, typename L>
inline bool regcall z86BaseDefault::binopSM_impl(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    T mval;
    if (modrm.is_mem()) {
        P data_addr = modrm.parse_memM(pc);
        mval = data_addr.read<T>();
    }
    else {
        mval = this->index_regMB<T>(modrm.M());
    }
    uint8_t seg_index = modrm.R();
    uint16_t rval = this->get_seg(seg_index);
    lambda(rval, mval);
    this->write_seg(seg_index, rval);
    return false;
}

template <z86BaseTemplate>
template <typename T, typename P, typename L>
inline bool regcall z86BaseDefault::binopMR_MM(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    MMXT<T>& rval = this->index_mmx_reg<T>(modrm.R());
    if (modrm.is_mem()) {
        P data_addr = modrm.parse_memM(pc);
        MMXT<T> mval = data_addr.read<MMXT<T>>();
        if (lambda(mval, rval)) {
            data_addr.write<MMXT<T>>(mval);
        }
    }
    else {
        lambda(this->index_mmx_reg<T>(modrm.M()), rval);
    }
    return false;
}

template <z86BaseTemplate>
template <typename T, typename P, typename L>
inline bool regcall z86BaseDefault::binopRM_MM(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    MMXT<T> mval;
    if (modrm.is_mem()) {
        P data_addr = modrm.parse_memM(pc);
        mval = data_addr.read<MMXT<T>>();
    }
    else {
        mval = this->index_mmx_reg<T>(modrm.M());
    }
    lambda(this->index_mmx_reg<T>(modrm.R()), mval);
    return false;
}

template <z86BaseTemplate>
template <typename T, typename P, typename L>
inline bool regcall z86BaseDefault::binopMR_XX(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    SSET<T>& rval = this->index_xmm_regR<T>(modrm.R());
    if (modrm.is_mem()) {
        P data_addr = modrm.parse_memM(pc);
        SSET<T> mval = data_addr.read<SSET<T>>();
        if (lambda(mval, rval)) {
            data_addr.write<SSET<T>>(mval);
        }
    }
    else {
        lambda(this->index_xmm_regMB<T>(modrm.M()), rval);
    }
    return false;
}

template <z86BaseTemplate>
template <typename T, typename P, typename L>
inline bool regcall z86BaseDefault::binopRM_XX(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    SSET<T> mval;
    if (modrm.is_mem()) {
        P data_addr = modrm.parse_memM(pc);
        mval = data_addr.read<SSET<T>>();
    }
    else {
        mval = this->index_xmm_regMB<T>(modrm.M());
    }
    lambda(this->index_xmm_regR<T>(modrm.R()), mval);
    return false;
}

template <z86BaseTemplate>
template <typename T, typename P, typename L>
inline bool regcall z86BaseDefault::binopRM_MX(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    SSET<T> mval;
    if (modrm.is_mem()) {
        P data_addr = modrm.parse_memM(pc);
        mval = { data_addr.read<T>() };
    }
    else {
        mval = this->index_xmm_regMB<T>(modrm.M());
    }
    lambda(this->index_mmx_reg<T>(modrm.R()), mval);
    return false;
}

template <z86BaseTemplate>
template <typename T, typename P, typename L>
inline bool regcall z86BaseDefault::unopM_impl(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    uint8_t r = modrm.R();
    uint8_t ret;
    if (modrm.is_mem()) {
        P data_addr = modrm.parse_memM(pc);
        T mval = data_addr.read<T>();
        ret = lambda(mval, r);
        if (OP_NEEDS_WRITE(ret)) {
            data_addr.write<T>(mval);
        }
    }
    else {
        ret = lambda(this->index_regMB<T>(modrm.M()), r);
    }
    if constexpr (FAULTS_ARE_TRAPS) {
        return false;
    }
    return OP_HAD_FAULT(ret);
}

template <z86BaseTemplate>
template <typename P, typename T>
inline void regcall z86BaseDefault::PUSH_impl(const T& src) {
    this->SP<P>() -= (std::max)(sizeof(T), (size_t)2);
    z86AddrSS stack = this->stack<P>();
    stack.write(src);
}

template <z86BaseTemplate>
template <typename P, typename T>
inline T z86BaseDefault::POP_impl() {
    z86AddrSS stack = this->stack<P>();
    T ret = stack.read<T>();
    this->SP<P>() += (std::max)(sizeof(T), (size_t)2);
    return ret;
}

// No wonder ENTER sucks
template <z86BaseTemplate>
template <typename T>
gnu_attr(minsize) inline void regcall z86BaseDefault::ENTER_impl(uint16_t alloc, uint8_t nesting) {
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        uint64_t cur_bp = this->rbp;
        uint64_t new_bp = this->rsp - sizeof(T);
        this->PUSH_impl<uint64_t>(cur_bp);
        switch (nesting) {
            default: {
                z86Addr stack_bp(this->ss, cur_bp);
                do {
                    stack_bp.offset -= sizeof(T); // sizeof(T) = 8
                    this->PUSH_impl<uint64_t>(stack_bp.read<T>());
                } while (--nesting != 1);
            }
            case 1:
                this->PUSH(new_bp);
            case 0:
                this->rbp = new_bp;
                this->rsp -= alloc;
        }
    }
    else if constexpr (sizeof(T) == sizeof(uint32_t)) {
        uint32_t cur_bp = this->ebp;
        uint32_t new_bp = this->esp - sizeof(T);
        if (expect(this->stack_size_32(), true)) {
            this->PUSH_impl<uint32_t>(cur_bp);
            switch (nesting) {
                default: {
                    z86Addr stack_bp(this->ss, cur_bp);
                    do {
                        stack_bp.offset -= sizeof(T); // sizeof(T) = 4
                        this->PUSH_impl<uint32_t>(stack_bp.read<T>());
                    } while (--nesting != 1);
                }
                case 1:
                    this->PUSH_impl<uint32_t>(new_bp);
                case 0:;
            }
        }
        else {
            this->PUSH_impl<uint16_t>(cur_bp);
            switch (nesting) {
                default: {
                    z86Addr stack_bp(this->ss, (uint16_t)cur_bp);
                    do {
                        stack_bp.offset -= sizeof(T); // sizeof(T) = 4
                        this->PUSH_impl<uint16_t>(stack_bp.read<T>());
                    } while (--nesting != 1);
                }
                case 1:
                    this->PUSH_impl<uint16_t>(new_bp);
                case 0:
                    break;
            }
        }
        this->ebp = new_bp;
        this->esp -= alloc;
    }
    else {
        uint16_t cur_bp = this->bp;
        uint16_t new_bp = this->sp - sizeof(T);
        if constexpr (bits > 16) {
            if (expect(this->stack_size_32(), false)) {
                this->PUSH_impl<uint32_t>(cur_bp);
                switch (nesting) {
                    default: {
                        z86Addr stack_bp(this->ss, this->ebp);
                        do {
                            stack_bp.offset -= sizeof(T); // sizeof(T) = 2
                            this->PUSH_impl<uint32_t>(stack_bp.read<T>());
                        } while (--nesting != 1);
                    }
                    case 1:
                        this->PUSH_impl<uint32_t>(new_bp);
                    case 0:
                        goto end_enter16;
                }
            }
            if constexpr (bits == 64) {
                if (expect(this->stack_size_64(), false)) {
                    this->PUSH_impl<uint64_t>(cur_bp);
                    switch (nesting) {
                        default: {
                            z86Addr stack_bp(this->ss, this->rbp);
                            do {
                                stack_bp.offset -= sizeof(T); // sizeof(T) = 2
                                this->PUSH_impl<uint64_t>(stack_bp.read<T>());
                            } while (--nesting != 1);
                        }
                        case 1:
                            this->PUSH_impl<uint64_t>(new_bp);
                        case 0:
                            goto end_enter16;
                    }
                }
            }
        }
        this->PUSH_impl<uint16_t>(cur_bp);
        switch (nesting) {
            default: {
                z86Addr stack_bp(this->ss, cur_bp);
                do {
                    stack_bp.offset -= sizeof(T); // sizeof(T) = 2
                    this->PUSH_impl<uint16_t>(stack_bp.read<T>());
                } while (--nesting != 1);
            }
            case 1:
                this->PUSH_impl<uint16_t>(new_bp);
            case 0:
                break;
        }
    end_enter16:
        this->bp = new_bp;
        this->sp -= alloc;
    }
}

// TODO: Check what happens if an interrupt toggles 
// the direction flag during a repeating string instruction
template <z86BaseTemplate>
template <typename T, typename P>
inline bool regcall z86BaseDefault::LODS_impl() {
    intptr_t offset = this->direction ? sizeof(T) : -sizeof(T);
    z86Addr src_addr = this->str_src<P>();
    if (this->has_rep()) {
        if (this->C<P>()) {
            do {
                // TODO: Interrupt check here
                this->A<T>() = src_addr.read_advance<T>(offset);
            } while (--this->C<P>());
        }
    }
    else {
        this->A<T>() = src_addr.read_advance<T>(offset);
    }
    this->SI<P>() = src_addr.offset;
    return false;
}

template <z86BaseTemplate>
template <typename T, typename P>
inline bool regcall z86BaseDefault::MOVS_impl() {
    intptr_t offset = this->direction ? sizeof(T) : -sizeof(T);
    z86Addr src_addr = this->str_src<P>();
    z86AddrES dst_addr = this->str_dst<P>();
    if (this->has_rep()) {
        if (this->C<P>()) {
            do {
                // TODO: Interrupt check here
                dst_addr.write_advance<T>(src_addr.read_advance<T>(offset), offset);
            } while (--this->C<P>());
        }
    }
    else {
        dst_addr.write_advance<T>(src_addr.read_advance<T>(offset), offset);
    }
    this->SI<P>() = src_addr.offset;
    this->DI<P>() = dst_addr.offset;
    return false;
}

template <z86BaseTemplate>
template <typename T, typename P>
inline bool regcall z86BaseDefault::STOS_impl() {
    intptr_t offset = this->direction ? sizeof(T) : -sizeof(T);
    z86AddrES dst_addr = this->str_dst<P>();
    if (this->has_rep()) {
        if (this->C<P>()) {
            do {
                // TODO: Interrupt check here
                dst_addr.write_advance<T>(this->A<T>(), offset);
            } while (--this->C<P>());
        }
    }
    else {
        dst_addr.write_advance<T>(this->A<T>(), offset);
    }
    this->DI<P>() = dst_addr.offset;
    return false;
}

template <z86BaseTemplate>
template <typename T, typename P>
inline bool regcall z86BaseDefault::SCAS_impl() {
    intptr_t offset = this->direction ? sizeof(T) : -sizeof(T);
    z86AddrES dst_addr = this->str_dst<P>();
    if (this->has_rep()) {
        if (this->C<P>()) {
            if constexpr (OPCODES_V20 && !OPCODES_80386) {
                if (this->is_repc()) {
                    do {
                        // TODO: Interrupt check here
                        this->CMP<T>(this->A<T>(), dst_addr.read_advance<T>(offset));
                    } while (--this->C<P>() && this->rep_type == this->carry + 2);
                    goto finish;
                }
            }
            do {
                // TODO: Interrupt check here
                this->CMP<T>(this->A<T>(), dst_addr.read_advance<T>(offset));
            } while (--this->C<P>() && this->rep_type == this->zero);
        }
    }
    else {
        this->CMP<T>(this->A<T>(), dst_addr.read_advance<T>(offset));
    }
finish:
    this->DI<P>() = dst_addr.offset;
    return false;
}

template <z86BaseTemplate>
template <typename T, typename P>
inline bool regcall z86BaseDefault::CMPS_impl() {
    intptr_t offset = this->direction ? sizeof(T) : -sizeof(T);
    z86Addr src_addr = this->str_src<P>();
    z86AddrES dst_addr = this->str_dst<P>();
    if (this->has_rep()) {
        if (this->C<P>()) {
            if constexpr (OPCODES_V20 && !OPCODES_80386) {
                if (this->is_repc()) {
                    do {
                        // TODO: Interrupt check here
                        this->CMP<T>(src_addr.read_advance<T>(offset), dst_addr.read_advance<T>(offset));
                    } while (--this->C<P>() && this->rep_type == this->carry + 2);
                    goto finish;
                }
            }
            do {
                // TODO: Interrupt check here
                this->CMP<T>(src_addr.read_advance<T>(offset), dst_addr.read_advance<T>(offset));
            } while (--this->C<P>() && this->rep_type == this->zero);
        }
    }
    else {
        this->CMP<T>(src_addr.read_advance<T>(offset), dst_addr.read_advance<T>(offset));
    }
finish:
    this->SI<P>() = src_addr.offset;
    this->DI<P>() = src_addr.offset;
    return false;
}

template <z86BaseTemplate>
template <typename P>
inline bool regcall z86BaseDefault::ADD4S_impl() {
    // How does this actually behave on real hardware?
    z86Addr src_addr = this->str_src<P>();
    z86AddrES dst_addr = this->str_dst<P>();
    // Yes, this overflows ridiculously for 0 and 255
    uint16_t count = (uint8_t)(this->cl + 1) >> 1;
    while (--count) {

    }
}

template <z86BaseTemplate>
template <typename T, typename P>
inline bool regcall z86BaseDefault::OUTS_impl() {
    intptr_t offset = this->direction ? sizeof(T) : -sizeof(T);
    z86Addr src_addr = this->str_src<P>();
    uint16_t port = this->dx;
    if (this->has_rep()) {
        if (this->C<P>()) {
            do {
                // TODO: Interrupt check here
                this->port_out_impl<T>(port, src_addr.read_advance<T>(offset));
            } while (--this->C<P>());
        }
    }
    else {
        this->port_out_impl<T>(port, src_addr.read_advance<T>(offset));
    }
    this->SI<P>() = src_addr.offset;
    return false;
}

template <z86BaseTemplate>
template <typename T, typename P>
inline bool regcall z86BaseDefault::INS_impl() {
    intptr_t offset = this->direction ? sizeof(T) : -sizeof(T);
    z86AddrES dst_addr = this->str_dst<P>();
    uint16_t port = this->dx;
    if (this->has_rep()) {
        if (this->C<P>()) {
            do {
                // TODO: Interrupt check here
                dst_addr.write_advance<T>(this->port_in_impl<T>(port), offset);
            } while (--this->C<P>());
        }
    }
    else {
        dst_addr.write_advance<T>(this->port_in_impl<T>(port), offset);
    }
    this->DI<P>() = dst_addr.offset;
    return false;
}

template <z86BaseTemplate>
template <typename T>
inline void regcall z86BaseDefault::port_out_impl(uint16_t port, T value) const {
    uint32_t full_port = port;

    if constexpr (sizeof(T) == sizeof(uint8_t)) {
        const std::vector<PortByteDevice*>& devices = io_byte_devices;
        for (auto device : devices) {
            if (device->out_byte(full_port, value)) {
                return;
            }
        }
        printf("Unhandled: OUT %X, %02X\n", port, value);
    }
    else if constexpr (sizeof(T) == sizeof(uint16_t)) {
        const std::vector<PortWordDevice*>& devices = io_word_devices;
        for (auto device : devices) {
            if constexpr (bus >= 16) {
                if (
                    is_aligned<uint16_t>(full_port) &&
                    device->out_word(full_port, value)
                ) {
                    return;
                }
            }
            if (
                device->out_byte(full_port, value) &&
                device->out_byte(full_port + 1, value >> 8)
            ) {
                return;
            }
        }
        printf("Unhandled: OUT %X, %04X\n", port, value);
    }
    else if constexpr (sizeof(T) == sizeof(uint32_t)) {
        const std::vector<PortDwordDevice*>& devices = io_dword_devices;
        for (auto device : devices) {
            if constexpr (bus >= 32) {
                if (
                    is_aligned<uint32_t>(full_port) &&
                    device->out_dword(full_port, value)
                ) {
                    return;
                }
            }
            if constexpr (bus >= 16) {
                if (
                    is_aligned<uint16_t>(full_port) &&
                    device->out_word(full_port, value) &&
                    device->out_word(full_port + 2, value >> 16)
                ) {
                    return;
                }
            }
            if (
                device->out_byte(full_port, value) &&
                device->out_byte(full_port + 1, value >> 8) &&
                device->out_byte(full_port + 2, value >> 16) &&
                device->out_byte(full_port + 3, value >> 24)
            ) {
                return;
            }
        }
        printf("Unhandled: OUT %X, %08X\n", port, value);
    }
}

template <z86BaseTemplate>
template <typename T>
inline T regcall z86BaseDefault::port_in_impl(uint16_t port) {
    uint32_t full_port = port;

    T value;

    if constexpr (sizeof(T) == sizeof(uint8_t)) {
        const std::vector<PortByteDevice*>& devices = io_byte_devices;
        for (auto device : devices) {
            if (device->in_byte(value, full_port)) {
                return value;
            }
        }
        printf("Unhandled: IN AL, %X\n", full_port);
    }
    else if constexpr (sizeof(T) == sizeof(uint16_t)) {
        const std::vector<PortWordDevice*>& devices = io_word_devices;
        for (auto device : devices) {
            if constexpr (bus >= 16) {
                if (
                    is_aligned<uint16_t>(full_port) &&
                    device->in_word(value, full_port)
                ) {
                    return value;
                }
            }
            if (
                device->in_byte(((uint8_t*)&value)[0], full_port) &&
                device->in_byte(((uint8_t*)&value)[1], full_port + 1)
            ) {
                return value;
            }
        }
        printf("Unhandled: IN AX, %X\n", full_port);
    }
    else if constexpr (sizeof(T) == sizeof(uint32_t)) {
        const std::vector<PortDwordDevice*>& devices = io_dword_devices;
        for (auto device : devices) {
            if constexpr (bus >= 32) {
                if (
                    is_aligned<uint32_t>(full_port) &&
                    device->in_dword(value, full_port)
                ) {
                    return value;
                }
            }
            if constexpr (bus >= 16) {
                if (
                    is_aligned<uint16_t>(full_port) &&
                    device->in_word(((uint16_t*)&value)[0], full_port) &&
                    device->in_word(((uint16_t*)&value)[1], full_port + 2)
                ) {
                    return value;
                }
            }
            if (
                device->in_byte(((uint8_t*)&value)[0], full_port) &&
                device->in_byte(((uint8_t*)&value)[1], full_port + 1) &&
                device->in_byte(((uint8_t*)&value)[2], full_port + 2) &&
                device->in_byte(((uint8_t*)&value)[3], full_port + 3)
            ) {
                return value;
            }
        }
        printf("Unhandled: IN EAX, %X\n", full_port);
    }
    return 0;
}

template <z86BaseTemplate>
template <typename T, typename P>
inline bool regcall z86BaseDefault::MASKMOV_impl(T& src, T mask) {
    z86Addr data_addr = this->addr(DS, this->DI<P>());

    uint8_t* data_vec = (uint8_t*)&src;

    // TODO: Get the compiler to generate PMOVMSKB for this
    for (size_t i = 0; i < sizeof(T); ++i) {
        if (mask[i] & 0x80) {
            data_addr.write<uint8_t>(data_vec[i], i);
        }
    }
    return false;
}

// Special unop for groups 4/5
template <z86BaseTemplate>
template <typename T, typename P>
inline bool regcall z86BaseDefault::unopMS_impl(P& pc) {
    ModRM modrm = pc.read_advance<ModRM>();
    uint8_t r = modrm.R();
    T mval;
    uint16_t sval = 0; // TODO: jank
    P data_addr;
    if (modrm.is_mem()) {
        data_addr = modrm.parse_memM(pc);
        mval = data_addr.read<T>();
        switch (r) {
            case 0:
                ctx.INC(mval);
                break;
            case 1:
                ctx.DEC(mval);
                break;
            case 2:
            
            call:
                ctx.CALLABS<T>(pc.offset, mval);
                return true;
            case 3:
                sval = data_addr.read<uint16_t>(sizeof(T)); // TODO: Byte offset?
            callf:
                ctx.CALLFABS<T>(pc.offset, mval, sval);
                return true;
            case 4: jmp:
                ctx.JMPABS(mval);
                return true;
            case 5:
                sval = data_addr.read<uint16_t>(sizeof(T)); // TODO: Byte offset?
            jmpf:
                ctx.JMPFABS(mval, sval);
                return true;
            case 6: case 7: push:
                ctx.PUSH(mval);
                return false;
            default:
                unreachable;
        }
        data_addr.write<T>(mval);
    }
    else {
        T& mval_ref = ctx.index_regMB<T>(modrm.M());
        mval = mval_ref;
        switch (r) {
            case 0:
                ctx.INC(mval_ref);
                break;
            case 1:
                ctx.DEC(mval_ref);
                break;
            case 2:
                goto call;
            case 3:
                // TODO: jank
                goto callf;
            case 4:
                goto jmp;
            case 5:
                // TODO: Jank
                goto jmpf;
            case 6: case 7: 
                goto push;
            default:
                unreachable;
        }
    }
    return false;
}

// Special unop for group 6
template <z86BaseTemplate>
template <typename T, typename P, typename L>
inline bool regcall z86BaseDefault::unopMW_impl(P& pc, const L& lambda) {
    ModRM modrm = pc.read_advance<ModRM>();
    uint8_t r = modrm.R();
    uint8_t ret;
    if (modrm.is_mem()) {
        P data_addr = modrm.parse_memM(pc);
        T mval = data_addr.read<uint16_t>();
        ret = lambda(mval, r);
        if (OP_NEEDS_WRITE(ret)) {
            data_addr.write<uint16_t>(mval);
        }
    }
    else {
        ret = lambda(this->index_regMB<T>(modrm.M()), r);
    }
    if constexpr (FAULTS_ARE_TRAPS) {
        return false;
    }
    return OP_HAD_FAULT(ret);
}

template <z86BaseTemplate>
template <typename T, typename P, typename LM, typename LR>
inline bool regcall z86BaseDefault::unopMM_impl(P& pc, const LM& lambdaM, const LR& lambdaR) {
    ModRM modrm = pc.read_advance<ModRM>();
    uint8_t r = modrm.R();
    uint8_t ret;
    if (modrm.is_mem()) {
        ret = lambdaM(modrm.parse_memM(pc), r);
    }
    else {
        ret = lambdaR(this->index_regMB<T>(modrm.M()), r);
    }
    if constexpr (FAULTS_ARE_TRAPS) {
        return false;
    }
    return OP_HAD_FAULT(ret);
}

#endif