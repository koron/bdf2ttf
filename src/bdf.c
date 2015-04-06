/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * bdf.c -
 *
 * Written By:	MURAOKA Taro <koron@tka.att.ne.jp>
 * Last Change:	09-Oct-2003.
 */

//#define USE_FLEXIBLE_GLYPHWIDTH

#ifndef UCS_TABLEDIR
# define UCS_TABLEDIR "ucstable.d"
#endif
#define USE_PIXELSIZE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bdf.h"
#include "ucsconv.h"
#include "debug.h"

#define RAS_BYTE(nbit) (((nbit) + 7) >> 3)

/* Count up entries in the array */
#ifndef elementof
# define elementof(a) (sizeof(a) / sizeof(a[0]))
#endif

/* Define _MAX_PATH if wasn't defined. */
#ifndef _MAX_PATH
# if defined(FILENAME_MAX)
#  define _MAX_PATH FILENAME_MAX
# endif
#endif

#ifdef _WIN32
# define QSORT_CALLBACK __cdecl
#else
# define QSORT_CALLBACK
#endif

typedef struct
{
    char*   name;
    unsigned char width;
} width_table_t;

static width_table_t cell_width_table[] = {
    { "JISX0208",	2 },
    { "JISX0212",	2 },
    { "JISX0213",	2 },
    { NULL,		0 },
};

static int	bdf_load_fh(bdf_t* font, FILE* fp);
static void	count_validglyph(bdf_t* font);
static bdf_glyph_t*	glyph_open();
static void		glyph_close(bdf_glyph_t *glyph);
static char*	iscmd(char* target, char* keyword);
static int	atoi_next(char** str);
static int	hex2n(char ch);
static int	hex2byte(char *str);
static void	chomp(char* str);
static int	check_bdfsize(char* filename);

    bdf_t*
bdf_open()
{
    bdf_t *font;

    font = (bdf_t*)calloc(1, sizeof(*font));
    font->indexFirst = -1;
    font->indexLast  = -1;
    memset(font->cell_width, 1, sizeof(font->cell_width));
    return font;
}

    void
bdf_close(bdf_t *font)
{
    if (font)
    {
	int i;

	for (i = 0; i < BDF_MAX_GLYPH; ++i)
	{
	    if (font->glyph[i])
		glyph_close(font->glyph[i]);
	}
	free(font);
    }
}

    bdf_glyph_t*
bdf_get_glyph(bdf_t* font, int id)
{
    return font ? font->glyph[id] : NULL;
}

    int
bdf_get_pixel_glyph(bdf_glyph_t* glyph, int x, int y)
{
    if (glyph && x >= 0 && y >= 0
	    && x < glyph->bbx.width
	    && y < glyph->bbx.height
	    && glyph->bitmap
       )
    {
	int base = RAS_BYTE(glyph->bbx.width) * y;
	return glyph->bitmap[base + (x / 8)] & (0x80 >> (x % 8))
	    ? 1 : 0;
    }
    return -1;
}

    int
bdf_get_pixel(bdf_t * font, int id, int x, int y)
{
    if (font)
    {
	bdf_glyph_t* glyph = bdf_get_glyph(font, id);
	return bdf_get_pixel_glyph(glyph, x, y);
    }
    return -1;
}

    static bdf_glyph_t *
glyph_open(int width, int height)
{
    bdf_glyph_t * glyph;
    size_t size;

    glyph = (bdf_glyph_t*)calloc(1, sizeof(*glyph));
    size = RAS_BYTE(width) * height;
    glyph->bitmap = (bdf_byte_t*)calloc(size, sizeof(bdf_byte_t));
    return glyph;
}

    static void
glyph_close(bdf_glyph_t *glyph)
{
    if (glyph)
    {
	if (glyph->bitmap)
	    free(glyph->bitmap);
	free(glyph);
    }
}

    static char*
skipspace(char* str)
{
    while (isspace(*str))
	++str;
    return str;
}

    static char*
