/*	$OpenBSD: md5.c,v 1.2 2004/04/28 16:52:08 millert Exp $	*/

/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.	This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static const char rcsid[] = "$OpenBSD: md5.c,v 1.2 2004/04/28 16:52:08 millert Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <string.h>
#include <md5.h>

#if BYTE_ORDER == LITTLE_ENDIAN

#define htole32_4(buf)		/* Nothing */
#define htole32_14(buf)		/* Nothing */
#define htole32_16(buf)		/* Nothing */

#else

#define htole32_4(buf) do {						\
	(buf)[ 0] = htole32((buf)[ 0]);					\
	(buf)[ 1] = htole32((buf)[ 1]);					\
	(buf)[ 2] = htole32((buf)[ 2]);					\
	(buf)[ 3] = htole32((buf)[ 3]);					\
} while (0)

#define htole32_14(buf) do {						\
	(buf)[ 0] = htole32((buf)[ 0]);					\
	(buf)[ 1] = htole32((buf)[ 1]);					\
	(buf)[ 2] = htole32((buf)[ 2]);					\
	(buf)[ 3] = htole32((buf)[ 3]);					\
	(buf)[ 4] = htole32((buf)[ 4]);					\
	(buf)[ 5] = htole32((buf)[ 5]);					\
	(buf)[ 6] = htole32((buf)[ 6]);					\
	(buf)[ 7] = htole32((buf)[ 7]);					\
	(buf)[ 8] = htole32((buf)[ 8]);					\
	(buf)[ 9] = htole32((buf)[ 9]);					\
	(buf)[10] = htole32((buf)[10]);					\
	(buf)[11] = htole32((buf)[11]);					\
	(buf)[12] = htole32((buf)[12]);					\
	(buf)[13] = htole32((buf)[13]);					\
} while (0)

#define htole32_16(buf) do {						\
	(buf)[ 0] = htole32((buf)[ 0]);					\
	(buf)[ 1] = htole32((buf)[ 1]);					\
	(buf)[ 2] = htole32((buf)[ 2]);					\
	(buf)[ 3] = htole32((buf)[ 3]);					\
	(buf)[ 4] = htole32((buf)[ 4]);					\
	(buf)[ 5] = htole32((buf)[ 5]);					\
	(buf)[ 6] = htole32((buf)[ 6]);					\
	(buf)[ 7] = htole32((buf)[ 7]);					\
	(buf)[ 8] = htole32((buf)[ 8]);					\
	(buf)[ 9] = htole32((buf)[ 9]);					\
	(buf)[10] = htole32((buf)[10]);					\
	(buf)[11] = htole32((buf)[11]);					\
	(buf)[12] = htole32((buf)[12]);					\
	(buf)[13] = htole32((buf)[13]);					\
	(buf)[14] = htole32((buf)[14]);					\
	(buf)[15] = htole32((buf)[15]);					\
} while (0)

