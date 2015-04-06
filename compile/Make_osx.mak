CFLAGS =
LFLAGS =

default: target

include config.mk
include compile/osx.mak
include src/depend.mak
include compile/depend.mak

target: $(TARGET)

$(TARGET):
	g++ -o $@ $(LIBS) $^ $(LDFLAGS)

.c.o:
	$(CC) $(CCFLAGS) -c -o $@ $<

.cpp.obj:
	$(CPP) $(CPPFLAGS) -c -o $@ $<

clean: clean-compile

distclean: distclean-compile
