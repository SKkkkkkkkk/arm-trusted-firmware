#include <stdint.h>
#include "dram.h"

void dram_init(void) {
    static volatile uint8_t dram_dummy_data[50 * 1024];
    dram_dummy_data[0] = 0xff;
    (void)dram_dummy_data[0]; 
}
