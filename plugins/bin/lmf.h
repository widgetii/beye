/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/lmf.c
 * @brief       This file contains lmf file structures and constants.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *              LMF file format header file based on Watcom C/QNX4 <sys/lmf.h>
 * @author      Andrew Golovnia
 * @since       2001
 * @note        Development, fixes and improvements
 * @todo        wc 10.6 debug information support!!! (need to use lmf.tgz)
**/
#ifndef __LMF_INC
#define __LMF_INC

#ifndef __SYS_DEP_H
#include "_sys_dep.h"
#endif

#if __WATCOMC__ > 1000
#pragma pack(__push,1);
#else
#pragma pack(1)
#endif

/*	LMF structure defenitions
 */
	
typedef struct tag_lmf_header	/* This preceeds each record defined below */
{
	tInt8 rec_type,
		zero1;
	tUInt16 data_nbytes,
		spare;
} lmf_header;

typedef struct tag_lmf_definition	/* Must be first record in load file */
{
	tUInt16 version_no,
		cflags,
		cpu,
		fpu,
		code_index,
		stack_index,
		heap_index,
		argv_index,
		zero1[4];
	tUInt32 code_offset,
		stack_nbytes,
		heap_nbytes,
		flat_offset,	/* Must be zero if not set _PCF_FLAT in cflags	(AG) */
		unmapped_size,	/* I never seen this field nonzero		(AG) */
		zero2;
	/* Variable length field of n longs starts here */
	/* Sizes of segments.      ^^^ n is segments number.		(AG) */
} lmf_definition;

typedef struct tag_lmf_data	/* Code or data record to load into memory */
{
	tUInt16 index;
	tUInt32 offset;
	/* Variable length field of n bytes starts here */
	/* Data to load in         ^^^ n is a length of loading data
	   segment numbered        n = lmf_header.data_nbytes of this record
	   by index.                   - sizeof(lmf_data).			(AG) */
} lmf_data;

typedef struct tag_lmf_resource
{
	tUInt16 resource_type;   /* 0 - usage messages */
	tUInt16 zero[3];
} lmf_resource;

/*	Record types
 */

#define _LMF_DEFINITION_REC     0
#define _LMF_COMMENT_REC        1
                            /* ^^^ Never seen this record.		(AG)  */
#define _LMF_DATA_REC           2
#define _LMF_FIXUP_SEG_REC      3
#define _LMF_FIXUP_80X87_REC    4
#define _LMF_EOF_REC            5
#define _LMF_RESOURCE_REC       6
#define _LMF_ENDDATA_REC        7
#define _LMF_FIXUP_LINEAR_REC   8
                            /* ^^^ Never seen this record.		(AG)  */
#define _LMF_PHRESOURCE			9	/* A widget resource for photon apps */
                            /* ^^^ Never seen this record.		(AG)  */

/*	Bit defitions for lh_code_flags
 */

#define _PCF_LONG_LIVED     0x0001
#define _PCF_32BIT          0x0002
#define _PCF_PRIVMASK       0x000c   /* Two bits */
#define _PCF_FLAT           0x0010
#define _PCF_NOSHARE        0x0020

/*	The top 4 bits of the segment sizes
 */

#define _LMF_CODE           0x2

#if __WATCOMC__ > 1000
#pragma pack(__pop);
#else
#pragma pack()
#endif

#endif/*__LMF_INC*/
