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
  tUInt32   peInterestingVASize;
  tUInt32   peExportTableRVA;
  tUInt32   peTotalExportDataSize;
  tUInt32   peImportTableRVA;
  tUInt32   peTotalImportDataSize;
  tUInt32   peResourceTableRVA;
  tUInt32   peTotalResourceDataSize;
  tUInt32   peExceptionTableRVA;
  tUInt32   peTotalExceptionDataSize;
  tUInt32   peSecurityTableRVA;
  tUInt32   peTotalSecurityDataSize;
  tUInt32   peFixupTableRVA;
  tUInt32   peTotalFixupDataSize;
  tUInt32   peDebugTableRVA;
  tUInt32   peTotalDebugDirectories;
  tUInt32   peImageDescriptionRVA;
  tUInt32   peTotalDescriptionSize;
  tUInt32   peMachineSpecificRVA;
  tUInt32   peMachineSpecificSize;
  tUInt32   peThreadLocalStorageRVA;
  tUInt32   peTotalTLSSize;
}PEHEADER;

typedef struct tagPE_ADDR
{
 tUInt32 rva;
 tUInt32 phys;
}PE_ADDR;

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