iscmd(char* target, char* keyword)
{
    size_t len = strlen(keyword);
    return strncmp(target, keyword, len) == 0 && !isgraph(target[len])
	? skipspace(target + len) : NULL;
}

    void
chomp(char* str)
{
    int len = strlen(str) - 1;
    for (; len >= 0 && isspace(str[len]); --len)
	str[len] = '\0';
}

    int
atoi_next(char** str)
{
    int retval = atoi(*str);
    while (isdigit(**str))
	++*str;
    *str = skipspace(*str);
    return retval;
}

    int
hex2n(char ch)
{
    if (ch >= '0' && ch <= '9')
	return (int)(ch - '0');
    else if (ch >= 'a' && ch <= 'f')
	return (int)(ch - 'a' + 10);
    else if (ch >= 'A' && ch <= 'F')
	return (int)(ch - 'A' + 10);
    else
	return 0;
}

    int
hex2byte(char *str)
{
    return (hex2n(str[0]) << 4) + hex2n(str[1]);
}

    void
count_validglyph(bdf_t* font)
{
    int i;

    font->numGlyph = 0;
    for (i = 0; i < BDF_MAX_GLYPH; ++i)
	if (font->glyph[i])
	{
	    font->glyph[i]->id = font->numGlyph;
	    ++font->numGlyph;

	    if (font->indexFirst < 0)
		font->indexFirst = i;
	    font->indexLast = i;
	}
}

    static char*
parse_FONT(char* cmd)
{
    int c = 13;
    int len;

    while (c && *cmd)
	if (*cmd++ == '-')
	    --c;
    len = strlen(cmd);
    if (len > 0)
    {
	int i;

	for (i = 0; i < len; ++i)
	{
	    if (cmd[i] == '.')
	    {
		/* Remove year notification, and plane 0 */
		int j, n;

		for (j = i + 1; cmd[j] != '\0' && cmd[j] != '-'; ++j)
		    ;
		if (cmd[j] == '-' && (n = atoi(&cmd[j + 1])) > 0)
		{
		    while (cmd[j] != '\0')
			cmd[i++] = cmd[j++];
		}
		cmd[i] = '\0';
		break;
	    }
	    cmd[i] = toupper(cmd[i]);
	}
	/* Remove prefix "ISO", if exists. */
	if (strncmp(cmd, "ISO", 3) == 0)
	    cmd += 3;
    }
    return cmd;
}

    static ucsconv_t*
init_conv(char* encname, ucsconv_t** pconv)
{
    ucsconv_t *conv;

    if (conv = ucsconv_open())
    {
	int i;
	char *postfix[] = { ".TXT", ".WIN.TXT" };
	int loaded = 0;

	/* Generate encode table filename */
	for (i = 0; i < elementof(postfix); ++i)
	{
	    int n;
	    char encfile[_MAX_PATH];
	    char *env = getenv("UCSTABLEDIR");
	    sprintf(encfile, "%s/%s%s", env ? env : UCS_TABLEDIR,
		    encname, postfix[i]);
	    n = ucsconv_load(conv, encfile);
	    loaded += n;
#if 0
	    printf ("  [%d] %s (%d)\n", i, encfile, n);
#endif
	}
	if (loaded == 0)
	{
	    ucsconv_close(conv);
	    conv = NULL;
	}
    }

    if (pconv)
    {
	if (*pconv)
	    ucsconv_close(*pconv);
	*pconv = conv;
    }
    return conv;
}

/*
 * Return 1 if succeeded.
 * Return -SIZE if failed.
 */
    int
