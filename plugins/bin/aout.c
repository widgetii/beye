/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/aout.c
 * @brief       This file contains implementation of a.out file format decoder.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <stdio.h>
#include <string.h>

#include "colorset.h"
#include "bmfile.h"
#include "biewutil.h"
#include "bin_util.h"
#include "biewhelp.h"
#include "bconsole.h"
#include "reg_form.h"
#include "plugins/disasm.h"
#define ARCH_SIZE 32
#include "plugins/bin/aout64.h"
#include "biewlib/kbd_code.h"
#include "biewlib/biewlib.h"

static const char * __NEAR__ __FASTCALL__ aout_encode_hdr(unsigned long info)
{
   switch(info & 0x0000FFFFUL)
   {
     case OMAGIC: return " a.out: Object file ";
     case NMAGIC: return " a.out: Pure executable ";
     case ZMAGIC: return " a.out: Demand-paged executable ";
     case BMAGIC: return " b.out: Object file ";
     case QMAGIC: return " a.out: 386BSD demand-paged executable ";
     default:     return " Unknow a.out or b.out format ";
   }
}

static unsigned long __FASTCALL__ ShowAOutHeader( void )
{
  struct external_exec aout;
  unsigned long fpos;
  unsigned keycode;
  TWindow *w;
  fpos = BMGetCurrFilePos();
  bmReadBufferEx(&aout,sizeof(struct external_exec),0,SEEKF_START);
  w = CrtDlgWndnls(aout_encode_hdr(*((unsigned long *)&aout.e_info)),54,7);
  twGotoXY(1,1);
  twPrintF("Length of text section      = %08lXH\n"
           "Length of data section      = %08lXH\n"
           "Length of bss area          = %08lXH\n"
           "Length of symbol table      = %08lXH\n"
           ,*((unsigned long *)&aout.e_text)
           ,*((unsigned long *)&aout.e_data)
           ,*((unsigned long *)&aout.e_bss)
           ,*((unsigned long *)&aout.e_syms));
  twSetColorAttr(dialog_cset.entry);
  twPrintF("Start address               = %08lXH"
           ,*((unsigned long *)&aout.e_entry));
  twClrEOL(); twPrintF("\n");
  twSetColorAttr(dialog_cset.main);
  twPrintF("Length of text relocation   = %08lXH\n"
           "Length of data relocation   = %08lXH"
           ,*((unsigned long *)&aout.e_trsize)
           ,*((unsigned long *)&aout.e_drsize));
  while(1)
  {
    keycode = GetEvent(drawEmptyPrompt,w);
    if(keycode == KE_ENTER) { fpos = *((unsigned long *)&aout.e_entry); break; }
    else
      if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
  }
  CloseWnd(w);
  return fpos;
}

static tBool __FASTCALL__ aout_check_fmt( void )
{
  tUIntFast16 id;
  id = bmReadWordEx(0,SEEKF_START);
  return !(N_BADMAG(id));
}

static void __FASTCALL__ aout_init_fmt( void ) {}
static void __FASTCALL__ aout_destroy_fmt( void ) {}

static int __FASTCALL__ aout_bitness(unsigned long off)
{
   UNUSED(off);
   return DAB_USE32;
}

static tBool __FASTCALL__ aout_AddrResolv(char *addr,unsigned long fpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  tBool bret = True;
  if(fpos < sizeof(struct external_exec))
  {
    strcpy(addr,"a.outh:");
    strcpy(&addr[7],Get2Digit(fpos));
  }
  else bret = False;
  return bret;
}

static unsigned long __FASTCALL__ aout_help( void )
{
  hlpDisplay(10000);
  return BMGetCurrFilePos();
}

static int __FASTCALL__ aout_platform( void ) { return DISASM_CPU_IX86; }

REGISTRY_BIN aoutTable =
{
  "a.out (Assembler and link Output)",
  { "AOutHl", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { aout_help, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  aout_check_fmt,
  aout_init_fmt,
  aout_destroy_fmt,
  ShowAOutHeader,
  NULL,
  NULL,
  aout_platform,
  aout_bitness,
  aout_AddrResolv,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
