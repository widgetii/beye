/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/qnx/mouse.c
 * @brief       This file contains implementation of mouse handles for QNX4.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Andrew Golovnia
 * @since       2001
 * @note        Development, fixes and improvements
**/
#ifdef __QNX4__

#include <limits.h>
#include <sys/qnxterm.h>

#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

#define MFL TERM_MOUSE_FOLLOW|TERM_MOUSE_ADJUST|TERM_MOUSE_HELD|\
			TERM_MOUSE_MOVED|TERM_MOUSE_MENU|TERM_MOUSE_SELECT|\
			TERM_MOUSE_RELEASE

#define MOUSEPTR(ch) ((0x7000-(ch&0x7000))|(0x700-(ch&0x700))|TERM_HILIGHT)

int _mouse_state=0;
int _mouse_buttons=0;
extern int photon,console;
extern int bit7;
extern tAbsCoord tvioWidth,tvioHeight;
extern tAbsCoord saveX,saveY;
extern unsigned char frames_dumb[0x30];
extern unsigned violen;
extern unsigned char *viomem;
extern unsigned long biew_kbdFlags;

#define _PSMIN 0xb0
#define _PSMAX 0xdf

#define	_addr(x,y) (viomem+((x)+(y)*tvioWidth))

int _mouse_handler(unsigned*,struct mouse_event*);

int __FASTCALL__ __init_mouse(void)
{
	if(!(console||photon)) return 0;
	term_mouse_on();
	term_mouse_flags(MFL,MFL);
	term_mouse_handler(&_mouse_handler);
	_mouse_state=True;
	if(!photon)
		_mouse_show();
	term_state.mouse_cursor=0;
	return 0;
}

void __FASTCALL__ __term_mouse(void)
{
	if(!photon)
		_mouse_hide();
	term_mouse_off();
	return;
}

void _mouse_hide(void)
{
	int c,ca,ch;
	char *addr=_addr(term_state.mouse_col,term_state.mouse_row);
	c=*(addr+violen);
	if(bit7)
	{
		if(c>=_PSMIN&&c<=_PSMAX)
			c=frames_dumb[c-_PSMIN];
		if(c>=0&&c<=0x1f)
			c=0x20;
	}
	ca=*addr;
	ch=((ca&0x77)<<8)|((ca&0x08)>>2)|((ca&0x80)>>7)|TERM_BLACK;
	term_type(term_state.mouse_row,term_state.mouse_col,&c,1,ch);
	term_cur(saveY,saveX);
}
	
void _mouse_show(void)
{
	int c,ca,ch;
	char *addr=_addr(term_state.mouse_col,term_state.mouse_row);
	c=*(addr+violen);
	if(bit7)
	{
		if(c>=_PSMIN&&c<=_PSMAX)
			c=frames_dumb[c-_PSMIN];
		if(c>=0&&c<=0x1f)
			c=0x20;
	}
	ca=*addr;
	ch=((0x77-(ca&0x77))<<8)|((ca&0x08)>>2)|((ca&0x80)>>7)|TERM_BLACK;
	term_type(term_state.mouse_row,term_state.mouse_col,&c,1,ch);
	term_cur(saveY,saveX);
}

int _mouse_handler(unsigned *key,struct mouse_event *me)
{
	register b=0;
	if(key)
	{
		if(*key&K_MOUSE_BLEFT)
			b|=MS_LEFTPRESS;
		if(*key&K_MOUSE_BRIGHT)
			b|=MS_RIGHTPRESS;
		if(*key&K_MOUSE_BMIDDLE)
			b|=MS_MIDDLEPRESS;
		_mouse_buttons=b;
	}
	if(!photon)
	{
		if(_mouse_state)
			_mouse_hide();	
		term_mouse_process(key,me);
		if(_mouse_state)
			_mouse_show();
	}
	else
		term_mouse_process(key,me);
	return 0;
}

tBool __FASTCALL__ __MsGetState(void)
{
	return _mouse_state;
}

void __FASTCALL__ __MsSetState(tBool is_visible)
{
	if(!photon)
	{
		if(is_visible==False&&_mouse_state==True)
			_mouse_hide();
		if(_mouse_state==False&&is_visible==True)
			_mouse_show();
	}
	_mouse_state=is_visible;
}

void __FASTCALL__ __MsGetPos(tAbsCoord *mx, tAbsCoord *my)
{
	*mx=term_state.mouse_col;
	*my=term_state.mouse_row;
}

int __FASTCALL__ __MsGetBtns(void)
{
	register int m=_mouse_buttons,c;
	c=__kbdTestKey(0);
	if(m==_mouse_buttons) __OsYield();
	return _mouse_buttons;
}

#else
#include "biewlib/sysdep/generic/unix/mouse.c"
#endif
