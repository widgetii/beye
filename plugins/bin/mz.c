/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/mz.c
 * @brief       This file contains implementation of MZ file format.
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
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "bconsole.h"
#include "bmfile.h"
#include "beyehelp.h"
#include "bin_util.h"
#include "colorset.h"
#include "codeguid.h"
#include "reg_form.h"
#include "tstrings.h"
#include "libbeye/kbd_code.h"
#include "libbeye/pmalloc.h"
#include "plugins/disasm.h"
#include "plugins/bin/mz.h"

static MZHEADER mz;
static unsigned long HeadSize;

static __filesize_t __FASTCALL__ mzVA2PA(__filesize_t va)
{
  return va >= HeadSize ? va + HeadSize : 0L;
}

static __filesize_t __FASTCALL__ mzPA2VA(__filesize_t pa)
{
  return pa >= HeadSize ? pa - HeadSize : 0L;
}

static const char * __NEAR__ __FASTCALL__ __QueryAddInfo( unsigned char *memmap )
{
  static char rbuff[41];
  unsigned long idl;
  unsigned short idw,idw0;
  idl = ((unsigned long *)memmap)[0];
  idw0 = ((unsigned short *)memmap)[0];
  idw = ((unsigned short *)memmap)[2];
  if(memcmp(memmap,"RJSX",4) == 0) { ArjARC: return "ARJ self-extracting archive"; }
  else
    if(memcmp(memmap,"LZ09",4) == 0) return "LZEXE 0.90 compressed executable";
    else
      if(memcmp(memmap,"LZ91",4) == 0) return "LZEXE 0.91 compressed executable";
      else
        if(memmap[2] == 0xFB)
        {
           char hi,low;
           hi = (memmap[3] >> 4) & 0x0F;
           low = memmap[3] & 0x0F;
           sprintf(rbuff,"Borland TLINK version: %u.%u",(unsigned)hi,(unsigned)low);
           return rbuff;
        }
        else
          if(memcmp(&memmap[2],"PKLITE",6) == 0)
          {
             char hi, low;
             low = memmap[0];
             hi =  memmap[1] & 0x0F;
             sprintf(rbuff,"PKLITE v%u.%u compressed executable",(unsigned)hi,(unsigned)low);
             return rbuff;
          }
          else
            if(memcmp(&memmap[9],"LHarc's SFX",11) == 0) return "LHarc 1.x self-extracting archive";
            else
              if(memcmp(&memmap[8],"LHa's SFX",9) == 0) return "LHa 2.x self-extracting archive";
              else
                if(idl == 0x018A0001L && idw == 0x1565) return "TopSpeed 3.0 CRUNCH compressed file";
                else
                  if(idl == 0x00020001L && idw == 0x0700) return "PKARCK 3.5 self-extracting-archive";
                  else
                    if(idw0 == 0x000F && memmap[2] == 0xA7) return "BSA (Soviet archiver) selft-extarcting";
                    else
                      if(memcmp(&memmap[4],"SFX by LARC",11) == 0) return "LARC self-extracting archive";
                      else
                        if(memcmp(&memmap[8],"LH's SFX",8) == 0) return "LH self-extracting archive";
                        else
                        {
                          unsigned i;
                          for(i = 0;i < 1000-6;i++)
                          {
                            if(memmap[i] == 'a' && memcmp(&memmap[i+1],"RJsfX",5) == 0)
                            {
                               goto ArjARC;
                            }
                          }
                        }
 return 0;
}

static const char * __NEAR__ __FASTCALL__ QueryAddInfo( void )
{
   unsigned char *memmap;
   memmap = PMalloc(1000);
   if(memmap)
   {
     const char *ret;
     __filesize_t fpos;
     fpos = bmGetCurrFilePos();
     bmReadBufferEx(memmap,1000,0x1C,BM_SEEK_SET);
     bmSeek(fpos,BM_SEEK_SET);
     ret = __QueryAddInfo(memmap);
     PFREE(memmap);
     return ret;
   }
   return NULL;
}

