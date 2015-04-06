/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * bdf.h -
 *
 * Written By:	MURAOKA Taro <koron@tka.att.ne.jp>
 * Last Change:	12-Aug-2005.
 */
#ifndef BDF_H
#define BDF_H

#define BDF_MAX_GLYPH 0x10000

typedef unsigned char bdf_byte_t;

typedef struct _bdf_coord_t
{
    int x;
    int y;
} bdf_coord_t;

typedef struct _bdf_boundingbox_t
{
    int width;
    int height;
    bdf_coord_t offset;
} bdf_boundingbox_t;

typedef struct _bdf_glyph_t
{
    bdf_coord_t swidth;
    bdf_coord_t dwidth;
    bdf_boundingbox_t bbx;
    bdf_byte_t * bitmap;

    int id;
} bdf_glyph_t;

typedef struct _bdf_t
{
    int size;
    int pixel_size;
    int ascent;
    int descent;
    bdf_boundingbox_t bbx;
    bdf_glyph_t* glyph[BDF_MAX_GLYPH];
    unsigned char cell_width[BDF_MAX_GLYPH];

    int numGlyph;
    int indexFirst;
    int indexLast;

    int verbose;
    char *loadingFilename;
} bdf_t;

#ifdef __cplusplus
extern "C" {
#endif

bdf_t*		bdf_open();
void		bdf_close(bdf_t* font);
int		bdf_load(bdf_t* font, char* filename);
bdf_glyph_t*	bdf_get_glyph(bdf_t* font, int id);
int		bdf_get_pixel(bdf_t* font, int id, int x, int y);
int		bdf_get_pixel_glyph(bdf_glyph_t* glyph, int x, int y);

#ifdef __cplusplus
}
#endif

/*****************************************************************************
 * BDF2
 *****************************************************************************/

typedef struct _bdf_list_t
{
    bdf_t* bdf;
    struct _bdf_list_t* next;
} bdf_list_t;

typedef struct _bdf2_t
{
    int count;
    bdf_list_t* list;
    int *sizelist;
    int has_multicell;

    int emX;
    int emY;
    int emAscent;
    int emDescent;
    int flagAutoName;
    int flagRegular;
    int flagBold;
    int flagItalic;

    int			glyph_id[BDF_MAX_GLYPH];
    unsigned char	glyph_flag[BDF_MAX_GLYPH];
    unsigned char	glyph_width[BDF_MAX_GLYPH];
    int numGlyph;
    int indexFirst;
    int indexLast;

    int verbose;
} bdf2_t;

#define BDF2GF_AVAILABLE (1 << 0)

#ifdef __cplusplus
extern "C" {
#endif

bdf2_t*		bdf2_open();
void		bdf2_close(bdf2_t* font);
int		bdf2_load(bdf2_t* font, char* filename);
bdf_glyph_t*	bdf2_get_glyph(bdf2_t* font, int size, int id);
int		bdf2_get_pixel(bdf2_t* font, int size, int id, int x, int y);
bdf_t*		bdf2_get_bdf1(bdf2_t* font, int size);
int		bdf2_get_count(bdf2_t* font);
int*		bdf2_get_sizelist(bdf2_t* font);
int		bdf2_get_glyph_id(bdf2_t* font, int id);
unsigned char	bdf2_get_glyph_flag(bdf2_t* font, int id);
unsigned char	bdf2_get_glyph_width(bdf2_t* font, int id);
int		bdf2_is_glyph_available(bdf2_t* font, int id);

#ifdef __cplusplus
}
#endif

#endif /* BDF_H */
