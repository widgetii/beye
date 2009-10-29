/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/coff386.c
 * @brief       This file contains implementation of COFF-i386 file format decoder.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "reg_form.h"
#include "tstrings.h"
#include "codeguid.h"
#include "colorset.h"
#include "bconsole.h"
#include "bin_util.h"
#include "bmfile.h"
#include "biewutil.h"
#include "biewhelp.h"
#include "plugins/bin/coff386.h"
#include "plugins/disasm.h"
#include "biewlib/kbd_code.h"
#include "biewlib/pmalloc.h"

#define COFF_WORD(cval)  ((tUInt16)(*(const tUInt16 *)(const tUInt8 *)cval))
#define COFF_DWORD(cval) ((tUInt32)(*(const tUInt32 *)(const tUInt8 *)cval))


static struct external_filehdr coff386hdr;
static AOUTHDR coff386ahdr;
static SCNHDR *coff386so;
static tUIntFast16 nsections;
static BGLOBAL coff_cache;
static __filesize_t strings_ptr;

static void __FASTCALL__ coff_ReadPubNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *));
static void __FASTCALL__ coff_ReadPubName(BGLOBAL b_cache,const struct PubName *it,
                           char *buff,unsigned cb_buff);
static unsigned __FASTCALL__ coff386_GetObjAttr(__filesize_t pa,char *name,unsigned cb_name,
                             __filesize_t *start,__filesize_t *end,int *_class,int *bitness);

static tBool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa)
{
  return fmtFindPubName(coff_cache,buff,cb_buff,pa,
                        coff_ReadPubNameList,
                        coff_ReadPubName);
}


static void __NEAR__ __FASTCALL__ coffReadLongName(BGLOBAL handle,__filesize_t offset,
                                      char *str, unsigned slen)
{
  unsigned i;
  __filesize_t fpos;
   fpos = bioTell(handle);
   bioSeek(handle,offset+strings_ptr,BIO_SEEK_SET);
   for(i = 0;i < slen;i++)
   {
     unsigned char ch;
     ch = bioReadByte(handle);
     if(ch && !bioEOF(handle)) str[i] = ch;
     else  break;
   }
   str[i] = 0;
   bioSeek(handle,fpos,BIO_SEEK_SET);
}

static __filesize_t __FASTCALL__ coff386_VA2PA(__filesize_t va)
{
  tUIntFast16 i;
  for(i = 0;i < nsections;i++)
  {
    if(va >= COFF_DWORD(coff386so[i].s_vaddr) && va <= COFF_DWORD(coff386so[i].s_vaddr)+COFF_DWORD(coff386so[i].s_size))
    {
      return COFF_DWORD(coff386so[i].s_scnptr) + (va - COFF_DWORD(coff386so[i].s_vaddr));
    }
  }
  return 0L;
}

static __filesize_t __FASTCALL__ coff386_PA2VA(__filesize_t pa)
{
  tUIntFast16 i;
  for(i = 0;i < nsections;i++)
  {
    if(pa >= COFF_DWORD(coff386so[i].s_scnptr) && pa <= COFF_DWORD(coff386so[i].s_scnptr)+COFF_DWORD(coff386so[i].s_size))
    {
      return COFF_DWORD(coff386so[i].s_vaddr) + (pa - COFF_DWORD(coff386so[i].s_scnptr));
    }
  }
  return 0L;
}


