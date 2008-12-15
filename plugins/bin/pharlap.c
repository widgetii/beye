/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/pharlap.c
 * @brief       This file contains implementation of PharLap file format decoder.
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "colorset.h"
#include "bconsole.h"
#include "bin_util.h"
#include "biewutil.h"
#include "biewhelp.h"
#include "tstrings.h"
#include "bmfile.h"
#include "reg_form.h"
#include "plugins/bin/pharlap.h"
#include "plugins/disasm.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

static newPharLap nph;

static BGLOBAL pl_cache = &bNull;

static __filesize_t __FASTCALL__ ShowPharLapHeader( void )
{
  __filesize_t fpos;
  TWindow *w;
  unsigned keycode;
  char sign[3];
  fpos = BMGetCurrFilePos();
  strncpy(sign,(char *)nph.plSignature,2);
  sign[2] = 0;
  w = CrtDlgWndnls(" New PharLap executable ",59,23);
  twGotoXY(1,1);
  twPrintF("Signature                        = %s\n"
           "Level                            = %s\n"
           "Header size                      = %04XH\n"
           "File size                        = %08lXH\n"
           "Check sum (32-bit checksum)      = %04XH (%08lXH)\n"
           "Run-time parameters (offset/size)= %08lXH/%08lXH\n"
           "Relocations (offset/size)        = %08lXH/%08lXH\n"
           "Segment info table (offset/size) = %08lXH/%08lXH\n"
           "Image (offset/size)              = %08lXH/%08lXH\n"
           "Symbol table (offset/size)       = %08lXH/%08lXH\n"
           "GDT (offset/size)                = %08lXH/%08lXH\n"
           "LDT (offset/size)                = %08lXH/%08lXH\n"
           "IDT (offset/size)                = %08lXH/%08lXH\n"
           "TSS (offset/size)                = %08lXH/%08lXH\n"
           "Min. number of extra 4K pages    = %08lXH\n"
           "Max. number of extra 4K pages    = %08lXH\n"
           "Image base (flat level only)     = %08lXH\n"
           "Initial stack (SS:ESP)           = %04XH:%08lXH\n"
           "Initail code  (CS:EIP)           = %04XH:%08lXH\n"
           "Initial LDT/TSS                  = %04XH/%04XH\n"
           "Flags                            = %04XH\n"
           "Memory requirement for image     = %08lXH\n"
           "Stack size                       = %08lXH"
           ,sign
           ,nph.plLevel == 0x01 ? "Flat" : nph.plLevel == 0x02 ? "Multisegmented" : "Unknown"
           ,nph.plHeaderSize
           ,nph.plFileSize
           ,nph.plCheckSum,nph.plChecksum32
           ,nph.plRunTimeParms,nph.plRunTimeSize
           ,nph.plRelocOffset,nph.plRelocSize
           ,nph.plSegInfoOffset,nph.plSegInfoSize
           ,nph.plImageOffset,nph.plImageSize
           ,nph.plSymTabOffset,nph.plSymTabSize
           ,nph.plGDTOffset,nph.plGDTSize
           ,nph.plLDTOffset,nph.plLDTSize
           ,nph.plIDTOffset,nph.plIDTSize
           ,nph.plTSSOffset,nph.plTSSSize
           ,nph.plMinExtraPages
           ,nph.plMaxExtraPages
           ,nph.plBase
           ,nph.plSS,nph.plESP
           ,nph.plCS,nph.plEIP
           ,nph.plLDT,nph.plTSS
           ,nph.plFlags
           ,nph.plMemReq
           ,nph.plStackSize);
  while(1)
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    if(keycode == KE_ENTER) { /* fpos = entrypoint*/; break; }
    else
      if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
  }
  CloseWnd(w);
  return fpos;
}