bdf_load_fh(bdf_t * font, FILE* fp)
{
    bdf_glyph_t tmp, *ptmp = NULL;
    int predat = 1;
    int encnum = 0;
    int ras = 0, line = 0;
    char buf[8192], *cmd;
    ucsconv_t* conv = NULL;
    unsigned char cell_width = 1;

    memset(&tmp, 0, sizeof(tmp));
    while (fgets(buf, sizeof(buf), fp))
    {
	chomp(buf);
	if (predat)
	{
	    /* Pre data section parser */
	    if (cmd = iscmd(buf, "CHARS"))
		predat = 0;
	    else if (cmd = iscmd(buf, "SIZE"))
	    {
		int size = atoi_next(&cmd);
		if (font->size != size)
		{
		    if (font->verbose > 0)
			fprintf(stderr,
				"WARNING: SIZE was changed (%d -> %d)\n",
				font->size, size);
		    if (font->size < size)
			font->size = size;
		}
	    }
	    else if (cmd = iscmd(buf, "PIXEL_SIZE"))
	    {
		int pixel_size = atoi_next(&cmd);
		font->pixel_size = pixel_size;
	    }
	    else if (cmd = iscmd(buf, "FONTBOUNDINGBOX"))
	    {
		font->bbx.width = atoi_next(&cmd);
		font->bbx.height = atoi_next(&cmd);
		font->bbx.offset.x = atoi_next(&cmd);
		font->bbx.offset.y = atoi_next(&cmd);
	    }
	    else if (cmd = iscmd(buf, "FONT"))
	    {
		if (cmd = parse_FONT(cmd))
		{
		    width_table_t *p;

		    TRACE("bdf_load: encoding is %s\n", cmd);
		    init_conv(cmd, &conv);
		    for (p = cell_width_table; p->name != NULL; ++p)
			if (strcmp(p->name, cmd) == 0)
			    break;
		    if (p->name != NULL)
			cell_width = p->width;
		}
		else if (conv)
		{
		    ucsconv_close(conv);
		    conv = NULL;
		}
	    }
	    else if (cmd = iscmd(buf, "FONT_ASCENT"))
		font->ascent = atoi(cmd);
	    else if (cmd = iscmd(buf, "FONT_DESCENT"))
		font->descent = atoi(cmd);
	}
	else if (ptmp)
	{
	    /* Parse bitmap */
	    if (cmd = iscmd(buf, "ENDCHAR"))
	    {
		if (encnum >= 0)
		{
		    if (font->glyph[encnum])
			glyph_close(font->glyph[encnum]);
		    font->glyph[encnum] = ptmp;
		    //font->cell_width[encnum] = cell_width;
		    font->cell_width[encnum] = ptmp->dwidth.x > (font->size / 2)
			? 2 : 1;
		}
		else
		    glyph_close(ptmp);
		ptmp = NULL;
	    }
	    else
	    {
		int i;
		bdf_byte_t *p = ptmp->bitmap + ras * line;
#if 0
		if (line == 5)
		{
		    fprintf(stderr, "(%s)", buf);
		    for (i = 0; i < ras; ++i)
			fprintf(stderr, " %02X", hex2byte(buf + i * 2));
		    fprintf(stderr, "\n");
		}
#endif
		for (i = 0; i < ras; ++i)
		    p[i] = hex2byte(buf + i * 2);
		++line;
	    }
	}
	/* Parse font glyph headers */
	else if (cmd = iscmd(buf, "ENCODING"))
	{
	    /* Convert encondig in UCS.  When encondig wasn't converted, use
	     * original value */
	    encnum = atoi(cmd);
	    if (conv)
	    {
		int convnum = (int)ucsconv_toUCS(conv, (ucschar_t)encnum);
		if (convnum)
		    encnum = convnum;
		else
		{
		    char *filename = "STREAM";
		    if (font->loadingFilename)
			filename = font->loadingFilename;
		    if (font->verbose > 0)
		    {
			fprintf(stderr, "%04x can't convert to UCS (%s)\n",
				encnum, filename);
		    }
		    encnum = -1;
		}
	    }
	}
	else if (cmd = iscmd(buf, "SWIDTH"))
	{
	    tmp.swidth.x = atoi_next(&cmd);
	    tmp.swidth.y = atoi_next(&cmd);
	}
	else if (cmd = iscmd(buf, "DWIDTH"))
	{
	    tmp.dwidth.x = atoi_next(&cmd);
	    tmp.dwidth.y = atoi_next(&cmd);
	}
	else if (cmd = iscmd(buf, "BBX"))
	{
	    tmp.bbx.width = atoi_next(&cmd);
	    tmp.bbx.height = atoi_next(&cmd);
	    tmp.bbx.offset.x = atoi_next(&cmd);
	    tmp.bbx.offset.y = atoi_next(&cmd);
	}
	else if (cmd = iscmd(buf, "BITMAP"))
	{
	    ptmp = glyph_open(tmp.bbx.width, tmp.bbx.height);
	    tmp.bitmap = ptmp->bitmap;
	    *ptmp = tmp;

	    ras = RAS_BYTE(ptmp->bbx.width);
	    line = 0;
	}
    }
    if (font->pixel_size == 0)
    {
	font->pixel_size = font->size;
	fprintf(stderr, "bdf_load: PIXEL_SIZE was not found, use %d\n", font->size);
    }
    /* Remove a glyph for charcode 0 */
    if (font->glyph[0])
    {
	glyph_close(font->glyph[0]);
	font->glyph[0] = NULL;
    }
    /* Caliculate glyph statistics information */
    count_validglyph(font);
    return 1;
}

    int
