/*
 * cifra - embedded cryptography library
 * Written in 2020 by Silex Insight.
 *
 * To the extent possible under law, the author(s) have dedicated all
 * copyright and related and neighboring rights to this software to the
 * public domain worldwide. This software is distributed without any
 * warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication
 * along with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#include "sha3_shake.h"
#include "handy.h"
#include "cutest.h"
#include "testutil.h"
#include "testshake.h"

/* unless otherwise specified, reference outputs were generated online */
static void test_shake_128(void)
{
  const cf_cshake *H = &cf_shake_128;

  vector_shake(H, "", 0,
    "\x7f\x9c\x2b\xa4\xe8\x8f\x82\x7d"
    "\x61\x60\x45\x50\x76\x05\x85\x3e"
    "\xd7\x3b\x80\x93\xf6\xef\xbc\x88"
    "\xeb\x1a\x6e\xac\xfa\x66\xef\x26"
    "\x3c\xb1\xee\xa9\x88\x00\x4b\x93"
    "\x10", 41);

  vector_shake(H, "abc", 3,
    "\x58\x81\x09\x2d\xd8\x18\xbf\x5c"
    "\xf8\xa3\xdd\xb7\x93\xfb\xcb\xa7"
    "\x40\x97\xd5\xc5\x26\xa6\xd3\x5f"
    "\x97\xb8\x33\x51\x94\x0f\x2c\xc8", 32);

  vector_shake(H, "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
    "\x1a\x96\x18\x2b\x50\xfb\x8c\x7e"
    "\x74\xe0\xa7\x07\x78\x8f\x55\xe9"
    "\x82\x09\xb8\xd9", 20);

  vector_shake(H,
    "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
    "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 112,
    "\x7b\x6d\xf6\xff\x18\x11\x73\xb6"
    "\xd7\x89\x8d\x7f\xf6\x3f\xb0\x7b"
    "\x7c\x23\x7d\xaf\x47\x1a\x5a\xe5"
    "\x60\x2a\xdb\xcc\xef\x9c\xcf\x4b"
    "\x37\xe0\x6b\x4a\x35\x43\x16\x4f"
    "\xfb\xe0\xd0\x55\x7c\x02\xf9\xb2"
    "\x5a\xd4\x34\x00\x55\x26\xd8\x8c"
    "\xa0\x4a\x60\x94\xb9\x3e\xe5\x7a"
    "\x55\xd5\xea\x66\xe7\x44\xbd\x39"
    "\x1f\x8f\x52\xba\xf4\xe0\x31\xd9"
    "\xe6\x0e\x5c\xa3\x2a\x0e\xd1\x62"
    "\xbb\x89\xfc\x90\x80\x97\x98\x45"
    "\x48\x79\x66\x52\x95\x2d\xd4\x73"
    "\x7d\x2a\x23\x4a\x40\x1f\x48\x57"
    "\xf3\xd1\x86\x6e\xfa\x73\x6f\xd6"
    "\xa8\xf7\xc0\xb5\xd0\x2a\xb0\x6e", 128);

  vector_shake(H,
    "erguihqprgbivvsdvslvjw;oer93askcavcndiqp,gwgp[f;dfnweufjcsncapqq"
    "erigeornbqe;nq;23742rge8238e7vvnqwwqqwqoro93nmpp[];;;9327v2v8511"
    "aaaaaaaaabbbbbbbbbbbbbbdddddddddbbbbbvvvvvvvvvssssssssssssuuuuuu"
    "ggggggggdddddddddddffffffffffffseeeeeeeeeeeeewwwwwwwwwwwiiiiiiii"
    "tttttttttpppppppkkkkkkkkkkllllllpouetgdcknqgwueggghghow;oeoeoewq"
    "rrrrrrrrrrrrrrrr", 336,
    "\x03\xdc\xc8\xf8\x8d\xc8\x14\xeb"
    "\xf1\x43\x2d\x72\xa1\x07\x4d\xcb"
    "\x62\x74\x93\x4e\xc7\x8b\x9d\x2b"
    "\x0a\xde\x79\xb4\x9f\x55\x97\x53"
    "\xb2\x5b\xd0\x9a\x8e\x78\xce\xe0"
    "\xcf\x29\x47\xac\x27\x02\x71\x20"
    "\x32\xdb\xed\xcf\x6a\xd6\xb1\x5f"
    "\x01\xfc\xe9\xe0\x24\xd9\x79\xd6"
    "\x82\x10\xa4\x77\x20\x0f\x51\x7a"
    "\x6f\x3a\x09\x27\x0e\x56\x10\x06"
    "\x5f\x30\xe9\x62\x4f\xef\x7e\x97"
    "\xee\xf8\x0f\x31\x62\xd5\xa9\x6c"
    "\x30\x5a\xec\x22\x86\xd9\x0d\xd0"
    "\x26\xbc\x3a\x8c\xbc\xa1\x09\xe6"
    "\x10\xf0\x8e\x49\xa7\x0b\x6f\x80"
    "\xfa\xe8\xbc\x40\x98\x36\x63\x4e"
    "\x60\x58\xdd\x97\x0a\x9e\x2c", 135);

  vector_shake(H,
    "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
    "ijklmnopjklmnopqklmnopqrlmnopqrsmnop", 100,
    "\xd6\xc7\x3c\x8c\x87\x2a\x48\x4d"
    "\x88\xca\x62\x5e\x3d\xf8\xf9\xf6"
    "\x0f\xa8\xfb\xaa\x3c\x6b\x3f\x49"
    "\xfb\x77\xb8\x4f\x8d\x56\xf1\x23"
    "\x3a\x7b\xf1\x91\x1f\x97\x21\xf6"
    "\x39\x57\x44\xaa\xf1\xfe\xf9\x8b"
    "\xcc\x72\xf2\x28\x8f\x60\x9a\x19"
    "\xed\x8b\x96\x3b\xce\x7d\x83\xde"
    "\x86\x7b\x86\x24\x5a\x62\xe6\x11"
    "\x73\xa0\x04\x75\xf8\x7c\x8b\x37"
    "\x50\x49\x72\x7a\x0f\x26\x75\xf9"
    "\x69\xb3\x93\xbd\xcb\x81\x3a\x58"
    "\x49\x55\xae\x47\x6a\x71\x8c\xc1"
    "\x26\xce\xc9\xbd\x87\x38\x25\xdf"
    "\xa6\x03\x5d\x36\x51\xad\x24\x93"
    "\xfd\x75\x86\xc3\x88\xad\xc8\xfd"
    "\x2e\x09\xf3\x08\x4b\x64\x39\xa1"
    "\x29\x7d\x68\x24\xbf\x1c\xc2\x3e"
    "\x7a\xfe\x19\x67\xa8\x60\x6a\xdc"
    "\x6d\x94\x48\xd6\xeb\x11\x6a\xb3"
    "\x31\x49\xfe\x94\xd8\x37\x07\x19"
    "\x52\xdc", 170);

  vector_shake_abc_final(H, cf_shake_128_digest_final,
    "\x58\x81\x09\x2d\xd8\x18\xbf\x5c"
    "\xf8\xa3\xdd\xb7\x93\xfb\xcb\xa7"
    "\x40\x97\xd5\xc5\x26\xa6\xd3\x5f"
    "\x97\xb8\x33\x51\x94\x0f\x2c\xc8", 32);

  /* test vector from CAVP */
  test_one_shot_shake(H,
    "\xe8\xdd\x21\x5f\x31\x07\xd4\xf2"
    "\xb7\xfc\xa3\xba\x03\x6f\x86\x9d", 16,
    "\xb7\x27\xd3\xdb\x9e\xdf\x07\x2f"
    "\xf8\x7a\x1e\x69\x20\xab\xce\x8c"
    "\x66\xc3\x56\xf2", 20);
}