static void __FASTCALL__ coffObjPaint(TWindow * win,const void ** names,unsigned start,unsigned nlist)
{
 char buffer[81];
 const SCNHDR ** obj = (const SCNHDR **)names;
 const SCNHDR *  objs = obj[start];
 twUseWin(win);
 twFreezeWin(win);
 twClearWin();
 sprintf(buffer," Object Table [ %u / %u ] ",start + 1,nlist);
 twSetTitleAttr(win,buffer,TW_TMODE_CENTER,dialog_cset.title);
 twSetFooterAttr(win,PAGEBOX_SUB,TW_TMODE_RIGHT,dialog_cset.selfooter);
 twGotoXY(1,1);
 twPrintF("Object Name                    = %8s\n"
          "Physical address               = %08lXH\n"
          "Virtual address                = %08lXH\n"
          "Section size                   = %08lXH\n"
          "File pointer to raw data       = %08lXH\n"
          "File pointer to relocations    = %08lXH\n"
          "File pointer to line numbers   = %08lXH\n"
          "Number of relocations          = %04XH\n"
          "Number of line numbers         = %04XH\n"
          "Flags                          = %08lXH\n"
          "  [%c] - section type is text\n"
          "  [%c] - section type is data\n"
          "  [%c] - section type is bss"
          ,objs->s_name
          ,COFF_DWORD(objs->s_paddr)
          ,COFF_DWORD(objs->s_vaddr)
          ,COFF_DWORD(objs->s_size)
          ,COFF_DWORD(objs->s_scnptr)
          ,COFF_DWORD(objs->s_relptr)
          ,COFF_DWORD(objs->s_lnnoptr)
          ,COFF_WORD(objs->s_nreloc)
          ,COFF_WORD(objs->s_nlnno)
          ,COFF_DWORD(objs->s_flags)
          ,GetBool((COFF_DWORD(objs->s_flags) & STYP_TEXT) == STYP_TEXT)
          ,GetBool((COFF_DWORD(objs->s_flags) & STYP_DATA) == STYP_DATA)
          ,GetBool((COFF_DWORD(objs->s_flags) & STYP_BSS) == STYP_BSS)
          );
 twRefreshFullWin(win);
}

static tBool __NEAR__ __FASTCALL__ __coffReadObjects(BGLOBAL handle,memArray * obj,unsigned n)
{
 size_t i;
  for(i = 0;i < n;i++)
  {
    SCNHDR po;
    if(IsKbdTerminate() || bioEOF(handle)) break;
    bioReadBuffer(handle,&po,sizeof(SCNHDR));
    if(!ma_AddData(obj,&po,sizeof(SCNHDR),True)) break;
  }
  return True;
}

static __filesize_t __FASTCALL__ coffShowObjects( void )
{
 BGLOBAL handle;
 unsigned nnames;
 __filesize_t fpos,off;
 memArray * obj;
 fpos = BMGetCurrFilePos();
 nnames = COFF_WORD(coff386hdr.f_nscns);
 if(!nnames) { NotifyBox(NOT_ENTRY," Objects Table "); return fpos; }
 if(!(obj = ma_Build(nnames,True))) return fpos;
 handle = coff_cache;
 off = sizeof(coff386hdr);
 if(COFF_WORD(coff386hdr.f_opthdr)) off += COFF_WORD(coff386hdr.f_opthdr);
 bioSeek(handle,off,SEEK_SET);
 if(__coffReadObjects(handle,obj,nnames))
 {
  int ret;
    ret = PageBox(70,13,(const void **)obj->data,obj->nItems,coffObjPaint);
    if(ret != -1)  fpos = COFF_DWORD(((SCNHDR *)obj->data[ret])->s_scnptr);
 }
 ma_Destroy(obj);
 return fpos;
}

static const char * __NEAR__ __FASTCALL__ coff386_encode_hdr(unsigned info)
{
   switch(info)
   {
     case I386MAGIC: return " i386 coff ";
     case I386PTXMAGIC: return " i386 PTX coff ";
     case I386AIXMAGIC: return " i386 AIX coff ";
     case LYNXCOFFMAGIC: return " Lynx coff ";
     default:    return " Unknown coff i386 format ";
   }
}

