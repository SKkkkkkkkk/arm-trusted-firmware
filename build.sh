CROSS_COMPILE=aarch64-none-elf- \
make \
OPENSSL_DIR=/opt/homebrew/Cellar/openssl@3/3.2.1 \
DEBUG=1 LOG_LEVEL=50 HW_ASSISTED_COHERENCY=1 USE_COHERENT_MEM=0 \
PLAT=rhea bl1 \
-j