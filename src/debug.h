/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * debug.h -
 *
 * Written By:	MURAOKA Taro <koron.kaoriya@gmail.com>
 * Last Change:	06-Apr-2015.
 */

#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
# define TRACE		printf
#else
# define TRACE		1 ? (int)1 : printf
#endif

#endif /* DEBUG_H */
