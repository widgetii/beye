/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/linux/keyboard.c
 * @brief       Linux direct console / vt100 keyboard library
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
    Copyright (C) 1999-2001 Konstantin Boldyshev <konst@linuxassembly.org>

    $Id$
*/

#ifndef lint
static const char rcs_id[] = "$Id$";
#endif

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/kd.h>
#include <sys/vt.h>

#include "biewlib/kbd_code.h"
#include "biewlib/biewlib.h"
#include "console.h"

#ifdef HAVE_MOUSE
#include <gpm.h>
static int gpmhandle;
#endif

#define KBUFSIZE	64	/**< size of keyboard buffer */
#define KSCANSIZE	84	/**< size of scancode table */
#define KEYNUM		128	/* */

#define KSCAN_SHIFTL	0x2a
#define KSCAN_SHIFTR	0x36
#define KSCAN_CTRL	0x1d
#define KSCAN_ALT	0x38
#define KSCAN_CAPS	0x3a
#define KSCAN_NUM	0x45
#define KSCAN_SCROLL	0x46
#define KE_NONE		0xffffffff

static volatile int shift_status = 0,	/**< status of shift keys */
		    console_restart = 0,
		    keypressed[KEYNUM];	/**< indicates whether key is down */
static tBool mouse_status = True;
static struct termios sattr, tattr;	/**< terminal attributes */
static int in_fd;

/**
    keyboard FIFO
*/

static struct {
    int pool[KBUFSIZE];
    int current;
} keybuf;

/**
    scancode tables
*/

static unsigned int scancode_table[KSCANSIZE] =
{
    KE_ESCAPE,	'1',	'2',	'3',	'4',	'5',	'6',	'7',
    '8',	'9',	'0',	'-',	'=',	KE_BKSPACE,KE_TAB,'q',
    'w',	'e',	'r',	't',	'y',	'u',	'i',	'o',
    'p',	'[',	']',	KE_ENTER,KE_NONE,'a',	's',	'd',
    'f',	'g',	'h',	'j',	'k',	'l',	';',	0x27,
    '`',	KE_NONE,0x5c,	'z',	'x',	'c',	'v',	'b',
    'n',	'm',	',',	'.',	'/',	KE_NONE,'*',	KE_NONE,
    ' ',	KE_NONE,KE_F(1),KE_F(2),KE_F(3),KE_F(4),KE_F(5),KE_F(6),
    KE_F(7),	KE_F(8),KE_F(9),KE_F(10),KE_NONE,KE_NONE,KE_HOME,KE_UPARROW,
    KE_PGUP,	'-',	KE_LEFTARROW,0,KE_RIGHTARROW,'+',KE_END,KE_DOWNARROW,
    KE_PGDN,	KE_INS,	KE_DEL,KE_NONE
};

unsigned int scancode_caps_table[KSCANSIZE] =
{
    KE_ESCAPE,	'!',	'@',	'#',	'$',	'%',	'^',	'&',
    '*',	'(',	')',	'_',	'+',	KE_BKSPACE,KE_TAB,'Q',
    'W',	'E',	'R',	'T',	'Y',	'U',	'I',	'O',
    'P',	'{',	'}',	KE_ENTER,0,	'A',	'S',	'D',
    'F',	'G',	'H',	'J',	'K',	'L',	':',	0x22,
    '~',	KE_NONE,'|',	'Z',	'X',	'C',	'V',	'B',
    'N',	'M',	'<',	'>',	'?',	KE_NONE,'*',	KE_NONE,
    ' ',	0,	KE_F(1),KE_F(2),KE_F(3),KE_F(4),KE_F(5),KE_F(6),
    KE_F(7),	KE_F(8),	KE_F(9),	KE_F(10),	KE_NONE,	KE_NONE,	KE_HOME,KE_UPARROW,
    KE_PGUP,	'-',	KE_LEFTARROW,0,KE_RIGHTARROW,'+',KE_END,KE_DOWNARROW,
    KE_PGDN,	KE_INS,	KE_DEL,	KE_NONE
};

typedef struct {
    char c;
    int key;
} seqtbl;

