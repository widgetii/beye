/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/generic/_inlines.h
 * @brief       This file includes generic architecture little inline functions.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       2000
 * @note        Development, fixes and improvements
**/
#ifndef ___INLINES_H
#define ___INLINES_H 1

#ifdef __clpusplus
extern "C" {
#endif

#ifndef __NEAR__
#define __NEAR__           /**< Not usable for generic platform modifier of near call and data */
#endif
#ifndef __FAR__
#define __FAR__            /**< Not usable for generic platform modifier of far call and data */
#endif
#ifndef __HUGE__
#define __HUGE__           /**< Not usable for generic platform modifier of huge pointer */
#endif
#ifndef __INTERRUPT__
#define __INTERRUPT__      /**< Not usable for generic platform modifier of interrupt call */
#endif
#ifndef halloc
#define halloc malloc      /**< For generic platform is alias of huge malloc */
#endif
#ifndef hrealloc
#define hrealloc realloc   /**< For generic platform is alias of huge realloc */
#endif
#ifndef hfree
#define hfree free         /**< For generic platform is alias of huge free */
#endif
#ifndef HMemCpy
#define HMemCpy memcpy     /**< For generic platform is alias of huge memcpy */
#endif

#ifndef __FASTCALL__
#define __FASTCALL__       /**< Impossible for definition on generic platform modifier of fast call */
#endif
#ifndef __NORETURN__
#define __NORETURN__       /**< Impossible for definition on generic platform modifier of function that never return contol */
#endif
#ifndef __CONSTFUNC__
#define __CONSTFUNC__      /**< Impossible for definition on generic platform modifier of constant function */
#endif

                /** Changes byte order in 32-bit number */
#ifndef ByteSwapL
#define ByteSwapL(_val)\
  (((tUInt32)_val << 24) | (((tUInt32)_val & 0xFF00) << 8) |\
  (((tUInt32)_val & 0xFF0000L) >> 8) | ((tUInt32)_val >> 24))
#endif
                /** Changes byte order in 16-bit number */
#ifndef ByteSwapS
#define ByteSwapS(_val)\
  (((tUInt16)_val << 8) | ((tUInt16)_val >> 8))
#endif

                /** Changes byte order in 64-bit number */
#ifndef ByteSwapLL
__inline static tUInt64 __ByteSwapLL(tUInt64 x)\
{ union { tUInt64 __ll;		     		\
	  tUInt32 __l[2]; } __w, __r;		\
	 __w.__ll = (x);			\
	 __r.__l[0] = ByteSwapL (__w.__l[1]);	\
	 __r.__l[1] = ByteSwapL (__w.__l[0]);	\
	 return __r.__ll; }
#define ByteSwapLL(x) __ByteSwapLL(x)
#endif

                /** Translates byte via table lookup
                  * @return         byte readed from table \e t at offset \e i
                  * @param t        pointer to 256-byte memory block from which will be readed byte
                  * @param i        index of memory block where byte is to be readed
                **/
#ifndef __Xlat__
#define __Xlat__(t,i) (t[i])
#endif
                /** Compares two 4-byte numbers.
                  * @return         -1 if v1 < v2; +1 if v1 > v2 and 0 if v1 == v2
                  * @param _val1    specified first number to be compared
                  * @param _val2    specified second number to be compared
                **/
#ifndef __CmpLong__
#define __CmpLong__(_val1,_val2)\
        ((_val1) < (_val2) ? -1 : (_val1) > (_val2) ? 1 : 0)
#endif

                /** Exchanges two bytes in memory.
                  * @return         none
                  * @param _val1    specified pointer to the first byte to be exchanged
                  * @param _val2    specified pointer to the second byte to be exchanged
                  * @note           Main difference from ByteSwap function family -
                                    it is work with different number, rather than
                                    changing byte order within given number.
                 **/
#ifndef __XchgB__
#define __XchgB__(_val1,_val2)\
    {\
      register tUInt8 _charv;\
      _charv = *((tUInt8 *)_val2);\
      *((tUInt8 *)_val2) = *((tUInt8 *)_val1);\
      *((tUInt8 *)_val1) = _charv;\
    }
#endif

                /** Performs interleaving of two buffers into destinition one.
                  * @return         none
                  * @param limit    specified size of evenbuffer and oddbuffer
                  * @param destbuffer specified pointer to the destinition buffer
                                    where result will be placed.
                  * @param evenbuffer specified source buffer with even bytes.
                  * @param offbuffer specified source buffer with odd bytes.
                 **/
#ifndef __INTERLEAVE_BUFFERS
#define __INTERLEAVE_BUFFERS(limit, destbuffer, evenbuffer, oddbuffer)\
{\
  register size_t freq;\
  for(freq=0;freq<(size_t)limit;freq++)\
  {\
    ((char *)destbuffer)[freq+freq] = ((char *)evenbuffer)[freq];\
    ((char *)destbuffer)[freq+freq+1] = ((char *)oddbuffer)[freq];\
  }\
}
#endif

                /** Performs conversation string of characters to zero extended
                    string of short values.
                  * @return         none
                  * @param limit    specified size of evenbuffer and oddbuffer
                  * @param destbuffer specified pointer to the destinition buffer
                                    where result will be placed.
                  * @param evenbuffer specified source buffer with even bytes.
                  * @param zerofiller specified pointer to zero filled memory,
                                    which must have size of MMREG_SIZE.
                 **/
#ifndef __CHARS_TO_SHORTS
#define __CHARS_TO_SHORTS(limit, destbuffer, evenbuffer)\
{\
  register size_t freq;\
  for(freq=0;freq<(size_t)limit;freq++)\
  {\
    ((char *)destbuffer)[freq+freq] = ((char *)evenbuffer)[freq];\
    ((char *)destbuffer)[freq+freq+1] = 0;\
  }\
}
#endif

                /** Performs conversation string of zero extended short values
                    to string of characters.
                  * @return         none
                  * @param limit    specified size of evenbuffer and oddbuffer
                  * @param destbuffer specified pointer to the destinition buffer
                                    where result will be placed.
                  * @param srcbuffer specified source buffer to be converted.
                 **/
#ifndef __SHORTS_TO_CHARS
#define __SHORTS_TO_CHARS(limit, destbuffer, srcbuffer)\
{\
  register size_t freq;\
  for(freq=0;freq<(size_t)limit;freq++)\
  {\
    ((char *)destbuffer)[freq] = ((char *)srcbuffer)[freq+freq];\
  }\
}
#endif

#ifdef __cplusplus
}
#endif

#endif
