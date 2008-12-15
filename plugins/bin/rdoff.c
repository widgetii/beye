/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/rdoff.c
 * @brief       This file contains implementation of decoder for RDOFF v1 file format.
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
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "reg_form.h"
#include "colorset.h"
#include "bin_util.h"
#include "bmfile.h"
#include "codeguid.h"
#include "biewhelp.h"
#include "biewutil.h"
#include "bconsole.h"
#include "tstrings.h"
#include "plugins/disasm.h"
#include "plugins/bin/rdoff.h"
#include "biewlib/pmalloc.h"
#include "biewlib/kbd_code.h"

struct rdoff_ImpName
{
  unsigned short lsegno;
  unsigned short reserv;
  __filesize_t nameoff;
};

static unsigned long rdoff_hdrlen,ds_len;
static __filesize_t cs_start,ds_start;

static linearArray *rdoffReloc = NULL;
static linearArray *rdoffImpNames = NULL;

static void __NEAR__ __FASTCALL__ ReadImpNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *));
static void __FASTCALL__ rdoff_ReadPubNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *));
static tBool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa);

static tCompare __FASTCALL__ compare_impnames(const void __HUGE__ *v1,const void __HUGE__ *v2);

static tBool __FASTCALL__ rdoffLowMemFunc( unsigned long need_mem )
{
  tBool ret = False;
  UNUSED(need_mem);
  if(!fmtActiveState)
  {
    if(PubNames)
    {
       la_Destroy(PubNames);
       PubNames = NULL;
       ret = True;
    }
    if(rdoffImpNames)
    {
       la_Destroy(rdoffImpNames);
       rdoffImpNames = NULL;
       ret = True;
    }
    if(rdoffReloc)
    {
       la_Destroy(rdoffReloc);
       rdoffReloc = NULL;
       ret = True;
    }
  }
  return ret;
}

static __filesize_t __FASTCALL__ rdoff_Help( void )
{
  hlpDisplay(10011);
  return BMGetCurrFilePos();
}

/** return 0 if error */
static tBool __NEAR__ __FASTCALL__ rdoff_skiprec(unsigned char type)
{
  unsigned i;
  tBool ret;
  unsigned char nulch;
  ret = True;
  switch(type)
  {
    case 1: /** reloc record */
            bmSeek(8,BM_SEEK_CUR);
            break;
    case 2: /** import symbol */
            bmSeek(2,BM_SEEK_CUR);
            for(i = 0;i < 32;i++)
            {
              nulch = bmReadByte();
              if(!nulch) break;
            }
            break;
    case 3: /** export symbol */
            bmSeek(5,BM_SEEK_CUR);
            for(i = 0;i < 32;i++)
            {
              nulch = bmReadByte();
              if(!nulch) break;
            }
            break;
    case 4: /** import library */
            for(i = 0;i < 128;i++)
            {
              nulch = bmReadByte();
              if(!nulch) break;
            }
            break;
    case 5: /** reserve bss */
            bmSeek(4,BM_SEEK_CUR);
            break;
    default: /** unknown ??? */
            ErrMessageBox("Broken RDOFF file",NULL);
            ret = False;
            break;
  }
  return ret;
}

static __filesize_t __FASTCALL__ rdoff_ShowExport( void )
{
  __filesize_t fpos;
  unsigned char rec;
  unsigned i;
  char str[33],sout[256];
  unsigned char segno;
  __filesize_t segoff;
  __filesize_t abs_off;
  memArray * rdoff_et;
  fpos = BMGetCurrFilePos();
  if(!(rdoff_et = ma_Build(0,True))) return fpos;
  bmSeek(10,BM_SEEK_SET);
  while(bmGetCurrFilePos() < rdoff_hdrlen + 5)
  {
    tBool is_eof;
    rec = bmReadByte();
    is_eof = False;
    if(rec == 3)
    {
      char ch;
      segno = bmReadByte();
      segoff = bmReadDWord();
      for(i = 0;i < 32;i++)
      {
        ch = bmReadByte();
        str[i] = ch;
        is_eof = bmEOF();
        if(!ch || is_eof) break;
      }
      str[i] = 0;
      abs_off = segno == 0 ? cs_start : segno == 1 ? ds_start : FILESIZE_MAX;
      if(abs_off < FILESIZE_MAX) abs_off += segoff;
      sprintf(sout,"%-50s offset=%08lXH",str,(unsigned long)abs_off);
      if(!ma_AddString(rdoff_et,sout,True) || is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) goto exit;
      if(bmEOF()) break;
    }
  }
  if(rdoff_et->nItems)
  {
    int ret;
    ret = ma_Display(rdoff_et,EXP_TABLE,LB_SELECTIVE,0);
    if(ret != -1)
    {
      char *rets;
      rets = ((char **)rdoff_et->data)[ret];
      rets = strstr(rets,"offset=");
      if(rets) fpos = strtoul(&rets[7],NULL,16);
    }
  }
  else                 NotifyBox(NOT_ENTRY,EXP_TABLE);
  exit:
  ma_Destroy(rdoff_et);
  return fpos;
}

