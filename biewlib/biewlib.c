/**
 * @namespace   biewlib
 * @file        biewlib/biewlib.c
 * @brief       This file contains implementation of extension of C library.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1995
 * @note        Development, fixes and improvements
 * @todo        Increase number of functions
**/
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include "biewlib/sysdep/__config.h"
#if __WORDSIZE == 16
#include <mem.h>
#endif
#include "biewlib/pmalloc.h"

tBool __FASTCALL__ isseparate(int ch) { return (isspace(ch) || ispunct(ch)); }

void      __FASTCALL__ __nls_PrepareOEMForTVio(tvioBuff *it,unsigned size)
{
  unsigned i;
  unsigned char ch;
  for(i = 0;i < size;i++)
  {
    ch = it->chars[i];
    it->oem_pg[i] = NLS_IS_OEMPG(ch) ? ch : 0;
  }
  __nls_OemToOsdep(it->chars,size);
}

void __FASTCALL__ memupr(void *ptr,unsigned n)
{
   unsigned i;
   for(i = 0;i < n;i++)
   ((char *)ptr)[i] = toupper(((char *)ptr)[i]);
}

void __FASTCALL__ memlwr(void *ptr,unsigned n)
{
   unsigned i;
   for(i = 0;i < n;i++)
   ((char *)ptr)[i] = tolower(((char *)ptr)[i]);
}

int __FASTCALL__ szTrimTrailingSpace(char *str)
{
  unsigned len;
  int ret;
  len = strlen(str);
  ret = 0;
  while(len)
  {
      unsigned char ch;
      ch = str[len-1];
      if(isspace(ch) && ch < 0x80) { str[--len] = '\0'; ret++; }
      else break;
  }
  return ret;
}

int __FASTCALL__ szTrimLeadingSpace(char *str)
{
  unsigned i,freq,len;
  len = strlen(str);
  for(i = freq = 0;i < len;i++)
  {
    unsigned char ch;
    ch = str[i];
    if(isspace(ch) && ch < 0x80) freq++;
    else                         break;
  }
  if(freq)
  {
    len -= freq;
    memmove(str,&str[freq],len+1);
  }
  return freq;
}

#define TEXT_TAB 8

void __FASTCALL__ szSpace2Tab(char *dest,const char * src)
{
  unsigned int i,len,limit,dest_idx;
  int j;
  unsigned char buff[8],nspc;
  len = strlen(src);
  i = 0;
  dest_idx = 0;
  while(1)
  {
     if(i + TEXT_TAB < len)
     {
        memcpy(buff,&src[i],8);
        i+=8;
        /* scan */
        nspc = 0;
        for(j = TEXT_TAB-1;j >= 0;j--)
        {
          if(buff[j] != ' ') break;
          else nspc++;
        }
        limit = TEXT_TAB - nspc;
        memcpy(&dest[dest_idx],buff,limit);
        dest_idx += limit;
        if(nspc) dest[dest_idx++] = '\t';
     }
     else
     {
       limit = len - i;
       memcpy(&dest[dest_idx],&src[i],limit);
       dest_idx += limit;
       i += limit;
       break;
     }
  }
  dest[dest_idx] = '\0';
}

int __FASTCALL__ szTab2Space(char * dest,const char * src)
{
  int i,k,len;
  size_t size;
  unsigned int freq;
  unsigned char ch;
  len = strlen(src);
  for(freq = 0,i = k = 0;i < len;i++,freq++)
  {
    ch = src[i];
    if(ch == '\t')
    {
       size = TEXT_TAB - (freq%TEXT_TAB);
       memset(&dest[k],' ',size);
       k += size;
       freq += size-1;
    }
    else
    {
      dest[k] = ch;
      k++;
    }
  }
  return k;
}

char * __FASTCALL__ szKillSpaceAround(char *str,char *place)
{
  char *sptr;
  unsigned nmoves,len,idx,freq;
  unsigned char prev;
  unsigned char ch;
  prev = *place;
  len = strlen(str);
  *place = 0;
  idx = place - str;
  nmoves = szTrimTrailingSpace(str);
  sptr = place;
  freq = 0;
  sptr++;
  while((ch = *sptr) != 0)
  {
    if(isspace(ch)) freq++;
    else            break;
    sptr++;
  }
  memmove(&str[idx-nmoves],&str[idx+freq],len-idx+1-freq);
  str[idx-nmoves] = prev;
  return &str[idx-nmoves];
}


