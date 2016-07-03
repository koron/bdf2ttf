/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * bdf2ttf.h - BDF to TTF converter.
 *
 * Written By:	MURAOKA Taro <koron.kaoriya@gmail.com>
 * Last Change: 03-Jul-2016.
 */
#ifndef BDF2TTF_H
#define BDF2TTF_H

#ifdef __cplusplus
# define BDF2TTF_EXTERN extern "C"
#else
# define BDF2TTF_EXTERN extern
#endif

BDF2TTF_EXTERN const char	*g_copyright;
BDF2TTF_EXTERN const char	*g_copyright_cp;
BDF2TTF_EXTERN const char	*g_fontname;
BDF2TTF_EXTERN const char	*g_fontname_cp;
BDF2TTF_EXTERN const char	*g_style;
BDF2TTF_EXTERN const char	*g_version;
BDF2TTF_EXTERN const char	*g_version_cp;
BDF2TTF_EXTERN const char	*g_trademark;
BDF2TTF_EXTERN const char	*g_trademark_cp;

#ifdef __cplusplus
extern "C" {
#endif

int emCalc(int pix, int base);
int write_ttf(bdf2_t* font, char* ttfname);

#ifdef __cplusplus
}
#endif

#endif /* BDF2TTF_H */
