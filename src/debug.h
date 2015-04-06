/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * debug.h -
 *
 * Written By:	MURAOKA Taro <koron@tka.att.ne.jp>
 * Last Change:	03-Aug-2003.
 */

#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
# define TRACE		printf
#else
# define TRACE		1 ? (int)1 : printf
#endif

#endif /* DEBUG_H */
