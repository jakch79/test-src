	/* Force .data aligned to 4K, so that .got very likely gets at
	   0x60000000000031b0 (0x60 bytes .tdata and 0x150 bytes
	   .dynamic).  */
	.data
	.balign	4096
	.section ".tdata", "awT", @progbits
	.globl sg1, sg2, sg3, sg4, sg5, sg6, sg7, sg8
	.globl sh1, sh2, sh3, sh4, sh5, sh6, sh7, sh8
	.hidden sh1, sh2, sh3, sh4, sh5, sh6, sh7, sh8
sg1:	.long 17
sg2:	.long 18
sg3:	.long 19
sg4:	.long 20
sg5:	.long 21
sg6:	.long 22
sg7:	.long 23
sg8:	.long 24
sl1:	.long 65
sl2:	.long 66
sl3:	.long 67
sl4:	.long 68
sl5:	.long 69
sl6:	.long 70
sl7:	.long 71
sl8:	.long 72
sh1:	.long 257
sh2:	.long 258
sh3:	.long 259
sh4:	.long 260
sh5:	.long 261
sh6:	.long 262
sh7:	.long 263
sh8:	.long 264
	.explicit
	.pred.safe_across_calls p1-p5,p16-p63
	/* Force .text aligned to 4K, so it very likely gets at
	   0x4000000000001000.  */
	.text
	.balign	4096
	.globl	fn2#
	.proc	fn2#
fn2:
	.prologue 12, 33
	.mib
	.save	ar.pfs, r34
	alloc	r34 = ar.pfs, 0, 3, 2, 0
	.save	rp, r33
	mov	r33 = b0

	/* GD */
	addl	r14 = @ltoff(@dtpmod(sG1#)), gp
	addl	r15 = @ltoff(@dtprel(sG1#)), gp
	;;
	ld8	out0 = [r14]
	ld8	out1 = [r15]
	br.call.sptk.many b0 = __tls_get_addr#
	;;

	/* GD against local symbol */
	addl	r14 = @ltoff(@dtpmod(sl2#)), gp
	addl	r15 = @ltoff(@dtprel(sl2#)), gp
	;;
	ld8	out0 = [r14]
	ld8	out1 = [r15]
	br.call.sptk.many b0 = __tls_get_addr#
	;;

	/* LD */
	addl	r14 = @ltoff(@dtpmod(sl1#)), gp
	addl	out1 = @dtprel(sl1#) + 1, r0
	;;
	ld8	out0 = [r14]
	br.call.sptk.many b0 = __tls_get_addr#
	;;

	/* LD with 4 variables variables */
	addl	r14 = @ltoff(@dtpmod(sh1#)), gp
	mov	out1 = r0
	;;
	ld8	out0 = [r14]
	br.call.sptk.many b0 = __tls_get_addr#
	;;
	mov	r2 = r8
	;;
	addl	r14 = @dtprel(sh1#), r2
	addl	r15 = @dtprel(sh2#) + 2, r2
	;;
	adds	r14 = @dtprel(sh3#) + 3, r8
	movl	r15 = @dtprel(sh4#) + 1
	;;
	add	r15 = r15, r8
	;;

	mov	ar.pfs = r34
	mov	b0 = r33
	br.ret.sptk.many b0
	.endp	fn2#
