/**
 * @namespace   biew_plugins_I
 * @file        plugins/hexmode.c
 * @brief       This file contains implementation of hexadecimal mode viewers.
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
#include <stdlib.h>
#include <stdio.h>

#include "plugins/hexmode.h"
#include "colorset.h"
#include "bconsole.h"
#include "biewutil.h"
#include "biewhelp.h"
#include "bmfile.h"
#include "reg_form.h"
#include "codeguid.h"
#include "editor.h"
#include "tstrings.h"
#include "biewlib/file_ini.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

static unsigned virtWidthCorr=0;

typedef char *(__NEAR__ __FASTCALL__ *hexFunc)(__filesize_t);
typedef unsigned char (__NEAR__ __FASTCALL__ *sizeFunc) ( void );

typedef struct tag_hexView
{
  const char *  name;
  hexFunc       func;
  sizeFunc      width;
  unsigned char size;
  unsigned char hardlen;
}hexView;

static char * __NEAR__ __FASTCALL__ GetB(__filesize_t val) { return GetBinary(BMReadByteEx(val,BM_SEEK_SET)); }
static char * __NEAR__ __FASTCALL__ Get2D(__filesize_t val) { return Get2Digit(BMReadByteEx(val,BM_SEEK_SET)); }
static char * __NEAR__ __FASTCALL__ Get4D(__filesize_t val) { return Get4Digit(BMReadWordEx(val,BM_SEEK_SET)); }
static char * __NEAR__ __FASTCALL__ Get8D(__filesize_t val) { return Get8Digit(BMReadDWordEx(val,BM_SEEK_SET)); }

static unsigned char __NEAR__ __FASTCALL__ sizeBit( void )  { return (tvioWidth-HA_LEN)/(8+1+1); }
static unsigned char __NEAR__ __FASTCALL__ sizeByte( void ) { return ((tvioWidth-HA_LEN)/(12+1+4)*4); } /* always round on four-column boundary */
static unsigned char __NEAR__ __FASTCALL__ sizeWord( void ) { return (tvioWidth-HA_LEN)/(4+1+2); }
static unsigned char __NEAR__ __FASTCALL__ sizeDWord( void ){ return (tvioWidth-HA_LEN)/(8+1+4); }

hexView hexViewer[] =
{
  { "B~it",         GetB,   sizeBit,   1, 8 },
  { "~Byte",        Get2D,  sizeByte,  1, 2 },
  { "~Word",        Get4D,  sizeWord,  2, 4 },
  { "~Double word", Get8D,  sizeDWord, 4, 8 }
};


static unsigned hmode = 1;

tBool hexAddressResolv = False;

