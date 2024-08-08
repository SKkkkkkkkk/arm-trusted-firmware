#ifndef __MC_H__
#define __MC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "memmap.h"

void mc_init(uint64_t addr, uint8_t layer);

#ifdef __cplusplus
}
#endif
#endif

