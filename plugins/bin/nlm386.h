/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/nlm386.h
 * @brief       This file contains NLM file format definitions.
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
#ifndef __NLM_INC
#define __NLM_INC

#ifndef __SYS_DEP_H
#include "_sys_dep.h"
#endif

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

typedef tUInt32 file_ptr;
typedef tUInt32 bfd_size_type;
typedef tUInt32 bfd_vma;
typedef tUInt32 PTR;
#define NLM_SIGNATURE "NetWare Loadable Module\x1a"
#define NLM_SIGNATURE_SIZE 24
#define NLM_MODULE_NAME_SIZE 14
#define NLM_MAX_DESCRIPTION_LENGTH 41
#define NLM_OLD_THREAD_NAME_LENGTH 5
#define NLM_MAX_SCREEN_NAME_LENGTH 4
#define NLM_MAX_THREAD_NAME_LENGTH 7
#define NLM_MAX_COPYRIGHT_MESSAGE_LENGTH 1


typedef struct nlm_internal_fixed_header
{
  tInt8        nlm_signature[NLM_SIGNATURE_SIZE];
  tInt32       nlm_version;
  tInt8        nlm_moduleName[NLM_MODULE_NAME_SIZE];
  file_ptr      nlm_codeImageOffset;
  bfd_size_type nlm_codeImageSize;
  file_ptr      nlm_dataImageOffset;
  bfd_size_type nlm_dataImageSize;
  bfd_size_type nlm_uninitializedDataSize;
  file_ptr      nlm_customDataOffset;
  bfd_size_type nlm_customDataSize;
  file_ptr      nlm_moduleDependencyOffset;
  tInt32       nlm_numberOfModuleDependencies;
  file_ptr      nlm_relocationFixupOffset;
  tInt32       nlm_numberOfRelocationFixups;
  file_ptr      nlm_externalReferencesOffset;
  tInt32       nlm_numberOfExternalReferences;
  file_ptr      nlm_publicsOffset;
  tInt32       nlm_numberOfPublics;
  file_ptr      nlm_debugInfoOffset;
  tInt32       nlm_numberOfDebugRecords;
  file_ptr      nlm_codeStartOffset;
  file_ptr      nlm_exitProcedureOffset;
  file_ptr      nlm_checkUnloadProcedureOffset;
  tInt32       nlm_moduleType;
  tInt32       nlm_flags;
} Nlm_Internal_Fixed_Header;

typedef struct nlm_internal_variable_header
{
  tUInt8        descriptionLength;
  tInt8         descriptionText[NLM_MAX_DESCRIPTION_LENGTH + 1];
  tInt32        stackSize;
  tInt32        reserved; /**< should contain zero */
  tInt8         oldThreadName[NLM_OLD_THREAD_NAME_LENGTH]; /**< " LONG" */
  tUInt8        screenNameLength;
  tInt8         screenName[NLM_MAX_SCREEN_NAME_LENGTH + 1];
  tUInt8        threadNameLength;
  tInt8         threadName[NLM_MAX_THREAD_NAME_LENGTH + 1];
} Nlm_Internal_Variable_Header;

/** The header is recognized by "VeRsIoN#" in the stamp field. */
typedef struct nlm_internal_version_header
{
  tInt8          stamp[8];
  tInt32         majorVersion;
  tInt32         minorVersion;
  tInt32         revision;
  tInt32         year;
  tInt32         month;
  tInt32         day;
} Nlm_Internal_Version_Header;

/** The header is recognized by "CoPyRiGhT=" in the stamp field. */
typedef struct nlm_internal_copyright_header
{
  tInt8         stamp[10];
  tUInt8        copyrightMessageLength;
  tInt8         copyrightMessage[NLM_MAX_COPYRIGHT_MESSAGE_LENGTH];
} Nlm_Internal_Copyright_Header;

/** The header is recognized by "MeSsAgEs" in the stamp field. */
typedef struct nlm_internal_extended_header
{
  tInt8        stamp[8];
  tInt32       languageID;
  file_ptr      messageFileOffset;
  bfd_size_type messageFileLength;
  tInt32       messageCount;
  file_ptr      helpFileOffset;
  bfd_size_type helpFileLength;
  file_ptr      RPCDataOffset;
  bfd_size_type RPCDataLength;
  file_ptr      sharedCodeOffset;
  bfd_size_type sharedCodeLength;
  file_ptr      sharedDataOffset;
  bfd_size_type sharedDataLength;
  file_ptr      sharedRelocationFixupOffset;
  tInt32       sharedRelocationFixupCount;
  file_ptr      sharedExternalReferenceOffset;
  tInt32       sharedExternalReferenceCount;
  file_ptr      sharedPublicsOffset;
  tInt32       sharedPublicsCount;
  file_ptr      sharedDebugRecordOffset;
  tInt32       sharedDebugRecordCount;
  bfd_vma       SharedInitializationOffset;
  bfd_vma       SharedExitProcedureOffset;
  tInt32       productID;
  tInt32       reserved[6];
} Nlm_Internal_Extended_Header;

/** The format of a custom header as stored internally is different
   from the external format.  This is how we store a custom header
   which we do not recognize.  */
/** The header is recognized by "CuStHeAd" in the stamp field. */
typedef struct nlm_internal_custom_header
{
  tInt8        stamp[8];
  bfd_size_type hdrLength;
  file_ptr      dataOffset;
  bfd_size_type dataLength;
  tInt8        dataStamp[8];
  PTR           hdr;
} Nlm_Internal_Custom_Header;

/** The internal Cygnus header is written out externally as a custom
    header.  We don't try to replicate that structure here.  */

/** The header is recognized by "CyGnUsEx" in the stamp field. */
typedef struct nlm_internal_cygnus_ext_header
{
  tInt8        stamp[8];
  file_ptr      offset;  /**< File location of debugging information.  */
  bfd_size_type length;   /**< Length of debugging information.  */
} Nlm_Internal_Cygnus_Ext_Header;

/**
   Public names table:
   ===================
   +0 ( 1 byte )         - name length
   +1 ( length bytes )   - name
   +length+1 ( 4 bytes ) - physical offset from begin of code section
*/

/**
   External name table:
   ====================
   +0 ( 1 byte )         - name length
   +1 ( length bytes )   - name
   +length+1 ( 4 bytes ) - number of fixups
   +length+5 ( 4 bytes array) - physical offsets from begin of code section
*/


typedef tUInt32	Nlm32_Addr;	/**< Unsigned program address */
typedef tUInt32	Nlm32_Off;	/**< Unsigned file offset */
typedef tInt32		Nlm32_Sword;	/**< Signed large integer */
typedef tUInt32	Nlm32_Word;	/**< Unsigned large integer */
typedef tUInt16	Nlm32_Half;	/**< Unsigned medium integer */
typedef tUInt8		Nlm32_Char;	/**< Unsigned tiny integer */

typedef tInt32		Nlm64_Sword;
typedef tUInt32	Nlm64_Word;
typedef tUInt16	Nlm64_Half;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif


#endif
