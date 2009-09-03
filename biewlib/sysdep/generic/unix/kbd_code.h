/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/generic/unix/kbd_code.h
 * @brief       This file contains definitions of keyboard codes
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
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

#ifndef __KBD_CODE_H
#define __KBD_CODE_H

#define KS_SHIFT	3
#define KS_CTRL		4
#define KS_ALT		8
#define KS_CTRL_MASK	15
#define KS_SCRLOCK	16
#define KS_NUMLOCK	32
#define KS_CAPSLOCK	64
#define KS_INSERT	128

/*
    not real keys, for internal use only
*/

#define KE_SUPERKEY	0xFFFF
#define KE_JUSTFIND	0xFFFE
#define KE_MOUSE        0xFFFB
#define KE_SHIFTKEYS    0xFFFA
#define KE_BREAK	0xFFF0

/*
    status control keys
*/

#define KE_STATUS_ALT		'\x01'	/**< CTRL+A */
#define KE_STATUS_CONTROL	'\x03'	/**< CTRL+C */
#define KE_STATUS_SHIFT		'\x13'	/**< CTRL+S */
#define KE_STATUS_RESET		'\x1a'	/**< CTRL+Z */
#define KE_C_O			'\x0f'  /**< CTRL+O */

/*

*/

#define KE_ESCAPE	'\x1b'
#define KE_SPACE	' '
#define KE_TAB		'\t'
#define KE_ENTER	'\n'
#define KE_ENTER2	'\r'
#define KE_BKSPACE1	'\x08'
#define KE_BKSPACE2	'\x7f'

/*
    All non-symbol keys (function keys, arrows, etc) must be xx00
    because of biew dos-dependent key treatment (will be fixed)
*/

#ifdef	_SLANG_
#include <slang.h>
#define _2B(x)	((((x) & 0xff) << 8) + (((x) & 0xf00) << 4) * 4)
/* #define KE_ENTER	_2B(SL_KEY_ENTER) */
#define KE_BKSPACE	_2B(SL_KEY_BACKSPACE)
#define KE_DOWNARROW	_2B(SL_KEY_DOWN)
#define KE_UPARROW	_2B(SL_KEY_UP)
#define KE_LEFTARROW	_2B(SL_KEY_LEFT)
#define KE_RIGHTARROW	_2B(SL_KEY_RIGHT)
#define KE_HOME		_2B(SL_KEY_HOME)
#define KE_END		_2B(SL_KEY_END)
#define KE_PGUP		_2B(SL_KEY_PPAGE)
#define KE_PGDN		_2B(SL_KEY_NPAGE)
#define KE_INS		_2B(SL_KEY_IC)
#define KE_DEL		_2B(SL_KEY_DELETE)
#define KE_F(x)		_2B(SL_KEY_F(x))
#endif


#ifdef	_CURSES_
#include <curses.h>
#define _2B(x)	(((x) & 0xff) << 8)
/* #define KE_ENTER	_2B(KEY_ENTER) */
#define KE_BKSPACE	_2B(KEY_BACKSPACE)
#define KE_DOWNARROW	_2B(KEY_DOWN)
#define KE_UPARROW	_2B(KEY_UP)
#define KE_LEFTARROW	_2B(KEY_LEFT)
#define KE_RIGHTARROW	_2B(KEY_RIGHT)
#define KE_HOME		_2B(KEY_HOME)
#define KE_END		_2B(KEY_END)
#define KE_PGUP		_2B(KEY_PPAGE)
#define KE_PGDN		_2B(KEY_NPAGE)
#define KE_INS		_2B(KEY_IC)
#define KE_DEL		_2B(KEY_DC)
#define KE_F(x)		_2B(KEY_F(x))
#endif


/*
    these are not real keyboard codes on vt100,
    GetKey() translates escape sequence to these values
*/

