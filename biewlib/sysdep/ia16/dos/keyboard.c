/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/dos/keyboard.c
 * @brief       This file contains implementation of keyboard handles for DOS.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1995
 * @note        Development, fixes and improvements
**/
#include <bios.h>
#include <limits.h>

#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

static int __ms_nbtns;

void __FASTCALL__ __init_keyboard( void )
{
   if((__ms_nbtns = __init_mouse()) != INT_MAX)
   {
     __MsSetState(True);
   }
}

void __FASTCALL__ __term_keyboard( void )
{
  if(__ms_nbtns != INT_MAX)
  {
    __MsSetState(False);
    __term_mouse();
  }
}

static int __NEAR__ __FASTCALL__ __normalize_code(int code)
{
  switch(code)
  {
    case 0x50E0: return 0x5000; /* down arrow right(gray) */
    case 0x48E0: return 0x4800; /* up  arrow right(gray) */
    case 0x4B30: return 0x4B00; /* letf arrow right(gray) */
    case 0x4D30: return 0x4D00; /* right arrow right(gray) */
    default: return code;
  }
}

static int _mou_btns = 0;
static tAbsCoord _mou_x = 0,_mou_y = 0;

static int __NEAR__ __FASTCALL__ isMouseEventPresent( unsigned long flg, int flush_queue )
{
  int _btns;
  tAbsCoord x,y;
  if(__ms_nbtns == INT_MAX) return 0;
  _btns = __MsGetBtns();
  __MsGetPos(&x,&y);
  if(_btns != _mou_btns ||
     x != _mou_x ||
     y != _mou_y ||
     (((flg & KBD_NONSTOP_ON_MOUSE_PRESS) == KBD_NONSTOP_ON_MOUSE_PRESS) && _btns))
  {
    if(flush_queue)
    {
     _mou_btns = _btns;
     _mou_x = x;
     _mou_y = y;
    }
    return 1;
  }
  return 0;
}

static int __old_ks = 0;

static int __NEAR__ __FASTCALL__ isShiftKeysChange( int flush_queue )
{
  int __ks;
  __ks = __kbdGetShiftsKey();
  if(__ks != __old_ks)
  {
    if(flush_queue) __old_ks = __ks;
    return 1;
  }
  return 0;
}

static int __NEAR__ __FASTCALL__ __test_key( unsigned long flg, int flush_queue )
{
  int ret;
  ret = _bios_keybrd(_KEYBRD_READY);
  if(!ret)
  {
    if(isMouseEventPresent(flg,flush_queue)) return KE_MOUSE;
    if(isShiftKeysChange(flush_queue))   return KE_SHIFTKEYS;
  }
  return __normalize_code(ret);
}


int __FASTCALL__ __kbdTestKey( unsigned long flg )
{
  return __test_key(flg,0);
}

int __FASTCALL__ __kbdGetShiftsKey( void )
{
  int ret = _bios_keybrd(_KEYBRD_SHIFTSTATUS);
  if(ret & 0x01) ret |= 3; /* return right shift as letf */
  return ret;
}

int __FASTCALL__ __kbdGetKey ( unsigned long flg )
{
  int key;
  while(1)
  {
    key = __test_key(flg,1);
    if(key) break;
    __OsYield();
  }
  if(!(key == KE_MOUSE || key == KE_SHIFTKEYS))
           key = __normalize_code(_bios_keybrd(_KEYBRD_READ));
  return key;
}
