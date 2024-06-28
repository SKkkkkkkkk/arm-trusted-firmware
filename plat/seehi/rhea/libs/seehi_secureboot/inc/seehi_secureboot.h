#ifndef __SEEHI_SECUREBOOT_H__
#define __SEEHI_SECUREBOOT_H__

#include <common/bl_common.h>

bool seehi_secureboot_verify(unsigned int image_id, image_info_t *image_info);

#endif // __SEEHI_SECUREBOOT_H__