#if __WORDSIZE == 16
void huge * __FASTCALL__ HMemCpy(void huge *_dest, const void huge *_source, unsigned long n)
{
  long i;
  for(i = 0;i < n;i++)
  {
    ((char huge *)_dest)[i] = ((const char huge *)_source)[i];
  }
  return _dest;
}
#endif

#ifdef __GNUC__
/* (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */
char *ltoa (long value, char *string, int radix)
{
  char *dst;

  dst = string;
  if (radix < 2 || radix > 36) *dst = 0;
  else
  {
    unsigned long x;
    int i, n;
    char digits[32];
    if (radix == 10 && value < 0)
    {
      *dst++ = '-';
      x = -value;
    }
    else x = value;
    i = 0;
    do
    {
      n = x % radix;
      digits[i++] = n+(n < 10 ? '0' : 'A'-10);
      x /= radix;
    } while (x != 0);
    while (i > 0) *dst++ = digits[--i];
    *dst = 0;
  }
  return string;
}

char *ultoa (unsigned long value, char *string, int radix)
{
  char *dst;

  dst = string;
  if (radix < 2 || radix > 36) *dst = 0;
  else
  {
    int i;
    unsigned n;
    char digits[32];
    i = 0;
    do
    {
      n = value % radix;
      digits[i++] = n+(n < 10 ? '0' : 'A'-10);
      value /= radix;
    } while (value != 0);
    while (i > 0) *dst++ = digits[--i];
    *dst = 0;
  }
  return string;
}

#endif

/*
   Using own code for qsort and bsearch functions is guarantee of stable work */

/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
/* Modified for use with 16-bits huge arrays by Nick Kurshev */
/*-
 * Copyright (c) 1980, 1983 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * qsort.c:
 * Our own version of the system qsort routine which is faster by an average
 * of 25%, with lows and highs of 10% and 50%.
 * The THRESHold below is the insertion sort threshold, and has been adjusted
 * for records of size 48 bytes.
 * The MTHREShold is where we stop finding a better median.
 */

#define		THRESH		4		/**< threshold for insertion */
#define		MTHRESH		6		/**< threshold for median */

static  func_compare qcmp;                      /**< the comparison routine */
static  int		qsz;			/**< size of each record */
static  long		thresh;			/**< THRESHold in chars */
static  long		mthresh;		/**< MTHRESHold in chars */

/**
 * qst:
 * Do a quicksort
 * First, find the median element, and put that one in the first place as the
 * discriminator.  (This "median" is just the median of the first, last and
 * middle elements).  (Using this median instead of the first element is a big
 * win).  Then, the usual partitioning/swapping, followed by moving the
 * discriminator into the right place.  Then, figure out the sizes of the two
 * partions, do the smaller one recursively and the larger one via a repeat of
 * this code.  Stopping when there are less than THRESH elements in a partition
 * and cleaning up with an insertion sort (in our caller) is a huge win.
 * All data swaps are done in-line, which is space-losing but time-saving.
 * (And there are only three places where this is done).
 */
static void __NEAR__ qst(char __HUGE__ *base, char __HUGE__ *max)
{
  long ii,lo,hi;
  char __HUGE__ *i, __HUGE__ *j,__HUGE__ *jj;
  char __HUGE__ *mid, __HUGE__ *tmp;

  /*
   * At the top here, lo is the number of characters of elements in the
   * current partition.  (Which should be max - base).
   * Find the median of the first, last, and middle element and make
   * that the middle element.  Set j to largest of first and middle.
   * If max is larger than that guy, then it's that guy, else compare
   * max with loser of first and take larger.  Things are set up to
   * prefer the middle, then the first in case of ties.
   */
  lo = max - base;		/* number of elements as chars */
  do	{
    mid = i = base + qsz * ((lo / qsz) >> 1);
    if (lo >= mthresh)
    {
      j = (qcmp((jj = base), i) > 0 ? jj : i);
      if (qcmp(j, (tmp = max - qsz)) > 0)
      {
	/* switch to first loser */
	j = (j == jj ? i : jj);
	if (qcmp(j, tmp) < 0)
	  j = tmp;
      }
      if (j != i)
      {
	ii = qsz;
	do{
          __XchgB__(i,j);
          i++; j++;
	} while (--ii);
      }
    }
    /*
     * Semi-standard quicksort partitioning/swapping
     */
    for (i = base, j = max - qsz; ; )
    {
      while (i < mid && qcmp(i, mid) <= 0)
	i += qsz;
      while (j > mid)
      {
	if (qcmp(mid, j) <= 0)
	{
	  j -= qsz;
	  continue;
	}
	tmp = i + qsz;		/* value of i after swap */
	if (i == mid)
	{
	  /* j <-> mid, new mid is j */
	  mid = jj = j;
	}
	else
	{
	  /* i <-> j */
	  jj = j;
	  j -= qsz;
	}
	goto swap;
      }
      if (i == mid)
      {
	break;
      }
      else
      {
	/* i <-> mid, new mid is i */
	jj = mid;
	tmp = mid = i;		/* value of i after swap */
	j -= qsz;
      }
    swap:
      ii = qsz;
      do{
        __XchgB__(i,jj);
        i++; jj++;
      } while (--ii);
      i = tmp;
    }
    /*
     * Look at sizes of the two partitions, do the smaller
     * one first by recursion, then do the larger one by
     * making sure lo is its size, base and max are update
     * correctly, and branching back.  But only repeat
     * (recursively or by branching) if the partition is
     * of at least size THRESH.
     */
    i = (j = mid) + qsz;
    if ((lo = j - base) <= (hi = max - i))
    {
      if (lo >= thresh)
	qst(base, j);
      base = i;
      lo = hi;
    }
    else
    {
      if (hi >= thresh)
	qst(i, max);
      max = j;
    }
  } while (lo >= thresh);
}

