/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * bdf2ttf.cpp - BDF to TTF converter.
 *
 * Written By:	MURAOKA Taro <koron@tka.att.ne.jp>
 * Last Change: 12-Aug-2005.
 */

//#define USE_CMAP_3RD_TABLE 1

#include "version.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "table.h"
#include "bdf.h"
#include "bdf2ttf.h"
#include "debug.h"

// Count items in the array.
#ifndef elementof
# define elementof(a) (sizeof(a) / sizeof(a[0]))
#endif
#define MIN(a,b)    ((a) < (b) ? (a) : (b))

#ifdef WIN32
# define QSORT_CALLBACK int __cdecl
#else
# define QSORT_CALLBACK int
#endif

// Required Tables
#define CMAP	0
#define HEAD	1
#define HHEA	2
#define HMTX	3
#define MAXP	4
#define NAME	5
#define OS2	6
#define POST	7
// Tables Related TrueType Outliens
#define CVT	8
#define FPGM	9
#define GLYF	10
#define LOCA	11
#define PREP	12
// Tables Related to Bitmap Glyphs
#define EBDT	13
#define EBLC	14
#define EBSC	15
#define GASP	16
// for the Apple QuickDraw GX
#define BLOC	17
#define BDAT	18
// Total table number
#define NUM_TABLE 19

char *ttfTag[] = {
    // Required tables.
    "cmap", "head", "hhea", "hmtx", "maxp", "name", "OS/2", "post",
    // for Vector glyph
    "cvt ", "fpgm", "glyf", "loca", "prep",
    // for Bitmap glyph
    "EBDT", "EBLC", "EBSC",
    // for Anti-aliasing
    "gasp",
    // for the Apple QuickDraw GX
    "bloc", "bdat",
};

table ttfTbl[NUM_TABLE];

/* prep program */
unsigned char prep_prog[8] = {
   0xb9,
   1, 0xff, 0, 0, // push word 511, 0
   0x8d, // scan type
   0x85, // scan ctrl
   0x18, // round to grid
};

static const size_t	SIZE_BITMAPSIZETABLE = (sizeof(long) * 4
	+ sizeof(char[12]) * 2 + sizeof(short) * 2 + sizeof(char) * 4);
static const size_t	SIZE_INDEXSUBTABLEARRAY		= 8;
static const int	EMMAX				= 1024;
static const int	MAX_SCALE			= 100;

static char* STYLE_REGULAR	= "Regular";
static char* STYLE_BOLD		= "Bold";
static char* STYLE_ITALIC	= "Italic";
static char* STYLE_BOLDITALIC	= "Bold Italic";

char *g_copyright	= COPYRIGHT;
char *g_copyright_cp	= NULL;
char *g_fontname	= NULL;
char *g_fontname_cp	= NULL;
char *g_style		= STYLE_REGULAR;
char *g_version		= VERSION;
char *g_version_cp	= NULL;
char *g_trademark	= TRADEMARK;
char *g_trademark_cp	= NULL;

    int
emCalc(int pix, int base)
{
    return (EMMAX * pix + base / 2) / base;
}

    static QSORT_CALLBACK
order_sort(const void *a, const void *b)
{
    return strcmp(ttfTag[*(int*)a], ttfTag[*(int*)b]);
}

    static void
addbytes(bigfirst* p, unsigned char* buf, int len)
{
    for (int i = 0; i < len; ++i)
	p->addByte(buf[i]);
}

    static void
name_addstr(table* t, bigfirst* p, char* buf, short platformID, short encodingID, short languageID, short nameID)
{
    int len = strlen(buf);
    t->addShort(platformID);
    t->addShort(encodingID);
    t->addShort(languageID);
    t->addShort(nameID);
    t->addShort((short)len);
    t->addShort(p->getLen());
    addbytes(p, (unsigned char*)buf, len);
}

    static wchar_t
utf8ptr_to_ucs4(unsigned char* ptr, int* len)
{
    if ((ptr[0] & 0x80) == 0x00)
    {
	if (len)
	    *len = 1;
	return (wchar_t)(ptr[0] & 0x7F);
    }
    else if ((ptr[0] & 0xE0) == 0xC0)
    {
	if (len)
	    *len = 2;
	return (wchar_t)(ptr[0] & 0x1F) << 6
	    | (wchar_t)(ptr[1] & 0x3F);
    }
    else if ((ptr[0] & 0xF0) == 0xE0)
    {
	if (len)
	    *len = 3;
	return (wchar_t)(ptr[0] & 0x0F) << 12
	    | (wchar_t)(ptr[1] & 0x3F) << 6
	    | (wchar_t)(ptr[2] & 0x3F);
    }
    else if ((ptr[0] & 0xF8) == 0xF0)
    {
	if (len)
	    *len = 4;
	return (wchar_t)(ptr[0] & 0x07) << 18
	    | (wchar_t)(ptr[1] & 0x3F) << 12
	    | (wchar_t)(ptr[2] & 0x3F) << 6
	    | (wchar_t)(ptr[3] & 0x3F);
    }
    else if ((ptr[0] & 0xFC) == 0xF8)
    {
	if (len)
	    *len = 5;
	return (wchar_t)(ptr[0] & 0x03) << 24
	    | (wchar_t)(ptr[1] & 0x3F) << 18
	    | (wchar_t)(ptr[2] & 0x3F) << 12
	    | (wchar_t)(ptr[3] & 0x3F) << 6
	    | (wchar_t)(ptr[4] & 0x3F);
    }
    else if ((ptr[0] & 0xFE) == 0xFC)
    {
	if (len)
	    *len = 6;
	return (wchar_t)(ptr[0] & 0x01) << 30
	    | (wchar_t)(ptr[1] & 0x3F) << 24
	    | (wchar_t)(ptr[2] & 0x3F) << 18
	    | (wchar_t)(ptr[3] & 0x3F) << 12
	    | (wchar_t)(ptr[4] & 0x3F) << 6
	    | (wchar_t)(ptr[5] & 0x3F);
    }

    if (len)
	*len = 1;
    return (wchar_t)' ';
}

    wchar_t*
