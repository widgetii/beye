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
 * @bug         Under WinNT does not return correct values for some
 *              combinations of keys (like CtrlBkSpace)
**/
#include <windows.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "biewlib/kbd_code.h"
#include "biewlib/biewlib.h"

static int KB_Buff[64];
static unsigned char KB_freq = 0;
static int shiftkeys = 0;

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

static int __FASTCALL__ is_VKMouse(int code)
{
  return code == VK_LBUTTON ||
         code == VK_RBUTTON ||
         code == VK_MBUTTON;
}

extern tAbsCoord win32_mx,win32_my;
extern int win32_mbuttons;

static int __FASTCALL__ __normalize_keycode(int keycode)
{
  int ret;
  switch(keycode)
  {
    case 0x50E0: ret = 0x5000; break;
    case 0x48E0: ret = 0x4800; break;
    case 0x4BE0: ret = 0x4B00; break;
    case 0x4DE0: ret = 0x4D00; break;
    case 0x47E0: ret = 0x4700; break;
    case 0x4FE0: ret = 0x4F00; break;
    case 0x49E0: ret = 0x4900; break;
    case 0x51E0: ret = 0x5100; break;
    case 0x52E0: ret = 0x5200; break;
    case 0x53E0: ret = 0x5300; break;
    default: ret = keycode;
  }
  return ret;
}

