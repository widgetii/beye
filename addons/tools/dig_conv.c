/**
 * @namespace   biew_addons
 * @file        addons/tools/dig_conv.c
 * @brief       This file contains implementation of digital convertor.
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

#include "bconsole.h"
#include "biewutil.h"
#include "colorset.h"
#include "reg_form.h"
#include "biewlib/biewlib.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

static int __NEAR__ __FASTCALL__ GetFullBin(unsigned long value,char * buff)
{
 char byte,*b;
 tBool started = False;
 buff[0] = 0;
 byte = (value >> 28) & 0x0F;
 if(byte) { b = GetBinary(byte); strcat(buff,&b[4]); started = True; }
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

static int __NEAR__ __FASTCALL__ Dig2Str(unsigned long value,char * buff,int action)
{
 if(action == 0) return strlen(ultoa(value,buff,16));
 if(action == 1) return strlen(ltoa(value,buff,8));
 if(action == 2) return strlen(ultoa(value,buff,10));
 if(action == 3) return strlen(ltoa(value,buff,10));
 if(action == 4) return GetFullBin(value,buff);
 if(action == 5) return strlen(ultoa(value,buff,2));
 return 0;
}

static unsigned long __NEAR__ __FASTCALL__ Str2Dig(char * buff,int action)
{
 if(action == 0) return strtoul(buff,NULL,16);
 if(action == 1) return strtoul(buff,NULL,8);
 if(action == 2) return strtoul(buff,NULL,10);
 if(action == 3) return strtol(buff,NULL,10);
 if(action == 4) return strtoul(buff,NULL,2);
 return 0;
}

static void __NEAR__ __FASTCALL__ DCStaticPaint(TWindow * wdlg,char * wbuff,long digit)
{
 int rlen,i;
 TWindow * using = twUsedWin();
    twUseWin(wdlg);
    rlen = Dig2Str(digit,wbuff,0);
    twGotoXY(3,4); twPutS(wbuff); for(i = rlen;i < 8;i++)  twPutChar('±'); twPutS("   [Hex]");
    rlen = Dig2Str(digit,wbuff,1);
    twGotoXY(29,4); twPutS(wbuff); for(i = rlen;i < 11;i++)  twPutChar('±'); twPutS(" [Oct]");
    rlen = Dig2Str(digit,wbuff,2);
    twGotoXY(3,6); twPutS(wbuff); for(i = rlen;i < 10;i++)  twPutChar('±'); twPutS(" [Dec]");
    rlen = Dig2Str(digit,wbuff,3);
    twGotoXY(29,6); twPutS(wbuff); for(i = rlen;i < 11;i++) twPutChar('±'); twPutS(" [+-Dec]");
    rlen = Dig2Str(digit,wbuff,4);
    twGotoXY(5,8); twPutS(wbuff); for(i = rlen;i < 32;i++)  twPutChar('±'); twPutS(" [Bin]");
    twUseWin(using);
}

static void DigConv( void )
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,XX1,XX2,YY1,YY2;
 TWindow * wdlg = CrtDlgWndnls(" Digital convertor ",48,9);
 TWindow * ewnd[5];
 int i,active,oactive,_lastbyte;
 unsigned attr,stx = 0,rlen;
 char wbuff[34];
 tBool redraw;
 char * legal[5];
 unsigned mlen[5] = { 8, 11, 10, 11, 32 };
 unsigned long digit;
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
 X1 = x1;
 Y1 = y1;
 XX1 = X1 + 3;
 YY1 = Y1 + 4;
 XX2 = XX1 + 7;
 YY2 = YY1;
 ewnd[0] = WindowOpen(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twSetColorAttr(dialog_cset.editor.active);
 XX1 = X1 + 29;
 XX2 = XX1 + 10;
 ewnd[1] = WindowOpen(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twSetColorAttr(dialog_cset.editor.active);
 XX1 = X1 + 3;
 YY1 = Y1 + 6;
 XX2 = XX1 + 9;
 YY2 = YY1;
 ewnd[2] = WindowOpen(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twSetColorAttr(dialog_cset.editor.active);
 XX1 = X1 + 29;
 XX2 = XX1 + 10;
 ewnd[3] = WindowOpen(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twSetColorAttr(dialog_cset.editor.active);
 XX1 = X1 + 5;
 YY1 = Y1 + 8;
 XX2 = XX1 + 31;
 YY2 = YY1;
 ewnd[4] = WindowOpen(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twSetColorAttr(dialog_cset.editor.active);
 twUseWin(wdlg);
 digit = 0;
 twGotoXY(3,2); twPutS("Convert numbers between bases [16, 10, 8, 2]");
 DCStaticPaint(wdlg,wbuff,digit);
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
   if(redraw) DCStaticPaint(wdlg,wbuff,digit);
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