static __filesize_t __FASTCALL__ rdoff_FindExport(const char *name )
{
  __filesize_t ret;
  unsigned char rec;
  unsigned i;
  char str[33];
  unsigned char segno;
  __filesize_t segoff;
  __filesize_t abs_off;
  bmSeek(10,BM_SEEK_SET);
  ret = 0L;
  while(bmGetCurrFilePos() < rdoff_hdrlen + 5)
  {
    tBool is_eof;
    rec = bmReadByte();
    is_eof = False;
    if(rec == 3)
    {
      char ch;
      segno = bmReadByte();
      segoff = bmReadDWord();
      for(i = 0;i < 32;i++)
      {
        ch = bmReadByte();
        is_eof = bmEOF();
        str[i] = ch;
        if(!ch || is_eof) break;
      }
      str[i] = 0;
      if(strcmp(str,name) == 0)
      {
        abs_off = segno == 0 ? cs_start : segno == 1 ? ds_start : 0L;
        if(abs_off != 0L) abs_off += segoff;
        ret = abs_off;
        break;
      }
      if(is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) break;
      if(bmEOF()) break;
    }
  }
  return ret;
}

static __filesize_t __FASTCALL__ rdoff_ShowModRef( void )
{
  __filesize_t fpos;
  unsigned char rec;
  unsigned i;
  char str[129];
  memArray * rdoff_mr;
  fpos = BMGetCurrFilePos();
  if(!(rdoff_mr = ma_Build(0,True))) return fpos;
  bmSeek(10,BM_SEEK_SET);
  while(bmGetCurrFilePos() < rdoff_hdrlen + 5)
  {
    tBool is_eof;
    rec = bmReadByte();
    is_eof = False;
    if(rec == 4)
    {
      char ch;
      for(i = 0;i < 128;i++)
      {
        ch = bmReadByte();
        is_eof = bmEOF();
        str[i] = ch;
        if(!ch || is_eof) break;
      }
      str[i] = 0;
      if(!ma_AddString(rdoff_mr,str,True) || is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) goto exit;
      if(bmEOF()) break;
    }
  }
  if(rdoff_mr->nItems) ma_Display(rdoff_mr,MOD_REFER,LB_SORTABLE,0);
  else                 NotifyBox(NOT_ENTRY,MOD_REFER);
  exit:
  ma_Destroy(rdoff_mr);
  return fpos;
}

static __filesize_t __FASTCALL__ rdoff_ShowImport( void )
{
  __filesize_t fpos;
  unsigned char rec;
  unsigned i;
  char str[33];
  memArray * rdoff_it;
  fpos = BMGetCurrFilePos();
  if(!(rdoff_it = ma_Build(0,True))) return fpos;
  bmSeek(10,BM_SEEK_SET);
  while(bmGetCurrFilePos() < rdoff_hdrlen + 5)
  {
    tBool is_eof;
    rec = bmReadByte();
    is_eof = False;
    if(rec == 2)
    {
      char ch;
      bmSeek(2,BM_SEEK_CUR);
      for(i = 0;i < 32;i++)
      {
        ch = bmReadByte();
        is_eof = bmEOF();
        str[i] = ch;
        if(!ch || is_eof) break;
      }
      str[i] = 0;
      if(!ma_AddString(rdoff_it,str,True) || is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) goto exit;
      if(bmEOF()) break;
    }
  }
  if(rdoff_it->nItems) ma_Display(rdoff_it,IMPPROC_TABLE,LB_SORTABLE,0);
  else                 NotifyBox(NOT_ENTRY,EXP_TABLE);
  exit:
  ma_Destroy(rdoff_it);
  return fpos;
}

