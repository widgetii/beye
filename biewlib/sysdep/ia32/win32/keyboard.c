/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/win32/keyboard.c
 * @brief       This file contains implementation of keyboard handles for Win32s.
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
 * @author      Alexander Lokhan'ko <alex@eunet.lt>
 * @date        28.03.2000
 * @note        fixing code for workaround of console input bug under Win9x
 *              tested on Win95 PE, OSR2, Win98
 * @author      Alexander Lokhan'ko <alex@eunet.lt>
 * @date        04.04.2000
 * @note        removed slight delay in win9x console bug workaround code
 * @warning     May not work propertly under some Win32 releases
 * @author      Sergey Oblomov <hoopoepg@mail.ru>
 * @date        28.05.2003
 * @note        reworked console's reader to enable clipboard pasting
 *              through SysMenu->Edit->Paste facility of window.
 * @warning     May not work propertly under some Win32 releases
 * @bug         Under WinNT does not return correct values for some
 *              combinations of keys (like CtrlBkSpace)
 * @note        added mouse wheel support for Win2k+ (emit Up/Down key event)
 * @warning     ONE WHEEL SUPPORTED ONLY :-/
 * @bug         Some wheel processing freezing found
 * @author      Andrew Golovnia
 * @date        19.12.2003
**/
/* for cygwin - remove unnecessary includes */
#define _OLE_H
#define _OLE2_H
#include <windows.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "biewlib/kbd_code.h"
#include "biewlib/biewlib.h"

static int KB_Buff[64];
static unsigned char KB_freq = 0;
static int shiftkeys = 0;

#ifndef MOUSE_WHEELED
#define MOUSE_WHEELED	4 /*Windows NT and Windows Me/98/95:  This value is not supported.*/
#endif

HANDLE hIn;
tBool hInputTrigger = False;
extern OSVERSIONINFO win32_verinfo;
static int is_win9x;
extern void __FASTCALL__ win32_readNextMessage( void );

static int __mou_nbtns;

static int __FASTCALL__ is_VKCtrl(int code)
{
  return code == VK_MENU ||
         code == VK_CONTROL ||
         code == VK_SHIFT;
}

extern tAbsCoord win32_mx,win32_my;
extern int win32_mbuttons;

