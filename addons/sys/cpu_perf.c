/**
 * @namespace   biew_addons
 * @file        addons/sys/cpu_perf.c
 * @brief       This file contains cpu performance utility and tools.
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
#include "bconsole.h"
#include "biewutil.h"
#include "reg_form.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"
#include "biewlib/pmalloc.h"

static TWindow *pwnd;

static void paint_prcnt(int n_prcnt)
{
  if(n_prcnt > 100) n_prcnt = 100;
  ShowPercentInWnd(pwnd,n_prcnt);
}

static void ShowCPUInfo( void )
{
   TWindow *win;
   unsigned evt;
   char *cpu_info;
   cpu_info = PMalloc(2048);
   if(!cpu_info) { MemOutBox("Show CPU information"); return; }
   win = CrtDlgWndnls(" CPU information ",78,22);
   pwnd = PercentWnd("Analyze:","");
   __FillCPUInfo(cpu_info,2048,paint_prcnt);
   CloseWnd(pwnd);
   twGotoXY(1,1);
   twPrintF("%s", cpu_info);
   do
   {
     evt = GetEvent(drawEmptyPrompt,NULL,win);
   }
   while(!(evt == KE_ESCAPE || evt == KE_F(10)));
   CloseWnd(win);
   PFREE(cpu_info);
}

REGISTRY_SYSINFO CPUPerformance =
{
  "CPU ~performance",
  ShowCPUInfo,
  NULL,
  NULL
};

