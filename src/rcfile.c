/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * Last Change: 05-Aug-2003.
 * Written By:  MURAOKA Taro <koron@tka.att.ne.jp>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "rcfile.h"

typedef struct _rcentry_t rcentry_t;
struct _rcentry_t
{
    char* key;
    char* value;
    rcentry_t* next;
};

struct _rcfile_t
{
    int entry_num;
    rcentry_t* entry_ptr;
};

    static int
is_whitespace(int c)
{
#if 1
    return isspace(c);
#else
    switch (c)
    {
	default:
	    return 0;
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0c:
	case 0x0d:
	case 0x20:
	    return 1;
    }
#endif
}

    static rcentry_t**
get_entry(rcfile_t* rcfile, const char* key)
{
    rcentry_t **p = &rcfile->entry_ptr;

    while (*p)
    {
	if (!strcmp((**p).key, key))
	    break;
	p = &(**p).next;
    }
    return p;
}

/*
 * Parse rcfile by eachline
 */
    static void
rcfile_load_fp(rcfile_t* rcfile, FILE* fp)
{
    char buf[RC_MAXLINE_LEN];

    if (!rcfile)
	return;
    while (fgets(buf, sizeof(buf), fp))
    {
	unsigned char *p = buf;
	unsigned char *key, *value, *tail;
	rcentry_t **entry;

	while (is_whitespace(*p)) /* Skip leading white space */
	    ++p;
	if (*p == '#' || *p == '\r' || *p == '\n')
	    continue;

	/* Obtain key */
	key = p;
	p = strchr(p, '='); /* Find key,value separator */
	if (!p || key == p)
	    continue;
	/* keyの末尾の余分な空白は削除する */
	tail = p++;
	do
	{
	    *tail = '\0';
	}
	while (is_whitespace(*--tail));
	/* pは'='の次の文字を指している */

	/* Obtain value */
	while (is_whitespace(*p)) /* Skip leading white space */
	    ++p;
	value = p;
	for (; *p; ++p)
	    if (strchr("\r\n", *p))
		break;
	tail = p;
	do
	{
	    *tail = '\0';
	}
	while (is_whitespace(*--tail));

	/*printf("key=%s value=%s\n", key, value);*/
	/*
	printf("keys=%s value=", key);
	{
	    int i = 0;
	    while (value[i])
	    {
		printf("%02x ", (unsigned char)value[i]);
		++i;
	    }
	}
	printf("\n");
	*/

	entry = get_entry(rcfile, key);
	if (!*entry)
	{
	    if (!(*entry = malloc(sizeof(rcentry_t))))
		break;
	    (**entry).key	= strdup(key);
	    (**entry).value	= NULL;
	    (**entry).next	= NULL;
	}
	free((**entry).value);
	(**entry).value = strdup(value);
    }
}

    rcfile_t*
rcfile_open(const char* filename)
{
    FILE *fp;
    rcfile_t *rcfile = NULL;

    if (!(fp = fopen(filename, "rt")))
	return NULL;
    if (!(rcfile = (rcfile_t*)malloc(sizeof(rcentry_t))))
	return NULL;
    rcfile->entry_num = 0;
    rcfile->entry_ptr = 0;

    /* rcfileの解釈 */
    rcfile_load_fp(rcfile, fp);

    return rcfile;
}

    void
rcentry_close(rcentry_t* rcentry)
{
    rcentry_t *p, *next;
    for (p = rcentry; p; p = next)
    {
	next = p->next;
	free(p->key);
	free(p->value);
	free(p);
    }
}

    void
rcfile_close(rcfile_t* rcfile)
{
    if (!rcfile)
	return;
    rcentry_close(rcfile->entry_ptr);
}

    const char*
rcfile_get(rcfile_t* rcfile, const char* key)
{
    rcentry_t **p = get_entry(rcfile, key);
    return *p ? (**p).value : NULL;
}

#if 0 && !defined(_USRDLL) && !defined(_LIB)
/*
 * テスト用main()
 */
    int
main(int argc, char** argv)
{

    if (argc > 1)
    {
	rcfile_t *rcfile = NULL;

	rcfile = rcfile_open(argv[1]);
	if (rcfile)
	{
	    int i;

	    for (i = 2; i < argc; ++i)
	    {
		printf("[%d] key=%s value=%s\n", i - 1, argv[i],
			rcfile_get(rcfile, argv[i]));
	    }
	    rcfile_close(rcfile);
	}
    }
}
#endif