static __filesize_t __FASTCALL__ ShowMZHeader( void )
{
 unsigned keycode;
 TWindow * hwnd;
 __filesize_t newcpos,fpos;
 unsigned long FPageCnt;
 const char * addinfo;
 fpos = BMGetCurrFilePos();
 keycode = 16;
 if(IsNewExe()) keycode++;
 addinfo = QueryAddInfo();
 if(addinfo) keycode++;
 hwnd = CrtDlgWndnls(" Old Exe Header ",43,keycode-1);
 FPageCnt =  ((long)mz.mzPageCount - 1)*512;
 twUseWin(hwnd);
 twGotoXY(1,1);
 twPrintF("Signature            = 'MZ'\n"
          "Part Last Page       = %hu [ bytes ]\n"
          "Page count           = %hu [ pages ]\n"
          "Relocations count    = %hu\n"
          "Header size          = %hu [ paragraphs ]\n"
          "Minimum memory       = %04hXH [ paragraphs ]\n"
          "Maximum memory       = %04hXH [ paragraphs ]\n"
          "SS : SP              = %04hX:%04hXH\n"
          "Check summ           = %hu\n"
          "CS : IP              = %04hX:%04hXH\n"
          "Table offset         = %04hXH [ bytes ]\n"
          "Overlay Number       = %hu\n"
          ,mz.mzPartLastPage
          ,mz.mzPageCount
          ,mz.mzRelocationCount
          ,mz.mzHeaderSize
          ,mz.mzMinMem
          ,mz.mzMaxMem
          ,mz.mzRelocationSS,mz.mzExeSP
          ,mz.mzCheckSumm
          ,mz.mzRelocationCS,mz.mzExeIP
          ,mz.mzTableOffset
          ,mz.mzOverlayNumber);
 newcpos = HeadSize;
 newcpos += (((unsigned long)mz.mzRelocationCS) << 4) + (unsigned long)mz.mzExeIP;
 twSetColorAttr(dialog_cset.entry);
 twPrintF(">Entry Point         = %08lXH",newcpos); twClrEOL();
 twSetColorAttr(dialog_cset.addinfo);
 twPrintF("\nModule Length        = %lu [ bytes ]",(FPageCnt - HeadSize) + mz.mzPartLastPage);
 twClrEOL();
 twSetColorAttr(dialog_cset.main);
 twPrintF("\nImage offset         = %08lXH",(long)HeadSize);
 if(headshift)
 {
   twSetColorAttr(dialog_cset.altinfo);
   twPrintF("\nNew EXE header shift = %08lXH",(long)headshift);
   twClrEOL();
 }
 if(addinfo)
 {
   twSetColorAttr(dialog_cset.extrainfo);
   twPrintF("\n%s",addinfo);
   twClrEOL();
 }
 while(1)
 {
   keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
   if(keycode == KE_F(5) || keycode == KE_ENTER) { fpos = newcpos; break; }
   else
     if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
 }
 CloseWnd(hwnd);
 return fpos;
}

long __HUGE__ * CurrMZChain = 0;
static unsigned long CurrMZCount;
static char __codelen;

static tCompare __FASTCALL__ compare_ptr(const void __HUGE__ *e1,const void __HUGE__ *e2)
{
  unsigned long v1,v2;
  v1 = *((const unsigned long __HUGE__ *)e1);
  v2 = *((const unsigned long __HUGE__ *)e2);
  return __CmpLong__(v1,v2);
}

static void __NEAR__ __FASTCALL__ BuildMZChain( void )
{
  unsigned i;
  __filesize_t fpos;
  TWindow * w,*usd;
  usd = twUsedWin();
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  twUseWin(w);
  twGotoXY(1,1);
  twPutS(BUILD_REFS);
  twUseWin(usd);
  CurrMZCount = 0;
  fpos = bmGetCurrFilePos();
  for(i = 0;i < mz.mzRelocationCount;i++)
  {
    unsigned off,seg,j;
    __filesize_t ptr;
    void __HUGE__ * tptr;
    if(!CurrMZChain) tptr = PHMalloc(sizeof(void *));
    else             tptr = PHRealloc(CurrMZChain,(CurrMZCount + 1)*sizeof(void *));
    if(!tptr) break;
    CurrMZChain = tptr;
    j = mz.mzTableOffset + i*4;
    bmSeek(j,BM_SEEK_SET);
    off = bmReadWord();
    seg = bmReadWord();
    ptr = (((long)seg) << 4) + off + (((long)mz.mzHeaderSize) << 4);
    CurrMZChain[CurrMZCount++] = ptr;
  }
  HQSort(CurrMZChain,CurrMZCount,sizeof(void *),compare_ptr);
  bmSeek(fpos,BM_SEEK_SET);
  CloseWnd(w);
}

