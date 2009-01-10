/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/generic/linux/vio.c
 * @brief       general implementation of video i/o functions for linux.
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
 * @author      Alexander Krisak and Andrew Golovnia
 * @date        24.07.2003
 * @note        Russian locales support: KOI-8, CP866, CP1251, ISO8859-5.
 *              Tested at ASPLinux 7.3 and ASPLinux 9
 **/

/*
    Copyright (C) 1999-2002 Konstantin Boldyshev <konst@linuxassembly.org>

    $Id$
*/

#ifndef lint
static const char rcs_id[] = "$Id$";
#endif

#define _XOPEN_SOURCE 500

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#define __USE_BSD
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/vt.h>
#include "biewlib/biewlib.h"
#include "console.h"

#if (__GLIBC__ >= 2) && (__GLIBC_MINOR__ >= 1)
#define	PWRITE pwrite
#else
#define PWRITE(handle, buf, len, offset) lseek(handle, offset, SEEK_SET); write(handle, buf, len);
#endif

#define VIO_FILE "/dev/vcsa0"

#define VMAX_X __TVIO_MAXSCREENWIDTH
#define VMAX_Y 0x400

#define VTMP_LEN 0x80

tAbsCoord tvioWidth, tvioHeight;
unsigned tvioNumColors = 16;

int console_flags = 0, on_console = 0, output_7 = 0, transparent = 0;

static unsigned char __ansi_color[8] = {0,4,2,6,1,5,3,7};
static tAbsCoord saveX, saveY, firstX, firstY;
static int cursor_type = __TVIO_CUR_NORM;
static int viohandle, out_fd, initialized, output_G1 = 0, no_frames = 0;

static unsigned violen;
static unsigned char *viomem;
static char *vtmp;

static unsigned char frames_vt100[0x30] =
"aaaxuuukkuxkjjjkmvwtqnttmlvwtqnvvwwmmllnnjl00000";

static unsigned char frames_dumb[0x30] =
": %|{+++++|+++++`++}-++++++++-+++++++++++++#%[]~";

static char *screen_cp;
static unsigned is_unicode=0;
extern int   nls_init(const char *to,const char *from);
extern void  nls_term(void);
extern char *nls_get_screen_cp(void);
extern char *nls_recode2screen_cp(const char *srcb,unsigned* len);


#define twrite(x)	write(out_fd, (x), strlen(x))
#define _addr(x, y)	(viomem + (x) + (y) * tvioWidth)
#define _bg(x)		((x) >> 4)
#define _fg(x)		((x) & 0x0f)
#define _2color(x)	__Xlat__(__ansi_color, (x) & 7)
/* #define _2color(x) (char)__ansi_color[((x) & 7)] */

#define VT100_CLEARSCREEN	"\033[2J"
#define VT100_CLEARLINE		"\033[2K"
#define VT100_NORMALCHARS	"\033[0m"
#define VT100_BOLDCHARS		"\033[1m"
#define VT100_UNDERLINECHARS	"\033[1m"
#define VT100_MODE80		"\033[?3h"
#define VT100_MODE132		"\033[?3l"

#define VT100_SET_CHARSET_UK_G0		"\033(A"
#define VT100_SET_CHARSET_UK_G1		"\033)A"
#define VT100_SET_CHARSET_US_G0		"\033(B"
#define VT100_SET_CHARSET_US_G1		"\033)B"
#define VT100_SET_CHARSET_GR_G0		"\033(0"
#define VT100_SET_CHARSET_GR_G1		"\033)0"
#define VT100_SET_CHARSET_ROM_G0	"\033(1"
#define VT100_SET_CHARSET_ROM_G1	"\033)1"
#define VT100_SET_CHARSET_SPEC_GR_G0	"\033(2"
#define VT100_SET_CHARSET_SPEC_GR_G1	"\033)2"

static struct {
    unsigned char last;
    unsigned char color[0x10];
} tp = { 0, {0} };

static char *__FASTCALL__ _2ansi(unsigned char attr)
{
    int bg = _bg(attr);
    int bc = _2color(bg);

    if (transparent) {
	int i;
	for (i = 0; i < tp.last; i++)
	    if (bg == tp.color[i]) bc = 9;
    }

    sprintf(vtmp,
	"\033[%d;3%d;4%d%sm",
	_fg(attr) > 7,
	_2color(_fg(attr) & 7),
	bc,
	bg > 7 ? ";5" : ""
    );
    return vtmp;
}

inline static int __FASTCALL__ printable(unsigned char c)
{
    int result;
    if(is_unicode) result = !(c < 0x20 || c == 0x7f);
    else result = !(c < 0x20 || c == 0x7f || c == 0x9b); /* 0x80< c < 0xA0 */

    if (result && terminal->type == TERM_XTERM && !is_unicode)
	result = !(
		c == 0x84 || c == 0x85 || c == 0x88 ||
		(c >= 0x8D && c <= 0x90) ||
		(c >= 0x96 && c <= 0x98) || 
		(c >= 0x9A && c <= 0x9F) );

    return result;
}

