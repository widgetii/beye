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
 width = BWidth = twGetClientWidth(MainWnd);
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
     memset(buffer,TWC_DEF_FILLER,width);
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
 ewin = WindowOpen(1,2,tvioWidth,tvioHeight-1,TWS_CURSORABLE);
 twSetColorAttr(browser_cset.edit.main); twClearWin();
 drawEditPrompt();
 twUseWin(ewin);
 edit_x = edit_y = 0;
 if(editInitBuffs(tvioWidth))
 {
   FullEdit(NULL);
   editDestroyBuffs();
 }
 CloseWnd(ewin);
 PaintTitle();
}

static void __FASTCALL__ binReadIni( hIniProfile *ini )
{
  UNUSED(ini);
}

static void __FASTCALL__ binSaveIni( hIniProfile *ini )
{
  /** Nullify LastSubMode */
  iniWriteProfileString(ini,"Biew","Browser","LastSubMode","0");
}

static unsigned __FASTCALL__ binCharSize( void ) { return 1; }

REGISTRY_MODE binMode =
{
  "~Binary mode",
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
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





