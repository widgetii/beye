/**
 * @namespace   biew
 * @file        editors.c
 * @brief       This file contains low level editor implementation of BIEW project.
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
#include <errno.h>

#include "colorset.h"
#include "bmfile.h"
#include "tstrings.h"
#include "plugins/disasm.h"
#include "bconsole.h"
#include "biewutil.h"
#include "biewhelp.h"
#include "editor.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"
#include "biewlib/pmalloc.h"

__fileoff_t edit_cp = 0;
struct tag_emem EditorMem;

int edit_x,edit_y;
unsigned char edit_XX = 0;

void ExtHelp( void )
{
 TWindow * using = twUsedWin();
 hlpDisplay(2);
 twUseWin(using);
}

void __FASTCALL__ PaintETitle( int shift,tBool use_shift )
{
  TWindow * using = twUsedWin();
  unsigned eidx;
  char byte,obyte;
  twUseWin(TitleWnd);
  twFreezeWin(TitleWnd);
  twGotoXY(1,1);
  twClrEOL();
  twPrintF("%08lX: ",edit_cp + shift);
  eidx = use_shift ? (unsigned)shift : edit_y*EditorMem.width+edit_x;
  byte  = EditorMem.buff[eidx];
  obyte = EditorMem.save[eidx];
  if(byte != obyte) twSetColorAttr(title_cset.change);
  twPrintF("%c %02XH %sH %sB "
           ,byte ? byte : ' '
           ,byte & 0x00FF
           ,Get2SignDig(byte)
           ,GetBinary(byte));
  twSetColorAttr(title_cset.main);
  if(byte != obyte)
  {
    twPrintF("ORIGINAL: %c %02XH %sH %sB "
             ,obyte ? obyte : ' '
             ,obyte & 0x00FF
             ,Get2SignDig(obyte)
             ,GetBinary(obyte));
  }
  else
    twPrintF("                                ");
  twPrintF("MASK: %sH"
           ,Get2Digit(edit_XX));
  twRefreshWin(TitleWnd);
  twUseWin(using);
}

tBool __FASTCALL__ editInitBuffs(unsigned width,unsigned char *buff,unsigned size)
{
 __filesize_t flen,cfp,ssize;
 unsigned i,msize;
 msize = tvioWidth*tvioHeight;
 EditorMem.buff = PMalloc(msize);
 EditorMem.save = PMalloc(msize);
 EditorMem.alen = PMalloc(tvioHeight);
 if((!EditorMem.buff) || (!EditorMem.save) || (!EditorMem.alen))
 {
   if(EditorMem.buff) PFREE(EditorMem.buff);
   if(EditorMem.save) PFREE(EditorMem.save);
   if(EditorMem.alen) PFREE(EditorMem.alen);
   MemOutBox("Editor initialization");
   return False;
 }
 memset(EditorMem.buff,TWC_DEF_FILLER,msize);
 memset(EditorMem.save,TWC_DEF_FILLER,msize);
 flen = BMGetFLength();
 edit_cp = cfp = BMGetCurrFilePos();
 EditorMem.width = width;
 if(buff)
 {
    EditorMem.size = size;
    memcpy(EditorMem.buff,buff,size);
 }
 else
 {
    EditorMem.size = (unsigned)((__filesize_t)msize > (flen-cfp) ? (flen-cfp) : msize);
    BMReadBufferEx(EditorMem.buff,EditorMem.size,cfp,BM_SEEK_SET);
    BMSeek(cfp,BM_SEEK_SET);
 }
 memcpy(EditorMem.save,EditorMem.buff,EditorMem.size);
 /** initialize EditorMem.alen */
 ssize = flen-cfp;
 for(i = 0;i < tvioHeight;i++)
 {
    EditorMem.alen[i] = ssize >= width ? width : ssize;
    ssize -= min(ssize,width);
 }
 return True;
}

void __FASTCALL__ editDestroyBuffs( void )
{
  PFREE(EditorMem.buff);
  PFREE(EditorMem.save);
  PFREE(EditorMem.alen);
}

void __FASTCALL__ CheckBounds( void )
{
  tAbsCoord height = twGetClientHeight(MainWnd);
  if(edit_y < 0) edit_y = 0;
  if((unsigned)edit_y > height - 1) edit_y = height - 1;
  if(!EditorMem.alen[edit_y]) edit_y--;
  if(edit_x >= EditorMem.alen[edit_y]) edit_x = EditorMem.alen[edit_y] - 1;
}

void __FASTCALL__ CheckYBounds( void )
{
  tAbsCoord height = twGetClientHeight(MainWnd);
  if(edit_y < 0) edit_y = 0;
  if((unsigned)edit_y > height - 1) edit_y = height - 1;
  while(!EditorMem.alen[edit_y]) edit_y--;
}

void __FASTCALL__ CheckXYBounds( void )
{
   CheckYBounds();
   if(edit_x < 0) edit_x = EditorMem.alen[--edit_y]*2;
   if(edit_x >= EditorMem.alen[edit_y]*2) { edit_x = 0; edit_y++; }
   CheckYBounds();
}

