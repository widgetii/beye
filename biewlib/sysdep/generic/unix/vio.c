/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/generic/unix/vio.c
 * @brief       slang/curses/vt100 implementation of screen functions
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Konstantin Boldyshev
 * @since       1999
 * @note        Development, fixes and improvements
**/

/*
    Copyright (C) 1999-2002 Konstantin Boldyshev <konst@linuxassembly.org>

    $Id$
*/

#ifndef lint
static const char rcs_id[] = "$Id$";
#endif

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "biewlib/biewlib.h"
#include "console.h"

#define VMAX_X __TVIO_MAXSCREENWIDTH
#define VMAX_Y	0x400

#define	_addr(x, y)	(viomem + ((x) + (y) * tvioWidth))

int console_flags = 0, on_console = 0, output_7 = 0, transparent = 0;
tAbsCoord tvioWidth = 80, tvioHeight = 25;
unsigned tvioNumColors = 16;

static int initialized = 0, cursor_status = __TVIO_CUR_NORM;
static tAbsCoord saveX, saveY, firstX = 0, firstY = 0;

static unsigned violen;
static unsigned char *viomem;

static struct {
    unsigned char last;
    unsigned char color[0x10];
} tp = { 0, {0} };

#ifdef	_SLANG_
#include <slang.h>
#define _bg(x) ((x) >> 4)
#define _fg(x) ((x) & 0x0f)

static char *_color[16] = {
"black",
"blue",
"green",
"cyan",
"red",
"magenta",
"brown",
"lightgray",
"gray",
"brightblue",
"brightgreen",
"brightcyan",
"brightred",
"brightmagenta",
"yellow",
"white"
};

static unsigned char frames_slang[0x30] = {
SLSMG_CKBRD_CHAR, SLSMG_CKBRD_CHAR,	SLSMG_CKBRD_CHAR, SLSMG_VLINE_CHAR,
SLSMG_RTEE_CHAR, SLSMG_RTEE_CHAR,	SLSMG_RTEE_CHAR, SLSMG_URCORN_CHAR,
SLSMG_URCORN_CHAR, SLSMG_RTEE_CHAR,	SLSMG_VLINE_CHAR, SLSMG_URCORN_CHAR,
SLSMG_LRCORN_CHAR, SLSMG_LRCORN_CHAR,	SLSMG_LRCORN_CHAR, SLSMG_URCORN_CHAR,
SLSMG_LLCORN_CHAR, SLSMG_DTEE_CHAR,	SLSMG_UTEE_CHAR, SLSMG_LTEE_CHAR,
SLSMG_HLINE_CHAR, SLSMG_PLUS_CHAR,	SLSMG_LTEE_CHAR, SLSMG_LTEE_CHAR,
SLSMG_LLCORN_CHAR, SLSMG_ULCORN_CHAR,	SLSMG_DTEE_CHAR, SLSMG_UTEE_CHAR,
SLSMG_LTEE_CHAR, SLSMG_HLINE_CHAR,	SLSMG_PLUS_CHAR, SLSMG_DTEE_CHAR,
SLSMG_DTEE_CHAR, SLSMG_UTEE_CHAR,	SLSMG_UTEE_CHAR, SLSMG_LLCORN_CHAR,
SLSMG_LLCORN_CHAR, SLSMG_ULCORN_CHAR,	SLSMG_ULCORN_CHAR, SLSMG_PLUS_CHAR,
SLSMG_PLUS_CHAR, SLSMG_LRCORN_CHAR,	SLSMG_ULCORN_CHAR, SLSMG_BLOCK_CHAR,
SLSMG_BLOCK_CHAR, SLSMG_BLOCK_CHAR,	SLSMG_BLOCK_CHAR, SLSMG_BLOCK_CHAR
};
#endif

#ifdef	_CURSES_
#include <curses.h>
#define _bg(x)	(((x) >> 4) & 7)
#define _fg(x)	((x) & 7)
#define _2cpair(x) (_bg(x) * 010 + _fg(x))
static int _color[8] = {
    COLOR_BLACK,
    COLOR_BLUE,
    COLOR_GREEN,
    COLOR_CYAN,
    COLOR_RED,
    COLOR_MAGENTA,
    COLOR_YELLOW,
    COLOR_WHITE
};

