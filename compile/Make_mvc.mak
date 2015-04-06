!ifdef DEBUG
CFLAGS = -nologo -W3 -Zi -DDEBUG=1
LFLAGS = -nologo -Zi
!else
CFLAGS = -nologo -O2 -W3 -MD
LFLAGS = -nologo
!endif

default: target

!include config.mk
!include compile\dos.mak
!include src\depend.mak
!include compile\depend.mak

target: $(TARGET)

$(TARGET):
	$(CC) -Fe$@ $(LIBS) $** $(LDFLAGS)

.c.obj:
	$(CC) $(CCFLAGS) -c -Fo$@ $<

.cpp.obj:
	$(CPP) $(CPPFLAGS) -c -Fo$@ $<

clean: clean-compile
	-$(RM) *.pdb

distclean: distclean-compile
	-$(RM) *.pdb
	-$(RM) *.ilk
