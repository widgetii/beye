/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/dos_sys.c
 * @brief       This file contains implementation of DOS driver file format.
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
#include <stdio.h>
#include <stddef.h>

#include "bconsole.h"
#include "bmfile.h"
#include "biewhelp.h"
#include "colorset.h"
#include "reg_form.h"
#include "biewlib/kbd_code.h"
#include "biewlib/pmalloc.h"
#include "plugins/disasm.h"
#include "plugins/bin/dos_sys.h"

static DOSDRIVER drv;

static __filesize_t __FASTCALL__ ShowSysHeader( void )
{
 int keycode;
 TWindow *hwnd;
 tBool charun;
 __fileoff_t fpos;
 fpos = BMGetCurrFilePos();
 hwnd = CrtDlgWndnls(" DOS Device Driver Header ",57,13);
 charun = (drv.ddAttribute & 0x8000) == 0x8000;
 if(charun) twPrintF("Device Name               = %8s\n",drv.ddName);
 else       twPrintF("Device Supports             %u Block Units\n",(unsigned)drv.ddName[0]);
 twPrintF("Attributes :\n"
          "  [%c]  STDIN\n"
          "  [%c]  STDOUT\n"
          "  [%c]  STDNUL\n"
          "  [%c]  CLOCK\n"
          "  [%c]  Support Logical Device\n"
          "  [%c]  Support OPEN/CLOSE/RM\n"
          "  [%c]  Non IBM Block-Device\n"
          "  [%c]  Support IOCTL\n"
          "  Device is                %s\n"
         ,GetBool(drv.ddAttribute & 0x0001)
         ,GetBool(drv.ddAttribute & 0x0002)
         ,GetBool(drv.ddAttribute & 0x0004)
         ,GetBool(drv.ddAttribute & 0x0008)
         ,GetBool(drv.ddAttribute & 0x0040)
         ,GetBool(drv.ddAttribute & 0x0800)
         ,GetBool(drv.ddAttribute & 0x2000)
         ,GetBool(drv.ddAttribute & 0x4000)
         ,(drv.ddAttribute & 0x8000 ? "CHARACTERS" : "BLOCKS"));
 twSetColorAttr(dialog_cset.entry);
 twPrintF(">STRATEGY routine offset  = %04hXH bytes [Enter]",drv.ddStrategyOff);
 twClrEOL(); twPrintF("\n");
 twSetColorAttr(dialog_cset.altentry);
 twPrintF(">INTERRUPT routine offset = %04hXH bytes [Ctrl+Enter | F5]",drv.ddInterruptOff);
 twClrEOL();
 while(1)
 {
   keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
   if(keycode == KE_ENTER) { fpos = drv.ddStrategyOff; break; }
   else
     if(keycode == KE_CTL_ENTER || keycode == KE_F(5)) { fpos = drv.ddInterruptOff; break; }
     else
       if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
 }
 CloseWnd(hwnd);
 return fpos;
}


static tBool  __FASTCALL__ dossys_check_fmt( void )
{
  unsigned char id[4];
  tBool ret = False;
  bmReadBufferEx(id,sizeof(id),0,BM_SEEK_SET);
  if(id[0] == 0xFF && id[1] == 0xFF && id[2] == 0xFF && id[3] == 0xFF)
  {
     bmReadBufferEx((void  *)&drv,sizeof(DOSDRIVER),4,BM_SEEK_SET);
     ret = True;
  }
  return ret;
}

static void __FASTCALL__ dossys_init_fmt( void ) {}
static void __FASTCALL__ dossys_destroy_fmt(void) {}
static int  __FASTCALL__ dossys_platform( void) { return DISASM_CPU_IX86; }
static int  __FASTCALL__ dossys_endian(__filesize_t off) { return DAE_LITTLE; }

static tBool __FASTCALL__ dossys_AddressResolv(char *addr,__filesize_t cfpos)
{
  tBool bret = True;
  if(cfpos < sizeof(DOSDRIVER)+4) sprintf(addr,"SYSH:%s",Get4Digit(cfpos));
  else bret = False;
  return bret;
}

static __filesize_t __FASTCALL__ SYSHelp( void )
{
  hlpDisplay(10014);
  return BMGetCurrFilePos();
}


REGISTRY_BIN dossysTable =
{
  "DOS driver",
  { "SYSHlp", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { SYSHelp, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  dossys_check_fmt,
  dossys_init_fmt,
  dossys_destroy_fmt,
  ShowSysHeader,
  NULL,
  NULL,
  dossys_platform,
  NULL,
  dossys_endian,
  dossys_AddressResolv,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
