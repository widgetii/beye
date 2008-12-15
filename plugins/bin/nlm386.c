/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/nlm386.c
 * @brief       This file contains implementation of NLM-32 (Novell Loadable
 *              Module) file format decoder.
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
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "colorset.h"
#include "bin_util.h"
#include "plugins/disasm.h"
#include "plugins/bin/nlm386.h"
#include "codeguid.h"
#include "bmfile.h"
#include "bconsole.h"
#include "reg_form.h"
#include "tstrings.h"
#include "biewutil.h"
#include "biewhelp.h"
#include "biewlib/pmalloc.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

static Nlm_Internal_Fixed_Header nlm;

static tBool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa);
static void __FASTCALL__ nlm_ReadPubNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *));
static __filesize_t __FASTCALL__ NLMPA2VA(__filesize_t pa);

static BGLOBAL nlm_cache = &bNull;

static tBool __FASTCALL__ nlmLowMemFunc( unsigned long need_mem )
{
  UNUSED(need_mem);
  if(!fmtActiveState)
  {
    if(PubNames)
    {
       la_Destroy(PubNames);
       PubNames = NULL;
       return True;
    }
  }
  return False;
}

static __filesize_t __FASTCALL__ ShowNLMHeader( void )
{
  __filesize_t fpos;
  char modName[NLM_MODULE_NAME_SIZE];
  TWindow * w;
  unsigned keycode;
  fpos = BMGetCurrFilePos();
  w = CrtDlgWndnls(" NetWare Loadable Module ",59,23);
  twGotoXY(1,1);
  strncpy(modName,(char *)&nlm.nlm_moduleName[1],(int)nlm.nlm_moduleName[0]);
  modName[(unsigned)nlm.nlm_moduleName[0]] = 0;
  twPrintF("Module (Version)              = %s (%02hu.%02hu)\n"
           "Code image offset             = %08lXH\n"
           "Code image size               = %08lXH\n"
           "Data image offset             = %08lXH\n"
           "Data image size               = %08lXH\n"
           "Uninitialized data size       = %08lXH\n"
           "Custom data offset            = %08lXH\n"
           "Custom data size              = %08lXH\n"
           "Module dependency offset      = %08lXH\n"
           "Number of Module dependencies = %08lXH\n"
           "Relocation fixup offset       = %08lXH\n"
           "Number of relocations fixup   = %08lXH\n"
           "External reference offset     = %08lXH\n"
           "Number of external references = %08lXH\n"
           "Public offset                 = %08lXH\n"
           "Number of public              = %08lXH\n"
           "Debug info offset             = %08lXH\n"
           "Number of debug records       = %08lXH\n"
           ,modName
           ,(unsigned short)nlm.nlm_version
           ,(unsigned short)(nlm.nlm_version >> 16)
           ,nlm.nlm_codeImageOffset
           ,nlm.nlm_codeImageSize
           ,nlm.nlm_dataImageOffset
           ,nlm.nlm_dataImageSize
           ,nlm.nlm_uninitializedDataSize
           ,nlm.nlm_customDataOffset
           ,nlm.nlm_customDataSize
           ,nlm.nlm_moduleDependencyOffset
           ,nlm.nlm_numberOfModuleDependencies
           ,nlm.nlm_relocationFixupOffset
           ,nlm.nlm_numberOfRelocationFixups
           ,nlm.nlm_externalReferencesOffset
           ,nlm.nlm_numberOfExternalReferences
           ,nlm.nlm_publicsOffset
           ,nlm.nlm_numberOfPublics
           ,nlm.nlm_debugInfoOffset
           ,nlm.nlm_numberOfDebugRecords);
  twSetColorAttr(dialog_cset.entry);
  twPrintF("Code start offset             = %08lXH [Enter]",nlm.nlm_codeStartOffset);
  twClrEOL();
  twSetColorAttr(dialog_cset.altentry);
  twPrintF("\nExit procedure offset         = %08lXH [Ctrl+Enter | F5]",nlm.nlm_exitProcedureOffset);
  twClrEOL();
  twSetColorAttr(dialog_cset.main);
  twPrintF("\nCheck unload procedure offset = %08lXH\n"
           "Module type                   = %08lXH\n"
           "Flags                         = %08lXH"
           ,nlm.nlm_checkUnloadProcedureOffset
           ,nlm.nlm_moduleType
           ,nlm.nlm_flags);
  do
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    if(keycode == KE_ENTER)
    {
      fpos = nlm.nlm_codeImageOffset + nlm.nlm_codeStartOffset;
      break;
    }
    else
      if(keycode == KE_CTL_ENTER || keycode == KE_F(5))
      {
        fpos = nlm.nlm_codeImageOffset + nlm.nlm_exitProcedureOffset;
        break;
      }
  }while(!(keycode == KE_ESCAPE || keycode == KE_F(10)));
  CloseWnd(w);
  return fpos;
}