static __filesize_t __FASTCALL__ rdoff_ShowHeader( void )
{
  int endian;
  __filesize_t fpos,entry;
  unsigned long hs_len,cs_len;
  TWindow *w;
  fpos = BMGetCurrFilePos();
  endian = bmReadByteEx(5,BM_SEEK_SET);
  hs_len = bmReadDWord();
  bmSeek(hs_len,BM_SEEK_CUR);
  cs_len = bmReadDWord();
  bmSeek(cs_len,BM_SEEK_CUR);
  ds_len = bmReadDWord();
  cs_start = hs_len + 14;
  ds_start = cs_start + cs_len + 4;
  w = CrtDlgWndnls(endian == 0x01 ? " RDOFF big endian " : " RDOFF little endian ",54,6);
  twGotoXY(1,1);
  twPrintF("Length of header section    = %08lXH\n"
           "Length of code section      = %08lXH\n"
           "segment .code               = %08lXH\n"
           "Length of data secion       = %08lXH\n"
           "segment .data               = %08lXH\n"
           ,hs_len
           ,cs_len
           ,cs_start
           ,ds_len
           ,ds_start);
  twSetColorAttr(dialog_cset.entry);
  entry = rdoff_FindExport("_main");
  twPrintF("Entry point                 = %08lXH"
           ,entry);
  twClrEOL();
  twSetColorAttr(dialog_cset.main);
  while(1)
  {
    int keycode;
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    switch(keycode)
    {
      case KE_ENTER:      fpos = entry; goto exit;
      case KE_F(10):
      case KE_ESCAPE:     goto exit;
      default:            break;
    };
  }
  exit:
  CloseWnd(w);
  return fpos;
}

static tCompare __FASTCALL__ rdoff_compare_reloc(const void __HUGE__ *e1,const void __HUGE__ *e2)
{
  const RDOFF_RELOC __HUGE__ *r1,__HUGE__ *r2;
  r1 = e1;
  r2 = e2;
  return __CmpLong__(r1->offset,r2->offset);
}

static void __NEAR__ __FASTCALL__ BuildRelocRDOFF( void )
{
  unsigned char rec;
  if(!(rdoffReloc = la_Build(0,sizeof(RDOFF_RELOC),MemOutBox))) return;
  bmSeek(10,BM_SEEK_SET);
  while(bmGetCurrFilePos() < rdoff_hdrlen + 5)
  {
    RDOFF_RELOC rdf_r;
    rec = bmReadByte();
    if(rec == 1)
    {
      unsigned char segno;
      __filesize_t off;
      segno = bmReadByte();
      rdf_r.is_rel = 0;
      if(segno >= 64)
      {
        rdf_r.is_rel = 1;
        segno -= 64;
      }
      rdf_r.offset = segno == 0 ? cs_start : segno == 1 ? ds_start : FILESIZE_MAX;
      off = bmReadDWord();
      if(bmEOF()) break;
      if(rdf_r.offset < FILESIZE_MAX) rdf_r.offset += off;
      rdf_r.reflen = bmReadByte();
      rdf_r.segto = bmReadWord();
      if(!la_AddData(rdoffReloc,&rdf_r,MemOutBox)) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) goto exit;
      if(bmEOF()) break;
    }
  }
  exit:
  la_Sort(rdoffReloc,rdoff_compare_reloc);
}

static unsigned char __codelen;

