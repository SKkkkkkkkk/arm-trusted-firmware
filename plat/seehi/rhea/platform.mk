PLAT_INCLUDES := \
-Iplat/seehi/rhea/include \
-Iplat/seehi/common/drivers/gpio/dw_apb_gpio/inc

PLAT_BL_COMMON_SOURCES	:=	\
drivers/ti/uart/aarch64/16550_console.S \
drivers/delay_timer/delay_timer.c \
drivers/delay_timer/generic_delay_timer.c \
drivers/io/io_storage.c \
drivers/io/io_fip.c	\
drivers/io/io_memmap.c \
plat/seehi/rhea/plat_io_storage.c \
plat/seehi/common/drivers/gpio/dw_apb_gpio/src/dw_apb_gpio.c


include lib/xlat_tables_v2/xlat_tables.mk
PLAT_BL_COMMON_SOURCES	+=	${XLAT_TABLES_LIB_SRCS}

BL1_SOURCES +=	\
lib/cpus/aarch64/aem_generic.S \
lib/cpus/aarch64/cortex_a55.S

BL1_SOURCES += \
plat/seehi/rhea/plat_helper.S \
plat/seehi/rhea/bl1/plat_bl1_setup.c

# This applies errata 1530923 workaround to all revisions of Cortex-A55 CPU.
ERRATA_A55_1530923 := 1