void __FASTCALL__ win32_readNextMessage( void )
{
  DWORD total_nread,i;
  int vkeycode,keycode;
  int is_write = 0;
  INPUT_RECORD ir;
  GetNumberOfConsoleInputEvents(hIn,&total_nread);
  for(i = 0;i < total_nread;i++)
  {
    DWORD nread;
    int is_read = 0;
    if(is_win9x) PeekConsoleInput(hIn,&ir,1,&nread);
    else         ReadConsoleInput(hIn,&ir,1,&nread);
    if(!nread) break; /* sometimes happen */
    switch(ir.EventType)
    {
      case MOUSE_EVENT:
      {
         if(ir.Event.MouseEvent.dwEventFlags == MOUSE_MOVED)
         {
            win32_mx = ir.Event.MouseEvent.dwMousePosition.X;
            win32_my = ir.Event.MouseEvent.dwMousePosition.Y;
         }
         else  win32_mbuttons = ir.Event.MouseEvent.dwButtonState;
         if(KB_freq < sizeof(KB_Buff)/sizeof(int)) KB_Buff[KB_freq++] = KE_MOUSE;
      }
      break;
      case KEY_EVENT:
      {
        shiftkeys = ir.Event.KeyEvent.dwControlKeyState;
        vkeycode = ir.Event.KeyEvent.wVirtualKeyCode;
        if(is_VKMouse(vkeycode))
        {
          if(vkeycode == VK_LBUTTON)
          {
            if(ir.Event.KeyEvent.bKeyDown == TRUE) win32_mbuttons |= MS_LEFTPRESS;
            else                                   win32_mbuttons &= ~MS_LEFTPRESS;
          }
          if(vkeycode == VK_RBUTTON)
          {
            if(ir.Event.KeyEvent.bKeyDown == TRUE) win32_mbuttons |= MS_RIGHTPRESS;
            else                                   win32_mbuttons &= ~MS_RIGHTPRESS;
          }
          if(vkeycode == VK_MBUTTON)
          {
            if(ir.Event.KeyEvent.bKeyDown == TRUE) win32_mbuttons |= MS_MIDDLEPRESS;
            else                                   win32_mbuttons &= ~MS_MIDDLEPRESS;
          }
         if(KB_freq < sizeof(KB_Buff)/sizeof(int)) KB_Buff[KB_freq++] = KE_MOUSE;
        }
        if(ir.Event.KeyEvent.bKeyDown == TRUE)
        {
          /*
             We must do this stupid work cause of in win95 - OSR2
             when CAPSLOCK is ON and ESCAPE key is PRESSED then
             ...uChar.AsciiChar == 0 for it. I do not know any more such bugs.
          */
          if((unsigned char)ir.Event.KeyEvent.uChar.AsciiChar == 0 &&
             (unsigned char)ir.Event.KeyEvent.wVirtualKeyCode == 27 &&
             (unsigned char)ir.Event.KeyEvent.wVirtualScanCode == 1 &&
             is_win9x)
                   ir.Event.KeyEvent.uChar.AsciiChar = 27;
          keycode = ((ir.Event.KeyEvent.wVirtualScanCode << 8) & 0xFF00) |
                    (ir.Event.KeyEvent.uChar.AsciiChar & 0xFF);
          if(is_VKCtrl(vkeycode))
          {
            if(KB_freq < sizeof(KB_Buff)/sizeof(int)) KB_Buff[KB_freq++] = KE_SHIFTKEYS;
          }
          else
          if(KB_freq < sizeof(KB_Buff)/sizeof(int))
          {
            if(is_win9x)
            {
              if(shiftkeys & SHIFT_PRESSED) keycode |= ADD_SHIFT;
              else
                if((shiftkeys & LEFT_CTRL_PRESSED) ||
                   (shiftkeys & RIGHT_CTRL_PRESSED)) keycode |= ADD_CONTROL;
                else
                  if((shiftkeys & LEFT_ALT_PRESSED) ||
                     (shiftkeys & RIGHT_ALT_PRESSED))  keycode |= ADD_ALT;
              /*
                 ReadConsoleInput in win9x don't return correct keycode
                 for national language letters, so use ReadConsole (it works)
                 this method can't be enabled in winnt, because it has its own bugs
              */
              if((unsigned char)ir.Event.KeyEvent.uChar.AsciiChar>=0x20
                 && !(keycode & ADD_CONTROL) && !(keycode & ADD_ALT))
              {
                char key;
                /*
                   Put empty key up event to queue (we'll ignore it later),
                   otherwise ReadConsole works with delay
                   if called after PeekConsoleInput and queue is empty
                */
                if(!is_write && (i+1 == total_nread))
                {
                  memset(&ir.Event.KeyEvent,0,sizeof(ir.Event.KeyEvent));
                  WriteConsoleInput(hIn,&ir,1,&nread);
                  is_write = 1;
                }
                ReadConsole(hIn,&key,1,&nread,NULL);
                keycode = (unsigned char)key;
                if(shiftkeys & SHIFT_PRESSED) keycode |= ADD_SHIFT;
                /*
                   Another bug: if control key is pressed (BKSP, TAB, ENTER etc.),
                   then sometimes ReadConsole return its code instead of code
                   of current key returned by PeekConsoleInput
                   Can't do anything smart here, simply ignore obvious bad keys
                */
                if(keycode < 0x20) keycode=0;
                is_read = 1;
              }
            }
            else /* its nt - i.e. all good */
            {
              int sk;
              sk = __kbdGetShiftsKey();
              keycode = __normalize_keycode(keycode);
              if(sk & KS_SHIFT) keycode |= ADD_SHIFT;
              else
                if(sk & KS_CTRL) keycode |= ADD_CONTROL;
                else
                  if(sk & KS_ALT) keycode |= ADD_ALT;
            }
            if(keycode)
              if(KB_freq < sizeof(KB_Buff)/sizeof(int)) KB_Buff[KB_freq++] = keycode;
          }
        }
        else
         if(ir.Event.KeyEvent.bKeyDown == FALSE)
         {
           if(is_VKCtrl(vkeycode))
           {
             if(KB_freq < sizeof(KB_Buff)/sizeof(int)) KB_Buff[KB_freq++] = KE_SHIFTKEYS;
           }
         }
      }
      break;
      default: break;
    }
    if(is_win9x && !is_read) ReadConsoleInput(hIn,&ir,1,&nread);
  }
  hInputTrigger=False;
}

void __FASTCALL__ __init_keyboard( void )
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
  while(!KB_freq) { __OsYield(); win32_readNextMessage(); }
  ret = KB_Buff[0];
  --KB_freq;
  if(KB_freq) memmove(KB_Buff,&KB_Buff[1],KB_freq*sizeof(int));
  return ret;
}
