#objdump: -dr --prefix-addresses -mmips:3000
#name: MIPS ush
#as: -32 -mips1

# Test the ush macro.

.*: +file format .*mips.*

Disassembly of section .text:
0+0000 <[^>]*> sb	a0,[01]\(zero\)
0+0004 <[^>]*> srl	at,a0,0x8
0+0008 <[^>]*> sb	at,[01]\(zero\)
0+000c <[^>]*> sb	a0,[12]\(zero\)
0+0010 <[^>]*> srl	at,a0,0x8
0+0014 <[^>]*> sb	at,[12]\(zero\)
0+0018 <[^>]*> li	at,0x8000
0+001c <[^>]*> sb	a0,[01]\(at\)
0+0020 <[^>]*> srl	a0,a0,0x8
0+0024 <[^>]*> sb	a0,[01]\(at\)
0+0028 <[^>]*> lbu	at,[01]\(at\)
0+002c <[^>]*> sll	a0,a0,0x8
0+0030 <[^>]*> or	a0,a0,at
0+0034 <[^>]*> sb	a0,-3276[78]\(zero\)
0+0038 <[^>]*> srl	at,a0,0x8
0+003c <[^>]*> sb	at,-3276[78]\(zero\)
0+0040 <[^>]*> lui	at,0x1
0+0044 <[^>]*> sb	a0,[01]\(at\)
0+0048 <[^>]*> srl	a0,a0,0x8
0+004c <[^>]*> sb	a0,[01]\(at\)
0+0050 <[^>]*> lbu	at,[01]\(at\)
0+0054 <[^>]*> sll	a0,a0,0x8
0+0058 <[^>]*> or	a0,a0,at
0+005c <[^>]*> lui	at,0x1
0+0060 <[^>]*> ori	at,at,0xa5a5
0+0064 <[^>]*> sb	a0,[01]\(at\)
0+0068 <[^>]*> srl	a0,a0,0x8
0+006c <[^>]*> sb	a0,[01]\(at\)
0+0070 <[^>]*> lbu	at,[01]\(at\)
0+0074 <[^>]*> sll	a0,a0,0x8
0+0078 <[^>]*> or	a0,a0,at
0+007c <[^>]*> sb	a0,[01]\(a1\)
0+0080 <[^>]*> srl	at,a0,0x8
0+0084 <[^>]*> sb	at,[01]\(a1\)
0+0088 <[^>]*> sb	a0,[12]\(a1\)
0+008c <[^>]*> srl	at,a0,0x8
0+0090 <[^>]*> sb	at,[12]\(a1\)
0+0094 <[^>]*> lui	at,[-0-9x]+
[ 	]*94: [A-Z0-9_]*HI[A-Z0-9_]*	.data.*
0+0098 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*98: [A-Z0-9_]*LO[A-Z0-9_]*	.data.*
0+009c <[^>]*> sb	a0,[01]\(at\)
0+00a0 <[^>]*> srl	a0,a0,0x8
0+00a4 <[^>]*> sb	a0,[01]\(at\)
0+00a8 <[^>]*> lbu	at,[01]\(at\)
0+00ac <[^>]*> sll	a0,a0,0x8
0+00b0 <[^>]*> or	a0,a0,at
0+00b4 <[^>]*> lui	at,0x0
[ 	]*b4: [A-Z0-9_]*HI[A-Z0-9_]*	big_external_data_label
0+00b8 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*b8: [A-Z0-9_]*LO[A-Z0-9_]*	big_external_data_label
0+00bc <[^>]*> sb	a0,[01]\(at\)
0+00c0 <[^>]*> srl	a0,a0,0x8
0+00c4 <[^>]*> sb	a0,[01]\(at\)
0+00c8 <[^>]*> lbu	at,[01]\(at\)
0+00cc <[^>]*> sll	a0,a0,0x8
0+00d0 <[^>]*> or	a0,a0,at
0+00d4 <[^>]*> addiu	at,gp,0
[ 	]*d4: [A-Z0-9_]*GPREL[A-Z0-9_]*	small_external_data_label
0+00d8 <[^>]*> sb	a0,[01]\(at\)
0+00dc <[^>]*> srl	a0,a0,0x8
0+00e0 <[^>]*> sb	a0,[01]\(at\)
0+00e4 <[^>]*> lbu	at,[01]\(at\)
0+00e8 <[^>]*> sll	a0,a0,0x8
0+00ec <[^>]*> or	a0,a0,at
0+00f0 <[^>]*> lui	at,[-0-9x]+
[ 	]*f0: [A-Z0-9_]*HI[A-Z0-9_]*	big_external_common
0+00f4 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*f4: [A-Z0-9_]*LO[A-Z0-9_]*	big_external_common
0+00f8 <[^>]*> sb	a0,[01]\(at\)
0+00fc <[^>]*> srl	a0,a0,0x8
0+0100 <[^>]*> sb	a0,[01]\(at\)
0+0104 <[^>]*> lbu	at,[01]\(at\)
0+0108 <[^>]*> sll	a0,a0,0x8
0+010c <[^>]*> or	a0,a0,at
0+0110 <[^>]*> addiu	at,gp,0
[ 	]*110: [A-Z0-9_]*GPREL[A-Z0-9_]*	small_external_common
0+0114 <[^>]*> sb	a0,[01]\(at\)
0+0118 <[^>]*> srl	a0,a0,0x8
0+011c <[^>]*> sb	a0,[01]\(at\)
0+0120 <[^>]*> lbu	at,[01]\(at\)
0+0124 <[^>]*> sll	a0,a0,0x8
0+0128 <[^>]*> or	a0,a0,at
0+012c <[^>]*> lui	at,[-0-9x]+
[ 	]*12c: [A-Z0-9_]*HI[A-Z0-9_]*	.bss.*
0+0130 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*130: [A-Z0-9_]*LO[A-Z0-9_]*	.bss.*
0+0134 <[^>]*> sb	a0,[01]\(at\)
0+0138 <[^>]*> srl	a0,a0,0x8
0+013c <[^>]*> sb	a0,[01]\(at\)
0+0140 <[^>]*> lbu	at,[01]\(at\)
0+0144 <[^>]*> sll	a0,a0,0x8
0+0148 <[^>]*> or	a0,a0,at
0+014c <[^>]*> addiu	at,gp,[-0-9]+
[ 	]*14c: [A-Z0-9_]*GPREL[A-Z0-9_]*	.sbss.*
0+0150 <[^>]*> sb	a0,[01]\(at\)
0+0154 <[^>]*> srl	a0,a0,0x8
0+0158 <[^>]*> sb	a0,[01]\(at\)
0+015c <[^>]*> lbu	at,[01]\(at\)
0+0160 <[^>]*> sll	a0,a0,0x8
0+0164 <[^>]*> or	a0,a0,at
0+0168 <[^>]*> lui	at,0x0
[ 	]*168: [A-Z0-9_]*HI[A-Z0-9_]*	.data.*
0+016c <[^>]*> addiu	at,at,[-0-9]+
[ 	]*16c: [A-Z0-9_]*LO[A-Z0-9_]*	.data.*
0+0170 <[^>]*> sb	a0,[01]\(at\)
0+0174 <[^>]*> srl	a0,a0,0x8
0+0178 <[^>]*> sb	a0,[01]\(at\)
0+017c <[^>]*> lbu	at,[01]\(at\)
0+0180 <[^>]*> sll	a0,a0,0x8
0+0184 <[^>]*> or	a0,a0,at
0+0188 <[^>]*> lui	at,0x0
[ 	]*188: [A-Z0-9_]*HI[A-Z0-9_]*	big_external_data_label
0+018c <[^>]*> addiu	at,at,[-0-9]+
[ 	]*18c: [A-Z0-9_]*LO[A-Z0-9_]*	big_external_data_label
0+0190 <[^>]*> sb	a0,[01]\(at\)
0+0194 <[^>]*> srl	a0,a0,0x8
0+0198 <[^>]*> sb	a0,[01]\(at\)
0+019c <[^>]*> lbu	at,[01]\(at\)
0+01a0 <[^>]*> sll	a0,a0,0x8
0+01a4 <[^>]*> or	a0,a0,at
0+01a8 <[^>]*> addiu	at,gp,1
[ 	]*1a8: [A-Z0-9_]*GPREL[A-Z0-9_]*	small_external_data_label
0+01ac <[^>]*> sb	a0,[01]\(at\)
0+01b0 <[^>]*> srl	a0,a0,0x8
0+01b4 <[^>]*> sb	a0,[01]\(at\)
0+01b8 <[^>]*> lbu	at,[01]\(at\)
0+01bc <[^>]*> sll	a0,a0,0x8
0+01c0 <[^>]*> or	a0,a0,at
0+01c4 <[^>]*> lui	at,0x0
[ 	]*1c4: [A-Z0-9_]*HI[A-Z0-9_]*	big_external_common
0+01c8 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*1c8: [A-Z0-9_]*LO[A-Z0-9_]*	big_external_common
0+01cc <[^>]*> sb	a0,[01]\(at\)
0+01d0 <[^>]*> srl	a0,a0,0x8
0+01d4 <[^>]*> sb	a0,[01]\(at\)
0+01d8 <[^>]*> lbu	at,[01]\(at\)
0+01dc <[^>]*> sll	a0,a0,0x8
0+01e0 <[^>]*> or	a0,a0,at
0+01e4 <[^>]*> addiu	at,gp,1
[ 	]*1e4: [A-Z0-9_]*GPREL[A-Z0-9_]*	small_external_common
0+01e8 <[^>]*> sb	a0,[01]\(at\)
0+01ec <[^>]*> srl	a0,a0,0x8
0+01f0 <[^>]*> sb	a0,[01]\(at\)
0+01f4 <[^>]*> lbu	at,[01]\(at\)
0+01f8 <[^>]*> sll	a0,a0,0x8
0+01fc <[^>]*> or	a0,a0,at
0+0200 <[^>]*> lui	at,0x0
[ 	]*200: [A-Z0-9_]*HI[A-Z0-9_]*	.bss.*
0+0204 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*204: [A-Z0-9_]*LO[A-Z0-9_]*	.bss.*
0+0208 <[^>]*> sb	a0,[01]\(at\)
0+020c <[^>]*> srl	a0,a0,0x8
0+0210 <[^>]*> sb	a0,[01]\(at\)
0+0214 <[^>]*> lbu	at,[01]\(at\)
0+0218 <[^>]*> sll	a0,a0,0x8
0+021c <[^>]*> or	a0,a0,at
0+0220 <[^>]*> addiu	at,gp,[-0-9]+
[ 	]*220: [A-Z0-9_]*GPREL[A-Z0-9_]*	.sbss.*
0+0224 <[^>]*> sb	a0,[01]\(at\)
0+0228 <[^>]*> srl	a0,a0,0x8
0+022c <[^>]*> sb	a0,[01]\(at\)
0+0230 <[^>]*> lbu	at,[01]\(at\)
0+0234 <[^>]*> sll	a0,a0,0x8
0+0238 <[^>]*> or	a0,a0,at
0+023c <[^>]*> lui	at,[-0-9x]+
[ 	]*23c: [A-Z0-9_]*HI[A-Z0-9_]*	.data.*
0+0240 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*240: [A-Z0-9_]*LO[A-Z0-9_]*	.data.*
0+0244 <[^>]*> sb	a0,[01]\(at\)
0+0248 <[^>]*> srl	a0,a0,0x8
0+024c <[^>]*> sb	a0,[01]\(at\)
0+0250 <[^>]*> lbu	at,[01]\(at\)
0+0254 <[^>]*> sll	a0,a0,0x8
0+0258 <[^>]*> or	a0,a0,at
0+025c <[^>]*> lui	at,[-0-9x]+
[ 	]*25c: [A-Z0-9_]*HI[A-Z0-9_]*	big_external_data_label
0+0260 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*260: [A-Z0-9_]*LO[A-Z0-9_]*	big_external_data_label
0+0264 <[^>]*> sb	a0,[01]\(at\)
0+0268 <[^>]*> srl	a0,a0,0x8
0+026c <[^>]*> sb	a0,[01]\(at\)
0+0270 <[^>]*> lbu	at,[01]\(at\)
0+0274 <[^>]*> sll	a0,a0,0x8
0+0278 <[^>]*> or	a0,a0,at
0+027c <[^>]*> lui	at,[-0-9x]+
[ 	]*27c: [A-Z0-9_]*HI[A-Z0-9_]*	small_external_data_label
0+0280 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*280: [A-Z0-9_]*LO[A-Z0-9_]*	small_external_data_label
0+0284 <[^>]*> sb	a0,[01]\(at\)
0+0288 <[^>]*> srl	a0,a0,0x8
0+028c <[^>]*> sb	a0,[01]\(at\)
0+0290 <[^>]*> lbu	at,[01]\(at\)
0+0294 <[^>]*> sll	a0,a0,0x8
0+0298 <[^>]*> or	a0,a0,at
0+029c <[^>]*> lui	at,[-0-9x]+
[ 	]*29c: [A-Z0-9_]*HI[A-Z0-9_]*	big_external_common
0+02a0 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*2a0: [A-Z0-9_]*LO[A-Z0-9_]*	big_external_common
0+02a4 <[^>]*> sb	a0,[01]\(at\)
0+02a8 <[^>]*> srl	a0,a0,0x8
0+02ac <[^>]*> sb	a0,[01]\(at\)
0+02b0 <[^>]*> lbu	at,[01]\(at\)
0+02b4 <[^>]*> sll	a0,a0,0x8
0+02b8 <[^>]*> or	a0,a0,at
0+02bc <[^>]*> lui	at,[-0-9x]+
[ 	]*2bc: [A-Z0-9_]*HI[A-Z0-9_]*	small_external_common
0+02c0 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*2c0: [A-Z0-9_]*LO[A-Z0-9_]*	small_external_common
0+02c4 <[^>]*> sb	a0,[01]\(at\)
0+02c8 <[^>]*> srl	a0,a0,0x8
0+02cc <[^>]*> sb	a0,[01]\(at\)
0+02d0 <[^>]*> lbu	at,[01]\(at\)
0+02d4 <[^>]*> sll	a0,a0,0x8
0+02d8 <[^>]*> or	a0,a0,at
0+02dc <[^>]*> lui	at,[-0-9x]+
[ 	]*2dc: [A-Z0-9_]*HI[A-Z0-9_]*	.bss.*
0+02e0 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*2e0: [A-Z0-9_]*LO[A-Z0-9_]*	.bss.*
0+02e4 <[^>]*> sb	a0,[01]\(at\)
0+02e8 <[^>]*> srl	a0,a0,0x8
0+02ec <[^>]*> sb	a0,[01]\(at\)
0+02f0 <[^>]*> lbu	at,[01]\(at\)
0+02f4 <[^>]*> sll	a0,a0,0x8
0+02f8 <[^>]*> or	a0,a0,at
0+02fc <[^>]*> lui	at,[-0-9x]+
[ 	]*2fc: [A-Z0-9_]*HI[A-Z0-9_]*	.sbss.*
0+0300 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*300: [A-Z0-9_]*LO[A-Z0-9_]*	.sbss.*
0+0304 <[^>]*> sb	a0,[01]\(at\)
0+0308 <[^>]*> srl	a0,a0,0x8
0+030c <[^>]*> sb	a0,[01]\(at\)
0+0310 <[^>]*> lbu	at,[01]\(at\)
0+0314 <[^>]*> sll	a0,a0,0x8
0+0318 <[^>]*> or	a0,a0,at
0+031c <[^>]*> lui	at,0x0
[ 	]*31c: [A-Z0-9_]*HI[A-Z0-9_]*	.data.*
0+0320 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*320: [A-Z0-9_]*LO[A-Z0-9_]*	.data.*
0+0324 <[^>]*> sb	a0,[01]\(at\)
0+0328 <[^>]*> srl	a0,a0,0x8
0+032c <[^>]*> sb	a0,[01]\(at\)
0+0330 <[^>]*> lbu	at,[01]\(at\)
0+0334 <[^>]*> sll	a0,a0,0x8
0+0338 <[^>]*> or	a0,a0,at
0+033c <[^>]*> lui	at,0x0
[ 	]*33c: [A-Z0-9_]*HI[A-Z0-9_]*	big_external_data_label
0+0340 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*340: [A-Z0-9_]*LO[A-Z0-9_]*	big_external_data_label
0+0344 <[^>]*> sb	a0,[01]\(at\)
0+0348 <[^>]*> srl	a0,a0,0x8
0+034c <[^>]*> sb	a0,[01]\(at\)
0+0350 <[^>]*> lbu	at,[01]\(at\)
0+0354 <[^>]*> sll	a0,a0,0x8
0+0358 <[^>]*> or	a0,a0,at
0+035c <[^>]*> lui	at,0x0
[ 	]*35c: [A-Z0-9_]*HI[A-Z0-9_]*	small_external_data_label
0+0360 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*360: [A-Z0-9_]*LO[A-Z0-9_]*	small_external_data_label
0+0364 <[^>]*> sb	a0,[01]\(at\)
0+0368 <[^>]*> srl	a0,a0,0x8
0+036c <[^>]*> sb	a0,[01]\(at\)
0+0370 <[^>]*> lbu	at,[01]\(at\)
0+0374 <[^>]*> sll	a0,a0,0x8
0+0378 <[^>]*> or	a0,a0,at
0+037c <[^>]*> lui	at,0x0
[ 	]*37c: [A-Z0-9_]*HI[A-Z0-9_]*	big_external_common
0+0380 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*380: [A-Z0-9_]*LO[A-Z0-9_]*	big_external_common
0+0384 <[^>]*> sb	a0,[01]\(at\)
0+0388 <[^>]*> srl	a0,a0,0x8
0+038c <[^>]*> sb	a0,[01]\(at\)
0+0390 <[^>]*> lbu	at,[01]\(at\)
0+0394 <[^>]*> sll	a0,a0,0x8
0+0398 <[^>]*> or	a0,a0,at
0+039c <[^>]*> lui	at,0x0
[ 	]*39c: [A-Z0-9_]*HI[A-Z0-9_]*	small_external_common
0+03a0 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*3a0: [A-Z0-9_]*LO[A-Z0-9_]*	small_external_common
0+03a4 <[^>]*> sb	a0,[01]\(at\)
0+03a8 <[^>]*> srl	a0,a0,0x8
0+03ac <[^>]*> sb	a0,[01]\(at\)
0+03b0 <[^>]*> lbu	at,[01]\(at\)
0+03b4 <[^>]*> sll	a0,a0,0x8
0+03b8 <[^>]*> or	a0,a0,at
0+03bc <[^>]*> lui	at,0x0
[ 	]*3bc: [A-Z0-9_]*HI[A-Z0-9_]*	.bss.*
0+03c0 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*3c0: [A-Z0-9_]*LO[A-Z0-9_]*	.bss.*
0+03c4 <[^>]*> sb	a0,[01]\(at\)
0+03c8 <[^>]*> srl	a0,a0,0x8
0+03cc <[^>]*> sb	a0,[01]\(at\)
0+03d0 <[^>]*> lbu	at,[01]\(at\)
0+03d4 <[^>]*> sll	a0,a0,0x8
0+03d8 <[^>]*> or	a0,a0,at
0+03dc <[^>]*> lui	at,0x0
[ 	]*3dc: [A-Z0-9_]*HI[A-Z0-9_]*	.sbss.*
0+03e0 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*3e0: [A-Z0-9_]*LO[A-Z0-9_]*	.sbss.*
0+03e4 <[^>]*> sb	a0,[01]\(at\)
0+03e8 <[^>]*> srl	a0,a0,0x8
0+03ec <[^>]*> sb	a0,[01]\(at\)
0+03f0 <[^>]*> lbu	at,[01]\(at\)
0+03f4 <[^>]*> sll	a0,a0,0x8
0+03f8 <[^>]*> or	a0,a0,at
0+03fc <[^>]*> lui	at,[-0-9x]+
[ 	]*3fc: [A-Z0-9_]*HI[A-Z0-9_]*	.data.*
0+0400 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*400: [A-Z0-9_]*LO[A-Z0-9_]*	.data.*
0+0404 <[^>]*> sb	a0,[01]\(at\)
0+0408 <[^>]*> srl	a0,a0,0x8
0+040c <[^>]*> sb	a0,[01]\(at\)
0+0410 <[^>]*> lbu	at,[01]\(at\)
0+0414 <[^>]*> sll	a0,a0,0x8
0+0418 <[^>]*> or	a0,a0,at
0+041c <[^>]*> lui	at,[-0-9x]+
[ 	]*41c: [A-Z0-9_]*HI[A-Z0-9_]*	big_external_data_label
0+0420 <[^>]*> addiu	at,at,0
[ 	]*420: [A-Z0-9_]*LO[A-Z0-9_]*	big_external_data_label
0+0424 <[^>]*> sb	a0,[01]\(at\)
0+0428 <[^>]*> srl	a0,a0,0x8
0+042c <[^>]*> sb	a0,[01]\(at\)
0+0430 <[^>]*> lbu	at,[01]\(at\)
0+0434 <[^>]*> sll	a0,a0,0x8
0+0438 <[^>]*> or	a0,a0,at
0+043c <[^>]*> lui	at,[-0-9x]+
[ 	]*43c: [A-Z0-9_]*HI[A-Z0-9_]*	small_external_data_label
0+0440 <[^>]*> addiu	at,at,0
[ 	]*440: [A-Z0-9_]*LO[A-Z0-9_]*	small_external_data_label
0+0444 <[^>]*> sb	a0,[01]\(at\)
0+0448 <[^>]*> srl	a0,a0,0x8
0+044c <[^>]*> sb	a0,[01]\(at\)
0+0450 <[^>]*> lbu	at,[01]\(at\)
0+0454 <[^>]*> sll	a0,a0,0x8
0+0458 <[^>]*> or	a0,a0,at
0+045c <[^>]*> lui	at,[-0-9x]+
[ 	]*45c: [A-Z0-9_]*HI[A-Z0-9_]*	big_external_common
0+0460 <[^>]*> addiu	at,at,0
[ 	]*460: [A-Z0-9_]*LO[A-Z0-9_]*	big_external_common
0+0464 <[^>]*> sb	a0,[01]\(at\)
0+0468 <[^>]*> srl	a0,a0,0x8
0+046c <[^>]*> sb	a0,[01]\(at\)
0+0470 <[^>]*> lbu	at,[01]\(at\)
0+0474 <[^>]*> sll	a0,a0,0x8
0+0478 <[^>]*> or	a0,a0,at
0+047c <[^>]*> lui	at,[-0-9x]+
[ 	]*47c: [A-Z0-9_]*HI[A-Z0-9_]*	small_external_common
0+0480 <[^>]*> addiu	at,at,0
[ 	]*480: [A-Z0-9_]*LO[A-Z0-9_]*	small_external_common
0+0484 <[^>]*> sb	a0,[01]\(at\)
0+0488 <[^>]*> srl	a0,a0,0x8
0+048c <[^>]*> sb	a0,[01]\(at\)
0+0490 <[^>]*> lbu	at,[01]\(at\)
0+0494 <[^>]*> sll	a0,a0,0x8
0+0498 <[^>]*> or	a0,a0,at
0+049c <[^>]*> lui	at,[-0-9x]+
[ 	]*49c: [A-Z0-9_]*HI[A-Z0-9_]*	.bss.*
0+04a0 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*4a0: [A-Z0-9_]*LO[A-Z0-9_]*	.bss.*
0+04a4 <[^>]*> sb	a0,[01]\(at\)
0+04a8 <[^>]*> srl	a0,a0,0x8
0+04ac <[^>]*> sb	a0,[01]\(at\)
0+04b0 <[^>]*> lbu	at,[01]\(at\)
0+04b4 <[^>]*> sll	a0,a0,0x8
0+04b8 <[^>]*> or	a0,a0,at
0+04bc <[^>]*> lui	at,[-0-9x]+
[ 	]*4bc: [A-Z0-9_]*HI[A-Z0-9_]*	.sbss.*
0+04c0 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*4c0: [A-Z0-9_]*LO[A-Z0-9_]*	.sbss.*
0+04c4 <[^>]*> sb	a0,[01]\(at\)
0+04c8 <[^>]*> srl	a0,a0,0x8
0+04cc <[^>]*> sb	a0,[01]\(at\)
0+04d0 <[^>]*> lbu	at,[01]\(at\)
0+04d4 <[^>]*> sll	a0,a0,0x8
0+04d8 <[^>]*> or	a0,a0,at
0+04dc <[^>]*> lui	at,[-0-9x]+
[ 	]*4dc: [A-Z0-9_]*HI[A-Z0-9_]*	.data.*
0+04e0 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*4e0: [A-Z0-9_]*LO[A-Z0-9_]*	.data.*
0+04e4 <[^>]*> sb	a0,[01]\(at\)
0+04e8 <[^>]*> srl	a0,a0,0x8
0+04ec <[^>]*> sb	a0,[01]\(at\)
0+04f0 <[^>]*> lbu	at,[01]\(at\)
0+04f4 <[^>]*> sll	a0,a0,0x8
0+04f8 <[^>]*> or	a0,a0,at
0+04fc <[^>]*> lui	at,[-0-9x]+
[ 	]*4fc: [A-Z0-9_]*HI[A-Z0-9_]*	big_external_data_label
0+0500 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*500: [A-Z0-9_]*LO[A-Z0-9_]*	big_external_data_label
0+0504 <[^>]*> sb	a0,[01]\(at\)
0+0508 <[^>]*> srl	a0,a0,0x8
0+050c <[^>]*> sb	a0,[01]\(at\)
0+0510 <[^>]*> lbu	at,[01]\(at\)
0+0514 <[^>]*> sll	a0,a0,0x8
0+0518 <[^>]*> or	a0,a0,at
0+051c <[^>]*> lui	at,[-0-9x]+
[ 	]*51c: [A-Z0-9_]*HI[A-Z0-9_]*	small_external_data_label
0+0520 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*520: [A-Z0-9_]*LO[A-Z0-9_]*	small_external_data_label
0+0524 <[^>]*> sb	a0,[01]\(at\)
0+0528 <[^>]*> srl	a0,a0,0x8
0+052c <[^>]*> sb	a0,[01]\(at\)
0+0530 <[^>]*> lbu	at,[01]\(at\)
0+0534 <[^>]*> sll	a0,a0,0x8
0+0538 <[^>]*> or	a0,a0,at
0+053c <[^>]*> lui	at,[-0-9x]+
[ 	]*53c: [A-Z0-9_]*HI[A-Z0-9_]*	big_external_common
0+0540 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*540: [A-Z0-9_]*LO[A-Z0-9_]*	big_external_common
0+0544 <[^>]*> sb	a0,[01]\(at\)
0+0548 <[^>]*> srl	a0,a0,0x8
0+054c <[^>]*> sb	a0,[01]\(at\)
0+0550 <[^>]*> lbu	at,[01]\(at\)
0+0554 <[^>]*> sll	a0,a0,0x8
0+0558 <[^>]*> or	a0,a0,at
0+055c <[^>]*> lui	at,[-0-9x]+
[ 	]*55c: [A-Z0-9_]*HI[A-Z0-9_]*	small_external_common
0+0560 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*560: [A-Z0-9_]*LO[A-Z0-9_]*	small_external_common
0+0564 <[^>]*> sb	a0,[01]\(at\)
0+0568 <[^>]*> srl	a0,a0,0x8
0+056c <[^>]*> sb	a0,[01]\(at\)
0+0570 <[^>]*> lbu	at,[01]\(at\)
0+0574 <[^>]*> sll	a0,a0,0x8
0+0578 <[^>]*> or	a0,a0,at
0+057c <[^>]*> lui	at,[-0-9x]+
[ 	]*57c: [A-Z0-9_]*HI[A-Z0-9_]*	.bss.*
0+0580 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*580: [A-Z0-9_]*LO[A-Z0-9_]*	.bss.*
0+0584 <[^>]*> sb	a0,[01]\(at\)
0+0588 <[^>]*> srl	a0,a0,0x8
0+058c <[^>]*> sb	a0,[01]\(at\)
0+0590 <[^>]*> lbu	at,[01]\(at\)
0+0594 <[^>]*> sll	a0,a0,0x8
0+0598 <[^>]*> or	a0,a0,at
0+059c <[^>]*> lui	at,[-0-9x]+
[ 	]*59c: [A-Z0-9_]*HI[A-Z0-9_]*	.sbss.*
0+05a0 <[^>]*> addiu	at,at,[-0-9]+
[ 	]*5a0: [A-Z0-9_]*LO[A-Z0-9_]*	.sbss.*
0+05a4 <[^>]*> sb	a0,[01]\(at\)
0+05a8 <[^>]*> srl	a0,a0,0x8
0+05ac <[^>]*> sb	a0,[01]\(at\)
0+05b0 <[^>]*> lbu	at,[01]\(at\)
0+05b4 <[^>]*> sll	a0,a0,0x8
0+05b8 <[^>]*> or	a0,a0,at
0+05bc <[^>]*> nop