/* unless otherwise specified, reference outputs were generated online */
static void test_shake_256(void)
{
  const cf_cshake *H = &cf_shake_256;

  vector_shake(H, "", 0,
    "\x46\xb9\xdd\x2b\x0b\xa8\x8d\x13"
    "\x23\x3b\x3f\xeb\x74\x3e\xeb\x24"
    "\x3f\xcd\x52\xea\x62\xb8\x1b\x82"
    "\xb5\x0c\x27\x64\x6e\xd5\x76\x2f"
    "\xd7\x5d\xc4\xdd\xd8\xc0\xf2\x00"
    "\xcb", 41);

  vector_shake(H, "abc", 3,
    "\x48\x33\x66\x60\x13\x60\xa8\x77"
    "\x1c\x68\x63\x08", 12);

  vector_shake(H, "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
    "\x4d\x8c\x2d\xd2\x43\x5a\x01\x28"
    "\xee\xfb\xb8\xc3\x6f\x6f\x87\x13"
    "\x3a\x79\x11\xe1\x8d\x97\x9e\xe1"
    "\xae\x6b\xe5\xd4\xfd\x2e\x33\x29"
    "\x40\xd8\x68\x8a\x4e\x6a\x59\xaa"
    "\x80\x60\xf1\xf9\xbc\x99\x6c\x05"
    "\xac\xa3\xc6\x96\xa8\xb6\x62\x79"
    "\xdc\x67\x2c\x74\x0b\xb2\x24\xec"
    "\x37\xa9\x2b\x65\xdb\x05\x39", 71);

vector_shake(H,
    "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
    "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 112,
    "\x98\xbe\x04\x51\x6c\x04\xcc\x73"
    "\x59\x3f\xef\x3e\xd0\x35\x2e\xa9"
    "\xf6\x44\x39\x42\xd6\x95\x0e\x29"
    "\xa3\x72\xa6\x81\xc3\xde\xaf\x45"
    "\x35\x42\x37\x09\xb0\x28\x43\x94"
    "\x86\x84\xe0\x29\x01\x0b\xad\xcc"
    "\x0a\xcd\x83\x03\xfc\x85\xfd\xad"
    "\x3e\xab\xf4\xf7\x8c\xae\x16\x56"
    "\x35\xf5\x7a\xfd\x28\x81\x0f\xc2"
    "\x2a\xbf\x63\xdf\x55\xc5\xea\xd4"
    "\x50\xfd\xfb\x64\x20\x90\x10\xe9"
    "\x82\x10\x2a\xa0\xb5\xf0\xa4\xb4"
    "\x75\x3b\x53\xeb\x4b\x53\x19\xc0"
    "\x69\x86\xf5\xaa\xc5\xcc\x24\x72"
    "\x56\xd0\x6b\x05\xa2\x73\xd7\xef"
    "\x8d\x31\x86\x47\x77\xd4\x88\xd5", 128);

vector_shake(H,
    "erguihqprgbivvsdvslvjw;oer93askcavcndiqp,gwgp[f;dfnweufjcsncapqq"
    "erigeornbqe;nq;23742rge8238e7vvnqwwqqwqoro93nmpp[];;;9327v2v8511"
    "aaaaaaaaabbbbbbbbbbbbbbdddddddddbbbbbvvvvvvvvvssssssssssssuuuuuu"
    "ggggggggdddddddddddffffffffffffseeeeeeeeeeeeewwwwwwwwwwwiiiiiiii"
    "tttttttttpppppppkkkkkkkkkkllllllpouetgdcknqgwueggghghow;oeoeoewq"
    "rrrrrrrrrrrrrrrr", 336,
    "\xac\x23\x34\xc3\x21\xf7\x6b\xf1"
    "\x64\x2c\x39\xa6\xd2\x74\xc1\x0e"
    "\xde\x4f\xfb\x40\xe3\x9f\x56\x9d"
    "\xe1\xb1\x1d\x68\x0d\x6b\xe9\x23"
    "\xbc\x68\x2d\xba\x43\x30\x6f\xa3"
    "\xec\xcc\xa4\x6a\xa9\x7e\x4a\x7e"
    "\xf7\x4d\xdf\x58\x5c\x80\x13\x98"
    "\x1d\x63\x9a\x2c\x87\x57\xb3\x24"
    "\x0a\x42\x13\x6e\xb8\xd6\x71\xd9"
    "\x75\xe4\x46\x5b\x01\x4d\x76\x93"
    "\x5f\x81\xd4\x57\xc7\xeb\xa1\x0c"
    "\x28\x37\x33\x46\x7a\x99\x49\xb8"
    "\x24\xa2\x08\xfd\x2f\xe8\xac\xd0"
    "\x4c\xc2\x60\x1a\xd3\x92\x72\x69"
    "\x85\x42\x97\x40\x4a\xf2\x14\x7b"
    "\xca\x38\xdf\x7a\x7a\x88\x0e\xbb"
    "\xc4\x80\xbc\x72\x8a\x94\x94\xba"
    "\x14\xf0\x48\x1b\x36\xdd\x67\xf5"
    "\xeb\x9b\x19\x83\x4f\xd3", 150);

  vector_shake_abc_final(H, cf_shake_256_digest_final,
    "\x48\x33\x66\x60\x13\x60\xa8\x77"
    "\x1c\x68\x63\x08\x0c\xc4\x11\x4d"
    "\x8d\xb4\x45\x30\xf8\xf1\xe1\xee"
    "\x4f\x94\xea\x37\xe7\x8b\x57\x39", 32);

  /* test vector from CAVP */
  test_one_shot_shake(H,
    "\x17\xe7\xdf\xab\x5f\x6d\x76\xaf"
    "\x3c\x5c\x58\x42\x18\x48\x55\x43"
    "\x67\xa2\xad\x46\x7f\x3a\x81\x36"
    "\xaa\x03\x88\x3f\x7a\x16\x03\xea", 32,
    "\xf4\xef\x4a\xda\x87", 5);
}

TEST_LIST = {
  { "shake-128", test_shake_128 },
  { "shake-256", test_shake_256 },
  { 0 }
};
