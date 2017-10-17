/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/qnxnto/keyboard.c
 * @brief       This file contains implementation of keyboard handles for QNX6.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Andrew Golovnia
 * @since       2003
 * @note        Development, fixes and improvements
 * @note        Big thanks to Mike Gorchak for tvision-1.0.10-1/src.
**/

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <devctl.h>
#include <sys/dcmd_chr.h>
#include <Ph.h>

#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

#define DEV_SCRLOCK     _LINESTATUS_CON_SCROLL
#define DEV_NUMLOCK     _LINESTATUS_CON_NUM
#define DEV_CAPSLOCK    _LINESTATUS_CON_CAPS
#define DEV_SHIFT       _LINESTATUS_CON_SHIFT
#define DEV_CTRL        _LINESTATUS_CON_CTRL
#define DEV_ALT         _LINESTATUS_CON_ALT

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
extern struct _Ph_ctrl *(*p_PhAttach)(char const*,PhChannelParms_t const*);
#define so_PhAttach(a,b) (*p_PhAttach)(a,b)
extern int (*p_PhInputGroup)(PhEvent_t const*);
#define so_PhInputGroup(a) (*p_PhInputGroup)(a)
extern int (*p_PhQueryCursor)(unsigned short,PhCursorInfo_t*);
#define so_PhQueryCursor(a,b) (*p_PhQueryCursor)(a,b)

int __FASTCALL__ getms(void);

void __FASTCALL__ __init_keyboard( const char *user_cp )
{
	__init_mouse();
	_shift_state=0;
	raw();
	intrflush(stdscr,FALSE);
	meta(stdscr,TRUE);
	keypad(stdscr,TRUE);
	nodelay(stdscr,TRUE);
	return;
}

void __FASTCALL__ __term_keyboard( void )
{
	__term_mouse();
	keypad(stdscr,FALSE);
	nodelay(stdscr,FALSE);
}

int __FASTCALL__ __kbdGetShiftsKey( void )
{
        long dbuf;
	long rbuf;
	int ss=0;
	if(console)
	{
	        if(devctl(fileno(stdin),DCMD_CHR_LINESTATUS,&rbuf,sizeof(long),&dbuf))
		        return _shift_state;
		if(rbuf&DEV_ALT) ss|=KS_ALT;
		if(rbuf&DEV_CTRL) ss|=KS_CTRL;
		if(rbuf&DEV_SHIFT) ss|=KS_SHIFT;
		if(rbuf&DEV_SCRLOCK) ss|=KS_SCRLOCK;
		if(rbuf&DEV_CAPSLOCK) ss|=KS_CAPSLOCK;
		if(rbuf&DEV_NUMLOCK) ss|=KS_NUMLOCK;
		return ss;
	}
	else if(photon/*&&ph_ig!=0*/)
		{
			static PhCursorInfo_t buf;
			so_PhQueryCursor(ph_ig,&buf);
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

	c=getch();
/*	if(c!=-1)
	{
		FILE *f=fopen("___kb.out","a+");
		fprintf(f,"0x%08x - 0x%08x\n",c,_2B(c));
		fclose(f);
	}*/
	if(c==ERR)
	{
		c=getms();
		if(c==KE_MOUSE) return c;
		c=0;
	}

	if(isShiftKeysChange(0))
	{
		if(c!=0) ungetch(c);
		return KE_SHIFTKEYS;
	}
	if((!console)&&(!photon)||(photon&&ph_ig==0))
		switch(c)
		{
		case KE_STATUS_ALT:
			_shift_state=KS_ALT;
			return KE_SHIFTKEYS;
		case KE_STATUS_CONTROL:
			_shift_state=KS_CTRL;
			return KE_SHIFTKEYS;
		case KE_STATUS_SHIFT:
			_shift_state=KS_SHIFT;
			return KE_SHIFTKEYS;
		case KE_STATUS_RESET:
			_shift_state=0;
			return KE_SHIFTKEYS;
		}
	switch(c)
	{
	case KE_ENTER2:
		c=KE_ENTER;
		break;
	case KE_BKSPACE1:
	case KE_BKSPACE2:
		c=KE_BKSPACE;
		break;
	default:
		if(c>0xff) c=_2B(c);
		break;
	}
	return c;
}

int __FASTCALL__ __kbdGetKey ( unsigned long flg )
{
	int c;
	do
	{
		c=__kbdTestKey(flg);
		if(c==KE_SHIFTKEYS) return c;
		if(c==0)
		{
			__OsYield();
			if(flg==KBD_NONSTOP_ON_MOUSE_PRESS&&_mouse_buttons)
				return KE_MOUSE;
		}
	}
	while(c==0);
	if(_shift_state&KS_CTRL) c|=ADD_CONTROL;
	if(_shift_state&KS_ALT) c|=ADD_ALT;
	if(_shift_state&KS_SHIFT) c|=ADD_SHIFT;
	return c;
}
