/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/aout.c
 * @brief       This file contains implementation of a.out file format decoder.
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

#include "bswap.h"
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

static char is_msbf=0; /* is most significand byte first */
static char is_64bit=0;

#define AOUT_HALF(cval) FMT_WORD(cval,is_msbf)
#define AOUT_WORD(cval) FMT_DWORD(cval,is_msbf)
#define AOUT_QWORD(cval) FMT_QWORD(cval,is_msbf)

static const char * __NEAR__ __FASTCALL__ aout_encode_hdr(unsigned long info)
{
   switch(N_MAGIC(AOUT_WORD(&info)))
   {
     case OMAGIC: return " a.out-32: Object file ";
     case NMAGIC: return " a.out-32: Pure executable ";
     case ZMAGIC: return " a.out-32: Demand-paged executable ";
     case BMAGIC: return " b.out-32: Object file ";
     case QMAGIC: return " a.out-32: 386BSD demand-paged executable ";
     case CMAGIC: return " a.out-core";
     case OMAGIC64: return " a.out-64: Object file ";
     case NMAGIC64: return " a.out-64: Pure executable ";
     case ZMAGIC64: return " a.out-64: Demand-paged executable ";
     default:     return " Unknown a.out or b.out format ";
   }
}

static const char * __NEAR__ __FASTCALL__ aout_encode_machine(unsigned long info,unsigned* id)
{
   *id=DISASM_DATA;
   switch(N_MACHTYPE(AOUT_WORD(&info)))
   {
     case 0: *id=DISASM_CPU_SPARC; return "Old Sun-2";
     case 1: *id=DISASM_CPU_PPC; return "M-68010";
     case 2: *id=DISASM_CPU_PPC; return "M-68020";
     case 3: *id=DISASM_CPU_SPARC; return "Sparc";
     case 100: *id=DISASM_CPU_IX86; return "i386";
     case 151: *id=DISASM_CPU_MIPS; return "MIPS1(R3000)";
     case 152: *id=DISASM_CPU_MIPS; return "MIPS2(R6000)";
     default:  return "Unknown CPU";
   }
}

static __filesize_t __FASTCALL__ ShowAOutHeader( void )
{
  struct external_exec aout;
  __filesize_t fpos;
  unsigned keycode,dummy;
  TWindow *w;
  fpos = BMGetCurrFilePos();
  bmReadBufferEx(&aout,sizeof(struct external_exec),0,SEEKF_START);
  w = CrtDlgWndnls(aout_encode_hdr(*((unsigned long *)&aout.e_info)),54,7);
  twGotoXY(1,1);
  twPrintF("Flags & CPU                 = %02XH %s(%s)\n"
	   "Length of text section      = %08lXH\n"
           "Length of data section      = %08lXH\n"
           "Length of bss area          = %08lXH\n"
           "Length of symbol table      = %08lXH\n"
	   ,N_FLAGS(*((unsigned long *)&aout.e_info)),aout_encode_machine(*((unsigned long *)&aout.e_info),&dummy),is_msbf?"big-endian":"little-endian"
           ,AOUT_WORD((unsigned long *)&aout.e_text)
           ,AOUT_WORD((unsigned long *)&aout.e_data)
           ,AOUT_WORD((unsigned long *)&aout.e_bss)
           ,AOUT_WORD((unsigned long *)&aout.e_syms));
  twSetColorAttr(dialog_cset.entry);
  twPrintF("Start address               = %08lXH"
           ,AOUT_WORD((unsigned long *)&aout.e_entry));
  twClrEOL(); twPrintF("\n");
  twSetColorAttr(dialog_cset.main);
  twPrintF("Length of text relocation   = %08lXH\n"
           "Length of data relocation   = %08lXH"
           ,AOUT_WORD((unsigned long *)&aout.e_trsize)
           ,AOUT_WORD((unsigned long *)&aout.e_drsize));
  while(1)
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    if(keycode == KE_ENTER) { fpos = AOUT_WORD(*((unsigned long *)&aout.e_entry)); break; }
    else
      if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
  }
  CloseWnd(w);
  return fpos;
}

static tBool __FASTCALL__ __aout_check_fmt( tUInt32 id )
{
  int a32,a64;
  a32=!(N_BADMAG(id));
  a64=!(N_BADMAG64(id));
  if(a64) is_64bit=1;
  return a32 || a64 || N_MAGIC(id)==CMAGIC;
}

static tBool __FASTCALL__ aout_check_fmt( void )
{
  tUInt32 id;
  id = bmReadDWordEx(0,SEEKF_START);
  if(__aout_check_fmt(id)) return 1;
  id=be2me_32(id);
  if(__aout_check_fmt(id)) { is_msbf=1; return 1; }
  return 0;
}

static void __FASTCALL__ aout_init_fmt( void ) {}
static void __FASTCALL__ aout_destroy_fmt( void ) {}

static int __FASTCALL__ aout_bitness(__filesize_t off)
{
   UNUSED(off);
   return is_64bit?DAB_USE64:DAB_USE32;
}

static int __FASTCALL__ aout_endian(__filesize_t off)
{
   UNUSED(off);
   return is_msbf?DAE_BIG:DAE_LITTLE;
}

static tBool __FASTCALL__ aout_AddrResolv(char *addr,__filesize_t fpos)
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

static __filesize_t __FASTCALL__ aout_help( void )
{
  hlpDisplay(10000);
  return BMGetCurrFilePos();
}

static int __FASTCALL__ aout_platform( void ) {
 unsigned id;
 struct external_exec aout;
 bmReadBufferEx(&aout,sizeof(struct external_exec),0,SEEKF_START);
 aout_encode_machine(*((unsigned long *)&aout.e_info),&id);
 return id;
}

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
  aout_endian,
  aout_AddrResolv,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