/*
 * qsort:
 * First, set up some global parameters for qst to share.  Then, quicksort
 * with qst(), and then a cleanup insertion sort ourselves.  Sound simple?
 * It's not...
 */
void __FASTCALL__ HQSort(void __HUGE__ *base0,unsigned long num, unsigned width,
                         func_compare compare)
{
  char __HUGE__ *base = (char __HUGE__ *)base0;
  char __HUGE__ *i, __HUGE__ *j, __HUGE__ *lo, __HUGE__ *hi;
  char __HUGE__ *min, __HUGE__ *max;
  register char c;

  if (num <= 1)
    return;
  qsz = width;
  qcmp = compare;
  thresh = qsz * THRESH;
  mthresh = qsz * MTHRESH;
  max = base + num * qsz;
  if (num >= THRESH)
  {
    qst(base, max);
    hi = base + thresh;
  }
  else
  {
    hi = max;
  }
  /*
   * First put smallest element, which must be in the first THRESH, in
   * the first position as a sentinel.  This is done just by searching
   * the first THRESH elements (or the first n if n < THRESH), finding
   * the min, and swapping it into the first position.
   */
  for (j = lo = base; (lo += qsz) < hi; )
    if (qcmp(j, lo) > 0)
      j = lo;
  if (j != base)
  {
    /* swap j into place */
    for (i = base, hi = base + qsz; i < hi; )
    {
      __XchgB__(i,j);
      i++; j++;
    }
  }
  /*
   * With our sentinel in place, we now run the following hyper-fast
   * insertion sort.  For each remaining element, min, from [1] to [n-1],
   * set hi to the index of the element AFTER which this one goes.
   * Then, do the standard insertion sort shift on a character at a time
   * basis for each element in the frob.
   */
  for (min = base; (hi = min += qsz) < max; )
  {
    while (qcmp(hi -= qsz, min) > 0)
      /* void */;
    if ((hi += qsz) != min) {
      for (lo = min + qsz; --lo >= min; )
      {
	c = *lo;
	for (i = j = lo; (j -= qsz) >= hi; i = j)
	  *i = *j;
	*i = c;
      }
    }
  }
}

void __HUGE__ * __FASTCALL__ HLFind(const void *key,void __HUGE__ *base,unsigned long nelem,unsigned width,
                                    func_compare compare)
{
  unsigned long iter,start,end;
  void __HUGE__ *it;
  tCompare comp_result;
  start = 0;
  end = nelem;
  iter = nelem >> 1;
  while(1)
  {
     it = (char __HUGE__ *)base + iter*width;
     comp_result = (*compare)(key,it);
     if(!comp_result)  return it;
     if(end - start < 5) break;
     if(comp_result > 0) start = iter;
     else                end = iter;
     iter = start + ((end - start) >> 1L);
  }
  for(iter = start;iter < end;iter++)
  {
     it = (char __HUGE__ *)base + iter*width;
     if(!(*compare)(key,it)) return it;
  }
  return NULL;
}