typedef seqtbl pseq[];
typedef seqtbl p1seq[1]; /**< dummy type to make compiler happy */

typedef struct {
    char pre1;
    char pre2;
    char suf;
    p1seq *s;
} eseq;

/**
    translatable sequences
*/

const static pseq seq0 = {
    {'A',KE_UPARROW},
    {'B',KE_DOWNARROW},
    {'C',KE_RIGHTARROW},
    {'D',KE_LEFTARROW},
    {'P',KE_F(1)},
    {'Q',KE_F(2)},
    {'R',KE_F(3)},
    {'w',KE_F(3)},
    {'y',KE_F(3)},
    {'S',KE_F(4)},
    {'x',KE_F(4)},
    {'t',KE_F(5)},
    {'v',KE_F(5)},
    {'u',KE_F(6)},
    {'l',KE_F(6)},
    {'q',KE_F(7)},
    {'s',KE_F(7)},
    {'r',KE_F(8)},
    {'p',KE_F(9)},
    {'n',KE_F(9)},
    {0,0}},
seq1 = {
    {'A',KE_UPARROW},
    {'B',KE_DOWNARROW},
    {'C',KE_RIGHTARROW},
    {'D',KE_LEFTARROW},
    {'H',KE_HOME},
    {'K',KE_END},
    {0,0}},
seq2 = {
    {'A',KE_F(1)},
    {'B',KE_F(2)},
    {'C',KE_F(3)},
    {'D',KE_F(4)},
    {'E',KE_F(5)},
    {0,0}},
seq3 = {
    {'1',KE_HOME},
    {'2',KE_INS},
    {'3',KE_DEL},
    {'4',KE_END},
    {'5',KE_PGUP},
    {'6',KE_PGDN},
    {'7',KE_HOME},
    {'8',KE_END},
    {'J',KE_CTL_PGDN},
    {0,0}},
seq4 = {
    {'1',KE_F(1)},
    {'2',KE_F(2)},
    {'3',KE_F(3)},
    {'4',KE_F(4)},
    {'5',KE_F(5)},
    {'7',KE_F(6)},
    {'8',KE_F(7)},
    {'9',KE_F(8)},
    {0,0}},
seq5 = {
    {'0',KE_F(9)},
    {'1',KE_F(10)},
    {'3',KE_SHIFT_F(1)},
    {'4',KE_SHIFT_F(2)},
    {'5',KE_SHIFT_F(3)},
    {'6',KE_SHIFT_F(4)},
    {'8',KE_SHIFT_F(5)},
    {'9',KE_SHIFT_F(6)},
    {0,0}},
seq6 = {
    {'1',KE_SHIFT_F(7)},
    {'2',KE_SHIFT_F(8)},
    {'3',KE_SHIFT_F(9)},
    {'4',KE_SHIFT_F(10)},
    {0,0}},
seq7 = {
    {'1',KE_CTL_F(1)},
    {'2',KE_CTL_F(2)},
    {'3',KE_CTL_F(3)},
    {'4',KE_CTL_F(4)},
    {'5',KE_CTL_F(5)},
    {'7',KE_CTL_F(6)},
    {'8',KE_CTL_F(7)},
    {'9',KE_CTL_F(8)},
    {0,0}},
seq8 = {
    {'0',KE_CTL_F(10)},
    {'1',KE_CTL_F(11)},
    {0,0}};

#define SEQ_LEN 10	/**< max sequense length */
#define SEQ_NUM 9	/**< number of sequence categories */

static eseq S[SEQ_NUM] = {
{'O', 0, 0, (p1seq *)seq0 },
{'[', 0, 0, (p1seq *)seq1 },
{'[', '[', 0, (p1seq *)seq2 },
{'[', 0, '~', (p1seq *)seq3 },
{'[', '1', '~', (p1seq *)seq4 },
{'[', '2', '~', (p1seq *)seq5 },
{'[', '3', '~',	(p1seq *)seq6 },
{'[', '1', '^', (p1seq *)seq7 },
{'[', '2', '^', (p1seq *)seq8 },
};

static mevent mouse = {0, 0, 0, 0};

/*

*/

