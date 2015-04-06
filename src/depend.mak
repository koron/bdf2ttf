# vim:set ts=8 sts=8 sw=8 tw=0:
#
# 構成ファイルと依存関係
#
# Last Change:	10-Oct-2003.
# Written By:	MURAOKA Taro <koron@tka.att.ne.jp>

##############################################################################
# 構成ファイル
#
SRCS = $(srcdir)bdf.c		\
       $(srcdir)bdf2ttf.cpp	\
       $(srcdir)main.c		\
       $(srcdir)rcfile.c	\
       $(srcdir)table.cpp	\
       $(srcdir)ucsconv.c

OBJS = $(objdir)bdf.$(O)	\
       $(objdir)bdf2ttf.$(O)	\
       $(objdir)main.$(O)	\
       $(objdir)rcfile.$(O)	\
       $(objdir)table.$(O)	\
       $(objdir)ucsconv.$(O)

LIBS =

TARGET = $(outdir)bdf2ttf$(EXE)

HDRS = $(srcdir)bdf.h $(srcdir)rcfile.h $(srcdir)table.h $(srcdir)ucsconv.h

##############################################################################
# フラグ
#
CCFLAGS  = $(CFLAGS) $(DEFS) $(INCDIRS)
CPPFLAGS = $(CFLAGS) $(DEFS) $(INCDIRS)
LDFLAGS  = $(LFLAGS) $(LIBDIRS)

##############################################################################
# 依存関係の設定
#
$(objdir)bdf.$(O): $(srcdir)bdf.c  $(srcdir)bdf.h $(srcdir)ucsconv.h $(srcdir)debug.h

$(objdir)bdf2ttf.$(O): $(srcdir)bdf2ttf.cpp $(srcdir)version.h $(srcdir)table.h $(srcdir)bdf.h $(srcdir)bdf2ttf.h $(srcdir)debug.h

$(objdir)main.$(O): $(srcdir)main.c $(srcdir)rcfile.h $(srcdir)bdf.h $(srcdir)bdf2ttf.h $(srcdir)debug.h

$(objdir)rcfile.$(O): $(srcdir)rcfile.c  $(srcdir)rcfile.h

$(objdir)table.$(O): $(srcdir)table.cpp  $(srcdir)table.h

$(objdir)ucsconv.$(O): $(srcdir)ucsconv.c  $(srcdir)ucsconv.h
