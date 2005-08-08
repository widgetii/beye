/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/win32/nls.c
 * @brief       This file contains implementation of OEM codepages support
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
**/
/* for cygwin - remove unnecessary includes */
#define _OLE_H
#define _OLE2_H
#include <windows.h>

#include "biewlib/biewlib.h"

tBool win32_use_ansi;

void __FASTCALL__ __nls_OemToOsdep(unsigned char *buff,unsigned len)
{
 if(win32_use_ansi)
 {
   OemToCharBuff(buff,buff,len);
 }
}

void __FASTCALL__ __nls_OemToFs(unsigned char *buff,unsigned len)
{
}

void __FASTCALL__ __nls_CmdlineToOem(unsigned char *buff,unsigned len)
{
   CharToOemBuff(buff,buff,len);
}