static tCompare __FASTCALL__ compare_mz(const void __HUGE__ *e1,const void __HUGE__ *e2)
{
  long l1,l2;
  tCompare ret;
  l1 = *(const long __HUGE__ *)e1;
  l2 = *(const long __HUGE__ *)e2;
  if(l1 >= l2 && l1 < l2 + __codelen) ret = 0;
  else
    if(l1 < l2) ret = -1;
    else        ret = 1;
  return ret;
}

static tBool __NEAR__ __FASTCALL__ isMZReferenced(__filesize_t shift,char len)
{
  if(mz.mzRelocationCount)
  {
     __filesize_t mz_size;
     mz_size = (long)(mz.mzPageCount)*512 + mz.mzPartLastPage;
     if(shift <= mz_size && shift >= ((unsigned long)mz.mzHeaderSize) << 4)
     {
       if(!CurrMZChain) BuildMZChain();
       __codelen = len;
       return HLFind(&shift,CurrMZChain,CurrMZCount,sizeof(long),compare_mz) != 0;
     }
  }
  return False;
}
static unsigned long __FASTCALL__ AppendMZRef(char *str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  char stmp[256];
  unsigned long ret = RAPREF_NONE;
  if(flags & APREF_TRY_PIC) return RAPREF_NONE;
  if(isMZReferenced(ulShift,codelen))
  {
     unsigned wrd;
     wrd = bmReadWordEx(ulShift,BM_SEEK_SET);
     strcat(str,Get4Digit(wrd));
     strcat(str,"+PID");
     ret = RAPREF_DONE;
  }
  if(!DumpMode && !EditMode && (flags & APREF_TRY_LABEL) && codelen == 4)
  {
    r_sh += (((__filesize_t)mz.mzHeaderSize) << 4);
    if(udnFindName(r_sh,stmp,sizeof(stmp))==True) strcat(str,stmp);
    else strcat(str,Get8Digit(r_sh));
    GidAddGoAddress(str,r_sh);
    ret = RAPREF_DONE;
  }
  return ret;
}

static tBool  __FASTCALL__ mz_check_fmt( void )
{
  unsigned char id[2];
  tBool ret = False;
  bmReadBufferEx(id,sizeof(id),0,BM_SEEK_SET);
  if((id[0] == 'M' && id[1] == 'Z') ||
     (id[0] == 'Z' && id[1] == 'M'))
  {
    bmReadBufferEx((void  *)&mz,sizeof(MZHEADER),2,BM_SEEK_SET);
    HeadSize = ((unsigned long)mz.mzHeaderSize) << 4;
    ret = True;
  }
  return ret;
}

/* Special case: this module must not use init and destroy */
static void __FASTCALL__ mz_init_fmt( void ) {}
static void __FASTCALL__ mz_destroy_fmt(void) {}
static int  __FASTCALL__ mz_platform( void) { return DISASM_CPU_IX86; }

static tBool __FASTCALL__ mzAddressResolv(char *addr,__filesize_t cfpos)
{
  tBool bret = True;
  if(cfpos < sizeof(MZHEADER)+2) sprintf(addr,"MZH :%s",Get4Digit(cfpos));
  else
    if(cfpos >= sizeof(MZHEADER)+2 && cfpos < sizeof(MZHEADER)+2+(mz.mzRelocationCount<<2))
    {
      sprintf(addr,"MZRl:%s",Get4Digit(cfpos - sizeof(MZHEADER)));
    }
    else
     if(cfpos >= HeadSize)
     {
       addr[0] = '.';
       strcpy(&addr[1],Get8Digit(mzPA2VA(cfpos)));
     }
     else bret = False;
  return bret;
}

static __filesize_t __FASTCALL__ MZHelp( void )
{
  hlpDisplay(10013);
  return BMGetCurrFilePos();
}

REGISTRY_BIN mzTable =
{
  "MZ (Old DOS-exe)",
  { "MZHelp", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { MZHelp, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  mz_check_fmt,
  mz_init_fmt,
  mz_destroy_fmt,
  ShowMZHeader,
  AppendMZRef,
  NULL,
  mz_platform,
  NULL,
  NULL,
  mzAddressResolv,
  mzVA2PA,
  mzPA2VA,
  NULL,
  NULL,
  NULL,
  NULL
};