#endif

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void
MD5Init(MD5_CTX *ctx)
{
	ctx->count = 0;
	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xefcdab89;
	ctx->state[2] = 0x98badcfe;
	ctx->state[3] = 0x10325476;
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void
MD5Update(MD5_CTX *ctx, const unsigned char *buf, size_t len)
{
	u_int32_t count;

	/* Bytes already stored in ctx->buffer */
	count = (u_int32_t)((ctx->count >> 3) & 0x3f);

	/* Update bitcount */
	ctx->count += (u_int64_t)len << 3;

	/* Handle any leading odd-sized chunks */
	if (count) {
		unsigned char *p = (unsigned char *)ctx->buffer + count;

		count = MD5_BLOCK_LENGTH - count;
		if (len < count) {
			memcpy(p, buf, len);
			return;
		}
		memcpy(p, buf, count);
		htole32_16((u_int32_t *)ctx->buffer);
		MD5Transform(ctx->state, ctx->buffer);
		buf += count;
		len -= count;
	}

	/* Process data in MD5_BLOCK_LENGTH-byte chunks */
	while (len >= MD5_BLOCK_LENGTH) {
		memcpy(ctx->buffer, buf, MD5_BLOCK_LENGTH);
		htole32_16((u_int32_t *)ctx->buffer);
		MD5Transform(ctx->state, ctx->buffer);
		buf += MD5_BLOCK_LENGTH;
		len -= MD5_BLOCK_LENGTH;
	}

	/* Handle any remaining bytes of data. */
	memcpy(ctx->buffer, buf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern 
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void
MD5Final(unsigned char digest[MD5_DIGEST_LENGTH], MD5_CTX *ctx)
{
	u_int32_t count;
	unsigned char *p;

	/* number of bytes mod 64 */
	count = (u_int32_t)(ctx->count >> 3) & 0x3f;

	/*
	 * Set the first char of padding to 0x80.
	 * This is safe since there is always at least one byte free.
	 */
	p = ctx->buffer + count;
	*p++ = 0x80;

	/* Bytes of padding needed to make 64 bytes */
	count = 64 - 1 - count;

	/* Pad out to 56 mod 64 */
	if (count < 8) {
		/* Two lots of padding:  Pad the first block to 64 bytes */
		memset(p, 0, count);
		htole32_16((u_int32_t *)ctx->buffer);
		MD5Transform(ctx->state, ctx->buffer);

		/* Now fill the next block with 56 bytes */
		memset(ctx->buffer, 0, 56);
	} else {
		/* Pad block to 56 bytes */
		memset(p, 0, count - 8);
	}
	htole32_14((u_int32_t *)ctx->buffer);

	/* Append bit count and transform */
	((u_int32_t *)ctx->buffer)[14] = ctx->count & 0xffffffff;
	((u_int32_t *)ctx->buffer)[15] = (u_int32_t)(ctx->count >> 32);

	MD5Transform(ctx->state, ctx->buffer);
	htole32_4(ctx->state);
	memcpy(digest, ctx->state, MD5_DIGEST_LENGTH);
	memset(ctx, 0, sizeof(ctx));        /* In case it's sensitive */
}


/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
void
MD5Transform(u_int32_t buf[4], const unsigned char inc[MD5_BLOCK_LENGTH])
{
	u_int32_t a, b, c, d;
	const u_int32_t *in = (const u_int32_t *)inc;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	MD5STEP(F1, a, b, c, d, in[ 0] + 0xd76aa478,  7);
	MD5STEP(F1, d, a, b, c, in[ 1] + 0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, in[ 2] + 0x242070db, 17);
	MD5STEP(F1, b, c, d, a, in[ 3] + 0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, in[ 4] + 0xf57c0faf,  7);
	MD5STEP(F1, d, a, b, c, in[ 5] + 0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, in[ 6] + 0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, in[ 7] + 0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, in[ 8] + 0x698098d8,  7);
	MD5STEP(F1, d, a, b, c, in[ 9] + 0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122,  7);
	MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, in[ 1] + 0xf61e2562,  5);
	MD5STEP(F2, d, a, b, c, in[ 6] + 0xc040b340,  9);
	MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, in[ 0] + 0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, in[ 5] + 0xd62f105d,  5);
	MD5STEP(F2, d, a, b, c, in[10] + 0x02441453,  9);
	MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, in[ 4] + 0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, in[ 9] + 0x21e1cde6,  5);
	MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6,  9);
	MD5STEP(F2, c, d, a, b, in[ 3] + 0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, in[ 8] + 0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905,  5);
	MD5STEP(F2, d, a, b, c, in[ 2] + 0xfcefa3f8,  9);
	MD5STEP(F2, c, d, a, b, in[ 7] + 0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, in[ 5] + 0xfffa3942,  4);
	MD5STEP(F3, d, a, b, c, in[ 8] + 0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, in[ 1] + 0xa4beea44,  4);
	MD5STEP(F3, d, a, b, c, in[ 4] + 0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, in[ 7] + 0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6,  4);
	MD5STEP(F3, d, a, b, c, in[ 0] + 0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, in[ 3] + 0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, in[ 6] + 0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, in[ 9] + 0xd9d4d039,  4);
	MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, in[2 ] + 0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, in[ 0] + 0xf4292244,  6);
	MD5STEP(F4, d, a, b, c, in[7 ] + 0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, in[5 ] + 0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3,  6);
	MD5STEP(F4, d, a, b, c, in[3 ] + 0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, in[1 ] + 0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, in[8 ] + 0x6fa87e4f,  6);
	MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, in[6 ] + 0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, in[4 ] + 0xf7537e82,  6);
	MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, in[2 ] + 0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, in[9 ] + 0xeb86d391, 21);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}