static __filesize_t __FASTCALL__ ShowNewNLM( void )
{
  __filesize_t fpos,ssize,m,d,sharedEntry,sharedExit;
  char modName[256];
  unsigned char len;
  TWindow *w;
  unsigned keycode;
  sharedEntry = sharedExit = 0;
  fpos = BMGetCurrFilePos();
  w = CrtDlgWndnls(" NetWare Loadable Module ",74,23);
  twGotoXY(1,1);
  bmSeek(sizeof(Nlm_Internal_Fixed_Header),BM_SEEK_SET);
  len = bmReadByte();
  bmReadBuffer(modName,len + 1);
  ssize = bmReadDWord();
  bmSeek(4,BM_SEEK_CUR); /** skip reserved */
  twPrintF("%s\n"
           "Stack size                    = %08lXH\n"
           ,modName
           ,ssize);
  bmReadBuffer(modName,5);
  modName[5] = 0;
  twPrintF("Old thread name               = %s\n",modName);
  len = bmReadByte();
  bmReadBuffer(modName,len + 1);
  twPrintF("Screen name                   = %s\n",modName);
  len = bmReadByte();
  bmReadBuffer(modName,len + 1);
  twPrintF("Thread name                   = %s",modName);
  while(1)
  {
    bmReadBuffer(modName,9);
    if(bmEOF()) break;
    modName[9] = 0;
    if(memcmp(modName,"VeRsIoN#",8) == 0)
    {
      bmSeek(-1,BM_SEEK_CUR);
      ssize = bmReadDWord();
      d = bmReadDWord();
      m = bmReadDWord();
      twPrintF("\nVersion ( Revision )          = %lu.%lu ( %08lXH )\n",ssize,d,m);
      ssize = bmReadDWord();
      m     = bmReadDWord();
      d     = bmReadDWord();
      twPrintF("Date (DD.MM.YY)               = %lu.%lu.%lu",d,m,ssize);
    }
    else
      if(memcmp(modName,"CoPyRiGhT",9) == 0)
      {
        bmSeek(1,BM_SEEK_CUR);
        len = bmReadByte();
        bmReadBuffer(modName,len + 1);
        twPrintF("\nCopyright = %s",modName);
      }
      else
        if(memcmp(modName,"MeSsAgEs",8) == 0)
        {
          bmSeek(-1,BM_SEEK_CUR);
          ssize = bmReadDWord();
          twPrintF("\nLanguage                      = %08lXH\n",ssize);
          ssize = bmReadDWord();
          m = bmReadDWord();
          d = bmReadDWord();
          twPrintF("Messages (offset/length/count)= %08lXH/%08lXH/%08lXH\n",ssize,m,d);
          ssize = bmReadDWord();
          m = bmReadDWord();
          d = bmReadDWord();
          twPrintF("Help (offset/length/dataOff)  = %08lXH/%08lXH/%08lXH\n",ssize,m,d);
          ssize = bmReadDWord();
          m = bmReadDWord();
          twPrintF("SharedCode (offset/length)    = %08lXH/%08lXH\n",ssize,m);
          ssize = bmReadDWord();
          m = bmReadDWord();
          twPrintF("SharedData (offset/length)    = %08lXH/%08lXH\n",ssize,m);
          ssize = bmReadDWord();
          m = bmReadDWord();
          twPrintF("SharedReloc (offset/count)    = %08lXH/%08lXH\n",ssize,m);
          ssize = bmReadDWord();
          m = bmReadDWord();
          twPrintF("SharedExtRef (offset/count)   = %08lXH/%08lXH\n",ssize,m);
          ssize = bmReadDWord();
          m = bmReadDWord();
          twPrintF("SharedPublics (offset/count)  = %08lXH/%08lXH\n",ssize,m);
          ssize = bmReadDWord();
          m = bmReadDWord();
          twPrintF("SharedDebugRec (offset/count) = %08lXH/%08lXH\n",ssize,m);
          sharedEntry = bmReadDWord();
          twSetColorAttr(dialog_cset.entry);
          twPrintF("Shared initialization offset  = %08lXH [Enter]",sharedEntry);
          twClrEOL(); twPrintF("\n");
          sharedExit = bmReadDWord();
          twSetColorAttr(dialog_cset.altentry);
          twPrintF("Shared exit procedure offset  = %08lXH [Ctrl+Enter | F5]",sharedExit);
          twClrEOL(); twPrintF("\n");
          twSetColorAttr(dialog_cset.main);
          ssize = bmReadDWord();
          twPrintF("Product ID                    = %08lXH",ssize);
        }
        else
          if(memcmp(modName,"CuStHeAd",8) == 0)
          {
            unsigned long hdr;
            bmSeek(-1,BM_SEEK_CUR);
            ssize = bmReadDWord();
            d = bmReadDWord();
            m = bmReadDWord();
            bmReadBuffer(modName,8);
            modName[8] = 0;
            hdr = bmReadDWord();
            twPrintF("\nCustHead (name/hdrOff/hdrLen/dataOff/dataLen) = %s/%08lXH/%08lXH/%08lXH/%08lHX",modName,hdr,ssize,d,m);
          }
          else
            if(memcmp(modName,"CyGnUsEx",8) == 0)
            {
              bmSeek(-1,BM_SEEK_CUR);
              d = bmReadDWord();
              m = bmReadDWord();
              twPrintF("\nCygnus (offset/length) = %08lXH/%08lXH",d,m);
            }
            else break;
  }
  do
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    if(keycode == KE_ENTER)
    {
       if(sharedEntry) fpos = sharedEntry + nlm.nlm_codeImageOffset;
       break;
    }
    else
      if((keycode == KE_CTL_ENTER || keycode == KE_F(5)) && sharedExit)
      {
        fpos = sharedExit + nlm.nlm_codeImageOffset;
        break;
      }
  }while(!(keycode == KE_ESCAPE || keycode == KE_F(10)));
  CloseWnd(w);
  return fpos;
}

