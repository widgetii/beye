/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/qnxnto/vio.c
 * @brief       This file contains implementation of video subsystem handles for QNX6.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
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

#include <Ph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <curses.h>
#include <term.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <dlfcn.h>

#include "biewlib/biewlib.h"

#define _PSMIN 0xb0
#define _PSMAX 0xdf

#define	_addr(x,y) (viomem+((x)+(y)*tvioWidth))

int bit7=0,wrapped=0;
tAbsCoord tvioWidth,tvioHeight;
unsigned tvioNumColors=16;

static int initialized=0,cursor_status=__TVIO_CUR_NORM;
int photon,console;
tAbsCoord saveX,saveY;
static tAbsCoord firstX=0,firstY=0;
int mono=0;

FILE *tty_file;
int tty_fd;

unsigned char frames_dumb[0x30]=
": %|{+++++|+++++`++}-++++++++-+++++++++++++#%[]~";

unsigned violen;
unsigned char *viomem;
static struct termios old_term,new_term;		//!!!

void __FASTCALL__ _putp(char *str)
{
	if(str) fputs(str,tty_file);
}

void __FASTCALL__ __putp(char *str)
{
	_putp(str);
	fflush(tty_file);
}

void __FASTCALL__ _putbuf(char **p,char *str)
{
	if(str) strcpy(*p,str);
	(*p)+=strlen(str);
}

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
	char buf[40];
	char *p=buf;
	saveX=x;
	saveY=y;
	_putbuf(&p,tparm(cursor_address,y,x));
	p-=4;
	_putbuf(&p,"\x1B""[?7l");
	*p=0;
	__putp(buf);
}

void __FASTCALL__ __vioReadBuff(tAbsCoord x,tAbsCoord y,tvioBuff *buff,unsigned len)
{
	unsigned char *addr=_addr(x,y);

	memcpy(buff->attrs,addr,len);
	memcpy(buff->chars,addr+violen,len);
	memcpy(buff->oem_pg,addr+(violen<<1),len);
}

static int old_fore=-1,old_back=-1;

void __FASTCALL__ _mapcolor(char **p,int col)
{
	static const char map[]={0,4,2,6,1,5,3,7};
	int back,fore;
	back=(col>>4)&7;
        fore=col&15;
	if(fore!=old_fore&&back!=old_back)
		sprintf(*p,"\x1B[%d;%d;%dm",fore>7?1:22,
			30+map[fore & 7],40+map[back]);
	else
	{
		if(fore!=old_fore)
			sprintf(*p,"\033[%d;%dm",fore>7?1:22,30+map[fore&7]);
		else
			sprintf(*p,"\033[%dm",40+map[back]);
	}
	*p+=strlen(*p);
	old_fore=fore;
	old_back=back;
}

int PCTable=-1;

void __FASTCALL__ _insertchar(char **p,unsigned char c)
{
	if(c<0x80)
	{
		if(PCTable!=0)
		{
			PCTable=0;
			_putbuf(p,"\x1B""[11m");
		}
	}
	else
	{
		if(PCTable!=1)
		{
			PCTable=1;
			_putbuf(p,"\x1B""[12m");
		}
	}
	if(c==0x1B)
	{
		_putbuf(p,"\x1B""[10m");
		PCTable=-1;
	    	**p='-';
		(*p)++;
	}
	else
	{
		if(c==0) c=0x20;
		if(c==0x9B)
		{
			_putbuf(p,"\x1B""[10m");
			PCTable=-1;
			**p=0xA2;
			(*p)++;
		}
		else
		{
			**p=c;
			(*p)++;
		}
	}
}

static int prev_ca=-1;