into_unicode(int *outlen, char* buf)
{
    int unilen = 0;
    char *p;
    wchar_t *pout;

    for (p = buf; *p; ++unilen)
    {
	int len;
	utf8ptr_to_ucs4((unsigned char*)p, &len);
	p += len;
    }
    wchar_t *outbuf = (wchar_t*)malloc(sizeof(wchar_t) * (unilen + 1));
    for (p = buf, pout = outbuf; *p; ++pout)
    {
	int len;
	*pout = utf8ptr_to_ucs4((unsigned char*)p, &len);
	p += len;
    }
    *pout = (wchar_t)'\0';
    if (outlen)
	*outlen = unilen;
    return outbuf;
}

    static void
name_addunistr(table* t, bigfirst* p, char* buf, short platformID, short encodingID, short languageID, short nameID)
{
    int len = strlen(buf);

    t->addShort(platformID);
    t->addShort(encodingID);
    t->addShort(languageID);
    t->addShort(nameID);

    int unilen;
    wchar_t *unibuf;
    unibuf = into_unicode(&unilen, buf);
#if 0
    {
	int i;
	printf("FROM:");
	for (i = 0; i < len; ++i)
	    printf(" %02X", (unsigned char)buf[i]);
	printf("\n");
	printf("TO__:");
	for (i = 0; i < unilen; ++i)
	    printf(" %04X", unibuf[i]);
	printf("\n");
    }
#endif
    unilen = unilen * sizeof(wchar_t);

    t->addShort((short)unilen);
    t->addShort(p->getLen());
    p->addString(unibuf);
    free(unibuf);
}

    static int
search_topbit(unsigned long n)
{
    int topbit = 0;
    for (; n > 1UL; ++topbit)
	n >>= 1;
    return topbit;
}

    static void
glyph_simple(bdf2_t* font, bigfirst &glyf)
{
    glyf.addShort(1);			// numberOfContours (contour:輪郭)
    glyf.addShort(0);			// xMin
    glyf.addShort(-font->emDescent);	// yMin
    glyf.addShort(font->emX / 2); 	// xMax
		// hmtx define the kerning, not here
    glyf.addShort(font->emAscent);	// yMax

    glyf.addShort(0);			// endPtsOfContours[n]
    glyf.addShort(0);			// instructionLength
    glyf.addByte(0x37); 		// instructions[n]
		//On curve, both x,y are positive short
    glyf.addByte(1);			// flag[n]
    glyf.addByte(1);			// xCoordinates[]
    glyf.addShort((unsigned short)-1); 	// yCoordinates[]
		//for parent add
}

    static unsigned char*
get_glyph_bitmap(bdf_glyph_t* glyph)
{
    unsigned char* buf;
#if 0
    int w = MIN(glyph->bbx.width, g_bdf->bbx.width);
    int h = MIN(glyph->bbx.height, g_bdf->bbx.height);
#else
    int w = glyph->bbx.width;
    int h = glyph->bbx.height;
#endif
    int bufsize = (w * h + 7) >> 3;
    buf = (unsigned char*)calloc(1, bufsize);

    unsigned char *p = buf - 1;
    unsigned char mask = 0x00;
    for (int y = 0; y < h; ++y)
    {
	for (int x = 0; x < w; ++x)
	{
	    mask >>= 1;
	    if (!mask)
	    {
		mask = 0x80;
		++p;
	    }
	    if (bdf_get_pixel_glyph(glyph, x, y) > 0)
		*p |= mask;
	}
    }
#if 0
    printf("id=%d bufsize=%d\n", glyph->id, bufsize);
    for (int i = 0; i < bufsize; ++i)
	printf(" %02X", buf[i]);
    printf("\n");
#endif
    return buf;
}

    static void
add_bigGlyphMetrics(bigfirst* t, int height, int width,
	int horiBearingX, int horiBearingY, int horiAdvance,
	int vertBearingX, int vertBearingY, int vertAdvance)
{
    t->addByte(height);
    t->addByte(width);
    t->addByte(horiBearingX);
    t->addByte(horiBearingY);
    t->addByte(horiAdvance);
    t->addByte(vertBearingX);
    t->addByte(vertBearingY);
    t->addByte(vertAdvance);
}

#if 0
/*
 * Reference implementation
 */
    static int
