/**
 * @namespace   biewlib
 * @file        biewlib/pmalloc.c
 * @brief       This file contains implementation of preemptive memory allocation.
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
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "biewlib/sysdep/__config.h"
#if __WORDSIZE == 16
#include <malloc.h>
#endif
#include "biewlib/pmalloc.h"

#if !defined( NDEBUG ) && __WORDSIZE == 16 && defined(__TSC__)
/* 16-bits protected mode it's only way of quality stress test of code */
#include <tsxlib.h>
#include <dos.h>
#define malloc(s)       (MK_FP(AllocSeg(s),0))
#define realloc(p,s)    (ReallocSeg(s,FP_SEG(p)) ? p : 0)
#define free(p)         (FreeSeg(FP_SEG(p)))
extern  unsigned        _MaxReallocHugeSegments;
#ifdef halloc
#undef halloc
#endif
#define halloc(s)       (MK_FP(HugeAllocSeg((unsigned)s,(unsigned)(s>>16),_MaxReallocHugeSegments),0))
#ifdef hrealloc
#undef hrealloc
#endif
#define hrealloc(p,s)   (HugeReallocSeg((unsigned)s,(unsigned)(s>>16),FP_SEG(p)) ? p : 0)
#ifdef hfree
#undef hfree
#endif
#define hfree(p)        (FreeSeg(FP_SEG(p)))
#endif
static LowMemCallBack *lmstack = NULL;
unsigned               lmcount = 0;

tBool    __FASTCALL__ PMRegLowMemCallBack(LowMemCallBack func)
{
  void *new_ptr;
  tBool ret = False;
  if(lmcount < UINT_MAX)
  {
    if(!lmstack) new_ptr = malloc(sizeof(LowMemCallBack));
    else         new_ptr = realloc(lmstack,(lmcount+1)*sizeof(LowMemCallBack));
    if(!new_ptr) return 0;
    lmstack = new_ptr;
    lmstack[lmcount++] = func;
    ret = True;
  }
  return ret;
}

tBool  __FASTCALL__ PMUnregLowMemCallBack(LowMemCallBack func)
{
  void *new_ptr;
  unsigned i,fidx;
  tBool ret;
  fidx = UINT_MAX;
  for(i = 0;i < lmcount;i++)
  {
    if(lmstack[i] == func)
    {
      fidx = i;
      break;
    }
  }
  ret = False;
  if(fidx != UINT_MAX)
  {
    memmove(&lmstack[fidx],&lmstack[fidx+1],lmcount-(fidx+1));
    new_ptr = realloc(lmstack,(lmcount--)*sizeof(LowMemCallBack));
    if(new_ptr) lmstack = new_ptr;
    ret = True;
  }
  return ret;
}

void *           __FASTCALL__ PMalloc(size_t obj_size)
{
  void *ret;
  unsigned i;
  ret = malloc(obj_size);
  if(!ret)
  {
    for(i = 0;i < lmcount;i++)
    {
      if(lmstack[i](obj_size))
      {
        ret = malloc(obj_size);
        if(ret) break;
      }
    }
  }
  return ret;
}

void *           __FASTCALL__ PRealloc(void *ptr,size_t obj_size)
{
  void *ret;
  unsigned i;
  ret = realloc(ptr,obj_size);
  if(!ret)
  {
    for(i = 0;i < lmcount;i++)
    {
      if(lmstack[i](obj_size))
      {
        ret = realloc(ptr,obj_size);
        if(ret) break;
      }
    }
  }
  return ret;
}

void             __FASTCALL__ PFree(void *ptr)
{
  free(ptr);
}

#if __WORDSIZE == 16
void __HUGE__ *  __FASTCALL__ PHMalloc(unsigned long obj_size)
{
  void __HUGE__ *ret;
  unsigned i;
  ret = halloc(obj_size);
  if(!ret)
  {
    for(i = 0;i < lmcount;i++)
    {
      if(lmstack[i](obj_size))
      {
        ret = halloc(obj_size);
        if(ret) break;
      }
    }
  }
  return ret;
}

void __HUGE__ *  __FASTCALL__ PHRealloc(void __HUGE__ *ptr,unsigned long obj_size)
{
  void __HUGE__ *ret;
  unsigned i;
  ret = hrealloc(ptr,obj_size);
  if(!ret)
  {
    for(i = 0;i < lmcount;i++)
    {
      if(lmstack[i](obj_size))
      {
        ret = hrealloc(ptr,obj_size);
        if(ret) break;
      }
    }
  }
  return ret;
}

void             __FASTCALL__ PHFree(void __HUGE__ * ptr)
{
  hfree(ptr);
}
#else
void __HUGE__ *  __FASTCALL__ PHMalloc(unsigned long obj_size)
{
  return PMalloc(obj_size);
}

void __HUGE__ *  __FASTCALL__ PHRealloc(void __HUGE__ *ptr,unsigned long obj_size)
{
  return PRealloc(ptr,obj_size);
}

void             __FASTCALL__ PHFree(void __HUGE__ * ptr)
{
  PFree(ptr);
}
#endif
