/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/qnx/vio.c
 * @brief       This file contains implementation of video subsystem handles for QNX4.
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

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ioctl.h>
#include <sys/qnxterm.h>
#include <sys/term.h>
#include <sys/dev.h>
#include <Ph.h>

#include "biewlib/biewlib.h"

#define _PSMIN 0xb0
#define _PSMAX 0xdf

#define	_addr(x,y) (viomem+((x)+(y)*tvioWidth))

int bit7=0,wrapped=0;
tAbsCoord tvioWidth=80,tvioHeight=25;
unsigned tvioNumColors=16;

static int initialized=0,cursor_status=__TVIO_CUR_NORM;
int photon,console;
int ph_ig;
tAbsCoord saveX,saveY;
static tAbsCoord firstX=0,firstY=0;

unsigned char frames_dumb[0x30]=
": %|{+++++|+++++`++}-++++++++-+++++++++++++#%[]~";

unsigned violen;
unsigned char *viomem;

int terminal;
char *terms[]={
/*0*/	"qnx",
/*1*/	"qansi",
/*2*/	"ansi",
/*-1*/	NULL
};

int __FASTCALL__ __vioGetCursorType(void)
{
	return cursor_status;
}

void __FASTCALL__ __vioSetCursorType(int type)
{
	cursor_status=type;
	if(type==__TVIO_CUR_OFF)
		__putp(cursor_invisible);
	else
	{
		__putp(cursor_visible);
		if(type==__TVIO_CUR_NORM)
			__putp(cursor_normal);
	}
}

void __FASTCALL__ __vioGetCursorPos(tAbsCoord *x,tAbsCoord *y)
{
	*x=saveX;
	*y=saveY;
}

void __FASTCALL__ __vioSetCursorPos(tAbsCoord x,tAbsCoord y)
{
	saveX=x;
	saveY=y;
	term_cur(y,x);
}

void __FASTCALL__ __vioReadBuff(tAbsCoord x,tAbsCoord y,tvioBuff *buff,unsigned len)
{
	unsigned char *addr=_addr(x,y);

	memcpy(buff->attrs,addr,len);
	memcpy(buff->chars,addr+violen,len);
	memcpy(buff->oem_pg,addr+(violen<<1),len);
}

void __FASTCALL__ __vioWriteBuff(tAbsCoord x,tAbsCoord y,const tvioBuff *buff,unsigned len)
{
	unsigned char *addr;
	unsigned char c,ca;
	unsigned i,ch=0;

/*	if(!len) return;*/
	addr=_addr(x,y);
	memcpy(addr,buff->attrs,len);
	memcpy(addr+violen,buff->chars,len);
	memcpy(addr+(violen<<1),buff->oem_pg,len);
	for(i=0;i<len;i++)
	{
		c=buff->chars[i];
		if(bit7||wrapped)
		{
			if(c>=_PSMIN&&c<=_PSMAX) c=frames_dumb[c-_PSMIN];
			else
				if(c<=0x1f) c=0x20;
				else
					if(bit7&&c>=0x80) c='.';
		}
		ca=buff->attrs[i];
		ch=((ca&0x77)<<8)|((ca&0x08)>>2)|((ca&0x80)>>7)|TERM_BLACK;
		if(x>=tvioWidth)
		{
			y++;
			x-=tvioWidth;
		}
		term_type(y,x,&c,1,ch);
		x++;
	}
	term_cur(saveY,saveX);
	term_flush();
}

void __FASTCALL__ __init_vio(const char *user_cp,unsigned long flags)
{
	struct _dev_info_entry di;
	char *term_name;
	int i;

	term_video_on();
	if(term_state.is_mono) tvioNumColors=2;
	tvioWidth=term_state.num_cols;
	tvioHeight=term_state.num_rows;
	saveX=firstX;
	saveY=firstY;
	violen=tvioWidth*tvioHeight;
	if((viomem=malloc(violen*3))==NULL)
	{
		printm("Can't allocate memory for output: %s\nExiting..",strerror(errno));
		exit(errno);
	}
	memset(viomem,0,violen*3);
	dev_info(fileno(stdin),&di);
	if(strcmp(di.driver_type,"console")==0) console=1;
	else console=0;

	if(PhAttach(0,0)) photon=1;
	else photon=0;

	terminal=-1;
	if((term_name=getenv("TERM"))!=NULL)
		for(i=0;terms[i]!=NULL;i++)
			if(strcmp(term_name,terms[i])==0)
			{
				terminal=i;
				break;
			}

	/* 7-bit output */
	bit7=
		(flags&__TVIO_FLG_USE_7BIT)| /* 7-bit flag set */
		(terminal==-1)|	/* unknown terminal */
		(terminal==2)|	/* terminal=ansi */
		(photon&&terminal==1);	/* Photon MicroGUI & terminal=qansi */

	/* wrapped output */
	wrapped=
		bit7|			/* 7-bit output flag set */
		photon;			/* run in Photon MicroGUI */

	if(photon)
	{
		char *var;
		ph_ig=0;
		var=getenv("PHIG");
		if(var==NULL) ph_ig=1;
		else
			if(sscanf(var,"%d",&ph_ig)!=1||ph_ig==0) ph_ig=1;
	}

	initialized=1;
}

void __FASTCALL__ __term_vio(void)
{
	if(!initialized) return;
	__vioSetCursorPos(firstX,firstY);
	__vioSetCursorType(__TVIO_CUR_NORM);
	term_video_off();
	free(viomem);
	initialized=0;
}

void __FASTCALL__ __vioSetTransparentColor(unsigned char value)
{
}

