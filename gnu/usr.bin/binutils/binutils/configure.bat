@echo off
if "%1" == "h8/300" goto h8300

echo Configuring binutils for go32
update ../bfd/hosts/go32.h sysdep.h
goto common

:h8300
echo Configuring binutils for H8/300
update ..\bfd\hosts\h-go32.h sysdep.h

:common

echo # Makefile generated by "configure.bat"> Makefile

if exist config.sed del config.sed

sed -n "/^VERSION=/ p" Makefile.in | sed -e "s/^/s^/" -e "s/=/^\"/" -e "s/$/\"^/" > config.sed
sed -f config.sed version.c > version2.c

if exist config.sed del config.sed

echo "s/version\./version2\./g				">> config.sed
echo "s/-DVERSION=[^ ]* //				">> config.sed

echo "s/^	\$(srcdir)\/move-if-change/	update/	">> config.sed
echo "/^###$/ i\					">> config.sed
echo "CC = gcc						">> config.sed
echo "s/:\([^ 	]\)/: \1/g				">> config.sed
echo "s/^	\ *\.\//	go32 /			">> config.sed
echo "s/`echo \$(srcdir)\///g				">> config.sed
echo "s/ | sed 's,\^\\\.\/,,'`//g			">> config.sed
echo "s/^	cd \$(srcdir)[ 	]*;//			">> config.sed

echo "/^arparse\.c/ i\					">> config.sed
echo "arparse.o: arparse.c\				">> config.sed
echo "	$(CC) -c $(CFLAGS) $(INCLUDES) $(HDEFINES) $(TDEFINES) arparse.c ">> config.sed
echo "/\$(BISON)/ c\					">> config.sed
echo "	bison $(BISONFLAGS) -o $@ arparse.y		">> config.sed
echo "/y\.tab\./ d					">> config.sed

echo "/^arlex.c/ {					">> config.sed
echo " i\						">> config.sed
echo "arlex.o: arlex.c					">> config.sed
echo " i\						">> config.sed
echo "	$(CC) -c $(CFLAGS) $(INCLUDES) $(HDEFINES) $(TDEFINES) arlex.c ">> config.sed
echo "}							">> config.sed
echo "/\$(LEX)/ c\					">> config.sed
echo "	flex $(LEX_OPTIONS) arlex.l			">> config.sed
echo "s/lex\.yy\./lexyy./g				">> config.sed

echo "s/'"/\\"/g					">> config.sed
echo "s/"'/\\"/g					">> config.sed

echo "s/c++filt/cxxfilt/g				">> config.sed

sed -e "s/^\"//" -e "s/\"$//" -e "s/[ 	]*$//" config.sed > config2.sed
sed -f config2.sed Makefile.in >> Makefile
del config.sed
del config2.sed

echo int prepends_underscore = 1; > underscore.c

