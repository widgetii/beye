/**
 * @namespace   biew_plugins_I
 * @file        plugins/hexmode.h
 * @brief       This file contains function prototypes for hexadecinal mode.
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
#ifndef __HEXMODE_H
#define __HEXMODE_H

#ifndef __FILE_INI_RUNTIME_SUPPORT_SYSTEM__
#include "biewlib/file_ini.h"
#endif

extern tBool hexAddressResolv;
extern tBool __FASTCALL__ hexAddressResolution( void );
extern void  __FASTCALL__ ReadIniAResolv( hIniProfile *ini );
extern void  __FASTCALL__ WriteIniAResolv( hIniProfile *ini );

#endif
