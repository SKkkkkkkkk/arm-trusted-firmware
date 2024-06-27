# This applies errata 1530923 workaround to all revisions of Cortex-A55 CPU.
ERRATA_A55_1530923 := 1

# Enable dynamic memory mapping
PLAT_XLAT_TABLES_DYNAMIC :=	1

PLAT_INCLUDES := \
-Itools/seehi/time_stamp/inc \
-Iplat/seehi/rhea/include \
-Iplat/seehi/rhea/drivers/gpio/dw_apb_gpio/inc \
-Iplat/seehi/rhea/drivers/spi/dw_apb_ssi/inc \
-Iplat/seehi/rhea/drivers/mmc/dw_mmc/inc \
-Iplat/seehi/rhea/libs/flash/nor/inc \
-Iplat/seehi/rhea/libs/flash/nand/inc

PLAT_BL_COMMON_SOURCES	:=	\
drivers/ti/uart/aarch64/16550_console.S \
drivers/delay_timer/delay_timer.c \
drivers/delay_timer/generic_delay_timer.c \
plat/seehi/rhea/plat_helper.S \
plat/seehi/rhea/plat_rhea_common.c

include lib/xlat_tables_v2/xlat_tables.mk
PLAT_BL_COMMON_SOURCES	+=	${XLAT_TABLES_LIB_SRCS}

BL1_BL2_COMMON_SOURCES	:=	\
drivers/io/io_storage.c \
drivers/io/io_fip.c	\
drivers/io/io_memmap.c \
drivers/mmc/mmc.c \
plat/seehi/rhea/io_flash.c \
plat/seehi/rhea/io_mmc.c \
plat/seehi/rhea/plat_io_storage.c \
plat/seehi/rhea/drivers/gpio/dw_apb_gpio/src/dw_apb_gpio.c \
plat/seehi/rhea/drivers/spi/dw_apb_ssi/src/dw_apb_ssi.c \
plat/seehi/rhea/drivers/mmc/dw_mmc/src/dw_mmc.c \
plat/seehi/rhea/libs/flash/nor/src/nor_flash.c \
plat/seehi/rhea/libs/flash/nand/src/nand_flash.c


BL1_SOURCES +=	\
lib/cpus/aarch64/aem_generic.S \
lib/cpus/aarch64/cortex_a55.S

BL1_SOURCES += \
${BL1_BL2_COMMON_SOURCES} \
plat/seehi/rhea/bl1/plat_bl1_setup.c \
plat/seehi/rhea/bl1/bl1_img_desc.c

BL2_SOURCES += \
${BL1_BL2_COMMON_SOURCES} \
common/desc_image_load.c \
plat/seehi/rhea/bl2/plat_bl2_setup.c \
plat/seehi/rhea/bl2/rhea_bl2_mem_params_desc.c \
plat/seehi/rhea/bl2/rhea_image_load.c

# Include GICv3 driver files
GICV3_SUPPORT_GIC600 := 1
include drivers/arm/gic/v3/gicv3.mk
RHEA_GICV3_SOURCES	:=	\
${GICV3_SOURCES} \
plat/common/plat_gicv3.c \
plat/seehi/rhea/bl31/rhea_gicv3.c

BL31_SOURCES +=	\
lib/cpus/aarch64/aem_generic.S \
lib/cpus/aarch64/cortex_a55.S

BL31_SOURCES += \
${RHEA_GICV3_SOURCES} \
plat/common/plat_psci_common.c \
plat/seehi/rhea/bl31/plat_bl31_setup.c \
plat/seehi/rhea/bl31/rhea_pm.c \
plat/seehi/rhea/bl31/rhea_topology.c

.PHONY: rom
rom: ${BUILD_PLAT}/rom.bin
${BUILD_PLAT}/rom.bin: ${BUILD_PLAT}/bl1.bin ${NS_BL1U}
	$(ECHO) "  DD      $@"
	$(Q)dd if=${BUILD_PLAT}/bl1.bin of=$@ bs=76K conv=sync status=none
	$(Q)cat ${NS_BL1U} >> $@

ifeq (${SEEHI_SECUREBOOT},1)

SEEHI_SECUREBOOT_PRIVATE_KEY	:= plat/seehi/rhea/key/ec-secp256k1-private.pem
RHEA_GEN_SIG_TOOL				?= tools/seehi/gen_signature/generate_ecdsa_signature.py

${BUILD_PLAT}/bl2.bin.sig: ${BUILD_PLAT}/bl2.bin
	${ECHO} "  SIG     $@"
	$(Q)${RHEA_GEN_SIG_TOOL} ${SEEHI_SECUREBOOT_PRIVATE_KEY} $< $@
	
${BUILD_PLAT}/bl31.bin.sig: ${BUILD_PLAT}/bl31.bin
	${ECHO} "  SIG    $@"
	$(Q)${RHEA_GEN_SIG_TOOL} ${SEEHI_SECUREBOOT_PRIVATE_KEY} $< $@

FIP_DEPS += ${BUILD_PLAT}/bl2.bin.sig ${BUILD_PLAT}/bl31.bin.sig
FIP_ARGS += --blob uuid=cafe086d-4cfe-9846-9b95-2950cbbd5a02,file=${BUILD_PLAT}/bl2.bin.sig
FIP_ARGS += --blob uuid=cafe086d-4cfe-9846-9b95-2950cbbd5a03,file=${BUILD_PLAT}/bl31.bin.sig

endif