@echo off
if "%1" == "h8/300" goto h8300

echo Configuring gas for go32
update config/tc-i386.c targ-cpu.c
update config/tc-i386.h targ-cpu.h
update config/te-go32.h targ-env.h
update config/obj-coff.h obj-format.h
update config/obj-coff.c obj-format.c
update config/atof-ieee.c atof-targ.c
goto common

:h8300
echo Configuring gas for H8/300
copy config\ho-go32.h host.h
copy config\tc-h8300.c targ-cpu.c
copy config\tc-h8300.h targ-cpu.h
copy config\te-generic.h targ-env.h
copy config\objcoffbfd.h obj-format.h
copy config\objcoffbfd.c obj-format.c
copy config\atof-ieee.c atof-targ.c

:common

echo # Makefile generated by "configure.bat"> Makefile.2
echo all.dos : as.new gasp.new>> Makefile.2

if exist config.sed del config.sed

echo "s/@srcdir@/./g					">> config.sed
echo "s/@target_alias@/go32/				">> config.sed
echo "s/@prefix@//					">> config.sed
echo "s/@CC@/gcc/g					">> config.sed
echo "s/@OPCODES_LIB@/..\/opcodes\/libopcodes.a/g	">> config.sed
echo "s/@BFDLIB@/..\/bfd\/libbfd.a/g			">> config.sed
echo "s/@ALL_OBJ_DEPS@/..\/bfd\/bfd.h/g			">> config.sed

echo "/^all[ 	]*:/ a\					">> config.sed
echo "dummy:						">> config.sed

echo "s/\/usr[^ ]*.h//g					">> config.sed

echo "/^config.h[ 	]*:/ d				">> config.sed
echo "s/^Makefile/not-Makefile/				">> config.sed

sed -e "s/^\"//" -e "s/\"$//" -e "s/[ 	]*$//" config.sed > config2.sed
sed -f config2.sed Makefile.in >> Makefile.2
update Makefile.2 Makefile
del Makefile.2
del config.sed
del config2.sed

echo #ifndef GAS_VERSION> config.new
sed -n "/^VERSION=/p" Makefile.in | sed -e "s/VERSION=/#define GAS_VERSION \"/" -e "s/$/\"/">> config.new
type config\go32.cfg >> config.new
echo #endif>> config.new
update config.new config.h
