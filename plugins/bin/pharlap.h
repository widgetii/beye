/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/pharlap.h
 * @brief       This file contains PharLap executable file definitions.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1999
 * @note        Development, fixes and improvements
**/
#ifndef __PHARLAP_INC
#define __PHARLAP_INC

#ifndef __SYS_DEP_H
#include "_sys_dep.h"
#endif

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

/** Format of .old Phar Lap .EXP file header */
typedef struct tagoldPharLap
{
  tUInt8            plSignature[2];   /**< "MP" 0x4D 0x50 */
  tUInt16           plSizeRemaind;    /**< remainder of image size / page size (page size = 512h) */
  tUInt16           plImageSize;      /**< size of image in pages */
  tUInt16           plNRelocs;        /**< number of relocation items */
  tUInt16           plHeadSize;       /**< header size in paragraphs */
  tUInt16           plMinExtraPages;  /**< minimum number of extra 4K pages to be allocated at the end of program, when it is loaded */
  tUInt16           plMaxExtraPages;  /**< maximum number of extra 4K pages to be allocated at the end of program, when it is loaded */
  tUInt32           plESP;            /**< initial ESP */
  tUInt16           plCheckSum;       /**< check sum of file */
  tUInt32           plEIP;            /**< initial EIP */
  tUInt16           plFirstReloc;     /**< offset of first relocation item */
  tUInt16           plNOverlay;       /**< overlay number */
  tUInt16           plReserved;       /**< (???) wants to be 1 */
}oldPharLap;

/** Format of new Phar Lap .EXP file header */
typedef struct tagnewPharLap
{
  tUInt8            plSignature[2];   /**< "P2" for 286 .EXP and "P3" for 386 .EXP */
  tUInt16           plLevel;          /**< 0x01 - flat model 0x02 - multisegmented file */
  tUInt16           plHeaderSize;
  tUInt32           plFileSize;       /**< Size of file in bytes */
  tUInt16           plCheckSum;
  tUInt32           plRunTimeParms;   /**< offset of run-time parameters within file */
  tUInt32           plRunTimeSize;    /**< size of run-time parameters in bytes */
  tUInt32           plRelocOffset;    /**< offset of relocation table within file */
  tUInt32           plRelocSize;      /**< size of relocation table in bytes */
  tUInt32           plSegInfoOffset;  /**< offset of segment information table within file */
  tUInt32           plSegInfoSize;    /**< size of segment information table in bytes */
  tUInt16           plSegEntrySize;   /**< size of segment information table entry in bytes */
  tUInt32           plImageOffset;    /**< offset of load image within file */
  tUInt32           plImageSize;      /**< size of load image on disk */
  tUInt32           plSymTabOffset;   /**< offset of symbol table within file */
  tUInt32           plSymTabSize;     /**< size of symbol table in bytes */
  tUInt32           plGDTOffset;      /**< offset of GDT within load image */
  tUInt32           plGDTSize;        /**< size of GDT in bytes */
  tUInt32           plLDTOffset;      /**< offset of LDT within load image */
  tUInt32           plLDTSize;        /**< size of LDT in bytes */
  tUInt32           plIDTOffset;      /**< offset of IDT within load image */
  tUInt32           plIDTSize;        /**< size of IDT in bytes */
  tUInt32           plTSSOffset;      /**< offset of TSS within load image */
  tUInt32           plTSSSize;        /**< size of TSS in bytes */
  tUInt32           plMinExtraPages;  /**< minimum number of extra 4K pages to be allocated at the end of program, level 1 only */
  tUInt32           plMaxExtraPages;  /**< maximum number of extra 4K pages to be allocated at the end of program, level 1 only */
  tUInt32           plBase;           /**< base load offset (level 1 executables only) */
  tUInt32           plESP;            /**< initial ESP */
  tUInt16           plSS;             /**< initial SS */
  tUInt32           plEIP;            /**< initial EIP */
  tUInt16           plCS;             /**< initial CS */
  tUInt16           plLDT;            /**< initial LDT */
  tUInt16           plTSS;            /**< initial TSS */
  tUInt16           plFlags;          /**< bit 0: load image is packed */
                                     /**< bit 1: 32-bit checksum is present */
                                     /**< bits 4-2: type of relocation table */
  tUInt32           plMemReq;         /**< memory requirements for load image */
  tUInt32           plChecksum32;     /**< 32-bit checksum (optional) */
  tUInt32           plStackSize;      /**< size of stack segment in bytes */
  tUInt8            plReserv[256];
}newPharLap;

typedef struct tagPLSegInfo
{
  tUInt16           siSelector;       /**< selector number */
  tUInt16           siFlags;
  tUInt32           siBaseOff;        /**< base offset of selector */
  tUInt32           siMinAlloc;       /**< minimum number of extra bytes to be allocated to the segment */
}PLSegInfo;

typedef struct tagPLRunTimeParms
{
  tUInt8            rtSignature[2];   /**< "DX" 44h 58h */
  tUInt16           rtMinRModeParms;  /**< minimum number of real-mode params to leave free at run time */
  tUInt16           rtMaxRModeParms;  /**< maximum number of real-mode params to leave free at run time */
  tUInt16           rtMinIBuffSize;   /**< minimum interrupt buffer size in KB */
  tUInt16           rtMaxIBuffSize;   /**< maximum interrupt buffer size in KB */
  tUInt16           rtNIStacks;       /**< number of interrupt stacks */
  tUInt16           rtIStackSize;     /**< size in KB of each interrupt stack */
  tUInt32           rtEndRModeOffset; /**< offset of byte past end of real-mode code and data */
  tUInt16           rtCallBuffSize;   /**< size in KB of call buffers */
  tUInt16           rtFlags;
                                     /**< bit 0: file is virtual memory manager */
                                     /**< bit 1: file is a debugger */
  tUInt16           rtUnprivFlags;    /**< unprivileged flag (if nonzero, executes at ring 1, 2, or 3) */
  tUInt8            rtReserv[104];
}PLRunTimeParms;

typedef struct tagPLRepeatBlock
{
  tUInt16           rbCount;          /**< byte count */
  tUInt8            rbString[1];      /**< repeat string length */
}PLRepeatBlock;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif

#endif