static __filesize_t __FASTCALL__ rdoffBuildReferStr(char *str,RDOFF_RELOC *reloc,__filesize_t ulShift,int flags)
{
   char name[400],ch,buff[400];
   const char *ptr_type;
   __filesize_t field_val,foff,base_seg_off, retrf;
   unsigned i;
   if(flags & APREF_USE_TYPE)
   {
     switch(reloc->reflen)
     {
       case 1: ptr_type = "(b) "; break;
       case 2: ptr_type = "off16 "; break;
       default:
       case 4: ptr_type = "off32 "; break;
     }
     strcat(str,ptr_type);
   }
   base_seg_off = FILESIZE_MAX;
   switch(reloc->segto)
   {
     case 0:  ptr_type = "cs:";
              base_seg_off = cs_start;
              break;
     case 1:  ptr_type = "ds:";
              base_seg_off = ds_start;
              break;
     case 2:  ptr_type = "bss:";
              break;
     default:
              {
                 struct rdoff_ImpName key,*ret;
                 key.lsegno = reloc->segto;
                 ret = la_Find(rdoffImpNames,&key,compare_impnames);
                 if(ret)
                 {
                   bmSeek(ret->nameoff,BM_SEEK_SET);
                   name[0] = 0;
                   for(i = 0;i < sizeof(name);i++)
                   {
                     ch = bmReadByte();
                     name[i] = ch;
                     if(!ch || bmEOF()) break;
                   }
                   name[i] = 0;
                   ptr_type = name;
                 }
                 else
                 {
                   ptr_type = "???";
                 }
              }
   }
   retrf = RAPREF_DONE;
   if(reloc->segto >= 3) strcat(str,ptr_type); /** case extern symbol */
   if(reloc->segto < 3 || reloc->is_rel)
   {
     foff = bmGetCurrFilePos();
     switch(reloc->reflen)
     {
       default:
       case 4: field_val = bmReadDWordEx(reloc->offset,BM_SEEK_SET);
               break;
       case 2: field_val = bmReadWordEx(reloc->offset,BM_SEEK_SET);
               break;
       case 1: field_val = bmReadByteEx(reloc->offset,BM_SEEK_SET);
               break;
     }
     bmSeek(foff,BM_SEEK_SET);
     if(reloc->segto < 3)
     {
       if(base_seg_off < FILESIZE_MAX)
       {
         base_seg_off += field_val;
         if(FindPubName(buff,sizeof(buff),base_seg_off))
         {
           strcat(str,buff);
         }
         else goto unnamed;
       }
       else
       {
         unnamed:
         retrf = field_val;
         strcat(str,ptr_type);
         strcat(str,Get8Digit(field_val));
         if(!EditMode && !DumpMode && (flags & APREF_TRY_LABEL))
           GidAddGoAddress(str,reloc->segto ? ds_start + field_val : cs_start + field_val);
       }
     }
     if(reloc->is_rel)
     {
       __filesize_t seg_off;
       __fileoff_t fv;
       seg_off = ulShift < ds_start ? cs_start : ulShift < ds_start+ds_len ? ds_start : FILESIZE_MAX;
       fv = field_val;
       if(!(ulShift + fv + reloc->reflen == seg_off && (flags & APREF_TRY_LABEL)))
       {
         sprintf(&str[strlen(str)],"(%s)",Get8SignDig(field_val));
       }
     }
   }
   return retrf;
}

static unsigned long __FASTCALL__ rdoff_AppendRef(char *str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  RDOFF_RELOC *rrdoff,key;
  unsigned long ret;
  char buff[400];
  if(flags & APREF_TRY_PIC) return RAPREF_NONE;
  if(!rdoffReloc) BuildRelocRDOFF();
  if(!rdoffImpNames) ReadImpNameList(bmbioHandle(),MemOutBox);
  if(!PubNames) rdoff_ReadPubNameList(bmbioHandle(),MemOutBox);
  key.offset = ulShift;
  __codelen = codelen;
  rrdoff = la_Find(rdoffReloc,&key,rdoff_compare_reloc);
  ret = rrdoff ? rdoffBuildReferStr(str,rrdoff,ulShift,flags) : RAPREF_NONE;
  if(!ret && (flags & APREF_TRY_LABEL))
  {
     if(FindPubName(buff,sizeof(buff),r_sh))
     {
       strcat(str,buff);
       if(!DumpMode && !EditMode) GidAddGoAddress(str,r_sh);
       ret = RAPREF_DONE;
     }
  }
  return ret;
}

static tBool __FASTCALL__ rdoff_check_fmt( void )
{
  char rbuff[6];
  bmReadBufferEx(rbuff,sizeof(rbuff),0L,BM_SEEK_SET);
  return memcmp(rbuff,"RDOFF1",sizeof(rbuff)) == 0 ||
         memcmp(rbuff,"RDOFF\x1",sizeof(rbuff)) == 0;
}