static __filesize_t __FASTCALL__ ShowCoff386Header( void )
{
  __filesize_t fpos,entry,v_entry;
  unsigned keycode;
  TWindow * w;
  fpos = BMGetCurrFilePos();
  v_entry = entry = 0L;
  if(*(unsigned short *)coff386ahdr.magic == ZMAGIC)
  {
    v_entry = entry = *(__filesize_t *)&coff386ahdr.entry;
    entry = coff386_VA2PA(v_entry);
  }
  w = CrtDlgWndnls(coff386_encode_hdr(COFF_WORD(coff386hdr.f_magic)),54,12);
  twGotoXY(1,1);
  twPrintF("Number of sections          = %04XH\n"
           "Time & date stamp           = %08lXH\n"
           "File pointer to symtab      = %08lXH\n"
           "Number of symtab entries    = %08lXH\n"
           "Size of optional header     = %04XH\n"
           "COFF flags                  = %04XH\n"
           "  [%c] - Rel. info stripped\n"
           "  [%c] - File is executable (no unresolv. refs.)\n"
           "  [%c] - Line numbers stripped\n"
           "  [%c] - Local symbols stripped\n"
           "  [%c] - File is 32-bits little endian\n"
           ,COFF_WORD(coff386hdr.f_nscns)
           ,COFF_DWORD(coff386hdr.f_timdat)
           ,COFF_DWORD(coff386hdr.f_symptr)
           ,COFF_DWORD(coff386hdr.f_nsyms)
           ,COFF_WORD(coff386hdr.f_opthdr)
           ,COFF_WORD(coff386hdr.f_flags)
           ,GetBool((COFF_WORD(coff386hdr.f_flags) & F_RELFLG) == F_RELFLG)
           ,GetBool((COFF_WORD(coff386hdr.f_flags) & F_EXEC) == F_EXEC)
           ,GetBool((COFF_WORD(coff386hdr.f_flags) & F_LNNO) == F_LNNO)
           ,GetBool((COFF_WORD(coff386hdr.f_flags) & F_LSYMS) == F_LSYMS)
           ,GetBool((COFF_WORD(coff386hdr.f_flags) & F_AR32WR) == F_AR32WR));
  twSetColorAttr(dialog_cset.entry);
  twPrintF("Entry point                 = %08lXH (VA=%08lXH)"
           ,entry,v_entry);
  twClrEOL();
  twSetColorAttr(dialog_cset.main);
  while(1)
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    if(keycode == KE_ENTER) { fpos = entry; break; }
    else
      if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
  }
  CloseWnd(w);
  return fpos;
}

static const char * __NEAR__ __FASTCALL__ coffEncodeType(unsigned type)
{
  const char *ret;
  switch(type)
  {
    case T_NULL: ret = "T_NULL"; break;
    case T_VOID: ret = "T_VOID"; break;
    case T_CHAR: ret = "T_CHAR"; break;
    case T_SHORT: ret = "T_SHORT"; break;
    case T_INT: ret = "T_INT"; break;
    case T_LONG: ret = "T_LONG"; break;
    case T_FLOAT: ret = "T_FLOAT"; break;
    case T_DOUBLE: ret = "T_DOUBLE"; break;
    case T_STRUCT: ret = "T_STRUCT"; break;
    case T_UNION: ret = "T_UNION"; break;
    case T_ENUM: ret = "T_ENUM"; break;
    case T_MOE: ret = "T_MOE"; break;
    case T_UCHAR: ret = "T_UCHAR"; break;
    case T_USHORT: ret = "T_USHORT"; break;
    case T_UINT: ret = "T_UINT"; break;
    case T_ULONG: ret = "T_ULONG"; break;
    case T_LNGDBL: ret = "T_LNGDBL"; break;
    default: ret = "T_UNK"; break;
  }
  return ret;
}

