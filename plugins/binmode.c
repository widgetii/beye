/**
 * @namespace   biew_plugins_I
 * @file        plugins/binmode.c
 * @brief       This file contains implementation of binary mode viewer.
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

#include "colorset.h"
#include "bconsole.h"
#include "biewutil.h"
#include "biewhelp.h"
#include "bmfile.h"
#include "reg_form.h"
#include "editor.h"
#include "tstrings.h"
#include "biewlib/file_ini.h"
#include "biewlib/kbd_code.h"
#include "biewlib/biewlib.h"

static unsigned virtWidthCorr=0;

static unsigned __FASTCALL__ drawBinary( unsigned keycode,unsigned tshift )
{
 char buffer[__TVIO_MAXSCREENWIDTH];
 static unsigned long bmocpos = 0L;
 tAbsCoord width,height;
 unsigned BWidth;
 HLInfo hli;
 size_t j;
 unsigned long _index;
 unsigned long limit,flen,cfp;
 int len;
 cfp  = BMGetCurrFilePos();
 width = twGetClientWidth(MainWnd);
 BWidth = twGetClientWidth(MainWnd)-virtWidthCorr;
 height = twGetClientHeight(MainWnd);
 if(cfp != bmocpos || keycode == KE_SUPERKEY || keycode == KE_JUSTFIND)
 {
   bmocpos = cfp;
   flen = BMGetFLength();
   limit = flen - BWidth;
   if(flen < (unsigned long)BWidth) BWidth = (int)(limit = flen);
   twFreezeWin(MainWnd);
   for(j = 0;j < height;j++)
   {
     memset(buffer,TWC_DEF_FILLER,tvioWidth);
     _index = cfp + j*BWidth;
     len = _index < limit ? BWidth : _index < flen ? (int)(flen - _index) : 0;
     if(len) { lastbyte = _index + len; BMReadBufferEx((void *)buffer,len,_index,BM_SEEK_SET); }
     if(isHOnLine(_index,width))
     {
        hli.text = buffer;
        HiLightSearch(MainWnd,_index,0,BWidth,j,&hli,HLS_NORMAL);
     }
     else twDirectWrite(1,j + 1,buffer,width);
   }
   twRefreshWin(MainWnd);
 }
 return tshift;
}

static void __FASTCALL__ HelpBin( void )
{
   hlpDisplay(1000);
}

static unsigned long __FASTCALL__ binPrevPageSize( void ) { return twGetClientWidth(MainWnd)*twGetClientHeight(MainWnd); }
static unsigned long __FASTCALL__ binCurrPageSize( void ) { return twGetClientWidth(MainWnd)*twGetClientHeight(MainWnd); }
static unsigned long __FASTCALL__ binPrevLineWidth( void ) { return twGetClientWidth(MainWnd); }
static unsigned long __FASTCALL__ binCurrLineWidth( void ) { return twGetClientWidth(MainWnd); }
static const char *  __FASTCALL__ binMiscKeyName( void ) { return "Modify"; }

static tBool __FASTCALL__ binDetect( void ) { return True; }

static void __FASTCALL__ EditBin( void )
{
 TWindow *ewin;
 if(!BMGetFLength()) { ErrMessageBox(NOTHING_EDIT,NULL); return; }
 ewin = WindowOpen(1,2,tvioWidth-virtWidthCorr,tvioHeight-1,TWS_CURSORABLE);
 twSetColorAttr(browser_cset.edit.main); twClearWin();
 drawEditPrompt();
 twUseWin(ewin);
 edit_x = edit_y = 0;
 if(editInitBuffs(tvioWidth-virtWidthCorr))
 {
   FullEdit(NULL);
   editDestroyBuffs();
 }
 CloseWnd(ewin);
 PaintTitle();
}

static void __FASTCALL__ binReadIni( hIniProfile *ini )
{
  char tmps[10];
  biewReadProfileString(ini,"Biew","Browser","VirtWidthCorr","0",tmps,sizeof(tmps));
  virtWidthCorr = (unsigned)strtoul(tmps,NULL,10);
  if(virtWidthCorr>tvioWidth-1) virtWidthCorr=tvioWidth-1;
}

static void __FASTCALL__ binSaveIni( hIniProfile *ini )
{
  char tmps[10];
  /** Nullify LastSubMode */
  biewWriteProfileString(ini,"Biew","Browser","LastSubMode","0");
  sprintf(tmps,"%u",virtWidthCorr);
  biewWriteProfileString(ini,"Biew","Browser","VirtWidthCorr",tmps);
}

static unsigned __FASTCALL__ binCharSize( void ) { return 1; }

static tBool __FASTCALL__ binIncVirtWidth( void )
{
  if(virtWidthCorr) { virtWidthCorr--; return True; }
  return False;
}

static tBool __FASTCALL__ binDecVirtWidth( void )
{
  if(virtWidthCorr < tvioWidth-1) { virtWidthCorr++; return True; }
  return False;
}

REGISTRY_MODE binMode =
{
  "~Binary mode",
  { NULL, NULL, NULL, NULL, NULL, NULL, "<<<   ", "   >>>", NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, binDecVirtWidth, binIncVirtWidth, NULL, NULL },
  binDetect,
  __MF_NONE,
  drawBinary,
  NULL,
  binCharSize,
  binMiscKeyName,
  EditBin,
  binPrevPageSize,
  binCurrPageSize,
  binPrevLineWidth,
  binCurrLineWidth,
  HelpBin,
  binReadIni,
  binSaveIni,
  NULL,
  NULL,
  NULL
};