static void __FASTCALL__ gotoxy(tAbsCoord x, tAbsCoord y)
{
    sprintf(vtmp,"\033[%d;%dH", (unsigned)(y + 1), (unsigned)(x + 1));
    twrite(vtmp);
}

int __FASTCALL__ __vioGetCursorType( void )
{
    return cursor_type;
}

void __FASTCALL__ __vioSetCursorType(int type)
{
    cursor_type = type;
    if	(terminal->type == TERM_LINUX && (cursor_type == __TVIO_CUR_SOLID || type == __TVIO_CUR_SOLID)) {
	sprintf(vtmp,"\033[?%dc", type == __TVIO_CUR_SOLID ? 7 : 0);
	twrite(vtmp);
    }
    sprintf(vtmp,"\033[?25%c", type ? 'h' : 'l');
    twrite(vtmp);
}

void __FASTCALL__ __vioGetCursorPos(tAbsCoord *x, tAbsCoord *y)
{
    *x = saveX;
    *y = saveY;
}

void __FASTCALL__ __vioSetCursorPos(tAbsCoord x, tAbsCoord y)
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
#define	LEN(x) (x << 4)
    unsigned char cache_pb[LEN(VMAX_X)];
    unsigned char *pb, *dpb, *addr;

/*    if (!len) return; */

    pb = len > VMAX_X ? malloc(LEN(len)) : cache_pb;
    if (pb == NULL) {
	printm("Memory allocation failed: %s\nExiting..", strerror(errno));
	exit(errno);
    }

    addr = _addr(x, y);

    memcpy(addr, buff->attrs, len);
    memcpy(addr + violen, buff->chars, len);
    memcpy(addr + (violen << 1), buff->oem_pg, len);

    if (on_console) {

        __INTERLEAVE_BUFFERS(len, pb, buff->chars, buff->attrs);

	PWRITE(viohandle, pb, len << 1, 4 + ((x + y * tvioWidth) << 1));

    } else {
	unsigned i;
	unsigned char c,slen;
	unsigned mode = 0, old_mode = -1;
	tAbsCoord xx, yy;

	__vioGetCursorPos(&xx, &yy);

	gotoxy(x, y);

	dpb=pb;
	memset(pb, 0, LEN(len));

	for (i = 0; i < len; i++) {
	    c = buff->chars[i];
#define cp buff->oem_pg[i]
#define ca buff->attrs[i]

	    if (cp && cp >= _PSMIN && cp <= _PSMAX && !is_unicode) {
		c = (output_7 || no_frames) ? __Xlat__(frames_dumb,cp - _PSMIN) :
		    (output_G1)	? __Xlat__(frames_vt100,cp - _PSMIN) : cp;
/*
		c = (output_7)	? frames_dumb[cp - _PSMIN] :
		    (output_G1)	? frames_vt100[cp - _PSMIN] : cp;
*/
		mode = 1;
	    } else {
		mode = 0;
	    }

	    if (output_7) {
		c &= 0x7f;
	    } else {
		unsigned char *map = mode ?
			    (output_G1 ? VT100_SET_CHARSET_GR_G1"\016" : "\033(U"):
			    (output_G1 ? VT100_SET_CHARSET_US_G1"\017" : "\033(K");

		if (output_G1 && old_mode != mode)
		{
		    strcpy(dpb, map);
		    dpb += strlen(map);
		}
		old_mode = mode;
	    }

	    if (!c) c = ' '; else if (!printable(c)) c = '.';

	    if (!i || (ca != buff->attrs[i - 1])) /* [dBorca] short-circuit */
	    {
		unsigned char *ptr;
		ptr = _2ansi(ca);
		strcpy(dpb, ptr);
		dpb += strlen(ptr);
	    }
	    if(is_unicode) {
		unsigned len=1;
		char *destb=nls_recode2screen_cp(&c,&len);
		memcpy(dpb,destb,len);
		free(destb);
		dpb+=len;
	    }
	    else {
		*dpb=c; dpb++;
	    }
	}
	*dpb=0;
	/* Note: twRefreshWin() may pass HUGE buffer here.
	   But some stupid terminals can write only a few KB
	   per call! So this loop is required as workaround for them */
	dpb=pb;
	slen=strlen(dpb);
	while(slen)
	{
	    unsigned stored;
	    stored = twrite(dpb);
	    dpb+=stored;
	    slen-=stored;
	}
	gotoxy(xx, yy);
    }
    if (pb != cache_pb) free(pb);
#undef	LEN
#undef	ca
#undef	cp
}