void __FASTCALL__ editSaveContest( void )
{
  BGLOBAL bHandle;
  char *fname;
  fname = BMName();
  bHandle = biewOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
  if(bHandle == &bNull)
  {
      err:
      errnoMessageBox(WRITE_FAIL,NULL,errno);
      return;
  }
  bioSeek(bHandle,edit_cp,BIO_SEEK_SET);
  if(!bioWriteBuffer(bHandle,(void *)EditorMem.buff,EditorMem.size)) goto err;
  bioClose(bHandle);
  BMReRead();
}

tBool __FASTCALL__ edit_defaction(int _lastbyte)
{
 tBool redraw;
  redraw = False;
   switch(_lastbyte)
   {
     case KE_UPARROW  : edit_y--; break;
     case KE_DOWNARROW: edit_y++; break;
     case KE_ENTER:
     case KE_LEFTARROW:
     case KE_RIGHTARROW: break;
     case KE_F(3)     :
                      Get2DigitDlg(INIT_MASK,INPUT_MASK,&edit_XX);
                      break;
     default: redraw = True; break;
   }
 return redraw;
}

tBool __FASTCALL__ editDefAction(int _lastbyte)
{
 tBool redraw = True;
 int eidx;
 eidx = edit_y*EditorMem.width+edit_x;
   switch(_lastbyte)
   {
     case KE_F(4)     : EditorMem.buff[eidx] = ~EditorMem.buff[eidx]; break;
     case KE_F(5)     : EditorMem.buff[eidx] |= edit_XX; break;
     case KE_F(6)     : EditorMem.buff[eidx] &= edit_XX; break;
     case KE_F(7)     : EditorMem.buff[eidx] ^= edit_XX; break;
     case KE_F(8)     : EditorMem.buff[eidx]  = edit_XX; break;
     case KE_F(9)     : EditorMem.buff[eidx] = EditorMem.save[eidx]; break;
     default        : redraw = edit_defaction(_lastbyte); edit_x--; break;
   }
   edit_x++;
   return redraw;
}

int __FASTCALL__ FullEdit(TWindow * txtwnd,void (*save_func)(unsigned char *,unsigned))
{
 size_t i,j;
 unsigned mlen;
 unsigned int _lastbyte;
 unsigned flags;
 tAbsCoord height = twGetClientHeight(MainWnd);
 tBool redraw;
 char attr = __ESS_HARDEDIT | __ESS_WANTRETURN;
 twSetColorAttr(browser_cset.edit.main);
 __MsSetState(False);
 for(i = 0;i < height;i++)
 {
   for(j = 0;j < EditorMem.alen[i];j++)
   {
     unsigned eidx;
     eidx = i*EditorMem.width+j;
     twSetColorAttr(EditorMem.buff[eidx] == EditorMem.save[eidx] ? browser_cset.edit.main : browser_cset.edit.change);
     twDirectWrite(j + 1,i + 1,&EditorMem.buff[eidx],1);
   }
   if((unsigned)EditorMem.alen[i] + 1 < EditorMem.width)
   {
      twGotoXY(EditorMem.alen[i] + 1,i + 1); twClrEOL();
   }
 }
 __MsSetState(True);
 PaintETitle(edit_y*EditorMem.width + edit_x,0);
 twShowWin(twUsedWin());
 twSetCursorType(TW_CUR_NORM);
 redraw = True;
 if(txtwnd)
 {
   char work[__TVIO_MAXSCREENWIDTH];
   int len;
   TWindow * using = twUsedWin();
   twUseWin(txtwnd);
   twSetColorAttr(browser_cset.main);
   twFreezeWin(txtwnd);
   for(i = 0;i < height;i++)
   {
      mlen = EditorMem.alen[i];
      len = ExpandHex(work,&EditorMem.buff[i*EditorMem.width],mlen,2);
      twDirectWrite(11,i + 1,work,len);
      if((unsigned)EditorMem.alen[i] + 1 < EditorMem.width)
      {
        twGotoXY(11+len,i + 1); twClrEOL();
      }
   }
   twRefreshWin(txtwnd);
   twUseWin(using);
 }
 while(1)
 {
   unsigned eidx;
   eidx = edit_y*EditorMem.width;
   mlen = EditorMem.alen[edit_y];
   flags = attr;
   if(!redraw) flags |= __ESS_NOREDRAW;
   _lastbyte = eeditstring((char *)&EditorMem.buff[eidx],NULL,&mlen,(unsigned)(edit_y + 1),
                           (unsigned *)&edit_x,flags,(char *)&EditorMem.save[eidx], NULL);
   switch(_lastbyte)
   {
     case KE_F(1)   : ExtHelp(); continue;
     case KE_F(2)   : save_func?save_func(EditorMem.buff,EditorMem.size):editSaveContest();
     case KE_F(10)  :
     case KE_ESCAPE : goto bye;
     case KE_TAB : if(txtwnd) goto bye;
     default     : redraw = editDefAction(_lastbyte); break;
   }
   CheckBounds();
   if(redraw)
   {
     if(txtwnd)
     {
      char work[__TVIO_MAXSCREENWIDTH];
      int len;
      TWindow * using = twUsedWin();
      twUseWin(txtwnd);
      len = ExpandHex(work,&EditorMem.buff[edit_y*EditorMem.width],mlen,2);
      twDirectWrite(11,edit_y + 1,work,len);
      twUseWin(using);
     }
   }
   PaintETitle(edit_y*EditorMem.width + edit_x,0);
 }
 bye:
 twSetCursorType(TW_CUR_OFF);
 return _lastbyte;
}
