# vim:set ts=8 sts=8 sw=8 tw=0:
#
# Šî–{ˆË‘¶ŠÖŒW
#
# Last Change:	06-Jun-2003.
# Written By:	MURAOKA Taro <koron@tka.att.ne.jp>

TAGS = tags

$(TARGET): $(OBJS)

#tags: $(TAGS)

$(TAGS): $(SRCS) $(HDRS)
	$(CTAGS) -f $@ $(SRCS) $(HDRS)

clean-compile:
	-$(RM) $(OBJS)

distclean-compile: clean-compile
	-$(RM) $(TARGET)
	-$(RM) $(TAGS)
