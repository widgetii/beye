/**
 * @namespace   biew_addons
 * @file        addons/sys/ascii.c
 * @brief       This file contains simple implementation ASCII table viewer.
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

#include "bconsole.h"
#include "reg_form.h"
#include "biewutil.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

static void ShowASCII( void )
{
  TWindow * hwnd = CrtDlgWnd(" ASCII table ",34,18);
  unsigned evt;
  int i,j;
  unsigned char str[35];
  twUseWin(hwnd);
  twFreezeWin(hwnd);
  strcpy((char *)str,"°³0 1 2 3 4 5 6 7 8 9 A B C D E F");
  __nls_OemToOsdep(str,34);
  twDirectWrite(1,1,str,34);
  str[0] = TWC_SH;
  str[1] = 0;
  __nls_OemToOsdep(str,1);
  for(i = 0;i < 34;i++)  twDirectWrite(i+1,2,str,1);
  str[1] = TWC_SV;
  str[2] = 0;
  __nls_OemToOsdep(str,2);
  for(i = 0;i < 16;i++) { str[0] = i < 0x0A ? i + '0' : i - 0x0A + 'A'; twDirectWrite(1,i+3,str,2); }
  str[0] = TWC_SV_SH;
  str[1] = 0;
  __nls_OemToOsdep(str,1);
  twDirectWrite(2,2,str,1);
  for(i = 0;i < 16;i++)
  {
    for(j = 0;j < 16;j++) { str[j*2] = i*16 + j; str[j*2 + 1] = ' '; }
    twDirectWrite(3,i+3,str,31);
  }
  twRefreshWin(hwnd);
  do
  {
    evt = GetEvent(drawEmptyPrompt,hwnd);
  }
  while(!(evt == KE_ESCAPE || evt == KE_F(10)));
  CloseWnd(hwnd);
}

REGISTRY_SYSINFO AsciiTable =
{
  "~ASCII table",
  ShowASCII,
  NULL,
  NULL
};


