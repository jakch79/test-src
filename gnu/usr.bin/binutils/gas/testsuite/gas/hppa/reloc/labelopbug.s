	.SPACE $PRIVATE$
	.SUBSPA $DATA$,QUAD=1,ALIGN=8,ACCESS=31
	.SUBSPA $BSS$,QUAD=1,ALIGN=8,ACCESS=31,ZERO,SORT=82
	.SPACE $TEXT$
	.SUBSPA $LIT$,QUAD=0,ALIGN=8,ACCESS=44
	.SUBSPA $CODE$,QUAD=0,ALIGN=8,ACCESS=44,CODE_ONLY
	.IMPORT $global$,DATA
	.IMPORT $$dyncall,MILLICODE
; gcc_compiled.:
	.SPACE $TEXT$
	.SUBSPA $CODE$

	.align 4
s:
	.PROC
	.CALLINFO FRAME=128,CALLS,SAVE_RP
	.ENTRY
	stw 2,-20(0,30)
	copy 4,1
	copy 30,4
	stwm 1,128(0,30)
	stw %r30,12(0,%r4)
	ldil L'L$0007,%r19
	ldo R'L$0007(%r19),%r19
	comib,>= 0,%r26,L$0002
	stw %r19,8(0,%r4)
L$0003:
L$0002:
	bl L$0001,0
	ldo 1(0),%r28
L$0007:
	ldil L'L$0002,%r19
	ldo R'L$0002(%r19),%r19
	comb,= %r29,%r19,L$0002
	ldo -8(%r4),%r4
	.EXIT
	.PROCEND
