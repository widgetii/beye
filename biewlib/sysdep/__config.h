/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/_sys_dep.h
 * @brief       This file contains configuration part of BIEW project.
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
#ifndef __CONFIG_H
#define __CONFIG_H 1

#if defined(__WIN32__) && defined(_MSC_VER)
    #include "biewlib/sysdep/ia32/__config.h"
#else
    #define __CONFIG <biewlib/sysdep/__MACHINE__/__config.h>
    #include __CONFIG
#endif

#endif