static unsigned __FASTCALL__ drawHex( unsigned keycode,unsigned textshift )
{
 int i,I,Limit,dir;
 char outstr[__TVIO_MAXSCREENWIDTH+1];
 unsigned char HWidth;
 unsigned scrHWidth;
 __filesize_t sindex,cpos,flen,lindex,SIndex;
 static __filesize_t hmocpos = 0L;
 int __inc,dlen;
 cpos = BMGetCurrFilePos();
 if(hmocpos != cpos || keycode == KE_SUPERKEY || keycode == KE_JUSTFIND)
 {
   tAbsCoord height = twGetClientHeight(MainWnd);
   tAbsCoord width = twGetClientWidth(MainWnd);
   twFreezeWin(MainWnd);
   HWidth = hexViewer[hmode].width()-virtWidthCorr;
   if(!(hmocpos == cpos + HWidth || hmocpos == cpos - HWidth)) keycode = KE_SUPERKEY;
   hmocpos = cpos;
   __inc = hexViewer[hmode].size;
   dlen = hexViewer[hmode].hardlen;
   flen = BMGetFLength();
   scrHWidth = HWidth*__inc;
   if(flen < HWidth) HWidth = flen;
   if(keycode == KE_UPARROW)
   {
       I = height-1;
       dir = -1;
       Limit = -1;
       if(cpos >= HWidth)
       {
         twScrollWinDn(MainWnd,1,1);
         I = 0;
       }
       else keycode = KE_SUPERKEY;
   }
   else
   {
       I = 0;
       dir = 1;
       Limit = height;
   }
   if(keycode == KE_DOWNARROW && flen >= HWidth)
   {
     I = height-1;
     twScrollWinUp(MainWnd,I,1);
   }
   SIndex = cpos + HWidth*I;
   lindex = flen - SIndex;
   /* This loop is called only when line or screen is repainting */
   for(i = I,sindex = SIndex;i != Limit;i += 1*dir,sindex += scrHWidth*dir)
   {
     memset(outstr,TWC_DEF_FILLER,width);
     if(sindex < flen)
     {
       int freq,j,rwidth,xmin,len;
       lindex = (flen - sindex)/__inc;
       rwidth = lindex > HWidth ? HWidth : (int)lindex;
       len = HA_LEN;
       memcpy(outstr,GidEncodeAddress(sindex,hexAddressResolv),len);
       for(j = 0,freq = 0,lindex = sindex;j < rwidth;j++,lindex += __inc,freq++)
       {
          memcpy(&outstr[len],hexViewer[hmode].func(lindex),dlen);
          len += dlen + 1;
          if(hmode == 1) if(freq == 3) { freq = -1; len++; }
       }
       BMReadBufferEx((void *)&outstr[width - scrHWidth],rwidth*__inc,sindex,BM_SEEK_SET);
       xmin = tvioWidth-scrHWidth;
       twDirectWrite(1,i + 1,outstr,xmin);
       if(isHOnLine(sindex,scrHWidth))
       {
          HLInfo hli;
          hli.text = &outstr[xmin];
          HiLightSearch(MainWnd,sindex,xmin,width,i,&hli,HLS_NORMAL);
       }
       else  twDirectWrite(xmin + 1,i + 1,&outstr[xmin],width - xmin);
     }
     else twDirectWrite(1,i + 1,outstr,width);
   }
   lastbyte = lindex + __inc;
   twRefreshWin(MainWnd);
 }
 return textshift;
}

static void __FASTCALL__ HelpHex( void )
{
   hlpDisplay(1002);
}

static unsigned long __FASTCALL__ hexPrevPageSize( void ) { return (hexViewer[hmode].width()-virtWidthCorr)*hexViewer[hmode].size*twGetClientHeight(MainWnd); }
static unsigned long __FASTCALL__ hexCurrPageSize( void ) { return (hexViewer[hmode].width()-virtWidthCorr)*hexViewer[hmode].size*twGetClientHeight(MainWnd); }
static unsigned long __FASTCALL__ hexPrevLineWidth( void ) { return (hexViewer[hmode].width()-virtWidthCorr)*hexViewer[hmode].size; }
static unsigned long __FASTCALL__ hexCurrLineWidth( void ) { return (hexViewer[hmode].width()-virtWidthCorr)*hexViewer[hmode].size; }
static const char *  __FASTCALL__ hexMiscKeyName( void ) { return hmode == 1 ? "Modify" : "      "; }

static void __FASTCALL__ __NEAR__ __checkWidthCorr(void)
{
  if(virtWidthCorr>hexViewer[hmode].width()-1) virtWidthCorr=hexViewer[hmode].width()-1;
}

static tBool __FASTCALL__ hexSelectMode( void )
{
  const char *names[sizeof(hexViewer)/sizeof(hexView)];
  size_t i,nModes;
  int retval;
  nModes = sizeof(hexViewer)/sizeof(hexView);
  for(i = 0;i < nModes;i++) names[i] = hexViewer[i].name;
  retval = SelBoxA(names,nModes," Select hexadecimal mode: ",hmode);
  if(retval != -1)
  {
    hmode = retval;
    __checkWidthCorr();
    return True;
  }
  return False;
}

static const char *aresolv[] =
{
  "~Global (global file offset)",
  "~Local (local offset within blocks and virtual addresses)"
};

tBool __FASTCALL__ hexAddressResolution( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(aresolv)/sizeof(char *);
  i = SelBoxA(aresolv,nModes," Select address resolving: ",(unsigned)hexAddressResolv);
  if(i != -1)
  {
    hexAddressResolv = i ? True : False;
    return True;
  }
  return False;
}

static tBool __FASTCALL__ hexDetect( void ) { return True; }

