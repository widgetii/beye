/**
 * @namespace   libbeye
 * @file        libbeye/beyelib.h
 * @brief       This file contains extensions of standard C library, that needed
 *              for BEYE project.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
**/
#ifndef __BEYELIB_H
#define __BEYELIB_H 1

typedef enum { False = 0, True = 1 }tBool; /**< This is the data type used to represent boolean objects */
#include "libbeye/sysdep/generic/__config.h"

#include "bconfig.h"

#ifndef __NORECURSIVE
#ifndef __SYS_DEP_H
#include "libbeye/sysdep/_sys_dep.h"
#endif

#ifndef __OS_DEP_H
#include "libbeye/sysdep/__os_dep.h"
#endif

#ifndef __HRD_INF_H
#include "libbeye/sysdep/_hrd_inf.h"
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TESTFLAG(x,y) (((x) & (y)) == (y)) /**< Test y bits in x */
#define UNUSED(x) ((void)(x)) /**< Removes warning about unused arguments */

typedef int tCompare; /**< This is the data type used to represent comparition results */

                   /** Pointer to a user supplied function that compares array elements.
                     * @return                tCompare value indicated relationship between elements of array:
                                              if e1 < e2, return < 0
                                              if e1 > e2, return > 0
                                              if e1 == e2, return = 0
                     * @param e1,e2           pointers to array elements
                    **/
typedef tCompare (__FASTCALL__ *func_compare)(const void __HUGE__ *e1,const void __HUGE__ *e2);

                   /** Implements quick sort algorithm.
                     * @return                none
                     * @param base            specifies array being sorted
                     * @param num             specifies number of elements in array
                     * @param width           specifies with (in bytes) of one element of array
                     * @param fcompare        specifies pointer to user defined function
                     * @warning               After function call the original array
                     *                        is overwritten with sorted array in
                     *                        ascending order.
                     * @note                  Using own code for qsort and bsearch
                     *                        functions is guarantee of stable work
                     * @see                   HLFind HLFindNearest
                    **/
extern void  __FASTCALL__ HQSort(void __HUGE__ *base, unsigned long num, unsigned width,
                                 func_compare fcompare);

                   /** Performs a quick search on a sorted array.
                     * @return                pointer to the first matching element if found, otherwise NULL is returned
                     * @param key             pointer to the key
                     * @param base            specifies array being sorted
                     * @param nelem           specifies number of elements in array
                     * @param width           specifies with (in bytes) of one element of array
                     * @param fcompare        specifies pointer to user defined function
                     * @warning               Function can to cause infinity loop
                     *                        if array is unsorted
                     * @note                  Using own code for qsort and bsearch
                     *                        functions is guarantee of stable work
                     * @see                   HQSort HLFindNearest
                    **/
extern void __HUGE__ * __FASTCALL__ HLFind(const void *key,
                                     void __HUGE__ *base,
                                     unsigned long nelem,unsigned width,
                                     func_compare fcompare);

                   /** Performs a quick search on a sorted array of nearest element.
                     * @return                index of nearest element of array toward zero.
                     * @param key             pointer to the key
                     * @param base            specifies array being sorted
                     * @param nelem           specifies number of elements in array
                     * @param width           specifies with (in bytes) of one element of array
                     * @param fcompare        specifies pointer to user defined function
                     * @warning               Function can to cause infinity loop
                     *                        if array is unsorted
                     * @note                  Using own code for qsort and bsearch
                     *                        functions is guarantee of stable work
                     * @see                   HQSort HLFind
                    **/
extern unsigned long __FASTCALL__ HLFindNearest(const void *key,
                                     void __HUGE__ *base,
                                     unsigned long nelem,unsigned width,
                                     func_compare fcompare);

                   /** Tests wether character is a separator
                     * @return                True if given character is separator
                     * @param ch              character to be tested
                     * @note                  returns true if character is space
                     *                        or punctuator
                    **/
extern tBool  __FASTCALL__ isseparate(int ch);

/** ASCIIZ string extended support */

                   /** Removes all trailing spaces from string
                     * @return                number of removed spaces
                     * @param str             pointer to string to be trimmed
                     * @see                   szTrimLeadingSpace szKillSpaceAround
                    **/
extern int   __FASTCALL__ szTrimTrailingSpace(char *str);

                   /** Removes all leading spaces from string
                     * @return                number of removed spaces
                     * @param str             pointer to string to be trimmed
                     * @see                   szTrimTrailingSpace szKillSpaceAround
                    **/
extern int   __FASTCALL__ szTrimLeadingSpace(char *str);

                   /** Converts space into tabulation characters
                     * @return                none
                     * @param dest            pointer to string where will be placed result
                     * @param src             pointer to source string
                     * @see                   szTab2Space
                    **/
extern void  __FASTCALL__ szSpace2Tab(char *dest,const char *src);

                   /** Expands all tabulation characters with spaces
                     * @return                length of new string
                     * @param dest            pointer to string where will be placed result
                     * @param src             pointer to source string
                     * @see                   szSpace2Tab
                    **/
extern int   __FASTCALL__ szTab2Space(char *dest,const char *src);

                   /** Removes all spaces around given position
                     * @return                pointer onto next non space character
                     * @param str             pointer to string to be converted
                     * @param point_to        specifies position to be unspaced
                     * @see                   szTrimLeadingSpace szTrimTrailingSpace
                    **/
