
	.MACRO	RESERVE_STR P1=0 P2
	.SDATA	.SUBSTR("ABCDEFG",\P1,\P2)
	.ENDM

	RESERVE_STR	2,2
	RESERVE_STR	,3


	.MACRO FIND_STR P1
	.DATA.W	.INSTR("ABCDEFG","\P1", 0)
	.ENDM

	FIND_STR	CDE
	FIND_STR	H

	.MACRO	RESERVE_LENGTH P1
	.ALIGN	4
	.SRES	.LEN("\P1")
	.ENDM

	RESERVE_LENGTH	ABCDEF
	RESERVE_LENGTH	ABC

	.END