void __FASTCALL__ __vioWriteBuff(tAbsCoord x,tAbsCoord y,const tvioBuff *buff,unsigned len)
{
	unsigned char *addr;
	unsigned char c,ca;
	unsigned i;
	char line[__TVIO_MAXSCREENWIDTH*16+16];
	char *p;

/*	if(!len) return;*/
	PCTable=-1;
	prev_ca=-1;
	addr=_addr(x,y);
	memcpy(addr,buff->attrs,len);
	memcpy(addr+violen,buff->chars,len);
	memcpy(addr+(violen<<1),buff->oem_pg,len);

	p=line;

	_putbuf(&p,tparm(cursor_address,y,x));
	p-=4;
	_putbuf(&p,"\x1B""[?7l");

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
		if(x>=tvioWidth)
		{
			y++;
			x-=tvioWidth;
			_putp(line);
			p=line;
			_putbuf(&p,tparm(cursor_address,y,x));
			p-=4;
			_putbuf(&p,"\x1B""[?7l");
		}
		if(prev_ca!=ca||prev_ca==-1)
		{
			prev_ca=ca;
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
		}

		_insertchar(&p,c);
		*p=0;
		x++;
	}

	_putbuf(&p,"\x1B""[10m");
	PCTable=-1;
	_putbuf(&p,"\x1B""[?7h");
	_putbuf(&p,tparm(cursor_address,saveY,saveX));
	p-=4;
        if(mono) _putbuf(&p,exit_attribute_mode);
	*p=0;
	__putp(line);
//	fwrite(line,1,p-line,tty_file);
//	fflush(tty_file);
}

static cc_t oldKeys[5];

void static SpecialKeysDisable(int file)
{
	struct termios term;
	tcgetattr(file,&term);
	oldKeys[0]=term.c_cc[VSUSP];
	oldKeys[1]=term.c_cc[VSTART];
	oldKeys[2]=term.c_cc[VSTOP];
	oldKeys[3]=term.c_cc[VQUIT];
	oldKeys[4]=term.c_cc[VINTR];
	term.c_cc[VSUSP] =0;
	term.c_cc[VSTART]=0;
	term.c_cc[VSTOP] =0;
	term.c_cc[VQUIT] =0;
	term.c_cc[VINTR] =0;
	tcsetattr(file,TCSANOW,&term);
}

void static SpecialKeysRestore(int file)
{
	struct termios term;
	tcgetattr(file,&term);
	term.c_cc[VSUSP] =oldKeys[0];
	term.c_cc[VSTART]=oldKeys[1];
	term.c_cc[VSTOP] =oldKeys[2];
	term.c_cc[VQUIT] =oldKeys[3];
	term.c_cc[VINTR] =oldKeys[4];
	tcsetattr(file,TCSANOW,&term);
}

void static SetGTables()
{
	printf("\x0F");              // Select GL to G0
	printf("\x1B""\x7E");        // Select GR to G1
	printf("\x1B""\x28""\x42");  // Select G0 to ASCII charset.
	printf("\x1B""\x29""\x3C");  // Select G1 to supplement charset.
	fflush(stdout);
}

void static RestoreGTables()
{
	printf("\x0F");              // Select GL to G0
	printf("\x1B""\x7E");        // Select GR to G1
	printf("\x1B""\x28""\x42");  // Select G0 to ASCII charset.
	printf("\x1B""\x29""\x30");  // Select G1 to special charset.
	fflush(stdout);
}

static void *so_handle;

int ph_ig;
struct _Ph_ctrl *(*p_PhAttach)(char const*,PhChannelParms_t const*);
#define so_PhAttach(a,b) (*p_PhAttach)(a,b)
int (*p_PhInputGroup)(PhEvent_t const*);
#define so_PhInputGroup(a) (*p_PhInputGroup)(a)
int (*p_PhQueryCursor)(unsigned short,PhCursorInfo_t*);
#define so_PhQueryCursor(a,b) (*p_PhQueryCursor)(a,b)