static unsigned __FASTCALL__ NLMExtRefNumItems(BGLOBAL handle)
{
  UNUSED(handle);
  return (unsigned)nlm.nlm_numberOfExternalReferences;
}

static tBool __FASTCALL__ __ReadExtRefNamesNLM(BGLOBAL handle,memArray * obj,unsigned n)
{
 unsigned i;
 bioSeek(handle,nlm.nlm_externalReferencesOffset,SEEKF_START);
 for(i = 0;i < n;i++)
 {
   char stmp[256];
   unsigned char length;
   unsigned long nrefs;
   length = bioReadByte(handle);
   if(IsKbdTerminate() || bioEOF(handle)) break;
   bioReadBuffer(handle,stmp,length);
   stmp[length] = 0;
   nrefs = bioReadDWord(handle);
   bioSeek(handle,nrefs*4,SEEKF_CUR);
   if(!ma_AddString(obj,stmp,True)) break;
 }
 return True;
}

static __filesize_t __NEAR__ __FASTCALL__ CalcEntryNLM(unsigned ord,tBool dispmsg)
{
 unsigned char length;
 unsigned i;
 __filesize_t ret,fpos,cpos;
 fpos = BMGetCurrFilePos();
 cpos = nlm.nlm_publicsOffset;
 for(i = 0;i < ord;i++)
 {
   length = bmReadByteEx(cpos,SEEKF_START); cpos+=length + 5;
 }
 length = bmReadByteEx(cpos,SEEKF_START); cpos+=length + 1;
 ret = bmReadDWordEx(cpos,SEEKF_START);
 ret &= 0x00FFFFFFL;
 ret += nlm.nlm_codeImageOffset;
 bmSeek(fpos,SEEKF_START);
 if(ret > bmGetFLength())
 {
    ret = fpos;
    if(dispmsg) ErrMessageBox(NO_ENTRY,NULL);
 }
 return ret;
}

