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
#include <errno.h>

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
#include "biewlib/pmalloc.h"

static unsigned virtWidthCorr=0;

#define MOD_PLAIN    0
#define MOD_BINARY   1
#define MOD_REVERSE  2

#define MOD_MAXMODE  2

static const char * mod_names[] =
{
   "~Plain text",
   "~Video dump",
   "~Reversed video dump"
};
static unsigned bin_mode = MOD_PLAIN; /**< points to currently selected mode text mode */

static tBool __FASTCALL__ binSelectMode( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(mod_names)/sizeof(char *);
  i = SelBoxA(mod_names,nModes," Select binary mode: ",bin_mode);
  if(i != -1)
  {
    bin_mode = i;
    return True;
  }
  return False;
}

static unsigned __FASTCALL__ drawBinary( unsigned keycode,unsigned tshift )
{
 static unsigned long bmocpos = 0L;
 unsigned long _index;
 unsigned long limit,flen,cfp;
 int len;
 unsigned BWidth,_b_width,count;
 size_t j;
 HLInfo hli;
 tvioBuff it;
 char buffer[__TVIO_MAXSCREENWIDTH*2];
 char chars[__TVIO_MAXSCREENWIDTH];
 char oem_pg[__TVIO_MAXSCREENWIDTH];
 char attrs[__TVIO_MAXSCREENWIDTH];
 tAbsCoord width,height;

 it.chars=chars;
 it.oem_pg=oem_pg;
 it.attrs=attrs;
 cfp  = BMGetCurrFilePos();
 width = twGetClientWidth(MainWnd);
 BWidth = twGetClientWidth(MainWnd)-virtWidthCorr;
 height = twGetClientHeight(MainWnd);
 if(bin_mode==MOD_PLAIN) _b_width=1;
 else _b_width=2;
 if(cfp != bmocpos || keycode == KE_SUPERKEY || keycode == KE_JUSTFIND)
 {
   bmocpos = cfp;
   flen = BMGetFLength();
   limit = flen - BWidth;
   if(flen < (unsigned long)BWidth) BWidth = (int)(limit = flen);
   twFreezeWin(MainWnd);
   for(j = 0;j < height;j++)
   {
     count=BWidth*_b_width;
     _index = cfp + j*count;
     len = _index < limit ? count : _index < flen ? (int)(flen - _index) : 0;
     if(len) { lastbyte = _index + len; BMReadBufferEx((void *)buffer,len,_index,BM_SEEK_SET); }
     if(bin_mode!=MOD_PLAIN)
     {
        unsigned i,ii;
        for(i=ii=0;i<BWidth;i++)
	{
	    chars[i]=buffer[ii++];
	    attrs[i]=buffer[ii++];
	}
	memset(oem_pg,0,tvioWidth);
	if(bin_mode==MOD_REVERSE)
	{
	    char *t;
	    t=it.chars;
	    it.chars=it.attrs;
	    it.attrs=t;
	}
	count=len/2;
	memset(&it.chars[count],TWC_DEF_FILLER,tvioWidth-count);
	memset(&it.attrs[count],browser_cset.main,tvioWidth-count);
     }
     else
       memset(&buffer[len],TWC_DEF_FILLER,tvioWidth-len);
     if(isHOnLine(_index,width))
     {
        hli.text = buffer;
        HiLightSearch(MainWnd,_index,0,BWidth,j,&hli,HLS_NORMAL);
     }
     else
     {
        if(bin_mode==MOD_PLAIN)
	    twDirectWrite(1,j + 1,buffer,width);
	else
	    twWriteBuffer(MainWnd,1,j + 1,&it,width);
     }
   }
   twRefreshWin(MainWnd);
 }
 return tshift;
}

static void __FASTCALL__ HelpBin( void )
{
   hlpDisplay(1000);
}

static unsigned long __FASTCALL__ binPrevPageSize( void ) { return (twGetClientWidth(MainWnd)-virtWidthCorr)*twGetClientHeight(MainWnd)*(bin_mode==MOD_PLAIN?1:2); }
static unsigned long __FASTCALL__ binCurrPageSize( void ) { return (twGetClientWidth(MainWnd)-virtWidthCorr)*twGetClientHeight(MainWnd)*(bin_mode==MOD_PLAIN?1:2); }
static unsigned long __FASTCALL__ binPrevLineWidth( void ) { return (twGetClientWidth(MainWnd)-virtWidthCorr)*(bin_mode==MOD_PLAIN?1:2); }
static unsigned long __FASTCALL__ binCurrLineWidth( void ) { return (twGetClientWidth(MainWnd)-virtWidthCorr)*(bin_mode==MOD_PLAIN?1:2); }
static const char *  __FASTCALL__ binMiscKeyName( void ) { return "Modify"; }

