/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/qnxnto/mouse.c
 * @brief       This file contains implementation of mouse handles for QNX6.
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
**/

#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

int _mouse_state;
int _mouse_buttons;

int __FASTCALL__ __init_mouse(void)
{
	return 0;
}

void __FASTCALL__ __term_mouse(void)
{
}

tBool __FASTCALL__ __MsGetState(void)
{
	return _mouse_state;
}

void __FASTCALL__ __MsSetState(tBool is_visible)
{
	_mouse_state=is_visible;
}

void __FASTCALL__ __MsGetPos(tAbsCoord *mx, tAbsCoord *my)
{
	*mx=0;
	*my=0;
}

int __FASTCALL__ __MsGetBtns(void)
{
	return 0;
}