static const char * __NEAR__ __FASTCALL__ coffEncodeClass(unsigned _class)
{
  const char *ret;
  switch(_class)
  {
    case C_EFCN: ret = "C_EFCN"; break;
    case C_NULL: ret = "C_NULL"; break;
    case C_AUTO: ret = "C_AUTO"; break;
    case C_EXT: ret = "C_EXT"; break;
    case C_STAT: ret = "C_STAT"; break;
    case C_REG: ret = "C_REG"; break;
    case C_EXTDEF: ret = "C_EXTDEF"; break;
    case C_LABEL: ret = "C_LABEL"; break;
    case C_ULABEL: ret = "C_ULABEL"; break;
    case C_MOS: ret = "C_MOS"; break;
    case C_ARG: ret = "C_ARG"; break;
    case C_STRTAG: ret = "C_STRTAG"; break;
    case C_MOU: ret = "C_MOU"; break;
    case C_UNTAG: ret = "C_UNTAG"; break;
    case C_TPDEF: ret = "C_TPDEF"; break;
    case C_USTATIC: ret = "C_USTATIC"; break;
    case C_ENTAG: ret = "C_ENTAG"; break;
    case C_MOE: ret = "C_MOE"; break;
    case C_REGPARM: ret = "C_REGPARM"; break;
    case C_FIELD: ret = "C_FIELD"; break;
    case C_AUTOARG: ret = "C_AUTOARG"; break;
    case C_LASTENT: ret = "C_LASTENT"; break;
    case C_BLOCK: ret = "C_BLOCK"; break;
    case C_FCN: ret = "C_FCN"; break;
    case C_EOS: ret = "C_EOS"; break;
    case C_FILE: ret = "C_FILE"; break;
    case C_LINE: ret = "C_LINE"; break;
    case C_ALIAS: ret = "C_ALIAS"; break;
    case C_HIDDEN: ret = "C_HIDDEN"; break;
    default: ret = "C_UNK"; break;
  }
  return ret;
}


static unsigned __FASTCALL__ coffSymTabNumItems(BGLOBAL handle)
{
  UNUSED(handle);
  return (unsigned)COFF_DWORD(coff386hdr.f_nsyms);
}

static tBool  __FASTCALL__ coffSymTabReadItems(BGLOBAL handle,memArray * obj,unsigned nnames)
{
 unsigned i;
 unsigned length;
 bioSeek(handle,COFF_DWORD(coff386hdr.f_symptr),SEEKF_START);
 for(i = 0;i < nnames;i++)
 {
   struct external_syment cse;
   char stmp[256];
   memset(stmp,0,sizeof(stmp));
   bioReadBuffer(handle,&cse,sizeof(struct external_syment));
   if(COFF_DWORD(cse.e.e.e_zeroes) == 0L &&
      COFF_DWORD(cse.e.e.e_offset) >= 4)
        coffReadLongName(handle,COFF_DWORD(cse.e.e.e_offset),stmp,sizeof(stmp));
   else
   {
     memcpy(stmp,cse.e.e_name,E_SYMNMLEN);
     stmp[E_SYMNMLEN] = 0;
   }
   if(IsKbdTerminate() || bioEOF(handle)) break;
   length = strlen(stmp);
   if(length > 38) strcpy(&stmp[38],">>>");
   else  {  memset(&stmp[length],' ',38-length);  stmp[38] = 0; }
   sprintf(&stmp[strlen(stmp)-1],"(%04hX.%08lX) %s.%s"
           ,COFF_WORD(cse.e_scnum)
           ,(unsigned long)COFF_DWORD(cse.e_value)
           ,coffEncodeClass(cse.e_sclass[0])
           ,coffEncodeType(COFF_WORD(cse.e_type)));
   if(!ma_AddString(obj,stmp,True)) break;
 }
 return True;
}