static tBool __FASTCALL__ binDetect( void ) { return True; }

static void save_video(unsigned char *buff,unsigned size)
{
  BGLOBAL bHandle;
  char *fname;
  unsigned i;
  fname = BMName();
  bHandle = biewOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
  if(bHandle == &bNull)
  {
      err:
      errnoMessageBox(WRITE_FAIL,NULL,errno);
      return;
  }
  bioSeek(bHandle,BMGetCurrFilePos(),BIO_SEEK_SET);
  if(bin_mode==MOD_REVERSE) bioSeek(bHandle,1,BIO_SEEK_CUR);
  for(i=0;i<size;i++)
  {
    if(!bioWriteByte(bHandle,buff[i])) goto err;
    bioSeek(bHandle,1,BIO_SEEK_CUR);
  }
  bioClose(bHandle);
  BMReRead();
}

static void __FASTCALL__ EditBin( void )
{
 TWindow *ewin;
 tBool inited;
 if(!BMGetFLength()) { ErrMessageBox(NOTHING_EDIT,NULL); return; }
 ewin = WindowOpen(1,2,tvioWidth-virtWidthCorr,tvioHeight-1,TWS_CURSORABLE);
 twSetColorAttr(browser_cset.edit.main); twClearWin();
 drawEditPrompt();
 twUseWin(ewin);
 edit_x = edit_y = 0;
 if(bin_mode==MOD_PLAIN)
    inited=editInitBuffs(tvioWidth-virtWidthCorr,NULL,0);
 else
 {
    unsigned long flen,cfp;
    unsigned i,size,msize = tvioWidth*tvioHeight;
    unsigned char *buff = PMalloc(msize*2);
    if(buff)
    {
	flen = BMGetFLength();
	cfp = BMGetCurrFilePos();
	size = (unsigned)((unsigned long)msize > (flen-cfp) ? (flen-cfp) : msize);
	BMReadBufferEx(buff,size*2,cfp,BM_SEEK_SET);
	BMSeek(cfp,BM_SEEK_SET);
	for(i=0;i<size;i++) buff[i]=bin_mode==MOD_BINARY?buff[i*2]:buff[i*2+1];
	inited=editInitBuffs(tvioWidth-virtWidthCorr,buff,size);
	PFREE(buff);
    }
    else
    {
	MemOutBox("Editor initialization");
	inited=False;
    }
 }
 if(inited)
 {
   FullEdit(NULL,bin_mode==MOD_PLAIN?NULL:save_video);
   editDestroyBuffs();
 }
 CloseWnd(ewin);
 PaintTitle();
}

static void __FASTCALL__ binReadIni( hIniProfile *ini )
{
  char tmps[10];
  if(isValidIniArgs())
  {
    biewReadProfileString(ini,"Biew","Browser","LastSubMode","0",tmps,sizeof(tmps));
    bin_mode = (unsigned)strtoul(tmps,NULL,10);
    if(bin_mode > MOD_MAXMODE) bin_mode = MOD_MAXMODE;
    biewReadProfileString(ini,"Biew","Browser","VirtWidthCorr","0",tmps,sizeof(tmps));
    virtWidthCorr = (unsigned)strtoul(tmps,NULL,10);
    if(virtWidthCorr>tvioWidth-1) virtWidthCorr=tvioWidth-1;
  }
}

static void __FASTCALL__ binSaveIni( hIniProfile *ini )
{
  char tmps[10];
  /** Nullify LastSubMode */
  sprintf(tmps,"%i",bin_mode);
  biewWriteProfileString(ini,"Biew","Browser","LastSubMode",tmps);
  sprintf(tmps,"%u",virtWidthCorr);
  biewWriteProfileString(ini,"Biew","Browser","VirtWidthCorr",tmps);
}

static unsigned __FASTCALL__ binCharSize( void ) { return bin_mode==MOD_PLAIN?1:2; }

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
  { NULL, "BinMod", NULL, NULL, NULL, NULL, "<<<   ", "   >>>", NULL, NULL },
  { NULL, binSelectMode, NULL, NULL, NULL, NULL, binDecVirtWidth, binIncVirtWidth, NULL, NULL },
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