static void __FASTCALL__ pushEvent(unsigned event)
{
    if (event && keybuf.current < KBUFSIZE) {
	if (keybuf.current)
	    memmove(keybuf.pool, &keybuf.pool[1], keybuf.current);
	keybuf.pool[0] = event;
	keybuf.current++;
    }
}

static void __FASTCALL__ console_leave(void)
{
    ioctl(in_fd, TCSETSW, &sattr);
    ioctl(in_fd, KDSKBMODE, K_XLATE);
    ioctl(in_fd, VT_RELDISP, VT_ACKACQ);
    signal(SIGUSR1, (void *)(int) console_leave);
}

static void __FASTCALL__ console_enter(void)
{
    ioctl(in_fd, KDSKBMODE, K_RAW);
    ioctl(in_fd, TCSETSW, &tattr);
    memset(&keypressed, 0, KEYNUM * sizeof(int));
    console_restart = 1;
    signal(SIGUSR2, (void *)(int) console_enter);
}

/**
    ReadNextEvent is non-blocking
*/

void __FASTCALL__ ReadNextEvent(void)
{
#define get(x)	read(in_fd,&(x),1)
#define ret(x)	pushEvent((x)); return;
#define set_s(x) shift_status &= (x); shift_status ^= (x); ret(KE_SHIFTKEYS);

	unsigned key = 0;
	int i;

#ifdef HAVE_MOUSE
    if (gpmhandle) {
	fd_set gpmfds;
	struct timeval t = { 0, 0 };

	FD_ZERO(&gpmfds);
	FD_SET(gpmhandle, &gpmfds);

	if (select(gpmhandle + 1, &gpmfds, NULL, NULL, &t)) {
	    Gpm_Event ge;
	    Gpm_GetEvent(&ge);
	    if (ge.type & GPM_DOWN) {
		if (ge.buttons & GPM_B_LEFT)	
		    mouse.buttons |= MS_LEFTPRESS;
		if (ge.buttons & GPM_B_MIDDLE)
		    mouse.buttons |= MS_MIDDLEPRESS;
		if (ge.buttons & GPM_B_RIGHT)
		    mouse.buttons |= MS_RIGHTPRESS;
		mouse.x = ge.x - 1; 
		mouse.y = ge.y - 1;
		mouse.pressed = 1;
		if (mouse.buttons) ret(KE_MOUSE);
	    } else if (ge.type & GPM_UP) mouse.pressed = 0;
	}
    }
#endif

	/* If key is ready, place it into keyboard buffer */

        if (on_console) {
	    unsigned char c = 0;
	    int ss = shift_status;

	    do { get(c); } while (c == 0xe0);
	    if (c == 0xe1) get(c);
	    if (c == 0xC6) { break_status = True; return; } /* CTRL+BREAK check */
	    if (c) {
		keypressed[c & (KEYNUM - 1)] = c & KEYNUM ? 0 : 1;
		for (i = 1; i < KSCANSIZE; i++)
		    if (keypressed[i] &&
		    (i != KSCAN_SHIFTL) &&
		    (i != KSCAN_SHIFTR) &&
		    (i != KSCAN_CTRL) &&
		    (i != KSCAN_ALT) &&
		    (i != KSCAN_CAPS) &&
		    (i != KSCAN_NUM) &&
		    (i != KSCAN_SCROLL))
		    key = scancode_table[i - 1];
	    }
	    if (__kbdGetShiftsKey() != ss) key = KE_SHIFTKEYS;
	    if ((shift_status & KS_SHIFT) == KS_SHIFT) {
		for (i = 1; i < KSCANSIZE; i++)
		    if (keypressed[i] &&
		    (i != KSCAN_SHIFTL) &&
		    (i != KSCAN_SHIFTR) &&
		    (i != KSCAN_CTRL) &&
		    (i != KSCAN_ALT) &&
		    (i != KSCAN_CAPS) &&
		    (i != KSCAN_NUM) &&
		    (i != KSCAN_SCROLL))
		    key = scancode_caps_table[i - 1];
	    }
	    if (((shift_status & KS_ALT) == KS_ALT) &&
		((shift_status & KS_CTRL) == KS_CTRL) &&
		(key >= KE_F(1) && key <= KE_F(10))) {

		struct vt_stat vt;
		int newvt;

		if (ioctl(in_fd, VT_GETSTATE, &vt) != 0) goto c_end;
		switch (key) {
		    case KE_F(1): newvt = 1; break;
		    case KE_F(2): newvt = 2; break;
		    case KE_F(3): newvt = 3; break;
		    case KE_F(4): newvt = 4; break;
		    case KE_F(5): newvt = 5; break;
		    case KE_F(6): newvt = 6; break;
		    case KE_F(7): newvt = 7; break;
		    default: goto c_end;
		}

		if (vt.v_active != newvt) {
		    if (ioctl(in_fd, VT_ACTIVATE, newvt) != 0) goto c_end;
		    console_restart = 0;
		    while (!console_restart) __OsYield();
		}
	    }
c_end:
        } else {
/*
    VT100 emulation
*/
	    char c[SEQ_LEN];

	    if (get(c[0]) < 0) return;
	    
	    switch(c[0]) {
		case KE_ESCAPE		: break;
		case KE_STATUS_RESET	: set_s(0);
		case KE_STATUS_ALT	: set_s(KS_ALT);;
		case KE_STATUS_SHIFT	: set_s(KS_SHIFT);
		case KE_STATUS_CONTROL	: set_s(KS_CTRL);
		case 0			: break_status = True; break;
		case KE_ENTER2		: key = KE_ENTER; break;
		case KE_BKSPACE2	: key = KE_BKSPACE; break;
		case KE_C_O		: key = KE_CTL_(O); break;
		default			: key = c[0];
	    }
	    if (key) goto place_key;
	    for (i = 1; i < SEQ_LEN - 1; i++) {
		if(get(c[i]) < 0) break;
	    }
	    if (i < 3) {
		key = c[0];
		goto place_key;
	    }

    if (c[1] == '[' && c[2] == 'M' && i == 6) {
	switch (c[3] & 0x03) {
	    case 0: mouse.buttons |= MS_LEFTPRESS; break;
	    case 1: mouse.buttons |= MS_MIDDLEPRESS; break;
	    case 2: mouse.buttons |= MS_RIGHTPRESS; break;
	    default: mouse.pressed = 0; return;
	}
	mouse.x = c[4] - '!';
	mouse.y = c[5] - '!';
	mouse.pressed = 1;
	key = KE_MOUSE;
#ifdef	DEBUG
	printm("mouse at %02X, %02X - %02X\n", mouse.x, mouse.y, mouse.buttons);
#endif
	goto place_key;
    }

/*
    translate escape sequence
*/

#define S S[i]
	    for (i = 0; i < SEQ_NUM && !key; i++) {
		int j, n;

		if (c[1] != S.pre1) continue;
		n = 2;
		if (S.pre2) {
		    if (c[n] != S.pre2) continue;
		    n++;
		}
		for (j = 0; S.s[j]->c; j++) if (S.s[j]->c == c[n]) {
			if (!(S.suf && S.suf != c[n + 1])) {
			    key = S.s[j]->key; break;
			 }
	        }
	    }
#undef S
	}
place_key:
	ret(key);
#undef set_s
#undef ret
}

