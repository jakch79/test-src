#as: --isa=shmedia --abi=64 --no-exp -big
#objdump: -s
#name: SH64 Big Endian

.*:     file format elf64-sh64.*

Contents of section .text:
 0000 cc48d000 12345678 12340000.*