static void __FASTCALL__ rdoff_init_fmt( void )
{
  unsigned long cs_len;
  PMRegLowMemCallBack(rdoffLowMemFunc);
  rdoff_hdrlen = bmReadDWordEx(6,BM_SEEK_SET);
  bmSeek(rdoff_hdrlen,BM_SEEK_CUR);
  cs_len = bmReadDWord();
  bmSeek(cs_len,BM_SEEK_CUR);
  ds_len = bmReadDWord();
  cs_start = rdoff_hdrlen + 14;
  ds_start = cs_start + cs_len + 4;
}

static void __FASTCALL__ rdoff_destroy_fmt( void )
{
  if(rdoffReloc) { la_Destroy(rdoffReloc); rdoffReloc = NULL; }
  if(rdoffImpNames) { la_Destroy(rdoffImpNames); rdoffImpNames = NULL; }
  if(PubNames) { la_Destroy(PubNames); PubNames = NULL; }
  PMUnregLowMemCallBack(rdoffLowMemFunc);
}

static void __FASTCALL__ rdoff_ReadPubName(BGLOBAL b_cache,const struct PubName *it,
                       char *buff,unsigned cb_buff)
{
    unsigned char ch;
    unsigned i;
    bioSeek(b_cache,it->nameoff,SEEK_SET);
    for(i = 0;i < cb_buff;i++)
    {
      ch = bmReadByte();
      buff[i] = ch;
      if(!ch || bmEOF()) break;
    }
}

static tBool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa)
{
  BGLOBAL b_cache;
  b_cache = bmbioHandle();
  return fmtFindPubName(b_cache,buff,cb_buff,pa,
                        rdoff_ReadPubNameList,
                        rdoff_ReadPubName);
}

static void __FASTCALL__ rdoff_ReadPubNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *))
{
 unsigned char segno,rec;
 __filesize_t segoff,abs_off;
 unsigned i;
 struct PubName rdf_pn;
 UNUSED(handle);
 if(!PubNames)
   if(!(PubNames = la_Build(0,sizeof(struct PubName),mem_out))) return;
 bmSeek(10,BM_SEEK_SET);
 while(bmGetCurrFilePos() < rdoff_hdrlen + 5)
 {
    tBool is_eof;
    rec = bmReadByte();
    is_eof = False;
    if(rec == 3)
    {
      char ch;
      segno = bmReadByte();
      segoff = bmReadDWord();
      rdf_pn.nameoff = bmGetCurrFilePos();
      for(i = 0;i < 32;i++)
      {
        ch = bmReadByte();
        is_eof = bmEOF();
        if(!ch || is_eof) break;
      }
      abs_off = segno == 0 ? cs_start : segno == 1 ? ds_start : FILESIZE_MAX;
      if(abs_off < FILESIZE_MAX) abs_off += segoff;
      rdf_pn.pa = abs_off;
      rdf_pn.attr = SC_GLOBAL;
      if(!la_AddData(PubNames,&rdf_pn,MemOutBox) || is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) break;
      if(bmEOF()) break;
    }
 }
 if(PubNames->nItems) la_Sort(PubNames,fmtComparePubNames);
}

static tCompare __FASTCALL__ compare_impnames(const void __HUGE__ *v1,const void __HUGE__ *v2)
{
  const struct rdoff_ImpName __HUGE__ *pnam1,__HUGE__ *pnam2;
  pnam1 = (const struct rdoff_ImpName __HUGE__ *)v1;
  pnam2 = (const struct rdoff_ImpName __HUGE__ *)v2;
  return __CmpLong__(pnam1->lsegno,pnam2->lsegno);
}

