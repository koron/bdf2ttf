/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * Last Change: 05-Aug-2003.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#ifndef RCFILE_H
#define RCFILE_H

#ifndef RC_MAXLINE_LEN
# define RC_MAXLINE_LEN 512
#endif

typedef struct _rcfile_t rcfile_t;

#ifdef __cplusplus
# define EXTERN extern "C"
#else
# define EXTERN
#endif

EXTERN rcfile_t*	rcfile_open(const char* filename);
EXTERN void		rcfile_close(rcfile_t* rcfile);
EXTERN const char*	rcfile_get(rcfile_t* rcfile, const char* key);

#endif /* RCFILE_H */
