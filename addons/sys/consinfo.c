/**
 * @namespace   biew_addons
 * @file        addons/sys/consinfo.c
 * @brief       This file contains simple implementation console information.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       2000
 * @note        Development, fixes and improvements
**/
#include <string.h>
#include <stddef.h>

#include "colorset.h"
#include "bconsole.h"
#include "biewutil.h"
#include "reg_form.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

extern char biew_scheme_name[];

static void ShowConsInfo( void )
{
  TWindow * hwnd = CrtDlgWndnls(" Console information ",63,21);
  unsigned evt;
  int i,j,len;
  unsigned char str[80];
  twUseWin(hwnd);
  twFreezeWin(hwnd);
  strcpy((char *)str,"°³ 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F ³Name");
  len = strlen((char *)str);
  twDirectWrite(1,1,str,len);
  str[0] = TWC_SH;
  for(i = 0;i < 63;i++)  twDirectWrite(i+1,2,str,1);
  str[1] = TWC_SV;
  for(i = 0;i < 16;i++) { str[0] = i < 0x0A ? i + '0' : i - 0x0A + 'A'; twDirectWrite(1,i+3,str,2); }
  str[0] = TWC_SH_SV;
  twDirectWrite(2,2,str,1);
  for(i = 0;i < 16;i++)
  {
    for(j = 0;j < 16;j++)
    {
       twSetColor(i,j);
       str[0] = ' '; str[1] = '*'; str[2] = ' ';
       twDirectWrite(j*3+3,i+3,str,3);
    }
  }
  twSetColorAttr(dialog_cset.main);
  str[0] = TWC_SH;
  for(i = 0;i < 63;i++) twDirectWrite(i+1,19,str,1);
  str[0] = TWC_SH_Su;
  twDirectWrite(2,19,str,1);
  str[0] = TWC_SV;
  for(i = 0;i < 16;i++) twDirectWrite(51,i+3,str,1);
  str[0] = TWC_SH_SV;
  twDirectWrite(51,2,str,1);
  str[0] = TWC_SH_Su;
  twDirectWrite(51,19,str,1);
  for(i = 0;i < 16;i++) { twGotoXY(52,i+3); twPutS(named_color_def[i].name); }
  twGotoXY(1,20);
  twPrintF("Console: %ux%ux%u\n"
           "Skin:    %s"
           ,tvioWidth
           ,tvioHeight
           ,tvioNumColors
           ,biew_scheme_name);
  twRefreshWin(hwnd);
  do
  {
    evt = GetEvent(drawEmptyPrompt,NULL,hwnd);
  }
  while(!(evt == KE_ESCAPE || evt == KE_F(10)));
  CloseWnd(hwnd);
}

REGISTRY_SYSINFO ConsoleInfo =
{
  "~Console information",
  ShowConsInfo,
  NULL,
  NULL
};



