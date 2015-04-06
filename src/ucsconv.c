/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * ucsconv.c - Simple encoding converter.
 *
 * Written By:	MURAOKA Taro <koron@tka.att.ne.jp>
 * Last Change: 05-Aug-2003.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "ucsconv.h"

#ifdef UCSCONV_TEST
    int
main(int argc, char** argv)
{
    ucsconv_t *conv = ucsconv_open();
    ucsconv_load(conv, "JISX0212.WIN.TXT");
    ucsconv_close(conv);
    return 0;
}
#endif

    ucsconv_t*
ucsconv_open()
{
    ucsconv_t *conv;
    conv = (ucsconv_t*)calloc(1, sizeof(ucsconv_t));
    return conv;
}

    void
ucsconv_close(ucsconv_t* conv)
{
    free(conv);
}

    ucschar_t
ucsconv_toUCS(ucsconv_t* conv, ucschar_t ch)
{
    return conv->toUCS[ch];
}

    ucschar_t
ucsconv_fromUCS(ucsconv_t* conv, ucschar_t ch)
{
    return conv->fromUCS[ch];
}

    static char*
is_valid(char* p)
{
    while (isspace(*p))
	++p;
    if (*p == '#')
	return NULL;
    return p;
}

    static int
ucsconv_load_fh(ucsconv_t* conv, FILE* fp)
{
    char buf[8192];
    while (fgets(buf, sizeof(buf), fp))
    {
	char *p = is_valid(buf);
	int from, to;
	if (!p)
	    continue;
	if (sscanf(p, "%x %x", &from, &to) >= 2)
	{
	    ucschar_t f = (ucschar_t)(from & (UCSCHAR_T_MAX - 1));
	    ucschar_t t = (ucschar_t)(to & (UCSCHAR_T_MAX - 1));
	    conv->toUCS[f] = t;
	    conv->fromUCS[t] = f;
#if 0
	    printf("%04x -> %04x\n", from, to);
#endif
	}
    }
    return 1;
}

    int
ucsconv_load(ucsconv_t* conv, char *filename)
{
    if (conv)
    {
	FILE *fp = fopen((filename), "rt");
	if (fp)
	{
	    int retval = ucsconv_load_fh(conv, fp);
	    fclose(fp);
	    return retval;
	}
    }
    return 0;
}
