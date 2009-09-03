/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/win/nls.c
 * @brief       This file contains implementation of OEM codepages support for Win16
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
#include <windows.h>
#include "biewlib/biewlib.h"

void __FASTCALL__ __nls_OemToOsdep(unsigned char *buff,unsigned len)
{
  OemToAnsiBuff(buff,buff,len);
}

void __FASTCALL__ __nls_OemToFs(unsigned char *buff,unsigned len)
{
  OemToAnsiBuff(buff,buff,len);
}

void __FASTCALL__ __nls_CmdlineToOem(unsigned char *buff,unsigned len)
{
  AnsiToOemBuff(buff,buff,len);
}