/*
    Curse curses.. they are pure evil.
    Why the hell ACS_ stuff are not constants???
    I guess curses developers did this on purpose, to make programs slow.

    I made an attempt to make things faster (to some extent) with NCURSES,
    otherwise examine how acs_map called in your library.
*/

#if defined(NCURSES_VERSION) && defined(NDEBUG)

static unsigned char frames_vt100[0x30] =
"aaaxuuukkuxkjjjkmvwtqnttmlvwtqnvvwwmmllnnjl00000";

#define _2ps(x) acs_map[frames_vt100[x - _PSMIN]]

#else
chtype __FASTCALL__ _2ps(unsigned char c)
{
#define	ret(x) return(x)
    switch (c) {
	case BAR0:	ret(ACS_CKBOARD);
	case BAR1:	ret(ACS_CKBOARD);
	case BAR2:	ret(ACS_CKBOARD);
	case VSLINE:	ret(ACS_VLINE);
	case RSSTEE:	ret(ACS_RTEE);
	case RSDTEE:	ret(ACS_RTEE);
	case RDSTEE:	ret(ACS_RTEE);
        case TRSDCR:	ret(ACS_URCORNER);
        case TRDSCR:	ret(ACS_URCORNER);
        case RDDTEE:	ret(ACS_RTEE);
        case VDLINE:	ret(ACS_VLINE);
        case TRDDCR:	ret(ACS_URCORNER);
        case LRDDCR:	ret(ACS_LRCORNER);
        case LRSDCR:	ret(ACS_LRCORNER);
        case LRDSCR:	ret(ACS_LRCORNER);
        case TRSSCR:	ret(ACS_URCORNER);
        case LLSSCR:	ret(ACS_LLCORNER);
        case BSSTEE:	ret(ACS_BTEE);
        case TSSTEE:	ret(ACS_TTEE);
        case LSSTEE:	ret(ACS_LTEE);
        case HSLINE:	ret(ACS_HLINE);
        case SSPLUS:	ret(ACS_PLUS);
        case LSDTEE:	ret(ACS_LTEE);
        case LDSTEE:	ret(ACS_LTEE);
        case LLDDCR:	ret(ACS_LLCORNER);
        case TLDDCR:	ret(ACS_ULCORNER);
        case BDDTEE:	ret(ACS_BTEE);
        case TDDTEE:	ret(ACS_TTEE);
        case LDDTEE:	ret(ACS_LTEE);
        case HDLINE:	ret(ACS_HLINE);
        case DDPLUS:	ret(ACS_PLUS);
        case BDSTEE:	ret(ACS_BTEE);
        case BSDTEE:	ret(ACS_BTEE);
        case TDSTEE:	ret(ACS_TTEE);
        case TSDTEE:	ret(ACS_TTEE);
        case BLDSCR:	ret(ACS_LLCORNER);
        case BLSDCR:	ret(ACS_LLCORNER);
        case TLSDCR:	ret(ACS_ULCORNER);
        case TLDSCR:	ret(ACS_ULCORNER);
        case SDPLUS:	ret(ACS_PLUS);
        case DSPLUS:	ret(ACS_PLUS);
        case BRSSCR:	ret(ACS_LRCORNER);
        case TLSSCR:	ret(ACS_ULCORNER);
        case DBLK:	ret(ACS_BLOCK);
        case BBLK:	ret(ACS_BLOCK);
        case LBLK:	ret(ACS_BLOCK);
        case RBLK:	ret(ACS_BLOCK);
        case TBLK:	ret(ACS_BLOCK);
    }
    ret(0);
#undef ret
}
#endif

#endif	/* _CURSES_ */

#ifdef	_VT100_

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define _bg(x) ((x) >> 4)
#define _fg(x) ((x) & 0x0f)

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#define VTMP_LEN 100

