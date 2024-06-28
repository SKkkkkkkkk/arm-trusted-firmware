#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <common/debug.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <drivers/io/io_storage.h>
#include <dw_apb_gpio.h>
#include <sha2.h>
#include <uECC.h>

/*
00000000: cfda b924 8228 eba5 ec93 9fd2 7dfa 4a2a  ...$.(......}.J*
00000010: fa4a 07d5 6615 d361 120b 3454 8c28 5a19  .J..f..a..4T.(Z.
00000020: ab4a a797 4d6b f1f4 2fe4 37a4 9289 966b  .J..Mk../.7....k
00000030: d53a 3cff 6bd0 f327 5559 21c3 84e1 f156  .:<.k..'UY!....V
*/
static const uint8_t PUBKEY[] = {
	0xcf, 0xda, 0xb9, 0x24, 0x82, 0x28, 0xeb, 0xa5, 0xec, 0x93, 0x9f, 0xd2, 0x7d, 0xfa, 0x4a, 0x2a,
	0xfa, 0x4a, 0x07, 0xd5, 0x66, 0x15, 0xd3, 0x61, 0x12, 0x0b, 0x34, 0x54, 0x8c, 0x28, 0x5a, 0x19,
	0xab, 0x4a, 0xa7, 0x97, 0x4d, 0x6b, 0xf1, 0xf4, 0x2f, 0xe4, 0x37, 0xa4, 0x92, 0x89, 0x96, 0x6b,
	0xd5, 0x3a, 0x3c, 0xff, 0x6b, 0xd0, 0xf3, 0x27, 0x55, 0x59, 0x21, 0xc3, 0x84, 0xe1, 0xf1, 0x56,
};

static inline void* get_pubkey(void)
{
	return (void*)PUBKEY;
}

static void prv_sha256(const void *buf, uint32_t size, uint8_t *hash_out)
{
  cf_sha256_context ctx;
  cf_sha256_init(&ctx);
  cf_sha256_update(&ctx, buf, size);
  cf_sha256_digest_final(&ctx, hash_out);
}

static int get_signature(unsigned int image_id, void* signature)
{
	// 1. open sig image
	int io_result;
	uintptr_t dev_handle;
	uintptr_t image_handle;
	uintptr_t image_spec;
	size_t image_size;
	size_t bytes_read;

	/* Obtain a reference to the image by querying the platform layer */
	io_result = plat_get_image_source(image_id, &dev_handle, &image_spec);
	if (io_result != 0) {
		WARN("Failed to obtain reference to image id=%u (%i)\n",
			image_id, io_result);
		return io_result;
	}

	/* Attempt to access the image */
	io_result = io_open(dev_handle, image_spec, &image_handle);
	if (io_result != 0) {
		WARN("Failed to access image id=%u (%i)\n",
			image_id, io_result);
		return io_result;
	}

	/* Find the size of the image */
	io_result = io_size(image_handle, &image_size);
	if ((io_result != 0) || (image_size != 64U)) {
		WARN("Failed to determine the size of the image id=%u (%i)\n",
			image_id, io_result);
		goto exit;
	}

	io_result = io_read(image_handle, (uintptr_t)signature, 64, &bytes_read);
	if ((io_result != 0) || (bytes_read != 64U)) {
		WARN("Failed to load image id=%u (%i)\n", image_id, io_result);
		goto exit;
	}

	exit:
	(void)io_close(image_handle);
	/* Ignore improbable/unrecoverable error in 'close' */

	/* TODO: Consider maintaining open device connection from this bootloader stage */
	(void)io_dev_close(dev_handle);
	/* Ignore improbable/unrecoverable error in 'dev_close' */

	return 0;
}

bool seehi_secureboot_verify(unsigned int image_id, image_info_t *image_info)
{
	assert(image_info != NULL);

	if(image_id == BL2_IMAGE_ID)
		image_id = BL2_SIG_ID;
	else if(image_id == BL31_IMAGE_ID)
		image_id = BL31_SIG_ID;
	else
		return true;

	pinmux_select(PORTB, BYPSECURE_KEY_PIN, 7);
	gpio_init_config_t gpio_init_config = {
		.port = PORTB,
		.pin = BYPSECURE_KEY_PIN,
		.gpio_control_mode = Software_Mode,
		.gpio_mode = GPIO_Input_Mode
	};
	gpio_init(&gpio_init_config);

	if(gpio_read_pin(PORTB, BYPSECURE_KEY_PIN) == GPIO_PIN_SET)
	{
		NOTICE("Bypass secure boot\n");
		return true;
	}

	// 1. get signature.
	uint8_t singature[64];
	int ret = get_signature(image_id, singature);
	if (ret != 0) {
		ERROR("Failed to get signature of image\n");
		return false;
	}

	// 2. generate sha256 hash of image.
	uint8_t hash_sha256[CF_SHA256_HASHSZ];
	prv_sha256((void*)image_info->image_base, image_info->image_size, hash_sha256);

	// 3. verify signature.
	if (!uECC_verify(get_pubkey(), hash_sha256, CF_SHA256_HASHSZ, (void*)singature, uECC_secp256k1()))
	{
		ERROR("Signature is NOT valid\n");
		return false;
	}
	else
		NOTICE("Signature is valid\n");

	return true;
}