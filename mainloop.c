/**
 * @namespace   biew
 * @file        mainloop.c
 * @brief       This file is analog of message loop routine.
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
#include <string.h>

#include "colorset.h"
#include "bmfile.h"
#include "tstrings.h"
#include "reg_form.h"
#include "biewutil.h"
#include "biewhelp.h"
#include "bconsole.h"
#include "codeguid.h"
#include "search.h"
#include "setup.h"
#include "biewlib/kbd_code.h"
#include "biewlib/biewlib.h"

extern __filesize_t LastOffset;
extern unsigned strmaxlen;
extern REGISTRY_BIN mzTable;

__filesize_t lastbyte;
static __filesize_t OldCurrFilePos; /** means previous File position */
unsigned long CurrStrLen = 0;
unsigned long PrevStrLen = 2;
unsigned long CurrPageSize = 0;
unsigned long PrevPageSize = 0;

int textshift = 0;

int __FASTCALL__ isHOnLine(__filesize_t cp,int width)
{
  if(FoundTextSt == FoundTextEnd) return 0;
  return (FoundTextSt >= cp && FoundTextSt < cp + width)
          || (FoundTextEnd > cp && FoundTextEnd < cp + width)
          || (FoundTextSt <= cp && FoundTextEnd >= cp + width);
}

void __FASTCALL__ HiLightSearch(TWindow *out,__filesize_t cfp,tRelCoord minx,tRelCoord maxx,tRelCoord y,HLInfo *buff,unsigned flags)
{
 tvioBuff it;
 unsigned __len,width;
 int x;
 char attr;
 t_vchar chars[__TVIO_MAXSCREENWIDTH];
 t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
 ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
 it.chars = chars;
 it.oem_pg = oem_pg;
 it.attrs = attrs;
 width = (flags & HLS_USE_DOUBLE_WIDTH) == HLS_USE_DOUBLE_WIDTH ? maxx*2 : maxx-minx;
 attr = browser_cset.highline;
 if((flags & HLS_USE_BUFFER_AS_VIDEO) == HLS_USE_BUFFER_AS_VIDEO)
 {
   memcpy(chars,buff->buff.chars,width);
   memcpy(oem_pg,buff->buff.oem_pg,width);
   memset(attrs,attr,width);
 }
 else
 {
   memcpy(chars,buff->text,width);
   memset(oem_pg,0,width);
   memset(attrs,attr,width);
   __nls_PrepareOEMForTVio(&it,width);
 }
 x = (int)(FoundTextSt - cfp);
 if((flags & HLS_USE_DOUBLE_WIDTH) == HLS_USE_DOUBLE_WIDTH) x *= 2;
 __len = (unsigned)(FoundTextEnd - FoundTextSt);
 if((flags & HLS_USE_DOUBLE_WIDTH) == HLS_USE_DOUBLE_WIDTH) __len *= 2;
 if(__len > width - x) __len = width - x;
 if(x < 0) { __len += x; x = 0; }
 if(__len && x + __len <= width)
 {
   unsigned char end,st;
   st = x;
   end = (__len + x);
   attr = browser_cset.hlight;
   memset(&attrs[st],attr,end-st);
 }
 twWriteBuffer(out,minx + 1,y + 1,&it,width);
}

static void __NEAR__ __FASTCALL__ drawTitle( void )
{
  unsigned percent;
  __filesize_t flen;
  flen = BMGetFLength();
  percent = flen ? (unsigned)(( lastbyte*100 )/flen) : 100;
  if(percent > 100) percent = 100;
  twUseWin(TitleWnd);
  twGotoXY(twGetClientWidth(TitleWnd)-4,1);
  twPrintF("%u%%",percent);
  twClrEOL();
}

char legalchars[] = "+-0123456789ABCDEFabcdef";

