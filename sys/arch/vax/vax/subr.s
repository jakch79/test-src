/*      $NetBSD: subr.s,v 1.12 1995/11/10 19:08:59 ragge Exp $     */

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /* All bugs are subject to removal without further notice */
		

#include "sys/syscall.h"
#include "sys/errno.h"

#include "machine/mtpr.h"
#include "machine/vmparam.h"
#include "machine/pte.h"
#include "machine/nexus.h"


		.text

		.globl	_sigcode,_esigcode
_sigcode:	pushr	$0x3f
		subl2	$0xc,sp
		movl	0x24(sp),r0
		calls	$3,(r0)
		popr	$0x3f
		chmk	$SYS_sigreturn
		halt	
		.align	2
_esigcode:

		.globl	_idsptch, _eidsptch
_idsptch:	pushr	$0x3f
		pushl	$1
		nop
		calls	$1, *$0x12345678
		popr	$0x3f
		rei
_eidsptch:

		.globl	_subyte
_subyte:	.word 0x0
		movl	4(ap),r0
#		probew	$3,$1,(r0)
#		beql	suerr
		movb	8(ap),(r0)
		clrl	r0
		ret

suerr:		movl	$-1,r0
		ret

                .globl  _fubyte
_fubyte:        .word 0x0
                movl    4(ap),r0
#                prober  $3,$1,(r0)
#                beql    suerr
                movzbl	(r0),r0
                ret


		.globl _badaddr
_badaddr:	.word	0x0
					# Called with addr,b/w/l
		mfpr	$0x12,r0
		mtpr	$0x1f,$0x12
		movl	4(ap),r2 	# First argument, the address
		movl	8(ap),r1 	# Sec arg, b,w,l
		pushl	r0		# Save old IPL
		clrl	r3
		movl	$4f,_memtest	# Set the return adress

		caseb	r1,$1,$4	# What is the size
1:		.word	1f-1b		
		.word	2f-1b
		.word	3f-1b		# This is unused
		.word	3f-1b
		
1:		movb	(r2),r1		# Test a byte
		brb	5f

2:		movw	(r2),r1		# Test a word
		brb	5f

3:		movl	(r2),r1		# Test a long
		brb	5f

4:		incl	r3		# Got machine chk => addr bad
5:		mtpr	(sp)+,$0x12
		movl	r3,r0
		ret

#
# copyin(from, to, len) copies from userspace to kernelspace.
#

	.globl	_locopyin
_locopyin:.word	0x1c
	movl    16(ap),r0       # Get fault pointer flag
	movl	$ci,(r0)

	movl	4(ap),r0	# from
	movl	8(ap),r1	# to
	movl	12(ap),r2	# len

	movl	r0,r4
	movl	r2,r3

	tstl	r2
	beql	3f
2:      movb    (r0)+,(r1)+       # XXX Should be done in a faster way.
        decl    r2              
        bneq    2b
3:      movl	16(ap),r0
	clrl	(r0)
	clrl    r0
ci:	ret

#
# locopyout(from, to, len, addr) in the same manner as copyin()
#	addr is iftrap addr for faulting.
#

	.globl	_locopyout
_locopyout:.word   0x1c
	movl	16(ap),r0	# Get fault pointer flag
	movl	$co,(r0)	# and save ret addr

        movl    4(ap),r0        # from
        movl    8(ap),r1        # to
        movl    12(ap),r2       # len

        movl    r1,r4
        movl    r2,r3

	tstl	r2
	beql	3f
2:	movb	(r0)+,(r1)+	# XXX Should be done in a faster way.
	decl	r2
	bneq	2b
3:	movl    16(ap),r0
	clrl	(r0)
	clrl	r0
co:	ret

#
# copystr(from, to, maxlen, *copied, addr)
# Only used in kernel mode, doesnt check accessability.
#

	.globl	_copystr
_copystr:	.word 0x7c
        movl    4(ap),r4        # from
        movl    8(ap),r5        # to
        movl    12(ap),r2       # len
	movl	16(ap),r3	# copied

#if VAX630
        movl    r4, r1          # (3) string address == r1
        movl    r2, r0          # (2) string length == r0
        jeql    Llocc_out       # forget zero length strings
Llocc_loop:
        tstb    (r1)
        jeql    Llocc_out
        incl    r1
        sobgtr  r0,Llocc_loop
Llocc_out:
        tstl    r0              # be sure of condition codes
#else
        locc    $0, r2, (r4)    # check for null byte
#endif
	beql	1f

	subl3	r0, r2, r6	# Len to copy.
	incl	r6
	tstl	r3
	beql	7f
	movl	r6,(r3)
7:	movc3	r6,(r4),(r5)
	movl	$0,r0
cs:	ret

1:	movc3	r2,(r4),(r5)
	movl	$ENAMETOOLONG, r0
	ret


_loswtch:	.globl	_loswtch,_rei
	mtpr	_curpcb,$PR_PCBB
	svpctx
	mtpr	_nypcb,$PR_PCBB
	ldpctx
_rei:	rei

	.data

_memtest:	.long 0 ; .globl _memtest	# Memory test in progress.

#ifdef DDB
/*
 * DDB is the only routine that uses setjmp/longjmp.
 */
	.globl	_setjmp, _longjmp
_setjmp:.word	0
	movl	4(ap), r0
	movl	8(fp), (r0)
	movl	12(fp),	4(r0)
	movl	16(fp), 8(r0)
	addl3	fp,$28,12(r0)
	clrl	r0
	ret

_longjmp:.word	0
	movl	4(ap), r1
	movl	8(ap), r0
	movl	(r1), ap
	movl	4(r1), fp
	movl	12(r1), sp
	jmp	*8(r1)
#endif 
