/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/le.c
 * @brief       This file contains implementation of LE (Linear Executable) file
 *              format decoder.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "plugins/disasm.h"
#include "plugins/bin/lx_le.h"
#include "bin_util.h"
#include "bmfile.h"
#include "biewhelp.h"
#include "tstrings.h"
#include "biewutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

extern BGLOBAL lx_cache;

static __filesize_t __FASTCALL__ ShowNewHeaderLE( void )
{
  return ShowNewHeaderLX();
}

static tBool __FASTCALL__ __ReadMapTblLE(BGLOBAL handle,memArray * obj,unsigned n)
{
 size_t i;
  bioSeek(handle,lxe.le.leObjectPageMapTableOffset + headshift,SEEKF_START);
  for(i = 0;i < n;i++)
  {
    LE_PAGE lep;
    char stmp[80];
    if(IsKbdTerminate() || bioEOF(handle)) break;
    bioReadBuffer(handle,&lep,sizeof(LE_PAGE));
    sprintf(stmp,"#=%08lXH Flags: %04hX = %s",(long)lep.number,lep.flags,lxeGetMapAttr(lep.flags));
    if(!ma_AddString(obj,stmp,True)) break;
  }
  return True;
}

static __filesize_t __NEAR__ __FASTCALL__ __calcPageEntryLE(LE_PAGE *mt,unsigned long idx)
{
  __filesize_t ret;
  __filesize_t dataoff;
  dataoff = idx*lxe.le.lePageSize;
  if(mt->flags == 1 || mt->flags == 5) ret = lxe.le.leObjectIterDataMapOffset + dataoff;
  else
    if(mt->flags == 0) ret = lxe.le.leDataPagesOffset + dataoff;
    else               ret = -1;
  return ret;
}

__filesize_t __FASTCALL__ CalcPageEntryLE(unsigned long pageidx)
{
  BGLOBAL handle;
  tBool found;
  unsigned i;
  LE_PAGE mt;
  if(!pageidx) return -1;
  handle = lx_cache;
  bioSeek(handle,lxe.le.leObjectPageMapTableOffset + headshift,SEEK_SET);
  found = False;
  for(i = 0;i < lxe.le.lePageCount;i++)
  {
    bioReadBuffer(handle,(void *)&mt,sizeof(LE_PAGE));
    if(bioEOF(handle)) break;
    if(mt.number == pageidx)
    {
      found = True;
      break;
    }
  }
  if(found) return __calcPageEntryLE((void *)&mt,pageidx - 1);
  else      return BMGetCurrFilePos();
}

__filesize_t __FASTCALL__ CalcEntryPointLE(unsigned long objnum,__filesize_t _offset)
{
  BGLOBAL handle;
  unsigned long i,start,pidx,j;
  __filesize_t ret,pageoff;
  LX_OBJECT lo;
  LE_PAGE mt;
  if(!objnum) return -1;
  handle = lx_cache;
  bioSeek(handle,lxe.le.leObjectTableOffset + headshift,SEEK_SET);
  bioSeek(handle,sizeof(LX_OBJECT)*(objnum - 1),SEEKF_CUR);
  bioReadBuffer(handle,(void *)&lo,sizeof(LX_OBJECT));
/*  if((lo.o32_flags & 0x00002000L) == 0x00002000L) USE16 = 0;
  else                                            USE16 = 0xFF; */
  pageoff = lxe.le.leObjectPageMapTableOffset + headshift;
  start = 0;
  ret = -1;
  for(i = 0;i < lo.o32_mapsize;i++)
  {
    tBool is_eof;
    is_eof = False;
    if(_offset >= start && _offset < start + lxe.le.lePageSize)
    {
      tBool found;
      bioSeek(handle,pageoff,SEEKF_START);
      pidx = i + lo.o32_pagemap;
      found = False;
      for(j = 0;j < lxe.le.lePageCount;j++)
      {
        bioReadBuffer(handle,(void *)&mt,sizeof(LE_PAGE));
        if((is_eof = bioEOF(handle)) != 0) break;
        if(mt.number == pidx) { found = True; break; }
      }
      if(found) ret = __calcPageEntryLE((void *)&mt,pidx - 1) + _offset - start;
      else      ret = BMGetCurrFilePos();
      break;
    }
    if(is_eof) break;
    start += lxe.le.lePageSize;
  }
  return ret;
}

__filesize_t __FASTCALL__ CalcEntryLE(const LX_ENTRY *lxent)
{
  __filesize_t ret;
  ret = BMGetCurrFilePos();
      switch(lxent->b32_type)
      {
        case 1: ret = CalcEntryPointLE(lxent->b32_obj,lxent->entry.e32_variant.e32_offset.offset16);
                      break;
        case 2: ret = CalcEntryPointLE(lxent->b32_obj,lxent->entry.e32_variant.e32_callgate.offset);
                      break;
        case 3: ret = CalcEntryPointLE(lxent->b32_obj,lxent->entry.e32_variant.e32_offset.offset32);
                      break;
        case 4: ShowFwdModOrdLX(lxent);
        case 5:
        default: break;
      }
  return ret;
}