void MainLoop( void )
{
 int ch;
 __filesize_t savep = 0,cfp,nfp,flen;
 unsigned long lwidth;
 BMSeek(LastOffset,BM_SEEK_SET);
 drawPrompt();
 twUseWin(MainWnd);
 textshift = activeMode->paint(KE_SUPERKEY,textshift);
 BMSeek(LastOffset,BM_SEEK_SET);
 drawTitle();
 while(1)
 {
  unsigned che;
  ch = GetEvent(drawPrompt,MainActionFromMenu,NULL);
  nfp = cfp = OldCurrFilePos = BMGetCurrFilePos();
  flen = BMGetFLength();
  lwidth = activeMode->CurLineWidth();
  che = ch & 0x00FF;
  if(((che >= '0' && che <= '9') ||
      (che >= 'A' && che <= 'Z') ||
      (che >= 'a' && che <= 'z') ||
       ch == KE_BKSPACE) &&
       (activeMode->flags & __MF_USECODEGUIDE) == __MF_USECODEGUIDE)
     {
       nfp = GidGetGoAddress(ch);
       goto GO;
     }
  switch(ch)
  {
    case KE_CTL_F(1):
    case KE_CTL_F(2):
    case KE_CTL_F(3):
    case KE_CTL_F(4):
    case KE_CTL_F(5):
    case KE_CTL_F(6):
    case KE_CTL_F(7):
    case KE_CTL_F(8):
    case KE_CTL_F(9):
    case KE_CTL_F(10):
                    {
                       unsigned i;
                       i = (ch - KE_CTL_F(1)) >> 8;
                       if(activeMode->action[i])
                       {
                         if(activeMode->action[i]()) { ch = KE_SUPERKEY; drawPrompt(); }
                       }
                     }
                     break;
    case KE_ALT_F(1):
    case KE_ALT_F(2):
    case KE_ALT_F(3):
    case KE_ALT_F(4):
    case KE_ALT_F(5):
    case KE_ALT_F(6):
    case KE_ALT_F(7):
    case KE_ALT_F(8):
    case KE_ALT_F(9):
    case KE_ALT_F(10):
                      {
                        unsigned i;
                        i = (ch - KE_ALT_F(1)) >> 8;
                        if(detectedFormat->action[i]) nfp = detectedFormat->action[i]();
                      }
                      break;
    case KE_SUPERKEY: goto DRAW;
    case KE_F(1) : About();  continue;
    default : continue;
    case KE_SHIFT_F(1): activeMode->help();
                  break;
    case KE_F(10):
    case KE_ESCAPE : return;
    case KE_ENTER:
                  QuickSelectMode();
                  drawPrompt();
                  ch = KE_SUPERKEY;
                  break;
    case KE_F(2):
                  if(SelectMode()) ch = KE_SUPERKEY;
                  break;
    case KE_F(3):
                  if(NewSource())
                  {
                    ch = KE_SUPERKEY;
                    FoundTextSt = FoundTextEnd; ch = KE_SUPERKEY;
                    PaintTitle();
                  }
                  break;
    case KE_F(4):
                  if(activeMode->misc_action)
                  {
                     __filesize_t sfp;
                     sfp = BMGetCurrFilePos();
                     activeMode->misc_action();
                     ch = KE_SUPERKEY;
                     PaintTitle();
                     drawPrompt();
                     BMSeek(sfp,BM_SEEK_SET);
                  }
                  break;
    case KE_F(5):
           {
             static __filesize_t shift = 0;
	     static unsigned long flags = GJDLG_ABSOLUTE;
             if(GetJumpDlg(&shift,&flags))
             {
               switch(flags)
               {
                 default:
                 case GJDLG_ABSOLUTE: nfp = shift;
                                      break;
                 case GJDLG_RELATIVE: nfp += (long)shift;
                                      break;
                 case GJDLG_VIRTUAL:
                                      if(detectedFormat->va2pa)
                                      {
                                        __filesize_t temp_fp;
                                        temp_fp = detectedFormat->va2pa(shift);
                                        if(!temp_fp) ErrMessageBox(NOT_ENTRY,NULL);
                                        else nfp = temp_fp;
                                      }
                                      else nfp = shift;
                                      break;
               }
               if((activeMode->flags & __MF_USECODEGUIDE) == __MF_USECODEGUIDE)
                                                                 GidAddBackAddress();
               ch = KE_SUPERKEY;
             }
           }
           break;
    case KE_SHIFT_F(5): nfp = WhereAMI(nfp);
                        break;
    case KE_F(6): BMReRead();
             FoundTextSt = FoundTextEnd; ch = KE_SUPERKEY;
             PaintTitle();
             break;
    case KE_SHIFT_F(6): SelectSysInfo(); break;
    case KE_F(7): nfp = Search(False); ch = KE_JUSTFIND; break;
    case KE_SHIFT_F(7) : nfp = Search(True); ch = KE_JUSTFIND; break;
    case KE_F(8):  if(detectedFormat->showHdr) nfp = detectedFormat->showHdr();
                   else if(IsNewExe()) nfp = mzTable.showHdr();
                   break;
    case KE_SHIFT_F(8): SelectTool(); break;
    case KE_F(9): Setup(); break;
    case KE_SHIFT_F(10): if(FileUtils())
                   {
                      FoundTextSt = FoundTextEnd; ch = KE_SUPERKEY;
                      PaintTitle();
                   }
                   break;
    case KE_HOME: textshift = 0; break;
    case KE_END:  textshift = strmaxlen - tvioWidth/2; break;
    case KE_UPARROW:
                   nfp = cfp - activeMode->PrevLineWidth();
                   break;
    case KE_DOWNARROW:
                     nfp = cfp + activeMode->CurLineWidth();
                     break;
    case KE_RIGHTARROW:
                     if((activeMode->flags & __MF_TEXT) == __MF_TEXT)
                                      textshift+=activeMode->get_symbol_size();
                     else             nfp = cfp + activeMode->get_symbol_size();
                     break;
    case KE_LEFTARROW:
                     if((activeMode->flags & __MF_TEXT) == __MF_TEXT)
                                      textshift-=activeMode->get_symbol_size();
                     else             nfp = cfp - activeMode->get_symbol_size();
                     if(textshift < 0) textshift = 0;
                     break;
    case KE_CTL_RIGHTARROW:
                     if((activeMode->flags & __MF_TEXT) == __MF_TEXT)
                                      textshift+=8*activeMode->get_symbol_size();
                     else             nfp = cfp + 8*activeMode->get_symbol_size();
                     break;
    case KE_CTL_LEFTARROW:
                     if((activeMode->flags & __MF_TEXT) == __MF_TEXT)
                                      textshift-=8*activeMode->get_symbol_size();
                     else             nfp = cfp - 8*activeMode->get_symbol_size();
                     if(textshift < 0) textshift = 0;
                     break;
    case KE_PGUP:
                    nfp = cfp - activeMode->PrevPageSize();
                    break;
    case KE_PGDN:
                      nfp = cfp + activeMode->CurPageSize();
                      break;
    case KE_CTL_PGUP: nfp = 0;
                   break;
    case KE_CTL_PGDN:
                   nfp = flen;
                   break;
    case KE_CTL_(O): /** User screen */
                   {
                     unsigned evt;
                     twHideWin(MainWnd);
                     twHideWin(TitleWnd);
                     do
                     {
                        evt = GetEvent(drawEmptyPrompt,NULL,NULL);
                     }
                     while(!(evt == KE_ESCAPE || evt == KE_F(10) || evt == KE_CTL_(O)));
                     twShowWin(MainWnd);
                     twShowWin(TitleWnd);
                   }
                   continue;

  }
  GO:
  if(cfp != nfp)
  {
    unsigned long twidth = ( activeMode->flags & __MF_TEXT ) == __MF_TEXT ?
                           activeMode->get_symbol_size() :
                           ( activeMode->flags & __MF_DISASM ) == __MF_DISASM ?
                           1 : lwidth;
    __filesize_t p = flen - twidth;
    if((__fileoff_t)nfp < 0) nfp = 0;
    if(nfp > 0) if(nfp > p) nfp = p;
    BMSeek(nfp,BM_SEEK_SET);
  }
  DRAW:
  twUseWin(MainWnd);
  if((activeMode->flags & __MF_TEXT) != __MF_TEXT) savep = BMGetCurrFilePos();
  textshift = activeMode->paint(ch,textshift);
  if((activeMode->flags & __MF_TEXT) != __MF_TEXT) BMSeek(savep,BM_SEEK_SET);
  drawTitle();
 }
}