static char *vtmp;
static int out_fd;
static int _color[8] = {0,4,2,6,1,5,3,7};
static char *screen_cp;
static unsigned is_unicode=0;
extern int   nls_init(const char *to,const char *from);
extern void  nls_term(void);
extern char *nls_get_screen_cp(void);
extern char *nls_recode2screen_cp(const char *srcb,unsigned* len);


static unsigned char frames_vt100[0x30] =
"aaaxuuukkuxkjjjkmvwtqnttmlvwtqnvvwwmmllnnjlaaaaa";

#define twrite(x)	write(out_fd,(x),strlen(x))

/**
    convert vga attrubute to ansi escape sequence
*/
static char *__FASTCALL__ _2ansi(unsigned char attr)
{
    int bg = _bg(attr);
    int bc = _color[bg & 7];

    if (transparent) {
	int i;
	for (i = 0; i < tp.last; i++)
	    if (bg == tp.color[i]) bc = 9;
    }

    sprintf(vtmp,
	"\033[%d;3%d;4%d%sm",
	_fg(attr) > 7,
	_color[_fg(attr) & 7],
	bc,
	bg > 7 ? ";5" : ""
    );
    return vtmp;
}

#endif	/* _VT100_ */

static unsigned char frames_dumb[0x30] =
": %|{+++++|+++++`++}-++++++++-+++++++++++++#%[]~";

/*

*/

inline static int __FASTCALL__ printable(unsigned char c)
{
    int result;
    if(is_unicode) result = !(c < 0x20 || c == 0x7f);
    else    result = !(c < 0x20 || c == 0x7f || c == 0x9b); /* 0x80< c < 0xA0 */

    if (result && terminal->type == TERM_XTERM && !is_unicode)
	result = !(
		c == 0x84 || c == 0x85 || c == 0x88 ||
		(c >= 0x8D && c <= 0x90) ||
		(c >= 0x96 && c <= 0x98) || 
		(c >= 0x9A && c <= 0x9F) );

    return result;
}

static void gotoxy(int x, int y)
{
#ifdef	_SLANG_
    SLsmg_gotorc(y, x);
#endif
#ifdef	_CURSES_
    move(y, x);
#endif
#ifdef	_VT100_
    sprintf(vtmp,"\033[%d;%dH", y + 1, x + 1);
    twrite(vtmp);
#endif
}

int __FASTCALL__ __vioGetCursorType(void)
{
    return cursor_status;
}

void __FASTCALL__ __vioSetCursorType(int type)
{
    cursor_status = type;
#ifdef	_SLANG_
    SLtt_set_cursor_visibility(type);
#endif
#ifdef	_CURSES_
    curs_set(type);
#endif
#ifdef	_VT100_
    if	(terminal->type == TERM_LINUX &&
	(cursor_status == __TVIO_CUR_SOLID || type == __TVIO_CUR_SOLID)) {
	sprintf(vtmp,"\033[?%dc", type == __TVIO_CUR_SOLID ? 7 : 0);
	twrite(vtmp);
    }
    sprintf(vtmp, "\033[?25%c", type == __TVIO_CUR_OFF ? 'l' : 'h');
    twrite(vtmp);
#endif
}

void __FASTCALL__ __vioGetCursorPos(tAbsCoord *x,tAbsCoord *y)
{
    *x = saveX;
    *y = saveY;
}

void __FASTCALL__ __vioSetCursorPos(tAbsCoord x,tAbsCoord y)
{
    saveX = x;
    saveY = y;
    gotoxy(x, y);
}

void __FASTCALL__ __vioReadBuff(tAbsCoord x, tAbsCoord y, tvioBuff *buff, unsigned len)
{
    unsigned char *addr = _addr(x, y);

    memcpy(buff->attrs, addr, len);
    memcpy(buff->chars, addr + violen, len);
    memcpy(buff->oem_pg, addr + (violen << 1), len);
}