unsigned long __FASTCALL__ HLFindNearest(const void *key,void __HUGE__ *base,unsigned long nelem,
                            unsigned width,
                            func_compare compare)
{
  unsigned long iter,start,end;
  tCompare comp_result,comp_result2;
  start = 0;
  end = nelem;
  iter = nelem >> 1;
  while(1)
  {
     comp_result = (*compare)(key,(char __HUGE__ *)base + iter*width);
     if(!comp_result) return iter;
     if(end - start < 5) break;
     if(comp_result > 0) start = iter;
     else                end = iter;
     iter = start + ((end - start)>>1L);
  }
  for(iter = start;iter < end;iter++)
  {
     comp_result = (*compare)(key,(char __HUGE__ *)base + iter*width);
     comp_result2 = iter < (nelem-1) ? (*compare)(key,(char __HUGE__ *)base + (iter+1)*width):
                               -1;
     if(comp_result >= 0 && comp_result2 < 0) return iter;
  }
  return  comp_result < 0 ? (start ? start - 1 : 0L)
                          : end == nelem ? nelem-1 : end;
}

/*
    print message when window system is not initialized

    only this function must be used for error reporting
	(do not use printf, fprintf, etc. !)
*/

int printm(const char *str,...)
{

#define _out_ stderr

    int i;
    va_list args;


    va_start(args,str);
    i = vfprintf(_out_,str,args);
    va_end(args);

    fflush(_out_);

    return i;

#undef _out_

}

linearArray * __FASTCALL__ la_Build( unsigned long nitems, unsigned size_of_item,
                        void (__FASTCALL__ *mem_out)(const char *) )
{
  linearArray * ret;
  ret = PMalloc(sizeof(linearArray));
  if(ret)
  {
    memset(ret,0,sizeof(linearArray));
    ret->itemSize = size_of_item;
    if(nitems)
    {
      ret->data = PHMalloc(nitems*size_of_item);
      if(ret->data)
      {
        ret->nSize = nitems;
      }
    }
  }
  else
  {
    if(mem_out) (*mem_out)("Creating array");
  }
  return ret;
}

void  __FASTCALL__ la_ForEach(linearArray *obj,void (__FASTCALL__ *iter_func)(void __HUGE__ *))
{
  unsigned long i;
  for(i = 0;i < obj->nItems;i++)
  {
     (*iter_func)(((char *)obj->data)+i*obj->itemSize);
  }
}

void  __FASTCALL__ la_IterDestroy(linearArray *obj,void (__FASTCALL__ *del_it)(void __HUGE__ *))
{
  la_ForEach(obj,del_it);
  PHFREE(obj->data);
  PFREE(obj);
}

void  __FASTCALL__ la_Destroy(linearArray *obj)
{
  PHFREE(obj->data);
  PFREE(obj);
}

#define LST_STEP 16

void __HUGE__*  __FASTCALL__ la_AddData(linearArray *obj,const void *udata,void (__FASTCALL__ *mem_out)(const char *))
{
  void __HUGE__*to;
  if(obj->nSize > ULONG_MAX - (LST_STEP+1)) return 0;
  if(obj->nItems + 1 > obj->nSize)
  {
    void *ptr;
    if(!obj->data) ptr = PHMalloc((obj->nSize+LST_STEP)*obj->itemSize);
    else           ptr = PHRealloc(obj->data,obj->itemSize*(obj->nSize+LST_STEP));
    if(ptr)
    {
      obj->nSize = obj->nSize+LST_STEP;
      obj->data = ptr;
    }
    else
    {
      if(mem_out) (*mem_out)("Building List");
      return NULL;
    }
  }
  to = ((char __HUGE__ *)obj->data) + obj->nItems*obj->itemSize;
  HMemCpy(to,udata,obj->itemSize);
  obj->nItems++;
  return to;
}

void         __FASTCALL__ la_Sort(linearArray *obj,func_compare compare)
{
  if(obj)
    if(obj->nItems)
      HQSort(obj->data,obj->nItems,obj->itemSize,compare);
}

void __HUGE__ *__FASTCALL__ la_Find(linearArray * obj,const void *key,
                                    func_compare compare)
{
  void __HUGE__ * ret = NULL;
  if(obj)
    if(obj->nItems)
     ret = HLFind(key,obj->data,obj->nItems,obj->itemSize,compare);
  return ret;
}

unsigned long __FASTCALL__ la_FindNearest(linearArray *obj,const void *key,
                                          func_compare compare)
{
  unsigned long ret = 0L;
  if(obj)
    if(obj->nItems)
      ret = HLFindNearest(key,obj->data,obj->nItems,obj->itemSize,compare);
  return ret;
}