generate_eb_rawbitmap(bdf_t* bdf, table *ebdt, bigfirst* st)
{
    st->addShort(1);			// firstGlyphIndex
    st->addShort(bdf->numGlyph);	// lastGlyphIndex
    st->addLong(sizeof(long) * 1 + sizeof(short) * 2);
					// additionalOffsetToIndexSubtable
    // indexSubTable #0
    st->addShort(1);			// indexFormat
    st->addShort(6);			// imageFormat
    st->addLong(0);			// imageDataOffset

    //st->addLong(ebdt->getLen() - sizeof(long));
    st->addLong(ebdt->getLen());
    for (int i = 0; i < BDF_MAX_GLYPH; ++i)
    {
	bdf_glyph_t *glyph = bdf_get_glyph(bdf, i);
	if (!glyph)
	    continue;
	int gwidth  = MIN(glyph->bbx.width, bdf->bbx.width);
	int gheight = MIN(glyph->bbx.height, bdf->bbx.height);
	add_bigGlyphMetrics(ebdt, gheight, gwidth,
		0, gheight + glyph->bbx.offset.y, gwidth,
		-gwidth / 2, 0, gheight);

	int len = ((gwidth + 7) >> 3) * gheight;
	addbytes(ebdt, glyph->bitmap, len);
	st->addLong(ebdt->getLen());
    }

    return 1;
}
#endif

    static int
count_subtable(bdf2_t* font, bdf_t* bdf)
{
    int n_subtbl = 0;

    for (int i = 0; i < BDF_MAX_GLYPH; ++i)
    {
	int idStart, idEnd;
	int encStart, encEnd;
	bdf_glyph_t *glyph = bdf_get_glyph(bdf, i);
	if (!glyph)
	    continue;
	bdf_boundingbox_t bbx;
	memcpy(&bbx, &glyph->bbx, sizeof(bbx));
	idStart = idEnd = font ? bdf2_get_glyph_id(font, i) : glyph->id;
	encStart = encEnd = i;
	int j;
	for (j = i + 1; j < BDF_MAX_GLYPH; ++j)
	{
	    bdf_glyph_t *glyph = bdf_get_glyph(bdf, j);
	    if (!glyph)
		continue;
	    if (memcmp(&bbx, &glyph->bbx, sizeof(bbx)) != 0)
		break;
	    idEnd = font ? bdf2_get_glyph_id(font, j) : glyph->id;
	    encEnd = j;
	}
	i = j - 1;
	++n_subtbl;
#if 0
	printf("idStart=%d(%04X), idEnd=%d(%04X)\n",
		idStart, encStart, idEnd, encEnd);
#endif
    }
    return n_subtbl;
}

    static int
generate_eb_optbitmap(bdf2_t* font, bdf_t* bdf, table *ebdt, bigfirst* st)
{
    int n_subtbl = count_subtable(font, bdf);

    // for indexFormat 2, imageFormat 5
    {
	bigfirst subHeader, *sh = &subHeader;
	for (int i = 0; i < BDF_MAX_GLYPH; ++i)
	{
	    int idStart, idEnd;
	    int encStart, encEnd;
	    bdf_glyph_t *glyph = bdf_get_glyph(bdf, i);
	    if (!glyph)
		continue;
	    bdf_boundingbox_t bbx;
	    memcpy(&bbx, &glyph->bbx, sizeof(bbx));
	    idStart = idEnd = font ? bdf2_get_glyph_id(font, i) : glyph->id;
	    encStart = encEnd = i;
	    int j;
	    for (j = i + 1; j < BDF_MAX_GLYPH; ++j)
	    {
		bdf_glyph_t *glyph = bdf_get_glyph(bdf, j);
		if (!glyph)
		    continue;
		if (memcmp(&bbx, &glyph->bbx, sizeof(bbx)) != 0)
		    break;
		idEnd = font ? bdf2_get_glyph_id(font, j) : glyph->id;
		encEnd = j;
	    }
	    i = j - 1;

	    int w = MIN(bbx.width, bdf->bbx.width);
	    int h = MIN(bbx.height, bdf->bbx.height);
	    int imgsize = (w * h + 7) >> 3;

	    // indexSubTableArray
	    st->addShort(idStart + 1);	// firstGlyphIndex
	    st->addShort(idEnd + 1);	// lastGlyphIndex
	    st->addLong(sh->getLen() + SIZE_INDEXSUBTABLEARRAY * n_subtbl);
					// additionalOffsetToIndexSubtable

	    // indexSubTable format #2
	    // indexSubHeader
	    sh->addShort(2);		// indexFormat
	    sh->addShort(5);		// imageFormat
	    sh->addLong(ebdt->getLen());// imageDataOffset
	    // table body
	    sh->addLong(imgsize);	// imageSize
	    // bigMetrics
#if 0
	    /*
	     * ベースラインを加味する方法。ビットマップフォントを扱う上では
	     * 不利なので、現在は使っていない。
	     */
	    glyph = bdf_get_glyph(bdf, encStart);
	    add_bigGlyphMetrics(sh, h, w, 0, h + glyph->bbx.offset.y, w,
		    - w / 2, 0, h);
#else
	    add_bigGlyphMetrics(sh, h, w, 0, h, w, - w / 2, 0, h);
#endif

	    for (j = encStart; j <= encEnd; ++j)
	    {
		glyph = bdf_get_glyph(bdf, j);
		if (!glyph)
		{
		    if (font && bdf2_is_glyph_available(font, j))
			for (int k = 0; k < imgsize; ++k)
			    ebdt->addByte(0);
		}
		else
		{
		    unsigned char *buf = get_glyph_bitmap(glyph);
		    addbytes(ebdt, buf, imgsize);
		    free(buf);
		}
	    }
	}
	st->addArray(sh);
    }

    return n_subtbl;
}

    static void
