/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/qnx/keyboard.c
 * @brief       This file contains implementation of keyboard handles for QNX4.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Andrew Golovnia
 * @since       2001
 * @note        Development, fixes and improvements
**/

#include <limits.h>
#include <conio.h>
#include <ioctl.h>
#include <sys/qnxterm.h>
#include <sys/dev.h>
#include <sys/qioctl.h>
#include <Ph.h>

#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

#define DEV_SCRLOCK     0x00000001
#define DEV_NUMLOCK     0x00000002
#define DEV_CAPSLOCK    0x00000004
#define DEV_SHIFT       0x00010000
#define DEV_CTRL        0x00020000
#define DEV_ALT         0x00040000

#define PHK_SHIFT       Pk_KM_Shift
#define PHK_CTRL        Pk_KM_Ctrl
#define PHK_ALT         Pk_KM_Alt
#define PHK_CAPSLOCK    Pk_KM_Caps_Lock
#define PHK_NUMLOCK     Pk_KM_Num_Lock
#define PHK_SCRLOCK     Pk_KM_Scroll_Lock

int _shift_state=0;
int _old_dev_state;
extern int _mouse_buttons;
extern int photon,console;
extern int ph_ig;

void __FASTCALL__ __init_keyboard( const char *user_cp )
{
	__init_mouse();
	_shift_state=0;
	__keypad(0,1);
	_old_dev_state=dev_mode(0,0,0);
	dev_mode(0,0,_DEV_MODES);
	return;
}

void __FASTCALL__ __term_keyboard( void )
{
	__term_mouse();
	dev_mode(0,_old_dev_state,_DEV_MODES);
	__keypad(0,0);
	return;
}
int __FASTCALL__ __kbdGetShiftsKey( void )
{
	long sbuf[2]={0,0};
	long rbuf;
	int ss=0;
	if(console)
	{
		if(qnx_ioctl(0,QCTL_DEV_CTL,sbuf,8,&rbuf,4)==-1)
			return _shift_state;
		if(rbuf&DEV_ALT) ss|=KS_ALT;
		if(rbuf&DEV_CTRL) ss|=KS_CTRL;
		if(rbuf&DEV_SHIFT) ss|=KS_SHIFT;
		if(rbuf&DEV_SCRLOCK) ss|=KS_SCRLOCK;
		if(rbuf&DEV_CAPSLOCK) ss|=KS_CAPSLOCK;
		if(rbuf&DEV_NUMLOCK) ss|=KS_NUMLOCK;
		return ss;
	}
	else if(photon&&ph_ig!=0)
		{
			static PhCursorInfo_t buf;

			PhQueryCursor(ph_ig,&buf);
			if(buf.key_mods&PHK_ALT) ss|=KS_ALT;
			if(buf.key_mods&PHK_CTRL) ss|=KS_CTRL;
			if(buf.key_mods&PHK_SHIFT) ss|=KS_SHIFT;
			if(buf.key_mods&PHK_SCRLOCK) ss|=KS_SCRLOCK;
			if(buf.key_mods&PHK_CAPSLOCK) ss|=KS_CAPSLOCK;
			if(buf.key_mods&PHK_NUMLOCK) ss|=KS_NUMLOCK;
			return ss;
		}
	return _shift_state;
}

static int __NEAR__ __FASTCALL__ isShiftKeysChange( int flush_queue )
{
	int ss;
	ss=__kbdGetShiftsKey();
	if(ss!=_shift_state)
	{
		_shift_state=ss;
		return 1;
	}
	return 0;
}

int __FASTCALL__ __kbdTestKey( unsigned long flg )
{
	int c;
	if((c=kbhit())==0)
	{
		__nodelay(0,1);
		c=term_key();
		__nodelay(0,0);
	}
	else c=term_key();
	if(c==-1) c=0;
	if(c&K_MOUSE_POS)
	{
		if((c&K_MOUSE_ACTION)==K_MOUSE_CLICK) return KE_MOUSE;
		if((c&K_MOUSE_ACTION)==K_MOUSE_RELEASE) _mouse_buttons=0;
		return 0;
		
	}
	if(c&K_CLASS||c==0)
	{
		if(isShiftKeysChange(0)) return KE_SHIFTKEYS;
		return 0;
	}
	switch(c)
	{
	case 0x01:
		_shift_state=KS_ALT;
		break;
	case 0x03:
		_shift_state=KS_CTRL;
		break;
	case 0x13:
		_shift_state=KS_SHIFT;
		break;
	default:
		if(!console)
			_shift_state=0;
		if(c>0xff) return c<<8;
		return c;
	}
	return KE_SHIFTKEYS;
}

int __FASTCALL__ __kbdGetKey ( unsigned long flg )
{
	int c;
	do
	{
		c=__kbdTestKey(flg);
		if(c==KE_MOUSE||c==KE_SHIFTKEYS) return c;
		if(c==0)
		{
			__OsYield();
			if(flg==KBD_NONSTOP_ON_MOUSE_PRESS&&_mouse_buttons)
				return KE_MOUSE;
		}
	}
	while(c==0);
	return c;
}

