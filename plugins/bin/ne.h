/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/ne.h
 * @brief       This file contains NE executable file definitions.
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
#ifndef __NE_INC
#define __NE_INC

#ifndef __BIEWUTIL__H
#include "biewutil.h"
#endif

#ifndef __BBIO_H
#include "bbio.h"
#endif

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

#define NE_WIN2X_ON_3X_PROTMODE 2
#define NE_WIN2X_PROPORTFONT    4
#define NE_FASTLOADAREA         8

/** New EXE header */
typedef struct tagNEHEADER
{
 tUInt8   neSignature[2];               /**< 'NE' */
 tUInt8   neLinkerVersion;
 tUInt8   neLinkerRevision;
 tUInt16  neOffsetEntryTable;
 tUInt16  neLengthEntryTable;
 tUInt32  neChecksum;
 tUInt16  neContestEXE;
 tUInt16  neAutoDataSegmentCount;
 tUInt16  neHeapSize;
 tUInt16  neStackSize;
 tUInt32  neCSIPvalue;
 tUInt32  neSSSPvalue;
 tUInt16  neSegmentTableCount;
 tUInt16  neModuleReferenceTableCount;
 tUInt16  neLengthNonResidentNameTable;
 tUInt16  neOffsetSegmentTable;
 tUInt16  neOffsetResourceTable;
 tUInt16  neOffsetResidentNameTable;
 tUInt16  neOffsetModuleReferenceTable;
 tUInt16  neOffsetImportTable;
 tUInt32  neOffsetNonResidentNameTable;
 tUInt16  neMoveableEntryPointCount;
 tUInt16  neLogicalSectorShiftCount;
 tUInt16  neResourceSegmentCount;
 tUInt8   neOperatingSystem;
 tUInt8   neFlagsOther;
 /* os depended 64 bytes struct */
 tUInt16  neOffsetFastLoadArea;
 tUInt16  neLengthFastLoadArea;
 tUInt16  neReserved;
 tUInt16  neWindowsVersion;

}NEHEADER;

typedef struct tagSEGDEF
{
  tUInt16 sdOffset;
  tUInt16 sdLength;
  tUInt16 sdFlags;
  tUInt16 sdMinMemory;
}SEGDEF;

extern int ReadSegDef(SEGDEF *,tUInt16 numseg);

typedef struct tagENTRY
{
 tUInt8  eFlags;
 tUInt8  eFixed; /**< 1 - fixed 0 - moveable */
 /* tUInt16 eInt3F; */
 tUInt8  eSegNum;
 tUInt16 eSegOff;
}ENTRY;

typedef struct tagNAMEINFO
{
  tUInt16 rnOffset;
  tUInt16 rnLength;
  tUInt16 rnFlags;
  tUInt16 rnID;
  tUInt16 rnHandle;
  tUInt16 rnUsage;
} NAMEINFO;

extern int ReadEntry(ENTRY *,tUInt16 entnum);

typedef struct tagRELOC_NE
{
  tUInt8  AddrType;
  tUInt8  Type;
  tUInt16 RefOff;
  tUInt16 idx;
  tUInt16 ordinal;
}RELOC_NE;

extern const char * __FASTCALL__ GetPMWinAPI(unsigned flag);
extern const char * __nedata[];
extern unsigned __FASTCALL__ GetNamCountNE(BGLOBAL,__filesize_t);
extern tBool __FASTCALL__ RNamesReadItems(BGLOBAL,memArray *,unsigned,__filesize_t);


#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif

#endif