static __filesize_t __NEAR__ __FASTCALL__ CalcEntryBungleLE(unsigned ordinal,tBool dispmsg)
{
  BGLOBAL handle;
  tBool found;
  unsigned i;
  unsigned char j;
  unsigned char cnt,type;
  tUIntFast16 numobj = 0;
  LX_ENTRY lxent;
  __filesize_t ret;
  ret = BMGetCurrFilePos();
  handle = lx_cache;
  bioSeek(handle,lxe.le.leEntryTableOffset + headshift,SEEK_SET);
  i = 0;
  found = False;
  while(1)
  {
   cnt = bioReadByte(handle);
   type = bioReadByte(handle);
   if(!cnt) break;
   if(type) numobj = bioReadWord(handle);
   for(j = 0;j < cnt;j++,i++)
   {
     char size;
     switch(type)
     {
       case 0: size = 0; break;
       case 1: size = 2; break;
       case 2:
       case 0x80:
       case 3: size = 4; break;
       default:
       case 4: size = 6; break;
     }
     if(i == ordinal - 1)
     {
       lxent.b32_type = type;
       found = True;
       if(size)
       {
         lxent.b32_obj = numobj;
         lxent.entry.e32_flags = bioReadByte(handle);
         bioReadBuffer(handle,(void *)&lxent.entry.e32_variant,size);
       }
       break;
     }
     else
       if(size) bioSeek(handle,size + sizeof(char),SEEKF_CUR);
     if(bioEOF(handle)) break;
   }
   if(found) break;
 }
 if(found) ret = CalcEntryLE((LX_ENTRY *)&lxent);
 else      if(dispmsg) ErrMessageBox(NOT_ENTRY,NULL);
 return ret;
}

static unsigned __FASTCALL__ leMapTblNumEntries(BGLOBAL handle)
{
  UNUSED(handle);
  return (unsigned)lxe.le.lePageCount;
}

static __filesize_t __FASTCALL__ ShowMapTableLE( void )
{
 __filesize_t fpos;
 int ret;
 fpos = BMGetCurrFilePos();
 ret = fmtShowList(leMapTblNumEntries,__ReadMapTblLE,
                   " Map of pages ", LB_SELECTIVE, NULL);
 if(ret != -1)
 {
   fpos = CalcPageEntryLE(ret + 1);
 }
 return fpos;
}

static __filesize_t __FASTCALL__ ShowResNamLE( void )
{
  __filesize_t fpos = BMGetCurrFilePos();
  int ret;
  unsigned ordinal;
  ret = fmtShowList(LXRNamesNumItems,LXRNamesReadItems,
                    RES_NAMES,
                    LB_SELECTIVE | LB_SORTABLE,&ordinal);
  if(ret != -1)
  {
    fpos = CalcEntryBungleLE(ordinal,True);
  }
  return fpos;
}

static __filesize_t __FASTCALL__ ShowNResNmLE( void )
{
  __filesize_t fpos;
  fpos = BMGetCurrFilePos();
  {
    int ret;
    unsigned ordinal;
    ret = fmtShowList(LXNRNamesNumItems,LXNRNamesReadItems,
                      NORES_NAMES,
                      LB_SELECTIVE | LB_SORTABLE,&ordinal);
    if(ret != -1)
    {
      fpos = CalcEntryBungleLE(ordinal,True);
    }
  }
  return fpos;
}

static tBool __FASTCALL__ isLE( void )
{
   char id[2];
   headshift = IsNewExe();
   if(headshift)
   {
     bmReadBufferEx(id,sizeof(id),headshift,SEEKF_START);
     if(id[0] == 'L' && id[1] == 'E') return True;
   }
   return False;
}

static void __FASTCALL__ LEinit( void )
{
   BGLOBAL main_handle;
   LXType = FILE_LE;
   bmReadBufferEx(&lxe.le,sizeof(LEHEADER),headshift,SEEKF_START);
   main_handle = bmbioHandle();
   if((lx_cache = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) lx_cache = main_handle;
}

static void __FASTCALL__ LEdestroy( void )
{
   BGLOBAL main_handle;
   main_handle = bmbioHandle();
   if(lx_cache != &bNull && lx_cache != main_handle) bioClose(lx_cache);
}

static __filesize_t __FASTCALL__ LEHelp( void )
{
  hlpDisplay(10004);
  return BMGetCurrFilePos();
}

static tBool __FASTCALL__ leAddressResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  tBool bret = True;
  if(cfpos >= headshift && cfpos < headshift + sizeof(LEHEADER))
  {
     strcpy(addr,"LEH :");
     strcpy(&addr[5],Get4Digit(cfpos - headshift));
  }
  else
  if(cfpos >= headshift + lxe.le.leObjectTableOffset &&
     cfpos <  headshift + lxe.le.leObjectTableOffset + sizeof(LX_OBJECT)*lxe.le.leObjectCount)
  {
     strcpy(addr,"LEOD:");
     strcpy(&addr[5],Get4Digit(cfpos - headshift - lxe.le.leObjectTableOffset));
  }
  else
  if(cfpos >= headshift + lxe.le.leObjectPageMapTableOffset &&
     cfpos <  headshift + lxe.le.leObjectPageMapTableOffset + sizeof(LE_PAGE)*lxe.le.lePageCount)
  {
    strcpy(addr,"LEPD:");
    strcpy(&addr[5],Get4Digit(cfpos - headshift - lxe.le.leObjectPageMapTableOffset));
  }
  else bret = False;
  return bret;
}

static int __FASTCALL__ lePlatform( void ) { return DISASM_CPU_IX86; }
static int __FASTCALL__ leEndian(__filesize_t off) { return DAE_LITTLE; }

REGISTRY_BIN leTable =
{
  "LE (Linear Executable)",
  { "LEHelp", "Import", "ResNam", "NRsNam", "ImpNam", "Entry ", NULL, "LEHead", "MapTbl", "Object" },
  { LEHelp, ShowModRefLX, ShowResNamLE, ShowNResNmLE, ShowImpProcLXLE, ShowEntriesLX, NULL, ShowNewHeaderLE, ShowMapTableLE, ShowObjectsLX },
  isLE, LEinit, LEdestroy,
  NULL,
  NULL,
  NULL,
  lePlatform,
  NULL,
  leEndian,
  leAddressResolv,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
