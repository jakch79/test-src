/*	$OpenBSD: rnd.c,v 1.27 1997/06/21 04:59:36 flipk Exp $	*/

/*
 * random.c -- A strong random number generator
 *
 * Copyright (c) 1996 Michael Shalayeff.
 *
 * Version 1.00, last modified 26-May-96
 * 
 * Copyright Theodore Ts'o, 1994, 1995, 1996.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 * 
 * ALTERNATIVELY, this product may be distributed under the terms of
 * the GNU Public License, in which case the provisions of the GPL are
 * required INSTEAD OF the above restrictions.  (This clause is
 * necessary due to a potential bad interaction between the GPL and
 * the restrictions contained in a BSD-style copyright.)
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * (now, with legal B.S. out of the way.....) 
 * 
 * This routine gathers environmental noise from device drivers, etc.,
 * and returns good random numbers, suitable for cryptographic use.
 * Besides the obvious cryptographic uses, these numbers are also good
 * for seeding TCP sequence numbers, and other places where it is
 * desirable to have numbers which are not only random, but hard to
 * predict by an attacker.
 *
 * Theory of operation
 * ===================
 * 
 * Computers are very predictable devices.  Hence it is extremely hard
 * to produce truly random numbers on a computer --- as opposed to
 * pseudo-random numbers, which can easily generated by using a
 * algorithm.  Unfortunately, it is very easy for attackers to guess
 * the sequence of pseudo-random number generators, and for some
 * applications this is not acceptable.  So instead, we must try to
 * gather "environmental noise" from the computer's environment, which
 * must be hard for outside attackers to observe, and use that to
 * generate random numbers.  In a Unix environment, this is best done
 * from inside the kernel.
 * 
 * Sources of randomness from the environment include inter-keyboard
 * timings, inter-interrupt timings from some interrupts, and other
 * events which are both (a) non-deterministic and (b) hard for an
 * outside observer to measure.  Randomness from these sources are
 * added to an "entropy pool", which is mixed using a CRC-like function.
 * This is not cryptographically strong, but it is adequate assuming
 * the randomness is not chosen maliciously, and it is fast enough that
 * the overhead of doing it on every interrupt is very reasonable.
 * As random bytes are mixed into the entropy pool, the routines keep
 * an *estimate* of how many bits of randomness have been stored into
 * the random number generator's internal state.
 * 
 * When random bytes are desired, they are obtained by taking the MD5
 * hash of the contents of the "entropy pool".  The MD5 hash avoids
 * exposing the internal state of the entropy pool.  It is believed to
 * be computationally infeasible to derive any useful information
 * about the input of MD5 from its output.  Even if it is possible to
 * analyze MD5 in some clever way, as long as the amount of data
 * returned from the generator is less than the inherent entropy in
 * the pool, the output data is totally unpredictable.  For this
 * reason, the routine decreases its internal estimate of how many
 * bits of "true randomness" are contained in the entropy pool as it
 * outputs random numbers.
 * 
 * If this estimate goes to zero, the routine can still generate
 * random numbers; however, an attacker may (at least in theory) be
 * able to infer the future output of the generator from prior
 * outputs.  This requires successful cryptanalysis of MD5, which is
 * not believed to be feasible, but there is a remote possibility.
 * Nonetheless, these numbers should be useful for the vast majority
 * of purposes.
 * 
 * Exported interfaces ---- output
 * ===============================
 * 
 * There are three exported interfaces; the first is one designed to
 * be used from within the kernel:
 *
 * 	void get_random_bytes(void *buf, int nbytes);
 *
 * This interface will return the requested number of random bytes,
 * and place it in the requested buffer.
 * 
 * The two other interfaces are two character devices /dev/random and
 * /dev/urandom.  /dev/random is suitable for use when very high
 * quality randomness is desired (for example, for key generation or
 * one-time pads), as it will only return a maximum of the number of
 * bits of randomness (as estimated by the random number generator)
 * contained in the entropy pool.
 * 
 * The /dev/urandom device does not have this limit, and will return
 * as many bytes as are requested.  As more and more random bytes are
 * requested without giving time for the entropy pool to recharge,
 * this will result in random numbers that are merely cryptographically
 * strong.  For many applications, however, this is acceptable.
 *
 * Exported interfaces ---- input
 * ==============================
 * 
 * The current exported interfaces for gathering environmental noise
 * from the devices are:
 * 
 * 	void add_mouse_randomness(u_int32_t mouse_data);
 * 	void add_net_randomness(int isr);
 *	void add_tty_randomness(int c);
 * 	void add_disk_randomness(u_int32_t n);
 * 
 * add_mouse_randomness() uses the mouse interrupt timing, as well as
 * the reported position of the mouse from the hardware.
 *
 * add_net_randomness() times the finishing time of net input.
 * 
 * add_tty_randomness() uses the inter-keypress timing, as well as the
 * character as random inputs into the "entropy pool".
 * 
 * add_disk_randomness() times the finishing time of disk requests as well
 * as feeding both xfer size & time into the entropy pool.
 * 
 * All of these routines try to estimate how many bits of randomness a
 * particular randomness source.  They do this by keeping track of the
 * first and second order deltas of the event timings.
 *
 * Ensuring unpredictability at system startup
 * ============================================
 * 
 * When any operating system starts up, it will go through a sequence
 * of actions that are fairly predictable by an adversary, especially
 * if the start-up does not involve interaction with a human operator.
 * This reduces the actual number of bits of unpredictability in the
 * entropy pool below the value in entropy_count.  In order to
 * counteract this effect, it helps to carry information in the
 * entropy pool across shut-downs and start-ups.  To do this, put the
 * following lines an appropriate script which is run during the boot
 * sequence: 
 *
 *	echo "Initializing random number generator..."
 *	# Carry a random seed from start-up to start-up
 *	# Load and then save 512 bytes, which is the size of the entropy pool
 * 	if [ -f /etc/random-seed ]; then
 *		cat /etc/random-seed >/dev/urandom
 * 	fi
 *	dd if=/dev/urandom of=/etc/random-seed count=1
 *
 * and the following lines in an appropriate script which is run as
 * the system is shutdown:
 * 
 *	# Carry a random seed from shut-down to start-up
 *	# Save 512 bytes, which is the size of the entropy pool
 *	echo "Saving random seed..."
 *	dd if=/dev/urandom of=/etc/random-seed count=1
 * 
 * For example, on many Linux systems, the appropriate scripts are
 * usually /etc/rc.d/rc.local and /etc/rc.d/rc.0, respectively.
 * 
 * Effectively, these commands cause the contents of the entropy pool
 * to be saved at shut-down time and reloaded into the entropy pool at
 * start-up.  (The 'dd' in the addition to the bootup script is to
 * make sure that /etc/random-seed is different for every start-up,
 * even if the system crashes without executing rc.0.)  Even with
 * complete knowledge of the start-up activities, predicting the state
 * of the entropy pool requires knowledge of the previous history of
 * the system.
 *
 * Configuring the /dev/random driver under Linux
 * ==============================================
 *
 * The /dev/random driver under Linux uses minor numbers 8 and 9 of
 * the /dev/mem major number (#1).  So if your system does not have
 * /dev/random and /dev/urandom created already, they can be created
 * by using the commands:
 *
 * 	mknod /dev/random c 1 8
 * 	mknod /dev/urandom c 1 9
 * 
 * Acknowledgements:
 * =================
 *
 * Ideas for constructing this random number generator were derived
 * from the Pretty Good Privacy's random number generator, and from
 * private discussions with Phil Karn.  Colin Plumb provided a faster
 * random number generator, which speed up the mixing function of the
 * entropy pool, taken from PGP 3.0 (under development).  It has since
 * been modified by myself to provide better mixing in the case where
 * the input values to add_entropy_word() are mostly small numbers.
 * Dale Worley has also contributed many useful ideas and suggestions
 * to improve this driver.
 * 
 * Any flaws in the design are solely my responsibility, and should
 * not be attributed to the Phil, Colin, or any of authors of PGP.
 * 
 * The code for MD5 transform was taken from Colin Plumb's
 * implementation, which has been placed in the public domain.  The
 * MD5 cryptographic checksum was devised by Ronald Rivest, and is
 * documented in RFC 1321, "The MD5 Message Digest Algorithm".
 * 
 * Further background information on this topic may be obtained from
 * RFC 1750, "Randomness Recommendations for Security", by Donald
 * Eastlake, Steve Crocker, and Jeff Schiller.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/md5k.h>

#include <net/netisr.h>

#include <dev/rndvar.h>
#include <dev/rndioctl.h>

#ifdef	DEBUG
int	rnd_debug = 0x0000;
#define	RD_INPUT	0x000f	/* input data */
#define	RD_OUTPUT	0x00f0	/* output data */
#define	RD_WAIT		0x0100	/* sleep/wakeup for good data */
#endif

