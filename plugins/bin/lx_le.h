/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/lx_le.h
 * @brief       This file contains LX and LE executable file definitions.
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
#ifndef __LX_LE_INC
#define __LX_LE_INC

#ifndef __BIEWUTIL__H
#include "biewutil.h"
#endif

#ifndef __BBIO_H
#include "bbio.h"
#endif

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

/** Linear eXecutable header */
typedef struct tagLXHEADER
{
  tUInt8    lxSignature[2];  /**< 'LX' */
  tUInt8    lxByteOrdering;
  tUInt8    lxWordOrdering;
  tUInt32   lxFormatLevel;
  tUInt16   lxCPUType;
  tUInt16   lxOSType;
  tUInt32   lxModuleVersion;
  tUInt32   lxModuleFlags;
  tUInt32   lxPageCount;
  tUInt32   lxEIPObjectNumbers;
  tUInt32   lxEIP;
  tUInt32   lxESPObjectNumbers;
  tUInt32   lxESP;
  tUInt32   lxPageSize;
  /* ------ specific LX part --------- */
  tUInt32   lxPageOffsetShift;
  tUInt32   lxFixupSectionSize;
  tUInt32   lxFixupSectionChecksum;      /**< different LE/LX part */
  tUInt32   lxLoaderSectionSize;
  tUInt32   lxLoaderSectionChecksum;
  tUInt32   lxObjectTableOffset;
  tUInt32   lxObjectCount;
  tUInt32   lxObjectPageTableOffset;
  tUInt32   lxObjectIterPageOffset;
  tUInt32   lxResourceTableOffset;
  tUInt32   lxNumberResourceTableEntries;
  tUInt32   lxResidentNameTableOffset;
  tUInt32   lxEntryTableOffset;
  tUInt32   lxModuleDirectivesOffset;
  tUInt32   lxNumberModuleDirectives;
  tUInt32   lxFixupPageTableOffset;
  tUInt32   lxFixupRecordTableOffset;
  tUInt32   lxImportModuleTableOffset;
  tUInt32   lxImportModuleTableEntries;
  tUInt32   lxImportProcedureTableOffset;
  tUInt32   lxPerPageChecksumOffset;
  tUInt32   lxDataPagesOffset;
  tUInt32   lxNumberPreloadPages;
  tUInt32   lxNonResidentNameTableOffset;
  tUInt32   lxNonResidentNameTableLength;
  tUInt32   lxNonResidentNameTableChecksum;
  tUInt32   lxAutoDSObjectNumber;     /**< not present in LE */
  tUInt32   lxDebugInfoOffset;
  tUInt32   lxDebugInfoLength;
  tUInt32   lxNumberInstancePreload;  /**< not present in LE */
  tUInt32   lxNumberInstanceDemand;   /**< not present in LE */
  tUInt32   lxHeapSize;               /**< not present in LE */
  tUInt32   lxStackSize;              /**< not present in LE */
}LXHEADER;