static unsigned __FASTCALL__ NLMNamesNumItems(BGLOBAL handle)
{
  UNUSED(handle);
  return (unsigned)nlm.nlm_numberOfPublics;
}

static tBool __FASTCALL__ NLMNamesReadItems(BGLOBAL handle,memArray * obj,unsigned nnames)
{
 unsigned char length;
 unsigned i;
 bioSeek(handle,nlm.nlm_publicsOffset,SEEKF_START);
 for(i = 0;i < nnames;i++)
 {
   char stmp[256];
   length = bioReadByte(handle);
   if(IsKbdTerminate() || bioEOF(handle)) break;
   if(length > 66)
   {
     bioReadBuffer(handle,stmp,66);
     bioSeek(handle,length - 66,SEEKF_CUR);
     strcat(stmp,">>>");
   }
   else { bioReadBuffer(handle,stmp,length); stmp[length] = 0; }
   bioSeek(handle,4L,SEEKF_CUR);
   if(!ma_AddString(obj,stmp,True)) break;
 }
 return True;
}

static __filesize_t __FASTCALL__ ShowExtRefNLM( void )
{
  fmtShowList(NLMExtRefNumItems,
              __ReadExtRefNamesNLM,
              EXT_REFER,
              LB_SORTABLE,
              NULL);
   return BMGetCurrFilePos();
}

static unsigned __FASTCALL__ NLMModRefNumItems(BGLOBAL handle)
{
  UNUSED(handle);
  return (unsigned)nlm.nlm_numberOfModuleDependencies;
}

static tBool __FASTCALL__ __ReadModRefNamesNLM(BGLOBAL handle,memArray * obj,unsigned nnames)
{
 unsigned char length;
 unsigned i;
 bioSeek(handle,nlm.nlm_moduleDependencyOffset,SEEKF_START);
 for(i = 0;i < nnames;i++)
 {
   char stmp[256];
   length = bioReadByte(handle);
   if(IsKbdTerminate() || bioEOF(handle)) break;
   if(length > 66)
   {
     bioReadBuffer(handle,stmp,66);
     bioSeek(handle,length - 66,SEEKF_CUR);
     strcat(stmp,">>>");
   }
   else { bioReadBuffer(handle,stmp,length); stmp[length] = 0; }
   if(!ma_AddString(obj,stmp,True)) break;
 }
 return True;
}


static __filesize_t __FASTCALL__ ShowModRefNLM( void )
{
  fmtShowList(NLMModRefNumItems,
              __ReadModRefNamesNLM,
              MOD_REFER,
              LB_SORTABLE,
              NULL);
   return BMGetCurrFilePos();
}

static __filesize_t __FASTCALL__ ShowPubNamNLM( void )
{
  __filesize_t fpos = BMGetCurrFilePos();
  int ret;
  ret = fmtShowList(NLMNamesNumItems,NLMNamesReadItems,
                    EXP_TABLE,
                    LB_SELECTIVE,NULL);
  if(ret != -1)
  {
    fpos = CalcEntryNLM(ret,True);
  }
  return fpos;
}

