/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/pe.h
 * @brief       This file contains PE executable file definitions.
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
#ifndef __PE_INC
#define __PE_INC

#ifndef __SYS_DEP_H
#include "_sys_dep.h"
#endif

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

/* Portable EXE header */
typedef struct tagPEHEADER
{
  tUInt8    peSignature[4];        /**< 'PE\00\00' */
  tUInt16   peCPUType;
  tUInt16   peObjects;
  tUInt32   peTimeDataStamp;
  tUInt32   peReserv1;
  tUInt32   peReserv2;
  tUInt16   peNTHdrSize;
  tUInt16   peFlags;
  tUInt16   peReserv3;
  tUInt8    peLMajor;
  tUInt8    peLMinor;
  tUInt32   peReserv4;
  tUInt32   peReserv5;
  tUInt32   peReserv6;
  tUInt32   peEntryPointRVA;
  tUInt32   peReserv7;
  tUInt32   peReserv8;
  tUInt32   peImageBase;
  tUInt32   peObjectAlign;
  tUInt32   peFileAlign;
  tUInt16   peOSMajor;
  tUInt16   peOSMinor;
  tUInt16   peUserMajor;
  tUInt16   peUserMinor;
  tUInt16   peSubSystMajor;
  tUInt16   peSubSystMinor;
  tUInt32   peReserv9;
  tUInt32   peImageSize;
  tUInt32   peHeaderSize;
  tUInt32   peFileChecksum;
  tUInt16   peSubSystem;
  tUInt16   peDLLFlags;
  tUInt32   peStackReserveSize;
  tUInt32   peStackCommitSize;
  tUInt32   peHeapReserveSize;
  tUInt32   peHeapCommitSize;
  tUInt32   peReserv10;
  tUInt32   peDirSize;
}PEHEADER;

typedef struct tagPERVA
{
  tUInt32 rva;
  tUInt32 size;
} PERVA;

#define PE_EXPORT           0
#define PE_IMPORT           1
#define PE_RESOURCE         2
#define PE_EXCEPT           3
#define PE_SECURITY         4
#define PE_FIXUP            5
#define PE_DEBUG            6
#define PE_IMAGE_DESC       7
#define PE_MACHINE          8
#define PE_TLS              9
#define PE_LOAD_CONFIG      10
#define PE_BOUND_IMPORT     11
#define PE_IAT              12
#define PE_DELAY_IMPORT     13
#define PE_COM              14
#define PE_RESERVED         15

typedef struct tagPE_ADDR
{
  tUInt32 rva;
  tUInt32 phys;
} PE_ADDR;

typedef struct tagExportTablePE
{
  tUInt32 etFlags;
  tUInt32 etDateTime;
  tUInt16 etMajVer;
  tUInt16 etMinVer;
  tUInt32 etNameRVA;
  tUInt32 etOrdinalBase;
  tUInt32 etNumEATEntries;
  tUInt32 etNumNamePtrs;
  tUInt32 etAddressTableRVA;
  tUInt32 etNamePtrTableRVA;
  tUInt32 etOrdinalTableRVA;
}ExportTablePE;

typedef struct tagImportDirPE
{
  tUInt32 idFlags;
  tUInt32 idDateTime;
  tUInt16 idMajVer;
  tUInt16 idMinVer;
  tUInt32 idNameRVA;
  tUInt32 idLookupTableRVA;
}ImportDirPE;

typedef struct tagPE_OBJECT
{
  tInt8   oName[8];
  tUInt32 oVirtualSize;
  tUInt32 oRVA;
  tUInt32 oPhysicalSize;
  tUInt32 oPhysicalOffset;
  tUInt32 oRelocPtr;
  tUInt32 oLineNumbPtr;
  tUInt16 oNReloc;
  tUInt16 oNLineNumb;
  tUInt32 oFlags;
}PE_OBJECT;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif

#endif
