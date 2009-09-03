/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/dos/mouse.c
 * @brief       This file contains implementation of mouse handles for DOS.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
**/
#include <dos.h>
#include <limits.h>
#include <string.h>

#include "biewlib/biewlib.h"

static tBool ms_visible = False;
static tBool ms_inited = False;

int __FASTCALL__ __init_mouse( void )
{
  int ret;
  union REGS inreg,outreg;
  memset(&inreg,0,sizeof(inreg));
  inreg.x.ax = 0;
  int86(0x33,&inreg,&outreg);
  if(!outreg.x.ax) return INT_MAX;
  if(!outreg.x.bx || outreg.x.bx == -1) ret = 2;
  else                                  ret = outreg.x.bx;
  inreg.x.ax = 4;
  inreg.x.cx = tvioWidth / 2;
  inreg.x.dx = tvioHeight / 2;
  int86(0x33,&inreg,&outreg);
  ms_inited = True;
  return ret;
}

void __FASTCALL__ __term_mouse( void )
{
  if(ms_visible) __MsSetState(False);
}

tBool __FASTCALL__ __MsGetState( void )
{
  return ms_visible;
}

void __FASTCALL__ __MsSetState( tBool is_visible )
{
  union REGS inreg;
  if(ms_inited)
  {
    memset(&inreg,0,sizeof(inreg));
    inreg.x.ax = is_visible ? 1 : 2;
    int86(0x33,&inreg,&inreg);
    ms_visible = is_visible;
  }
}

#define PIXEL_PER_CHAR 8

void __FASTCALL__ __MsGetPos( tAbsCoord *mx, tAbsCoord *my )
{
  union REGS inreg,outreg;
  *mx = *my = 0;
  if(ms_inited)
  {
    memset(&inreg,0,sizeof(inreg));
    inreg.x.ax = 3;
    int86(0x33,&inreg,&outreg);
    *mx = outreg.x.cx / PIXEL_PER_CHAR;
    *my = outreg.x.dx / PIXEL_PER_CHAR;
  }
}

int __FASTCALL__ __MsGetBtns( void )
{
  union REGS inreg,outreg;
  if(ms_inited)
  {
    memset(&inreg,0,sizeof(inreg));
    inreg.x.ax = 3;
    int86(0x33,&inreg,&outreg);
    return outreg.x.bx;
  }
  else return 0;
}