/*
 * The pool is stirred with a primitive polynomial of degree 128
 * over GF(2), namely x^128 + x^99 + x^59 + x^31 + x^9 + x^7 + 1.
 * For a pool of size 64, try x^64+x^62+x^38+x^10+x^6+x+1.
 */
#define POOLBITS (POOLWORDS*32)
#if POOLWORDS == 128
#define TAP1    99     /* The polynomial taps */
#define TAP2    59
#define TAP3    31
#define TAP4    9
#define TAP5    7
#elif POOLWORDS == 64
#define TAP1    62      /* The polynomial taps */
#define TAP2    38
#define TAP3    10
#define TAP4    6
#define TAP5    1
#else
#error No primitive polynomial available for chosen POOLWORDS
#endif

/* There is actually only one of these, globally. */
struct random_bucket {
	u_int	add_ptr;
	u_int	entropy_count;
	int	input_rotate;
	u_int32_t *pool;
};

/* There is one of these per entropy source */
struct timer_rand_state {
	u_long	last_time;
	int	last_delta;
	int	dont_count_entropy:1;
};

struct arc4_stream {
	u_char	i;
	u_char	j;
	u_char	s[256];
};

/* tags for different random sources */
#define	ENT_NET		0x100
#define	ENT_DISK	0x200
#define ENT_TTY		0x300