static int __NEAR__ __FASTCALL__ FullHexEdit(TWindow * txtwnd)
{
 size_t i,j;
 unsigned mlen;
 unsigned int _lastbyte;
 unsigned flags;
 tAbsCoord height = twGetClientHeight(txtwnd);
 tAbsCoord width = twGetClientWidth(txtwnd);
 char work[__TVIO_MAXSCREENWIDTH],owork[__TVIO_MAXSCREENWIDTH];
 tBool redraw;
 {
   TWindow * using = twUsedWin();
   twUseWin(txtwnd);
   twSetColorAttr(browser_cset.main);
   twFreezeWin(txtwnd);
   for(i = 0;i < height;i++)
   {
     twDirectWrite(width - EditorMem.width + 1,i + 1,&EditorMem.buff[i*EditorMem.width],EditorMem.alen[i]);
     if((unsigned)EditorMem.alen[i] + 1 < EditorMem.width)
     {
        twGotoXY(width - EditorMem.width + EditorMem.alen[i] + 2,i + 1); twClrEOL();
     }
   }
   twRefreshWin(txtwnd);
   twUseWin(using);
 }
 twSetColorAttr(browser_cset.edit.main);
 for(i = 0;i < height;i++)
 {
   unsigned eidx;
   eidx = i*EditorMem.width;
   ExpandHex(work,&EditorMem.buff[eidx],EditorMem.alen[i],1);
   mlen = ExpandHex(owork,&EditorMem.save[eidx],EditorMem.alen[i],1);
   for(j = 0;j < mlen;j++)
   {
     twSetColorAttr(work[j] == owork[j] ? browser_cset.edit.main : browser_cset.edit.change);
     twDirectWrite(j + 1,i + 1,&work[j],1);
   }
   if(mlen + 1 < EditorMem.width)
   {
     twGotoXY(mlen + 1,i + 1);
     twClrEOL();
   }
 }
 redraw = True;
 PaintETitle(edit_y*EditorMem.width + edit_x,0);
 twShowWin(twUsedWin());
 twSetCursorType(TW_CUR_NORM);
 while(1)
 {
   unsigned eidx;
   mlen = EditorMem.alen[edit_y];
   eidx = edit_y*EditorMem.width;
   ExpandHex(work,&EditorMem.buff[eidx],mlen,1);
   mlen = ExpandHex(owork,&EditorMem.save[eidx],mlen,1);
   edit_x*=3;
   flags = __ESS_WANTRETURN | __ESS_HARDEDIT | __ESS_ASHEX;
   if(!redraw) flags |= __ESS_NOREDRAW;
   _lastbyte = eeditstring(work,&legalchars[2],&mlen,(unsigned)(edit_y + 1),(unsigned *)&edit_x,
                          flags,owork,NULL);
   edit_x/=3;
   CompressHex(&EditorMem.buff[eidx],work,mlen/3,True);
   switch(_lastbyte)
   {
     case KE_F(1)   : ExtHelp(); continue;
     case KE_F(2)   : editSaveContest();
     case KE_F(10)  :
     case KE_ESCAPE :
     case KE_TAB : goto bye;
     default     : redraw = editDefAction(_lastbyte); break;
   }
   CheckBounds();
   if(redraw)
   {
      TWindow * using = twUsedWin();
      twUseWin(txtwnd);
      twDirectWrite(width - EditorMem.width + 1,edit_y + 1,&EditorMem.buff[edit_y*EditorMem.width],mlen/3);
      twUseWin(using);
   }
   PaintETitle(edit_y*EditorMem.width + edit_x,0);
 }
 bye:
 twSetCursorType(TW_CUR_OFF);
 return _lastbyte;
}

