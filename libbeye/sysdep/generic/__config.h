/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/generic/__config.h
 * @brief       This file provides autoconfiguring project for any architecture.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       2000
 * @note        Development, fixes and improvements
 * @warning     Project needs with following macros:
 *              __WORDSIZE (16, 32, 64 e.t.c)
 *              __BYTE_ORDER (__LITTLE_ENDIAN or __BIG_ENDIAN)
 *              __intXX_t family (like tInt8 uint_least32_t e.t.c)
**/
#ifndef ____CONFIG_H
#define ____CONFIG_H

#include <sys/types.h>

// if endian present in all systems?
//#include <endian.h>
// if stdint.h present in all systems?
#include <stdint.h>

#define tInt8		int8_t
#define tInt16		int16_t
#define tInt32		int32_t
#define tInt64		int64_t
#define	tUInt8		uint8_t
#define tUInt16		uint16_t
#define tUInt32		uint32_t
#define tUInt64		uint64_t

#ifdef	__GLIBC__

#define tIntLeast8	int_least8_t
#define tIntLeast16	int_least16_t
#define	tIntLeast32	int_least32_t
#define tIntLeast64	int_least64_t
#define tUIntLeast8	uint_least8_t
#define tUIntLeast16	uint_least16_t
#define tUIntLeast32	uint_least32_t
#define tUIntLeast64	uint_least64_t

#define tIntFast8	int_fast8_t
#define tIntFast16	int_fast16_t
#define tIntFast32	int_fast32_t
#define tIntFast64	int_fast64_t
#define tUIntFast8	uint_fast8_t
#define tUIntFast16	uint_fast16_t
#define tUIntFast32	uint_fast32_t
#define tUIntFast64	uint_fast64_t

#else	/* !__GLIBC__ */

#define tIntLeast8	int8_t
#define tIntLeast16	int16_t
#define	tIntLeast32	int32_t
#define tIntLeast64	int64_t
#define tUIntLeast8	uint8_t
#define tUIntLeast16	uint16_t
#define tUIntLeast32	uint32_t
#define tUIntLeast64	uint64_t

#define tIntFast8	int8_t
#define tIntFast16	int16_t
#define tIntFast32	int32_t
#define tIntFast64	int64_t
#define tUIntFast8	uint8_t
#define tUIntFast16	uint16_t
#define tUIntFast32	uint32_t
#define tUIntFast64	uint64_t

/*
    These are usually defined in sys/types.h (machine/endian.h)

    Note that __WORDSIZE is not defined,
    which fortunately should be fine for non-16bit platforms.
*/

#ifndef	__LITTLE_ENDIAN
#define	__LITTLE_ENDIAN LITTLE_ENDIAN
#endif
#ifndef	__BIG_ENDIAN
#define	__BIG_ENDIAN BIG_ENDIAN
#endif
#ifndef	__PDP_ENDIAN
#define	__PDP_ENDIAN PDP_ENDIAN
#endif
#ifndef	__BYTE_ORDER
#define	__BYTE_ORDER BYTE_ORDER
#endif

#endif	/* __GLIBC__ */

#define tUntPtr		int_ptr_t
#define tUIntPtr	uint_ptr_t

#define tIntMax		intmax_t
#define tUIntMax	uintmax_t

#endif	/* ____CONFIG_H */