static void __FASTCALL__ PLSegPaint(TWindow * win,const void ** names,unsigned start,unsigned nlist)
{
 char buffer[81];
 const PLSegInfo ** nam = (const PLSegInfo **)names;
 twUseWin(win);
 twFreezeWin(win);
 twClearWin();
 sprintf(buffer," Segment Table [ %u / %u ] ",start + 1,nlist);
 twSetTitleAttr(win,buffer,TW_TMODE_CENTER,dialog_cset.title);
 twSetFooterAttr(win,PAGEBOX_SUB,TW_TMODE_RIGHT,dialog_cset.selfooter);
 twGotoXY(1,1);
 twPrintF("Selector number            = %04hXH\n"
          "Flags                      = %04hXH\n"
          "Base offset of selector    = %08lXH\n"
          "Min extra memory alloc     = %08lXH"
          ,nam[start]->siSelector
          ,nam[start]->siFlags
          ,nam[start]->siBaseOff
          ,nam[start]->siMinAlloc);
 twRefreshFullWin(win);
}

static tBool __FASTCALL__ __PLReadSegInfo(BGLOBAL handle,memArray * obj,unsigned nnames)
{
 unsigned i;
 for(i = 0;i < nnames;i++)
 {
   PLSegInfo plsi;
   if(IsKbdTerminate() || bioEOF(handle)) break;
   bioReadBuffer(handle,&plsi,sizeof(PLSegInfo));
   if(!ma_AddData(obj,&plsi,sizeof(PLSegInfo),True)) break;
 }
 return True;
}

static __filesize_t __FASTCALL__ PharLapSegInfo( void )
{
 BGLOBAL handle;
 unsigned nnames;
 __filesize_t fpos;
 memArray * obj;
 if(nph.plSegInfoOffset && nph.plSegInfoSize) nnames = (unsigned)(nph.plSegInfoSize / sizeof(PLSegInfo));
 else                                           nnames = 0;
 fpos = BMGetCurrFilePos();
 if(!nnames) { NotifyBox(NOT_ENTRY," Segment Info table "); return fpos; }
 if(!(obj = ma_Build(nnames,True))) return fpos;
 handle = pl_cache;
 bioSeek(handle,nph.plSegInfoOffset,SEEK_SET);
 if(__PLReadSegInfo(handle,obj,nnames))
 {
    int i;
    i = PageBox(50,4,(const void **)obj->data,obj->nItems,PLSegPaint) + 1;
    if(i > 0)
    {
      fpos = ((__filesize_t)((PLSegInfo *)obj->data[i - 1])->siBaseOff)+nph.plImageOffset;
    }
 }
 ma_Destroy(obj);
 return fpos;
}

static void __FASTCALL__ PLRunTimePaint(TWindow * win,const void ** names,unsigned start,unsigned nlist)
{
 char buffer[81];
 char sign[3];
 const PLRunTimeParms ** nam = (const PLRunTimeParms **)names;
 twUseWin(win);
 twFreezeWin(win);
 twClearWin();
 sprintf(buffer," Run-time Parameters Table [ %u / %u ] ",start + 1,nlist);
 twSetTitleAttr(win,buffer,TW_TMODE_CENTER,dialog_cset.title);
 twSetFooterAttr(win,PAGEBOX_SUB,TW_TMODE_RIGHT,dialog_cset.selfooter);
 strncpy(sign,(const char *)nam[start]->rtSignature,2);
 sign[2] = 0;
 twGotoXY(1,1);
 twPrintF("Signature                      = %s\n"
          "Min. number of real-mode parms = %04hXH\n"
          "Max. number of real-mode parms = %04hXH\n"
          "Min. interrupt buffer size     = %04hXH\n"
          "Max. interrupt buffer size     = %04hXH\n"
          "Number of interrupt stacks     = %04hXH\n"
          "Size of each interrupt stack   = %04hXH\n"
          "Offset of end of real-mode data= %08lXH\n"
          "Call buffer size               = %04hXH\n"
          "Flags                          = %04hXH\n"
          "Unpriviledge flags             = %04hXH"
          ,sign
          ,nam[start]->rtMinRModeParms
          ,nam[start]->rtMaxRModeParms
          ,nam[start]->rtMinIBuffSize
          ,nam[start]->rtMaxIBuffSize
          ,nam[start]->rtNIStacks
          ,nam[start]->rtIStackSize
          ,nam[start]->rtEndRModeOffset
          ,nam[start]->rtCallBuffSize
          ,nam[start]->rtFlags
          ,nam[start]->rtUnprivFlags);
 twRefreshFullWin(win);
}