#ifdef	_VT100_
#define KE_BKSPACE	'\x08'
#define KE_DOWNARROW	0x5000
#define KE_UPARROW	0x4800
#define KE_LEFTARROW	0x4b00
#define KE_RIGHTARROW	0x4d00
#define KE_HOME		0x4700
#define KE_END		0x4f00
#define KE_PGUP		0x4900
#define KE_PGDN		0x5100
#define KE_INS		0x5200
#define KE_DEL		0x5300
#define KE_F(x)		((0x3a + (x)) << 8)
#endif

/*
    do not edit below
*/

#define ADD_CONTROL	0x010000
#define ADD_ALT		0x020000
#define ADD_SHIFT	0x040000

#define KE_CTL_LEFTARROW	(KE_LEFTARROW | ADD_CONTROL)
#define KE_CTL_RIGHTARROW	(KE_RIGHTARROW | ADD_CONTROL)

#define KE_SHIFT_TAB	(KE_TAB | ADD_CONTROL)
#define KE_CTL_BKSPACE	(KE_BKSPACE | ADD_CONTROL)
#define KE_CTL_ENTER	(KE_ENTER | ADD_CONTROL)
#define KE_CTL_BREAK	(KE_BREAK | ADD_CONTROL)

#define KE_CTL_PGDN	(KE_PGDN | ADD_CONTROL)
#define KE_CTL_PGUP	(KE_PGUP | ADD_CONTROL)
#define KE_CTL_HOME	(KE_HOME | ADD_CONTROL)
#define KE_CTL_END	(KE_END | ADD_CONTROL)

#define KE_SHIFT_F(x)	(KE_F(x) | ADD_SHIFT)
#define KE_CTL_F(x)	(KE_F(x) | ADD_CONTROL)
#define KE_ALT_F(x)	(KE_F(x) | ADD_ALT)

/*
#define KE_ALT_(x)	( (x) | ADD_ALT)
#define KE_CTL_(x)	( (x) | ADD_CONTROL)
*/

#define SCAN_A  0x1e
#define SCAN_B  0x30
#define SCAN_C  0x2e
#define SCAN_D  0x20
#define SCAN_E  0x12
#define SCAN_F  0x21
#define SCAN_G  0x22
#define SCAN_H  0x23
#define SCAN_I  0x17
#define SCAN_J  0x24
#define SCAN_K  0x25
#define SCAN_L  0x26
#define SCAN_M  0x31
#define SCAN_N  0x32
#define SCAN_O  0x18
#define SCAN_P  0x19
#define SCAN_Q  0x10
#define SCAN_R  0x13
#define SCAN_S  0x1f
#define SCAN_T  0x14
#define SCAN_U  0x16
#define SCAN_V  0x2f
#define SCAN_W  0x11
#define SCAN_X  0x2d
#define SCAN_Y  0x15
#define SCAN_Z  0x2c

#define ENUM_A  0x01
#define ENUM_B  0x02
#define ENUM_C  0x03
#define ENUM_D  0x04
#define ENUM_E  0x05
#define ENUM_F  0x06
#define ENUM_G  0x07
#define ENUM_H  0x08
#define ENUM_I  0x09
#define ENUM_J  0x0a
#define ENUM_K  0x0b
#define ENUM_L  0x0c
#define ENUM_M  0x0d
#define ENUM_N  0x0e
#define ENUM_O  0x0f
#define ENUM_P  0x10
#define ENUM_Q  0x11
#define ENUM_R  0x12
#define ENUM_S  0x13
#define ENUM_T  0x14
#define ENUM_U  0x15
#define ENUM_V  0x16
#define ENUM_W  0x17
#define ENUM_X  0x18
#define ENUM_Y  0x19
#define ENUM_Z  0x1a

#define __SCAN_(x) (SCAN_##x)
#define __ENUM_(x) (ENUM_##x)

#define KE_CTL_(x) ((((__SCAN_(x) << 8) & 0xFF00) | ((__ENUM_(x)-__ENUM_(A)+1) & 0x00FF)) | ADD_CONTROL)
#define KE_ALT_(x) ((((__SCAN_(x) << 8) & 0xFF00) | ADD_ALT)

#endif	/* __KBD_CODE_H */
