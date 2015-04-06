#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#pragma comment(lib, "gdi32")
#pragma comment(lib, "user32")

    int
main(int argc, char** argv)
{
    HFONT hfont = NULL;
    int height = -12;
    int width = 0;
#if 1
    char *fontname = "AA‚Ä‚·‚ÆƒtƒHƒ“ƒg";
#else
    char *fontname = "BDF M+";
#endif

    hfont = CreateFont(
	    height,
	    width,
	    0,
	    0,
	    FW_DONTCARE,
	    FALSE,
	    FALSE,
	    FALSE,
	    DEFAULT_CHARSET,
	    OUT_DEFAULT_PRECIS,
	    CLIP_DEFAULT_PRECIS,
	    DEFAULT_QUALITY,
	    DEFAULT_PITCH | FF_DONTCARE,
	    fontname
	    );
    if (!hfont)
    {
	printf("Can't create %s\n", fontname);
    }
    else
    {
	HWND	    hwnd;
	HDC	    hdc;
	HFONT	    hfntOld;
	TEXTMETRIC  tm;

	hwnd = GetDesktopWindow();
	hdc = GetWindowDC(hwnd);
	hfntOld = SelectObject(hdc, hfont);
	GetTextMetrics(hdc, &tm);

#define print_tm(t,m) printf(#m"=%d\n", t.m)
	print_tm(tm, tmHeight);
	print_tm(tm, tmAscent);
	print_tm(tm, tmDescent);
	print_tm(tm, tmAveCharWidth);
	print_tm(tm, tmMaxCharWidth);

	SelectObject(hdc, hfntOld);
	ReleaseDC(hwnd, hdc);
	DeleteObject(hfont);
    }
    return 0;

}
