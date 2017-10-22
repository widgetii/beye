/**
 * @namespace   biew_addons
 * @file        addons/sys/cpu_perf.c
 * @brief       This file contains cpu performance utility and tools.
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
 * @author      Andrew Golovnya <andrew_golovnia at users dot sourceforge dot net>
 * @note        CPUInfo display window changed to scrollable window
**/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "bconsole.h"
#include "beyeutil.h"
#include "reg_form.h"
#include "libbeye/beyelib.h"
#include "libbeye/kbd_code.h"
#include "libbeye/pmalloc.h"

static TWindow *pwnd;

static void paint_prcnt(int n_prcnt)
{
  if(n_prcnt > 100) n_prcnt = 100;
  ShowPercentInWnd(pwnd,n_prcnt);
}

char ** __FASTCALL__ cpuPointStrings(char __HUGE__ *data,unsigned long data_size,unsigned long *nstr)
{
  char **str_ptr,**new_ptr;
  unsigned long i;
  char ch,ch1;
  *nstr = 0;
  if((str_ptr = PMalloc(sizeof(char *) * 2)) != NULL)
  {
     str_ptr[(*nstr)++] = &data[0];
     for(i = 0;i < data_size;i++)
     {
       ch = data[i];
       if(ch == '\n' || ch == '\r')
       {
         data[i] = 0;
         if(!(new_ptr = PRealloc(str_ptr,((unsigned)(*nstr)+1)*sizeof(char *)))) goto mem_off;
         str_ptr = new_ptr;
         ch1 = data[i+1];
         if((ch1 == '\n' || ch1 == '\r') && ch != ch1) ++i;
         str_ptr[(*nstr)++] = &data[i+1];
       }
       if(*nstr > UINT_MAX-2) { mem_off: PFree(str_ptr); return NULL; }
     }
  }
  return str_ptr;
}

static void ShowCPUInfo( void )
{
   char *cpu_info;
   char **str_ptr;
   unsigned long data_size;
   unsigned long nstr;
   cpu_info = PMalloc(4096);
   if(!cpu_info) { mem_out: MemOutBox("Show CPU information"); return; }
   pwnd = PercentWnd("Analyze:","");
   __FillCPUInfo(cpu_info,4096,paint_prcnt);
   CloseWnd(pwnd);
   data_size = strchr(cpu_info, 0) - cpu_info;
   if(!(str_ptr = cpuPointStrings(cpu_info,data_size,&nstr)))
      { PFREE(cpu_info); goto mem_out; }
   DisplayBox((const char**)str_ptr,(unsigned)nstr - 1," CPU information ");
   PFREE(str_ptr);
   PFREE(cpu_info);
}

REGISTRY_SYSINFO CPUPerformance =
{
  "CPU ~performance",
  ShowCPUInfo,
  NULL,
  NULL
};

