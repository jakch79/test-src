#as:
#objdump: -dr
#name: i860 pfmam

.*: +file format .*

Disassembly of section \.text:

00000000 <\.text>:
   0:	00 00 22 48 	mr2p1.ss	%f0,%f1,%f2
   4:	80 18 85 48 	mr2p1.sd	%f3,%f4,%f5
   8:	80 01 44 48 	mr2p1.dd	%f0,%f2,%f4
   c:	01 08 43 48 	mr2pt.ss	%f1,%f2,%f3
  10:	81 20 a6 48 	mr2pt.sd	%f4,%f5,%f6
  14:	81 11 86 48 	mr2pt.dd	%f2,%f4,%f6
  18:	02 10 64 48 	mr2mp1.ss	%f2,%f3,%f4
  1c:	82 30 e8 48 	mr2mp1.sd	%f6,%f7,%f8
  20:	82 21 c8 48 	mr2mp1.dd	%f4,%f6,%f8
  24:	03 18 85 48 	mr2mpt.ss	%f3,%f4,%f5
  28:	83 38 09 49 	mr2mpt.sd	%f7,%f8,%f9
  2c:	83 31 0a 49 	mr2mpt.dd	%f6,%f8,%f10
  30:	04 20 a6 48 	mi2p1.ss	%f4,%f5,%f6
  34:	84 40 2a 49 	mi2p1.sd	%f8,%f9,%f10
  38:	84 61 d0 49 	mi2p1.dd	%f12,%f14,%f16
  3c:	05 38 09 49 	mi2pt.ss	%f7,%f8,%f9
  40:	85 58 8d 49 	mi2pt.sd	%f11,%f12,%f13
  44:	85 71 12 4a 	mi2pt.dd	%f14,%f16,%f18
  48:	06 50 6c 49 	mi2mp1.ss	%f10,%f11,%f12
  4c:	86 70 f0 49 	mi2mp1.sd	%f14,%f15,%f16
  50:	86 81 54 4a 	mi2mp1.dd	%f16,%f18,%f20
  54:	07 68 cf 49 	mi2mpt.ss	%f13,%f14,%f15
  58:	87 88 53 4a 	mi2mpt.sd	%f17,%f18,%f19
  5c:	87 91 96 4a 	mi2mpt.dd	%f18,%f20,%f22
  60:	08 70 f0 49 	mrmt1p2.ss	%f14,%f15,%f16
  64:	88 a0 b6 4a 	mrmt1p2.sd	%f20,%f21,%f22
  68:	88 a1 d8 4a 	mrmt1p2.dd	%f20,%f22,%f24
  6c:	09 78 11 4a 	mm12mpm.ss	%f15,%f16,%f17
  70:	89 b8 19 4b 	mm12mpm.sd	%f23,%f24,%f25
  74:	89 b1 1a 4b 	mm12mpm.dd	%f22,%f24,%f26
  78:	0a 90 74 4a 	mrm1p2.ss	%f18,%f19,%f20
  7c:	8a d0 7c 4b 	mrm1p2.sd	%f26,%f27,%f28
  80:	8a a1 d8 4a 	mrm1p2.dd	%f20,%f22,%f24
  84:	0b 98 95 4a 	mm12ttpm.ss	%f19,%f20,%f21
  88:	8b e8 df 4b 	mm12ttpm.sd	%f29,%f30,%f31
  8c:	8b b1 1a 4b 	mm12ttpm.dd	%f22,%f24,%f26
  90:	0c a0 b6 4a 	mimt1p2.ss	%f20,%f21,%f22
  94:	8c 00 22 48 	mimt1p2.sd	%f0,%f1,%f2
  98:	8c c1 5c 4b 	mimt1p2.dd	%f24,%f26,%f28
  9c:	0d a8 d7 4a 	mm12tpm.ss	%f21,%f22,%f23
  a0:	8d 18 85 48 	mm12tpm.sd	%f3,%f4,%f5
  a4:	8d f1 02 48 	mm12tpm.dd	%f30,%f0,%f2
  a8:	0e b0 f8 4a 	mim1p2.ss	%f22,%f23,%f24
  ac:	8e 30 e8 48 	mim1p2.sd	%f6,%f7,%f8
  b0:	8e 21 c8 48 	mim1p2.dd	%f4,%f6,%f8
  b4:	0f bc 19 4b 	m12tpa.ss	%f23,%f24,%f25
  b8:	8f 4c 4b 49 	m12tpa.sd	%f9,%f10,%f11
  bc:	8f 35 0a 49 	m12tpa.dd	%f6,%f8,%f10
  c0:	00 02 22 48 	d.mr2p1.ss	%f0,%f1,%f2
  c4:	00 00 00 a0 	shl	%r0,%r0,%r0
  c8:	80 1a 85 48 	d.mr2p1.sd	%f3,%f4,%f5
  cc:	00 00 00 a0 	shl	%r0,%r0,%r0
  d0:	80 03 44 48 	d.mr2p1.dd	%f0,%f2,%f4
  d4:	00 00 00 a0 	shl	%r0,%r0,%r0
  d8:	01 0a 43 48 	d.mr2pt.ss	%f1,%f2,%f3
  dc:	00 00 00 a0 	shl	%r0,%r0,%r0
  e0:	81 22 a6 48 	d.mr2pt.sd	%f4,%f5,%f6
  e4:	00 00 00 a0 	shl	%r0,%r0,%r0
  e8:	81 13 86 48 	d.mr2pt.dd	%f2,%f4,%f6
  ec:	00 00 00 a0 	shl	%r0,%r0,%r0
  f0:	02 12 64 48 	d.mr2mp1.ss	%f2,%f3,%f4
  f4:	00 00 00 a0 	shl	%r0,%r0,%r0
  f8:	82 32 e8 48 	d.mr2mp1.sd	%f6,%f7,%f8
  fc:	00 00 00 a0 	shl	%r0,%r0,%r0
 100:	82 23 c8 48 	d.mr2mp1.dd	%f4,%f6,%f8
 104:	00 00 00 a0 	shl	%r0,%r0,%r0
 108:	03 1a 85 48 	d.mr2mpt.ss	%f3,%f4,%f5
 10c:	00 00 00 a0 	shl	%r0,%r0,%r0
 110:	83 3a 09 49 	d.mr2mpt.sd	%f7,%f8,%f9
 114:	00 00 00 a0 	shl	%r0,%r0,%r0
 118:	83 33 0a 49 	d.mr2mpt.dd	%f6,%f8,%f10
 11c:	00 00 00 a0 	shl	%r0,%r0,%r0
 120:	04 22 a6 48 	d.mi2p1.ss	%f4,%f5,%f6
 124:	00 00 00 a0 	shl	%r0,%r0,%r0
 128:	84 42 2a 49 	d.mi2p1.sd	%f8,%f9,%f10
 12c:	00 00 00 a0 	shl	%r0,%r0,%r0
 130:	84 63 d0 49 	d.mi2p1.dd	%f12,%f14,%f16
 134:	00 00 00 a0 	shl	%r0,%r0,%r0
 138:	05 3a 09 49 	d.mi2pt.ss	%f7,%f8,%f9
 13c:	00 00 00 a0 	shl	%r0,%r0,%r0
 140:	85 5a 8d 49 	d.mi2pt.sd	%f11,%f12,%f13
 144:	00 00 00 a0 	shl	%r0,%r0,%r0
 148:	85 73 12 4a 	d.mi2pt.dd	%f14,%f16,%f18
 14c:	00 00 00 a0 	shl	%r0,%r0,%r0
 150:	06 52 6c 49 	d.mi2mp1.ss	%f10,%f11,%f12
 154:	00 00 00 a0 	shl	%r0,%r0,%r0
 158:	86 72 f0 49 	d.mi2mp1.sd	%f14,%f15,%f16
 15c:	00 00 00 a0 	shl	%r0,%r0,%r0
 160:	86 83 54 4a 	d.mi2mp1.dd	%f16,%f18,%f20
 164:	00 00 00 a0 	shl	%r0,%r0,%r0
 168:	07 6a cf 49 	d.mi2mpt.ss	%f13,%f14,%f15
 16c:	00 00 00 a0 	shl	%r0,%r0,%r0
 170:	87 8a 53 4a 	d.mi2mpt.sd	%f17,%f18,%f19
 174:	00 00 00 a0 	shl	%r0,%r0,%r0
 178:	87 93 96 4a 	d.mi2mpt.dd	%f18,%f20,%f22
 17c:	00 00 00 a0 	shl	%r0,%r0,%r0
 180:	08 72 f0 49 	d.mrmt1p2.ss	%f14,%f15,%f16
 184:	00 00 00 a0 	shl	%r0,%r0,%r0
 188:	88 a2 b6 4a 	d.mrmt1p2.sd	%f20,%f21,%f22
 18c:	00 00 00 a0 	shl	%r0,%r0,%r0
 190:	88 a3 d8 4a 	d.mrmt1p2.dd	%f20,%f22,%f24
 194:	00 00 00 a0 	shl	%r0,%r0,%r0
 198:	09 7a 11 4a 	d.mm12mpm.ss	%f15,%f16,%f17
 19c:	00 00 00 a0 	shl	%r0,%r0,%r0
 1a0:	89 ba 19 4b 	d.mm12mpm.sd	%f23,%f24,%f25
 1a4:	00 00 00 a0 	shl	%r0,%r0,%r0
 1a8:	89 b3 1a 4b 	d.mm12mpm.dd	%f22,%f24,%f26
 1ac:	00 00 00 a0 	shl	%r0,%r0,%r0
 1b0:	0a 92 74 4a 	d.mrm1p2.ss	%f18,%f19,%f20
 1b4:	00 00 00 a0 	shl	%r0,%r0,%r0
 1b8:	8a d2 7c 4b 	d.mrm1p2.sd	%f26,%f27,%f28
 1bc:	00 00 00 a0 	shl	%r0,%r0,%r0
 1c0:	8a a3 d8 4a 	d.mrm1p2.dd	%f20,%f22,%f24
 1c4:	00 00 00 a0 	shl	%r0,%r0,%r0
 1c8:	0b 9a 95 4a 	d.mm12ttpm.ss	%f19,%f20,%f21
 1cc:	00 00 00 a0 	shl	%r0,%r0,%r0
 1d0:	8b ea df 4b 	d.mm12ttpm.sd	%f29,%f30,%f31
 1d4:	00 00 00 a0 	shl	%r0,%r0,%r0
 1d8:	8b b3 1a 4b 	d.mm12ttpm.dd	%f22,%f24,%f26
 1dc:	00 00 00 a0 	shl	%r0,%r0,%r0
 1e0:	0c a2 b6 4a 	d.mimt1p2.ss	%f20,%f21,%f22
 1e4:	00 00 00 a0 	shl	%r0,%r0,%r0
 1e8:	8c 02 22 48 	d.mimt1p2.sd	%f0,%f1,%f2
 1ec:	00 00 00 a0 	shl	%r0,%r0,%r0
 1f0:	8c c3 5c 4b 	d.mimt1p2.dd	%f24,%f26,%f28
 1f4:	00 00 00 a0 	shl	%r0,%r0,%r0
 1f8:	0d aa d7 4a 	d.mm12tpm.ss	%f21,%f22,%f23
 1fc:	00 00 00 a0 	shl	%r0,%r0,%r0
 200:	8d 1a 85 48 	d.mm12tpm.sd	%f3,%f4,%f5
 204:	00 00 00 a0 	shl	%r0,%r0,%r0
 208:	8d f3 02 48 	d.mm12tpm.dd	%f30,%f0,%f2
 20c:	00 00 00 a0 	shl	%r0,%r0,%r0
 210:	0e b2 f8 4a 	d.mim1p2.ss	%f22,%f23,%f24
 214:	00 00 00 a0 	shl	%r0,%r0,%r0
 218:	8e 32 e8 48 	d.mim1p2.sd	%f6,%f7,%f8
 21c:	00 00 00 a0 	shl	%r0,%r0,%r0
 220:	8e 23 c8 48 	d.mim1p2.dd	%f4,%f6,%f8
 224:	00 00 00 a0 	shl	%r0,%r0,%r0
 228:	0f be 19 4b 	d.m12tpa.ss	%f23,%f24,%f25
 22c:	00 00 00 a0 	shl	%r0,%r0,%r0
 230:	8f 4e 4b 49 	d.m12tpa.sd	%f9,%f10,%f11
 234:	00 00 00 a0 	shl	%r0,%r0,%r0
 238:	8f 37 0a 49 	d.m12tpa.dd	%f6,%f8,%f10
 23c:	00 00 00 a0 	shl	%r0,%r0,%r0