bdf_load(bdf_t* font, char* filename)
{
    FILE *fp;
    int retval = 0;

    if (!font)
	return retval;
    fp = fopen(filename, "rb");
    if (fp)
    {
	font->loadingFilename = filename;
	retval = bdf_load_fh(font, fp);
	font->loadingFilename = NULL;
	fclose(fp);
    }
    return retval;
}

/*****************************************************************************
 * BDF2
 *****************************************************************************/

    bdf2_t*
bdf2_open()
{
    bdf2_t *font;
    int i;

    font = (bdf2_t*)calloc(1, sizeof(*font));
    for (i = 0; i < BDF_MAX_GLYPH; ++i)
    {
	font->glyph_id[i] = -1;
	font->glyph_width[i] = 1;
    }

    return font;
}

    void
bdf2_close(bdf2_t* font)
{
    if (!font)
	return;
    while (font->list)
    {
	bdf_list_t *next = font->list->next;

	bdf_close(font->list->bdf);
	free(font->list);
	font->list = next;
    }
    if (font->sizelist)
    {
	free(font->sizelist);
	font->sizelist = NULL;
    }
    free(font);
    return;
}

    int
check_bdfsize(char* filename)
{
    FILE *fp;

    fp = fopen(filename, "rb");
    if (!fp)
	return 0;
    else
    {
	int	pixel_size = 0;
	int	size = 0;
	char	buf[8192], *cmd;

	while (fgets(buf, sizeof(buf), fp))
	{
	    chomp(buf);
	    if (cmd = iscmd(buf, "SIZE"))
		size = atoi_next(&cmd);
#ifdef USE_PIXELSIZE
	    else if (cmd = iscmd(buf, "PIXEL_SIZE"))
	    {
		pixel_size = atoi_next(&cmd);
		break;
	    }
#endif
	}
	fclose(fp);
#ifdef USE_PIXELSIZE
	return pixel_size > 0 ? pixel_size : size;
#else
	return size;
#endif
    }
}

    int QSORT_CALLBACK
numsort(const void* a, const void* b)
{
    int *pa = (int*)a;
    int *pb = (int*)b;

    return *pa - *pb;
}

    int
