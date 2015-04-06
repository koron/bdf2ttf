/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * ucsconv.h - Simple encoding converter.
 *
 * Written By:	MURAOKA Taro <koron@tka.att.ne.jp>
 * Last Change: 05-Aug-2003.
 */
#ifndef UCSCONV_H
#define UCSCONV_H

#define UCSCHAR_T_MAX 0x10000

typedef unsigned short ucschar_t;
typedef struct _ucsconv_t
{
    ucschar_t fromUCS[UCSCHAR_T_MAX];
    ucschar_t toUCS[UCSCHAR_T_MAX];
} ucsconv_t;

#ifdef __cplusplus
extern "C" {
#endif

ucsconv_t*	ucsconv_open();
void		ucsconv_close(ucsconv_t* conv);
int		ucsconv_load(ucsconv_t* conv, char *filename);
ucschar_t	ucsconv_toUCS(ucsconv_t* conv, ucschar_t ch);
ucschar_t	ucsconv_fromUCS(ucsconv_t* conv, ucschar_t ch);

#ifdef __cplusplus
}
#endif

#endif /* UCSCONV_H */
