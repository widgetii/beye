/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/os2/keyboard.c
 * @brief       This file contains implementation of keyboard handles for OS/2.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <limits.h>
#define INCL_SUB
#define INCL_DOSSIGNALS
#include <os2.h>

#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

static HKBD kbdHandle;
static int __ms_nbtns;

void __FASTCALL__ __init_keyboard( const char *user_cp )
{
   KbdOpen(&kbdHandle);
   if(kbdHandle) KbdGetFocus(0,kbdHandle);
   if((__ms_nbtns = __init_mouse()) != INT_MAX)
   {
     __MsSetState(True);
   }
}

void __FASTCALL__ __term_keyboard( void )
{
  KbdFreeFocus(kbdHandle);
  KbdClose(kbdHandle);
  if(__ms_nbtns != INT_MAX)
  {
    __MsSetState(False);
    __term_mouse();
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
	USHORT retval = 0;
	USHORT save;
	KBDINFO info;
	KBDKEYINFO key;

	info.cb = sizeof(KBDINFO);

	KbdGetStatus(&info,kbdHandle);
	save = info.fsMask;
	info.fsMask &= 0xfff7;
	KbdSetStatus(&info,kbdHandle);

	KbdPeek(&key,kbdHandle);
	if (key.fbStatus & 0x40) {
		retval = key.chChar + (key.chScan << 8);
	} else
		retval = 0;

	if (key.chChar == 224)
		retval &= 0xff00;
	info.fsMask = save;
	KbdSetStatus(&info,kbdHandle);
        if(retval == 0)
        {
           if(isMouseEventPresent(flg,flush_queue)) retval = KE_MOUSE;
           else if(isShiftKeysChange(flush_queue)) retval = KE_SHIFTKEYS;
        }
	return retval;
}


int __FASTCALL__ __kbdTestKey( unsigned long flg )
{
  return __test_key(flg,0);
}

int __FASTCALL__ __kbdGetShiftsKey( void )
{
	USHORT retval = 0;
	USHORT save;
	KBDINFO info;

	info.cb = sizeof(KBDINFO);

	KbdGetStatus(&info,kbdHandle);
	save = info.fsMask;
	info.fsMask &= 0xfff7;
	KbdSetStatus(&info,kbdHandle);

	retval = info.fsState;

	info.fsMask = save;
	KbdSetStatus(&info,kbdHandle);
        if(retval & 0x01) retval |= 3; /* return right shift as letf */
	return retval;
}

int __FASTCALL__ __kbdGetKey( unsigned long flg )
{
  USHORT retval = 0;
  while(1)
  {
    retval = __test_key(flg,1);
    if(retval) break;
    __OsYield();
  }
  if(!(retval == KE_MOUSE || retval == KE_SHIFTKEYS))
  {
	USHORT save;
	KBDINFO info;
	KBDKEYINFO key;

	info.cb = sizeof(KBDINFO);

	KbdGetStatus(&info,kbdHandle);
	save = info.fsMask;
	info.fsMask &= 0xfff7;
	KbdSetStatus(&info,kbdHandle);

	KbdCharIn(&key,0,kbdHandle);
	retval = key.chChar + (key.chScan << 8);

	if (key.chChar == 224)
		retval &= 0xff00;
	info.fsMask = save;
	KbdSetStatus(&info,kbdHandle);
   }
   return retval;
}
