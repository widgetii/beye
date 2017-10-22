/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/mz.h
 * @brief       This file contains DOS driver executable file definitions.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
**/
#ifndef __DOS_SYS_INC
#define __DOS_SYS_INC

#ifndef __BIEWUTIL__H
#include "beyeutil.h"
#endif

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

typedef struct tagDOSDRIVER
{
  tUInt16 ddAttribute;
  tUInt16 ddStrategyOff;
  tUInt16 ddInterruptOff;
  tUInt8  ddName[8];
}DOSDRIVER;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif

#endif
