#include "mc.h"

#define MC_REG32(a) (*(volatile uint32_t*)(a))

void mc_init(uint64_t addr, uint8_t layer) {
    // global
    if (layer == 4) {
        MC_REG32(addr+0x00013054) = 0x00000000;
        MC_REG32(addr+0x00013004) = 0x00000000;
        MC_REG32(addr+0x00013004) = 0x80000000;
    } else {
        MC_REG32(addr+0x00013054) = 0x00000000;
        MC_REG32(addr+0x00013004) = 0x00000010;
        MC_REG32(addr+0x00013004) = 0x80000010;
    }

    // bank
    uint32_t i, j, k;
    for (i = 0; i < 72; i++) {
        j = i / 18;
        k = i + j; // skip hub regs
        MC_REG32(addr+k*0x400+0x004) = 0x00000005;
        MC_REG32(addr+k*0x400+0x004) = 0x00000001;
        MC_REG32(addr+k*0x400+0x004) = 0x80000001;
    }
}