/***************************************************************************/
/************************  FOR NLM  ****************************************/
/***************************************************************************/
static char __codelen;

typedef struct tagRELOC_NLM
{
  unsigned long offset;
  unsigned long nameoff; /** if refnum == -1 then internal */
}RELOC_NLM;

static linearArray *RelocNlm = NULL;

static tCompare __FASTCALL__ nlm_compare_s(const void __HUGE__ *e1,const void __HUGE__ *e2)
{
  const RELOC_NLM __HUGE__ *r1,__HUGE__ *r2;
  r1 = e1;
  r2 = e2;
  return __CmpLong__(r1->offset,r2->offset);
}

static tCompare __FASTCALL__ nlm_compare_f(const void __HUGE__ *e1,const void __HUGE__ *e2)
{
  const RELOC_NLM __HUGE__ *r1,__HUGE__ *r2;
  r1 = e1;
  r2 = e2;
  return __CmpLong__(r1->offset,r2->offset);
}

static void __NEAR__ __FASTCALL__ BuildRelocNlm( void )
{
  unsigned i,j;
  unsigned long val,niter,noff;
  __filesize_t cpos;
  unsigned char len;
  TWindow * w,*usd;
  RELOC_NLM rel;
  usd = twUsedWin();
  if(!(RelocNlm = la_Build(0,sizeof(RELOC_NLM),MemOutBox))) return;
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  if(!PubNames) nlm_ReadPubNameList(bmbioHandle(),MemOutBox);
  twUseWin(w);
  twGotoXY(1,1);
  twPutS(BUILD_REFS);
  twUseWin(usd);
  /** -- for external references */
  cpos = nlm.nlm_externalReferencesOffset;
  for(j = 0;j < (unsigned)nlm.nlm_numberOfExternalReferences;j++)
  {
    tBool is_eof;
    noff = cpos;
    is_eof = False;
    len = bmReadByteEx(cpos,SEEKF_START); cpos += len + 1;
    niter = bmReadDWordEx(cpos,SEEKF_START); cpos += 4;
    for(i = 0;i < niter;i++)
    {
      val = bmReadDWordEx(cpos,SEEKF_START); cpos += 4;
      if((is_eof = bmEOF()) != 0) break;
      rel.offset = (val&0x00FFFFFFL) + nlm.nlm_codeImageOffset;
      rel.nameoff = noff;
      if(!la_AddData(RelocNlm,&rel,MemOutBox)) goto next;
    }
    if(is_eof) break;
  }
  /** -- for internal references */
  cpos = nlm.nlm_relocationFixupOffset;
  for(j = 0;j < (unsigned)nlm.nlm_numberOfRelocationFixups;j++)
  {
    val = bmReadDWord();
    if(bmEOF()) break;
    rel.offset = (val&0x00FFFFFFL) + nlm.nlm_codeImageOffset;
    rel.nameoff = -1;
    if(!la_AddData(RelocNlm,&rel,MemOutBox)) break;
  }
  next:
  la_Sort(RelocNlm,nlm_compare_s);
  CloseWnd(w);
}

static __filesize_t __NEAR__ __FASTCALL__ BuildReferStrNLM(char *str,RELOC_NLM*rne,int flags)
{
  __filesize_t val;
  __filesize_t retrf;
  char name[256];
  BGLOBAL b_cache;
  unsigned char len;
  b_cache = nlm_cache;
  bioSeek(b_cache,rne->nameoff,BM_SEEK_SET);
  retrf = RAPREF_DONE;
  if(rne->nameoff != 0xFFFFFFFFUL)
  {
    len = bioReadByte(b_cache);
    bioReadBuffer(b_cache,name,len);
    name[len] = 0;
    strcat(str,name);
  }
  else
  {
    val = bmReadDWordEx(rne->offset,SEEKF_START);
    if(FindPubName(name,sizeof(name),val))
    {
      strcat(str,name);
    }
    else
     if(!(flags & APREF_SAVE_VIRT))
     {
       strcat(str,"(*this)+");
       strcat(str,Get8Digit(val));
       retrf = val;
     }
     else retrf = RAPREF_NONE;
  }
  return retrf;
}

