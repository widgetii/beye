/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/stdint.h
 * @brief       ISO C 9X: 7.18 Integer types <stdint.h>
 * @version     7.18
 * @remark      Copyright (C) 1997, 1998, 1999 Free Software Foundation, Inc.
 *              This file is part of the GNU C Library.
 *              The GNU C Library is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU Library General Public License as
 *              published by the Free Software Foundation; either version 2 of the
 *              License, or (at your option) any later version.
 *              The GNU C Library is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *              Library General Public License for more details.
 *              You should have received a copy of the GNU Library General Public
 *              License along with the GNU C Library; see the file COPYING.LIB.  If not,
 *              write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *              Boston, MA 02111-1307, USA.
 * @note        Requires POSIX compatible development system
 *
 * @author      GNU FSF
 * @since       1997
**/
#ifndef _STDINT_H
#define _STDINT_H	1

/** It is possible to compile containing GCC extensions even if GCC is
    run in pedantic mode if the uses are carefully marked using the
    `__extension__' keyword.  But this is not generally available before
    version 2.8.  */
#if !defined __GNUC__ || __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8)
# define __extension__		/**< Ignore */
#endif

#define __WORDSIZE 16

/** Exact integral types.  */

/** Signed.  */

/** There is some amount of overlap with <sys/types.h> as known by inet code */
typedef signed char		tInt8;
typedef signed short int	tInt16;
typedef signed long int		tInt32;
#ifdef __GNUC__
__extension__
typedef signed long long int	tInt64;
#else
# ifdef __WATCOMC__
typedef __int64			tInt64;
# endif
#endif

/** Unsigned.  */
typedef unsigned char		tUInt8;
typedef unsigned short int	tUInt16;
typedef unsigned long int	tUInt32;
#ifdef __GNUC__
__extension__
typedef unsigned long long int	tUInt64;
#else
# ifdef __WATCOMC__
typedef unsigned __int64	tUInt64;
# endif
#endif


/** Small types.  */

/** Signed.  */
typedef signed char		tIntLeast8;
typedef signed short int	tIntLeast16;
typedef signed long int		tIntLeast32;
#ifdef __GNUC__
__extension__
typedef signed long long int	tIntLeast64;
#else
# ifdef __WATCOMC__
typedef __int64			tIntLeast64;
# endif
#endif

/** Unsigned.  */
typedef unsigned char		tUIntLeast8;
typedef unsigned short int	tUIntLeast16;
typedef unsigned long int	tUIntLeast32;
#ifdef __GNUC__
__extension__
typedef unsigned long long int	tUIntLeast64;
#else
# ifdef __WATCOMC__
typedef unsigned __int64	tUIntLeast64;
# endif
#endif


/** Fast types.  */

/** Signed.  */
typedef signed char		tIntFast8;
typedef signed int		tIntFast16;
typedef signed long int		tIntFast32;
#ifdef __GNUC__
__extension__
typedef signed long long int	tIntFast64;
#else
# ifdef __WATCOMC__
typedef __int64			tIntFast64;
# endif
#endif

/** Unsigned.  */
typedef unsigned char		tUIntFast8;
typedef unsigned int		tUIntFast16;
typedef unsigned long int	tUIntFast32;
#ifdef __GNUC__
__extension__
typedef unsigned long long int	tUIntFast64;
#else
# ifdef __WATCOMC__
typedef unsigned __int64	tUIntFast64;
# endif
#endif


/** Types for `void *' pointers.  */
#if defined( M_I86SM ) || defined( M_I86CM)
typedef signed int		tIntPtr;
#else
typedef long int		tIntPtr;
#endif
#if defined( M_I86SM ) || defined( M_I86CM)
typedef unsigned int		tUIntPtr;
#else
typedef unsigned long int	tUIntPtr;
#endif

/** Largest integral types.  */
#ifdef __GNUC__
__extension__
typedef signed long long int	tIntMax;
__extension__
typedef unsigned long long int	tUIntMax;
#else
#ifdef __WATCOMC__
typedef __int64			tIntMax;
typedef unsigned __int64	tUIntMax;
#else
typedef signed long int		tIntMax;
typedef unsigned long int	tUIntMax;
#endif
#endif

/** The ISO C 9X standard specifies that in C++ implementations these
    macros should only be defined if explicitly requested.  */
#if !defined __cplusplus || defined __STDC_LIMIT_MACROS

#ifdef __GNUC__
#define __INT64_C(c)	c ## LL
#define __UINT64_C(c)	c ## ULL
#else /* Watcom C */
#define __INT64_C(c)	c ## i64
#define __UINT64_C(c)	c ## Ui64
#endif

/** Limits of integral types.  */

/** Minimum of signed integral types.  */
# define INT8_MIN		(-128)
# define INT16_MIN		(-32767-1)
# define INT32_MIN		(-2147483647L-1)
#if defined( __GNUC__ ) || defined( __WATCOMC__ )
# define INT64_MIN		(-__INT64_C(9223372036854775807)-1)
#endif
/** Maximum of signed integral types.  */
# define INT8_MAX		(127)
# define INT16_MAX		(32767)
# define INT32_MAX		(2147483647L)
#if defined( __GNUC__ ) || defined( __WATCOMC__ )
# define INT64_MAX		(__INT64_C(9223372036854775807))
#endif

/** Maximum of unsigned integral types.  */
# define UINT8_MAX		(255U)
# define UINT16_MAX		(65535U)
# define UINT32_MAX		(4294967295UL)
#if defined( __GNUC__ ) || defined( __WATCOMC__ )
# define UINT64_MAX		(__UINT64_C(18446744073709551615))
#endif

