SCRIPT_NAME=elf
ELFSIZE=32
OUTPUT_FORMAT="elf32-hppa"
TEXT_START_ADDR=0x1000
TARGET_PAGE_SIZE=0x1000
MAXPAGESIZE=0x1000
ARCH=hppa
MACHINE=hppa1.1		# We use 1.1 specific features.
NOP=0x08000240
START="_start"
OTHER_READONLY_SECTIONS='.PARISC.unwind : { *(.PARISC.unwind) } '
DATA_START_SYMBOLS='PROVIDE ($global$ = .);'
DATA_PLT=
GENERATE_SHLIB_SCRIPT=yes
TEMPLATE_NAME=elf32
EXTRA_EM_FILE=hppaelf
