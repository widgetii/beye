/**
 * @namespace   biew
 * @file        bin_util.c
 * @brief       This file contains common functions of plugins/bin of BIEW project.
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
**/
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "bin_util.h"
#include "reg_form.h"
#include "biewutil.h"
#include "bmfile.h"
#include "bconsole.h"
#include "tstrings.h"

unsigned fmtActiveState = 0;

linearArray *PubNames = NULL;

void __FASTCALL__ fmtSetState(int state)
{
   if(state == PS_ACTIVE) { if(fmtActiveState < UINT_MAX) fmtActiveState++; }
   else  { if(fmtActiveState) fmtActiveState--; }
}

tCompare __FASTCALL__ fmtComparePubNames(const void __HUGE__ *v1,const void __HUGE__ *v2)
{
  const struct PubName __HUGE__ *pnam1,__HUGE__ *pnam2;
  pnam1 = (const struct PubName __HUGE__ *)v1;
  pnam2 = (const struct PubName __HUGE__ *)v2;
  return __CmpLong__(pnam1->pa,pnam2->pa);
}

tBool __FASTCALL__ fmtFindPubName(BGLOBAL fmt_cache,char *buff,unsigned cb_buff,
                   __filesize_t pa,
                   ReadPubNameList fmt_readlist,
                   ReadPubName fmt_readpub)
{
  struct PubName *ret,key;
  key.pa = pa;
  if(!PubNames) (*fmt_readlist)(fmt_cache,MemOutBox);
  ret = la_Find(PubNames,&key,fmtComparePubNames);
  if(ret)
  {
    (*fmt_readpub)(fmt_cache,ret,buff,cb_buff);
    return True;
  }
  return False;
}

__filesize_t __FASTCALL__ fmtGetPubSym(BGLOBAL fmt_cache,char *str,unsigned cb_str,
                           unsigned *func_class,__filesize_t pa,tBool as_prev,
                           ReadPubNameList fmt_readlist,
                           ReadPubName fmt_readpub)
{
  __filesize_t cfpos,ret_addr,cur_addr;
  unsigned long i,idx,nitems;
  struct PubName key,*it;
  cfpos = bmGetCurrFilePos();
  if(!PubNames) (*fmt_readlist)(fmt_cache,NULL);
  if(!PubNames->nItems) return 0;
  ret_addr = 0L;
  idx = UINT_MAX;
  key.pa = pa;
  i = (unsigned)la_FindNearest(PubNames,&key,fmtComparePubNames);
  nitems = PubNames->nItems;
  if(as_prev) idx = i;
  else
  {
    static unsigned long multiref_i = 0;
    get_next:
    while((cur_addr = ((struct PubName __HUGE__ *)PubNames->data)[i].pa) <= pa)
    {
      i++;
      if((cur_addr == pa && i > multiref_i) || (i >= nitems - 1)) break;
    }
    idx = i;
    if(idx < PubNames->nItems) ret_addr = cur_addr;
    else ret_addr = 0L;
    if(ret_addr && ret_addr == pa)
    {
      if(idx <= multiref_i) { i = idx; goto get_next; }
      else multiref_i = idx;
    }
    else multiref_i = 0;
  }
  if(idx < PubNames->nItems)
  {
    ret_addr = ((struct PubName __HUGE__ *)PubNames->data)[idx].pa;
    *func_class = ((struct PubName __HUGE__ *)PubNames->data)[idx].attr;
    if(!idx && pa < ret_addr && as_prev)
    {
      ret_addr = 0;
    }
    else
    {
      it = &((struct PubName __HUGE__ *)PubNames->data)[idx];
      (*fmt_readpub)(fmt_cache,it,str,cb_str);
      str[cb_str-1] = 0;
    }
  }
  bmSeek(cfpos,BIO_SEEK_SET);
  return ret_addr;
}

static BGLOBAL __NEAR__ __FASTCALL__ ReopenSeek(__filesize_t dist)
{
 BGLOBAL handle;
 handle = bioDupEx(bmbioHandle(),BBIO_SMALL_CACHE_SIZE);
 if(handle != &bNull) bioSeek(handle,dist,BIO_SEEK_SET);
 else                 errnoMessageBox(READ_FAIL,NULL,errno);
 return handle;
}

int __FASTCALL__ fmtShowList( GetNumItems gni,ReadItems ri,const char * title,int flags,unsigned * ordinal)
{
 int ret;
 tBool bool;
 BGLOBAL handle;
 unsigned nnames;
 memArray * obj;
 TWindow* w;
 ret = -1;
 if((handle = ReopenSeek(0)) == &bNull) return ret;
 nnames = gni ? (*gni)(handle) : -1;
 if(!(obj = ma_Build(nnames,True))) goto exit;
 w = PleaseWaitWnd();
 bool = (*ri)(handle,obj,nnames);
 CloseWnd(w);
 if(bool)
 {
   if(!obj->nItems) { NotifyBox(NOT_ENTRY,title); goto exit; }
   if(flags)
   {
     ret = ma_Display(obj,title,flags,-1);
     if(ordinal && ret != -1)
     {
       char * cptr;
       char buff[40];
       cptr = strrchr(obj->data[ret],LB_ORD_DELIMITER);
       cptr++;
       strcpy(buff,cptr);
       *ordinal = atoi(buff);
     }
   }
   else    { ret = -1; ma_Display(obj,title,LB_SORTABLE,-1); }
 }
 ma_Destroy(obj);
 exit:
 bioClose(handle);
 return ret;
}
