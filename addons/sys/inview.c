/**
 * @namespace   biew_addons
 * @file        addons/sys/kbdview.c
 * @brief       This file contains simple implementation console information.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       2003
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

static void InputViewLoop( void )
{
  TWindow * hwnd = CrtDlgWndnls(" Input viewer ",78,2);
  int rval, do_exit;
  char head[80], text[80];
  drawEmptyPrompt();
  twUseWin(hwnd);
  twFreezeWin(hwnd);
  twSetFooterAttr(hwnd," [Escape] - quit ",TW_TMODE_RIGHT,dialog_cset.selfooter);
  twRefreshWin(hwnd);
  do_exit=0;
  do
  {
    rval = __inputRawInfo(head,text);
    if(rval==-1)
    {
	ErrMessageBox("Not implemented yet!",NULL);
	break;
    }
    twGotoXY(1,1);
    twPutS(head);
    twClrEOL();
    twGotoXY(1,2);
    twPutS(text);
    twClrEOL();
    if(!rval) do_exit++;
  }
  while(do_exit<2);
  CloseWnd(hwnd);
}

REGISTRY_SYSINFO InputViewer =
{
  "~Input viewer",
  InputViewLoop,
  NULL,
  NULL
};