extern char *__FASTCALL__ szKillSpaceAround(char *str,char *point_to);

                   /** Prints formatted message into standard error stream
                     * @return                number of printed characters
                     * @param str             pointer to formatted string
                     * @warning               Only this function must be used
                     *                        for error reporting. (do not use
                     *                        printf, fprintf, etc. !)
                    **/
extern int  printm(const char *str,...);

/** Internal structure of Linear memory container */
typedef struct tag_linearArray
{
  unsigned long   nItems;    /**< Number of stored items */
  void __HUGE__ * data;      /**< Pointer into linear array */
  unsigned long   nSize;     /**< Size of linear array (May differ from nItems) */
  unsigned        itemSize;  /**< Size of one item in linear array */
}linearArray;

                   /** Builds linear arrays
                     * @return                pointer to builded array or NULL if error
                     * @param maxitems        specifies maximal number of item that can be stored in array. 0 - indicates dynamic change of size
                     * @param size_of_item    specifies size of each array element.
                     * @param mem_out         specifies user-defined function to be called when low-memory. May be NULL.
                     * @note                  Linear array consist from elements
                     *                        same width.
                     * @see                   la_Destroy la_IterDestroy
                    **/
extern linearArray *__FASTCALL__ la_Build( unsigned long maxitems,unsigned size_of_item,
                                           void (__FASTCALL__ *mem_out)(const char *));

                   /** Adds new element to linear array
                     * @return                location of new element or NULL if no memory
                     * @param obj             specifies linear array where new element will be stored
                     * @param data            specifies new element
                     * @param mem_out         specifies user-defined function to be called when low-memory. May be NULL.
                     * @see                   la_Build la_Find
                    **/
extern void __HUGE__*__FASTCALL__ la_AddData(linearArray *obj,const void *data,void (__FASTCALL__ *mem_out)(const char *));

                   /** Removes given element from linear array
                     * @param obj             specifies linear array where element will be removed
                     * @param idx             specifies index of element to be removed
                     * @param mem_out         specifies user-defined function to be called when low-memory. May be NULL.
                     * @see                   la_Build la_Find
                    **/
extern void __FASTCALL__          la_DeleteData(linearArray *obj,unsigned long idx);

                   /** Destroys of linear array
                     * @return                none
                     * @param obj             specifies linear array to be destroyed
                     * @warning               if elements contain pointers to
                     *                        dynamically allocated memory, then
                     *                        it will be lost
                     * @see                   la_Build la_IterDestroy
                    **/
extern void         __FASTCALL__ la_Destroy(linearArray *obj);

                   /** Destroys of linear array and calls "destructor" for each element of array.
                     * @return                none
                     * @param obj             specifies linear array to be destroyed
                     * @param del_func        specifies user-defined function, that will be used as destructor
                     * @note                  Before freeing memory of linear array
                     *                        will be called user-defined function
                     *                        with pointer onto each elemet as
                     *                        arguments
                     * @see                   la_Build la_Destroy
                    **/
extern void         __FASTCALL__ la_IterDestroy(linearArray *obj,void (__FASTCALL__ *del_func)(void __HUGE__ *));

                   /** Calls the given iterator function on each array element
                     * @return                none
                     * @param obj             specifies linear array to be destroyed
                     * @param iter_func       specifies iterator function which is to be called for each array element
                     * @see                   la_IterDestroy
                    **/
extern void         __FASTCALL__ la_ForEach(linearArray *obj,void (__FASTCALL__ *iter_func)(void __HUGE__ *));

                   /** Implements quick sort algorithm for linear array
                     * @return                none
                     * @param obj             specifies linear array to be destroyed
                     * @param fcompare        specifies pointer to user defined function
                     * @warning               After function call the original array
                     *                        is overwritten with sorted array in
                     *                        ascending order.
                     * @note                  Based on HQSort function
                     * @see                   la_Find la_FindNearest
                    **/
extern void         __FASTCALL__ la_Sort(linearArray *obj,func_compare fcompare);

                   /** Performs a quick search on a sorted linear array.
                     * @return                pointer to the first matching element if found, otherwise NULL is returned
                     * @param key             pointer to the key
                     * @param fcompare        specifies pointer to user defined function
                     * @warning               Function can to cause infinity loop
                     *                        if array is unsorted
                     * @note                  Based on HLFind function
                     *                        functions is guarantee of stable work
                     * @see                   la_Sort la_FindNearest
                    **/
extern void __HUGE__ *__FASTCALL__ la_Find(linearArray *obj,const void *key,
                                           func_compare fcompare);

                   /** Performs a quick search on a sorted linear array of nearest element.
                     * @return                index of nearest element of array toward zero.
                     * @param key             pointer to the key
                     * @param fcompare        specifies pointer to user defined function
                     * @warning               Function can to cause infinity loop
                     *                        if array is unsorted
                     * @note                  Based on HLFindNearest function
                     * @see                   HQSort HLFind
                    **/
extern unsigned long __FASTCALL__ la_FindNearest(linearArray *obj, const void *key,
                                           func_compare fcompare);

#ifdef __cplusplus
}
#endif

#endif