static struct random_bucket random_state;
static int arc4random_uninitialized = 2;
static struct arc4_stream arc4random_state;
static u_int32_t random_pool[POOLWORDS];
static struct timer_rand_state mouse_timer_state;
static struct timer_rand_state extract_timer_state;
static struct timer_rand_state disk_timer_state;
static struct timer_rand_state net_timer_state;
static struct timer_rand_state tty_timer_state;
static int rnd_sleep = 0;

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

static __inline void add_entropy_word __P((struct random_bucket *,
	    const u_int32_t));
void	add_timer_randomness __P((struct random_bucket *,
	    struct timer_rand_state *, u_int));
static __inline int extract_entropy __P((struct random_bucket *, char *, int));
void	arc4_init __P((struct arc4_stream *, u_char *, int));
static __inline void arc4_stir (struct arc4_stream *);
static __inline u_char arc4_getbyte __P((struct arc4_stream *));

/* Arcfour random stream generator.  This code is derived from section
 * 17.1 of Applied Cryptography, second edition, which describes a
 * stream cipher allegedly compatible with RSA Labs "RC4" cipher (the
 * actual description of which is a trade secret).  The same algorithm
 * is used as a stream cipher called "arcfour" in Tatu Ylonen's ssh
 * package.
 *
 * The initialization function here has been modified not to discard
 * old state, and its input always includes the time of day in
 * microseconds.  Moreover, bytes from the stream may at any point be
 * diverted to multiple processes or even kernel functions desiring
 * random numbers.  This increases the strenght of the random stream,
 * but makes it impossible to use this code for encryption--There is
 * no way ever to reproduce the same stream of random bytes.
 *
 * RC4 is a registered trademark of RSA Laboratories.
 */

void
arc4_init (struct arc4_stream *as, u_char *data, int len)
{
	int n;
	u_char si;

	as->i--;
	for (n = 0; n < 256; n++) {
		as->i = (as->i + 1) & 0xff;
		si = as->s[as->i];
		as->j = (as->j + si + data[n % len]) & 0xff;
		as->s[as->i] = as->s[as->j];
		as->s[as->j] = si;
	}
}

static __inline u_char
arc4_getbyte (struct arc4_stream *as)
{
	u_char si, sj;

	as->i = (as->i + 1) & 0xff;
	si = as->s[as->i];
	as->j = (as->j + si) & 0xff;
	sj = as->s[as->j];
	as->s[as->i] = sj;
	as->s[as->j] = si;
	return (as->s[(si + sj) & 0xff]);
}

static inline void
arc4maybeinit (void)
{
  if (arc4random_uninitialized) {
    if (arc4random_uninitialized > 1
	|| random_state.entropy_count >= 128) {
      arc4random_uninitialized--;
      arc4_stir (&arc4random_state);
    }
  }
}

u_int32_t
arc4random (void)
{
  arc4maybeinit ();
  return ((arc4_getbyte (&arc4random_state) << 24)
	  | (arc4_getbyte (&arc4random_state) << 16)
	  | (arc4_getbyte (&arc4random_state) << 8)
	  | arc4_getbyte (&arc4random_state));
}

