/**
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/avr/avr.c
 * @brief       This file contains implementation of Atmel AVR disassembler (main module).
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Stephan Linz
 * @since       2003
 * @note        Development, fixes and improvements
 *
 * @note        ported from GNU binutils -- most stuff made by Denis Chertykov
**/
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "biewhelp.h"
#include "bmfile.h"
#include "plugins/disasm.h"
#include "biewutil.h"
#include "reg_form.h"
#include "bconsole.h"
#include "codeguid.h"
#include "biewlib/file_ini.h"
#include "biewlib/kbd_code.h"
#include "biewlib/pmalloc.h"
#include "biewlib/biewlib.h"

#define _(STR)     STR
#define _MAX(A,B)  ( (A) > (B) ? (A) : (B) )

struct avr_opcodes_s
{
  const char *name;
  const char *constraints;
  const char *opcode;
  int insn_size;		/* in words */
  int isa;
  unsigned int bin_opcode;
};

#define AVR_INSN(NAME, CONSTR, OPCODE, SIZE, ISA, BIN) \
{#NAME, CONSTR, OPCODE, SIZE, ISA, BIN},

const struct avr_opcodes_s avr_opcodes[] =
{
  #include "plugins/disasm/avr/avr.h"
  {NULL, NULL, NULL, 0, 0, 0}
};

static unsigned int *avr_bin_masks;
static unsigned int *maskptr;

static void avr_assert( int expression )
{
  if (!expression)
  {
    MemOutBox("AVR dissasembler fault");
    exit(EXIT_FAILURE);
  }
}

static unsigned int avr_getl16( MBuffer addr )
{
  return (addr[1] << 8) | addr[0];
}

static int avr_fillout( char *str, int to )
{
  int i, j;
  for (i = to, j = 0; i --> 0; j++) sprintf(str + j, " ");
  return j;
}

static int avr_operand( unsigned int insn,
			unsigned int insn2,
			unsigned int pc,
			int constraint,
			char *buf,
			char *comment,
			int regs )
{
  int ok = 1;

  switch (constraint)
    {
      /* Any register operand.  */
    case 'r':
      if (regs)
	insn = (insn & 0xf) | ((insn & 0x0200) >> 5); /* source register */
      else
	insn = (insn & 0x01f0) >> 4; /* destination register */
      
      sprintf (buf, "r%d", insn);
      break;

    case 'd':
      if (regs)
	sprintf (buf, "r%d", 16 + (insn & 0xf));
      else
	sprintf (buf, "r%d", 16 + ((insn & 0xf0) >> 4));
      break;
      
    case 'w':
      sprintf (buf, "r%d", 24 + ((insn & 0x30) >> 3));
      break;
      
    case 'a':
      if (regs)
	sprintf (buf, "r%d", 16 + (insn & 7));
      else
	sprintf (buf, "r%d", 16 + ((insn >> 4) & 7));
      break;

    case 'v':
      if (regs)
	sprintf (buf, "r%d", (insn & 0xf) * 2);
      else
	sprintf (buf, "r%d", ((insn & 0xf0) >> 3));
      break;

    case 'e':
      {
	char *xyz;

	switch (insn & 0x100f)
	  {
	    case 0x0000: xyz = "Z";  break;
	    case 0x1001: xyz = "Z+"; break;
	    case 0x1002: xyz = "-Z"; break;
	    case 0x0008: xyz = "Y";  break;
	    case 0x1009: xyz = "Y+"; break;
	    case 0x100a: xyz = "-Y"; break;
	    case 0x100c: xyz = "X";  break;
	    case 0x100d: xyz = "X+"; break;
	    case 0x100e: xyz = "-X"; break;
	    default: xyz = "??"; ok = 0;
	  }
	sprintf (buf, xyz);

	if (AVR_UNDEF_P (insn))
	  sprintf (comment, _("undefined"));
      }
      break;

    case 'z':
      *buf++ = 'Z';
      if (insn & 0x1)
	*buf++ = '+';
      *buf = '\0';
      if (AVR_UNDEF_P (insn))
	sprintf (comment, _("undefined"));
      break;

    case 'b':
      {
	unsigned int x;
	
	x = (insn & 7);
	x |= (insn >> 7) & (3 << 3);
	x |= (insn >> 8) & (1 << 5);
	
	if (insn & 0x8)
	  *buf++ = 'Y';
	else
	  *buf++ = 'Z';
	sprintf (buf, "+%d", x);
	sprintf (comment, "0x%02x", x);
      }
      break;
      
    case 'h':
      sprintf (buf, "0x%x",
	       ((((insn & 1) | ((insn & 0x1f0) >> 3)) << 16) | insn2) * 2);
      break;
      
    case 'L':
      {
	int rel_addr = (((insn & 0xfff) ^ 0x800) - 0x800) * 2;
	sprintf (buf, ".%+-8d", rel_addr);
	sprintf (comment, "0x%x", pc + 2 + rel_addr);
      }
      break;

    case 'l':
      {
	int rel_addr = ((((insn >> 3) & 0x7f) ^ 0x40) - 0x40) * 2;
	sprintf (buf, ".%+-8d", rel_addr);
	sprintf (comment, "0x%x", pc + 2 + rel_addr);
      }
      break;

    case 'i':
      sprintf (buf, "0x%04X", insn2);
      break;
      
    case 'M':
      sprintf (buf, "0x%02X", ((insn & 0xf00) >> 4) | (insn & 0xf));
      sprintf (comment, "%d", ((insn & 0xf00) >> 4) | (insn & 0xf));
      break;

    case 'n':
      sprintf (buf, "??");
      //fprintf (stderr, _("Internal disassembler error"));
      ok = 0;
      break;
      
    case 'K':
      {
	unsigned int x;

	x = (insn & 0xf) | ((insn >> 2) & 0x30);
	sprintf (buf, "0x%02x", x);
	sprintf (comment, "%d", x);
      }
      break;
      
    case 's':
      sprintf (buf, "%d", insn & 7);
      break;
      
    case 'S':
      sprintf (buf, "%d", (insn >> 4) & 7);
      break;
      
    case 'P':
      {
	unsigned int x;
	x = (insn & 0xf);
	x |= (insn >> 5) & 0x30;
	sprintf (buf, "0x%02x", x);
	sprintf (comment, "%d", x);
      }
      break;

    case 'p':
      {
	unsigned int x;
	
	x = (insn >> 3) & 0x1f;
	sprintf (buf, "0x%02x", x);
	sprintf (comment, "%d", x);
      }
      break;
      
    case '?':
      *buf = '\0';
      break;
      
    default:
      sprintf (buf, "??");
      //fprintf (stderr, _("unknown constraint `%c'"), constraint);
      ok = 0;
    }

    return ok;
}






static char *outstr;

static DisasmRet __FASTCALL__ AVRDisassembler( __filesize_t ulShift,
					       MBuffer buffer,
					       unsigned flags )
{
  unsigned int insn, insn2, clone, type;
  const struct avr_opcodes_s *opcode;
  int cmd_len = 2;
  int ok = 0;
  int out = 0;
  char op1[20], op2[20], comment1[40], comment2[40];
  DisasmRet Ret;

  /* extract 1st instruction opcode */
  insn = avr_getl16(buffer);

  for ( opcode = avr_opcodes, maskptr = avr_bin_masks;
	opcode->name;
	opcode++, maskptr++ )
  {
    if ( (insn & *maskptr) == opcode->bin_opcode ) break;
  }

  /* Special case: disassemble `ldd r,b+0' as `ld r,b', and
   * `std b+0,r' as `st b,r' (next entry in the table).  */
  if (AVR_DISP0_P (insn))
    opcode++;

  op1[0] = 0;
  op2[0] = 0;
  comment1[0] = 0;
  comment2[0] = 0;

  /* extract 2nd instruction opcode */
  insn2 = 0;
  if (opcode->name)
  {
    ok = 1;

    if (opcode->insn_size > 1)
    {
      insn2 = avr_getl16 (buffer + 2);
      cmd_len = 4;
    }
  }

  /* determine opcode related cpu clone (avr[1-5] or Prescott) */
  if ( (opcode->isa & AVR_ISA_TINY1) == opcode->isa )
    clone = 0;
  else if ( (opcode->isa & AVR_ISA_2xxx) == opcode->isa )
    clone = 1;
  else if ( (opcode->isa & AVR_ISA_M103) == opcode->isa )
    clone = 2;
  else if ( (opcode->isa & AVR_ISA_M8) == opcode->isa )
    clone = 3;
  else if ( (opcode->isa & AVR_ISA_ALL) == opcode->isa )
    clone = 4;
  else
    clone = 10;

#if 0
  /* determine opcode related instruction type class (ret, leave, jmp) */
  if ( AVR_RET_CLASS(insn) )
    type = __INSNT_RET;
  else if ( AVR_LEAVE_CLASS(insn) )
    type = __INSNT_LEAVE;
  else if ( AVR_JMPVVT_CLASS(insn) )
    type = __INSNT_JMPVVT;
  else if ( AVR_JUMPPIC_CLASS(insn) )
    type = __INSNT_JMPPIC;
  else
#endif
    type = __INSNT_ORDINAL;

  if ( !( (flags & __DISF_SIZEONLY) == __DISF_SIZEONLY ) )
  {
    if (ok)
    {
      const char *op = opcode->constraints;

      if (*op && *op != '?')
      {
	int regs = REGISTER_P (*op);

	ok = avr_operand (insn, insn2, (unsigned int)ulShift, *op, op1, comment1, 0);

	if (ok && *(++op) == ',')
	  ok = avr_operand (insn, insn2, (unsigned int)ulShift, *(++op), op2,
			*comment1 ? comment2 : comment1, regs);
      }
    }

    if (!ok)
    {
      /* Unknown opcode, or invalid combination of operands.  */
      sprintf (op1, "0x%04x", insn);
      op2[0] = 0;
      sprintf (comment1, "????");
      comment2[0] = 0;
    }

    out += sprintf(outstr + out, "%s", ok ? opcode->name : ".word");
    out += avr_fillout(outstr + out, 10 - out);

    if (*op1) out += sprintf(outstr + out, "%s", op1);
    if (*op2) out += sprintf(outstr + out, ", %s", op2);
    out += avr_fillout(outstr + out, 30 - out);

    if (*comment1) out += sprintf(outstr + out, "; %s", comment1);
    if (*comment2) out += sprintf(outstr + out, " %s", comment2);

    memset(&Ret, 0, sizeof(Ret));
    Ret.str = outstr;
    Ret.codelen = cmd_len;

    Ret.pro_clone = clone;
  }
  else
    if(flags & __DISF_GETTYPE)
    {
      /* __DISF_GETTYPE must be reentrance. Must do not modify any internal
       * disassembler variables and flags and e.t.c.
       * It is calling from disassembler recursively. */
      Ret.pro_clone = type;
    }
    else
    {
      Ret.codelen = cmd_len;
    }

  return Ret;
}



/* !!!!!!!!!!! have to be 10 string elements !!!!!!!!!!! */
static const char * AVRCoreNames[] =
{
  " avr1 ",
  " avr2 ",
  " avr3 ",
  " avr4 ",
  " avr5 ",
  "      ",
  "      ",
  "      ",
  "      ",
  "          "
};

static tBool __FASTCALL__ AVRAsmRef( void )
{
  hlpDisplay(20020);
  return False;
}

static void __FASTCALL__ AVRHelpAsm( void )
{
  char *msgAsmText,*title;
  char **strs;
  unsigned size,i,evt;
  unsigned long nstrs;
  TWindow * hwnd;

  if(!hlpOpen(True)) return;

  size = (unsigned)hlpGetItemSize(20021);
  if (!size) goto avrhlp_bye;

  msgAsmText = PMalloc(size+1);
  if (!msgAsmText)
  {
mem_off:
    MemOutBox(" Help display ");
    goto avrhlp_bye;
  }

  if (!hlpLoadItem(20021, msgAsmText))
  {
    PFree(msgAsmText);
    goto avrhlp_bye;
  }
  msgAsmText[size] = '\0';

  if (!(strs = hlpPointStrings(msgAsmText, size, &nstrs))) goto mem_off;
  title = msgAsmText;

  hwnd = CrtHlpWndnls(title, 72, 21);
  twUseWin(hwnd);
  for (i = 0; i < nstrs; i++)
  {
    unsigned rlen;
    tvioBuff it;
    t_vchar chars[__TVIO_MAXSCREENWIDTH];
    t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
    ColorAttr attrs[__TVIO_MAXSCREENWIDTH];

    it.chars = chars;
    it.oem_pg = oem_pg;
    it.attrs = attrs;

    rlen = strlen(strs[i]);
    rlen = hlpFillBuffer(&it, __TVIO_MAXSCREENWIDTH, strs[i], rlen, 0, NULL, 0);

    twWriteBuffer(hwnd, 2, i + 2, &it, rlen);
  }

  PFree(msgAsmText);
  twGotoXY(5, 3);
  for (i = 0; i < 10; i++)
  {
    twSetColorAttr(disasm_cset.cpu_cset[0].clone[i]);
    twPutS(AVRCoreNames[i]);
  }

  do
  {
    evt = GetEvent(drawEmptyPrompt,NULL,hwnd);
  }
  while (!(evt == KE_ESCAPE || evt == KE_F(10)));

  CloseWnd(hwnd);
  
avrhlp_bye:
  hlpClose();
}

static int __FASTCALL__ AVRMaxInsnLen( void )
{
  const struct avr_opcodes_s *opcode;
  int size = 0;

  for ( opcode = avr_opcodes; opcode->name; opcode++ )
  {
    size = _MAX( opcode->insn_size, size);
  }

  return (size << 2);
}

static ColorAttr __FASTCALL__ AVRGetOpcodeColor( unsigned long clone )
{
  UNUSED(clone);
  return disasm_cset.opcodes0;
}

static ColorAttr __FASTCALL__ AVRGetAsmColor( unsigned long clone )
{
  return disasm_cset.cpu_cset[0].clone[clone & 0xff];
}

static int __FASTCALL__ AVRGetBitness( void )
{
  return DAB_USE16;
}

static char __FASTCALL__ AVRGetClone( unsigned long clone )
{
  UNUSED(clone);
  return ' ';
}

static void __FASTCALL__ AVRInit( void )
{
  const struct avr_opcodes_s *opcode;
  unsigned int nopcodes;

  nopcodes = sizeof (avr_opcodes) / sizeof (struct avr_opcodes_s);
  avr_bin_masks = (unsigned int *)PMalloc(nopcodes * sizeof (unsigned int));
  outstr = PMalloc(1000);

  if (!outstr || !avr_bin_masks)
  {
    MemOutBox("AVR dissasembler initialization");
    exit(EXIT_FAILURE);
  }


  for (opcode = avr_opcodes, maskptr = avr_bin_masks;
	   opcode->name;
	   opcode++, maskptr++)
  {
    const char * s;
    unsigned int bin = 0;
    unsigned int mask = 0;
	
    for (s = opcode->opcode; *s; ++s)
    {
      bin <<= 1;
      mask <<= 1;
      bin |= (*s == '1');
      mask |= (*s == '1' || *s == '0');
    }
    avr_assert(s - opcode->opcode == 16);
    avr_assert(opcode->bin_opcode == bin);
    *maskptr = mask;
  }
}

static void __FASTCALL__ AVRTerm( void )
{
   PFREE(avr_bin_masks);
   PFREE(outstr);
}

REGISTRY_DISASM AVR_Disasm =
{
  DISASM_CPU_AVR,
  "Atmel ~AVR",                          /* disassembler name */
  { "AVRHlp", NULL, NULL, NULL, NULL },  /* prompt for Ctrl-(F1,F3,F4,F5,F10) */
  { AVRAsmRef, NULL, NULL, NULL, NULL }, /* action for Ctrl-(F1,F3,F4,F5,F10) */
  AVRDisassembler,                       /* main disassembler function */
  NULL,                                  /* main assembler function (???) */
  AVRHelpAsm,                            /* display short help (Shift-F1) */
  AVRMaxInsnLen,                         /* max opcode length of 1 insn */
  AVRGetAsmColor,                        /* color of insn */
  AVRGetOpcodeColor,                     /* color of opcode */
  AVRGetAsmColor,                        /* color of insn */
  AVRGetOpcodeColor,                     /* color of opcode */
  AVRGetBitness,                         /* currently ised bitness */
  AVRGetClone,                           /* short clone name of insn */
  AVRInit,                               /* plugin initializer */
  AVRTerm,                               /* plugin terminator */
  NULL,                                  /* plugin setting reader (from .ini) */
  NULL                                   /* plugin setting writer (to .ini) */
};
