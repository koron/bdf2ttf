# vim:set ts=8 sts=8 sw=8 tw=0:
#
# bdf2ttf
#
# Last Change: 23-Sep-2002.
# Written By:  MURAOKA Taro <koron@tka.att.ne.jp>

default: usage

usage:

##############################################################################
# for Borland C 5
#
bc: bc-rel
bc-rel:
	$(MAKE) -f compile\Make_bc5.mak
bc-clean:
	$(MAKE) -f compile\Make_bc5.mak clean
bc-distclean:
	$(MAKE) -f compile\Make_bc5.mak distclean

##############################################################################
# for Cygwin
#
cyg: cyg-rel
cyg-rel:
	$(MAKE) -f compile/Make_cyg.mak
cyg-install: cyg-all
	$(MAKE) -f compile/Make_cyg.mak install
cyg-uninstall:
	$(MAKE) -f compile/Make_cyg.mak uninstall
cyg-clean:
	$(MAKE) -f compile/Make_cyg.mak clean
cyg-distclean:
	$(MAKE) -f compile/Make_cyg.mak distclean

##############################################################################
# for GNU/gcc (Linux and others)
#	(Tested on Vine Linux 2.1.5)
#
gcc: gcc-rel
gcc-rel:
	$(MAKE) -f compile/Make_gcc.mak
gcc-install: gcc-all
	$(MAKE) -f compile/Make_gcc.mak install
gcc-uninstall:
	$(MAKE) -f compile/Make_gcc.mak uninstall
gcc-clean:
	$(MAKE) -f compile/Make_gcc.mak clean
gcc-distclean:
	$(MAKE) -f compile/Make_gcc.mak distclean

##############################################################################
# for Microsoft Visual C
#
msvc: msvc-rel
msvc-rel:
	$(MAKE) /nologo /f compile\Make_mvc.mak
msvc-dbg:
	$(MAKE) /nologo /f compile\Make_mvc.mak DEBUG=yes
msvc-clean:
	$(MAKE) /nologo /f compile\Make_mvc.mak clean
msvc-distclean:
	$(MAKE) /nologo /f compile\Make_mvc.mak distclean

##############################################################################
# for MacOS X
#
osx: osx-rel
osx-rel:
	$(MAKE) -f compile/Make_osx.mak
osx-install: osx-all
	$(MAKE) -f compile/Make_osx.mak install
osx-uninstall:
	$(MAKE) -f compile/Make_osx.mak uninstall
osx-clean:
	$(MAKE) -f compile/Make_osx.mak clean
osx-distclean:
	$(MAKE) -f compile/Make_osx.mak distclean

##############################################################################
# for Sun's Solaris/gcc
#	(Tested on Solaris 8)
#
sun: sun-rel
sun-rel:
	$(MAKE) -f compile/Make_sun.mak
sun-install: sun-all
	$(MAKE) -f compile/Make_sun.mak install
sun-uninstall:
	$(MAKE) -f compile/Make_sun.mak uninstall
sun-clean:
	$(MAKE) -f compile/Make_sun.mak clean
sun-distclean:
	$(MAKE) -f compile/Make_sun.mak distclean