void
randomattach(void)
{
	int i;
	struct timeval tv;

	random_state.add_ptr = 0;
	random_state.entropy_count = 0;
	random_state.pool = random_pool;
	extract_timer_state.dont_count_entropy = 1;

	for (i = 0; i < 256; i++)
		arc4random_state.s[i] = i;
	microtime (&tv);
	arc4_init (&arc4random_state, (u_char *) &tv, sizeof (tv));
}

int
randomopen(dev, flag, mode, p)
	dev_t	dev;
	int	flag;
	int	mode;
	struct proc *p;
{
	return (minor (dev) < RND_NODEV) ? 0 : ENXIO;
}

int
randomclose(dev, flag, mode, p)
	dev_t	dev;
	int	flag;
	int	mode;
	struct proc *p;
{
	return 0;
}

/*
 * This function adds a byte into the entropy "pool".  It does not
 * update the entropy estimate.  The caller must do this if appropriate.
 *
 * The pool is stirred with a primitive polynomial of degree 128
 * over GF(2), namely x^128 + x^99 + x^59 + x^31 + x^9 + x^7 + 1.
 * For a pool of size 64, try x^64+x^62+x^38+x^10+x^6+x+1.
 * 
 * We rotate the input word by a changing number of bits, to help
 * assure that all bits in the entropy get toggled.  Otherwise, if we
 * consistently feed the entropy pool small numbers (like jiffies and
 * scancodes, for example), the upper bits of the entropy pool don't
 * get affected. --- TYT, 10/11/95
 */
static __inline void
add_entropy_word(r, input)
	struct random_bucket	*r;
	const u_int32_t		input;
{
	u_int		i;
	u_int32_t	w;

	w = (input << r->input_rotate) | (input >> (32 - r->input_rotate));
	i = r->add_ptr = (r->add_ptr - 1) & (POOLWORDS-1);
	if (i)
		r->input_rotate = (r->input_rotate + 7) & 31;
	else
		/*
		 * At the beginning of the pool, add an extra 7 bits
		 * rotation, so that successive passes spread the
		 * input bits across the pool evenly.
		 */
		r->input_rotate = (r->input_rotate + 14) & 31;

	/* XOR in the various taps */
	w ^= r->pool[(i+TAP1)&(POOLWORDS-1)];
	w ^= r->pool[(i+TAP2)&(POOLWORDS-1)];
	w ^= r->pool[(i+TAP3)&(POOLWORDS-1)];
	w ^= r->pool[(i+TAP4)&(POOLWORDS-1)];
	w ^= r->pool[(i+TAP5)&(POOLWORDS-1)];
	w ^= r->pool[i];
	/* Rotate w left 1 bit (stolen from SHA) and store */
	r->pool[i] = (w << 1) | (w >> 31);
}

/*
 * This function adds entropy to the entropy "pool" by using timing
 * delays.  It uses the timer_rand_state structure to make an estimate
 * of how many bits of entropy this call has added to the pool.
 *
 * The number "num" is also added to the pool - it should somehow describe
 * the type of event which just happened.  This is currently 0-255 for
 * keyboard scan codes, and 256 upwards for interrupts.
 * On the i386, this is assumed to be at most 16 bits, and the high bits
 * are used for a high-resolution timer.
 *
 */
void
add_timer_randomness(r, state, num)
	struct random_bucket	*r;
	struct timer_rand_state	*state;
	u_int	num;
{
	int	delta, delta2;
	u_int	nbits;
	u_long	time;
	struct timeval	tv;

	microtime(&tv);
	time = tv.tv_usec ^ tv.tv_sec;

	add_entropy_word(r, (u_int32_t)num);
	add_entropy_word(r, time);

	/*
	 * Calculate number of bits of randomness we probably
	 * added.  We take into account the first and second order
	 * deltas in order to make our estimate.
	 */
	if (!state->dont_count_entropy) {
		delta = time - state->last_time;
		state->last_time = time;

		delta2 = delta - state->last_delta;
		state->last_delta = delta;

		if (delta < 0) delta = -delta;
		if (delta2 < 0) delta2 = -delta2;
		delta = MIN(delta, delta2) >> 1;
		for (nbits = 0; delta; nbits++)
			delta >>= 1;

		r->entropy_count += nbits;
	
		/* Prevent overflow */
		if (r->entropy_count > POOLBITS)
			r->entropy_count = POOLBITS;
	}

	if (r->entropy_count > 8 && rnd_sleep != 0) {
		rnd_sleep--;
#ifdef	DEBUG
		if (rnd_debug & RD_WAIT)
			printf("rnd: wakeup[%d]{%u}\n",
				rnd_sleep, r->entropy_count);
#endif
		wakeup(&rnd_sleep);
	}
}