static void __FASTCALL__ EditHex( void )
{
 TWindow * ewnd[2];
 tBool has_show[2];
 int active = 0,oactive = 0;
 unsigned bound;
 tAbsCoord width = twGetClientWidth(MainWnd);
 if(hmode != 1) return;
 if(!BMGetFLength()) { ErrMessageBox(NOTHING_EDIT,NULL); return; }
 bound = width-(hexViewer[hmode].width()-virtWidthCorr);
 ewnd[0] = WindowOpen(HA_LEN+1,2,bound,tvioHeight-1,TWS_CURSORABLE);
 twSetColorAttr(browser_cset.edit.main); twClearWin();
 ewnd[1] = WindowOpen(bound+1,2,width,tvioHeight-1,TWS_CURSORABLE);
 twSetColorAttr(browser_cset.edit.main); twClearWin();
 drawEditPrompt();
 has_show[0] = has_show[1] = False;
 if(editInitBuffs(hexViewer[hmode].width()-virtWidthCorr,NULL,0))
 {
   edit_x = edit_y = 0;
   while(1)
   {
     int _lastbyte;
     if(active != oactive)
     {
       twHideWin(ewnd[oactive]);
       if(has_show[active]) twShowWin(ewnd[active]);
       oactive = active;
     }
     twUseWin(ewnd[active]);
     if(!active) _lastbyte = FullHexEdit(MainWnd);
     else        _lastbyte = FullEdit(MainWnd,NULL);
     has_show[active] = True;
     if(_lastbyte == KE_TAB) active = active ? 0 : 1;
     else break;
   }
   editDestroyBuffs();
 }
 CloseWnd(ewnd[0]);
 CloseWnd(ewnd[1]);
 PaintTitle();
}

void __FASTCALL__ ReadIniAResolv( hIniProfile *ini )
{
  char tmps[10];
  biewReadProfileString(ini,"Biew","Browser","SubSubMode6","0",tmps,sizeof(tmps));
  hexAddressResolv = (unsigned)strtoul(tmps,NULL,10);
  if(hexAddressResolv > 1) hexAddressResolv = 0;
}

void __FASTCALL__ WriteIniAResolv( hIniProfile *ini )
{
  char tmps[10];
  sprintf(tmps,"%i",hexAddressResolv);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode6",tmps);
  sprintf(tmps,"%u",virtWidthCorr);
  biewWriteProfileString(ini,"Biew","Browser","VirtWidthCorr",tmps);
}

static void __FASTCALL__ hexReadIni( hIniProfile *ini )
{
  char tmps[10];
  if(isValidIniArgs())
  {
    biewReadProfileString(ini,"Biew","Browser","LastSubMode","2",tmps,sizeof(tmps));
    hmode = (unsigned)strtoul(tmps,NULL,10);
    if(hmode > 3) hmode = 1;
    ReadIniAResolv(ini);
    biewReadProfileString(ini,"Biew","Browser","VirtWidthCorr","0",tmps,sizeof(tmps));
    virtWidthCorr = (unsigned)strtoul(tmps,NULL,10);
    __checkWidthCorr();
  }
}

static void __FASTCALL__ hexSaveIni( hIniProfile * ini)
{
  char tmps[10];
  sprintf(tmps,"%i",hmode);
  biewWriteProfileString(ini,"Biew","Browser","LastSubMode",tmps);
  WriteIniAResolv(ini);
  sprintf(tmps,"%u",virtWidthCorr);
  biewWriteProfileString(ini,"Biew","Browser","VirtWidthCorr",tmps);
}

static unsigned __FASTCALL__ hexCharSize( void ) { return 1; }

static tBool __FASTCALL__ hexIncVirtWidth( void )
{
  if(virtWidthCorr) { virtWidthCorr--; return True; }
  return False;
}

static tBool __FASTCALL__ hexDecVirtWidth( void )
{
  if(virtWidthCorr < hexViewer[hmode].width()-1) { virtWidthCorr++; return True; }
  return False;
}

REGISTRY_MODE hexMode =
{
  "~Hexadecimal mode",
  { NULL, "HexMod", NULL, NULL, NULL, "AResol", "<<<   ", "   >>>", NULL, NULL },
  { NULL, hexSelectMode, NULL, NULL, NULL, hexAddressResolution, hexDecVirtWidth, hexIncVirtWidth, NULL, NULL },
  hexDetect,
  __MF_NONE,
  drawHex,
  NULL,
  hexCharSize,
  hexMiscKeyName,
  EditHex,
  hexPrevPageSize,
  hexCurrPageSize,
  hexPrevLineWidth,
  hexCurrLineWidth,
  HelpHex,
  hexReadIni,
  hexSaveIni,
  NULL,
  NULL,
  NULL
};