add_sbitLineMetric(bigfirst* t,
	int ascender, int descender, int widthMax, 
	int caretSlopeNumerator, int caretSlopeDenominator, int caretOffset,
	int minOriginSB, int minAdvanceSB, int maxBeforeBL, int minAfterBL,
	int pad1, int pad2)
{
    t->addByte(ascender);
    t->addByte(descender);
    t->addByte(widthMax);
    t->addByte(caretSlopeNumerator);
    t->addByte(caretSlopeDenominator);
    t->addByte(caretOffset);
    t->addByte(minOriginSB);
    t->addByte(minAdvanceSB);
    t->addByte(maxBeforeBL);
    t->addByte(minAfterBL);
    t->addByte(pad1);
    t->addByte(pad2);
}

    static void
generate_eb_location(bdf2_t* font, bdf_t* bdf,
	table* eblc, bigfirst* subtable, bigfirst* starray,
	int n_plane, int n_subtbl, int width, int height, int origsize)
{
    // bitmapSizeTable
    eblc->addLong(sizeof(long) * 2 + SIZE_BITMAPSIZETABLE * n_plane
	    + starray->getLen());
					// indexSubTableArrayOffset
    eblc->addLong(subtable->getLen());	// indexTableSize
    eblc->addLong(n_subtbl);		// numberOfIndexSubTables
    eblc->addLong(         0);		// colorRef
    // sbitLineMetrics: hori
#if 0
    int a = bdf->ascent;
    int d = -bdf->descent;
#else
    int a = width;
    int d = height;
#endif
    int s = origsize;
    add_sbitLineMetric(eblc, a, d, s, 1, 0, 0, 0, s, a, d, 0, 0);
    //TRACE("w=%d h=%d ascent=%d descent=%d origsize=%d\n", width, height, bdf->ascent, bdf->descent, origsize);
    // sbitLineMetrics: vert
    add_sbitLineMetric(eblc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    eblc->addShort(     1);		// startGlyphIndex
    int n_glyph = font ? font->numGlyph : bdf->numGlyph;
    eblc->addShort(n_glyph);		// endGlyphIndex
    eblc->addByte(origsize);		// ppemX
    eblc->addByte(origsize);		// ppemY
    eblc->addByte(   1);		// bitDepth
    eblc->addByte(0x01);		// flags:
					//	0x01=Horizontal
					//	0x02=Vertical
}

    static void
add_scaletable(table* ebsc, int targetsize, int width, int height, int orig)
{
    int size = targetsize;
    if (size == orig)
	return;
    int hsize = (targetsize * height + width / 2) / width;

    // sbitLineMetrics: hori
    add_sbitLineMetric(ebsc,
	    0, 0, targetsize,
	    0, 0, 0,
	    0, 0, hsize, 0,
	    0, 0);
    // sbitLineMetrics: vert
    add_sbitLineMetric(ebsc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    ebsc->addByte(targetsize);
    ebsc->addByte(targetsize);
    ebsc->addByte(orig);
    ebsc->addByte(orig);
}

    static void
generate_EBDT_EBLC_EBSC(bdf2_t* bdf2)
{
    int n_plane	    = bdf2_get_count(bdf2);
    int *sizelist   = bdf2_get_sizelist(bdf2);

    table *ebdt = &ttfTbl[EBDT];
    table *eblc = &ttfTbl[EBLC];
    table *ebsc = &ttfTbl[EBSC];
    ebdt->addLong(0x00020000);
    eblc->addLong(0x00020000);
    eblc->addLong(n_plane);
    ebsc->addLong(0x00020000);
    ebsc->addLong(MAX_SCALE - n_plane);

    int scsize	= 1;
    bigfirst starray;
    for (int i = 0; i < n_plane; ++i)
    {
	int cursize = sizelist[i];
	bdf_t *bdf = bdf2_get_bdf1(bdf2, cursize);

	if (!bdf)
	{
	    fprintf(stderr, "bdf2_t: size %d has not font glyph\n", cursize);
	    continue;
	}

	int w		= bdf->bbx.width;
	int h		= bdf->bbx.height;
	//int orig	= MIN(w, h);
	int orig	= bdf->pixel_size; //(w > h / 2) ? w : w * 2;
	int n_subtable	= 0;
	bigfirst subtable;

	n_subtable = generate_eb_optbitmap(bdf2, bdf, ebdt, &subtable);
	generate_eb_location(bdf2, bdf,
		eblc, &subtable, &starray, 
		n_plane, n_subtable, w, h, orig);
	starray.addArray(subtable);

	int nextsize = (i < n_plane - 1) ? sizelist[i + 1] : MAX_SCALE;
	for (; scsize <= nextsize; ++scsize)
	    add_scaletable(ebsc, scsize, w, h, orig);
    }

    ebdt->calcSum();
    ebdt->commitLen();
    eblc->addArray(starray);
    eblc->calcSum();
    eblc->commitLen();
    ebsc->calcSum();
    ebsc->commitLen();
}

    static void
generate_LOCA_GLYF(bdf2_t* font)
{
    table *loca = &ttfTbl[LOCA], *glyf = &ttfTbl[GLYF];
    // 0
    loca->addLong(0);
    glyph_simple(font, *glyf);

    int remain = font->numGlyph;
    for (int i = 0; i < BDF_MAX_GLYPH; ++i)
    {
	if (bdf2_is_glyph_available(font, i))
	{
	    loca->addLong((long)glyf->getLen());
	    --remain;
	}
    }

    glyf->calcSum();
    glyf->commitLen();

    loca->addLong((long)glyf->getLen());
    loca->calcSum();
    // Reserve
    loca->commitLen();
}

    static void
generate_CMAP(bdf2_t* font)
{
    bigfirst fmat0, fmat4;

    // Make format 0
    {
	int i;

	fmat0.addShort(0);			/* format */
	fmat0.addShort(sizeof(short) * 3 + 256);
	/* len */
	fmat0.addShort(0);			/* Revision */
	for (i = 0; i < 0x100; ++i)
	{
	    int gid = bdf2_is_glyph_available(font, i) ?
		bdf2_get_glyph_id(font, i) + 1 : 0;
	    fmat0.addByte(gid);
	    //printf("GLYPH_ID: %02x -> %d\n", i, bdf2_get_glyph_id(font, i));
	}
    }

    // Make format 4
    {
	bigfirst endCount, startCount, idDeleta, idRangeOffset;
	int segCount = 0;

	for (int i = 0; i < BDF_MAX_GLYPH; ++i)
	{
	    if (!bdf2_is_glyph_available(font, i))
	    {
		if (i == BDF_MAX_GLYPH - 1)
		{
		    ++segCount;
		    endCount.addShort(BDF_MAX_GLYPH - 1);
		    startCount.addShort(BDF_MAX_GLYPH - 1);
		    idDeleta.addShort(1);
		    idRangeOffset.addShort(0);
		}
		continue;
	    }
	    int j = i + 1;
	    while (bdf2_is_glyph_available(font, j))
		++j;
	    j -= 1;
	    endCount.addShort(j);
	    startCount.addShort(i);
	    idDeleta.addShort(bdf2_get_glyph_id(font, i) - i + 1);
	    idRangeOffset.addShort(0);
	    ++segCount;
	    i = j + 1;
	}

	unsigned short segCountX2	= segCount * 2;
	unsigned short entrySelector	= search_topbit(segCount);
	unsigned short searchRange	= 1 << (entrySelector + 1);

	fmat4.addShort(4);		// format
	fmat4.addShort(sizeof(short) * 8 + segCount * sizeof(short) * 4);
					// length
	fmat4.addShort(0);		// language
	fmat4.addShort(segCountX2);	// segCountX2
	fmat4.addShort(searchRange);	// searchRange
	fmat4.addShort(entrySelector);	// entrySelector
	fmat4.addShort(segCountX2 - searchRange);
					// rangeShift
	fmat4.addArray(endCount);
	fmat4.addShort(0);		// reservedPad
	fmat4.addArray(startCount);
	fmat4.addArray(idDeleta);
	fmat4.addArray(idRangeOffset);
    }

    /* follow make cmap */
#ifdef USE_CMAP_3RD_TABLE
    /* for 3rd table */
    int numtbl = 3;
#else
    int numtbl = 2;
#endif
    int offset = sizeof(short) * 2
	+ (sizeof(short) * 2 + sizeof(long)) * numtbl;

    table *cmap = &ttfTbl[CMAP];
    cmap->addShort(0);		/* Version */
    cmap->addShort(numtbl);	/* #tbl */
    /* Table 1 (format 0) */
    cmap->addShort(1);		/* Platform ID = 1 -> Macintosh */
    cmap->addShort(0);		/* Encoding ID = 0 -> Roman */
    cmap->addLong(offset);
    /* Table 2 (format 4) */
    cmap->addShort(3);		/* Platform ID = 3 -> Microsoft */
    cmap->addShort(1);		/* Encoding ID = 1 -> Unicode */
    cmap->addLong(offset + fmat0.getLen());
#if USE_CMAP_3RD_TABLE
    /* Table 3 (format X) */
    cmap->addShort(0);		/* Platform ID = X -> Unknown */
    cmap->addShort(0);		/* Encoding ID = X -> Unknown */
    cmap->addLong(offset + fmat0.getLen());
    //cmap->addLong(offset + fmat0.getLen());
#endif

    /* Add table main data stream */
    cmap->addArray(fmat0);
    cmap->addArray(fmat4);

    cmap->calcSum();
    cmap->commitLen();
}

    static void
generate_OS2_PREP(bdf2_t* font)
{
    short fsSelection = 0x0000;
    short usWeightClass = 400;
    char bWeight = 5;
    if (font->flagBold)
    {
	fsSelection |= 0x0020;
	usWeightClass = 700;
	bWeight = 8;
    }
    if (font->flagItalic)
	fsSelection |= 0x0001;
    if (font->flagRegular)
	fsSelection |= 0x0040;
    //TRACE("fsSelection=%04x\n", fsSelection);

    /* Generate OS/2 and PREP */
    table* os2 = &ttfTbl[OS2];
    os2->addShort(0x0001);		// version
    os2->addShort(font->emX / 2);	// xAvgCharWidth
    os2->addShort(usWeightClass);	// usWeightClass
    os2->addShort(     5);		// usWidthClass
    os2->addShort(0x0004);		// fsType
    os2->addShort(   512);		// ySubscriptXSize
    os2->addShort(   512);		// ySubscriptYSize
    os2->addShort(     0);		// ySubscriptXOffset
    os2->addShort(     0);		// ySubscriptYOffset
    os2->addShort(   512);		// ySuperscriptXSize
    os2->addShort(   512);		// ySuperscriptYSize
    os2->addShort(     0);		// ySuperscriptXOffset
    os2->addShort(   512);		// ySuperscriptYOffset
    os2->addShort(    51);		// yStrikeoutSize
    os2->addShort(   260);		// yStrikeoutPosition
    os2->addShort(     0);		// sFamilyClass
    os2->addByte(2);			// panose.bFamilyType
    os2->addByte(0);			// panose.bSerifStyle
    os2->addByte(bWeight);		// panose.bWeight
    os2->addByte(9);			// panose.bProportion
    os2->addByte(0);			// panose.bContrast
    os2->addByte(0);			// panose.bStrokeVariation
    os2->addByte(0);			// panose.bArmStyle
    os2->addByte(0);			// panose.bLetterform
    os2->addByte(0);			// panose.bMidline
    os2->addByte(0);			// panose.bXHeight
    // TODO グリフにどのユニコード文字が含まれるかを示す。(要計算)
#if 0
    os2->addLong(0xa00002bf);		// ulUnicodeRange1
    os2->addLong(0x68c7fcfb);		// ulUnicodeRange2
    os2->addLong(0x00000000);		// ulUnicodeRange3
    os2->addLong(0x00000000);		// ulUnicodeRange4
#else
    os2->addLong(0xffffffff);		// ulUnicodeRange1
    os2->addLong(0xffffffff);		// ulUnicodeRange2
    os2->addLong(0xffffffff);		// ulUnicodeRange3
    os2->addLong(0xffffffff);		// ulUnicodeRange4
#endif
    os2->array::addString(VENDORID);	// achVendID
    os2->addShort(fsSelection);		// fsSelection
    os2->addShort(font->indexFirst);	// usFirstCharIndex
    os2->addShort(font->indexLast);	// usLastCharIndex
    // フォントの縦横比をコントロールする
    os2->addShort(font->emAscent);	// sTypoAscender
    os2->addShort(-font->emDescent);	// sTypoDescender
    os2->addShort(0);			// sTypoLineGap
    os2->addShort(font->emAscent);	// usWinAscent
    os2->addShort(font->emDescent);	// usWinDesent
#if 1
    // TODO サポートしているキャラクタコード(要計算)
#if 0
    os2->addLong(0x4002000F);		// ulCodePageRange1
    os2->addLong(0xD2000000);		// ulCodePageRange2
#else
    os2->addLong(0xffffffff);		// ulCodePageRange1
    os2->addLong(0xffffffff);		// ulCodePageRange2
#endif
#endif
#if 0
    // For version 2.
    //	    ttfdumpが対応していないので検証不可能
    os2->addShort(0x0040);		// sxHeight
    os2->addShort(0x0040);		// sCapHeight
    os2->addShort(0x0040);		// usDefaultChar
    os2->addShort(0x0040);		// usBreakChar
    os2->addShort(0x0040);		// usMaxContext
#endif
    os2->calcSum();
    os2->commitLen();

    addbytes(&ttfTbl[PREP], &prep_prog[0], elementof(prep_prog));
    ttfTbl[PREP].calcSum();
    ttfTbl[PREP].commitLen();
}

    static void
generate_GASP(bdf2_t* font)
{
    //int size = MIN(font->width, font->height);
    table* gasp = &ttfTbl[GASP];
    gasp->addShort(0); // version
#if 0
    if (size < 16)
	gasp->addShort(3);
    else
	gasp->addShort(2);

    gasp->addShort(size - 1);
    gasp->addShort(0x0002);
    if (size < 16)
    {
	gasp->addShort(16);
	gasp->addShort(0x0001);
    }
    gasp->addShort(0xffff);
    gasp->addShort(0x0003);
#else
    gasp->addShort(1);		// numRange
    gasp->addShort(0xffff);	// rangeMaxPPEM
    gasp->addShort(0x0001);	// rangeGaspBehavior
#endif

    gasp->calcSum();
    gasp->commitLen();
}

    static void
generate_HEAD_HHEA_HMTX_MAXP_POST(bdf2_t* font)
{
    //gene head, hhea, hmtx, maxp, and post

    short macStyle = 0x0000;
    if (font->flagBold)
	macStyle |= 0x0001;
    if (font->flagItalic)
	macStyle |= 0x0002;
    //TRACE("macStyle=%04x\n", macStyle);

    table *tmp = &ttfTbl[HEAD];
    tmp->addLong( 0x10000); 		// version
    tmp->addLong( 0x10000); 		// fontRevision
    tmp->addLong( 0); 			// checkSumAdjustment
    tmp->addLong( 0x5f0f3cf5); 		// magicNumber
    tmp->addShort(0x000b); 		// flags
    tmp->addShort(EMMAX); 		// unitPerEm
    tmp->addLong( 0);			// createdTime
    tmp->addLong( 0);
    tmp->addLong( 0);			// modifiedTime
    tmp->addLong( 0);
    tmp->addShort(0);			// xMin
    tmp->addShort(- font->emDescent);	// yMin
    tmp->addShort(font->emX);		// xMax
    tmp->addShort(font->emAscent);	// yMax
    tmp->addShort(macStyle);		// macStyle
    tmp->addShort(7);	 		// lowestRecPPEM
    tmp->addShort(1); 			// fontDirectionHint
    tmp->addShort(1); 			// indexToLocFormat
    tmp->addShort(0);			// glyphDataFormat
    tmp->calcSum();
    tmp->commitLen();

    tmp = &ttfTbl[HHEA];
    tmp->addLong( 0x10000); 		// version
    tmp->addShort(font->emAscent);	// Ascender
    tmp->addShort(- font->emDescent);	// Descender
    tmp->addShort(0);			// yLineGap
    tmp->addShort(font->emX); 		// advanceWidthMax
    tmp->addShort(0); 			// minLeftSideBearing
    tmp->addShort(0); 			// minRightSideBearing
    tmp->addShort(font->emX); 		// xMaxExtent
    tmp->addShort(1); 			// caretSlopeRise
    tmp->addShort(0);			// caretSlopeRun
    tmp->addShort(0);			// caretOffset
    tmp->addShort(0);			// (reserved)
    tmp->addShort(0);			// (reserved)
    tmp->addShort(0);			// (reserved)
    tmp->addShort(0);			// (reserved)
    tmp->addShort(0); 			// metricDataFormat
    tmp->addShort(font->numGlyph + 1);	// numberOfHMetrics
    tmp->calcSum();
    tmp->commitLen();

    tmp = &ttfTbl[HMTX];
    // #0
    tmp->addShort(emCalc(1, 2));
    tmp->addShort(0);
    for (int i = 0; i < BDF_MAX_GLYPH; ++i)
    {
	if (!bdf2_is_glyph_available(font, i))
	    continue;
	tmp->addShort(emCalc(bdf2_get_glyph_width(font, i), 2));
	tmp->addShort(0);
    }
    tmp->calcSum();
    tmp->commitLen();

    tmp = &ttfTbl[MAXP];
    tmp->addLong( 0x10000); 		// Version
    tmp->addShort(font->numGlyph + 1);	// numGlyphs
    tmp->addShort(800);			// maxPoints
    tmp->addShort(40);			// maxContours
    tmp->addShort(800); 		// maxCompositePoints
    tmp->addShort(40);			// maxCompositeContours
    tmp->addShort(2);			// maxZones
    tmp->addShort(0); 			// maxTwilightPoints
    tmp->addShort(10); 			// maxStorage
    tmp->addShort(0); 			// maxFunctionDefs
    tmp->addShort(0); 			// maxInstructionDefs
    tmp->addShort(1024); 		// maxStackElements
    tmp->addShort(0); 			// maxSizeOfInstructions
    tmp->addShort(1); 			// maxComponentElements
    tmp->addShort(1); 			// maxComponentDepth
    tmp->calcSum();
    tmp->commitLen();

    tmp = &ttfTbl[POST];
    tmp->addLong(0x30000); 		// Version
    tmp->addLong(0);			// italicAngle
    tmp->addShort(- font->emDescent);	// underlinePosition
#if 0
    tmp->addShort(emCalc(1, g_bdf->bbx.width));
#else
    tmp->addShort(1);			// underlineThickness
#endif
					// underlineThickness
    tmp->addLong(1); 			// isFixedPitch
    tmp->addLong(0);			// minMemType42
    tmp->addLong(0);			// maxMemType42
    tmp->addLong(0);			// minMemType1
    tmp->addLong(0);			// maxMemType1
    tmp->calcSum();
    tmp->commitLen();
}

    static void
generate_NAME(bdf2_t* font)
{
    static const int NUM = 24;
    bigfirst p;
    table *t = &ttfTbl[NAME];
    g_style = STYLE_REGULAR;
    if (font->flagAutoName)
    {
	if (font->flagBold && font->flagItalic)
	    g_style = STYLE_BOLDITALIC;
	else if (font->flagBold)
	    g_style = STYLE_BOLD;
	else if (font->flagItalic)
	    g_style = STYLE_ITALIC;
    }

    char fullname[1024];
    char fullname_cp[1024];
    strcpy(fullname, g_fontname);
    strcpy(fullname_cp, g_fontname_cp);
    if (g_style != STYLE_REGULAR)
    {
	strcat(fullname, " ");
	strcat(fullname, g_style);
	strcat(fullname_cp, " ");
	strcat(fullname_cp, g_style);
    }

    t->addShort(0);
    t->addShort(NUM);		// #NameRec
    t->addShort(6 + NUM * 12);	// Offet of strage

    name_addstr(t, &p, g_copyright,	1, 0, 11, 0);
    name_addstr(t, &p, g_fontname,	1, 0, 11, 1);
    name_addstr(t, &p, g_style,		1, 0, 11, 2);
    name_addstr(t, &p, g_fontname,	1, 0, 11, 3);
    name_addstr(t, &p, fullname,	1, 0, 11, 4);
    name_addstr(t, &p, g_version,	1, 0, 11, 5);
    name_addstr(t, &p, g_fontname,	1, 0, 11, 6);
    name_addstr(t, &p, g_trademark,	1, 0, 11, 7);

    name_addunistr(t, &p, g_copyright,	3, 1, 0x409, 0);
    name_addunistr(t, &p, g_fontname,	3, 1, 0x409, 1);
    name_addunistr(t, &p, g_style,	3, 1, 0x409, 2);
    name_addunistr(t, &p, g_fontname,	3, 1, 0x409, 3);
    name_addunistr(t, &p, fullname,	3, 1, 0x409, 4);
    name_addunistr(t, &p, g_version,	3, 1, 0x409, 5);
    name_addunistr(t, &p, g_fontname,	3, 1, 0x409, 6);
    name_addunistr(t, &p, g_trademark,	3, 1, 0x409, 7);

    name_addunistr(t, &p, g_copyright_cp,3,1, 0x411, 0);
    name_addunistr(t, &p, g_fontname_cp,3, 1, 0x411, 1);
    name_addunistr(t, &p, g_style,	3, 1, 0x411, 2);
    name_addunistr(t, &p, g_fontname_cp,3, 1, 0x411, 3);
    name_addunistr(t, &p, fullname_cp,	3, 1, 0x411, 4);
    name_addunistr(t, &p, g_version_cp,	3, 1, 0x411, 5);
    name_addunistr(t, &p, g_fontname_cp,3, 1, 0x411, 6);
    name_addunistr(t, &p, g_trademark_cp,	3, 1, 0x411, 7);

    t->addArray(p);
    t->calcSum();
    t->commitLen();
}

    static void
write_all_tables(bdf2_t* font, FILE *fn)
{
    int i;
    /* tableOrder */
    const int order[] = {
	OS2, CMAP, HEAD, HHEA, HMTX, MAXP, NAME, POST,
	EBLC, EBDT, EBSC,
	GASP,
	//CVT, FPGM,
	PREP, LOCA, GLYF,
	BLOC, BDAT,
    };
    table hob;
    unsigned long sum = 0L;
    int numTbl = elementof(order);

    hob.addLong (0x10000); /* Version */
    hob.addShort(numTbl);
    hob.addShort(128);
    hob.addShort(3);
    hob.addShort(numTbl * 16 - 128);
    // 各テーブルのオフセット計算
    ttfTbl[order[0]].off = 12 + numTbl * 16;
    for (i = 1; i < numTbl; ++i)
    {
	int prev = order[i - 1];
	ttfTbl[order[i]].off = ttfTbl[prev].off + ttfTbl[prev].getTableLen();
    }

    int *order2 = new int[numTbl];
    memcpy(order2, order, sizeof(order));
    qsort(order2, numTbl, sizeof(int), order_sort);
    for (i = 0; i < numTbl; i++)
    {
	int ntbl = order2[i];
	hob.addByte(ttfTag[ntbl][0]);
	hob.addByte(ttfTag[ntbl][1]);
	hob.addByte(ttfTag[ntbl][2]);
	hob.addByte(ttfTag[ntbl][3]);
	table *mapTo = ttfTbl[ntbl].getMapTable();
	if (!mapTo)
	{
	    hob.addLong(ttfTbl[ntbl].chk);
	    hob.addLong(ttfTbl[ntbl].off);
	    hob.addLong(ttfTbl[ntbl].getTableLen());
	    sum += ttfTbl[ntbl].chk;
	}
	else
	{
	    hob.addLong(mapTo->chk);
	    hob.addLong(mapTo->off);
	    hob.addLong(mapTo->getTableLen());
	}
    }
    delete[] order2;

    sum += hob.calcSum();
    hob.write(fn);
    for (i = 0; i < numTbl; ++i)
	if (!ttfTbl[order[i]].getMapTable())
	    ttfTbl[order[i]].write(fn);
#if 0
    {
	// テーブルをログファイルに書き出す
	for (int i = 0; i < numTbl; ++i)
	{
	    char buf[_MAX_PATH];
	    strcpy(buf, "bin/");
	    strcat(buf, ttfTag[i]);
	    strcat(buf, ".bin");
	    for (char *p = buf + 4; *p; ++p)
		if (*p == '/' || *p == '\\')
		    *p = '_';
	    //printf("buf=%s\n", buf);
	    FILE *fp = fopen(buf, "wb");
	    ttfTbl[i].write(fp);
	    fclose(fp);
	}
    }
#endif
    sum = 0xb1b0afba - sum;
    fseek(fn, ttfTbl[HEAD].off + 8, SEEK_SET);
    fwrite(&sum,1,4,fn);
}

    static void
map_table_for_macintosh(bdf2_t* font)
{
    ttfTbl[BLOC].setMapTable(&ttfTbl[EBLC]);
    ttfTbl[BDAT].setMapTable(&ttfTbl[EBDT]);
}

    int
write_ttf(bdf2_t* font, char* ttfname)
{
    FILE *fp = fopen(ttfname, "wb+");
    if (!fp)
    {
	printf("write_ttf:ERROR: Can't open a file\n");
	return 0;
    }

    map_table_for_macintosh(font);
    generate_EBDT_EBLC_EBSC(font);
    generate_LOCA_GLYF(font);
    generate_CMAP(font);
    generate_OS2_PREP(font);
    generate_GASP(font);
    generate_HEAD_HHEA_HMTX_MAXP_POST(font);
    generate_NAME(font);

    write_all_tables(font, fp);
    fclose(fp);

    return 1;
}