void
add_mouse_randomness(mouse_data)
	u_int32_t	mouse_data;
{
	add_timer_randomness(&random_state, &mouse_timer_state, mouse_data);
}

void
add_net_randomness(isr)
	int	isr;
{
	add_timer_randomness(&random_state, &net_timer_state, ENT_NET + isr);
}

void
add_disk_randomness(n)
	u_int32_t n;
{
	u_int8_t c;

	/* Has randomattach run yet?  */
	if (random_state.pool == NULL)
		return;

	c = n & 0xff;
	n >>= 8;
	c ^= n & 0xff;
	n >>= 8;
	c ^= n & 0xff;
	n >>= 8;
	c ^= n & 0xff;
	add_timer_randomness(&random_state, &disk_timer_state, ENT_DISK + c);
}

void
add_tty_randomness(c)
	int	c;
{
	/* Has randomattach run yet?  */
	if (random_state.pool == NULL)
		return;

	add_timer_randomness(&random_state, &tty_timer_state, ENT_TTY + c);
}

#if POOLWORDS % 16
#error extract_entropy() assumes that POOLWORDS is a multiple of 16 words.
#endif
/*
 * This function extracts randomness from the "entropy pool", and
 * returns it in a buffer.  This function computes how many remaining
 * bits of entropy are left in the pool, but it does not restrict the
 * number of bytes that are actually obtained.
 */
static __inline int
extract_entropy(r, buf, nbytes)
	struct random_bucket *r;
	char	*buf;
	int	nbytes;
{
	int	ret, i;
	MD5_CTX tmp;
	
	add_timer_randomness(r, &extract_timer_state, nbytes);
	
	/* Redundant, but just in case... */
	if (r->entropy_count > POOLBITS) 
		r->entropy_count = POOLBITS;

	ret = nbytes;
	if (r->entropy_count / 8 >= nbytes)
		r->entropy_count -= nbytes*8;
	else
		r->entropy_count = 0;

	while (nbytes) {
		/* Hash the pool to get the output */
		MD5Init(&tmp);

		for (i = 0; i < POOLWORDS; i += 16)
			MD5Update(&tmp, (u_int8_t*)r->pool+i, 16);

		/* Modify pool so next hash will produce different results */
		for (i = 0; i < sizeof(tmp.buffer)/sizeof(tmp.buffer[0]); i++)
			add_entropy_word(r, tmp.buffer[i]);
		/*
		 * Run the MD5 Transform one more time, since we want
		 * to add at least minimal obscuring of the inputs to
		 * add_entropy_word().  --- TYT
		 */
		MD5Update(&tmp, (u_int8_t*)r->pool, 16);

		/*
		 * In case the hash function has some recognizable
		 * output pattern, we fold it in half.
		 */
		{
			register u_int8_t	*cp, *dp;
			cp = (u_int8_t *) &tmp.buffer;
			dp = cp + sizeof(tmp.buffer) - 1;
			while (cp < dp)
				*cp++ ^= *dp--;
		}

		/* Copy data to destination buffer */
		i = MIN(nbytes, sizeof(tmp.buffer));
		bcopy((caddr_t)&tmp.buffer, buf, i);
		nbytes -= i;
		buf += i;
		add_timer_randomness(r, &extract_timer_state, nbytes);
	}

	/* Wipe data from memory */
	bzero(&tmp, sizeof(tmp));
	
	return ret;
}

/*
 * This function is the exported kernel interface.  It returns some
 * number of good random numbers, suitable for seeding TCP sequence
 * numbers, etc.
 */
void
get_random_bytes(buf, nbytes)
	void	*buf;
	size_t	nbytes;
{
	extract_entropy(&random_state, (char *) buf, nbytes);
}