static unsigned long __FASTCALL__ AppendNLMRef(char *str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  RELOC_NLM *rnlm,key;
  unsigned long retrf;
  char buff[400];
  if(flags & APREF_TRY_PIC) return RAPREF_NONE;
  if(!nlm.nlm_numberOfExternalReferences || nlm.nlm_externalReferencesOffset >= bmGetFLength()) retrf = RAPREF_NONE;
  else
  {
    if(!RelocNlm) BuildRelocNlm();
    key.offset = ulShift;
    __codelen = codelen;
    rnlm = la_Find(RelocNlm,&key,nlm_compare_f);
    retrf = rnlm ? BuildReferStrNLM(str,rnlm,flags) : RAPREF_NONE;
  }
  if(!retrf && (flags & APREF_TRY_LABEL))
  {
     if(!PubNames) nlm_ReadPubNameList(bmbioHandle(),MemOutBox);
     if(FindPubName(buff,sizeof(buff),r_sh))
     {
       strcat(str,buff);
       if(!DumpMode && !EditMode) GidAddGoAddress(str,r_sh);
       retrf = RAPREF_DONE;
     }
  }
  return retrf;
}

static tBool __FASTCALL__ IsNLM( void )
{
  char ctrl[NLM_SIGNATURE_SIZE];
  bmReadBufferEx(ctrl,NLM_SIGNATURE_SIZE,0,BM_SEEK_SET);
  return memcmp(ctrl,NLM_SIGNATURE,NLM_SIGNATURE_SIZE) == 0;
}

static void __FASTCALL__ NLMinit( void )
{
  BGLOBAL main_handle;
  PMRegLowMemCallBack(nlmLowMemFunc);
  bmReadBufferEx(&nlm,sizeof(Nlm_Internal_Fixed_Header),0,SEEKF_START);
  main_handle = bmbioHandle();
  if((nlm_cache = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) nlm_cache = main_handle;
}

static void __FASTCALL__ NLMdestroy( void )
{
  BGLOBAL main_handle;
  main_handle = bmbioHandle();
  if(nlm_cache != &bNull && nlm_cache != main_handle) bioClose(nlm_cache);
  PMUnregLowMemCallBack(nlmLowMemFunc);
}

static int __FASTCALL__ NLMbitness(__filesize_t off)
{
  UNUSED(off);
  return DAB_USE32;
}

static tBool __FASTCALL__ NLMAddrResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  tBool bret = True;
  tUInt32 res;
  if(cfpos < sizeof(Nlm_Internal_Fixed_Header))
  {
    strcpy(addr,"nlm32h:");
    strcpy(&addr[7],Get2Digit(cfpos));
  }
  else
    if((res=NLMPA2VA(cfpos)) != 0)
    {
      addr[0] = '.';
      strcpy(&addr[1],Get8Digit(res));
    }
    else bret = False;
  return bret;
}

static __filesize_t __FASTCALL__ HelpNLM( void )
{
  hlpDisplay(10007);
  return BMGetCurrFilePos();
}

static void __FASTCALL__ nlm_ReadPubName(BGLOBAL b_cache,const struct PubName *it,
                            char *buff,unsigned cb_buff)
{
    unsigned char length;
    bioSeek(b_cache,it->nameoff,SEEK_SET);
    length = bioReadByte(b_cache);
    length = min(length,cb_buff);
    bioReadBuffer(b_cache,buff,length);
    buff[length] = 0;
}

static tBool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa)
{
  return fmtFindPubName(nlm_cache,buff,cb_buff,pa,
                        nlm_ReadPubNameList,
                        nlm_ReadPubName);
}