static tBool __FASTCALL__ __PLReadRunTime(BGLOBAL handle,memArray * obj,unsigned nnames)
{
 unsigned i;
 for(i = 0;i < nnames;i++)
 {
   PLRunTimeParms plrtp;
   if(IsKbdTerminate() || bioEOF(handle)) break;
   bioReadBuffer(handle,&plrtp,sizeof(PLRunTimeParms));
   if(!ma_AddData(obj,&plrtp,sizeof(PLRunTimeParms),True)) break;
 }
 return True;
}

static __filesize_t __FASTCALL__ PharLapRunTimeParms( void )
{
 BGLOBAL handle;
 unsigned nnames;
 __filesize_t fpos;
 memArray * obj;
 if(nph.plRunTimeParms && nph.plRunTimeSize) nnames = (unsigned)(nph.plRunTimeSize / sizeof(PLRunTimeParms));
 else                                          nnames = 0;
 fpos = BMGetCurrFilePos();
 if(!nnames) { NotifyBox(NOT_ENTRY," Run-time parameters "); return fpos; }
 if(!(obj = ma_Build(nnames,True))) return fpos;
 handle = pl_cache;
 bioSeek(handle,nph.plRunTimeParms,SEEK_SET);
 if(__PLReadRunTime(handle,obj,nnames))
 {
    int i;
    i = PageBox(50,11,(const void **)obj->data,obj->nItems,PLRunTimePaint) + 1;
    if(i > 0)
    {
      fpos = nph.plRunTimeParms+i*sizeof(PLRunTimeParms);
    }
 }
 ma_Destroy(obj);
 return fpos;
}

static tBool __FASTCALL__ IsPharLap( void )
{
   char sign[2];
   bmReadBufferEx(sign,2,0,BM_SEEK_SET);
   if(sign[0] == 'P' && (sign[1] == '2' || sign[1] == '3')) return True;
   return False;
}

static void __FASTCALL__ PharLapInit( void )
{
  BGLOBAL main_handle;
  bmReadBufferEx(&nph,sizeof(nph),0,BM_SEEK_SET);
  main_handle = bmbioHandle();
  if((pl_cache = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) pl_cache = main_handle;
}

static void __FASTCALL__ PharLapDestroy( void )
{
  BGLOBAL main_handle;
  main_handle = bmbioHandle();
  if(pl_cache != &bNull && pl_cache != main_handle) bioClose(pl_cache);
}

static tBool __FASTCALL__ PharLapAddrResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  tBool bret = True;
  if(cfpos < sizeof(newPharLap))
  {
    strcpy(addr,"nplhdr:");
    strcpy(&addr[7],Get2Digit(cfpos));
  }
  else bret = False;
  return bret;
}

static __filesize_t __FASTCALL__ PharLapHelp( void )
{
  hlpDisplay(10010);
  return BMGetCurrFilePos();
}

static int __FASTCALL__ PharLapPlatform( void ) { return DISASM_CPU_IX86; }

REGISTRY_BIN PharLapTable =
{
  "PharLap",
  { "PLHelp", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "RunTim", "SegInf" },
  { PharLapHelp, NULL, NULL, NULL, NULL, NULL, NULL, NULL, PharLapRunTimeParms, PharLapSegInfo },
  IsPharLap,
  PharLapInit,
  PharLapDestroy,
  ShowPharLapHeader,
  NULL,
  NULL,
  PharLapPlatform,
  NULL,
  NULL,
  PharLapAddrResolv,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
