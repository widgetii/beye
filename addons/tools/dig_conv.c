/**
 * @namespace   biew_addons
 * @file        addons/tools/dig_conv.c
 * @brief       This file contains implementation of digital convertor.
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
#include <string.h>
#include <stdlib.h>

#include "bconsole.h"
#include "biewutil.h"
#include "colorset.h"
#include "reg_form.h"
#include "biewlib/biewlib.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

static int __NEAR__ __FASTCALL__ GetFullBin(tUIntMax value,char * buff)
{
 char byte,*b;
 tBool started = False;
 buff[0] = 0;
#if __WORDSIZE >= 32
 byte = (value >> 60) & 0x0F;
 if(byte) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
 byte = (value >> 56) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
 byte = (value >> 52) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
 byte = (value >> 48) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
 byte = (value >> 44) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
 byte = (value >> 40) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
 byte = (value >> 36) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
 byte = (value >> 32) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
 byte = (value >> 28) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
#else
 byte = (value >> 28) & 0x0F;
 if(byte) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
#endif
 byte = (value >> 24) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
 byte = (value >> 20) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
 byte = (value >> 16) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
 byte = (value >> 12) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
 byte = (value >> 8) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
 byte = (value >> 4) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); }
 byte = value & 0x0F;
 b = GetBinary(byte); strcat(buff,&b[4]);
 return strlen(buff);
}

static int __NEAR__ __FASTCALL__ Dig2Str(tUIntMax value,char * buff,int action)
{
#if __WORDSIZE >= 32
 if(action == 0) return strlen(ulltoa(value,buff,16));
 if(action == 1) return strlen(lltoa(value,buff,8));
 if(action == 2) return strlen(ulltoa(value,buff,10));
 if(action == 3) return strlen(lltoa(value,buff,10));
 if(action == 4) return GetFullBin(value,buff);
 if(action == 5) return strlen(ulltoa(value,buff,2));
#else
 if(action == 0) return strlen(ultoa(value,buff,16));
 if(action == 1) return strlen(ltoa(value,buff,8));
 if(action == 2) return strlen(ultoa(value,buff,10));
 if(action == 3) return strlen(ltoa(value,buff,10));
 if(action == 4) return GetFullBin(value,buff);
 if(action == 5) return strlen(ultoa(value,buff,2));
#endif
 return 0;
}

static tUIntMax __NEAR__ __FASTCALL__ Str2Dig(char * buff,int action)
{
#if __WORDSIZE >= 32
 if(action == 0) return strtoull(buff,NULL,16);
 if(action == 1) return strtoull(buff,NULL,8);
 if(action == 2) return strtoull(buff,NULL,10);
 if(action == 3) return strtoll(buff,NULL,10);
 if(action == 4) return strtoull(buff,NULL,2);
#else
 if(action == 0) return strtoul(buff,NULL,16);
 if(action == 1) return strtoul(buff,NULL,8);
 if(action == 2) return strtoul(buff,NULL,10);
 if(action == 3) return strtol(buff,NULL,10);
 if(action == 4) return strtoul(buff,NULL,2);
#endif
 return 0;
}

static void __NEAR__ __FASTCALL__ DCStaticPaint(TWindow * wdlg,char * wbuff,tIntMax digit,unsigned *mlen)
{
 int rlen;
 tAbsCoord x1,y1,x2,y2;
 unsigned i,w;
 TWindow * using = twUsedWin();
    twUseWin(wdlg);
    twGetWinPos(wdlg,&x1,&y1,&x2,&y2);
    w=x2-x1;
    rlen = Dig2Str(digit,wbuff,0);
    twGotoXY(3,4); twPutS(wbuff); for(i = rlen;i < mlen[0];i++)  twPutChar('±'); twPutS("   [Hex]");
    rlen = Dig2Str(digit,wbuff,1);
    twGotoXY(w-11-mlen[0],4); twPutS(wbuff); for(i = rlen;i < mlen[1];i++)  twPutChar('±'); twPutS(" [Oct]");
    rlen = Dig2Str(digit,wbuff,2);
    twGotoXY(3,6); twPutS(wbuff); for(i = rlen;i < mlen[2];i++)  twPutChar('±'); twPutS(" [Dec]");
    rlen = Dig2Str(digit,wbuff,3);
    twGotoXY(w-11-mlen[0],6); twPutS(wbuff); for(i = rlen;i < mlen[3];i++) twPutChar('±'); twPutS(" [+-Dec]");
    rlen = Dig2Str(digit,wbuff,4);
    twGotoXY(5,8); twPutS(wbuff); for(i = rlen;i < mlen[4];i++)  twPutChar('±'); twPutS(" [Bin]");
    twUseWin(using);
}

static void DigConv( void )
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,XX1,XX2,YY1,YY2;
#if __WORDSIZE >= 32
 TWindow * wdlg = CrtDlgWndnls(" Digital convertor ",78,9);
 unsigned mlen[5] = { 16, 22, 19, 20, 64 };
#else
 TWindow * wdlg = CrtDlgWndnls(" Digital convertor ",48,9);
 unsigned mlen[5] = { 8, 11, 10, 11, 32 };
#endif
 TWindow * ewnd[5];
 int i,active,oactive,_lastbyte;
 unsigned attr,stx = 0,rlen,w;
 char wbuff[68];
 tBool redraw;
 char * legal[5];
 tUIntMax digit;
 char decleg[13],oleg[9],bleg[3];
 memcpy(decleg,legalchars,12);
 decleg[12] = '\0';
 memcpy(oleg,&legalchars[2],8);
 oleg[8] = '0';
 memcpy(bleg,oleg,2);
 bleg[2] = '\0';
 legal[0] = &legalchars[2];
 legal[1] = oleg;
 legal[2] = &decleg[2];
 legal[3] = decleg;
 legal[4] = bleg;
 twGetWinPos(wdlg,&x1,&y1,&x2,&y2);
 w=x2-x1;
 X1 = x1;
 Y1 = y1;
 XX1 = X1 + 3;
 YY1 = Y1 + 4;
 XX2 = XX1 + (mlen[0]-1);
 YY2 = YY1;
 ewnd[0] = WindowOpen(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twSetColorAttr(dialog_cset.editor.active);
 XX1 = X1 + (w-11-mlen[0]);
 XX2 = XX1 + (mlen[1]-1);
 ewnd[1] = WindowOpen(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twSetColorAttr(dialog_cset.editor.active);
 XX1 = X1 + 3;
 YY1 = Y1 + 6;
 XX2 = XX1 + (mlen[2]-1);
 YY2 = YY1;
 ewnd[2] = WindowOpen(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twSetColorAttr(dialog_cset.editor.active);
 XX1 = X1 + (w-11-mlen[0]);
 XX2 = XX1 + (mlen[3]-1);
 ewnd[3] = WindowOpen(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twSetColorAttr(dialog_cset.editor.active);
 XX1 = X1 + 5;
 YY1 = Y1 + 8;
 XX2 = XX1 + (mlen[4]-1);
 YY2 = YY1;
 ewnd[4] = WindowOpen(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twSetColorAttr(dialog_cset.editor.active);
 twUseWin(wdlg);
 digit = 0;
 twGotoXY(3,2); twPutS("Convert numbers between bases [16, 10, 8, 2]");
 DCStaticPaint(wdlg,wbuff,digit,mlen);
 oactive = 1;
 active =
 wbuff[0] = 0;
 digit = 0;
 attr = __ESS_NOTUPDATELEN | __ESS_WANTRETURN | __ESS_ENABLEINSERT;
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
   if(active == 4)
   {
     rlen=0;
   }
   rlen = Dig2Str(digit,wbuff,active == 4 ? 5 : active);
   if(stx > rlen) stx = rlen;
   NextCh:
   _lastbyte = eeditstring(wbuff,legal[active],&mlen[active],1,&stx,attr,
                          NULL,NULL);
   if((char)_lastbyte == '-' || (char)_lastbyte == '+') goto NextCh;
   if(_lastbyte == KE_ESCAPE || _lastbyte == KE_F(10)) break;
   digit = Str2Dig(wbuff,active);
   redraw = False;
   switch(_lastbyte)
   {
     case KE_TAB        : active++; break;
     case KE_SHIFT_TAB  : active--; break;
     case KE_LEFTARROW  :
     case KE_RIGHTARROW : break;
     case KE_UPARROW    : active -= 2; break;
     case KE_DOWNARROW  : active += 2; break;
     default: redraw = True; break;
   }
   if(active < 0) active = 4;
   if(active > 4) active = 0;
   if(redraw) DCStaticPaint(wdlg,wbuff,digit,mlen);
 }
 CloseWnd(wdlg);
 for(i = 0;i < 5;i++) CloseWnd(ewnd[i]);
}

REGISTRY_TOOL DigitalConvertor =
{
  "~Digital convertor",
  DigConv,
  NULL,
  NULL
};