int
randomread(dev, uio, ioflag)
	dev_t	dev;
	struct uio *uio;
	int	ioflag;
{
	int	ret = 0;
	int	s, i;
	
	if (uio->uio_resid == 0)
		return 0;

	while (!ret && uio->uio_resid > 0) {
		u_int32_t buf[ POOLWORDS ];
		int	n = min(sizeof(buf), uio->uio_resid);

		s = splhigh();
		switch(minor(dev)) {
		case RND_RND:
			ret = EIO;	/* no chip -- error */
			break;
		case RND_SRND:
			if (random_state.entropy_count < 8) {
				if (ioflag & IO_NDELAY) {
					ret = EWOULDBLOCK;
					break;
				}
#ifdef	DEBUG
				if (rnd_debug & RD_WAIT)
					printf("rnd: sleep[%d]\n",
					    rnd_sleep);
#endif
				rnd_sleep++;
				ret = tsleep(&rnd_sleep, PWAIT | PCATCH,
				    "rndrd", 0);
#ifdef	DEBUG
				if (rnd_debug & RD_WAIT)
					printf("rnd: awakened(%d)\n", ret);
#endif
				if (ret)
					break;
			}
			n = min(n, random_state.entropy_count / 8);
#ifdef	DEBUG
			if (rnd_debug & RD_OUTPUT)
				printf("rnd: %u possible output\n", n);
#endif
		case RND_URND:
			n = extract_entropy(&random_state, (char *)buf, n);
#ifdef	DEBUG
			if (rnd_debug & RD_OUTPUT)
				printf("rnd: %u bytes for output\n", n);
#endif
			break;
		case RND_PRND:	
			i = (n + 3) / 4;
			while (i--)
				buf[i] = random();
			break;
		case RND_ARND:
		    {
			u_char *cp = (u_char *) buf;
			u_char *end = cp + n;
			arc4maybeinit ();
			while (cp < end)
				*cp++ = arc4_getbyte (&arc4random_state);
			break;
		    }
		}
		splx(s);
		if (n != 0 && ret == 0)
			ret = uiomove((caddr_t)buf, n, uio);
	}
	return ret;
}

int
randomselect(dev, rw, p)
	dev_t	dev;
	int	rw;
	struct proc *p;
{
	switch (rw) {
	case FREAD:
		return random_state.entropy_count > 0;
	case FWRITE:
		return 1;
	}
	return 0;
}

static __inline void
arc4_stir (struct arc4_stream *as)
{
	u_char buf[256];

	microtime ((struct timeval *) buf);
	get_random_bytes (buf + sizeof (struct timeval),
			  sizeof (buf) - sizeof (struct timeval));
	arc4_init (&arc4random_state, buf, sizeof (buf));
}

int
randomwrite(dev, uio, flags)
	dev_t	dev;
	struct uio *uio;
	int	flags;
{
	int	ret = 0;

	if (minor(dev) == RND_RND || minor(dev) == RND_PRND)
		return ENXIO;

	if (uio->uio_resid == 0)
		return 0;

	while (!ret && uio->uio_resid > 0) {
		u_int32_t	buf[ POOLWORDS ];
		u_short		n = min(sizeof(buf),uio->uio_resid);

		ret = uiomove((caddr_t)buf, n, uio);
		if (!ret) {
			int	i;
			while (n % sizeof(u_int32_t))
				((u_char *) buf)[n++] = 0;
			n >>= 2;
			for (i = 0; i < n; i++)
				add_entropy_word(&random_state, buf[i]);
		}
	}

	if (minor(dev) == RND_ARND && !ret)
		arc4_stir (&arc4random_state);

	return ret;
}

int
randomioctl(dev, cmd, data, flag, p)
	dev_t	dev;
	u_long	cmd;
	caddr_t	data;
	int	flag;
	struct proc *p;
{
	int	ret;
	u_int	cnt;

	switch (cmd) {
	case RNDGETENTCNT:
		ret = copyout(&random_state.entropy_count, data,
		    sizeof(random_state.entropy_count));
		break;
	case RNDADDTOENTCNT:
		if (suser(p->p_ucred, &p->p_acflag) != 0)
			return EPERM;
		copyin(&cnt, data, sizeof(cnt));
		random_state.entropy_count += cnt;
		if (random_state.entropy_count > POOLBITS)
			random_state.entropy_count = POOLBITS;
		ret = 0;
		break;
	case RNDZAPENTCNT:
		if (suser(p->p_ucred, &p->p_acflag) != 0)
			return EPERM;
		random_state.entropy_count = 0;
		ret = 0;
		break;
	case RNDSTIRARC4:
		if (suser(p->p_ucred, &p->p_acflag) != 0)
			return EPERM;
		if (random_state.entropy_count < 64)
			return EAGAIN;
		arc4_stir (&arc4random_state);
		ret = 0;
		break;
	default:
		ret = EINVAL;
	}
	return ret;
}