static __filesize_t __NEAR__ __FASTCALL__ CalcEntryCoff(unsigned long idx,tBool display_msg)
{
  struct external_syment cse;
  tUIntFast16 sec_num;
  __filesize_t fpos;
  fpos = 0L;
  bmSeek(COFF_DWORD(coff386hdr.f_symptr)+idx*sizeof(struct external_syment),BM_SEEK_SET);
  bmReadBuffer(&cse,sizeof(struct external_syment));
  sec_num = COFF_WORD(cse.e_scnum);
  if(sec_num && sec_num <= COFF_WORD(coff386hdr.f_nscns) &&
    ((cse.e_sclass[0] == C_EXT ||
      cse.e_sclass[0] == C_STAT ||
      cse.e_sclass[0] == C_HIDDEN) ||
     (cse.e_sclass[0] == C_LABEL &&
      COFF_DWORD(cse.e_value))))
  {
    fpos = COFF_DWORD(coff386so[sec_num-1].s_scnptr)+COFF_DWORD(cse.e_value);
  }
  else
    if(display_msg)
      ErrMessageBox(NO_ENTRY,NULL);
  return fpos;
}

static __filesize_t __FASTCALL__ coffShowSymTab( void )
{
  __filesize_t fpos = BMGetCurrFilePos();
  int ret;
  ret = fmtShowList(coffSymTabNumItems,coffSymTabReadItems,
                    "Symbol Table",
                    LB_SELECTIVE,NULL);
  if(ret != -1)
  {
    fpos = CalcEntryCoff(ret,True);
  }
  return fpos;
}

/***************************************************************************/
/*********************  REFS COFF386  **************************************/
/***************************************************************************/
static char __codelen;

typedef struct tagRELOC_COFF386
{
  unsigned long offset;
  unsigned long nameoff;
  unsigned      type;
}RELOC_COFF386;

static linearArray *RelocCoff386 = NULL;

static tCompare __FASTCALL__ coff386_compare_rels(const void __HUGE__ *e1,const void __HUGE__ *e2)
{
  const RELOC_COFF386 __HUGE__ *r1,__HUGE__ *r2;
  r1 = e1;
  r2 = e2;
  return __CmpLong__(r1->offset,r2->offset);
}

static void __NEAR__ __FASTCALL__ BuildRelocCoff386( void )
{
  TWindow * w,*usd;
  size_t j,segcount, nr;
  RELOC_COFF386 rel;
  usd = twUsedWin();
  if(!(RelocCoff386 = la_Build(0,sizeof(RELOC_COFF386),MemOutBox))) return;
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  if(!PubNames) coff_ReadPubNameList(bmbioHandle(),MemOutBox);
  twUseWin(w);
  twGotoXY(1,1);
  twPutS(BUILD_REFS);
  twUseWin(usd);
  for(segcount = 0;segcount < COFF_WORD(coff386hdr.f_nscns);segcount++)
  {
    tBool is_eof;
    is_eof = False;
    bmSeek(COFF_DWORD(coff386so[segcount].s_relptr),BM_SEEK_SET);
    nr = COFF_WORD(coff386so[segcount].s_nreloc);
    for(j = 0;j < nr;j++)
    {
      struct external_reloc er;
      bmReadBuffer(&er,sizeof(struct external_reloc));
      if((is_eof = bmEOF()) != 0) break;
      rel.offset = er.r_vaddr + COFF_DWORD(coff386so[segcount].s_scnptr);
      rel.nameoff = er.r_symndx;
      rel.type = er.r_type;
      if(!la_AddData(RelocCoff386,&rel,MemOutBox)) goto next;
    }
    if(is_eof) break;
  }
  next:
  la_Sort(RelocCoff386,coff386_compare_rels);
  CloseWnd(w);
}

static tBool  __NEAR__ __FASTCALL__ coffSymTabReadItemsIdx(BGLOBAL handle,unsigned long idx,
                                            char *name,unsigned cb_name,
                                            unsigned *secnum,
                                            __filesize_t *offset)
{
 struct external_syment cse;
 if(idx >= COFF_DWORD(coff386hdr.f_nsyms)) return False;
 bioSeek(handle,COFF_DWORD(coff386hdr.f_symptr) + idx*sizeof(struct external_syment),SEEKF_START);
 bioReadBuffer(handle,&cse,sizeof(struct external_syment));
 if(COFF_DWORD(cse.e.e.e_zeroes) == 0L &&
    COFF_DWORD(cse.e.e.e_offset) >= 4)
          coffReadLongName(handle,COFF_DWORD(cse.e.e.e_offset),name,cb_name);
 else
 {
   memcpy(name,cse.e.e_name,E_SYMNMLEN);
   name[E_SYMNMLEN] = 0;
 }
 *secnum = COFF_WORD(cse.e_scnum);
 *offset = COFF_DWORD(cse.e_value);
 return True;
}

