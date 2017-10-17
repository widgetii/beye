/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/_inlines.h
 * @brief       This file includes 16-bit Intel architecture little inline functions.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       2000
 * @note        Development, fixes and improvements
**/

#include "biewlib/sysdep/generic/_inlines.h"

#define halloc halloc
#define hrealloc hrealloc
#define hfree hfree
#define HMemCpy HMemCpy

#define COREDUMP() { __asm __volatile(".short 0xffff":::"memory"); }

#undef ___INLINES_H
#include "biewlib/sysdep/generic/_inlines.h"


