/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/x86_64/_inlines.h
 * @brief       This file includes 64-bit AMD architecture little inline functions.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       2009
 * @note        Development, fixes and improvements
**/
#if defined(_MSC_VER) || defined(__WATCOMC__)
#define inline __inline
#endif

#if !defined(CAN_COMPILE_X86_GAS)
#include "biewlib/sysdep/generic/_inlines.h"
#else
#ifndef ___INLINES_H
#define ___INLINES_H 1

#define __NEAR__           /**< Obsolete for x86_64 platform modifier of near call and data */
#define __FAR__            /**< Obsolete for x86_64 platform modifier of far call and data */
#define __HUGE__           /**< Obsolete for x86_64 platform modifier of huge pointer */
#define __INTERRUPT__      /**< Impossible for definition with gcc modifier of interrupt call */
#define halloc malloc      /**< For x86_64 platform is alias of huge malloc */
#define hrealloc realloc   /**< For x86_64 platform is alias of huge realloc */
#define hfree free         /**< For x86_64 platform is alias of huge free */
#define HMemCpy memcpy     /**< For x86_64 platform is alias of huge memcpy */

#ifdef __ENABLE_FASTCALL
#define __FASTCALL__ __attribute__ (( __regparm__(6) )) /**< Fastcall modifier for x86_64 , Note: gcc is not ready to speedup with __stdcall__ */
#else
#define __FASTCALL__
#endif
#define __NORETURN__ __attribute__ (( __noreturn__ ))                 /**< Noreturn modifier for x86_64 */
#define __CONSTFUNC__ __attribute__ (( __const__ ))                   /**< Modifier of contant function for x86_64 */
#ifdef __clpusplus
extern "C" {
#endif

#include "biewlib/sysdep/x86_64/fastcopy.h"

                /** Changes byte order in 16-bit number */
__inline static tUInt16 __FASTCALL__ __CONSTFUNC__ ByteSwapS(tUInt16 _val)
{
  __asm("rorw $8, %w0"	:
	"=r" (_val)	:
	"0" (_val)	:
	"cc");
    return _val;
}
#define ByteSwapS ByteSwapS

                /** Changes byte order in 32-bit number */
__inline static tUInt32 __FASTCALL__ __CONSTFUNC__ ByteSwapL(tUInt32 _val)
{
 __asm("bswapl	%0":
      "=r" (_val)  :
      "0" (_val));
  return _val;
}
#define ByteSwapL ByteSwapL

                /** Changes byte order in 64-bit number */
__inline static tUInt64 __FASTCALL__ __CONSTFUNC__ ByteSwapLL(tUInt64 x)
{
 __asm("bswapq	%0":
      "=r" (x)     :
      "0" (x));
  return x;
}
#define ByteSwapLL ByteSwapLL

                /** Translates byte via table lookup
                  * @return         byte readed from table \e t at offset \e i
                  * @param t        pointer to 256-byte memory block from which will be readed byte
                  * @param i        index of memory block where byte is to be readed
                **/
__inline static tUInt8 __FASTCALL__ __CONSTFUNC__ __Xlat__(const tUInt8 *_table, tUInt8 _idx)
{
 /*
 __asm("xlatb"   :
      "=a" (_idx):
      "0"(_idx),
      "b"(_table));
  return _idx;
 */
 return _table[_idx];
}
#define __Xlat__ __Xlat__

                /** Compares two 4-byte numbers.
                  * @return         -1 if v1 < v2; +1 if v1 > v2 and 0 if v1 == v2
                  * @param _val1    specified first number to be compared
                  * @param _val2    specified second number to be compared
                  * @note           This implementation does not contain conditional
                                    jump that is being generated by gcc. It is
                                    much better for pipeline stream on modern
                                    processors. refer to: "Intel Architecture
                                    Software Developer's Manual Volume 3: System
                                    Programming" Order Number 243192. Section:
                                    "14.2.3. Eliminating and Reducing the
                                    Number of Branches" and
                                    "14.3. REDUCING PARTIAL REGISTER STALLS ON
                                     P6 FAMILY PROCESSORS".
                **/
__inline static int __FASTCALL__ __CONSTFUNC__ __CmpLong__(tUInt32 _val1, tUInt32 _val2)
{
 register int _ret;
#if __CPU__ > 586
  /* For 2 staging pipeline (PII/Celeron) this code will be ran on 1 CPU cycle
     quicker. For 3 staging (PIII/PPro) it's don't matter. P4 probably has unit
     for instruction reordering and it's don't matter also. */
 __asm("cmpl	%3, %2\n"
      "	seta	%b0\n"
      "	movl	$-1, %2\n"
      "	cmovb	%2, %0":
      "=q"(_ret)       :
      "0"(0),
      "r"(_val1),
      "g"(_val2));
#else
 __asm("cmpl	%3, %2\n"
      "	seta	%b0\n"
      "	sbbl	$0, %0":
      "=q"(_ret)       :
      "0"(0),
      "r"(_val1),
      "g"(_val2));
#endif
 return _ret;
}
#define __CmpLong__ __CmpLong__
                /** Exchanges two bytes in memory.
                  * @return         none
                  * @param _val1    specified pointer to the first byte to be exchanged
                  * @param _val2    specified pointer to the second byte to be exchanged
                  * @note           Main difference from ByteSwap function family -
                                    it is work with different number, rather than
                                    changing byte order within given number.
                 **/
__inline static void __FASTCALL__ __XchgB__(tUInt8 *_val1, tUInt8 *_val2)
{
 register char _tmp;
 __asm("xchgb	%b1,(%2)":
      "=q"(_tmp):
      "0"(*_val2),
      "r"(_val1));
  *_val2 = _tmp;
}
#define __XchgB__ __XchgB__

extern void (__FASTCALL__ *InterleaveBuffers_ptr)(tUInt32 limit,
                                    void *destbuffer,
                                    const void *evenbuffer, 
                                    const void *oddbuffer);
#ifdef InterleaveBuffers
#undef InterleaveBuffers
#endif
#define InterleaveBuffers(a,b,c,d) (*InterleaveBuffers_ptr)(a,b,c,d)
#define __INTERLEAVE_BUFFERS InterleaveBuffers


extern void (__FASTCALL__ *CharsToShorts_ptr)(tUInt32 limit,
                                             void *destbuffer,
                                             const void *evenbuffer);
#ifdef CharsToShorts
#undef CharsToShorts
#endif
#define CharsToShorts(a,b,c) (*CharsToShorts_ptr)(a,b,c)
#define __CHARS_TO_SHORTS CharsToShorts

extern void (__FASTCALL__ *ShortsToChars_ptr)(tUInt32 limit,
                                     void * destbuffer, const void * srcbuffer);

#ifdef ShortsToChars
#undef ShortsToChars
#endif
#define ShortsToChars(a,b,c) (*ShortsToChars_ptr)(a,b,c)
#define __SHORTS_TO_CHARS ShortsToChars

#define COREDUMP() { __asm__ __volatile__(".short 0xffff":::"memory"); }

#endif
#endif
#undef ___INLINES_H
#include "biewlib/sysdep/generic/_inlines.h"