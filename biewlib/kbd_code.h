/**
 * @namespace   biewlib
 * @file        biewlib/kbd_code.h
 * @brief       This file includes OS depended keyboard codes.
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
#include "biewlib/biewlib.h"
#if __WORDSIZE == 16
 #if defined( __MSDOS__ ) || defined ( __OS2__ ) || defined ( __WINDOWS__ )
   #include "biewlib/sysdep/ia16/dos/kbd_code.h"
 #else
   #error Unknown operationg system for IA-16 architecture
 #endif
#else
  #if defined(__WIN32__) && defined(_MSC_VER)
    #define __OS_KEYBOARD <biewlib/sysdep/ia32/win32/kbd_code.h>
  #else
    #define __OS_KEYBOARD <biewlib/sysdep/__MACHINE__/__OS__/kbd_code.h>
  #endif
  #include __OS_KEYBOARD
#endif

