/*	$OpenBSD: acss_enc.c,v 1.3 2004/01/23 19:23:33 hshoexer Exp $	*/
/*
 * Copyright (c) 2004 The OpenBSD project
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <openssl/acss.h>

/* decryption sbox */
static unsigned char sboxdec[] = {
	0x33, 0x73, 0x3b, 0x26, 0x63, 0x23, 0x6b, 0x76, 
	0x3e, 0x7e, 0x36, 0x2b, 0x6e, 0x2e, 0x66, 0x7b, 
	0xd3, 0x93, 0xdb, 0x06, 0x43, 0x03, 0x4b, 0x96, 
	0xde, 0x9e, 0xd6, 0x0b, 0x4e, 0x0e, 0x46, 0x9b, 
	0x57, 0x17, 0x5f, 0x82, 0xc7, 0x87, 0xcf, 0x12, 
	0x5a, 0x1a, 0x52, 0x8f, 0xca, 0x8a, 0xc2, 0x1f, 
	0xd9, 0x99, 0xd1, 0x00, 0x49, 0x09, 0x41, 0x90, 
	0xd8, 0x98, 0xd0, 0x01, 0x48, 0x08, 0x40, 0x91, 
	0x3d, 0x7d, 0x35, 0x24, 0x6d, 0x2d, 0x65, 0x74, 
	0x3c, 0x7c, 0x34, 0x25, 0x6c, 0x2c, 0x64, 0x75, 
	0xdd, 0x9d, 0xd5, 0x04, 0x4d, 0x0d, 0x45, 0x94, 
	0xdc, 0x9c, 0xd4, 0x05, 0x4c, 0x0c, 0x44, 0x95, 
	0x59, 0x19, 0x51, 0x80, 0xc9, 0x89, 0xc1, 0x10, 
	0x58, 0x18, 0x50, 0x81, 0xc8, 0x88, 0xc0, 0x11, 
	0xd7, 0x97, 0xdf, 0x02, 0x47, 0x07, 0x4f, 0x92, 
	0xda, 0x9a, 0xd2, 0x0f, 0x4a, 0x0a, 0x42, 0x9f, 
	0x53, 0x13, 0x5b, 0x86, 0xc3, 0x83, 0xcb, 0x16, 
	0x5e, 0x1e, 0x56, 0x8b, 0xce, 0x8e, 0xc6, 0x1b, 
	0xb3, 0xf3, 0xbb, 0xa6, 0xe3, 0xa3, 0xeb, 0xf6, 
	0xbe, 0xfe, 0xb6, 0xab, 0xee, 0xae, 0xe6, 0xfb, 
	0x37, 0x77, 0x3f, 0x22, 0x67, 0x27, 0x6f, 0x72, 
	0x3a, 0x7a, 0x32, 0x2f, 0x6a, 0x2a, 0x62, 0x7f, 
	0xb9, 0xf9, 0xb1, 0xa0, 0xe9, 0xa9, 0xe1, 0xf0, 
	0xb8, 0xf8, 0xb0, 0xa1, 0xe8, 0xa8, 0xe0, 0xf1, 
	0x5d, 0x1d, 0x55, 0x84, 0xcd, 0x8d, 0xc5, 0x14, 
	0x5c, 0x1c, 0x54, 0x85, 0xcc, 0x8c, 0xc4, 0x15, 
	0xbd, 0xfd, 0xb5, 0xa4, 0xed, 0xad, 0xe5, 0xf4, 
	0xbc, 0xfc, 0xb4, 0xa5, 0xec, 0xac, 0xe4, 0xf5, 
	0x39, 0x79, 0x31, 0x20, 0x69, 0x29, 0x61, 0x70, 
	0x38, 0x78, 0x30, 0x21, 0x68, 0x28, 0x60, 0x71, 
	0xb7, 0xf7, 0xbf, 0xa2, 0xe7, 0xa7, 0xef, 0xf2, 
	0xba, 0xfa, 0xb2, 0xaf, 0xea, 0xaa, 0xe2, 0xff
};