/** Linear EXE header */
typedef struct tagLEHEADER
{
  /* --------- common LE/LX part ------------- */
  tUInt8    leSignature[2];  /**< 'LE' */
  tUInt8    leByteOrdering;
  tUInt8    leWordOrdering;
  tUInt32   leFormatLevel;
  tUInt16   leCPUType;
  tUInt16   leOSType;
  tUInt32   leModuleVersion;
  tUInt32   leModuleFlags;
  tUInt32   lePageCount;
  tUInt32   leEIPObjectNumbers;
  tUInt32   leEIP;
  tUInt32   leESPObjectNumbers;
  tUInt32   leESP;
  tUInt32   lePageSize;
  /* ---------- specific LE part ----------------- */
  tUInt32   lePageOffsetShift; /**< possible not used */
  tUInt32   leFixupSize;
  tUInt32   lePageChecksum;            /**< different LE/LX part */
  tUInt32   leLoaderSectionSize;
  tUInt32   leLoaderSectionChecksum;
  tUInt32   leObjectTableOffset;
  tUInt32   leObjectCount;
  tUInt32   leObjectPageMapTableOffset;
  tUInt32   leObjectIterDataMapOffset;
  tUInt32   leResourceTableOffset;
  tUInt32   leResourceCount;
  tUInt32   leResidentNameTableOffset;
  tUInt32   leEntryTableOffset;
  tUInt32   leModuleDirectivesOffset;
  tUInt32   leModuleDirectivesCount;
  tUInt32   leFixupPageTableOffset;
  tUInt32   leFixupRecordTableOffset;
  tUInt32   leImportModuleTableOffset;
  tUInt32   leImportModuleEntryCount;
  tUInt32   leImportProcedureNamesTableOffset;
  tUInt32   lePerPageChecksumTableOffset;
  tUInt32   leDataPagesOffset;
  tUInt32   lePreloadPageCount;
  tUInt32   leNonResidentNameTableOffset;
  tUInt32   leNonResidentNameTableLength;
  tUInt32   leNonResidentNameTableChecksum;
  tUInt32   leDebugInfoOffset;
  tUInt32   leDebugInfoLength;
}LEHEADER;

typedef struct tag_VxD_Desc_Block
{
   tUInt32       DDB_Next                ; /**< VMM RESERVED FIELD */
   tUInt16       DDB_SDK_Version         ; /**< VMM RESERVED FIELD */
   tUInt16       DDB_Req_Device_Number   ; /**< Required device number */
   tUInt8        DDB_Dev_Major_Version   ; /**< Major device number */
   tUInt8        DDB_Dev_Minor_Version   ; /**< Minor device number */
   tUInt16       DDB_Flags               ; /**< Flags for init calls complete */
   tInt8         DDB_Name[8]             ; /**< Device name */
   tUInt32       DDB_Init_Order          ; /**< Initialization Order */
   tUInt32       DDB_Control_Proc        ; /**< Offset of control procedure */
   tUInt32       DDB_V86_API_Proc        ; /**< Offset of API procedure (or 0) */
   tUInt32       DDB_PM_API_Proc         ; /**< Offset of API procedure (or 0) */
   tUInt32       DDB_V86_API_CSIP        ; /**< CS:IP of API entry point */
   tUInt32       DDB_PM_API_CSIP         ; /**< CS:IP of API entry point */
   tUInt32       DDB_Reference_Data      ; /**< Reference data from real mode */
   tUInt32       DDB_Service_Table_Ptr   ; /**< Pointer to service table */
   tUInt32       DDB_Service_Table_Size  ; /**< Number of services */
}VxD_Desc_Block;


extern union LX_LE
{
  LEHEADER le;
  LXHEADER lx;
}lxe;

/** Flat .EXE object table entry */
typedef struct o32_obj
{
    tUInt32       o32_size;       /**< Object virtual size */
    tUInt32       o32_base;       /**< Object base virtual address */
    tUInt32       o32_flags;      /**< Attribute flags */
    tUInt32       o32_pagemap;    /**< Object page map index */
    tUInt32       o32_mapsize;    /**< Number of entries in object page map */
    tUInt32       o32_reserved;   /**< Reserved */
}LX_OBJECT;

#define PAGE_VALID       0x0000    /**< Valid Physical Page in .EXE */
#define PAGE_ITERDATA    0x0001    /**< Iterated Data Page */
#define PAGE_INVALID     0x0002    /**< Invalid Page */
#define PAGE_ZEROED      0x0003    /**< Zero Filled Page */
#define PAGE_RANGE       0x0004    /**< Range of pages */
#define PAGE_ITERDATA2   0x0005    /**< Iterated Data Page Type II */

/* Object Page Table entry */
typedef struct o32_map
{
    tUInt32  o32_pagedataoffset;     /**< file offset of page */
    tUInt16  o32_pagesize;           /**< # bytes of page data */
    tUInt16  o32_pageflags;          /**< Per-Page attributes */
}LX_MAP_TABLE;