static __filesize_t __NEAR__ __FASTCALL__ BuildReferStrCoff386(char *str,RELOC_COFF386 *rne,int flags)
{
  __filesize_t offset,retval,s,e;
  unsigned long val;
  tUIntFast16 secnum=0;
  tBool is_idx,val_assigned;
  int c,b;
  char name[256],pubname[256],secname[256];
  retval = RAPREF_DONE;
  val = bmReadDWordEx(rne->offset,BM_SEEK_SET);
  /* rne->nameoff it's only pointer to name descriptor */
  is_idx = coffSymTabReadItemsIdx(coff_cache,rne->nameoff,name,sizeof(name),&secnum,&offset);
  val_assigned = False;
  if(is_idx)
  {
    if(!offset && secnum)
    {
      /*
         try resolve some relocations (like .text+1234) with names from
         public name list
      */
      if(FindPubName(pubname,sizeof(pubname),COFF_DWORD(coff386so[secnum-1].s_scnptr) + val))
      {
        strcat(str,pubname);
        val_assigned = True;
      }
      else goto def_val;
    }
    else
    {
      def_val:
      strcat(str,name);
    }
  }
  else
    strcat(str,".?bad?");
  if(val && !val_assigned)
  {
     strcat(str,"+");
     strcat(str,Get8Digit(val));
     if(secnum) retval = COFF_DWORD(coff386so[secnum-1].s_scnptr)+val;
     else       retval = RAPREF_NONE;
  }
  if(rne->type == RELOC_REL32 && (flags & APREF_TRY_LABEL) != APREF_TRY_LABEL)
  {
     coff386_GetObjAttr(rne->offset,secname,sizeof(secname),&s,&e,&c,&b);
     strcat(str,"-");
     strcat(str,secname);
  }
  e = CalcEntryCoff(rne->nameoff,False);
  if(!DumpMode && !EditMode && e && (flags & APREF_TRY_LABEL) == APREF_TRY_LABEL)
                                                  GidAddGoAddress(str,e);
  return retval;
}

static unsigned long __FASTCALL__ coff386_AppendRef(char *str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  RELOC_COFF386 *rcoff386,key;
  __filesize_t ret;
  char buff[400];
  ret = RAPREF_NONE;
  if(flags & APREF_TRY_PIC) return RAPREF_NONE;
  if(!PubNames) coff_ReadPubNameList(bmbioHandle(),MemOutBox);
  if((COFF_WORD(coff386hdr.f_flags) & F_RELFLG) == F_RELFLG) goto try_pub;
  if(!RelocCoff386) BuildRelocCoff386();
  key.offset = ulShift;
  __codelen = codelen;
  rcoff386 = la_Find(RelocCoff386,&key,coff386_compare_rels);
  if(rcoff386) ret = BuildReferStrCoff386(str,rcoff386,flags);
  try_pub:
  if(!ret && (flags & APREF_TRY_LABEL))
  {
     if(FindPubName(buff,sizeof(buff),r_sh))
     {
       strcat(str,buff);
       if(!DumpMode && !EditMode) GidAddGoAddress(str,r_sh);
       ret = RAPREF_DONE;
     }
  }
  return flags & APREF_TRY_LABEL ? ret ? RAPREF_DONE : RAPREF_NONE : ret;
}

static tBool __FASTCALL__ coff386_check_fmt( void )
{
  tUIntFast16 id;
  id = bmReadWordEx(0,SEEKF_START);
  return !(I386BADMAG(id));
}