static void __FASTCALL__ nlm_ReadPubNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *))
{
 unsigned char length;
 unsigned i;
 unsigned nnames = (unsigned)nlm.nlm_numberOfPublics;
 if(!PubNames)
   if(!(PubNames = la_Build(0,sizeof(struct PubName),mem_out))) return;
 bioSeek(handle,nlm.nlm_publicsOffset,SEEKF_START);
 for(i = 0;i < nnames;i++)
 {
   struct PubName nlm_pn;
   nlm_pn.nameoff = bioTell(handle);
   length         = bioReadByte(handle);
   bioSeek(handle,length,SEEK_CUR);
   nlm_pn.pa      = (bioReadDWord(handle) & 0x00FFFFFFL) + nlm.nlm_codeImageOffset;
   nlm_pn.attr    = SC_GLOBAL;
   if(!la_AddData(PubNames,&nlm_pn,mem_out)) break;
   if(bioEOF(handle)) break;
 }
 if(PubNames->nItems) la_Sort(PubNames,fmtComparePubNames);
}

static __filesize_t __FASTCALL__ NLMGetPubSym(char *str,unsigned cb_str,unsigned *func_class,
                           __filesize_t pa,tBool as_prev)
{
  return fmtGetPubSym(nlm_cache,str,cb_str,func_class,pa,as_prev,
                      nlm_ReadPubNameList,
                      nlm_ReadPubName);
}

static unsigned __FASTCALL__ NLMGetObjAttr(__filesize_t pa,char *name,unsigned cb_name,
                      __filesize_t *start,__filesize_t *end,int *_class,int *bitness)
{
  unsigned ret;
  UNUSED(cb_name);
  *start = 0;
  *end = bmGetFLength();
  *_class = OC_NOOBJECT;
  *bitness = NLMbitness(pa);
  name[0] = 0;
  if(pa < nlm.nlm_codeImageOffset)
  {
    *end = nlm.nlm_codeImageOffset;
    ret = 0;
  }
  else
    if(pa >= nlm.nlm_codeImageOffset && pa < nlm.nlm_codeImageOffset + nlm.nlm_codeImageSize)
    {
      *start = nlm.nlm_codeImageOffset;
      *_class = OC_CODE;
      *end = nlm.nlm_codeImageOffset + nlm.nlm_codeImageSize;
      ret = 1;
    }
    else
    if(pa >= nlm.nlm_dataImageOffset && pa < nlm.nlm_dataImageOffset + nlm.nlm_dataImageSize)
    {
      *_class = OC_DATA;
      *start = nlm.nlm_dataImageOffset;
      *end = *start + nlm.nlm_dataImageSize;
      ret = 2;
    }
    else
    {
      *_class = OC_NOOBJECT;
      *start = nlm.nlm_dataImageOffset + nlm.nlm_dataImageSize;
      ret = 3;
    }
  return ret;
}

static __filesize_t __FASTCALL__ NLMVA2PA(__filesize_t va)
{
  return va + min(nlm.nlm_codeImageOffset,nlm.nlm_dataImageOffset);
}

static __filesize_t __FASTCALL__ NLMPA2VA(__filesize_t pa)
{
  __filesize_t base = min(nlm.nlm_codeImageOffset,nlm.nlm_dataImageOffset);
  return pa > base ? pa - base : 0L;
}

static int __FASTCALL__ NLMPlatform( void ) { return DISASM_CPU_IX86; }

REGISTRY_BIN nlm386Table =
{
  "nlm-i386 (Novell Loadable Module)",
  { "NlmHlp", "ModRef", "PubDef", NULL, "ExtNam", NULL, NULL, "NlmHdr", NULL, NULL },
  { HelpNLM, ShowModRefNLM, ShowPubNamNLM, NULL, ShowExtRefNLM, NULL, NULL, ShowNewNLM, NULL, NULL },
  IsNLM, NLMinit, NLMdestroy,
  ShowNLMHeader,
  AppendNLMRef,
  fmtSetState,
  NLMPlatform,
  NLMbitness,
  NULL,
  NLMAddrResolv,
  NLMVA2PA,
  NLMPA2VA,
  NLMGetPubSym,
  NLMGetObjAttr,
  NULL,
  NULL
};
