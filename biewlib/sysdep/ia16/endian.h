/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/endian.h
 * @brief       Definitions for byte order, according to significance of bytes,
 *              from low addresses to high addresses.  The value is what you get
 *              by putting '4' in the most significant byte, '3' in the second
 *              most significant byte, '2' in the second least significant byte,
 *              and '1' in the least significant byte.
 * @version     -
 * @remark      Copyright (C) 1992, 1996, 1997 Free Software Foundation, Inc.
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
 * @since       1992
**/
#ifndef	_ENDIAN_H
#define	_ENDIAN_H	1

#define	__LITTLE_ENDIAN	1234
#define	__BIG_ENDIAN	4321
#define	__PDP_ENDIAN	3412

#define __BYTE_ORDER __LITTLE_ENDIAN

/* Some machines may need to use a different endianness for floating point
    values.  */
#ifndef __FLOAT_WORD_ORDER
# define __FLOAT_WORD_ORDER __BYTE_ORDER
#endif

#ifdef	__USE_BSD
# define LITTLE_ENDIAN	__LITTLE_ENDIAN
# define BIG_ENDIAN	__BIG_ENDIAN
# define PDP_ENDIAN	__PDP_ENDIAN
# define BYTE_ORDER	__BYTE_ORDER
#endif

#endif	/* endian.h */
