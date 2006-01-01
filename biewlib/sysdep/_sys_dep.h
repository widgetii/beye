/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/_sys_dep.h
 * @brief       This file contains all development system depended part of BIEW project.
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
#ifndef __SYS_DEP_H
#define __SYS_DEP_H 1

#ifdef __TSC__
   #include "biewlib/sysdep/ia16/__config.h"
   #include "biewlib/sysdep/ia16/_inlines.h"
#else
  #if defined(__WIN32__) && defined(_MSC_VER)
    #include "biewlib/sysdep/ia32/__config.h"
    #include "biewlib/sysdep/ia32/_inlines.h"
  #else
    #define _INLINES <biewlib/sysdep/__MACHINE__/_inlines.h>
    #define __CONFIG <biewlib/sysdep/__MACHINE__/__config.h>
    #include __CONFIG
    #include _INLINES
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b)) /** Returns the greater of the two values a and b. */
#endif
#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b)) /** Returns the lesser of the two values a and b. */
#endif

                   /** Converts all alphabetic characters in buffer to upper case.
                     * @return                none
                     * @param buff            buffer to be converted
                     * @param cbBuff          size of buffer
                     * @see                   memlwr
                    **/
extern void __FASTCALL__ memupr(void *buffer,unsigned cb_buffer);

                   /** Converts all alphabetic characters in buffer to lower case.
                     * @return                none
                     * @param buff            buffer to be converted
                     * @param cbBuff          size of buffer
                     * @see                   memlwr
                    **/
extern void __FASTCALL__ memlwr(void *buffer,unsigned cb_buffer);

#if defined(__GLIBC__) || defined (__UNIX__)
#define strupr(s) memupr(s,strlen(s)) /**< C library of *nix systems lacks strupr function */
#define strlwr(s) memlwr(s,strlen(s)) /**< C library of *nix systems lacks strlwr function */
#define stricmp strcasecmp            /**< Alias of stricmp for *nix systems */
#endif
#if (defined( __GNUC__ ) && !defined(__WIN32__)) || defined(__GLIBC__)
                   /** Converts the long integer to a NULL terminated ASCII string.
                     * @return                pointer to received buffer
                     * @param buff            pointer to the buffer in which the string version of the converted number will be stored
                     * @param cbBuff          size of buffer
                     * @note                  C library of *nix systems lacks it function
                     * @see                   ultoa
                    **/
extern char *        ltoa(long _value, char *_s, int _radix);
#endif
#if defined( DJGPP ) || (defined( __GNUC__ ) && defined(__WIN32__)) || defined(__QNX4__)
#ifdef atoll
#undef atoll
#endif
#define atoll(s) strtoll(s, NULL, 10) /* temporal workaround */
#endif
#if defined( __GNUC__ ) && defined(__WIN32__)
extern long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
#endif
#ifdef __GNUC__
#define atoul atoi /**< Ugly alias. Must be removed in future. */

                   /** Converts the unsigned long integer to a NULL terminated ASCII string.
                     * @return                pointer to received buffer
                     * @param buff            pointer to the buffer in which the string version of the converted number will be stored
                     * @param cbBuff          size of buffer
                     * @note                  C library of *nix systems lacks it function
                     * @see                   ltoa
                    **/
extern char *        ultoa(unsigned long _value, char *_s, int _radix);
#endif
#if __WORDSIZE >= 32
extern char *        lltoa(long long int _value, char *_s, int _radix);
extern char *        ulltoa(unsigned long long int _value, char *_s, int _radix);
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
    #define BYTE_SWAP_L(longval)      ByteSwapL(longval)   /**< Swaps long integer if current system has little endian */
    #define BYTE_SWAP_S(shortval)     ByteSwapS(shortval)  /**< Swaps short integer if current system has little endian */
    #define BIG_BYTE_SWAP_L(longval)  (longval)
    #define BIG_BYTE_SWAP_S(shortval) (shortval)
#else
    #define BYTE_SWAP_L(longval)      (longval)
    #define BYTE_SWAP_S(shortval)     (shortval)
    #define BIG_BYTE_SWAP_L(longval)  ByteSwapL(longval)   /**< Swaps long integer if current system has big endian */
    #define BIG_BYTE_SWAP_S(shortval) ByteSwapS(shortval)  /**< Swaps short integer if current system has big endian */
#endif

#ifdef __cplusplus
}
#endif

#endif