typedef struct LX_Iter
{
    tUInt16 LX_nIter;            /**< number of iterations */
    tUInt16 LX_nBytes;           /**< number of bytes */
    tUInt8  LX_Iterdata;         /**< iterated data byte(s) */
}LX_ITER;

typedef struct b32_bundle
{
    tUInt8       b32_cnt;        /**< Number of entries in this bundle */
    tUInt8       b32_type;       /**< Bundle type */
    tUInt16      b32_obj;        /**< Object number */
}LX_BUNGLE;                       /* Follows entry types */

/** 16-bit or 32-bit offset */
typedef union _offset
{
    tUInt16 offset16;
    tUInt32 offset32;
}offset;

typedef struct e32_entry
{
    tUInt8       e32_flags;      /**< Entry point flags */
    union entrykind
    {
        offset          e32_offset;     /**< 16-bit/32-bit offset entry */
        struct callgate
        {
            tUInt16 offset;      /**< Offset in segment */
            tUInt16 callgate;    /**< Callgate selector */
        }e32_callgate;   /**< 286 (16-bit) call gate */
        struct fwd
        {
            tUInt16  modord;     /**< Module ordinal number */
            tUInt32  value;      /**< Proc name offset or ordinal */
        }e32_fwd;        /**< Forwarder */
    }e32_variant;    /**< Entry variant */
}e32_ENTRY;

/*
 *  In 32-bit .EXE file run-time relocations are written as varying size
 *  records, so we need many size definitions.
 */

#define RINTSIZE16      8
#define RINTSIZE32      10
#define RORDSIZE        8
#define RNAMSIZE16      8
#define RNAMSIZE32      10
#define RADDSIZE16      10
#define RADDSIZE32      12

/*
 *  BUNDLE TYPES
 */

#define EMPTY        0x00               /* Empty bundle */
#define ENTRY16      0x01               /* 16-bit offset entry point */
#define GATE16       0x02               /* 286 call gate (16-bit IOPL) */
#define ENTRY32      0x03               /* 32-bit offset entry point */
#define ENTRYFWD     0x04               /* Forwarder entry point */
#define TYPEINFO     0x80               /* Typing information present flag */

typedef struct lxEntry
{
  tInt8 b32_type;
  tInt8 b32_obj;
  e32_ENTRY entry;
}LX_ENTRY;

typedef struct tagLE_PAGE
{
  tUInt16 flags;
  tUInt16 number;
}LE_PAGE;

typedef struct tagLXResource
{
   tUInt16 typeID;
   tUInt16 nameID;
   tUInt32 resourceSize;
   tUInt16 object;
   tUInt32 offset;
}LXResource;


extern void          __FASTCALL__ ShowFwdModOrdLX(const LX_ENTRY *_lxe);
extern unsigned long __FASTCALL__ ShowNewHeaderLX( void );
extern unsigned long __FASTCALL__ ShowObjectsLX( void );
extern unsigned      __FASTCALL__ LXRNamesNumItems(BGLOBAL);
extern tBool         __FASTCALL__ LXRNamesReadItems(BGLOBAL,memArray *,unsigned);
extern unsigned long __FASTCALL__ ShowModRefLX( void );
extern unsigned      __FASTCALL__ LXNRNamesNumItems(BGLOBAL);
extern tBool         __FASTCALL__ LXNRNamesReadItems(BGLOBAL,memArray *,unsigned);
extern unsigned long __FASTCALL__ ShowImpProcLXLE( void );
extern unsigned long __FASTCALL__ ShowEntriesLX( void );
extern const char *  __FASTCALL__ lxeGetMapAttr(unsigned long attr);
extern unsigned long __FASTCALL__ CalcEntryPointLE(unsigned long objnum,unsigned long _offset);
extern unsigned long __FASTCALL__ CalcPageEntryLE(unsigned long idx);
extern unsigned long __FASTCALL__ CalcEntryLE(const LX_ENTRY *);

#define FILE_LX 1
#define FILE_LE 2
extern int LXType;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif

#endif