void __FASTCALL__ win32_readNextMessage( void )
{
  //DWORD total_nread,i;
  int vkeycode,keycode;
  INPUT_RECORD ir;
  DWORD nread;
  hInputTrigger=False;

  PeekConsoleInput( hIn, &ir, 1, &nread );
  if( nread )
  {
    ReadConsoleInput(hIn,&ir,1,&nread);
    //if(!nread) break; /* sometimes happen */
    switch(ir.EventType)
    {
      case MOUSE_EVENT:
      {
        static int buttons = 0;
        win32_mbuttons = ir.Event.MouseEvent.dwButtonState & 0xffff;
        if( buttons != win32_mbuttons )
        {
    	    win32_mx = ir.Event.MouseEvent.dwMousePosition.X;
            win32_my = ir.Event.MouseEvent.dwMousePosition.Y;
            if(KB_freq < sizeof(KB_Buff)/sizeof(int)) KB_Buff[KB_freq++] = KE_MOUSE;
            buttons = win32_mbuttons & 0xffff;
        }
        if( ir.Event.MouseEvent.dwEventFlags & MOUSE_WHEELED )
        {
            int wheel = ( (int)ir.Event.MouseEvent.dwButtonState ) >> 16;
            if( wheel < 0 ) KB_Buff[KB_freq++] = KE_DOWNARROW;
            if( wheel > 0 ) KB_Buff[KB_freq++] = KE_UPARROW;
        }
      }
      return;
      case KEY_EVENT:
      {
        shiftkeys = ir.Event.KeyEvent.dwControlKeyState;
        vkeycode = ir.Event.KeyEvent.wVirtualKeyCode;

        if( ir.Event.KeyEvent.wVirtualKeyCode == 27 && ir.Event.KeyEvent.wVirtualScanCode == 1 &&
            !ir.Event.KeyEvent.uChar.AsciiChar )
            ir.Event.KeyEvent.uChar.AsciiChar = ir.Event.KeyEvent.wVirtualKeyCode;

        if(is_VKCtrl(vkeycode))
        {
          if(KB_freq < sizeof(KB_Buff)/sizeof(int)) KB_Buff[KB_freq++] = KE_SHIFTKEYS;
        }
        else
        {
            if( vkeycode < ' ' )
                vkeycode = 0;
            keycode = ((ir.Event.KeyEvent.wVirtualScanCode << 8) & 0xFF00) |
                      (ir.Event.KeyEvent.uChar.AsciiChar & 0xFF);
            if(shiftkeys & SHIFT_PRESSED) keycode |= ADD_SHIFT;
            else
                if(shiftkeys & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) keycode |= ADD_CONTROL;
                else
                    if(shiftkeys & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) keycode |= ADD_ALT;
            if(keycode && ir.Event.KeyEvent.bKeyDown)
                {if(KB_freq < sizeof(KB_Buff)/sizeof(int)) KB_Buff[KB_freq++] = keycode;}
            else
              if(KB_freq < sizeof(KB_Buff)/sizeof(int)) KB_Buff[KB_freq++] = KE_SHIFTKEYS;
        }
      }
      return;
      default: break;
    }
//    if(is_win9x && !is_read) ReadConsoleInput(hIn,&ir,1,&nread);
  }
  else if( win32_mbuttons )
  {
    if(KB_freq < sizeof(KB_Buff)/sizeof(int)) KB_Buff[KB_freq++] = KE_MOUSE;
  }
  else
      Sleep( 1 );
}

void __FASTCALL__ __init_keyboard( const char *user_cp )
{
  hIn = GetStdHandle(STD_INPUT_HANDLE);
  is_win9x = win32_verinfo.dwMajorVersion == 4 &&
             win32_verinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS;
  if((__mou_nbtns = __init_mouse()) != INT_MAX)
  {
    __MsSetState(True);
  }
}

void __FASTCALL__ __term_keyboard( void )
{
  if(__mou_nbtns != INT_MAX)
  {
    __MsSetState(False);
    __term_mouse();
  }
}

int __FASTCALL__ __kbdTestKey( unsigned long flg )
{
  if(__MsGetBtns() && flg == KBD_NONSTOP_ON_MOUSE_PRESS) return KE_MOUSE;
  if(hInputTrigger) win32_readNextMessage();
  return KB_freq;
}

int __FASTCALL__ __kbdGetShiftsKey( void )
{
  int ret;
  ret = 0;
  if(hInputTrigger) win32_readNextMessage();
  if(shiftkeys & SHIFT_PRESSED) ret |= KS_SHIFT;
  if((shiftkeys & LEFT_ALT_PRESSED) || (shiftkeys & RIGHT_ALT_PRESSED)) ret |= KS_ALT;
  if((shiftkeys & LEFT_CTRL_PRESSED) || (shiftkeys & RIGHT_CTRL_PRESSED)) ret |= KS_CTRL;
  if(shiftkeys & CAPSLOCK_ON) ret |= KS_CAPSLOCK;
  if(shiftkeys & NUMLOCK_ON) ret |= KS_NUMLOCK;
  if(shiftkeys & SCROLLLOCK_ON) ret |= KS_SCRLOCK;
  return ret;
}

int __FASTCALL__ __kbdGetKey ( unsigned long flg )
{
  int ret;
  if(__MsGetBtns() && flg == KBD_NONSTOP_ON_MOUSE_PRESS) return KE_MOUSE;
  while(!KB_freq) { /*__OsYield()*/; win32_readNextMessage(); }
  ret = KB_Buff[0];
  --KB_freq;
  if(KB_freq) memmove(KB_Buff,&KB_Buff[1],KB_freq*sizeof(int));
  return ret;
}