void __FASTCALL__ __vioWriteBuff(tAbsCoord x, tAbsCoord y, const tvioBuff *buff, unsigned len)
{
    unsigned i;
    unsigned char c;
    unsigned char *addr;
    tAbsCoord xx, yy;
#ifdef	_CURSES_
    chtype ch = 0;
#endif

#ifdef	_VT100_

#define	LEN(x) (x << 4)
    unsigned char mode = 0, old_mode = -1;
    unsigned char cache_pb[LEN(VMAX_X)];
    unsigned char *dpb,*pb = len > VMAX_X ? malloc(LEN(len)) : cache_pb;
    unsigned slen;

    if (pb == NULL) {
	printm("Memory allocation failed: %s\nExiting..", strerror(errno));
	exit(errno);
    }
    dpb=pb;

    memset(pb, 0, LEN(len));
#endif

/*    if (!len) return; */

    addr = _addr(x, y);
    memcpy(addr, buff->attrs, len);
    memcpy(addr + violen, buff->chars, len);
    memcpy(addr + (violen << 1), buff->oem_pg, len);

    __vioGetCursorPos(&xx, &yy);
    gotoxy(x, y);

    for (i = 0; i < len; i++) {
	c = buff->chars[i];

#define cp buff->oem_pg[i]
#define ca buff->attrs[i]

#ifdef	_CURSES_
	ch = 0;
#endif

	if (cp && cp >= _PSMIN && cp <= _PSMAX && !is_unicode) {
#ifdef	_SLANG_
		c = output_7 ?
		    frames_dumb[cp - _PSMIN] :
		    frames_slang[cp - _PSMIN];
#endif
#ifdef	_CURSES_
		c = output_7 ?
		    frames_dumb[cp - _PSMIN] :
		    (ch = _2ps(cp)) & 0xff;
#endif
#ifdef	_VT100_
		mode = 1;
		c = output_7 ?
		    frames_dumb[cp - _PSMIN] :
		    frames_vt100[cp - _PSMIN];
	} else {
	    mode = 0;
#endif
	}

	if (output_7) c &= 0x7f;
#ifdef	_VT100_
	else {
	    char *map = mode ? "\016" : "\017";
	    if (old_mode != mode)
	    {
		strcpy(dpb,map);
		dpb += strlen(map);
	    }
	    old_mode = mode;
	}
#endif
	if (!c) c = ' '; else if (!printable(c)) c = '.';

#ifdef	_SLANG_
	if (SLsmg_get_column() >= SLtt_Screen_Cols)
		SLsmg_write_char ('\n');

	SLsmg_set_color(ca);
	SLsmg_set_char_set(!output_7 && cp);
	SLsmg_write_char (c);
#endif
#ifdef	_CURSES_
	ch |= COLOR_PAIR(_2cpair(ca));
	if ((ca & 0xf) > 7)	ch |= A_BOLD;
	if ((ca & 0xf0) > 0x70)	ch |= A_BLINK;
	addch(ch | c);
#endif
#ifdef	_VT100_
	if ((ca != buff->attrs[i - 1] && i) || i == len || !i)
	{
	    unsigned char *d;
	    d = _2ansi(ca);
	    strcpy(dpb, d);
	    dpb += strlen(d);
	}
	if(!is_unicode) {
	    *dpb=c; dpb++;
	}
	else {
	    unsigned len=1;
	    char *destb=nls_recode2screen_cp(&c,&len);
	    memcpy(dpb,destb,len);
	    free(destb);
	    dpb+=len;
	}
#endif
    }
#ifdef	_SLANG_
    gotoxy(xx, yy);
    SLsmg_refresh();
#endif
#ifdef	_CURSES_
    refresh();
    gotoxy(xx, yy);
#endif
#ifdef	_VT100_
    *dpb=0;
    dpb=pb;
    slen=strlen(dpb);
    while(slen)
    {
	unsigned stored=twrite(dpb);
	dpb+=stored;
	slen-= stored;
    }
    gotoxy(xx, yy);
    if (pb != cache_pb) free(pb);
#undef	LEN
#endif

#undef	ca
#undef	cp
}

