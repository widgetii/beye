/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/qnxnto/mouse.c
 * @brief       This file contains implementation of mouse handles for QNX6.
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
 * @note        Big thanks to Mike Gorchak for tvision source codes.
**/

#include <stdio.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/dcmd_input.h>
//#include <curses.h>
#include <term.h>
#undef buttons

#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

#define DEV_MOUSE	"/dev/devi/mouse0"
/**
 * @note        This mouse access method is based at QSSL mouse drivers.
 *              Mouse access is posible in console mode via character device
 *              named /dev/devi/mouse0. In QNX 6.1.0 - 6.2.1 this name is
 *              provided by devi-hirun photon input manager. Manager may be
 *              runed as a resource manager like this:
 *              # /usr/photon/bin/devi-hirun -Pr ps2 mousedev
 *              Situation may changes after QNX 6.3.0 release...
**/

int _mouse_fd=-1;
int _mouse_state=0;
int _mouse_buttons=0;
static int _mouse_x=0,_mouse_y=0;
static int _mouse_x_raw=0,_mouse_y_raw=0;
#define RAW_DX	8
#define RAW_DY	16

extern tAbsCoord tvioWidth,tvioHeight;
extern mono;
extern unsigned violen;
extern unsigned char *viomem;
extern tAbsCoord saveX,saveY;
extern int PCTable;

#define	_addr(x,y) (viomem+((x)+(y)*tvioWidth))

void __FASTCALL__ _putp(char *str);
void __FASTCALL__ __putp(char *str);
void __FASTCALL__ _putbuf(char **p,char *str);
void __FASTCALL__ _mapcolor(char **p,int col);
void __FASTCALL__ _insertchar(char **p,unsigned char c);

void __FASTCALL__ _mouse_hide(void);
void __FASTCALL__ _mouse_show(void);

int __FASTCALL__ __init_mouse(void)
{
	_mouse_fd=open(DEV_MOUSE,O_RDONLY);
	if(_mouse_fd==-1) return 0;
	_mouse_state=True;

	return 0;
}

void __FASTCALL__ __term_mouse(void)
{
	if(_mouse_fd!=-1) close(_mouse_fd);
}

tBool __FASTCALL__ __MsGetState(void)
{
	return _mouse_state;
}

void __FASTCALL__ __MsSetState(tBool is_visible)
{
	if(_mouse_fd!=-1)
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
	*mx=_mouse_x;
	*my=_mouse_y;
}

int __FASTCALL__ __MsGetBtns(void)
{
	register int m=_mouse_buttons,c;
	c=__kbdTestKey(0);
	if(m==_mouse_buttons) __OsYield();
	return _mouse_buttons;
}

void __FASTCALL__ _write_char(int c,int ca,int x,int y)
{
	char line[64];
	char *p;

	p=line;
	_putbuf(&p,tparm(cursor_address,y,x));
	p-=4;
	_putbuf(&p,"\x1B""[?7l");
	if(!mono)
	{
		_mapcolor(&p,ca);
	}
	else
	{
		_putbuf(&p,exit_attribute_mode);
		if(ca==0x0f) _putbuf(&p,enter_bold_mode);
		else if(ca==0x70) _putbuf(&p,enter_reverse_mode);
	}

	_insertchar(&p,c);
	*p=0;

	_putbuf(&p,"\x1B""[10m");
	PCTable=-1;
	_putbuf(&p,"\x1B""[?7h");
	_putbuf(&p,tparm(cursor_address,saveY,saveX));
	p-=4;
        if(mono) _putbuf(&p,exit_attribute_mode);
	*p=0;
	__putp(line);
}

void __FASTCALL__ _mouse_hide(void)
{
	int c,ca;
	char *addr;

	addr=_addr(_mouse_x,_mouse_y);
	c=addr[violen];
	ca=addr[0];
	_write_char(c,ca,_mouse_x,_mouse_y);
}
	
void __FASTCALL__ _mouse_show(void)
{
	int c,ca;
	char *addr;

	addr=_addr(_mouse_x,_mouse_y);
	c=addr[violen];
	ca=addr[0];
	_write_char(c,0xff^ca,_mouse_x,_mouse_y);
}

int __FASTCALL__ getms(void)
{
	fd_set fds;
	int ret;
	register int b;
	int n_mouse_x,n_mouse_y;
	struct timeval tv;
	struct _mouse_packet pkt;
#define PKT_SIZE	sizeof(struct _mouse_packet)

	if(_mouse_fd==-1) return 0;

	FD_ZERO(&fds);
	FD_SET(_mouse_fd,&fds);
	tv.tv_sec=0;
	tv.tv_usec=1;

	ret=select(_mouse_fd+1,&fds,NULL,NULL,&tv);
	if(ret==0||ret==-1) return 0;
	if(!FD_ISSET(_mouse_fd,&fds)) return 0;
	
	ret=read(_mouse_fd,&pkt,PKT_SIZE);
	if(ret!=PKT_SIZE) return 0;

	_mouse_x_raw+=pkt.dx;
	_mouse_y_raw-=pkt.dy;

	ret=0;
	if(_mouse_x_raw<0) _mouse_x_raw=0;
	if(_mouse_y_raw<0) _mouse_y_raw=0;
	n_mouse_x=_mouse_x_raw/RAW_DX;
	n_mouse_y=_mouse_y_raw/RAW_DY;
	if(n_mouse_x>=tvioWidth)
	{
		n_mouse_x=tvioWidth-1;
		_mouse_x_raw=n_mouse_x*RAW_DX;
	}
	if(n_mouse_y>=tvioHeight)
	{
		n_mouse_y=tvioHeight-1;
		_mouse_y_raw=n_mouse_y*RAW_DY;
	}

	b=0;
	if(pkt.hdr.buttons&_POINTER_BUTTON_LEFT)
		b|=MS_LEFTPRESS;
	if(pkt.hdr.buttons&_POINTER_BUTTON_RIGHT)
		b|=MS_RIGHTPRESS;
	if(pkt.hdr.buttons&_POINTER_BUTTON_MIDDLE)
		b|=MS_MIDDLEPRESS;

	if(_mouse_buttons!=b||_mouse_x!=n_mouse_x||_mouse_y!=n_mouse_y)
		ret=KE_MOUSE;

	_mouse_buttons=b;
	if(_mouse_state)
		_mouse_hide();
	_mouse_x=n_mouse_x;
	_mouse_y=n_mouse_y;
	if(_mouse_state)
		_mouse_show();

	return ret;
}