int __FASTCALL__ __kbdGetShiftsKey(void)
{
	/* shift_status = 6;ioctl(in_fd,TIOCLINUX,&shift_status); */
    if (on_console) {
	 shift_status = 0;
	 if (keypressed[KSCAN_SHIFTL] || keypressed[KSCAN_SHIFTR]) shift_status |= KS_SHIFT;
	 if (keypressed[KSCAN_CTRL])	shift_status |= KS_CTRL;
	 if (keypressed[KSCAN_ALT])	shift_status |= KS_ALT;
	 if (keypressed[KSCAN_CAPS])	shift_status |= KS_CAPSLOCK;
	 if (keypressed[KSCAN_NUM])	shift_status |= KS_NUMLOCK;
	 if (keypressed[KSCAN_SCROLL])	shift_status |= KS_SCRLOCK;
    }
    return shift_status;
}

int __FASTCALL__ __kbdTestKey(unsigned long flg)
{
    if(__MsGetBtns() && flg == KBD_NONSTOP_ON_MOUSE_PRESS) return KE_MOUSE;
    return keybuf.current;
}

int __FASTCALL__ __kbdGetKey (unsigned long flg)
{
    int key = 0, s = 0;

    if (__MsGetBtns() && flg == KBD_NONSTOP_ON_MOUSE_PRESS) return KE_MOUSE;

    while (!keybuf.current) {
	__OsYield();
#ifdef	HAVE_MOUSE
	if (gpmhandle) ReadNextEvent();
#endif
    }
    key = keybuf.pool[--keybuf.current];

    if (!(key == KE_MOUSE || key == KE_SHIFTKEYS)) {
	if ((shift_status & KS_ALT) == KS_ALT)		s |= ADD_ALT;
	if ((shift_status & KS_SHIFT) == KS_SHIFT)	s |= ADD_SHIFT;
	if ((shift_status & KS_CTRL) == KS_CTRL) {
	    s |= ADD_CONTROL;
	    if (key == 'o' || key == 'O') key = KE_CTL_(O);    /* CTRL+O */
	}
	if (!on_console) shift_status = 0;
    }

    return key | s;
}

