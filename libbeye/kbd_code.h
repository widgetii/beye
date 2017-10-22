/**
 * @namespace   biewlib
 * @file        biewlib/kbd_code.h
 * @brief       This file includes OS depended keyboard codes.
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
#include "bconfig.h"
#include "libbeye/beyelib.h"

#if defined(DOS) || defined(OS2)
    #define HDR "libbeye/sysdep/generic/dos/kbd_code.h"
#elif defined(WINDOWS)
    #define HDR "libbeye/sysdep/generic/windows/kbd_code.h"
#elif defined(LINUX) || defined(OSX) || defined(BSD)
    #define HDR "libbeye/sysdep/generic/unix/kbd_code.h"
#elif defined(QNX4)
    #define HDR "libbeye/sysdep/ia32/qnx/kbd_code.h"
#elif defined(QNX6)
    #define HDR "libbeye/sysdep/ia32/qnxnto/kbd_code.h"
#endif

#include HDR