void __FASTCALL__ __init_vio(const char *user_cp,unsigned long flags)
{
#ifdef	_VT100_
    struct winsize w;
#else
    int i;
#endif

#ifdef HAVE_ICONV
    screen_cp=nls_get_screen_cp();
    if(strncmp(screen_cp,"UTF",3)==0) {
	is_unicode=1;
    }
    if(nls_init(screen_cp,user_cp)) is_unicode=0;
#endif
    console_flags = flags;

    if (!output_7) output_7 = TESTFLAG(console_flags, __TVIO_FLG_USE_7BIT);
    do_nls = 1;

#ifdef	_SLANG_
    SLtt_get_terminfo();
    SLkp_init();
    SLang_init_tty(0xff, 1, 1);
    for (i = 1; i < 0xff; i++) {
	char *bc = _color[_bg(i)];
	if (transparent) {
	    int j;
	    for (j = 0; j < tp.last; j++)
		if (_bg(i) == tp.color[j]) bc = "default";
	}
	SLtt_set_color(i, NULL, _color[_fg(i)], bc);
    }
    SLsmg_init_smg();
    SLsmg_Display_Eight_Bit = 0x80;
    SLsmg_Newline_Behavior = SLSMG_NEWLINE_SCROLLS;
    tvioWidth = SLtt_Screen_Cols;
    tvioHeight = SLtt_Screen_Rows;
    tvioNumColors = SLtt_Use_Ansi_Colors ? 16 : 2;
    if (!SLtt_Has_Alt_Charset) output_7 = 1;
#endif
#ifdef	_CURSES_
    initscr();
    if (has_colors()) start_color();
    else tvioNumColors = 2;
    tvioWidth = COLS;
    tvioHeight = LINES;
    for (i = 0; i < 0xff; i++)
	init_pair(_2cpair(i), _color[_fg(i)], _color[_bg(i)]);
#endif
#ifdef	_VT100_
    if ((vtmp = malloc(VTMP_LEN)) == NULL) {
	printm("Can't allocate memory for output: %s\nExiting..", strerror(errno));
	exit(errno);
    }
    memset(vtmp, 0, VTMP_LEN);

    out_fd = open(ttyname(STDOUT_FILENO), O_WRONLY);
    if (out_fd < 0) out_fd = STDOUT_FILENO;

    ioctl(out_fd, TIOCGWINSZ, &w);
    tvioWidth = w.ws_col;
    tvioHeight = w.ws_row;
#endif

    if (tvioWidth <= 0) tvioWidth = 80;
    if (tvioHeight <= 0) tvioHeight = 25;
    if (tvioWidth > VMAX_X) tvioWidth = VMAX_X;
    if (tvioHeight > VMAX_Y) tvioHeight = VMAX_Y;
    saveX = firstX;
    saveY = firstY;
    violen = tvioWidth * tvioHeight;

    if ((viomem = malloc((violen << 1) + violen)) == NULL) {
	printm("Can't allocate memory for output: %s\nExiting..", strerror(errno));
	exit(errno);
    }
    memset(viomem, 0, (violen << 1) + violen);

#ifdef	_VT100_
    if (terminal->type != TERM_XTERM || transparent)
	twrite("\033(K\033)0");
    if (terminal->type == TERM_XTERM)
	twrite("\033[?1001s\033[?1000h\033]0;BIEW: Binary vIEWer\007");
    twrite("\033[0m\033[3h");
#endif
    initialized = 1;
}

void __FASTCALL__ __term_vio(void)
{
    if (!initialized) return;

    __vioSetCursorPos(firstX, firstY);
    __vioSetCursorType(__TVIO_CUR_NORM);

#ifdef	_SLANG_
    SLsmg_reset_smg();
    SLang_reset_tty();
#endif
#ifdef	_CURSES_
    endwin();
#endif
#ifdef	_VT100_
    if (terminal->type == TERM_XTERM) {
	twrite("\033[?1001r\033[?1000l");
	if (transparent) twrite("\033(K");
    }
    twrite("\033[3l\033[0m\033[2J");
    close(out_fd);
    free(vtmp);
#endif
    free(viomem);
    nls_term();
    initialized = 0;
}

void __FASTCALL__ __vioSetTransparentColor(unsigned char value)
{
    if (value == 0xff) {
	int i = 0;
	for (i = 0; i < 0x10; i++) tp.color[i] = 0xff;
	tp.last = 0;
    } else if (tp.last < 0x10) {
	tp.color[tp.last] = value;
	tp.last++;
    }
}