/* encryption sbox */
static unsigned char sboxenc[] = {
	0x33, 0x3b, 0x73, 0x15, 0x53, 0x5b, 0x13, 0x75,
	0x3d, 0x35, 0x7d, 0x1b, 0x5d, 0x55, 0x1d, 0x7b,
	0x67, 0x6f, 0x27, 0x81, 0xc7, 0xcf, 0x87, 0x21,
	0x69, 0x61, 0x29, 0x8f, 0xc9, 0xc1, 0x89, 0x2f,
	0xe3, 0xeb, 0xa3, 0x05, 0x43, 0x4b, 0x03, 0xa5,
	0xed, 0xe5, 0xad, 0x0b, 0x4d, 0x45, 0x0d, 0xab,
	0xea, 0xe2, 0xaa, 0x00, 0x4a, 0x42, 0x0a, 0xa0,
	0xe8, 0xe0, 0xa8, 0x02, 0x48, 0x40, 0x08, 0xa2,
	0x3e, 0x36, 0x7e, 0x14, 0x5e, 0x56, 0x1e, 0x74,
	0x3c, 0x34, 0x7c, 0x16, 0x5c, 0x54, 0x1c, 0x76,
	0x6a, 0x62, 0x2a, 0x80, 0xca, 0xc2, 0x8a, 0x20,
	0x68, 0x60, 0x28, 0x82, 0xc8, 0xc0, 0x88, 0x22,
	0xee, 0xe6, 0xae, 0x04, 0x4e, 0x46, 0x0e, 0xa4,
	0xec, 0xe4, 0xac, 0x06, 0x4c, 0x44, 0x0c, 0xa6,
	0xe7, 0xef, 0xa7, 0x01, 0x47, 0x4f, 0x07, 0xa1,
	0xe9, 0xe1, 0xa9, 0x0f, 0x49, 0x41, 0x09, 0xaf,
	0x63, 0x6b, 0x23, 0x85, 0xc3, 0xcb, 0x83, 0x25,
	0x6d, 0x65, 0x2d, 0x8b, 0xcd, 0xc5, 0x8d, 0x2b,
	0x37, 0x3f, 0x77, 0x11, 0x57, 0x5f, 0x17, 0x71,
	0x39, 0x31, 0x79, 0x1f, 0x59, 0x51, 0x19, 0x7f,
	0xb3, 0xbb, 0xf3, 0x95, 0xd3, 0xdb, 0x93, 0xf5,
	0xbd, 0xb5, 0xfd, 0x9b, 0xdd, 0xd5, 0x9d, 0xfb,
	0xba, 0xb2, 0xfa, 0x90, 0xda, 0xd2, 0x9a, 0xf0,
	0xb8, 0xb0, 0xf8, 0x92, 0xd8, 0xd0, 0x98, 0xf2,
	0x6e, 0x66, 0x2e, 0x84, 0xce, 0xc6, 0x8e, 0x24,
	0x6c, 0x64, 0x2c, 0x86, 0xcc, 0xc4, 0x8c, 0x26,
	0x3a, 0x32, 0x7a, 0x10, 0x5a, 0x52, 0x1a, 0x70,
	0x38, 0x30, 0x78, 0x12, 0x58, 0x50, 0x18, 0x72,
	0xbe, 0xb6, 0xfe, 0x94, 0xde, 0xd6, 0x9e, 0xf4,
	0xbc, 0xb4, 0xfc, 0x96, 0xdc, 0xd4, 0x9c, 0xf6,
	0xb7, 0xbf, 0xf7, 0x91, 0xd7, 0xdf, 0x97, 0xf1,
	0xb9, 0xb1, 0xf9, 0x9f, 0xd9, 0xd1, 0x99, 0xff
};

/*
 * Two linear feedback shift registers are used:
 *
 * lfsr17:  polynomial of degree 17, primitive modulo 2 (listed in Schneier)
 *          x^15 + x + 1
 * lfsr25:  polynomial of degree 25, not know if primitive modulo 2
 *          x^13 + x^5 + x^4 + x^1 + 1
 *
 * Output bits are discarded, instead the feedback bits are added to produce
 * the cipher stream.  Depending on the mode, feedback bytes may be inverted
 * bit-wise before addition.
 *
 * The lfsrs are seeded with bytes from the raw key:
 *
 * lfsr17:  byte 0[0:7] at bit 9
 *          byte 1[0:7] at bit 0
 *
 * lfsr25:  byte 2[0:4] at bit 16
 *          byte 2[5:7] at bit 22
 *          byte 3[0:7] at bit 8
 *          byte 4[0:7] at bit 0
 *
 * To prevent 0 cycles, 1's are inject at bit 8 in lfrs17 and bit 21 in
 * lfsr25.
 *
 */

void
acss(ACSS_KEY *key, unsigned long len, const unsigned char *in,
    unsigned char *out)
{
	unsigned long i;
	unsigned long lfsr17tmp, lfsr25tmp, lfsrsumtmp;

	lfsrsumtmp = lfsr17tmp = lfsr25tmp = 0;

	/* keystream is sum of lfsrs */
	for (i = 0; i < len; i++) {
		lfsr17tmp = key->lfsr17 ^ (key->lfsr17 >> 14);
		key->lfsr17 = (key->lfsr17 >> 8)
			^ (lfsr17tmp << 9)
			^ (lfsr17tmp << 12)
			^ (lfsr17tmp << 15);
		key->lfsr17 &= 0x1ffff;	/* 17 bit LFSR */

		lfsr25tmp = key->lfsr25
			^ (key->lfsr25 >> 3)
			^ (key->lfsr25 >> 4)
			^ (key->lfsr25 >> 12);
		key->lfsr25 = (key->lfsr25 >> 8) ^ (lfsr25tmp << 17);
		key->lfsr25 &= 0x1ffffff;	/* 25 bit LFSR */

		lfsrsumtmp = key->lfsrsum;

		/* addition */
		switch (key->mode) {
		case ACSS_SESSIONKEY:
			key->lfsrsum = key->lfsr17 >> 9;
			key->lfsrsum += key->lfsr25 >> 17;
			break;
		case ACSS_TITLEKEY:
			key->lfsrsum = key->lfsr17 >> 9;
			key->lfsrsum += 0xff & ~(key->lfsr25 >> 17);
			break;
		case ACSS_AUTHENTICATE:
		case ACSS_DATA:
		default:
			key->lfsrsum = 0xff & ~(key->lfsr17 >> 9);
			key->lfsrsum += key->lfsr25 >> 17;
			break;
		}
		key->lfsrsum += (lfsrsumtmp >> 8);

		if (key->encrypt) {
			out[i] = sboxenc[(in[i] ^ key->lfsrsum) & 0xff];
		} else {
			out[i] = (sboxdec[in[i]] ^ key->lfsrsum) & 0xff;
		}
	}
}