bdf2_load(bdf2_t* font, char* filename)
{
    int size;
    bdf_t *bdf;
    int newsize = 0;
    int retval;
    int i;

    size = check_bdfsize(filename);
    TRACE("bdf2_load: size=%d\n", size);
    if (size <= 0)
    {
	fprintf(stderr, "check_bdfsize(%s) returns %d\n", filename, size);
	return 0;
    }
    bdf = bdf2_get_bdf1(font, size);
    /*
     * If not exists BDF in that size, create new and set no zero value for
     * newsize
     */
    if (!bdf)
    {
	bdf_list_t *list;

	list = (bdf_list_t*)malloc(sizeof(*list));
	list->bdf = bdf = bdf_open();
	bdf->verbose = font->verbose;
	list->next = font->list;
	font->list = list;
	++font->count;
	newsize = size;
	TRACE("bdf2_load: create a font size %d\n", newsize);
    }
    retval = bdf_load(bdf, filename);
    if (retval != 1)
    {
	/* Unexpected error!  This'll not be caused */
	fprintf(stderr, "bdf2_load: bdf_load(%p, %s) return %d\n",
		bdf, filename, retval);
	return retval;
    }

    /* Check glyph existance */
    font->numGlyph	= 0;
    font->indexFirst	= -1;
    font->indexLast	= -1;
    for (i = 0; i < BDF_MAX_GLYPH; ++i)
    {
	bdf_glyph_t *g = bdf_get_glyph(bdf, i);
	if (g)
	{
	    font->glyph_flag[i] |= BDF2GF_AVAILABLE;
#ifdef USE_FLEXIBLE_GLYPHWIDTH
	    if (font->glyph_width[i] != 2 && g->bbx.width > (size / 2))
	    {
		/* Multi cell glyph has always 2 cell */
		font->glyph_width[i] = 2;
		font->has_multicell = 1;
		//TRACE("i=%d s=%d w=%d\n", i, size, g->bbx.width);
	    }
#else
	    font->glyph_width[i] = 1;
#endif
	}
	if (font->glyph_flag[i] & BDF2GF_AVAILABLE)
	{
	    font->glyph_id[i] = font->numGlyph;
	    ++font->numGlyph;
	    if (font->indexFirst < 0)
		font->indexFirst = i;
	    font->indexLast = i;
	}
	else
	    font->glyph_id[i] = -1;
    }
    TRACE("bdf2_load: numGlyph=%d\n", font->numGlyph);
    /* Add size in the sizelist */
    if (newsize > 0)
    {
	font->sizelist = (int*)realloc(font->sizelist,
		sizeof(int) * font->count);
	font->sizelist[font->count - 1] = newsize;
	qsort((void*)font->sizelist, font->count, sizeof(int), numsort);
    }

#ifndef USE_FLEXIBLE_GLYPHWIDTH
    /* Chech cell width */
    for (i = 0; i < font->count; ++i)
    {
	int j;

	bdf = bdf2_get_bdf1(font, font->sizelist[i]);
	for (j = 0; j < BDF_MAX_GLYPH; ++j)
	    if (font->glyph_width[j] < bdf->cell_width[j])
		font->glyph_width[j] = bdf->cell_width[j];
    }
#endif

    return 1;
}

    bdf_glyph_t*
bdf2_get_glyph(bdf2_t* font, int size, int id)
{
    bdf_t *bdf;

    bdf = bdf2_get_bdf1(font, size);
    return bdf ? bdf_get_glyph(bdf, id) : NULL;
}

    int
bdf2_get_pixel(bdf2_t* font, int size, int id, int x, int y)
{
    bdf_t *bdf;

    bdf = bdf2_get_bdf1(font, size);
    return bdf ? bdf_get_pixel(bdf, id, x, y) : -1;
}

/*
 * Search bdf_t by size.
 */
    bdf_t*
bdf2_get_bdf1(bdf2_t* font, int size)
{
    bdf_list_t **list = &font->list;
    bdf_list_t *tmp;
    while (1)
    {
	tmp = *list;
	if (!tmp)
	    return NULL; /* not found */
#ifdef USE_PIXELSIZE
	else if (tmp->bdf->pixel_size == size)
	    break; /* found */
#else
	else if (tmp->bdf->size == size)
	    break; /* found */
#endif
	list = &tmp->next;
    }
    /* Put found bdf_t top in the list */
    if (tmp != font->list)
    {
	*list = tmp->next;
	tmp->next = font->list;
	font->list = tmp;
    }
    return font->list->bdf;
}

    int
bdf2_get_count(bdf2_t* font)
{
    return font->count;
}

    int*
bdf2_get_sizelist(bdf2_t* font)
{
    return font->sizelist;
}

    int
