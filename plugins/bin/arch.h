/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/arch.h
 * @brief       This file contains Archive file definitions.
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
#ifndef __ARCH_INC
#define __ARCH_INC

#ifndef __SYS_DEP_H
#include "_sys_dep.h"
#endif

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif


/** Note that the usual '\n' in magic strings may translate to different
   characters, as allowed by ANSI.  '\012' has a fixed value, and remains
   compatible with existing BSDish archives. */

#define ARMAG  "!<arch>\012"	/**< For COFF and a.out archives */
#define ARMAGB "!<bout>\012"	/**< For b.out archives */
#define SARMAG 8
#define ARFMAG "`\012"

/** The ar_date field of the armap (__.SYMDEF) member of an archive
   must be greater than the modified date of the entire file, or
   BSD-derived linkers complain.  We originally write the ar_date with
   this offset from the real file's mod-time.  After finishing the
   file, we rewrite ar_date if it's not still greater than the mod date.  */

#define ARMAP_TIME_OFFSET       60

typedef struct tag_ar_hdr {
  tInt8  ar_magic[8];          /**< !<arch>012 */
  tInt8  ar_name[16];		/**< name of this member */
  tInt8  ar_date[12];		/**< file mtime */
  tUInt8 ar_uid[6];		/**< owner uid; printed as decimal */
  tUInt8 ar_gid[6];		/**< owner gid; printed as decimal */
  tUInt8 ar_mode[8];		/**< file mode, printed as octal   */
  tUInt8 ar_size[10];		/**< file size, printed as decimal */
  tUInt8 ar_fmag[2];		/**< should contain ARFMAG */
}ar_hdr;

typedef struct tag_ar_sub_hdr {
  tInt8  ar_name[16];		/**< name of this member */
  tInt8  ar_date[12];		/**< file mtime */
  tUInt8 ar_uid[6];		/**< owner uid; printed as decimal */
  tUInt8 ar_gid[6];		/**< owner gid; printed as decimal */
  tUInt8 ar_mode[8];		/**< file mode, printed as octal   */
  tUInt8 ar_size[10];		/**< file size, printed as decimal */
  tUInt8 ar_fmag[2];		/**< should contain ARFMAG */
}ar_sub_hdr;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif

#endif
