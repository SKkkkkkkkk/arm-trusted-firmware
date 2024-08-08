#include <stdint.h>
#include "dram.h"
#include "mc.h"
#include "memmap.h"

void dram_init(void) {
    static volatile uint8_t dram_dummy_data[50 * 1024];
    dram_dummy_data[0] = 0xff;
    (void)dram_dummy_data[0];
    mc_init(TCM_CFG_BASE, 4); // verification needs this to be done.
}