bdf2_get_glyph_id(bdf2_t* font, int id)
{
    if (id < 0 || id >= BDF_MAX_GLYPH)
	return -1;
    else
	return font->glyph_id[id];
}

    unsigned char
bdf2_get_glyph_flag(bdf2_t* font, int id)
{
    if (id < 0 || id >= BDF_MAX_GLYPH)
	return 0;
    else
	return font->glyph_flag[id];
}

    unsigned char
bdf2_get_glyph_width(bdf2_t* font, int id)
{
    if (id < 0 || id >= BDF_MAX_GLYPH)
	return 0;
    else
	return font->glyph_width[id];
}

    int
bdf2_is_glyph_available(bdf2_t* font, int id)
{
    return bdf2_get_glyph_flag(font, id) & BDF2GF_AVAILABLE ? 1 : 0;
}

/*****************************************************************************
 * TEST
 *****************************************************************************/

#ifdef BDF_TEST
    void
out_bitmap(bdf_t* font)
{
    int i;

    printf("size=%d\n", font->size);
    printf("bbx=%d %d %d %d\n", font->bbx.width, font->bbx.height,
	    font->bbx.offset.x, font->bbx.offset.y);
    printf("numGlyph=%d\n", font->numGlyph);
    for (i = 0; i < BDF_MAX_GLYPH; ++i)
    {
	int y;
	bdf_glyph_t *glyph;

	glyph = bdf_get_glyph(font, i);
	if (!glyph)
	    continue;
	printf("\n[%04x] %d %d %d %d\n", i,
		glyph->bbx.width, glyph->bbx.height,
		glyph->bbx.offset.x, glyph->bbx.offset.y);
	for (y = 0; bdf_get_pixel(font, i, 0, y) >= 0; ++y)
	{
	    int x;

	    printf("  ");
	    for (x = 0; ; ++x)
	    {
		int pixel = bdf_get_pixel(font, i, x, y);
		if (pixel < 0)
		    break;
		printf("%c", pixel ? '1' : '0');
	    }
	    printf("\n");
	}
    }
}

    void
out_bdf(bdf_t* font)
{
    int i;

    printf("SIZE %d %d %d\n", font->size, 72, 72);
    printf("FONTBOUNDINGBOX %d %d %d %d\n",
	    font->bbx.width, font->bbx.height,
	    font->bbx.offset.x, font->bbx.offset.y);
    printf("CHARS %d\n", font->numGlyph);
    for (i = 0; i < BDF_MAX_GLYPH; ++i)
    {
	int x, y, X, Y;
	bdf_glyph_t *glyph;

	glyph = bdf_get_glyph(font, i);
	if (!glyph)
	    continue;
	printf("STARTCHAR .%04x.%04x\n", i, glyph->id);
	printf("ENCODING %d\n", i);
	printf("SWIDTH %d %d\n", glyph->swidth.x, glyph->swidth.y);
	printf("DWIDTH %d %d\n", glyph->dwidth.x, glyph->dwidth.y);
	printf("BBX %d %d %d %d\n",
		glyph->bbx.width, glyph->bbx.height,
		glyph->bbx.offset.x, glyph->bbx.offset.y);
	printf("BITMAP\n");
	X = RAS_BYTE(glyph->bbx.width);
	Y = glyph->bbx.height;
	for (y = 0; y < Y; ++y)
	{
	    for (x = 0; x < X; ++x)
		printf("%02x", glyph->bitmap[x + y * X]);
	    printf("\n");
	}
	printf("ENDCHAR\n");
    }
}

    int
main(int argc, char** argv)
{
    int i;
    bdf_t* font;

    if (argc < 2)
    {
	printf("Usage: %s {bdffile} [{bdffile} ...]\n", argv[0]);
	return -1;
    }

    font = bdf_open();
    for (i = 1; argv[i]; ++i)
	bdf_load(font, argv[i]);

    //out_bitmap(font);
    out_bdf(font);

    bdf_close(font);
    return 0;
}
#endif
