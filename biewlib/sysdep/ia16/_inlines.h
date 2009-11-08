/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/_inlines.h
 * @brief       This file includes 16-bit Intel architecture little inline functions.
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
**/
#ifndef __TSC__
#include "biewlib/sysdep/generic/_inlines.h"
#else
#ifndef ___INLINES_H
#define ___INLINES_H 1

#define __NEAR__  near           /**< Modifier of near call and data */
#define __FAR__   far            /**< Modifier of far call and data */
#define __HUGE__  huge           /**< Modifier of huge pointer */
#define __INTERRUPT__ interrupt  /**< Modifier of interrupt call */

/* TopSpeed's calling conventions are already "fast".
    Hence, the definition is not required. */

#define __FASTCALL__             /**< Modifier of fast call */
#define __NORETURN__             /**< Modifier of function that never return control */
#define __CONSTFUNC__            /**< Modifier of contant function */

#ifdef __clpusplus
extern "C" {
#endif

                   /** Copies given number of bytes from one huge memory block to other.
                     * @return                pointer to destinition block.
                     * @param _dest           destinition huge memory block
                     * @param _source         source huge memory block
                     * @param n               indicates number of bytes to be copied
                     * @depricated            We need for own version of function hmemcpy because in some 16-bits libraries (like TopSpeed) last argument defined as unsigned instead long.
                    **/
extern void __HUGE__ * __FASTCALL__ HMemCpy(void __HUGE__ *_dest, const void __HUGE__ *_source, unsigned long n);
#define HMemCpy HMemCpy

#pragma save, call(inline=>on,reg_param=>(ax,bx),reg_return=>(ax,bx))
                /** Changes byte order in 16-bit number */
static tUInt16 __FASTCALL__ ByteSwapS(tUInt16 val) =
{
    0x86, 0xC4  /* xchg al, ah */
};
#define ByteSwapS ByteSwapS

                /** Changes byte order in 32-bit number */
static tUInt32 __FASTCALL__ ByteSwapL(tUInt32  val) =
{
    0x86, 0xC7,  /* xchg al, bh */
    0x86, 0xDC   /* xchg bl, ah */
};
#define ByteSwapL ByteSwapL
#pragma restore

#ifndef ByteSwapLL
#define ByteSwapLL(x)\
{ union { tUInt64 __ll;		     		\
	  tUInt32 __l[2]; } __w, __r;		\
	 __w.__ll = (x);			\
	 __r.__l[0] = ByteSwapL (__w.__l[1]);	\
	 __r.__l[1] = ByteSwapL (__w.__l[0]);	\
	 __r.__ll; }
#endif


#ifdef __cplusplus
}
#endif

#endif
#endif

#define halloc halloc
#define hrealloc hrealloc
#define hfree hfree
#define HMemCpy HMemCpy

#define COREDUMP() { __asm __volatile(".short 0xffff":::"memory"); }

#undef ___INLINES_H
#include "biewlib/sysdep/generic/_inlines.h"


