/**
 * @namespace   biew
 * @file        codeguid.h
 * @brief       This file contains prototypes code navigator.
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
#ifndef __CODEGUID__H
#define __CODEGUID__H

#ifndef __SYS_DEP_H
#include "_sys_dep.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern char codeguid_image[];

extern void              __FASTCALL__ GidResetGoAddress( int keycode );
extern void              __FASTCALL__ GidAddGoAddress(char *str,__filesize_t addr);
extern void              __FASTCALL__ GidAddBackAddress( void );
extern __filesize_t      __FASTCALL__ GidGetGoAddress(unsigned keycode);
extern char *            __FASTCALL__ GidEncodeAddress(__filesize_t cfpos,tBool aresolv);

extern tBool             __FASTCALL__ initCodeGuider( void );
extern void              __FASTCALL__ termCodeGuider( void );

#ifdef __cplusplus
}
#endif

#endif