/** Minimum of signed integral types having a minimum size.  */
# define INT_LEAST8_MIN		(-128)
# define INT_LEAST16_MIN	(-32767-1)
# define INT_LEAST32_MIN	(-2147483647L-1)
#if defined( __GNUC__ ) || defined( __WATCOMC__ )
# define INT_LEAST64_MIN	(-__INT64_C(9223372036854775807)-1)
#endif
/** Maximum of signed integral types having a minimum size.  */
# define INT_LEAST8_MAX		(127)
# define INT_LEAST16_MAX	(32767)
# define INT_LEAST32_MAX	(2147483647L)
#if defined( __GNUC__ ) || defined( __WATCOMC__ )
# define INT_LEAST64_MAX	(__INT64_C(9223372036854775807))
#endif

/** Maximum of unsigned integral types having a minimum size.  */
# define UINT_LEAST8_MAX	(255U)
# define UINT_LEAST16_MAX	(65535U)
# define UINT_LEAST32_MAX	(4294967295UL)
#if defined( __GNUC__ ) || defined( __WATCOMC__ )
# define UINT_LEAST64_MAX	(__UINT64_C(18446744073709551615))
#endif

/** Minimum of fast signed integral types having a minimum size.  */
# define INT_FAST8_MIN		(-128)
# define INT_FAST16_MIN		(-32767-1)
# define INT_FAST32_MIN		(-2147483647L-1)
#if defined( __GNUC__ ) || defined( __WATCOMC__ )
# define INT_FAST64_MIN		(-__INT64_C(9223372036854775807)-1)
#endif
/** Maximum of fast signed integral types having a minimum size.  */
# define INT_FAST8_MAX		(127)
# define INT_FAST16_MAX		(32767)
# define INT_FAST32_MAX		(2147483647L)
#if defined( __GNUC__ ) || defined( __WATCOMC__ )
# define INT_FAST64_MAX		(__INT64_C(9223372036854775807))
#endif

/** Maximum of fast unsigned integral types having a minimum size.  */
# define UINT_FAST8_MAX		(255U)
# define UINT_FAST16_MAX	(65535U)
# define UINT_FAST32_MAX	(4294967295UL)
#if defined( __GNUC__ ) || defined( __WATCOMC__ )
# define UINT_FAST64_MAX	(__UINT64_C(18446744073709551615))
#endif


/** Values to test for integral types holding `void *' pointer.  */
# define INTPTR_MIN		(-2147483647L-1)
# define INTPTR_MAX		(2147483647L)
# define UINTPTR_MAX		(4294967295UL)


#if defined( __GNUC__ ) || defined( __WATCOMC__ )
/* Minimum for largest signed integral type.  */
# define INTMAX_MIN		(-__INT64_C(9223372036854775807)-1)
/** Maximum for largest signed integral type.  */
# define INTMAX_MAX		(__INT64_C(9223372036854775807))
/** Maximum for largest unsigned integral type.  */
# define UINTMAX_MAX		(__UINT64_C(18446744073709551615))
#else
/** Minimum for largest signed integral type.  */
# define INTMAX_MIN		(-2147483647L-1)
/** Maximum for largest signed integral type.  */
# define INTMAX_MAX		(2147483647L)
/** Maximum for largest unsigned integral type.  */
# define UINTMAX_MAX		(4294967295UL)
#endif

/** Limits of other integer types.  */
#if defined(M_I86SM) || defined(M_I86CM)
/** Limits of `ptrdiff_t' type.  */
# define PTRDIFF_MIN		(-32767-1)
# define PTRDIFF_MAX		(32767)

/** Limits of `sig_atomic_t'.  */
# define SIG_ATOMIC_MIN		(-32767-1)
# define SIG_ATOMIC_MAX		(32767)
#else
/** Limits of `ptrdiff_t' type.  */
# define PTRDIFF_MIN		(-2147483647L-1)
# define PTRDIFF_MAX		(2147483647L)

/** Limits of `sig_atomic_t'.  */
# define SIG_ATOMIC_MIN		(-2147483647L-1)
# define SIG_ATOMIC_MAX		(2147483647L)
#endif
/** Limit of `size_t' type.  */
# define SIZE_MAX		(65535U)

/** Limits of `wchar_t'.  */
# ifndef WCHAR_MIN
/** These constants might also be defined in <wchar.h>.  */
#  define WCHAR_MIN		(-32767-1)
#  define WCHAR_MAX		(32767)
# endif

/** Limits of `wint_t'.  */
#ifndef WINT_MIN
# define WINT_MIN		(0)
# define WINT_MAX		(65535U)
#endif
#endif	/** C++ && limit macros */


/** The ISO C 9X standard specifies that in C++ implementations these
   should only be defined if explicitly requested.  */
#if !defined __cplusplus || defined __STDC_CONSTANT_MACROS

/** Signed.  */
# define INT8_C(c)	c
# define INT16_C(c)	c
# define INT32_C(c)	c ## L
#ifdef __GNUC__
# define INT64_C(c)	c ## LL
#else
#ifdef __WATCOMC__
# define INT64_C(c)	c ## i64
#endif
#endif

/** Unsigned.  */
# define UINT8_C(c)	c ## U
# define UINT16_C(c)	c ## U
# define UINT32_C(c)	c ## UL
#ifdef __GNUC__
# define UINT64_C(c)	c ## ULL
/** Maximal type.  */
# define INTMAX_C(c)	c ## LL
# define UINTMAX_C(c)	c ## ULL
#else
#ifdef __WATCOMC__
# define UINT64_C(c)	c ## Ui64
/** Maximal type.  */
# define INTMAX_C(c)	c ## i64
# define UINTMAX_C(c)	c ## Ui64
#endif
#endif

#endif	/** C++ && constant macros */

#endif /* stdint.h */
