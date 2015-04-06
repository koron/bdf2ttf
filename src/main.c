/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * main.c - BDF to TTF converter entry point.
 *
 * Written By:	MURAOKA Taro <koron@tka.att.ne.jp>
 * Last Change: 12-Aug-2005.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rcfile.h"
#include "bdf.h"
#include "bdf2ttf.h"
#include "debug.h"

    int
main(int argc, char** argv)
{
    int retval = 1;
    int i;
    int count;
    int *sizelist;
    int n_coresize = 0;
    int em_basesize;
    char *outfile;
    char *cr, *ver, *trade, *coresize;
    bdf2_t *font    = NULL;
    bdf_t *bdf1	    = NULL;
    rcfile_t *rc    = NULL;
    char* progname  = argv[0];
    int flag_autoname	= 1;
    int	flag_regular	= 1;
    int flag_bold	= 0;
    int flag_italic	= 0;
    int flag_stylecheck	= 1;
    int verbose_level	= 0;

    ++argv;
    --argc;

    while (argc > 0 && argv[0] && (argv[0])[0] == '-')
    {
	char *p = &argv[0][1];

	if (!strcmp(p, "b") || !strcmp(p, "-bold"))
	    flag_bold = 1;
	else if (!strcmp(p, "i") || !strcmp(p, "-italic"))
	    flag_italic = 1;
	else if (!strcmp(p, "v") || !strcmp(p, "-verbose"))
	    ++verbose_level;
	else if (!strcmp(p, "-no-autoname"))
	    flag_autoname = 0;
	else if (!strcmp(p, "-no-stylecheck"))
	    flag_stylecheck = 0;
	++argv;
	--argc;
    }

    /* Check and correct style coherency */
    if (flag_stylecheck)
    {
	if (flag_bold || flag_italic)
	    flag_regular = 0;
    }

    if (argc < 3) {
	printf("Usage: %s [flags] {outname} {info} {infile} [infiles..]\n", progname);
	goto FINISH;
    }

    /* Get all parameters */
    rc = rcfile_open(argv[1]);
    if (!rc)
    {
	printf("Can't open %s as convert_info.\n", argv[1]);
	goto FINISH;
    }
    outfile	= argv[0];
    g_fontname	= (char*)rcfile_get(rc, "Fontname");
    cr		= (char*)rcfile_get(rc, "Copyright");
    ver		= (char*)rcfile_get(rc, "Version");
    trade	= (char*)rcfile_get(rc, "Trademark");
    coresize	= (char*)rcfile_get(rc, "Coresize");
    g_fontname_cp	= (char*)rcfile_get(rc, "FontnameCP");
    g_copyright_cp	= (char*)rcfile_get(rc, "CopyrightCP");
    g_version_cp	= (char*)rcfile_get(rc, "VersionCP");
    g_trademark_cp	= (char*)rcfile_get(rc, "TrademarkCP");

    /* Verify parameters */
    if (!g_fontname) {
	printf("Didn't specify Fontname in %s.\n", argv[0]);
	goto FINISH;
    }
    g_copyright	= cr	? cr	: g_copyright;
    g_version	= ver	? ver	: g_version;
    g_trademark	= trade	? trade	: g_trademark;
    g_copyright_cp  = g_copyright_cp	? g_copyright_cp: g_copyright;
    g_fontname_cp   = g_fontname_cp	? g_fontname_cp	: g_fontname;
    g_version_cp    = g_version_cp	? g_version_cp	: g_version;
    g_trademark_cp  = g_trademark_cp	? g_trademark_cp: g_trademark;

    /* Read BDF files */
    font = bdf2_open();
    font->verbose = verbose_level;
    printf("Input files:\n");
    for (i = 2; argv[i]; ++i)
    {
	int result;
	
	printf("  %s\n", argv[i]);
	result = bdf2_load(font, argv[i]);
    }
    count = bdf2_get_count(font);
    TRACE("bdf2_get_count()=%d\n", count);
    if (count == 0)
    {
	printf("Any BDF files are not loaded.\n");
	goto FINISH;
    }

    /* Set font global information */
    sizelist = bdf2_get_sizelist(font);
    if (coresize)
	n_coresize = atoi(coresize);
    if (n_coresize <= 0 || !bdf2_get_bdf1(font, n_coresize))
	n_coresize = sizelist[count - 1];
    TRACE("n_coresize=%d\n", n_coresize);
    bdf1 = bdf2_get_bdf1(font, n_coresize);
    em_basesize = n_coresize; //bdf1->bbx.width;
    font->emX	    = emCalc(em_basesize, em_basesize);
    font->emY	    = emCalc(bdf1->bbx.height, em_basesize);
#if 0
    /*
     * ベースラインを加味したY軸併せが行なえる。しかしビットマップフォントを
     * 扱う上では不利なので、現在は使っていない。
     */
    font->emAscent  = emCalc(bdf1->ascent, bdf1->bbx.width);
    font->emDescent = emCalc(bdf1->descent, bdf1->bbx.width);
#else
    font->emAscent  = font->emY;
    font->emDescent = 0;
#endif
    TRACE("  emX=%d\n", font->emX);
    TRACE("  emY=%d\n", font->emY);
    TRACE("  emAscent=%d\n", font->emAscent);
    TRACE("  emDescent=%d\n", font->emDescent);

    /* for debugging */
    TRACE("bdf2_t.sizelist:\n");
    for (i = 0; i < count; ++i)
    {
	bdf_t *bdf;
	int size;

	size = sizelist[i];
	bdf = bdf2_get_bdf1(font, size);
	TRACE("  %d: w=%d h=%d ascent=%d descent=%d\n", size,
		bdf->bbx.width, bdf->bbx.height, bdf->ascent, bdf->descent);
    }

    font->flagAutoName	= flag_autoname;
    font->flagRegular	= flag_regular;
    font->flagBold	= flag_bold;
    font->flagItalic	= flag_italic;

    if (!write_ttf(font, outfile))
	goto FINISH;
    retval = 0;

FINISH:
    if (font)
	bdf2_close(font);
    if (rc)
	rcfile_close(rc);
    return retval;
}