int init_libph_so(void)
{
	ph_ig=0;
	so_handle=dlopen("libph.so",RTLD_NOW);
	if(so_handle!=NULL)
	{
		p_PhAttach=dlsym(so_handle,"PhAttach");
		p_PhInputGroup=dlsym(so_handle,"PhInputGroup");
		p_PhQueryCursor=dlsym(so_handle,"PhQueryCursor");
		if(p_PhAttach!=NULL&&p_PhInputGroup!=NULL&&p_PhQueryCursor!=NULL)
			if(so_PhAttach(NULL,NULL))
			{
				ph_ig=so_PhInputGroup(NULL);
				return 1;
			}
	}
	return 0;
}

void done_libph_so(void)
{
	if(so_handle!=NULL) dlclose(so_handle);
	so_handle=NULL;
}

void __FASTCALL__ __init_vio(const char *user_cp,unsigned long flags)
{
	char *tty_name;
	char *terminal;
	struct winsize win;

	tcgetattr(fileno(stdout),&old_term);

	terminal=getenv("TERM");

	SpecialKeysDisable(fileno(stdin));

	ioctl(STDIN_FILENO,TIOCGWINSZ,&win);
	if(win.ws_col>0&&win.ws_row>0)
	{
		tvioWidth=win.ws_col;
		tvioHeight=win.ws_row;
	}
	else
	{
		tvioWidth=0;
		tvioHeight=0;
	}

	if(!isatty(fileno(stdout)))
	{
		printm("Do not redirect output!\n");
	        exit(-1);
        }
	tty_name=ttyname(fileno(stdout));
	if(!tty_name)
	{
		printm("Can't get the name of the current terminal!\n");
		exit(-1);
	}
        tty_file=fopen(tty_name,"w+b");
	if(!tty_file)
	{
		printm("Can't open the %s terminal!\n",tty_name);
	        exit(-1);
	}
        tty_fd=fileno(tty_file);
	if(!newterm(terminal,tty_file,stdin))
	{
		printm("Not connected to a terminal %s!\n",terminal);
		exit(-1);
	}

	initscr();
	stdscr->_flags|=_ISPAD;
	keypad(stdscr,TRUE);
	cbreak();
	noecho();
	nonl();
	if(has_colors()) start_color();
	else mono=1;
	refresh();
	timeout(0);

//	__putp(enter_pc_charset_mode);
	tcgetattr(tty_fd,&new_term);		//!!!
	ESCDELAY=1;

	if(tvioWidth==0) tvioWidth=COLS;
	if(tvioHeight==0) tvioHeight=LINES;
	saveX=firstX;
	saveY=firstY;
	violen=tvioWidth*tvioHeight;
	if((viomem=malloc(violen*3))==NULL)
	{
		printm("Can't allocate memory for output: %s\nExiting..",strerror(errno));
		exit(errno);
	}
	memset(viomem,0,violen*3);

	SetGTables();

	if(init_libph_so()) photon=1;
	else
	{
		photon=0;
		if(isatty(fileno(stdin))) console=1;
		else console=0;
	}

	/* 7-bit output */
	bit7=(flags&__TVIO_FLG_USE_7BIT); /* 7-bit flag set */

	/* wrapped output */
	wrapped=bit7;			/* 7-bit output flag set */

	initialized=1;
}

void __FASTCALL__ __term_vio(void)
{
	if(!initialized) return;
	done_libph_so();

	stdscr->_flags&=~_ISPAD;

	__putp(tparm(set_attributes,0,0,0,0,0,0,0,0,0));

	clear();
	refresh();
	resetterm();
	echo();
	endwin();

//	__vioSetCursorPos(firstX,firstY);
//	__vioSetCursorType(__TVIO_CUR_NORM);
	free(viomem);

	RestoreGTables();
	tcsetattr(fileno(stdout),TCSANOW,&old_term);	//!!!
	SpecialKeysRestore(fileno(stdin));
	fclose(tty_file);

	initialized=0;
}

void __FASTCALL__ __vioSetTransparentColor(unsigned char value)
{
}

