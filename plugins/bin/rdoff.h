/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/rdoff.h
 * @brief       This file contains RDOFF v1 file definitions.
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
#ifndef __RDOFF_INC
#define __RDOFF_INC

#ifndef __SYS_DEP_H
#include "_sys_dep.h"
#endif

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

/** RDOFF v1 fixed header */
typedef struct tag_rdoff_Header
{
  tUInt8  id[6]; /**< RDOFF1 or RDOFF\01 */
  tUInt32 header_len;
}rdoff_Header;

typedef struct tag_RDOFF_RELOC
{
  tUInt8  reflen; /**< length of references */
  tUInt8  is_rel; /**< is relative fixup */
  tUInt16 segto;  /**< logical # of segment or ext. reference
                            i.e. max external refers = 65536-2 */
  tUInt32 offset; /**< absolute offset within file */
}RDOFF_RELOC;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif

#endif