static void __FASTCALL__ coff386_init_fmt( void )
{
  BGLOBAL main_handle;
  __filesize_t s_off = sizeof(coff386hdr);
  tUIntFast16 i;
  bmReadBufferEx(&coff386hdr,sizeof(struct external_filehdr),0,SEEKF_START);
  if(COFF_WORD(coff386hdr.f_opthdr)) bmReadBuffer(&coff386ahdr,sizeof(AOUTHDR));
  nsections = COFF_WORD(coff386hdr.f_nscns);
  if(!(coff386so = PMalloc((unsigned)(sizeof(SCNHDR)*nsections))))
  {
     MemOutBox("Coff386 initialization");
     exit(EXIT_FAILURE);
  }
  main_handle = bmbioHandle();
  if((coff_cache = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) coff_cache = main_handle;
  if(COFF_WORD(coff386hdr.f_opthdr)) s_off += COFF_WORD(coff386hdr.f_opthdr);
  bioSeek(coff_cache,s_off,BIO_SEEK_SET);
  for(i = 0;i < nsections;i++)
  {
    bioReadBuffer(coff_cache,&coff386so[i],sizeof(SCNHDR));
  }
  strings_ptr = COFF_DWORD(coff386hdr.f_symptr)+COFF_DWORD(coff386hdr.f_nsyms)*sizeof(struct external_syment);
}

static void __FASTCALL__ coff386_destroy_fmt( void )
{
  BGLOBAL main_handle;
  PFREE(coff386so);
  if(PubNames) { la_Destroy(PubNames); PubNames = 0; }
  main_handle = bmbioHandle();
  if(coff_cache != &bNull && coff_cache != main_handle) bioClose(coff_cache);
}

static int __FASTCALL__ coff386_bitness(__filesize_t off)
{
  UNUSED(off);
  return DAB_USE32;
}

static tBool __FASTCALL__ coff386_AddrResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  tBool bret = True;
  tUInt32 res;
  if(cfpos < sizeof(struct external_filehdr))
  {
    strcpy(addr,"coffih:");
    strcpy(&addr[7],Get2Digit(cfpos));
  }
  else
    if((res=coff386_PA2VA(cfpos))!=0)
    {
      addr[0] = '.';
      strcpy(&addr[1],Get8Digit(res));
    }
    else bret = False;
  return bret;
}

static __filesize_t __FASTCALL__ coff386Help( void )
{
  hlpDisplay(10002);
  return BMGetCurrFilePos();
}

static void __FASTCALL__ coff_ReadPubName(BGLOBAL b_cache,const struct PubName *it,
                           char *buff,unsigned cb_buff)
{
    if(!it->addinfo)
      coffReadLongName(b_cache,it->nameoff,buff,cb_buff);
    else
    {
      bioSeek(b_cache,it->nameoff,BIO_SEEK_SET);
      bioReadBuffer(b_cache,buff,it->addinfo);
      buff[it->addinfo] = 0;
    }
}

