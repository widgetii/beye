/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/mz.h
 * @brief       This file contains MZ executable file definitions.
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
#ifndef __MZ_INC
#define __MZ_INC

#ifndef __BIEWUTIL__H
#include "biewutil.h"
#endif

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

typedef struct tagMZHEADER
{
 tUInt16 mzPartLastPage;
 tUInt16 mzPageCount;
 tUInt16 mzRelocationCount;
 tUInt16 mzHeaderSize;
 tUInt16 mzMinMem;
 tUInt16 mzMaxMem;
 tUInt16 mzRelocationSS;
 tUInt16 mzExeSP;
 tUInt16 mzCheckSumm;
 tUInt16 mzExeIP;
 tUInt16 mzRelocationCS;
 tUInt16 mzTableOffset;
 tUInt16 mzOverlayNumber;
}MZHEADER;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif

#endif
