# vim:set ts=8 sts=8 sw=8 tw=0:
#
# デフォルトコンフィギュレーションファイル
#
# Last Change:	03-Jan-2003.
# Written By:	MURAOKA Taro <koron@tka.att.ne.jp>

srcdir = ./src/
objdir = ./src/
outdir = ./

##############################################################################
# インストールディレクトリの設定
#
prefix		= /usr/local
bindir		= $(prefix)/bin
libdir		= $(prefix)/lib
incdir		= $(prefix)/include
# 警告: $(ucstabledir)はアンインストール実行時にディレクトリごと消去されます。
ucstabledir	= $(prefix)/share/bdf2ttf

##############################################################################
# コマンド設定
#
RM		= rm -f
CP		= cp
MKDIR		= mkdir -p
RMDIR		= rm -rf
CTAGS		= ctags
INSTALL		= /usr/bin/install -c
INSTALL_PROGRAM	= $(INSTALL) -m 755
INSTALL_DATA	= $(INSTALL) -m 644

##############################################################################
# 定数
#
O = o
EXE =