void __FASTCALL__ __init_vio(const char *user_cp,unsigned long flags)
{
    struct vt_mode vtmode;
    char *tty = ttyname(STDOUT_FILENO);

    console_flags = flags;

    out_fd = open(tty, O_WRONLY);
    if (out_fd < 0) out_fd = STDOUT_FILENO;

    if (TESTFLAG(console_flags, __TVIO_FLG_DIRECT_CONSOLE_ACCESS))
	on_console = ioctl(out_fd, VT_GETMODE, &vtmode) >= 0;

    if (on_console) {
	char vcsa_name[0x10];
	int len = strlen(tty);
	int i = 0;

	for (i = 0; i < len; i ++) if (isdigit((int)*(tty + i))) break;

	if (i < len) sprintf(vcsa_name, "/dev/vcsa%d", atoi(tty + i));

	if ((viohandle = open(vcsa_name, O_RDWR)) < 0) {
	    sprintf(vcsa_name, "/dev/vcsa");
	    if ((viohandle = open(vcsa_name, O_RDWR)) < 0) {
		printm("Can't open %s: %s\nDirect console access disabled..\n", vcsa_name, strerror(errno));
		on_console = 0;
	    }
	}
    }

    if (on_console) {
	unsigned char b[4];
	read(viohandle, &b, 4);
	tvioHeight = b[0]; tvioWidth = b[1];
	firstX = b[2]; firstY = b[3];
    } else {
	struct winsize w;
	ioctl(out_fd, TIOCGWINSZ, &w);
	tvioWidth = w.ws_col;
	tvioHeight = w.ws_row;
    }
#ifdef HAVE_ICONV
    screen_cp=nls_get_screen_cp();
    if(strncasecmp(screen_cp,"UTF",3)==0 && !on_console) {
	is_unicode=1;
    }
#endif
    if(nls_init(screen_cp,user_cp)) is_unicode=0;
    if (!output_7) output_7 = TESTFLAG(console_flags, __TVIO_FLG_USE_7BIT);
    if (tvioWidth <= 0) tvioWidth = 80;
    if (tvioHeight <= 0) tvioHeight = 25;
    if (tvioWidth > VMAX_X) tvioWidth = VMAX_X;
    if (tvioHeight > VMAX_Y) tvioHeight = VMAX_Y;
    saveX = firstX;
    saveY = firstY;
    violen = tvioWidth * tvioHeight;

    vtmp = malloc(VTMP_LEN);
    viomem = malloc((violen << 1) + violen);
    if (vtmp == NULL || viomem == NULL) {
	printm("Can't allocate memory for output: %s\nExiting..\n", strerror(errno));
	exit(errno);
    }
    memset(viomem, 0, (violen << 1) + violen);
    memset(vtmp, 0, VTMP_LEN);

    do_nls = 0;		/* disable nls convertion */

    if (on_console) {
	unsigned char *buf = malloc(violen << 1);

	if (buf != NULL) {
	    unsigned i;

	    read(viohandle, buf, violen << 1);
	    for (i = 0; i < (violen << 1); i += 2) {
    		viomem[violen + (i >> 1)] = buf[i];
		viomem[i >> 1] = buf[i + 1];
	    }
	    free(buf);
	}
    } else {
	twrite(VT100_NORMALCHARS VT100_MODE80);
	if (terminal->type != TERM_LINUX) {
	    do_nls = 1;
	    output_G1 = 1;
	    if (terminal->type == TERM_XTERM) {
		twrite("\033[?1000h\033]0;BIEW: Binary vIEWer\007");
		frames_vt100[0x2B] =
		frames_vt100[0x2C] =
		frames_vt100[0x2D] =
		frames_vt100[0x2E] =
		frames_vt100[0x2F] = 'a';
		if (transparent)
		    twrite("\033(K\033)0");
	    }
	} else { char* lang;
	    if (((lang = getenv("LANG")) != NULL) && 
	        (strncmp(lang, "ru_", 3) == 0) &&
	        (strcasecmp(lang, "ru_RU.utf8") != 0) &&
	        (strcasecmp(lang, "ru_UA.utf8") != 0))
	    {// twrite("\033(K\\033)K");/*set user defined mapping*/
	      do_nls = 1;
	      no_frames = 1;
	    }
	    else twrite("\033(U");	/* set null mapping */
	}
    }
    initialized = 1;
}

void __FASTCALL__ __term_vio(void)
{
    if (!initialized) return;

    __vioSetCursorType(__TVIO_CUR_NORM);
    if (!on_console) {
	if (terminal->type == TERM_XTERM) twrite("\033[?1001r\033[?1000l");
	if (terminal->type != TERM_XTERM || transparent)
	    twrite("\033(K");
	twrite(VT100_MODE80 VT100_NORMALCHARS VT100_CLEARSCREEN);
    }
    __vioSetCursorPos(firstX, firstY);

    nls_term();
    if (on_console) close(viohandle);

    free(vtmp);
    free(viomem);

    close(out_fd);
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