tBool __FASTCALL__ __MsGetState(void)
{
    return mouse_status;
}

void __FASTCALL__ __MsSetState(tBool ms_visible)
{
    mouse_status = ms_visible;
}

void __FASTCALL__ __MsGetPos(tAbsCoord *x, tAbsCoord *y)
{
    *x = mouse.x;
    *y = mouse.y;
}

int __FASTCALL__ __MsGetBtns(void)
{
#ifdef	HAVE_MOUSE
    if (gpmhandle) ReadNextEvent();
#endif
    return mouse.pressed ? mouse.buttons : 0;
}

/*

*/

void __FASTCALL__ __init_keyboard(void)
{
    in_fd = open(ttyname(STDIN_FILENO), O_RDONLY);
    if (in_fd < 0) in_fd = STDIN_FILENO;

#define _MODE_ O_NONBLOCK | O_ASYNC

    if (fcntl(in_fd, F_SETFL, fcntl(in_fd, F_GETFL) | _MODE_) < 0)
    {
        printm("Can't set %X for %d: %s\nExiting..\n",
		_MODE_,
		in_fd,
		strerror(errno));
        exit(EXIT_FAILURE);
    }

    tcgetattr(in_fd, &tattr);
    sattr = tattr;
    tattr.c_lflag &= ~(ICANON | ECHO | ISIG);
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tattr.c_iflag &= ~(INPCK | ISTRIP | IXON | ICRNL);
    tattr.c_oflag |= OPOST | ONLCR;
    tcsetattr(in_fd, TCSANOW, &tattr);

    if (on_console && (ioctl(in_fd, KDSKBMODE, K_RAW) < 0)) {
	printm("Can't set keyboard raw mode: %s\nUsing VT100 emulation..\n", strerror(errno));
	on_console = 0;
    }

    if (on_console) {	/* init vt switching */
	struct vt_mode vt;

	signal(SIGUSR1, (void *)(int) console_leave);
	signal(SIGUSR2, (void *)(int) console_enter);
	ioctl(in_fd, VT_GETMODE, &vt);
	vt.mode = VT_PROCESS;
	vt.relsig = SIGUSR1;
	vt.acqsig = SIGUSR2;
	ioctl(in_fd, VT_SETMODE, &vt);
    }

#ifdef	HAVE_MOUSE
    {
	Gpm_Connect gc = { ~0, GPM_MOVE|GPM_HARD, 0, 0};
	gpmhandle = Gpm_Open(&gc, 0);
	if (gpmhandle < 0) gpmhandle = 0;
    }
#endif

    signal(SIGIO, (void *)(int) ReadNextEvent);
}

void __FASTCALL__ __term_keyboard(void)
{
    if (on_console) ioctl(in_fd, KDSKBMODE, K_XLATE);
    tcsetattr(in_fd, TCSANOW, &sattr);
    close(in_fd);
#ifdef	HAVE_MOUSE
    if (gpmhandle) Gpm_Close();
#endif
}