static void __FASTCALL__ coff_ReadPubNameList(BGLOBAL handle,
                                    void (__FASTCALL__ *mem_out)(const char *))
{
 unsigned i,nnames;
 struct PubName pn;
 if(!(PubNames = la_Build(0,sizeof(struct PubName),mem_out))) return;
 bioSeek(handle,COFF_DWORD(coff386hdr.f_symptr),SEEKF_START);
 nnames = coffSymTabNumItems(handle);
 for(i = 0;i < nnames;i++)
 {
   struct external_syment cse;
   __filesize_t where;
   tUIntFast16 sec_num;
   where = bioTell(handle);
   bioReadBuffer(handle,&cse,sizeof(struct external_syment));
   sec_num = COFF_WORD(cse.e_scnum);
   if(sec_num && sec_num <= COFF_WORD(coff386hdr.f_nscns) &&
     (cse.e_sclass[0] == C_EXT ||
      cse.e_sclass[0] == C_STAT ||
      cse.e_sclass[0] == C_HIDDEN ||
      cse.e_sclass[0] == C_LABEL))
   {
     if(COFF_DWORD(cse.e.e.e_zeroes) == 0L &&
        COFF_DWORD(cse.e.e.e_offset) >= 4)
     {
       pn.nameoff = COFF_DWORD(cse.e.e.e_offset);
       pn.addinfo = 0;
     }
     else
     {
       pn.nameoff = where;
       pn.addinfo = E_SYMNMLEN;
     }
     pn.pa = COFF_DWORD(coff386so[sec_num-1].s_scnptr)+COFF_DWORD(cse.e_value);
     pn.attr = cse.e_sclass[0] == C_EXT ? SC_GLOBAL : SC_LOCAL;
     if(!la_AddData(PubNames,&pn,mem_out)) break;
   }
   if(bioEOF(handle)) break;
 }
 la_Sort(PubNames,fmtComparePubNames);
}

static __filesize_t __FASTCALL__ coff386_GetPubSym(char *str,unsigned cb_str,unsigned *func_class,
                          __filesize_t pa,tBool as_prev)
{
  return fmtGetPubSym(coff_cache,str,cb_str,func_class,pa,as_prev,
                      coff_ReadPubNameList,
                      coff_ReadPubName);
}

static unsigned __FASTCALL__ coff386_GetObjAttr(__filesize_t pa,char *name,unsigned cb_name,
                            __filesize_t *start,__filesize_t *end,int *_class,int *bitness)
{
  unsigned ret;
  tUIntFast16 i;
  *start = 0;
  *end = bmGetFLength();
  *_class = OC_NOOBJECT;
  *bitness = DAB_USE32;
  name[0] = 0;
  ret = 0;
  for(i = 0;i < nsections;i++)
  {
    if(pa >= *start && pa < COFF_DWORD(coff386so[i].s_scnptr))
    {
      /** means between two objects */
      *end = COFF_DWORD(coff386so[i].s_scnptr);
      ret = 0;
      break;
    }
    if(pa >= COFF_DWORD(coff386so[i].s_scnptr) && pa < COFF_DWORD(coff386so[i].s_scnptr) + COFF_DWORD(coff386so[i].s_size))
    {
      *start = COFF_DWORD(coff386so[i].s_scnptr);
      *end = *start + COFF_DWORD(coff386so[i].s_size);
      *_class = (COFF_DWORD(coff386so[i].s_flags) & STYP_TEXT) == STYP_TEXT ? OC_CODE : OC_DATA;
      strncpy(name,(const char *)coff386so[i].s_name,min(sizeof(coff386so[i].s_name),cb_name));
      name[sizeof(coff386so[i].s_name)] = 0;
      ret = i+1;
      break;
    }
    *start = COFF_DWORD(coff386so[i].s_scnptr) + COFF_DWORD(coff386so[i].s_size);
  }
  return ret;
}

static int __FASTCALL__ coff386_platform( void ) { return DISASM_CPU_IX86; }

REGISTRY_BIN coff386Table =
{
  "coff-i386 (Common Object File Format)",
  { "CofHlp", NULL, NULL, NULL, NULL, NULL, "SymTab", NULL, NULL, "Objects" },
  { coff386Help, NULL, NULL, NULL, NULL, NULL, coffShowSymTab, NULL, NULL, coffShowObjects },
  coff386_check_fmt,
  coff386_init_fmt,
  coff386_destroy_fmt,
  ShowCoff386Header,
  coff386_AppendRef,
  fmtSetState,
  coff386_platform,
  coff386_bitness,
  NULL,
  coff386_AddrResolv,
  coff386_VA2PA,
  coff386_PA2VA,
  coff386_GetPubSym,
  coff386_GetObjAttr,
  NULL,
  NULL
};
