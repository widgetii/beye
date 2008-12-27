/**
 * @namespace   biew
 * @file        dialogs.c
 * @brief       This file contains common dialogs of BIEW project.
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
#include "bmfile.h"
#include "tstrings.h"
#include "biewhelp.h"
#include "bconsole.h"
#include "biewutil.h"
#include "bin_util.h"
#include "biewlib/kbd_code.h"
#include "biewlib/twin.h"

tBool __FASTCALL__ Get2DigitDlg(const char *title,const char * text,unsigned char *xx)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,X2,Y2;
 TWindow * hwnd,*ewnd,*using;
 tBool ret;
 int retval;
 char str[3] = "";
 using = twUsedWin();
 hwnd = CrtDlgWndnls(title,24,1);
 twUseWin(hwnd);
 twGetWinPos(hwnd,&x1,&y1,&x2,&y2);
 twGotoXY(1,1); twPutS(text);
 X1 = x1;
 Y1 = y1;
 X2 = x2;
 Y2 = y2;
 X1 += 22;
 Y1 += 1;
 X2 = X1 + 1;
 Y2 = Y1;
 ewnd = CreateEditor(X1,Y1,X2,Y2,TWS_VISIBLE | TWS_CURSORABLE | TWS_NLSOEM);
 twUseWin(ewnd);
 if(*xx) sprintf(str,"%X",(unsigned int)*xx);
 while(1)
 {
   retval = xeditstring(str,&legalchars[2],2,NULL);
   if(retval == KE_ESCAPE || retval == KE_F(10)) { ret = False; break; }
   else
     if(retval == KE_ENTER) { ret = True; break; }
 }
 CloseWnd(ewnd);
 CloseWnd(hwnd);
 if(ret) *xx = (unsigned char)strtoul(str,NULL,16);
 twUseWin(using);
 return ret;
}

#define HEX     0x00
#define UNSIGN  0x00
#define SIGN    0x01
#define DECIMAL 0x02

tBool __FASTCALL__ Get8DigitDlg(const char *title,const char *text,char attr,unsigned long *xx)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,X2,Y2;
 int key;
 TWindow * hwnd,*ewnd,*using;
 char base = attr & DECIMAL ? 10 : 16;
 char len  = attr & DECIMAL ? 10 : 8;
 tBool ret;
 char decleg[13];
 char str[12] = "";
 char * legals;
 memcpy(decleg,legalchars,12);
 decleg[12] = '\0';
 len += attr & SIGN ? 1 : 0;
 using = twUsedWin();
 hwnd = CrtDlgWndnls(title,34,1);
 twUseWin(hwnd);
 twGetWinPos(hwnd,&x1,&y1,&x2,&y2);
 twGotoXY(1,1); twPutS(text);
 X1 = x1;
 Y1 = y1;
 X2 = x2;
 Y2 = y2;
 Y1 += 1;
 X2 = X1 + 33;
 X1 = X2 - (len - 1);
 Y2 = Y1;
 ewnd = CreateEditor(X1,Y1,X2,Y2,TWS_VISIBLE | TWS_CURSORABLE | TWS_NLSOEM);
 twUseWin(ewnd);
 if(attr & DECIMAL) legals = attr & SIGN ? decleg : &decleg[2];
 else               legals = attr & SIGN ? legalchars : &legalchars[2];
 if(*xx)
 {
   if(attr & SIGN) ltoa(*xx,str,base);
   else            ultoa(*xx,str,base);
 }
 while(1)
 {
   key = xeditstring(str,legals,len,NULL);
   if(key == KE_ESCAPE || key == KE_F(10)) { ret = False; break; }
   else
     if(key == KE_ENTER) { ret = True; break; }
 }
 CloseWnd(ewnd);
 CloseWnd(hwnd);
 if(ret) *xx = attr & SIGN ? (unsigned long)strtol(str,NULL,base) : strtoul(str,NULL,base);
 twUseWin(using);
 return ret;
}

#if __WORDSIZE >= 32
tBool        __FASTCALL__ Get16DigitDlg(const char *title,const char *text,char attr,
					unsigned long long int *xx)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,X2,Y2;
 int key;
 TWindow * hwnd,*ewnd,*using;
 char base = attr & DECIMAL ? 10 : 16;
 char len  = attr & DECIMAL ? 20 : 16;
 tBool ret;
 char decleg[13];
 char str[20] = "";
 char * legals;
 memcpy(decleg,legalchars,12);
 decleg[12] = '\0';
 len += attr & SIGN ? 1 : 0;
 using = twUsedWin();
 hwnd = CrtDlgWndnls(title,44,1);
 twUseWin(hwnd);
 twGetWinPos(hwnd,&x1,&y1,&x2,&y2);
 twGotoXY(1,1); twPutS(text);
 X1 = x1;
 Y1 = y1;
 X2 = x2;
 Y2 = y2;
 Y1 += 1;
 X2 = X1 + 43;
 X1 = X2 - (len - 1);
 Y2 = Y1;
 ewnd = CreateEditor(X1,Y1,X2,Y2,TWS_VISIBLE | TWS_CURSORABLE | TWS_NLSOEM);
 twUseWin(ewnd);
 if(attr & DECIMAL) legals = attr & SIGN ? decleg : &decleg[2];
 else               legals = attr & SIGN ? legalchars : &legalchars[2];
 if(*xx)
 {
   if(attr & SIGN) lltoa(*xx,str,base);
   else            ulltoa(*xx,str,base);
 }
 while(1)
 {
   key = xeditstring(str,legals,len,NULL);
   if(key == KE_ESCAPE || key == KE_F(10)) { ret = False; break; }
   else
     if(key == KE_ENTER) { ret = True; break; }
 }
 CloseWnd(ewnd);
 CloseWnd(hwnd);
 if(ret) *xx = attr & SIGN ? (unsigned long long int)strtoll(str,NULL,base) : strtoull(str,NULL,base);
 twUseWin(using);
 return ret;
}
#endif

static void __NEAR__ __FASTCALL__ paintJumpDlg(TWindow *wdlg,unsigned long flags)
{
  TWindow *usd;
  usd = twUsedWin();
  twUseWin(wdlg);
  twSetColorAttr(dialog_cset.group.active);
  twGotoXY(4,2); twPutChar(flags == GJDLG_ABSOLUTE ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  twGotoXY(4,3); twPutChar(flags == GJDLG_RELATIVE ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  twGotoXY(4,4); twPutChar(flags == GJDLG_REL_EOF  ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  twGotoXY(4,5); twPutChar(flags == GJDLG_VIRTUAL  ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  twGotoXY(4,6); twPutChar(flags == GJDLG_PERCENTS ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  twUseWin(usd);
}

static const char * jmptxt[] =
{
  "      ",
  "Mode  ",
  "      ",
  "      ",
  "UsrNam",
  "      ",
  "      ",
  "      ",
  "      ",
  "Escape"
};

static void drawJumpPrompt( void )
{
   __drawSinglePrompt(jmptxt);
}


tBool __FASTCALL__ GetJumpDlg( __filesize_t * addr,unsigned long *flags)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,X2,Y2;
 int key;
 TWindow * hwnd,*ewnd,*using;
 unsigned len = HA_LEN-1,stx = 0;
 tBool ret,update;
 static char str[12] = "";
 char * legals;
 unsigned attr;
 using = twUsedWin();
 hwnd = CrtDlgWndnls(" Jump within file ",(BMFileFlags&BMFF_USE64)?34:26,6);
 twUseWin(hwnd);
 twGetWinPos(hwnd,&x1,&y1,&x2,&y2);
 twGotoXY(2,1); twPutS("Enter offset :");
 twSetColorAttr(dialog_cset.group.active);
 twGotoXY(2,2); twPutS(" ( ) - Absolute       ");
 twGotoXY(2,3); twPutS(" ( ) - Relative       ");
 twGotoXY(2,4); twPutS(" ( ) - Relatively EOF ");
 twGotoXY(2,5); twPutS(" ( ) - Virtual        ");
 twGotoXY(2,6); twPutS(" ( ) - Percents       ");
 twSetColorAttr(dialog_cset.main);
 X1 = x1;
 Y1 = y1;
 X2 = x2;
 Y2 = y2;
 Y1 += 1;
 X1 += 17;
 X2 = X1 + len - 1;
 Y2 = Y1;
 ewnd = CreateEditor(X1,Y1,X2,Y2,TWS_VISIBLE | TWS_CURSORABLE | TWS_NLSOEM);
 twUseWin(ewnd);
 legals = *flags == GJDLG_RELATIVE ? legalchars : &legalchars[2];
 paintJumpDlg(hwnd,*flags);
 update = True;
 while(1)
 {
   attr = __ESS_NOTUPDATELEN | __ESS_WANTRETURN | __ESS_ENABLEINSERT;
   if(!update) attr |= __ESS_NOREDRAW;
   key = eeditstring(str,legals,&len,1,&stx,
                     attr,NULL,drawJumpPrompt);
   if(key == KE_ESCAPE || key == KE_F(10)) { ret = False; break; }
   else
     if(key == KE_ENTER) { ret = True; break; }
   update = True;
   switch(key)
   {
      case KE_F(1):  hlpDisplay(6);
                     update = False;
                     break;
      case KE_F(5):  if(udnSelectName(addr)) {
			if(BMFileFlags&BMFF_USE64) sprintf(str,"%016llX",*addr);
			else			   sprintf(str,"%08lX",(unsigned long)*addr);
		     }
		     break;
      case KE_F(2):  if(((*flags)&0xFF) < GJDLG_PERCENTS) (*flags)++;
                     else                                 (*flags) = 0;
                     legals = (*flags) == GJDLG_RELATIVE ||
                              (*flags) == GJDLG_REL_EOF ? legalchars : &legalchars[2];
                     update = False;
                     break;
      case KE_LEFTARROW:
      case KE_RIGHTARROW:
                     update = False;
                     break;
      default:       break;
   }
   paintJumpDlg(hwnd,*flags);
 }
 CloseWnd(ewnd);
 CloseWnd(hwnd);
 if(ret)
 {
#if __WORDSIZE >= 32
 if(BMFileFlags&BMFF_USE64)
    *addr = (*flags) == GJDLG_RELATIVE ||
            (*flags) == GJDLG_REL_EOF ? (unsigned long long int)strtoll(str,NULL,16) : strtoull(str,NULL,16);
 else
#endif
    *addr = (*flags) == GJDLG_RELATIVE ||
            (*flags) == GJDLG_REL_EOF ? (unsigned long)strtol(str,NULL,16) : strtoul(str,NULL,16);
 }
 twUseWin(using);
 return ret;
}

tBool __FASTCALL__ GetStringDlg(char * buff,const char * title,const char *subtitle,const char *prompt)
{
  tAbsCoord x1,y1,x2,y2;
  tRelCoord X1,Y1,X2,Y2;
  int key;
  tBool ret;
  TWindow * wdlg,*ewnd;
  char estr[81];
  wdlg = CrtDlgWndnls(title,78,2);
  if(subtitle) twSetFooterAttr(wdlg,subtitle,TW_TMODE_RIGHT,dialog_cset.footer);
  twGetWinPos(wdlg,&x1,&y1,&x2,&y2);
  X1 = x1;
  Y1 = y1;
  X2 = x2;
  Y2 = y2;
  X1 += 2;
  X2 -= 2;
  Y1 += 2;
  Y2 = Y1;
  ewnd = CreateEditor(X1,Y1,X2,Y2,TWS_CURSORABLE | TWS_NLSOEM);
  twUseWin(wdlg);
  twGotoXY(2,1); twPutS(prompt);
  strcpy(estr,buff);
  twShowWin(ewnd);
  twUseWin(ewnd);
  while(1)
  {
   key = xeditstring(estr,NULL,76,NULL);
   if(key == KE_ESCAPE || key == KE_F(10)) { ret = False; break; }
   else
     if(key == KE_ENTER) { ret = True; break; }
  }
  if(ret) strcpy(buff,estr);
  CloseWnd(ewnd);
  CloseWnd(wdlg);
  return ret;
}

static void __NEAR__ __FASTCALL__ FFStaticPaint(TWindow * wdlg,char * fname,char * st,char *end,unsigned long flg)
{
  int len,i;
  TWindow * using = twUsedWin();
    twUseWin(wdlg);
    if(!(flg & FSDLG_USEBITNS))
    {
      len = strlen(fname);
      twGotoXY(3,7); twPutS(fname); for(i = len;i < 71;i++) twPutChar(TWC_MED_SHADE);
    }
    len = strlen(st);
    twGotoXY(8,4); twPutS(st);
#if __WORDSIZE >= 32
    for(i = len;i < 18;i++)
#else
    for(i = len;i < 10;i++)
#endif
	twPutChar(TWC_MED_SHADE);
    len = strlen(end);
    twGotoXY(35,4); twPutS(end);
#if __WORDSIZE >= 32
    for(i = len;i < 18;i++)
#else
    for(i = len;i < 10;i++)
#endif
	twPutChar(TWC_MED_SHADE);
    if(flg & FSDLG_USEMODES)
    {
      twSetColorAttr(dialog_cset.group.active);
      twGotoXY(54,1); twPutS(msgTypeComments[0]);
      twGotoXY(61,1); twPutS(flg & FSDLG_ASMMODE ? " Asm              " : " Bin              ");
      if(!(flg & FSDLG_ASMMODE)) twSetColorAttr(dialog_cset.group.disabled);
      for(i = 1;i < 5;i++) { twGotoXY(54,i + 1); twPutS(msgTypeComments[i]); }
      twGotoXY(56,2); twPutChar(flg & FSDLG_STRUCTS ? TWC_CHECK_CHAR : TWC_DEF_FILLER);
      twGotoXY(56,4); twPutChar(flg & FSDLG_COMMENT ? TWC_DEF_FILLER : TWC_RADIO_CHAR);
      twGotoXY(56,5); twPutChar(flg & FSDLG_COMMENT ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      twSetColorAttr(dialog_cset.main);
    }
    else
    if(flg & FSDLG_USEBITNS)
    {
      twSetColorAttr(dialog_cset.group.active);
      for(i = 0;i < 4;i++) { twGotoXY(54,i + 1); twPutS(msgTypeBitness[i]); }
#ifndef INT64_C
      twSetColorAttr(dialog_cset.group.disabled);
#endif
      twGotoXY(54,5); twPutS(msgTypeBitness[i]);
#ifndef INT64_C
      twSetColorAttr(dialog_cset.group.active);
#endif
      twGotoXY(56,2); twPutChar((flg & FSDLG_BTNSMASK) == 0 ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      twGotoXY(56,3); twPutChar((flg & FSDLG_BTNSMASK) == 1 ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      twGotoXY(56,4); twPutChar((flg & FSDLG_BTNSMASK) == 2 ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      twGotoXY(56,5); twPutChar((flg & FSDLG_BTNSMASK) == 3 ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      twSetColorAttr(dialog_cset.main);
    }
    twUseWin(using);
}

static const char * fs1_txt[] =
{
  "      ",
  "Mode  ",
  "Coment",
  "Struct",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "Escape"
};

static const char * fs2_txt[] =
{
  "      ",
  "Bitnes",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "Escape"
};

static const char * fs3_txt[] =
{
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "Escape"
};

const char ** fs_txt;

static void drawFSPrompt( void )
{
   __drawSinglePrompt(fs_txt);
}

tBool __FASTCALL__ GetFStoreDlg(const char *title,char * fname,unsigned long * flags,
                   __filesize_t * start,__filesize_t * end,
                   const char *prompt)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,XX1,YY1,XX2,YY2;
 TWindow *wdlg = CrtDlgWndnls(title,78,8);
 TWindow *ewnd[3];
 int i,active,oactive,_lastbyte, neditors;
 tBool redraw;
 unsigned attr,stx = 0;
#if __WORDSIZE >= 32
 char startdig[19],enddig[19];
 unsigned mlen[3] = { 19, 19, 71 };
#else
 char startdig[11],enddig[11];
 unsigned mlen[3] = { 10, 10, 71 };
#endif
 char * legal[3] = { &legalchars[2], &legalchars[2], NULL };
 char *wbuff[3];

 if((*flags) & FSDLG_USEMODES) fs_txt = fs1_txt;
 else
 if((*flags) & FSDLG_USEBITNS) fs_txt = fs2_txt;
 else                          fs_txt = fs3_txt;
 twSetFooterAttr(wdlg," [Enter] - Run ",TW_TMODE_CENTER,dialog_cset.footer);

 twGetWinPos(wdlg,&x1,&y1,&x2,&y2);
 X1 = x1;
 Y1 = y1;

 wbuff[0] = startdig;
 wbuff[1] = enddig;
 wbuff[2] = fname;

 neditors = 2;
 XX1 = X1 + 3;
 YY1 = Y1 + 7;
 XX2 = XX1 + 70;
 YY2 = YY1;
 if(!((*flags) & FSDLG_USEBITNS))
 {
   ewnd[2] = CreateEditor(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
   neditors++;
 }
 XX1 += 5;
#if __WORDSIZE >= 32
 XX2  = XX1 + 17;
#else
 XX2  = XX1 + 9;
#endif
 YY1 -= 3;
 YY2  = YY1;
 ewnd[0] = CreateEditor(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 XX1 += 27;
#if __WORDSIZE >= 32
 XX2 = XX1 + 17;
#else
 XX2 = XX1 + 9;
#endif
 ewnd[1] = CreateEditor(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twUseWin(wdlg);
 twGotoXY(1,2);  twPutS(TYPE_HEX_FORM);
 if(prompt)    { twGotoXY(3,6);  twPutS(prompt); }
 twGotoXY(1,4);  twPutS(START_PRMT);
 twGotoXY(27,4); twPutS(LENGTH_PRMT);
 ultoa(*start,startdig,16);
 ultoa(*end,enddig,16);
 FFStaticPaint(wdlg,fname,startdig,enddig,*flags);
 active = 0;
 oactive = 1;
 redraw = True;
 neditors--;
 while(1)
 {
   if(active != oactive)
   {
     twHideWin(ewnd[oactive]);
     twShowWin(ewnd[active]);
     twUseWin(ewnd[active]);
     oactive = active;
     stx = 0;
   }
   attr = __ESS_NOTUPDATELEN | __ESS_WANTRETURN | __ESS_ENABLEINSERT;
   if(!redraw) attr |= __ESS_NOREDRAW;
   _lastbyte = eeditstring(wbuff[active],legal[active],&mlen[active],1,&stx,attr,NULL,drawFSPrompt);
   if(_lastbyte == KE_ESCAPE || _lastbyte == KE_ENTER || _lastbyte == KE_F(10))
                                                                          break;
   redraw = True;
   switch(_lastbyte)
   {
     case KE_TAB         : active++; break;
     case KE_SHIFT_TAB   : active--; break;
     case KE_LEFTARROW   :
     case KE_RIGHTARROW  : redraw = False; break;
     case KE_F(2)        : if((*flags) & FSDLG_USEMODES) *flags ^= FSDLG_ASMMODE;
                           else
                            if((*flags) & FSDLG_USEBITNS)
                            {
                              unsigned long val;
                              val = ((*flags) & FSDLG_BTNSMASK);
                              val++;
#ifndef INT64_C
                              if(val > 2) val = 0;
#endif
                              val &= FSDLG_BTNSMASK;
                              (*flags) &= ~FSDLG_BTNSMASK;
                              (*flags) |= val;
                            }
                           break;
     case KE_F(3)        : if(((*flags) & FSDLG_USEMODES) && ((*flags) & FSDLG_ASMMODE))
                                                         *flags ^= FSDLG_COMMENT;
                           break;
     case KE_F(4)        : if(((*flags) & FSDLG_USEMODES) && ((*flags) & FSDLG_ASMMODE))
                                                         *flags ^= FSDLG_STRUCTS;
                           break;
     default:              break;
   }
   if(active < 0) active = neditors;
   if(active > neditors) active = 0;
   if(redraw) FFStaticPaint(wdlg,fname,startdig,enddig,*flags);
 }
 CloseWnd(wdlg);
 for(i = 0;i < neditors+1;i++) CloseWnd(ewnd[i]);
 *start = strtoul(startdig,NULL,16);
 *end = strtoul(enddig,NULL,16);
 return !(_lastbyte == KE_ESCAPE || _lastbyte == KE_F(10));
}

static void __NEAR__ __FASTCALL__ FFStaticPaintInsDel(TWindow * wdlg,char * st,char *end)
{
  int len,i;
  TWindow * using = twUsedWin();
    twUseWin(wdlg);
    len = strlen(st);
    twGotoXY(8,4); twPutS(st);
#if __WORDSIZE >= 32
    for(i = len;i < 18;i++)
#else
    for(i = len;i < 10;i++)
#endif
	twPutChar(TWC_MED_SHADE);
    len = strlen(end);
    twGotoXY(35,4); twPutS(end);
#if __WORDSIZE >= 32
    for(i = len;i < 18;i++)
#else
    for(i = len;i < 10;i++)
#endif
	twPutChar(TWC_MED_SHADE);
    twUseWin(using);
}

tBool __FASTCALL__ GetInsDelBlkDlg(const char *title,__filesize_t * start,__fileoff_t * size)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,XX1,YY1,XX2,YY2;
 TWindow * wdlg = CrtDlgWndnls(title,78,5);
 TWindow * ewnd[2];
 int i,active,oactive,_lastbyte;
 unsigned stx = 0,attr;
 tBool redraw;
 char startdig[11],enddig[11];
 char * legal[2] = { &legalchars[2], legalchars };
#if __WORDSIZE >= 32
 unsigned mlen[2] = { 19, 19 };
#else
 unsigned mlen[2] = { 10, 10 };
#endif
 char *wbuff[2];

 twGetWinPos(wdlg,&x1,&y1,&x2,&y2);
 X1 = x1;
 Y1 = y1;

 wbuff[0] = startdig;
 wbuff[1] = enddig;
 XX1 = X1 + 8;
 YY2 = YY1 = Y1 + 4;
#if __WORDSIZE >= 32
 XX2 = XX1 + 17;
#else
 XX2 = XX1 + 9;
#endif
 ewnd[0] = CreateEditor(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 XX1 += 27;
#if __WORDSIZE >= 32
 XX2 = XX1 + 17;
#else
 XX2 = XX1 + 9;
#endif
 ewnd[1] = CreateEditor(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twUseWin(wdlg);
 twGotoXY(1,2); twPutS(TYPE_HEX_FORM);
 twGotoXY(1,4);  twPutS(START_PRMT);
 twGotoXY(27,4); twPutS(LENGTH_PRMT);
 twinDrawFrameAttr(53,1,78,5,TW_UP3D_FRAME,dialog_cset.main);
 twGotoXY(55,2); twPutS("Remarks:             ");
 twGotoXY(55,3); twPutS("+(pos) - insert block");
 twGotoXY(55,4); twPutS("-(neg) - delete block");
 ultoa(*start,startdig,16);
 if(*size < 0)
 {
   ltoa(labs(*size),&enddig[1],16);
   enddig[0] = '-';
 }
 else ltoa(*size,enddig,16);
 FFStaticPaintInsDel(wdlg,startdig,enddig);
 active = 0;
 oactive = 1;
 redraw = True;
 while(1)
 {
   if(active != oactive)
   {
     twHideWin(ewnd[oactive]);
     twShowWin(ewnd[active]);
     twUseWin(ewnd[active]);
     oactive = active;
     stx = 0;
   }
   attr = __ESS_NOTUPDATELEN | __ESS_WANTRETURN | __ESS_ENABLEINSERT;
   if(!redraw) attr |= __ESS_NOREDRAW;
   _lastbyte = eeditstring(wbuff[active],legal[active],&mlen[active],1,&stx,attr,NULL,NULL);
   if(_lastbyte == KE_ESCAPE || _lastbyte == KE_ENTER || _lastbyte == KE_F(10))
                                                                         break;
   redraw = True;
   switch(_lastbyte)
   {
     case KE_TAB        : active++; break;
     case KE_SHIFT_TAB  : active--; break;
     case KE_LEFTARROW  :
     case KE_RIGHTARROW : redraw = False; break;
     default: break;
   }
   if(active != oactive) active = active > 1 ? 0 : 1;
   if(redraw) FFStaticPaintInsDel(wdlg,startdig,enddig);
 }
 CloseWnd(wdlg);
 for(i = 0;i < 2;i++) CloseWnd(ewnd[i]);
 *start = strtoul(startdig,NULL,16);
 *size = strtol(enddig,NULL,16);
 return !(_lastbyte == KE_ESCAPE || _lastbyte == KE_F(10));
}
