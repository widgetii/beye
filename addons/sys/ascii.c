/**
 * @namespace   biew_addons
 * @file        addons/sys/ascii.c
 * @brief       This file contains simple implementation ASCII table viewer.
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
 * @author      Alexander Krisak and Andrew Golovnia
 * @date        23.07.2003
 * @note        Russian locales support: KOI-8, CP866, CP1251, ISO8859-5.
 *              Tested at ASPLinux 7.3 and ASPLinux 9
**/
#include <string.h>

#include "bconsole.h"
#include "reg_form.h"
#include "biewutil.h"
#include "colorset.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

static TWindow * __NEAR__ __FASTCALL__ CreatePanelNF(tAbsCoord x1,tAbsCoord y1,tAbsCoord x2,tAbsCoord y2)
{
 TWindow *win;
 unsigned flags = 0;
 win = WindowOpen(x1,y1,x2,y2,flags);
 twSetColorAttr(dialog_cset.main);
 twClearWin();
 twShowWin(win);
 return win;
}

static void ShowASCII( void )
{
  TWindow * hwnd = CrtDlgWndnls(" ASCII table ",34,18);
  TWindow * hpnl = CreatePanelNF(hwnd->X1+4,hwnd->Y1+4,hwnd->X2-1,hwnd->Y2-1);
  unsigned evt;
  int i,j;
  unsigned char str[35];
  twUseWin(hwnd);
  twFreezeWin(hwnd);
  strcpy((char *)str,"°³0 1 2 3 4 5 6 7 8 9 A B C D E F");
  twDirectWrite(1,1,str,34);
  str[0] = TWC_SH;
  str[1] = 0;
  for(i = 0;i < 34;i++)  twDirectWrite(i+1,2,str,1);
  str[1] = TWC_SV;
  str[2] = 0;
  for(i = 0;i < 16;i++) { str[0] = i < 0x0A ? i + '0' : i - 0x0A + 'A'; twDirectWrite(1,i+3,str,2); }
  str[0] = TWC_SV_SH;
  str[1] = 0;
  twDirectWrite(2,2,str,1);
  twUseWin(hpnl);
  twFreezeWin(hpnl);
  for(i = 0;i < 16;i++)
  {
    for(j = 0;j < 16;j++) { str[j*2] = i*16 + j; str[j*2 + 1] = ' '; }
    twDirectWrite(1,i+1,str,31);
  }
  twRefreshWin(hpnl);
  twRefreshWin(hwnd);
  do
  {
    evt = GetEvent(drawEmptyPrompt,NULL,hwnd);
  }
  while(!(evt == KE_ESCAPE || evt == KE_F(10)));
  CloseWnd(hpnl);
  CloseWnd(hwnd);
}

REGISTRY_SYSINFO AsciiTable =
{
  "~ASCII table",
  ShowASCII,
  NULL,
  NULL
};


