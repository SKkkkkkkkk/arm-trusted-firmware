#ifndef PLAT_COMMON_H
#define PLAT_COMMON_H

extern int plat_io_setup(void);
extern bool plat_bl1_fwu_needed(void);
extern void console_16550_with_dlf_init(void);
extern void rhea_pcie_ep_init(void);

#endif /* PLAT_COMMON_H */
