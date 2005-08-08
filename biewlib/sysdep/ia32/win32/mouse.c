/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/win32/mouse.c
 * @brief       This file contains implementation of mouse handles for Win32s.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1999
 * @note        Development, fixes and improvements
 * @bug         __MsGetPos does not return precision coordinates of mouse
 * @todo        Attempt to precise mouse coordinates
**/
/* for cygwin - remove unnecessary includes */
#define _OLE_H
#define _OLE2_H
#include <windows.h>
#include <limits.h>
#include <string.h>

#include "biewlib/biewlib.h"

static tBool ms_visible = False;
tAbsCoord win32_mx = 0,win32_my = 0;
int win32_mbuttons = 0;
extern tBool hInputTrigger;
extern void __FASTCALL__ win32_readNextMessage( void );

int __FASTCALL__ __init_mouse( void )
{
  DWORD ret;
  SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),ENABLE_MOUSE_INPUT);
  if(GetNumberOfConsoleMouseButtons(&ret)== 0) ret = INT_MAX;
  return (int)ret; /* return number of mouse button */
}

void __FASTCALL__ __term_mouse( void )
{
  DWORD cmode;
  GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),&cmode);
  cmode &= ~ENABLE_MOUSE_INPUT;
  SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),ENABLE_MOUSE_INPUT);
}

tBool __FASTCALL__ __MsGetState( void )
{
  return ms_visible;
}

void __FASTCALL__ __MsSetState( tBool state )
{
  ms_visible = state;
}

void __FASTCALL__ __MsGetPos( tAbsCoord *mx, tAbsCoord *my )
{
  /*
     Under win32 mouse cursor can not be overwritten by video output.
     It not significant, that we do not have precision coordinate of
     mouse, intsead we have serious speed up of video ouput.
  */
  if(hInputTrigger) win32_readNextMessage();
  *mx = win32_mx;
  *my = win32_my;
}

int __FASTCALL__ __MsGetBtns( void )
{
  int ret;
  ret = 0;
 /* if(hInputTrigger)    REM: only kbdGetKey calls OsYield function */
  win32_readNextMessage();
  if(win32_mbuttons & FROM_LEFT_1ST_BUTTON_PRESSED) ret |= MS_LEFTPRESS;
  if(win32_mbuttons & RIGHTMOST_BUTTON_PRESSED) ret |= MS_RIGHTPRESS;
  if(win32_mbuttons & FROM_LEFT_2ND_BUTTON_PRESSED &
     !(win32_mbuttons & RIGHTMOST_BUTTON_PRESSED)) ret |= MS_MIDDLEPRESS;
  return ret;
}
