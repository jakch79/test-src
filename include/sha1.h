/*	$OpenBSD: sha1.h,v 1.18 2004/04/27 15:54:56 millert Exp $	*/

/*
 * SHA-1 in C
 * By Steve Reid <steve@edmweb.com>
 * 100% Public Domain
 */

#ifndef _SHA1_H
#define _SHA1_H

#define	SHA1_BLOCK_LENGTH		64
#define	SHA1_DIGEST_LENGTH		20
#define	SHA1_DIGEST_STRING_LENGTH	(SHA1_DIGEST_LENGTH * 2 + 1)

typedef struct {
    u_int32_t state[5];
    u_int64_t count;
    u_int8_t buffer[SHA1_BLOCK_LENGTH];
} SHA1_CTX;

#include <sys/cdefs.h>

__BEGIN_DECLS
void SHA1Init(SHA1_CTX *);
void SHA1Transform(u_int32_t [5], const u_int8_t [SHA1_BLOCK_LENGTH])
		__attribute__((__bounded__(__minbytes__,1,5)))
		__attribute__((__bounded__(__minbytes__,2,SHA1_BLOCK_LENGTH)));
void SHA1Update(SHA1_CTX *, const u_int8_t *, unsigned int)
		__attribute__((__bounded__(__string__,2,3)));
void SHA1Final(u_int8_t [SHA1_DIGEST_LENGTH], SHA1_CTX *)
		__attribute__((__bounded__(__minbytes__,1,SHA1_DIGEST_LENGTH)));
char *SHA1End(SHA1_CTX *, char [SHA1_DIGEST_STRING_LENGTH])
		__attribute__((__bounded__(__minbytes__,2,SHA1_DIGEST_STRING_LENGTH)));
char *SHA1File(char *, char [SHA1_DIGEST_STRING_LENGTH])
		__attribute__((__bounded__(__minbytes__,2,SHA1_DIGEST_STRING_LENGTH)));
char *SHA1Data(const u_int8_t *, size_t, char [SHA1_DIGEST_STRING_LENGTH])
		__attribute__((__bounded__(__string__,1,2)))
		__attribute__((__bounded__(__minbytes__,3,SHA1_DIGEST_STRING_LENGTH)));
__END_DECLS

#define HTONDIGEST(x) do {                                              \
        x[0] = htonl(x[0]);                                             \
        x[1] = htonl(x[1]);                                             \
        x[2] = htonl(x[2]);                                             \
        x[3] = htonl(x[3]);                                             \
        x[4] = htonl(x[4]); } while (0)

#define NTOHDIGEST(x) do {                                              \
        x[0] = ntohl(x[0]);                                             \
        x[1] = ntohl(x[1]);                                             \
        x[2] = ntohl(x[2]);                                             \
        x[3] = ntohl(x[3]);                                             \
        x[4] = ntohl(x[4]); } while (0)

#endif /* _SHA1_H */
