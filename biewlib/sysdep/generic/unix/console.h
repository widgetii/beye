/** 
 * @namespace   biewlib
 * @file        biewlib/sysdep/generic/unix/console.h
 * @brief       unix console internals
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

#ifndef	__CONSOLE_H
#define	__CONSOLE_H

/*
    sanity checks
*/

#ifdef	_SLANG_
#undef	_CURSES_
#undef	_VT100_
#endif

#ifdef	_CURSES_
#undef	_SLANG_
#undef	_VT100_
#endif

#ifdef	_VT100_
#undef	_SLANG_
#undef	_CURSES_
#endif

/*
    can we use SIGIO?
*/

#if defined (_VT100_) && !defined(__BEOS__)
#define __ENABLE_SIGIO
#endif

/*
    directly supported terminals
*/

#define	TERM_UNKNOWN	0
#define	TERM_LINUX	1
#define	TERM_XTERM	2
#define	TERM_VT100	3
#define	TERM_ANSI	4

/*
    pseudographics map:

    T	top
    B	bottom
    L	left
    R	right
    S	single
    D	double
    V	vertical
    H	horizontal
*/

#define BAR0	0xb0
#define BAR1	0xb1
#define BAR2	0xb2
#define VSLINE	0xb3
#define RSSTEE	0xb4
#define RSDTEE	0xb5
#define	RDSTEE	0xb6
#define TRSDCR	0xb7
#define TRDSCR	0xb8
#define	RDDTEE	0xb9
#define VDLINE	0xba
#define TRDDCR	0xbb
#define	LRDDCR	0xbc
#define	LRSDCR	0xbd
#define	LRDSCR	0xbe
#define	TRSSCR	0xbf
#define LLSSCR	0xc0
#define	BSSTEE	0xc1
#define TSSTEE	0xc2
#define	LSSTEE	0xc3
#define	HSLINE	0xc4
#define	SSPLUS	0xc5
#define	LSDTEE	0xc6
#define	LDSTEE	0xc7
#define	LLDDCR	0xc8
#define	TLDDCR	0xc9
#define	BDDTEE	0xca
#define	TDDTEE	0xcb
#define	LDDTEE	0xcc
#define	HDLINE	0xcd
#define	DDPLUS	0xce
#define	BDSTEE	0xcf
#define	BSDTEE	0xd0
#define	TDSTEE	0xd1
#define	TSDTEE	0xd2
#define	BLDSCR	0xd3
#define	BLSDCR	0xd4
#define	TLSDCR	0xd5
#define	TLDSCR	0xd6
#define	SDPLUS	0xd7
#define	DSPLUS	0xd8
#define	BRSSCR	0xd9
#define	TLSSCR	0xda
#define	DBLK	0xdb
#define	BBLK	0xdc
#define	LBLK	0xdd
#define	RBLK	0xde
#define	TBLK	0xdf

#define _PSMIN	0xb0
#define	_PSMAX	0xdf

extern int on_console, terminal, transparent, do_nls;
extern tBool break_status;

#define TESTFLAG(x,y) (((x) & y) == y)

typedef struct {
    int x;
    int y;
    int buttons;
    int pressed;
} mevent;

/*
    console plugin
*/

/*
typedef struct {
	unsigned long flags;
	unsigned width;
	unsigned height;
	unsigned colors;
	unsigned type;
	unsigned reserved[3];

	void (*initialize)(unsigned long);
	void (*terminate)(void);
	void (*update)(void);

	void (*ReadBuf)(int, int, void *, unsigned);
	void (*WriteBuf)(int, int, void *, unsigned);

	int (*GetCursorType)(void);
	void (*SetCursorType)(int);
	void (*GetCursorPos)(int *, int *);
	void (*SetCursorPos)(int, int);

	int (*TestKey)(void);
	int (*GetKey)(void);
	int (*GetMouse)(int *, int *, int *);

} Console;

extern Console console;

#define	__init_vio		console.initialize
#define	__term_vio		console.terminate

#define	__init_keyboard()
#define	__term_keyboard()

#define __vioRereadState	console.update
#define	__vioReadBuff		console.ReadBuf
#define	__vioWriteBuff		console.WriteBuf
#define	__vioGetCursorPos	console.GetCursorPos
#define	__vioSetCursorPos	console.SetCursorPos
#define	__vioGetCursorType	console.GetCursorType
#define	__vioSetCursorType	console.SetCursorType
*/

#endif	/* __CONSOLE_H */
