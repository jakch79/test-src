#objdump: -r 
#name: Relaxation of conditional branches

.*: +file format.*elf32-[am33lin|mn10300].*

RELOCATION RECORDS FOR \[.rlcb\]:
OFFSET   TYPE              VALUE 
0+8003 R_MN10300_PCREL8  .L0._0\+0x00000001
0+8005 R_MN10300_PCREL32  .L1\+0x00000001

RELOCATION RECORDS FOR \[.rlfcb\]:
OFFSET   TYPE              VALUE 
0+8004 R_MN10300_PCREL8  .L0._1\+0x00000002
0+8006 R_MN10300_PCREL32  .L2\+0x00000001

RELOCATION RECORDS FOR \[.rscb\]:
OFFSET   TYPE              VALUE 
0+103 R_MN10300_PCREL8  .L0._2\+0x00000001
0+105 R_MN10300_PCREL16  .L3\+0x00000001

RELOCATION RECORDS FOR \[.rsfcb\]:
OFFSET   TYPE              VALUE 
0+104 R_MN10300_PCREL8  .L0._3\+0x00000002
0+106 R_MN10300_PCREL16  .L4\+0x00000001

RELOCATION RECORDS FOR \[.rsucb\]:
OFFSET   TYPE              VALUE 
0+104 R_MN10300_PCREL8  .L0._4\+0x00000002
0+106 R_MN10300_PCREL16  .L5\+0x00000001

RELOCATION RECORDS FOR \[.rlucb\]:
OFFSET   TYPE              VALUE 
0+8004 R_MN10300_PCREL8  .L0._5\+0x00000002
0+8006 R_MN10300_PCREL32  .L6\+0x00000001