static void __NEAR__ __FASTCALL__ ReadImpNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *))
{
 unsigned char rec;
 unsigned i;
 struct rdoff_ImpName rdf_in;
 UNUSED(handle);
 if(!rdoffImpNames)
   if(!(rdoffImpNames = la_Build(0,sizeof(struct rdoff_ImpName),mem_out))) return;
 bmSeek(10,BM_SEEK_SET);
 while(bmGetCurrFilePos() < rdoff_hdrlen + 5)
 {
    tBool is_eof;
    rec = bmReadByte();
    is_eof = False;
    if(rec == 2)
    {
      char ch;
      rdf_in.lsegno = bmReadWord();
      rdf_in.nameoff = bmGetCurrFilePos();
      for(i = 0;i < 32;i++)
      {
        ch = bmReadByte();
        is_eof = bmEOF();
        if(!ch) break;
      }
      if(!la_AddData(rdoffImpNames,&rdf_in,MemOutBox) || is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) break;
      if(bmEOF()) break;
    }
 }
 if(rdoffImpNames->nItems) la_Sort(rdoffImpNames,compare_impnames);
}

/** bitness not declared, but we assign 32-bit as default */
static int __FASTCALL__ rdoff_bitness(__filesize_t pa)
{
  UNUSED(pa);
  return DAB_USE32;
}

static __filesize_t __FASTCALL__ rdoffGetPubSym(char *str,unsigned cb_str,unsigned *func_class,
                           __filesize_t pa,tBool as_prev)
{
  BGLOBAL b_cache;
  b_cache = bmbioHandle();
  return fmtGetPubSym(b_cache,str,cb_str,func_class,pa,as_prev,
                      rdoff_ReadPubNameList,
                      rdoff_ReadPubName);
}

static unsigned __FASTCALL__ rdoffGetObjAttr(__filesize_t pa,char *name,unsigned cb_name,
                         __filesize_t *start,__filesize_t *end,int *_class,int *bitness)
{
  unsigned ret;
  UNUSED(cb_name);
  *start = 0;
  *end = bmGetFLength();
  *_class = OC_NOOBJECT;
  *bitness = rdoff_bitness(pa);
  name[0] = 0;
  if(pa < cs_start)
  {
    *end = cs_start;
    ret = 0;
  }
  else
    if(pa >= cs_start && pa < ds_start - 4)
    {
      *start = cs_start;
      *_class = OC_CODE;
      *end = ds_start-4;
      ret = 1;
    }
    else
    {
      if(pa >= ds_start - 4 && pa < ds_start)
      {
        *start = ds_start-4;
        *end = ds_start;
        *_class = OC_NOOBJECT;
        ret = 2;
      }
      else
      {
        if(pa >= ds_start && pa < ds_start + ds_len)
        {
          *_class = OC_DATA;
          *start = ds_start;
          *end = ds_start + ds_len;
          ret = 3;
        }
        else
        {
          *start = ds_start+ds_len;
          *_class = OC_NOOBJECT;
          ret = 4;
        }
      }
    }
  return ret;
}

static __filesize_t __FASTCALL__ rdoffVA2PA(__filesize_t va)
{
  return va + cs_start;
}

static __filesize_t __FASTCALL__ rdoffPA2VA(__filesize_t pa)
{
  return pa > cs_start ? pa - cs_start : 0L;
}

static tBool __FASTCALL__ rdoff_AddressResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
 tBool bret = True;
 tUInt32 res;
 if(cfpos < cs_start)
 {
    strcpy(addr,"RDFH:");
    strcpy(&addr[5],Get4Digit(cfpos));
 }
 else
   if((res=rdoffPA2VA(cfpos))!=0)
   {
     addr[0] = '.';
     strcpy(&addr[1],Get8Digit(res));
   }
   else bret = False;
  return bret;
}

static int __FASTCALL__ rdoff_platform( void ) { return DISASM_CPU_IX86; }

REGISTRY_BIN rdoffTable =
{
  "RDOFF (Relocatable Dynamic Object File Format)",
  { "RdHelp", "ModRef", "Export", NULL, "Import", NULL, NULL, NULL, NULL, NULL },
  { rdoff_Help, rdoff_ShowModRef, rdoff_ShowExport, NULL, rdoff_ShowImport, NULL, NULL, NULL, NULL, NULL },
  rdoff_check_fmt,
  rdoff_init_fmt,
  rdoff_destroy_fmt,
  rdoff_ShowHeader,
  rdoff_AppendRef,
  fmtSetState,
  rdoff_platform,
  rdoff_bitness,
  NULL,
  rdoff_AddressResolv,
  rdoffVA2PA,
  rdoffPA2VA,
  rdoffGetPubSym,
  rdoffGetObjAttr,
  NULL,
  NULL
};
