/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/opharlap.c
 * @brief       This file contains implementation of Old PharLap file format decoder.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <stdio.h>
#include <string.h>

#include "colorset.h"
#include "bin_util.h"
#include "bmfile.h"
#include "biewutil.h"
#include "biewhelp.h"
#include "bconsole.h"
#include "reg_form.h"
#include "plugins/bin/pharlap.h"
#include "plugins/disasm.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

static oldPharLap oph;

static tBool __FASTCALL__ IsOldPharLap( void )
{
   char sign[2];
   bmReadBufferEx(sign,2,0,SEEKF_START);
   if(sign[0] == 'M' && sign[1] == 'P') return True;
   return False;
}

static __filesize_t __FASTCALL__ ShowOPharLapHeader( void )
{
  __filesize_t fpos,entrypoint;
  TWindow * w;
  unsigned keycode;
  fpos = BMGetCurrFilePos();
  entrypoint = oph.plHeadSize*16 + oph.plEIP;
  w = CrtDlgWndnls(" Old PharLap executable ",54,11);
  twGotoXY(1,1);
  twPrintF("Image size reminder on last page   = %04XH\n"
           "Image size in pages                = %04XH\n"
           "Number of relocation items         = %04XH\n"
           "Header size in paragraphs          = %04XH\n"
           "Min. number of extra 4K pages      = %04XH\n"
           "Max. number of extra 4K pages      = %04XH\n"
           "Initial ESP                        = %08lXH\n"
           "File checksum                      = %04XH\n"
           "Initial EIP                        = %08lXH\n"
           "Offset of first relocation item    = %04XH\n"
           "Number of overlays                 = %04XH\n"
           ,oph.plSizeRemaind
           ,oph.plImageSize
           ,oph.plNRelocs
           ,oph.plHeadSize
           ,oph.plMinExtraPages
           ,oph.plMaxExtraPages
           ,oph.plESP
           ,oph.plCheckSum
           ,oph.plEIP
           ,oph.plFirstReloc
           ,oph.plNOverlay);
  twSetColorAttr(dialog_cset.entry);
  twPrintF("Entry Point                        = %08lXH",entrypoint);
  twClrEOL();
  while(1)
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    if(keycode == KE_ENTER) { fpos = entrypoint; break; }
    else
      if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
  }
  CloseWnd(w);
  return fpos;
}

static void __FASTCALL__ OPharLapInit( void )
{
  bmReadBufferEx(&oph,sizeof(oph),0,SEEKF_START);
}

static void __FASTCALL__ OPharLapDestroy( void )
{
}

static tBool __FASTCALL__ OldPharLapAddrResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  tBool bret = True;
  if(cfpos < sizeof(oldPharLap))
  {
    strcpy(addr,"oplhdr:");
    strcpy(&addr[7],Get2Digit(cfpos));
  }
  else bret = False;
  return bret;
}

static __filesize_t __FASTCALL__ HelpOPharLap( void )
{
  hlpDisplay(10008);
  return BMGetCurrFilePos();
}

static int __FASTCALL__ OldPharLapPlatform( void ) { return DISASM_CPU_IX86; }

REGISTRY_BIN OldPharLapTable =
{
  "Pharlap",
  { "PLHelp", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { HelpOPharLap, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  IsOldPharLap,
  OPharLapInit,
  OPharLapDestroy,
  ShowOPharLapHeader,
  NULL,
  NULL,
  OldPharLapPlatform,
  NULL,
  NULL,
  OldPharLapAddrResolv,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
