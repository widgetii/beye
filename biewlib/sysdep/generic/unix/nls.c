/** 
 * @namespace   biewlib
 * @file        biewlib/sysdep/generic/unix/nls.c
 * @brief       This file contains implementation of cyrillic codepages support
 *              for KOI8-R designed by Chernov.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Konstantin Boldyshev
 * @since       1999
 * @note        Development, fixes and improvements
**/

/*
    Copyright (C) 1999-2001 Konstantin Boldyshev <konst@linuxassembly.org>

    $Id$
*/

#ifndef lint
static const char copyright[] = "$Id$";
#endif

#include "biewlib/biewlib.h"

int do_nls = 1;

static unsigned char alt2koi[] =
{
0xe1,0xe2,0xf7,0xe7,0xe4,0xe5,0xf6,0xfa,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,
0xf2,0xf3,0xf4,0xf5,0xe6,0xe8,0xe3,0xfe,0xfb,0xfd,0xff,0xf9,0xf8,0xfc,0xe0,0xf1,
0xc1,0xc2,0xd7,0xc7,0xc4,0xc5,0xd6,0xda,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,
0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0xba,0x8b,0x8c,0x8d,0x8e,0x8f,
0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
0xd2,0xd3,0xd4,0xd5,0xc6,0xc8,0xc3,0xde,0xdb,0xdd,0xdf,0xd9,0xd8,0xdc,0xc0,0xd1,
0xb3,0xa3,0x99,0x98,0x93,0x9b,0x9f,0x97,0x9c,0x95,0x9e,0x96,0xbf,0x9d,0x94,0x9a
};

void __FASTCALL__ __nls_OemToOsdep(unsigned char *buff, unsigned int len)
{
    if (do_nls) __nls_OemToFs(buff, len);
}

void __FASTCALL__ __nls_OemToFs(unsigned char *buff, unsigned int len)
{
	int i;

	for (i = 0; i < len; i++)
	    if (buff[i] >= 0x80) buff[i] = __Xlat__(alt2koi,buff[i] - 0x80);
}

void __FASTCALL__ __nls_CmdlineToOem(unsigned char *buff, unsigned int len)
{
}
