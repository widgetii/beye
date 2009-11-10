/*
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/ix86/ix86.c
 * @brief       This file contains implementation of Intel x86 disassembler (main module).
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
 * @author      Mauro Giachero
 * @date        11.2007
 * @note        Implemented x86 inline assembler as a NASM/YASM wrapper
**/
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "biewhelp.h"
#include "bmfile.h"
#include "plugins/disasm.h"
#include "plugins/disasm/ix86/ix86.h"
#include "biewutil.h"
#include "reg_form.h"
#include "bconsole.h"
#include "codeguid.h"
#include "biewlib/file_ini.h"
#include "biewlib/kbd_code.h"
#include "biewlib/pmalloc.h"
#include "biewlib/biewlib.h"

#define MAX_IX86_INSN_LEN 15

const char ix86CloneSNames[3] = { 'i', 'a', 'c' };
const char * ix86_sizes[] = { "", "(b)", "(w)", "(d)", "(p)", "(q)", "(t)" };
const char * ix86_A16[] = { "bx+si", "bx+di", "bp+si", "bp+di", "si" , "di" , "bp" , "bx" };

const char * i8086_ByteRegs[] = { "al", "cl", "dl", "bl", "ah",  "ch",  "dh",  "bh" };
const char * k64_ByteRegs[]   = { "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b" };
const char * ix86_MMXRegs[]  = { "mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm7" };
const char * ix86_SegRegs[]  = { "es", "cs", "ss", "ds", "fs", "gs", "?s", "?s" };

const char * k64_WordRegs[] =  { "ax",  "cx",  "dx",  "bx",  "sp",  "bp",  "si",  "di",  "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w" };
const char * k64_DWordRegs[] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };
const char * k64_QWordRegs[] = { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8",  "r9",  "r10",  "r11",  "r12",  "r13",  "r14",  "r15" };
const char * k64_XMMXRegs[] = { "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7","xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15" };
const char * k64_YMMXRegs[] = { "ymm0","ymm1","ymm2","ymm3","ymm4","ymm5","ymm6","ymm7","ymm8","ymm9","ymm10","ymm11","ymm12","ymm13","ymm14","ymm15" };
const char * k64_CrxRegs[]  = { "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7", "cr8", "cr9", "cr10", "cr11", "cr12", "cr13", "cr14", "cr15" };
const char * k64_DrxRegs[]  = { "dr0", "dr1", "dr2", "dr3", "dr4", "dr5", "dr6", "dr7", "dr8", "dr9", "dr10", "dr11", "dr12", "dr13", "dr14", "dr15" };
const char * k64_TrxRegs[]  = { "tr0", "tr1", "tr2", "tr3", "tr4", "tr5", "tr6", "tr7", "tr8", "tr9", "tr10", "tr11", "tr12", "tr13", "tr14", "tr15" };
const char * k64_XrxRegs[]  = { "?r0", "?r1", "?r2", "?r3", "?r4", "?r5", "?r6", "?r7", "?r8", "?r9", "?r10", "?r11", "?r12", "?r13", "?r14", "?r15" };

#ifdef IX86_64
#define DECLARE_BASE_INSN(n16, n32, n64, func, func64, flags, flags64)\
{ n16, n32, n64, func, flags, func64, flags64 }
#else
#define DECLARE_BASE_INSN(n16, n32, n64, func, func64, flags, flags64)\
{ n16, n32, func, flags }
#endif

const ix86_Opcodes ix86_table[256] =
{
  /*0x00*/ DECLARE_BASE_INSN("add","add","add",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x01*/ DECLARE_BASE_INSN("add","add","add",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x02*/ DECLARE_BASE_INSN("add","add","add",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x03*/ DECLARE_BASE_INSN("add","add","add",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x04*/ DECLARE_BASE_INSN("add","add","add",arg_r0_imm,arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x05*/ DECLARE_BASE_INSN("add","add","add",arg_r0_imm,arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x06*/ DECLARE_BASE_INSN("push","push","???",ix86_ArgES,ix86_ArgES,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x07*/ DECLARE_BASE_INSN("pop","pop","???",ix86_ArgES,ix86_ArgES,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x08*/ DECLARE_BASE_INSN("or","or","or",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x09*/ DECLARE_BASE_INSN("or","or","or",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x0A*/ DECLARE_BASE_INSN("or","or","or",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x0B*/ DECLARE_BASE_INSN("or","or","or",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x0C*/ DECLARE_BASE_INSN("or","or","or",arg_r0_imm,arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x0D*/ DECLARE_BASE_INSN("or","or","or",arg_r0_imm,arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x0E*/ DECLARE_BASE_INSN("push","push","???", ix86_ArgCS,ix86_ArgCS,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x0F*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ExOpCodes,ix86_ExOpCodes,IX86_CPU086,K64_ATHLON),
  /*0x10*/ DECLARE_BASE_INSN("adc","adc","adc",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x11*/ DECLARE_BASE_INSN("adc","adc","adc",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x12*/ DECLARE_BASE_INSN("adc","adc","adc",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x13*/ DECLARE_BASE_INSN("adc","adc","adc",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x14*/ DECLARE_BASE_INSN("adc","adc","adc",arg_r0_imm,arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x15*/ DECLARE_BASE_INSN("adc","adc","adc",arg_r0_imm,arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x16*/ DECLARE_BASE_INSN("push","push","???",ix86_ArgSS,ix86_ArgSS,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x17*/ DECLARE_BASE_INSN("pop","pop","???",ix86_ArgSS,ix86_ArgSS,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x18*/ DECLARE_BASE_INSN("sbb","sbb","sbb",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x19*/ DECLARE_BASE_INSN("sbb","sbb","sbb",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x1A*/ DECLARE_BASE_INSN("sbb","sbb","sbb",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x1B*/ DECLARE_BASE_INSN("sbb","sbb","sbb",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x1C*/ DECLARE_BASE_INSN("sbb","sbb","sbb",arg_r0_imm,arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x1D*/ DECLARE_BASE_INSN("sbb","sbb","sbb",arg_r0_imm,arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x1E*/ DECLARE_BASE_INSN("push","push","???",ix86_ArgDS,ix86_ArgDS,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x1F*/ DECLARE_BASE_INSN("pop","pop","???",ix86_ArgDS,ix86_ArgDS,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x20*/ DECLARE_BASE_INSN("and","and","and",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x21*/ DECLARE_BASE_INSN("and","and","and",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x22*/ DECLARE_BASE_INSN("and","and","and",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x23*/ DECLARE_BASE_INSN("and","and","and",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x24*/ DECLARE_BASE_INSN("and","and","and",arg_r0_imm,arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x25*/ DECLARE_BASE_INSN("and","and","and",arg_r0_imm,arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x26*/ DECLARE_BASE_INSN("seg","seg","seg",ix86_ArgES,ix86_ArgES,IX86_CPU086,K64_ATHLON),
  /*0x27*/ DECLARE_BASE_INSN("daa","daa","???",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x28*/ DECLARE_BASE_INSN("sub","sub","sub",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x29*/ DECLARE_BASE_INSN("sub","sub","sub",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x2A*/ DECLARE_BASE_INSN("sub","sub","sub",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x2B*/ DECLARE_BASE_INSN("sub","sub","sub",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x2C*/ DECLARE_BASE_INSN("sub","sub","sub",arg_r0_imm,arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x2D*/ DECLARE_BASE_INSN("sub","sub","sub",arg_r0_imm,arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x2E*/ DECLARE_BASE_INSN("seg","seg","seg",ix86_ArgCS,ix86_ArgCS,IX86_CPU086,K64_ATHLON),
  /*0x2F*/ DECLARE_BASE_INSN("das","das","???",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x30*/ DECLARE_BASE_INSN("xor","xor","xor",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x31*/ DECLARE_BASE_INSN("xor","xor","xor",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x32*/ DECLARE_BASE_INSN("xor","xor","xor",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x33*/ DECLARE_BASE_INSN("xor","xor","xor",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x34*/ DECLARE_BASE_INSN("xor","xor","xor",arg_r0_imm,arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x35*/ DECLARE_BASE_INSN("xor","xor","xor",arg_r0_imm,arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x36*/ DECLARE_BASE_INSN("seg","seg","seg",ix86_ArgSS,ix86_ArgSS,IX86_CPU086,K64_ATHLON),
  /*0x37*/ DECLARE_BASE_INSN("aaa","aaa","???",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x38*/ DECLARE_BASE_INSN("cmp","cmp","cmp",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x39*/ DECLARE_BASE_INSN("cmp","cmp","cmp",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x3A*/ DECLARE_BASE_INSN("cmp","cmp","cmp",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x3B*/ DECLARE_BASE_INSN("cmp","cmp","cmp",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x3C*/ DECLARE_BASE_INSN("cmp","cmp","cmp",arg_r0_imm,arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x3D*/ DECLARE_BASE_INSN("cmp","cmp","cmp",arg_r0_imm,arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x3E*/ DECLARE_BASE_INSN("seg","seg","seg",ix86_ArgDS,ix86_ArgDS,IX86_CPU086,K64_ATHLON),
  /*0x3F*/ DECLARE_BASE_INSN("aas","aas","???",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x40*/ DECLARE_BASE_INSN("inc","inc","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x41*/ DECLARE_BASE_INSN("inc","inc","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x42*/ DECLARE_BASE_INSN("inc","inc","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x43*/ DECLARE_BASE_INSN("inc","inc","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x44*/ DECLARE_BASE_INSN("inc","inc","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x45*/ DECLARE_BASE_INSN("inc","inc","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x46*/ DECLARE_BASE_INSN("inc","inc","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x47*/ DECLARE_BASE_INSN("inc","inc","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x48*/ DECLARE_BASE_INSN("dec","dec","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x49*/ DECLARE_BASE_INSN("dec","dec","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x4A*/ DECLARE_BASE_INSN("dec","dec","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x4B*/ DECLARE_BASE_INSN("dec","dec","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x4C*/ DECLARE_BASE_INSN("dec","dec","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x4D*/ DECLARE_BASE_INSN("dec","dec","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x4E*/ DECLARE_BASE_INSN("dec","dec","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x4F*/ DECLARE_BASE_INSN("dec","dec","rex",arg_insnreg,NULL,IX86_CPU086,K64_ATHLON),
  /*0x50*/ DECLARE_BASE_INSN("push","push","push",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x51*/ DECLARE_BASE_INSN("push","push","push",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x52*/ DECLARE_BASE_INSN("push","push","push",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x53*/ DECLARE_BASE_INSN("push","push","push",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x54*/ DECLARE_BASE_INSN("push","push","push",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x55*/ DECLARE_BASE_INSN("push","push","push",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x56*/ DECLARE_BASE_INSN("push","push","push",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x57*/ DECLARE_BASE_INSN("push","push","push",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x58*/ DECLARE_BASE_INSN("pop","pop","pop",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x59*/ DECLARE_BASE_INSN("pop","pop","pop",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x5A*/ DECLARE_BASE_INSN("pop","pop","pop",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x5B*/ DECLARE_BASE_INSN("pop","pop","pop",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x5C*/ DECLARE_BASE_INSN("pop","pop","pop",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x5D*/ DECLARE_BASE_INSN("pop","pop","pop",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x5E*/ DECLARE_BASE_INSN("pop","pop","pop",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x5F*/ DECLARE_BASE_INSN("pop","pop","pop",arg_insnreg,arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x60*/ DECLARE_BASE_INSN("pushaw","pushad","???",NULL,NULL,IX86_CPU186,K64_ATHLON|K64_NOCOMPAT),
  /*0x61*/ DECLARE_BASE_INSN("popaw","popad","???",NULL,NULL,IX86_CPU186,K64_ATHLON|K64_NOCOMPAT),
  /*0x62*/ DECLARE_BASE_INSN("bound","bound","???",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU286,K64_ATHLON|K64_NOCOMPAT),
  /*0x63*/ DECLARE_BASE_INSN("arpl","arpl","movsxd",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU286|INSN_STORE,K64_ATHLON|K64_NOCOMPAT),
  /*0x64*/ DECLARE_BASE_INSN("seg","seg","seg",ix86_ArgFS,ix86_ArgFS,IX86_CPU386,K64_ATHLON),
  /*0x65*/ DECLARE_BASE_INSN("seg","seg","seg",ix86_ArgGS,ix86_ArgGS,IX86_CPU386,K64_ATHLON),
  /*0x66*/ DECLARE_BASE_INSN("???","???","???",NULL,NULL,IX86_CPU386,K64_ATHLON),
  /*0x67*/ DECLARE_BASE_INSN("???","???","???",NULL,NULL,IX86_CPU386,K64_ATHLON),
  /*0x68*/ DECLARE_BASE_INSN("push","push","push",arg_imm,arg_imm,IX86_CPU186,K64_ATHLON),
  /*0x69*/ DECLARE_BASE_INSN("imul","imul","imul",arg_cpu_modregrm_imm,arg_cpu_modregrm_imm,IX86_CPU186,K64_ATHLON),
  /*0x6A*/ DECLARE_BASE_INSN("push","push","push",arg_imm,arg_imm,IX86_CPU186|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x6B*/ DECLARE_BASE_INSN("imul","imul","imul",arg_cpu_modregrm_imm,arg_cpu_modregrm_imm,IX86_CPU186|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x6C*/ DECLARE_BASE_INSN("insb","insb","insb",NULL,NULL,IX86_CPU186|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x6D*/ DECLARE_BASE_INSN("insw","insd","insd",NULL,NULL,IX86_CPU186|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x6E*/ DECLARE_BASE_INSN("outsb","outsb","outsb",NULL,NULL,IX86_CPU186|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x6F*/ DECLARE_BASE_INSN("outsw","outsd","outsd",NULL,NULL,IX86_CPU186|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x70*/ DECLARE_BASE_INSN("jo","jo","jo",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x71*/ DECLARE_BASE_INSN("jno","jno","jno",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x72*/ DECLARE_BASE_INSN("jc","jc","jc",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x73*/ DECLARE_BASE_INSN("jnc","jnc","jnc",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x74*/ DECLARE_BASE_INSN("je","je","je",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x75*/ DECLARE_BASE_INSN("jne","jne","jne",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x76*/ DECLARE_BASE_INSN("jna","jna","jna",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x77*/ DECLARE_BASE_INSN("ja","ja","ja",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x78*/ DECLARE_BASE_INSN("js","js","js",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x79*/ DECLARE_BASE_INSN("jns","jns","jns",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x7A*/ DECLARE_BASE_INSN("jp","jp","jp",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x7B*/ DECLARE_BASE_INSN("jnp","jnp","jnp",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x7C*/ DECLARE_BASE_INSN("jl","jl","jl",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x7D*/ DECLARE_BASE_INSN("jnl","jnl","jnl",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x7E*/ DECLARE_BASE_INSN("jle","jle","jle",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x7F*/ DECLARE_BASE_INSN("jg","jg","jg",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x80*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgOp1,ix86_ArgOp1,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x81*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgOp1,ix86_ArgOp1,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x82*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgOp2,ix86_ArgOp2,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x83*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgOp2,ix86_ArgOp2,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x84*/ DECLARE_BASE_INSN("test","test","test",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x85*/ DECLARE_BASE_INSN("test","test","test",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x86*/ DECLARE_BASE_INSN("xchg","xchg","xchg",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x87*/ DECLARE_BASE_INSN("xchg","xchg","xchg",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x88*/ DECLARE_BASE_INSN("mov","mov","mov",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x89*/ DECLARE_BASE_INSN("mov","mov","mov",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x8A*/ DECLARE_BASE_INSN("mov","mov","mov",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x8B*/ DECLARE_BASE_INSN("mov","mov","mov",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x8C*/ DECLARE_BASE_INSN("mov","mov","mov",arg_cpu_modsegrm,arg_cpu_modsegrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x8D*/ DECLARE_BASE_INSN("lea","lea","lea",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x8E*/ DECLARE_BASE_INSN("mov","mov","mov",arg_cpu_modsegrm,arg_cpu_modsegrm,IX86_CPU086,K64_ATHLON),
  /*0x8F*/ DECLARE_BASE_INSN("pop","pop","xop",arg_cpu_mod_rm,NULL,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x90*/ DECLARE_BASE_INSN("xchg","xchg","xchg",arg_r0rm,arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x91*/ DECLARE_BASE_INSN("xchg","xchg","xchg",arg_r0rm,arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x92*/ DECLARE_BASE_INSN("xchg","xchg","xchg",arg_r0rm,arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x93*/ DECLARE_BASE_INSN("xchg","xchg","xchg",arg_r0rm,arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x94*/ DECLARE_BASE_INSN("xchg","xchg","xchg",arg_r0rm,arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x95*/ DECLARE_BASE_INSN("xchg","xchg","xchg",arg_r0rm,arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x96*/ DECLARE_BASE_INSN("xchg","xchg","xchg",arg_r0rm,arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x97*/ DECLARE_BASE_INSN("xchg","xchg","xchg",arg_r0rm,arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x98*/ DECLARE_BASE_INSN("cbw","cwde","cdqe",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0x99*/ DECLARE_BASE_INSN("cwd","cdq","cqo",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0x9A*/ DECLARE_BASE_INSN("callf16","callf32","???",arg_segoff,arg_segoff,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x9B*/ DECLARE_BASE_INSN("wait","wait","wait",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0x9C*/ DECLARE_BASE_INSN("pushfw","pushfd","pushfq",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x9D*/ DECLARE_BASE_INSN("popfw","popfd","popfq",NULL,NULL,IX86_CPU086|INSN_CPL0,K64_ATHLON|K64_NOCOMPAT|INSN_CPL0),
  /*0x9E*/ DECLARE_BASE_INSN("sahf","sahf","sahf",NULL,NULL,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x9F*/ DECLARE_BASE_INSN("lahf","lahf","lahf",NULL,NULL,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xA0*/ DECLARE_BASE_INSN("mov","mov","mov",arg_r0mem,arg_r0mem,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xA1*/ DECLARE_BASE_INSN("mov","mov","mov",arg_r0mem,arg_r0mem,IX86_CPU086,K64_ATHLON),
  /*0xA2*/ DECLARE_BASE_INSN("mov","mov","mov",arg_r0mem,arg_r0mem,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xA3*/ DECLARE_BASE_INSN("mov","mov","mov",arg_r0mem,arg_r0mem,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xA4*/ DECLARE_BASE_INSN("movsb","movsb","movsb",NULL,NULL,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xA5*/ DECLARE_BASE_INSN("movsw","movsd","movsq",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xA6*/ DECLARE_BASE_INSN("cmpsb","cmpsb","cmpsb",NULL,NULL,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xA7*/ DECLARE_BASE_INSN("cmpsw","cmpsd","cmpsq",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xA8*/ DECLARE_BASE_INSN("test","test","test",arg_r0_imm,arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xA9*/ DECLARE_BASE_INSN("test","test","test",arg_r0_imm,arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0xAA*/ DECLARE_BASE_INSN("stosb","stosb","stosb",NULL,NULL,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xAB*/ DECLARE_BASE_INSN("stosw","stosd","stosq",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xAC*/ DECLARE_BASE_INSN("lodsb","lodsb","lodsb",NULL,NULL,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xAD*/ DECLARE_BASE_INSN("lodsw","lodsd","lodsq",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xAE*/ DECLARE_BASE_INSN("scasb","scasb","scasb",NULL,NULL,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xAF*/ DECLARE_BASE_INSN("scasw","scasd","scasq",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xB0*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB1*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB2*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB3*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB4*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB5*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB6*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB7*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB8*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xB9*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xBA*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xBB*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xBC*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xBD*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xBE*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xBF*/ DECLARE_BASE_INSN("mov","mov","mov",arg_insnreg_imm,arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xC0*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ShOp2,ix86_ShOp2,IX86_CPU186|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xC1*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ShOp2,ix86_ShOp2,IX86_CPU186|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xC2*/ DECLARE_BASE_INSN("retn16","retn32","retn32",arg_imm16,arg_imm16,IX86_CPU086,K64_ATHLON),
  /*0xC3*/ DECLARE_BASE_INSN("retn16","retn32","retn32",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xC4*/ DECLARE_BASE_INSN("les","les","vex",arg_cpu_modregrm,NULL,IX86_CPU086,K64_ATHLON),
  /*0xC5*/ DECLARE_BASE_INSN("lds","lds","vex",arg_cpu_modregrm,NULL,IX86_CPU086,K64_ATHLON),
  /*0xC6*/ DECLARE_BASE_INSN("mov","mov","mov",arg_cpu_mod_rm_imm,arg_cpu_mod_rm_imm,IX86_CPU086|INSN_OP_BYTE|IMM_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|IMM_BYTE|INSN_STORE),
  /*0xC7*/ DECLARE_BASE_INSN("mov","mov","mov",arg_cpu_mod_rm_imm,arg_cpu_mod_rm_imm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xC8*/ DECLARE_BASE_INSN("enter","enter","enter",arg_imm16_imm8,arg_imm16_imm8,IX86_CPU186,K64_ATHLON),
  /*0xC9*/ DECLARE_BASE_INSN("leave","leave","leave",NULL,NULL,IX86_CPU186,K64_ATHLON),
  /*0xCA*/ DECLARE_BASE_INSN("retf16","retf32","retf32",arg_imm16,arg_imm16,IX86_CPU086,K64_ATHLON),
  /*0xCB*/ DECLARE_BASE_INSN("retf16","retf32","retf32",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xCC*/ DECLARE_BASE_INSN("int3","int3","int3",NULL,NULL,IX86_CPU086|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xCD*/ DECLARE_BASE_INSN("int","int","int",arg_imm8,arg_imm8,IX86_CPU086,K64_ATHLON),
  /*0xCE*/ DECLARE_BASE_INSN("into16","into32","???",NULL,NULL,IX86_CPU086|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xCF*/ DECLARE_BASE_INSN("iret16","iret32","iretq",NULL,NULL,IX86_CPU086|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xD0*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ShOp1,ix86_ShOp1,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xD1*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ShOp1,ix86_ShOp1,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xD2*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ShOpCL,ix86_ShOpCL,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xD3*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ShOpCL,ix86_ShOpCL,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xD4*/ DECLARE_BASE_INSN("aam on","aam on","???",arg_imm8,arg_imm8,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0xD5*/ DECLARE_BASE_INSN("aad on","aad on","???",arg_imm8,arg_imm8,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0xD6*/ DECLARE_BASE_INSN("salc","salc","salc",NULL,NULL,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xD7*/ DECLARE_BASE_INSN("xlat","xlat","xlat",NULL,NULL,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xD8*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xD9*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xDA*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xDB*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xDC*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xDD*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xDE*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xDF*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xE0*/ DECLARE_BASE_INSN("loopnz","loopnz","loopnz",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0xE1*/ DECLARE_BASE_INSN("loopz","loopz","loopz",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0xE2*/ DECLARE_BASE_INSN("loop","loop","loop",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0xE3*/ DECLARE_BASE_INSN("jcxz","jecxz","jrcxz",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0xE4*/ DECLARE_BASE_INSN("in","in","in",ix86_InOut,ix86_InOut,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xE5*/ DECLARE_BASE_INSN("in","in","in",ix86_InOut,ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xE6*/ DECLARE_BASE_INSN("out","out","out",ix86_InOut,ix86_InOut,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xE7*/ DECLARE_BASE_INSN("out","out","out",ix86_InOut,ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xE8*/ DECLARE_BASE_INSN("calln16","calln32","calln32",arg_offset,arg_offset,IX86_CPU086,K64_ATHLON),
  /*0xE9*/ DECLARE_BASE_INSN("jmpn16","jmpn32","jmpn32",arg_offset,arg_offset,IX86_CPU086,K64_ATHLON),
  /*0xEA*/ DECLARE_BASE_INSN("jmpf16","jmpf32","???",arg_segoff,arg_segoff,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0xEB*/ DECLARE_BASE_INSN("jmps","jmps","jmps",arg_offset,arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0xEC*/ DECLARE_BASE_INSN("in","in","in",ix86_InOut,ix86_InOut,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xED*/ DECLARE_BASE_INSN("in","in","in",ix86_InOut,ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xEE*/ DECLARE_BASE_INSN("out","out","out",ix86_InOut,ix86_InOut,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xEF*/ DECLARE_BASE_INSN("out","out","out",ix86_InOut,ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xF0*/ DECLARE_BASE_INSN("lock","lock","lock",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xF1*/ DECLARE_BASE_INSN("icebp","icebp","icebp",NULL,NULL,IX86_CPU386,K64_ATHLON),
  /*0xF2*/ DECLARE_BASE_INSN("repne","repne","repne",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xF3*/ DECLARE_BASE_INSN("rep","rep","rep",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xF4*/ DECLARE_BASE_INSN("hlt","hlt","hlt",NULL,NULL,IX86_CPU086|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xF5*/ DECLARE_BASE_INSN("cmc","cmc","cmc",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xF6*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgGrp1,ix86_ArgGrp1,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xF7*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgGrp1,ix86_ArgGrp1,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xF8*/ DECLARE_BASE_INSN("clc","clc","clc",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xF9*/ DECLARE_BASE_INSN("stc","stc","stc",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xFA*/ DECLARE_BASE_INSN("cli","cli","cli",NULL,NULL,IX86_CPU086|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xFB*/ DECLARE_BASE_INSN("sti","sti","sti",NULL,NULL,IX86_CPU086|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xFC*/ DECLARE_BASE_INSN("cld","cld","cld",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xFD*/ DECLARE_BASE_INSN("std","std","std",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xFE*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgGrp2,ix86_ArgGrp2,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xFF*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgGrp2,ix86_ArgGrp2,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE)
};

#ifdef IX86_64
#define DECLARE_EX_INSN(n32, n64, func, func64, flags, flags64)\
{ n32, n64, func, func64, flags64, flags }
#else
#define DECLARE_EX_INSN(n32, n64, func, func64, flags, flags64)\
{ n32, func, flags }
#endif

const ix86_ExOpcodes ix86_0F38_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN("pshufb", "pshufb",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x01*/ DECLARE_EX_INSN("phaddw", "phaddw",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x02*/ DECLARE_EX_INSN("phaddd", "phaddd",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x03*/ DECLARE_EX_INSN("phaddsw", "phaddsw",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x04*/ DECLARE_EX_INSN("pmaddubsw", "pmaddubsw",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x05*/ DECLARE_EX_INSN("phsubw", "phsubw",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x06*/ DECLARE_EX_INSN("phsubd", "phsubd",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x07*/ DECLARE_EX_INSN("phsubsw", "phsubsw",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x08*/ DECLARE_EX_INSN("psingb", "psignb",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x09*/ DECLARE_EX_INSN("psignw", "psignw",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x0A*/ DECLARE_EX_INSN("psignd", "psignd",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x0B*/ DECLARE_EX_INSN("pmulhrsw", "pmulhrsw",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN("pabsb", "pabsb",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x1D*/ DECLARE_EX_INSN("pabsw", "pabsw",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x1E*/ DECLARE_EX_INSN("pabsd", "pabsd",arg_simd,arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN("movbe", "movbe", arg_cpu_modregrm, arg_cpu_modregrm, IX86_P8, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN("movbe", "movbe", arg_cpu_modregrm, arg_cpu_modregrm, IX86_P8|INSN_STORE, K64_ATHLON|INSN_STORE),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_0F3A_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN("palignr", "palignr",arg_simd_imm8,arg_simd_imm8, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON)
};

/*
  NOP:
  90
  66 90
  0F 1F /0
  0F 1F 00
  0F 1F 40 00
  0F 1F 44 00 00
  66 0F 1F 44 00 00
  0F 1F 80 00 00 00 00
  0F 1F 84 00 00 00 00 00
  66 0F 1F 84 00 00 00 00 00
*/

const ix86_ExOpcodes ix86_extable[256] = /* for 0FH leading */
{
  /*0x00*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgExGr0,ix86_ArgExGr0,IX86_CPU286|INSN_STORE,K64_ATHLON|INSN_CPL0|INSN_STORE),
  /*0x01*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgExGr1,ix86_ArgExGr1,IX86_CPU286|INSN_CPL0|INSN_STORE,K64_ATHLON|INSN_CPL0|INSN_STORE),
  /*0x02*/ DECLARE_EX_INSN("lar","lar",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU286|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x03*/ DECLARE_EX_INSN("lsl","lsl",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU286|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x04*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN("syscall","syscall",NULL,NULL,IX86_K6|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x06*/ DECLARE_EX_INSN("clts","clts",NULL,NULL,IX86_CPU286|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x07*/ DECLARE_EX_INSN("sysret","sysret",NULL,NULL,IX86_K6|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x08*/ DECLARE_EX_INSN("invd","invd",NULL,NULL,IX86_CPU486|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x09*/ DECLARE_EX_INSN("wbinvd","wbinvd",NULL,NULL,IX86_CPU486|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x0A*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN("ud","ud",NULL,NULL,IX86_CPU686,K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN("!!!","!!!",ix86_3dNowPrefetchGrp,ix86_3dNowPrefetchGrp,IX86_3DNOW,K64_ATHLON|INSN_MMX),
  /*0x0E*/ DECLARE_EX_INSN("femms","femms",NULL,NULL,IX86_3DNOW,K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN("!!!","!!!",ix86_3dNowOpCodes,ix86_3dNowOpCodes,IX86_3DNOW,K64_ATHLON|INSN_MMX),
  /*0x10*/ DECLARE_EX_INSN("movups","movups",arg_simd,arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x11*/ DECLARE_EX_INSN("movups","movups",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x12*/ DECLARE_EX_INSN("movlps","movlps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x13*/ DECLARE_EX_INSN("movlps","movlps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x14*/ DECLARE_EX_INSN("unpcklps","unpcklps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x15*/ DECLARE_EX_INSN("unpckhps","unpckhps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x16*/ DECLARE_EX_INSN("movhps","movhps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x17*/ DECLARE_EX_INSN("movhps","movhps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x18*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgKatmaiGrp2,ix86_ArgKatmaiGrp2,IX86_P3|INSN_SSE|INSN_OP_BYTE,K64_ATHLON|INSN_SSE|INSN_OP_BYTE),
  /*0x19*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN("nop","nop",arg_cpu_modregrm,arg_cpu_modregrm,IX86_P5,K64_FAM9),
  /*0x20*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x21*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x22*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x23*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x24*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x25*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x26*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x27*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x28*/ DECLARE_EX_INSN("movaps","movaps",arg_simd,arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x29*/ DECLARE_EX_INSN("movaps","movaps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x2A*/ DECLARE_EX_INSN("cvtpi2ps","cvtpi2ps",bridge_sse_mmx,bridge_sse_mmx,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x2B*/ DECLARE_EX_INSN("movntps","movntps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x2C*/ DECLARE_EX_INSN("cvttps2pi","cvttps2pi",bridge_sse_mmx,bridge_sse_mmx,IX86_P3|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE),
  /*0x2D*/ DECLARE_EX_INSN("cvtps2pi","cvtps2pi",bridge_sse_mmx,bridge_sse_mmx,IX86_P3|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE),
  /*0x2E*/ DECLARE_EX_INSN("ucomiss","ucomiss",arg_simd,arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x2F*/ DECLARE_EX_INSN("comiss","comiss",arg_simd,arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x30*/ DECLARE_EX_INSN("wrmsr","wrmsr",NULL,NULL,IX86_CPU586|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x31*/ DECLARE_EX_INSN("rdtsc","rdtsc",NULL,NULL,IX86_CPU586|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x32*/ DECLARE_EX_INSN("rdmsr","rdmsr",NULL,NULL,IX86_CPU586|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x33*/ DECLARE_EX_INSN("rdpmc","rdpmc",NULL,NULL,IX86_CPU686|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x34*/ DECLARE_EX_INSN("sysenter","???",NULL,NULL,IX86_P2|INSN_CPL0,K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN("sysexit","???",NULL,NULL,IX86_P2|INSN_CPL0,K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN("rdshr","???",NULL,NULL,IX86_CYRIX686,K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN("getsec","getsec",NULL,NULL,IX86_P6,K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(ix86_0F38_Table,ix86_0F38_Table,NULL,NULL,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x39*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(ix86_0F3A_Table,ix86_0F3A_Table,NULL,NULL,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x3B*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN("cmovo","cmovo",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN("cmovno","cmovno",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN("cmovc","cmovc",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN("cmovnc","cmovnc",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN("cmove","cmove",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN("cmovne","cmovne",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN("cmovna","cmovna",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN("cmova","cmova",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN("cmovs","cmovs",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN("cmovns","cmovns",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN("cmovp","cmovp",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN("cmovnp","cmovnp",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN("cmovl","cmovl",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN("cmovnl","cmovnl",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN("cmovle","cmovle",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN("cmovg","cmovg",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN("movmskps","movmskps",bridge_simd_cpu,bridge_simd_cpu,IX86_P3|INSN_SSE|INSN_STORE|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|INSN_STORE|BRIDGE_CPU_SSE),
  /*0x51*/ DECLARE_EX_INSN("sqrtps","sqrtps",arg_simd,arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x52*/ DECLARE_EX_INSN("rsqrtps","rsqrtps",arg_simd,arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x53*/ DECLARE_EX_INSN("rcpps","rcpps",arg_simd,arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x54*/ DECLARE_EX_INSN("andps","andps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x55*/ DECLARE_EX_INSN("andnps","andnps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x56*/ DECLARE_EX_INSN("orps","orps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x57*/ DECLARE_EX_INSN("xorps","xorps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x58*/ DECLARE_EX_INSN("addps","addps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x59*/ DECLARE_EX_INSN("mulps","mulps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5A*/ DECLARE_EX_INSN("cvtps2pd","cvtps2pd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5B*/ DECLARE_EX_INSN("cvtdq2ps","cvtdq2ps",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5C*/ DECLARE_EX_INSN("subps","subps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5D*/ DECLARE_EX_INSN("minps","minps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5E*/ DECLARE_EX_INSN("divps","divps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5F*/ DECLARE_EX_INSN("maxps","maxps",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x60*/ DECLARE_EX_INSN("punpcklbw","punpcklbw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x61*/ DECLARE_EX_INSN("punpcklwd","punpcklwd",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x62*/ DECLARE_EX_INSN("punpckldq","punpckldq",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x63*/ DECLARE_EX_INSN("packsswb","packsswb",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x64*/ DECLARE_EX_INSN("pcmpgtb","pcmpgtb",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x65*/ DECLARE_EX_INSN("pcmpgtw","pcmpgtw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x66*/ DECLARE_EX_INSN("pcmpgtd","pcmpgtd",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x67*/ DECLARE_EX_INSN("packuswb","packuswb",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x68*/ DECLARE_EX_INSN("punpckhbw","punpckhbw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x69*/ DECLARE_EX_INSN("punpckhwd","punpckhwd",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x6A*/ DECLARE_EX_INSN("punpckhdq","punpckhdq",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x6B*/ DECLARE_EX_INSN("packssdw","packssdw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x6C*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKMMX,K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKMMX,K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN("movd","movd",bridge_simd_cpu,bridge_simd_cpu,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x6F*/ DECLARE_EX_INSN("movq","movq",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x70*/ DECLARE_EX_INSN("pshufw","pshufw",arg_simd_imm8,arg_simd_imm8,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x71*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgMMXGr1,ix86_ArgMMXGr1,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x72*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgMMXGr2,ix86_ArgMMXGr2,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x73*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgMMXGr3,ix86_ArgMMXGr3,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x74*/ DECLARE_EX_INSN("pcmpeqb","pcmpeqb",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x75*/ DECLARE_EX_INSN("pcmpeqw","pcmpeqw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x76*/ DECLARE_EX_INSN("pcmpeqd","pcmpeqd",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x77*/ DECLARE_EX_INSN("emms","emms",arg_emms,arg_emms,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x78*/ DECLARE_EX_INSN("vmread","vmread",arg_cpu_modregrm,arg_cpu_modregrm,IX86_P6|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x79*/ DECLARE_EX_INSN("vmwrite","vmwrite",arg_cpu_modregrm,arg_cpu_modregrm,IX86_P6,K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN("svldt","???",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CYRIX486,K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN("rsldt","???",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CYRIX486,K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN("svts","???",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CYRIX486,K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN("rsts","???",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CYRIX486,K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN("movd","movd",bridge_simd_cpu,bridge_simd_cpu,IX86_CPU586|INSN_MMX|INSN_STORE,K64_ATHLON|INSN_MMX|INSN_STORE),
  /*0x7F*/ DECLARE_EX_INSN("movq","movq",arg_simd,arg_simd,IX86_CPU586|INSN_MMX|INSN_STORE,K64_ATHLON|INSN_MMX|INSN_STORE),
  /*0x80*/ DECLARE_EX_INSN("jo","jo",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN("jno","jno",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN("jc","jc",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN("jnc","jnc",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN("je","je",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN("jne","jne",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN("jna","jna",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN("ja","ja",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN("js","js",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN("jns","jns",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN("jp","jp",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN("jnp","jnp",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN("jl","jl",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN("jnl","jnl",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN("jle","jle",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN("jg","jg",arg_offset,arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN("seto","seto",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x91*/ DECLARE_EX_INSN("setno","setno",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x92*/ DECLARE_EX_INSN("setc","setc",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x93*/ DECLARE_EX_INSN("setnc","setnc",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x94*/ DECLARE_EX_INSN("sete","sete",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x95*/ DECLARE_EX_INSN("setne","setne",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x96*/ DECLARE_EX_INSN("setna","setna",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x97*/ DECLARE_EX_INSN("seta","seta",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x98*/ DECLARE_EX_INSN("sets","sets",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x99*/ DECLARE_EX_INSN("setns","setns",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x9A*/ DECLARE_EX_INSN("setp","setp",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x9B*/ DECLARE_EX_INSN("setnp","setnp",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x9C*/ DECLARE_EX_INSN("setl","setl",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x9D*/ DECLARE_EX_INSN("setnl","setnl",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x9E*/ DECLARE_EX_INSN("setle","setle",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x9F*/ DECLARE_EX_INSN("setg","setg",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0xA0*/ DECLARE_EX_INSN("push","push",ix86_ArgFS,ix86_ArgFS,IX86_CPU386,K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN("pop","pop",ix86_ArgFS,ix86_ArgFS,IX86_CPU386,K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN("cpuid","cpuid",NULL,NULL,IX86_CPU486,K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN("bt","bt",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xA4*/ DECLARE_EX_INSN("shld","shld",ix86_DblShift,ix86_DblShift,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xA5*/ DECLARE_EX_INSN("shld","shld",ix86_DblShift,ix86_DblShift,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xA6*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN("push","push",ix86_ArgGS,ix86_ArgGS,IX86_CPU386,K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN("pop","pop",ix86_ArgGS,ix86_ArgGS,IX86_CPU386,K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN("rsm","rsm",NULL,NULL,IX86_CPU586|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xAB*/ DECLARE_EX_INSN("bts","bts",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xAC*/ DECLARE_EX_INSN("shrd","shrd",ix86_DblShift,ix86_DblShift,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xAD*/ DECLARE_EX_INSN("shrd","shrd",ix86_DblShift,ix86_DblShift,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xAE*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgKatmaiGrp1,ix86_ArgKatmaiGrp1,IX86_P3|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0xAF*/ DECLARE_EX_INSN("imul","imul",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU386,K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN("cmpxchg","cmpxchg",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU486|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xB1*/ DECLARE_EX_INSN("cmpxchg","cmpxchg",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU486|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xB2*/ DECLARE_EX_INSN("lss","lss",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU386,K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN("btr","btr",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xB4*/ DECLARE_EX_INSN("lfs","lfs",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU386,K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN("lgs","lgs",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU386,K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN("movzx","movzx",ix86_ArgMovYX,ix86_ArgMovYX,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xB7*/ DECLARE_EX_INSN("movzx","movzx",ix86_ArgMovYX,ix86_ArgMovYX,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xB8*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN("ud2","ud2",NULL,NULL,IX86_CPU686,K64_ATHLON|INSN_CPL0),
  /*0xBA*/ DECLARE_EX_INSN("!!!","!!!",ix86_BitGrp,ix86_BitGrp,IX86_CPU386,K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN("btc","btc",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xBC*/ DECLARE_EX_INSN("bsf","bsf",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU386,K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN("bsr","bsr",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU386,K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN("movsx","movsx",ix86_ArgMovYX,ix86_ArgMovYX,IX86_CPU386|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xBF*/ DECLARE_EX_INSN("movsx","movsx",ix86_ArgMovYX,ix86_ArgMovYX,IX86_CPU386,K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN("xadd","xadd",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU486|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xC1*/ DECLARE_EX_INSN("xadd","xadd",arg_cpu_modregrm,arg_cpu_modregrm,IX86_CPU486|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xC2*/ DECLARE_EX_INSN("cmpps","cmpps",ix86_ArgXMMCmp,ix86_ArgXMMCmp,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC3*/ DECLARE_EX_INSN("movnti","movnti",arg_cpu_modregrm,arg_cpu_modregrm,IX86_P4|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xC4*/ DECLARE_EX_INSN("pinsrw","pinsrw",bridge_simd_cpu_imm8,bridge_simd_cpu_imm8,IX86_P3|INSN_MMX|INSN_VEX_V,K64_ATHLON|INSN_MMX|INSN_VEX_V),
  /*0xC5*/ DECLARE_EX_INSN("pextrw","pextrw",bridge_simd_cpu_imm8,bridge_simd_cpu_imm8,IX86_P3|INSN_MMX|INSN_VEX_V|BRIDGE_CPU_SSE|INSN_STORE,K64_ATHLON|INSN_MMX|INSN_VEX_V|BRIDGE_CPU_SSE|INSN_STORE),
  /*0xC6*/ DECLARE_EX_INSN("shufps","shufps",arg_simd_imm8,arg_simd_imm8,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC7*/ DECLARE_EX_INSN("!!!","!!!",ix86_0FVMX,ix86_0FVMX,IX86_CPU586,K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN("bswap","bswap",arg_insnreg,arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN("bswap","bswap",arg_insnreg,arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN("bswap","bswap",arg_insnreg,arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN("bswap","bswap",arg_insnreg,arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN("bswap","bswap",arg_insnreg,arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN("bswap","bswap",arg_insnreg,arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN("bswap","bswap",arg_insnreg,arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN("bswap","bswap",arg_insnreg,arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN("???","???",arg_simd,arg_simd,IX86_UNKMMX,K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN("psrlw","psrlw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xD2*/ DECLARE_EX_INSN("psrld","psrld",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xD3*/ DECLARE_EX_INSN("psrlq","psrlq",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xD4*/ DECLARE_EX_INSN("paddq","paddq",arg_simd,arg_simd,IX86_P4|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xD5*/ DECLARE_EX_INSN("pmullw","pmullw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xD6*/ DECLARE_EX_INSN("???","???",arg_simd,arg_simd,IX86_UNKMMX,K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN("pmovmskb","pmovmskb",bridge_simd_cpu,bridge_simd_cpu,IX86_P3|INSN_MMX|BRIDGE_CPU_SSE,K64_ATHLON|INSN_MMX|BRIDGE_CPU_SSE),
  /*0xD8*/ DECLARE_EX_INSN("psubusb","psubusb",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xD9*/ DECLARE_EX_INSN("psubusw","psubusw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xDA*/ DECLARE_EX_INSN("pminub","pminub",arg_simd,arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xDB*/ DECLARE_EX_INSN("pand","pand",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xDC*/ DECLARE_EX_INSN("paddusb","paddusb",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xDD*/ DECLARE_EX_INSN("paddusw","paddusw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xDE*/ DECLARE_EX_INSN("pmaxub","pmaxub",arg_simd,arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xDF*/ DECLARE_EX_INSN("pandn","pandn",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE0*/ DECLARE_EX_INSN("pavgb","pavgb",arg_simd,arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE1*/ DECLARE_EX_INSN("psraw","psraw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE2*/ DECLARE_EX_INSN("psrad","psrad",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE3*/ DECLARE_EX_INSN("pavgw","pavgw",arg_simd,arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE4*/ DECLARE_EX_INSN("pmulhuw","pmulhuw",arg_simd,arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE5*/ DECLARE_EX_INSN("pmulhw","pmulhw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE6*/ DECLARE_EX_INSN("???","???",arg_simd,arg_simd,IX86_UNKMMX,K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN("movntq","movntq",arg_simd,arg_simd,IX86_P3|INSN_MMX|INSN_STORE,K64_ATHLON|INSN_MMX|INSN_STORE),
  /*0xE8*/ DECLARE_EX_INSN("psubsb","psubsb",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE9*/ DECLARE_EX_INSN("psubsw","psubsw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xEA*/ DECLARE_EX_INSN("pminsw","pminsw",arg_simd,arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xEB*/ DECLARE_EX_INSN("por","por",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xEC*/ DECLARE_EX_INSN("paddsb","paddsb",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xED*/ DECLARE_EX_INSN("paddsw","paddsw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xEE*/ DECLARE_EX_INSN("pmaxsw","pmaxsw",arg_simd,arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xEF*/ DECLARE_EX_INSN("pxor","pxor",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF0*/ DECLARE_EX_INSN("???","???",arg_simd,arg_simd,IX86_UNKMMX,K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN("psllw","psllw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF2*/ DECLARE_EX_INSN("pslld","pslld",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF3*/ DECLARE_EX_INSN("psllq","psllq",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF4*/ DECLARE_EX_INSN("pmuludq","pmuludq",arg_simd,arg_simd,IX86_P4|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF5*/ DECLARE_EX_INSN("pmaddwd","pmaddwd",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF6*/ DECLARE_EX_INSN("psadbw","psadbw",arg_simd,arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF7*/ DECLARE_EX_INSN("maskmovq","maskmovq",arg_simd,arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF8*/ DECLARE_EX_INSN("psubb","psubb",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF9*/ DECLARE_EX_INSN("psubw","psubw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xFA*/ DECLARE_EX_INSN("psubd","psubd",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xFB*/ DECLARE_EX_INSN("psubq","psubq",arg_simd,arg_simd,IX86_P4|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xFC*/ DECLARE_EX_INSN("paddb","paddb",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xFD*/ DECLARE_EX_INSN("paddw","paddw",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xFE*/ DECLARE_EX_INSN("paddd","paddd",arg_simd,arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xFF*/ DECLARE_EX_INSN("???","???",arg_simd,arg_simd,IX86_UNKMMX,K64_ATHLON)
};

const ix86_3dNowopcodes ix86_3dNowtable[256] =
{
  /*0x00*/ { "???", IX86_UNKAMD },
  /*0x01*/ { "???", IX86_UNKAMD },
  /*0x02*/ { "???", IX86_UNKAMD },
  /*0x03*/ { "???", IX86_UNKAMD },
  /*0x04*/ { "???", IX86_UNKAMD },
  /*0x05*/ { "???", IX86_UNKAMD },
  /*0x06*/ { "???", IX86_UNKAMD },
  /*0x07*/ { "???", IX86_UNKAMD },
  /*0x08*/ { "???", IX86_UNKAMD },
  /*0x09*/ { "???", IX86_UNKAMD },
  /*0x0A*/ { "???", IX86_UNKAMD },
  /*0x0B*/ { "???", IX86_UNKAMD },
  /*0x0C*/ { "pi2fw", IX86_ATHLON },
  /*0x0D*/ { "pi2fd", IX86_3DNOW },
  /*0x0E*/ { "???", IX86_UNKAMD },
  /*0x0F*/ { "???", IX86_UNKAMD },
  /*0x10*/ { "???", IX86_UNKAMD },
  /*0x11*/ { "???", IX86_UNKAMD },
  /*0x12*/ { "???", IX86_UNKAMD },
  /*0x13*/ { "???", IX86_UNKAMD },
  /*0x14*/ { "???", IX86_UNKAMD },
  /*0x15*/ { "???", IX86_UNKAMD },
  /*0x16*/ { "???", IX86_UNKAMD },
  /*0x17*/ { "???", IX86_UNKAMD },
  /*0x18*/ { "???", IX86_UNKAMD },
  /*0x19*/ { "???", IX86_UNKAMD },
  /*0x1A*/ { "???", IX86_UNKAMD },
  /*0x1B*/ { "???", IX86_UNKAMD },
  /*0x1C*/ { "pf2iw", IX86_ATHLON },
  /*0x1D*/ { "pf2id", IX86_3DNOW },
  /*0x1E*/ { "???", IX86_UNKAMD },
  /*0x1F*/ { "???", IX86_UNKAMD },
  /*0x20*/ { "???", IX86_UNKAMD },
  /*0x21*/ { "???", IX86_UNKAMD },
  /*0x22*/ { "???", IX86_UNKAMD },
  /*0x23*/ { "???", IX86_UNKAMD },
  /*0x24*/ { "???", IX86_UNKAMD },
  /*0x25*/ { "???", IX86_UNKAMD },
  /*0x26*/ { "???", IX86_UNKAMD },
  /*0x27*/ { "???", IX86_UNKAMD },
  /*0x28*/ { "???", IX86_UNKAMD },
  /*0x29*/ { "???", IX86_UNKAMD },
  /*0x2A*/ { "???", IX86_UNKAMD },
  /*0x2B*/ { "???", IX86_UNKAMD },
  /*0x2C*/ { "???", IX86_UNKAMD },
  /*0x2D*/ { "???", IX86_UNKAMD },
  /*0x2E*/ { "???", IX86_UNKAMD },
  /*0x2F*/ { "???", IX86_UNKAMD },
  /*0x30*/ { "???", IX86_UNKAMD },
  /*0x31*/ { "???", IX86_UNKAMD },
  /*0x32*/ { "???", IX86_UNKAMD },
  /*0x33*/ { "???", IX86_UNKAMD },
  /*0x34*/ { "???", IX86_UNKAMD },
  /*0x35*/ { "???", IX86_UNKAMD },
  /*0x36*/ { "???", IX86_UNKAMD },
  /*0x37*/ { "???", IX86_UNKAMD },
  /*0x38*/ { "???", IX86_UNKAMD },
  /*0x39*/ { "???", IX86_UNKAMD },
  /*0x3A*/ { "???", IX86_UNKAMD },
  /*0x3B*/ { "???", IX86_UNKAMD },
  /*0x3C*/ { "???", IX86_UNKAMD },
  /*0x3D*/ { "???", IX86_UNKAMD },
  /*0x3E*/ { "???", IX86_UNKAMD },
  /*0x3F*/ { "???", IX86_UNKAMD },
  /*0x40*/ { "???", IX86_UNKAMD },
  /*0x41*/ { "???", IX86_UNKAMD },
  /*0x42*/ { "???", IX86_UNKAMD },
  /*0x43*/ { "???", IX86_UNKAMD },
  /*0x44*/ { "???", IX86_UNKAMD },
  /*0x45*/ { "???", IX86_UNKAMD },
  /*0x46*/ { "???", IX86_UNKAMD },
  /*0x47*/ { "???", IX86_UNKAMD },
  /*0x48*/ { "???", IX86_UNKAMD },
  /*0x49*/ { "???", IX86_UNKAMD },
  /*0x4A*/ { "???", IX86_UNKAMD },
  /*0x4B*/ { "???", IX86_UNKAMD },
  /*0x4C*/ { "???", IX86_UNKAMD },
  /*0x4D*/ { "???", IX86_UNKAMD },
  /*0x4E*/ { "???", IX86_UNKAMD },
  /*0x4F*/ { "???", IX86_UNKAMD },
  /*0x50*/ { "???", IX86_UNKAMD },
  /*0x51*/ { "???", IX86_UNKAMD },
  /*0x52*/ { "???", IX86_UNKAMD },
  /*0x53*/ { "???", IX86_UNKAMD },
  /*0x54*/ { "???", IX86_UNKAMD },
  /*0x55*/ { "???", IX86_UNKAMD },
  /*0x56*/ { "???", IX86_UNKAMD },
  /*0x57*/ { "???", IX86_UNKAMD },
  /*0x58*/ { "???", IX86_UNKAMD },
  /*0x59*/ { "???", IX86_UNKAMD },
  /*0x5A*/ { "???", IX86_UNKAMD },
  /*0x5B*/ { "???", IX86_UNKAMD },
  /*0x5C*/ { "???", IX86_UNKAMD },
  /*0x5D*/ { "???", IX86_UNKAMD },
  /*0x5E*/ { "???", IX86_UNKAMD },
  /*0x5F*/ { "???", IX86_UNKAMD },
  /*0x60*/ { "???", IX86_UNKAMD },
  /*0x61*/ { "???", IX86_UNKAMD },
  /*0x62*/ { "???", IX86_UNKAMD },
  /*0x63*/ { "???", IX86_UNKAMD },
  /*0x64*/ { "???", IX86_UNKAMD },
  /*0x65*/ { "???", IX86_UNKAMD },
  /*0x66*/ { "???", IX86_UNKAMD },
  /*0x67*/ { "???", IX86_UNKAMD },
  /*0x68*/ { "???", IX86_UNKAMD },
  /*0x69*/ { "???", IX86_UNKAMD },
  /*0x6A*/ { "???", IX86_UNKAMD },
  /*0x6B*/ { "???", IX86_UNKAMD },
  /*0x6C*/ { "???", IX86_UNKAMD },
  /*0x6D*/ { "???", IX86_UNKAMD },
  /*0x6E*/ { "???", IX86_UNKAMD },
  /*0x6F*/ { "???", IX86_UNKAMD },
  /*0x70*/ { "???", IX86_UNKAMD },
  /*0x71*/ { "???", IX86_UNKAMD },
  /*0x72*/ { "???", IX86_UNKAMD },
  /*0x73*/ { "???", IX86_UNKAMD },
  /*0x74*/ { "???", IX86_UNKAMD },
  /*0x75*/ { "???", IX86_UNKAMD },
  /*0x76*/ { "???", IX86_UNKAMD },
  /*0x77*/ { "???", IX86_UNKAMD },
  /*0x78*/ { "???", IX86_UNKAMD },
  /*0x79*/ { "???", IX86_UNKAMD },
  /*0x7A*/ { "???", IX86_UNKAMD },
  /*0x7B*/ { "???", IX86_UNKAMD },
  /*0x7C*/ { "???", IX86_UNKAMD },
  /*0x7D*/ { "???", IX86_UNKAMD },
  /*0x7E*/ { "???", IX86_UNKAMD },
  /*0x7F*/ { "???", IX86_UNKAMD },
  /*0x80*/ { "???", IX86_UNKAMD },
  /*0x81*/ { "???", IX86_UNKAMD },
  /*0x82*/ { "???", IX86_UNKAMD },
  /*0x83*/ { "???", IX86_UNKAMD },
  /*0x84*/ { "???", IX86_UNKAMD },
  /*0x85*/ { "???", IX86_UNKAMD },
  /*0x86*/ { "???", IX86_UNKAMD },
  /*0x87*/ { "???", IX86_UNKAMD },
  /*0x88*/ { "???", IX86_UNKAMD },
  /*0x89*/ { "???", IX86_UNKAMD },
  /*0x8A*/ { "pfnacc", IX86_ATHLON },
  /*0x8B*/ { "???", IX86_UNKAMD },
  /*0x8C*/ { "???", IX86_UNKAMD },
  /*0x8D*/ { "???", IX86_UNKAMD },
  /*0x8E*/ { "pfpnacc", IX86_ATHLON },
  /*0x8F*/ { "???", IX86_UNKAMD },
  /*0x90*/ { "pfcmpge", IX86_3DNOW },
  /*0x91*/ { "???", IX86_UNKAMD },
  /*0x92*/ { "???", IX86_UNKAMD },
  /*0x93*/ { "???", IX86_UNKAMD },
  /*0x94*/ { "pfmin", IX86_3DNOW },
  /*0x95*/ { "???", IX86_UNKAMD },
  /*0x96*/ { "pfrcp", IX86_3DNOW },
  /*0x97*/ { "pfrsqrt", IX86_3DNOW },
  /*0x98*/ { "???", IX86_UNKAMD },
  /*0x99*/ { "???", IX86_UNKAMD },
  /*0x9A*/ { "pfsub", IX86_3DNOW },
  /*0x9B*/ { "???", IX86_UNKAMD },
  /*0x9C*/ { "???", IX86_UNKAMD },
  /*0x9D*/ { "???", IX86_UNKAMD },
  /*0x9E*/ { "pfadd", IX86_3DNOW },
  /*0x9F*/ { "???", IX86_UNKAMD },
  /*0xA0*/ { "pfcmpgt", IX86_3DNOW },
  /*0xA1*/ { "???", IX86_UNKAMD },
  /*0xA2*/ { "???", IX86_UNKAMD },
  /*0xA3*/ { "???", IX86_UNKAMD },
  /*0xA4*/ { "pfmax", IX86_3DNOW },
  /*0xA5*/ { "???", IX86_UNKAMD },
  /*0xA6*/ { "pfrcpit1", IX86_3DNOW },
  /*0xA7*/ { "pfrsqit1", IX86_3DNOW },
  /*0xA8*/ { "???", IX86_UNKAMD },
  /*0xA9*/ { "???", IX86_UNKAMD },
  /*0xAA*/ { "pfsubr", IX86_3DNOW },
  /*0xAB*/ { "???", IX86_UNKAMD },
  /*0xAC*/ { "???", IX86_UNKAMD },
  /*0xAD*/ { "???", IX86_UNKAMD },
  /*0xAE*/ { "pfacc", IX86_3DNOW },
  /*0xAF*/ { "???", IX86_UNKAMD },
  /*0xB0*/ { "pfcmpeq", IX86_3DNOW },
  /*0xB1*/ { "???", IX86_UNKAMD },
  /*0xB2*/ { "???", IX86_UNKAMD },
  /*0xB3*/ { "???", IX86_UNKAMD },
  /*0xB4*/ { "pfmul", IX86_3DNOW },
  /*0xB5*/ { "???", IX86_UNKAMD },
  /*0xB6*/ { "pfrcpit2", IX86_3DNOW },
  /*0xB7*/ { "pmulhrwa", IX86_3DNOW },
  /*0xB8*/ { "???", IX86_UNKAMD },
  /*0xB9*/ { "???", IX86_UNKAMD },
  /*0xBA*/ { "???", IX86_UNKAMD },
  /*0xBB*/ { "pswapd", IX86_ATHLON },
  /*0xBC*/ { "???", IX86_UNKAMD },
  /*0xBD*/ { "???", IX86_UNKAMD },
  /*0xBE*/ { "???", IX86_UNKAMD },
  /*0xBF*/ { "pavgusb", IX86_3DNOW },
  /*0xC0*/ { "???", IX86_UNKAMD },
  /*0xC1*/ { "???", IX86_UNKAMD },
  /*0xC2*/ { "???", IX86_UNKAMD },
  /*0xC3*/ { "???", IX86_UNKAMD },
  /*0xC4*/ { "???", IX86_UNKAMD },
  /*0xC5*/ { "???", IX86_UNKAMD },
  /*0xC6*/ { "???", IX86_UNKAMD },
  /*0xC7*/ { "???", IX86_UNKAMD },
  /*0xC8*/ { "???", IX86_UNKAMD },
  /*0xC9*/ { "???", IX86_UNKAMD },
  /*0xCA*/ { "???", IX86_UNKAMD },
  /*0xCB*/ { "???", IX86_UNKAMD },
  /*0xCC*/ { "???", IX86_UNKAMD },
  /*0xCD*/ { "???", IX86_UNKAMD },
  /*0xCE*/ { "???", IX86_UNKAMD },
  /*0xCF*/ { "???", IX86_UNKAMD },
  /*0xD0*/ { "???", IX86_UNKAMD },
  /*0xD1*/ { "???", IX86_UNKAMD },
  /*0xD2*/ { "???", IX86_UNKAMD },
  /*0xD3*/ { "???", IX86_UNKAMD },
  /*0xD4*/ { "???", IX86_UNKAMD },
  /*0xD5*/ { "???", IX86_UNKAMD },
  /*0xD6*/ { "???", IX86_UNKAMD },
  /*0xD7*/ { "???", IX86_UNKAMD },
  /*0xD8*/ { "???", IX86_UNKAMD },
  /*0xD9*/ { "???", IX86_UNKAMD },
  /*0xDA*/ { "???", IX86_UNKAMD },
  /*0xDB*/ { "???", IX86_UNKAMD },
  /*0xDC*/ { "???", IX86_UNKAMD },
  /*0xDD*/ { "???", IX86_UNKAMD },
  /*0xDE*/ { "???", IX86_UNKAMD },
  /*0xDF*/ { "???", IX86_UNKAMD },
  /*0xE0*/ { "???", IX86_UNKAMD },
  /*0xE1*/ { "???", IX86_UNKAMD },
  /*0xE2*/ { "???", IX86_UNKAMD },
  /*0xE3*/ { "???", IX86_UNKAMD },
  /*0xE4*/ { "???", IX86_UNKAMD },
  /*0xE5*/ { "???", IX86_UNKAMD },
  /*0xE6*/ { "???", IX86_UNKAMD },
  /*0xE7*/ { "???", IX86_UNKAMD },
  /*0xE8*/ { "???", IX86_UNKAMD },
  /*0xE9*/ { "???", IX86_UNKAMD },
  /*0xEA*/ { "???", IX86_UNKAMD },
  /*0xEB*/ { "???", IX86_UNKAMD },
  /*0xEC*/ { "???", IX86_UNKAMD },
  /*0xED*/ { "???", IX86_UNKAMD },
  /*0xEE*/ { "???", IX86_UNKAMD },
  /*0xEF*/ { "???", IX86_UNKAMD },
  /*0xF0*/ { "???", IX86_UNKAMD },
  /*0xF1*/ { "???", IX86_UNKAMD },
  /*0xF2*/ { "???", IX86_UNKAMD },
  /*0xF3*/ { "???", IX86_UNKAMD },
  /*0xF4*/ { "???", IX86_UNKAMD },
  /*0xF5*/ { "???", IX86_UNKAMD },
  /*0xF6*/ { "???", IX86_UNKAMD },
  /*0xF7*/ { "???", IX86_UNKAMD },
  /*0xF8*/ { "???", IX86_UNKAMD },
  /*0xF9*/ { "???", IX86_UNKAMD },
  /*0xFA*/ { "???", IX86_UNKAMD },
  /*0xFB*/ { "???", IX86_UNKAMD },
  /*0xFC*/ { "???", IX86_UNKAMD },
  /*0xFD*/ { "???", IX86_UNKAMD },
  /*0xFE*/ { "???", IX86_UNKAMD },
  /*0xFF*/ { "???", IX86_UNKAMD }
};

#ifdef IX86_64
#define DECLARE_EX_INSN(n32, n64, func, func64, flags, flags64)\
{ n32, n64, func, func64, flags64, flags }
#else
#define DECLARE_EX_INSN(n32, n64, func, func64, flags, flags64)\
{ n32, func, flags }
#endif

/*
  note: first column describes XOP_mmmm=08
	second column describes XOP_mmmm=09
*/

const ix86_ExOpcodes K64_XOP_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x01*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x02*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x03*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x04*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x05*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x06*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x07*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x08*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x09*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x0A*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x0B*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x0C*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x0D*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x0E*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x0F*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x10*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x11*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x12*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x13*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x14*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x15*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x16*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x17*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x18*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x19*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x1A*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x1B*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x1C*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x1D*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x1E*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x1F*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x20*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x21*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x22*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x23*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x24*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x25*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x26*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x27*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x28*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x29*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x2A*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x2B*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x2C*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x2D*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x2E*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x2F*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x30*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x31*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x32*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x33*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x34*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x35*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x36*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x37*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x38*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x39*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x3A*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x3B*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x3C*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x3D*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x3E*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x3F*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x40*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x41*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x42*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x43*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x44*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x45*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x46*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x47*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x48*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x49*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x4A*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x4B*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x4C*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x4D*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x4E*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x4F*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x50*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x51*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x52*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x53*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x54*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x55*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x56*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x57*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x58*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x59*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x5A*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x5B*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x5C*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x5D*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x5E*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x5F*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x60*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x61*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x62*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x63*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x64*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x65*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x66*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x67*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x68*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x69*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x6A*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x6B*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x6C*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x6D*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x6E*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x6F*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x70*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x71*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x72*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x73*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x74*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x75*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x76*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x77*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x78*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x79*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x7A*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x7B*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x7C*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x7D*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x7E*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x7F*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x80*/ DECLARE_EX_INSN("vfrczps", "vfrczps", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0x81*/ DECLARE_EX_INSN("vfrczpd", "vfrczpd", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0x82*/ DECLARE_EX_INSN("vfrczss", "vfrczss", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0x83*/ DECLARE_EX_INSN("vfrczsd", "vfrczsd", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0x84*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x85*/ DECLARE_EX_INSN("vpmacssww", "vpmacssww", arg_fma4, arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0x86*/ DECLARE_EX_INSN("vpmacsswd", "vpmacsswd", arg_fma4, arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0x87*/ DECLARE_EX_INSN("vpmacssdql", "vpmacssdql", arg_fma4, arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0x88*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x89*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x8A*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x8B*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x8C*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x8D*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x8E*/ DECLARE_EX_INSN("vpmacssdd", "vpmacssdd", arg_fma4, arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0x8F*/ DECLARE_EX_INSN("vpmacssdqh", "vpmacssdqh", arg_fma4, arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0x90*/ DECLARE_EX_INSN("vprotb", "vprotb", arg_simd, arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x91*/ DECLARE_EX_INSN("vprotw", "vprotw", arg_simd, arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x92*/ DECLARE_EX_INSN("vprotd", "vprotd", arg_simd, arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x93*/ DECLARE_EX_INSN("vprotq", "vprotq", arg_simd, arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x94*/ DECLARE_EX_INSN("vpshlb", "vpshlb", arg_simd, arg_simd, K64_FAM11|INSN_VEX_V|INSN_SSE|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_VEX_V|INSN_SSE|INSN_VEXW_AS_SWAP),
  /*0x95*/ DECLARE_EX_INSN("vpmacsww", "vpshlw", arg_fma4, arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x96*/ DECLARE_EX_INSN("vpmacswd", "vpshld", arg_fma4, arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x97*/ DECLARE_EX_INSN("vpmacsdql","vpshlq", arg_fma4, arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x98*/ DECLARE_EX_INSN("vpshab", "vpshab", arg_simd, arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x99*/ DECLARE_EX_INSN("vpshaw", "vpshaw", arg_simd, arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x9A*/ DECLARE_EX_INSN("vpshad", "vpshad", arg_simd, arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x9B*/ DECLARE_EX_INSN("vpshaq", "vpshaq", arg_simd, arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x9C*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x9D*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0x9E*/ DECLARE_EX_INSN("vpmacsdd", "vpmacsdd", arg_fma4, arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0x9F*/ DECLARE_EX_INSN("vpmacsdqh", "vpmacsdqh", arg_fma4, arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xA0*/ DECLARE_EX_INSN("vcvtph2ps", "vcvtph2ps", arg_simd_imm8, arg_simd_imm8, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xA1*/ DECLARE_EX_INSN("vcvtps2ph", "vcvtps2ph", arg_simd_imm8, arg_simd_imm8, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xA2*/ DECLARE_EX_INSN("vpcmov", "vpcmov", arg_fma4, arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xA3*/ DECLARE_EX_INSN("vpperm", "vpperm", arg_fma4, arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xA4*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xA5*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xA6*/ DECLARE_EX_INSN("vpmadcsswd", "vpmadcsswd", arg_fma4, arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xA7*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xA8*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xA9*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xAA*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xAB*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xAC*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xAD*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xAE*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xAF*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xB0*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xB1*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xB2*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xB3*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xB4*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xB5*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xB6*/ DECLARE_EX_INSN("vpmadcswd", "vpmadcswd", arg_fma4, arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xB7*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xB8*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xB9*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xBA*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xBB*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xBC*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xBD*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xBE*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xBF*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xC0*/ DECLARE_EX_INSN("vprotb", "vprotb", arg_simd_imm8, arg_simd_imm8, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xC1*/ DECLARE_EX_INSN("vprotw", "vphaddbw", arg_simd_imm8, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xC2*/ DECLARE_EX_INSN("vprotd", "vphaddbd", arg_simd_imm8, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xC3*/ DECLARE_EX_INSN("vprotq", "vphaddbq", arg_simd_imm8, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xC4*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xC5*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xC6*/ DECLARE_EX_INSN("vphaddwd", "vphaddwd", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xC7*/ DECLARE_EX_INSN("vphaddwq", "vphaddwq", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xC8*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xC9*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xCA*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xCB*/ DECLARE_EX_INSN("vphadddq", "vphadddq", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xCC*/ DECLARE_EX_INSN("vpcomb", "vpcomb", arg_simd_imm8, arg_simd_imm8, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xCD*/ DECLARE_EX_INSN("vpcomw", "vpcomw", arg_simd_imm8, arg_simd_imm8, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xCE*/ DECLARE_EX_INSN("vpcomd", "vpcomd", arg_simd_imm8, arg_simd_imm8, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xCF*/ DECLARE_EX_INSN("vpcomq", "vpcomq", arg_simd_imm8, arg_simd_imm8, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xD0*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xD1*/ DECLARE_EX_INSN("vphaddubw", "vphaddubw", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xD2*/ DECLARE_EX_INSN("vphaddubd", "vphaddubd", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xD3*/ DECLARE_EX_INSN("vphaddubq", "vphaddubq", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xD4*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xD5*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xD6*/ DECLARE_EX_INSN("vphadduwd", "vphadduwd", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xD7*/ DECLARE_EX_INSN("vphadduwq", "vphadduwq", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xD8*/ DECLARE_EX_INSN("vphaddudq", "vphaddudq", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xD9*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xDA*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xDB*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xDC*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xDD*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xDE*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xDF*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xE0*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xE1*/ DECLARE_EX_INSN("vphsubbw", "vphsubbw", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xE2*/ DECLARE_EX_INSN("vphsubwd", "vphsubwd", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xE3*/ DECLARE_EX_INSN("vphsubdq", "vphsubdq", arg_simd, arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xE4*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xE5*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xE6*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xE7*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xE8*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xE9*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xEA*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xEB*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xEC*/ DECLARE_EX_INSN("vpcomub", "vpcomub", arg_simd_imm8, arg_simd_imm8, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xED*/ DECLARE_EX_INSN("vpcomuw", "vpcomuw", arg_simd_imm8, arg_simd_imm8, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xEE*/ DECLARE_EX_INSN("vpcomud", "vpcomud", arg_simd_imm8, arg_simd_imm8, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xEF*/ DECLARE_EX_INSN("vpcomuq", "vpcomuq", arg_simd_imm8, arg_simd_imm8, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xF0*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xF1*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xF2*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xF3*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xF4*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xF5*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xF6*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xF7*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xF8*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xF9*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xFA*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xFB*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xFC*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xFD*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xFE*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11),
  /*0xFF*/ DECLARE_EX_INSN("???", "???", NULL, NULL, K64_FAM11, K64_FAM11)
};

const ix86_ExOpcodes ix86_660F01_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN("vmclear", "vmclear", ix86_VMX, ix86_VMX, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_660F38_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN("pshufb", "pshufb",arg_simd,arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x01*/ DECLARE_EX_INSN("phaddw", "phaddw",arg_simd,arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x02*/ DECLARE_EX_INSN("phaddd", "phaddd",arg_simd,arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x03*/ DECLARE_EX_INSN("phaddsw", "phaddsw",arg_simd,arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x04*/ DECLARE_EX_INSN("pmaddubsw", "pmaddubsw",arg_simd,arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE),
  /*0x05*/ DECLARE_EX_INSN("phsubw", "phsubw",arg_simd,arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x06*/ DECLARE_EX_INSN("phsubd", "phsubd",arg_simd,arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x07*/ DECLARE_EX_INSN("phsubsw", "phsubsw",arg_simd,arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x08*/ DECLARE_EX_INSN("psingb", "psignb",arg_simd,arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x09*/ DECLARE_EX_INSN("psignw", "psignw",arg_simd,arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0A*/ DECLARE_EX_INSN("psignd", "psignd",arg_simd,arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0B*/ DECLARE_EX_INSN("pmulhrsw", "pmulhrsw",arg_simd,arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0C*/ DECLARE_EX_INSN("vpermilps", "vpermilps",arg_simd,arg_simd, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x0D*/ DECLARE_EX_INSN("vpermilpd", "vpermilpd",arg_simd,arg_simd, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x0E*/ DECLARE_EX_INSN("vtestps", "vtestps",arg_simd,arg_simd, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x0F*/ DECLARE_EX_INSN("vtestpd", "vtestpd",arg_simd,arg_simd, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x10*/ DECLARE_EX_INSN("pblendvb", "pblendvb",arg_simd_xmm0,arg_simd_xmm0, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN("blendvps", "blendvps",arg_simd_xmm0,arg_simd_xmm0, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x15*/ DECLARE_EX_INSN("blendvpd", "blendvpd",arg_simd_xmm0,arg_simd_xmm0, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN("ptest", "ptest",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x18*/ DECLARE_EX_INSN("vbroadcastss", "vbroadcastss", arg_simd, arg_simd, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x19*/ DECLARE_EX_INSN("vbroadcastsd", "vbroadcastsd", arg_simd, arg_simd, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x1A*/ DECLARE_EX_INSN("vbroadcastf128", "vbroadcastf128", arg_simd, arg_simd, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN("pabsb", "pabsb",arg_simd,arg_simd, IX86_P6|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x1D*/ DECLARE_EX_INSN("pabsw", "pabsw",arg_simd,arg_simd, IX86_P6|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x1E*/ DECLARE_EX_INSN("pabsd", "pabsd",arg_simd,arg_simd, IX86_P6|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN("pmovsxbw", "pmovsxbw",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x21*/ DECLARE_EX_INSN("pmovsxbd", "pmovsxbd",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x22*/ DECLARE_EX_INSN("pmovsxbq", "pmovsxbq",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x23*/ DECLARE_EX_INSN("pmovsxwd", "pmovsxwd",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x24*/ DECLARE_EX_INSN("pmovsxwq", "pmovsxwq",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x25*/ DECLARE_EX_INSN("pmovsxdq", "pmovsxdq",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN("pmuldq", "pmuldq",arg_simd,arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x29*/ DECLARE_EX_INSN("pcmpeqq", "pcmpeqq",arg_simd,arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x2A*/ DECLARE_EX_INSN("movntdqa", "movntdqa",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x2B*/ DECLARE_EX_INSN("packusdw", "packusdw",arg_simd,arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x2C*/ DECLARE_EX_INSN("vmaskmovps", "vmaskmovps",arg_simd,arg_simd, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x2D*/ DECLARE_EX_INSN("vmaskmovpd", "vmaskmovpd",arg_simd,arg_simd, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x2E*/ DECLARE_EX_INSN("vmaskmovps", "vmaskmovps",arg_simd,arg_simd, IX86_P8|INSN_AVX|INSN_VEX_V|INSN_STORE, INSN_AVX|INSN_VEX_V|INSN_STORE),
  /*0x2F*/ DECLARE_EX_INSN("vmaskmovpd", "vmaskmovpd",arg_simd,arg_simd, IX86_P8|INSN_AVX|INSN_VEX_V|INSN_STORE, INSN_AVX|INSN_VEX_V|INSN_STORE),
  /*0x30*/ DECLARE_EX_INSN("pmovzxbw", "pmovzxbw",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x31*/ DECLARE_EX_INSN("pmovzxbd", "pmovzxbd",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x32*/ DECLARE_EX_INSN("pmovzxbq", "pmovzxbq",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x33*/ DECLARE_EX_INSN("pmovzxwd", "pmovzxwd",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x34*/ DECLARE_EX_INSN("pmovzxwq", "pmovzxwq",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x35*/ DECLARE_EX_INSN("pmovzxdq", "pmovzxdq",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN("pcmpgtq","pcmpgtq",arg_simd,arg_simd,IX86_P7|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x38*/ DECLARE_EX_INSN("pminsb", "pminb",arg_simd,arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x39*/ DECLARE_EX_INSN("pminsd", "pminsd",arg_simd,arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x3A*/ DECLARE_EX_INSN("pminuw", "pminuw",arg_simd,arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x3B*/ DECLARE_EX_INSN("pminud", "pminud",arg_simd,arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x3C*/ DECLARE_EX_INSN("pmaxsb", "pmaxsb",arg_simd,arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x3D*/ DECLARE_EX_INSN("pmaxsd", "pmaxsd",arg_simd,arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x3E*/ DECLARE_EX_INSN("pmaxuw", "pmaxuw",arg_simd,arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x3F*/ DECLARE_EX_INSN("pmaxud", "pmaxud",arg_simd,arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x40*/ DECLARE_EX_INSN("pmulld", "pmulld",arg_simd,arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x41*/ DECLARE_EX_INSN("phminposuw", "phminposuw",arg_simd,arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN("invept", "invept", arg_cpu_modregrm, arg_cpu_modregrm, IX86_P7, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN("invvpid", "invvpid", arg_cpu_modregrm, arg_cpu_modregrm, IX86_P7, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN("vfmaddsub132pd", "vfmaddsub132pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x97*/ DECLARE_EX_INSN("vfmsubadd132pd", "vfmsubadd132pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x98*/ DECLARE_EX_INSN("vfmadd132pd", "vfmadd132pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x99*/ DECLARE_EX_INSN("vfmadd132sd", "vfmadd132sd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x9A*/ DECLARE_EX_INSN("vfmsub132pd", "vfmsub132pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x9B*/ DECLARE_EX_INSN("vfmsub132sd", "vfmsub132sd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x9C*/ DECLARE_EX_INSN("vfnmadd132pd", "vfnmadd132pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x9D*/ DECLARE_EX_INSN("vfnmadd132sd", "vfnmadd132sd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x9E*/ DECLARE_EX_INSN("vfnmsub132pd", "vfnmsub132pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x9F*/ DECLARE_EX_INSN("vfnmsub132sd", "vfnmsub132sd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN("vfmaddsub213pd", "vfmaddsub213pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xA7*/ DECLARE_EX_INSN("vfmsubadd213pd", "vfmsubadd213pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xA8*/ DECLARE_EX_INSN("vfmadd213pd", "vfmadd213pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xA9*/ DECLARE_EX_INSN("vfmadd213sd", "vfmadd213sd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xAA*/ DECLARE_EX_INSN("vfmsub213pd", "vfmsub213pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xAB*/ DECLARE_EX_INSN("vfmsub213sd", "vfmsub213sd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xAC*/ DECLARE_EX_INSN("vfnmadd213pd", "vfnmadd213pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xAD*/ DECLARE_EX_INSN("vfnmadd213sd", "vfnmadd213sd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xAE*/ DECLARE_EX_INSN("vfnmsub213pd", "vfnmsub213pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xAF*/ DECLARE_EX_INSN("vfnmsub213sd", "vfnmsub213sd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN("vfmaddsub231pd", "vfmaddsub231pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xB7*/ DECLARE_EX_INSN("vfmsubadd231pd", "vfmsubadd231pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xB8*/ DECLARE_EX_INSN("vfmadd231pd", "vfmadd231pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xB9*/ DECLARE_EX_INSN("vfmadd231sd", "vfmadd231sd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xBA*/ DECLARE_EX_INSN("vfmsub231pd", "vfmsub231pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xBB*/ DECLARE_EX_INSN("vfmsub231sd", "vfmsub231sd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xBC*/ DECLARE_EX_INSN("vfnmadd231pd", "vfnmadd231pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xBD*/ DECLARE_EX_INSN("vfnmadd231sd", "vfnmadd231sd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xBE*/ DECLARE_EX_INSN("vfnmsub231pd", "vfnmsub231pd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xBF*/ DECLARE_EX_INSN("vfnmsub231sd", "vfnmsub231sd",arg_fma,arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN("aesimc", "aesimc", arg_simd,arg_simd, IX86_P8|INSN_SSE|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xDC*/ DECLARE_EX_INSN("aesenc", "aesenc", arg_simd,arg_simd, IX86_P8|INSN_SSE|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xDD*/ DECLARE_EX_INSN("aesenclast", "aesenclast", arg_simd,arg_simd, IX86_P8|INSN_SSE|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xDE*/ DECLARE_EX_INSN("aesdec", "aesdec", arg_simd,arg_simd, IX86_P8|INSN_SSE|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xDF*/ DECLARE_EX_INSN("aesdeclast", "aesdeclast", arg_simd,arg_simd, IX86_P8|INSN_SSE|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_660F3A_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN("vpermilps", "vpermilps",arg_simd_imm8,arg_simd_imm8, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x05*/ DECLARE_EX_INSN("vpermilpd", "vpermilpd",arg_simd_imm8,arg_simd_imm8, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x06*/ DECLARE_EX_INSN("vperm2f128", "vperm2f128",arg_simd_imm8,arg_simd_imm8, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN("roundps", "roundps",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x09*/ DECLARE_EX_INSN("roundpd", "roundpd",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x0A*/ DECLARE_EX_INSN("roundss", "roundss",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0B*/ DECLARE_EX_INSN("roundsd", "roundsd",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0C*/ DECLARE_EX_INSN("blendps", "blendps",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0D*/ DECLARE_EX_INSN("blendpd", "blendpd",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0E*/ DECLARE_EX_INSN("pblendw", "pblendw",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0F*/ DECLARE_EX_INSN("palignr", "palignr",arg_simd_imm8,arg_simd_imm8, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN("pextrb", "pextrb",bridge_simd_cpu_imm8,bridge_simd_cpu_imm8, IX86_P7|INSN_SSE|INSN_STORE, K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x15*/ DECLARE_EX_INSN("pextrw", "pextrw",bridge_simd_cpu_imm8,bridge_simd_cpu_imm8, IX86_P7|INSN_SSE|INSN_STORE, K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x16*/ DECLARE_EX_INSN("pextrd", "pextrq",bridge_simd_cpu_imm8,bridge_simd_cpu_imm8, IX86_P7|INSN_SSE|INSN_STORE, K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x17*/ DECLARE_EX_INSN("extractps", "extractps",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x18*/ DECLARE_EX_INSN("vinsertf128", "vinsertf128",arg_simd_imm8,arg_simd_imm8, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x19*/ DECLARE_EX_INSN("vextractf128", "vextractf128",arg_simd_imm8,arg_simd_imm8, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN("pinsrb", "pinsrb",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x21*/ DECLARE_EX_INSN("insertps", "insertps",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x22*/ DECLARE_EX_INSN("pinsrd", "pinsrq",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN("dpps", "dpps",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x41*/ DECLARE_EX_INSN("dppd", "dppd",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x42*/ DECLARE_EX_INSN("mpsadbw", "mpsadbw",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN("pclmulqdq", "pclmulqdq",arg_simd_imm8,arg_simd_imm8, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN("blendvps", "blendvps", arg_fma4,arg_fma4, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x4B*/ DECLARE_EX_INSN("blendvpd", "blendvpd", arg_fma4,arg_fma4, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x4C*/ DECLARE_EX_INSN("vpblendvb", "vpblendvb", arg_fma4,arg_fma4, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN("vfmaddsubps", "vfmaddsubps", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x5D*/ DECLARE_EX_INSN("vfmaddsubpd", "vfmaddsubpd", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x5E*/ DECLARE_EX_INSN("vfmsubaddps", "vfmsubaddps", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x5F*/ DECLARE_EX_INSN("vfmsubaddpd", "vfmsubaddpd", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x60*/ DECLARE_EX_INSN("pcmpestrm", "pcmpestrm",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x61*/ DECLARE_EX_INSN("pcmpestri", "pcmpestri",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x62*/ DECLARE_EX_INSN("pcmpistrm", "pcmpistrm",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x63*/ DECLARE_EX_INSN("pcmpistri", "pcmpistri",arg_simd_imm8,arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN("vfmaddps", "vfmaddps", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x69*/ DECLARE_EX_INSN("vfmaddpd", "vfmaddpd", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x6A*/ DECLARE_EX_INSN("vfmaddss", "vfmaddss", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x6B*/ DECLARE_EX_INSN("vfmaddsd", "vfmaddsd", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x6C*/ DECLARE_EX_INSN("vfmsubps", "vfmsubps", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x6D*/ DECLARE_EX_INSN("vfmsubpd", "vfmsubpd", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x6E*/ DECLARE_EX_INSN("vfmsubss", "vfmsubss", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x6F*/ DECLARE_EX_INSN("vfmsubsd", "vfmsubsd", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN("vfnmaddps", "vfnmaddps", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x79*/ DECLARE_EX_INSN("vfnmaddpd", "vfnmaddpd", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x7A*/ DECLARE_EX_INSN("vfnmaddss", "vfnmaddss", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x7B*/ DECLARE_EX_INSN("vfnmaddsd", "vfnmaddsd", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x7C*/ DECLARE_EX_INSN("vfnmsubps", "vfnmsubps", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x7D*/ DECLARE_EX_INSN("vfnmsubpd", "vfnmsubpd", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x7E*/ DECLARE_EX_INSN("vfnmsubss", "vfnmsubss", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x7F*/ DECLARE_EX_INSN("vfnmsubsd", "vfnmsubsd", arg_fma4, arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN("aeskeygenassist", "aeskeygenassist", bridge_simd_cpu_imm8,bridge_simd_cpu_imm8, IX86_P8|INSN_SSE|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_660F_PentiumTable[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x01*/ DECLARE_EX_INSN(ix86_660F01_Table,ix86_660F01_Table,NULL,NULL,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x02*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x03*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x04*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x05*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x06*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x07*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x08*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x09*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x10*/ DECLARE_EX_INSN("movupd","movupd",arg_simd,arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x11*/ DECLARE_EX_INSN("movupd","movupd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x12*/ DECLARE_EX_INSN("movlpd","movlpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x13*/ DECLARE_EX_INSN("movlpd","movlpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x14*/ DECLARE_EX_INSN("unpcklpd","unpcklpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x15*/ DECLARE_EX_INSN("unpckhpd","unpckhpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x16*/ DECLARE_EX_INSN("movhpd","movhpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x17*/ DECLARE_EX_INSN("movhpd","movhpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x18*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x19*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x20*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x21*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x22*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x23*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x24*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x25*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x26*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x27*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x28*/ DECLARE_EX_INSN("movapd","movapd",arg_simd,arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x29*/ DECLARE_EX_INSN("movapd","movapd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x2A*/ DECLARE_EX_INSN("cvtpi2pd","cvtpi2pd",bridge_sse_mmx,bridge_sse_mmx,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x2B*/ DECLARE_EX_INSN("movntpd","movntpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x2C*/ DECLARE_EX_INSN("cvttpd2pi","cvttpd2pi",bridge_sse_mmx,bridge_sse_mmx,IX86_P4|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE),
  /*0x2D*/ DECLARE_EX_INSN("cvtpd2pi","cvtpd2pi",bridge_sse_mmx,bridge_sse_mmx,IX86_P4|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE),
  /*0x2E*/ DECLARE_EX_INSN("ucomisd","ucomisd",arg_simd,arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x2F*/ DECLARE_EX_INSN("comisd","comisd",arg_simd,arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x30*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x31*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x32*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x33*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x34*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x35*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x36*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x37*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x38*/ DECLARE_EX_INSN(ix86_660F38_Table,ix86_660F38_Table,NULL,NULL,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x39*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3A*/ DECLARE_EX_INSN(ix86_660F3A_Table,ix86_660F3A_Table,NULL,NULL,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x3B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x40*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x41*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x42*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x43*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x44*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x45*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x46*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x47*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x48*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x49*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x50*/ DECLARE_EX_INSN("movmskpd","movmskpd",bridge_simd_cpu,bridge_simd_cpu,IX86_P4|INSN_SSE|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|BRIDGE_CPU_SSE),
  /*0x51*/ DECLARE_EX_INSN("sqrtpd","sqrtpd",arg_simd,arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x52*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x53*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x54*/ DECLARE_EX_INSN("andpd","andpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x55*/ DECLARE_EX_INSN("andnpd","andnpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x56*/ DECLARE_EX_INSN("orpd","orpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x57*/ DECLARE_EX_INSN("xorpd","xorpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x58*/ DECLARE_EX_INSN("addpd","addpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x59*/ DECLARE_EX_INSN("mulpd","mulpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5A*/ DECLARE_EX_INSN("cvtpd2ps","cvtpd2ps",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5B*/ DECLARE_EX_INSN("cvtps2dq","cvtps2dq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5C*/ DECLARE_EX_INSN("subpd","subpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5D*/ DECLARE_EX_INSN("minpd","minpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5E*/ DECLARE_EX_INSN("divpd","divpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5F*/ DECLARE_EX_INSN("maxpd","maxpd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x60*/ DECLARE_EX_INSN("punpcklbw","punpcklbw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x61*/ DECLARE_EX_INSN("punpcklwd","punpcklwd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x62*/ DECLARE_EX_INSN("punpckldq","punpckldq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x63*/ DECLARE_EX_INSN("packsswb","packsswb",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x64*/ DECLARE_EX_INSN("pcmpgtb","pcmpgtb",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x65*/ DECLARE_EX_INSN("pcmpgtw","pcmpgtw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x66*/ DECLARE_EX_INSN("pcmpgtd","pcmpgtd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x67*/ DECLARE_EX_INSN("packuswb","packuswb",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x68*/ DECLARE_EX_INSN("punpckhbw","punpckhbw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x69*/ DECLARE_EX_INSN("punpckhwd","punpckhwd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x6A*/ DECLARE_EX_INSN("punpckhdq","punpckhdq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x6B*/ DECLARE_EX_INSN("packssdw","packssdw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x6C*/ DECLARE_EX_INSN("punpcklqdq","punpcklqdq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x6D*/ DECLARE_EX_INSN("punpckhqdq","punpckhqdq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x6E*/ DECLARE_EX_INSN("movd","movd",bridge_simd_cpu,bridge_simd_cpu,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x6F*/ DECLARE_EX_INSN("movdqa","movdqa",arg_simd,arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x70*/ DECLARE_EX_INSN("pshufd","pshufd",arg_simd_imm8,arg_simd_imm8,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x71*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgXMMXGr1,ix86_ArgXMMXGr1,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x72*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgXMMXGr2,ix86_ArgXMMXGr2,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x73*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgXMMXGr3,ix86_ArgXMMXGr3,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x74*/ DECLARE_EX_INSN("pcmpeqb","pcmpeqb",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x75*/ DECLARE_EX_INSN("pcmpeqw","pcmpeqw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x76*/ DECLARE_EX_INSN("pcmpeqd","pcmpeqd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x77*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x78*/ DECLARE_EX_INSN("extrq","extrq",arg_simd_rm_imm8_imm8,arg_simd_rm_imm8_imm8,IX86_P7|INSN_SSE,K64_FAM10|INSN_SSE),
  /*0x79*/ DECLARE_EX_INSN("extrq","extrq",arg_simd_regrm,arg_simd_regrm,IX86_P7|INSN_SSE,K64_FAM10|INSN_SSE),
  /*0x7A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7C*/ DECLARE_EX_INSN("haddpd","haddpd",arg_simd,arg_simd,IX86_P5|INSN_SSE|INSN_VEX_V,K64_FAM9|INSN_SSE|INSN_VEX_V),
  /*0x7D*/ DECLARE_EX_INSN("hsubpd","hsubpd",arg_simd,arg_simd,IX86_P5|INSN_SSE|INSN_VEX_V,K64_FAM9|INSN_SSE|INSN_VEX_V),
  /*0x7E*/ DECLARE_EX_INSN("movd","movd",bridge_simd_cpu,bridge_simd_cpu,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x7F*/ DECLARE_EX_INSN("movdqa","movdqa",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x80*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x81*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x82*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x83*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x84*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x85*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x86*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x87*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x88*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x89*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x90*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x91*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x92*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x93*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x94*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x95*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x96*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x97*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x98*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x99*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC2*/ DECLARE_EX_INSN("cmppd","cmppd",ix86_ArgXMMCmp,ix86_ArgXMMCmp,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC4*/ DECLARE_EX_INSN("pinsrw","pinsrw",bridge_simd_cpu_imm8,bridge_simd_cpu_imm8,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC5*/ DECLARE_EX_INSN("pextrw","pextrw",bridge_simd_cpu_imm8,bridge_simd_cpu_imm8,IX86_P4|INSN_SSE|BRIDGE_CPU_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE|BRIDGE_CPU_SSE),
  /*0xC6*/ DECLARE_EX_INSN("shufpd","shufpd",arg_simd_imm8,arg_simd_imm8,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC7*/ DECLARE_EX_INSN("!!!","!!!",ix86_660FVMX,ix86_660FVMX,IX86_CPU586,K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD0*/ DECLARE_EX_INSN("addsubpd","addsubpd",arg_simd,arg_simd,IX86_P5|INSN_SSE|INSN_VEX_V,K64_FAM9|INSN_SSE|INSN_VEX_V),
  /*0xD1*/ DECLARE_EX_INSN("psrlw","psrlw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD2*/ DECLARE_EX_INSN("psrld","psrld",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD3*/ DECLARE_EX_INSN("psrlq","psrlq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD4*/ DECLARE_EX_INSN("paddq","paddq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD5*/ DECLARE_EX_INSN("pmullw","pmullw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD6*/ DECLARE_EX_INSN("movq","movq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0xD7*/ DECLARE_EX_INSN("pmovmskb","pmovmskb",bridge_simd_cpu,bridge_simd_cpu,IX86_P4|INSN_SSE|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|BRIDGE_CPU_SSE),
  /*0xD8*/ DECLARE_EX_INSN("psubusb","psubusb",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD9*/ DECLARE_EX_INSN("psubusw","psubusw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xDA*/ DECLARE_EX_INSN("pminub","pminub",arg_simd,arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0xDB*/ DECLARE_EX_INSN("pand","pand",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xDC*/ DECLARE_EX_INSN("paddusb","paddusb",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xDD*/ DECLARE_EX_INSN("paddusw","paddusw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xDE*/ DECLARE_EX_INSN("pmaxub","pmaxub",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xDF*/ DECLARE_EX_INSN("pandn","pandn",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE0*/ DECLARE_EX_INSN("pavgb","pavgb",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE1*/ DECLARE_EX_INSN("psraw","psraw",arg_simd,arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0xE2*/ DECLARE_EX_INSN("psrad","psrad",arg_simd,arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0xE3*/ DECLARE_EX_INSN("pavgw","pavgw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE4*/ DECLARE_EX_INSN("pmulhuw","pmulhuw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE5*/ DECLARE_EX_INSN("pmulhw","pmulhw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE6*/ DECLARE_EX_INSN("cvttpd2dq","cvttpd2dq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE7*/ DECLARE_EX_INSN("movntdq","movntdq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_STORE),
  /*0xE8*/ DECLARE_EX_INSN("psubsb","psubsb",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE9*/ DECLARE_EX_INSN("psubsw","psubsw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xEA*/ DECLARE_EX_INSN("pminsw","pminsw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xEB*/ DECLARE_EX_INSN("por","por",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xEC*/ DECLARE_EX_INSN("paddsb","paddsb",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xED*/ DECLARE_EX_INSN("paddsw","paddsw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xEE*/ DECLARE_EX_INSN("pmaxsw","pmaxsw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xEF*/ DECLARE_EX_INSN("pxor","pxor",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF1*/ DECLARE_EX_INSN("psllw","psllw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF2*/ DECLARE_EX_INSN("pslld","pslld",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF3*/ DECLARE_EX_INSN("psllq","psllq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF4*/ DECLARE_EX_INSN("pmuludq","pmuludq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF5*/ DECLARE_EX_INSN("pmaddwd","pmaddwd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF6*/ DECLARE_EX_INSN("psadbw","psadbw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF7*/ DECLARE_EX_INSN("maskmovdqu","maskmovdqu",arg_simd,arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0xF8*/ DECLARE_EX_INSN("psubb","psubb",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF9*/ DECLARE_EX_INSN("psubw","psubw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xFA*/ DECLARE_EX_INSN("psubd","psubd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xFB*/ DECLARE_EX_INSN("psubq","psubq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xFC*/ DECLARE_EX_INSN("paddb","paddb",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xFD*/ DECLARE_EX_INSN("paddw","paddw",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xFE*/ DECLARE_EX_INSN("paddd","paddd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xFF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE)
};

const ix86_ExOpcodes ix86_F20F38_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN("crc32", "crc32",arg_cpu_modREGrm,arg_cpu_modREGrm, IX86_P7|INSN_OP_BYTE, K64_ATHLON|INSN_OP_BYTE),
  /*0xF1*/ DECLARE_EX_INSN("crc32", "crc32",arg_cpu_modREGrm,arg_cpu_modREGrm, IX86_P7, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_F20F_PentiumTable[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x01*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x02*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x03*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x04*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x05*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x06*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x07*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x08*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x09*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x10*/ DECLARE_EX_INSN("movsd","movsd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x11*/ DECLARE_EX_INSN("movsd","movsd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_STORE),
  /*0x12*/ DECLARE_EX_INSN("movddup","movddup",arg_simd,arg_simd,IX86_P5|INSN_SSE,K64_FAM9|INSN_SSE),
  /*0x13*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x14*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x15*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x16*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x17*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x18*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x19*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x20*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x21*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x22*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x23*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x24*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x25*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x26*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x27*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x28*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x29*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x2A*/ DECLARE_EX_INSN("cvtsi2sd","cvtsi2sd",bridge_simd_cpu,bridge_simd_cpu,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x2B*/ DECLARE_EX_INSN("movntsd","movntsd",arg_simd,arg_simd,IX86_P7|INSN_SSE|INSN_STORE,K64_FAM10|INSN_SSE|INSN_STORE),
  /*0x2C*/ DECLARE_EX_INSN("cvttsd2si","cvttsd2si",bridge_simd_cpu,bridge_simd_cpu,IX86_P4|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE),
  /*0x2D*/ DECLARE_EX_INSN("cvtsd2si","cvtsd2si",bridge_simd_cpu,bridge_simd_cpu,IX86_P4|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE),
  /*0x2E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x2F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x30*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x31*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x32*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x33*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x34*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x35*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x36*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x37*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x38*/ DECLARE_EX_INSN(ix86_F20F38_Table,ix86_F20F38_Table,NULL,NULL,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x39*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x40*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x41*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x42*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x43*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x44*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x45*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x46*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x47*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x48*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x49*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x50*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x51*/ DECLARE_EX_INSN("sqrtsd","sqrtsd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x52*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x53*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x54*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x55*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x56*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x57*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x58*/ DECLARE_EX_INSN("addsd","addsd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x59*/ DECLARE_EX_INSN("mulsd","mulsd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5A*/ DECLARE_EX_INSN("cvtsd2ss","cvtsd2ss",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x5C*/ DECLARE_EX_INSN("subsd","subsd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5D*/ DECLARE_EX_INSN("minsd","minsd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5E*/ DECLARE_EX_INSN("divsd","divsd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5F*/ DECLARE_EX_INSN("maxsd","maxsd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x60*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x61*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x62*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x63*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x64*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x65*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x66*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x67*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x68*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x69*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x70*/ DECLARE_EX_INSN("pshuflw","pshuflw",arg_simd_imm8,arg_simd_imm8,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x71*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x72*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x73*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x74*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x75*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x76*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x77*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x78*/ DECLARE_EX_INSN("insertq","insertq",arg_simd_regrm_imm8_imm8,arg_simd_regrm_imm8_imm8,IX86_P7|INSN_SSE,K64_FAM10|INSN_SSE),
  /*0x79*/ DECLARE_EX_INSN("insertq","insertq",arg_simd_regrm,arg_simd_regrm,IX86_P7|INSN_SSE,K64_FAM10|INSN_SSE),
  /*0x7A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7C*/ DECLARE_EX_INSN("haddps","haddps",arg_simd,arg_simd,IX86_P5|INSN_SSE|INSN_VEX_V,K64_FAM9|INSN_SSE|INSN_VEX_V),
  /*0x7D*/ DECLARE_EX_INSN("hsubps","hsubps",arg_simd,arg_simd,IX86_P5|INSN_SSE|INSN_VEX_V,K64_FAM9|INSN_SSE|INSN_VEX_V),
  /*0x7E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x80*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x81*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x82*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x83*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x84*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x85*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x86*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x87*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x88*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x89*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x90*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x91*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x92*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x93*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x94*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x95*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x96*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x97*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x98*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x99*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC2*/ DECLARE_EX_INSN("cmpsd","cmpsd",ix86_ArgXMMCmp,ix86_ArgXMMCmp,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD0*/ DECLARE_EX_INSN("addsubps","addsubps",arg_simd,arg_simd,IX86_P5|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD6*/ DECLARE_EX_INSN("movdq2q","movdq2q",bridge_sse_mmx,bridge_sse_mmx,IX86_P4|INSN_SSE|BRIDGE_MMX_SSE,K64_ATHLON|INSN_SSE|BRIDGE_MMX_SSE),
  /*0xD7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE6*/ DECLARE_EX_INSN("cvtpd2dq","cvtpd2dq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xED*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF0*/ DECLARE_EX_INSN("lddqu","lddqu",arg_simd,arg_simd,IX86_P5|INSN_SSE,K64_FAM9|INSN_SSE),
  /*0xF1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE)
};

const ix86_ExOpcodes ix86_F30F_PentiumTable[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x01*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x02*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x03*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x04*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x05*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x06*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x07*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x08*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x09*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x10*/ DECLARE_EX_INSN("movss","movss",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x11*/ DECLARE_EX_INSN("movss","movss",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_STORE),
  /*0x12*/ DECLARE_EX_INSN("movsldup","movsldup",arg_simd,arg_simd,IX86_P5|INSN_SSE,K64_FAM9|INSN_SSE),
  /*0x13*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x14*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x15*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x16*/ DECLARE_EX_INSN("movshdup","movshdup",arg_simd,arg_simd,IX86_P5|INSN_SSE,K64_FAM9|INSN_SSE),
  /*0x17*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x18*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x19*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x20*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x21*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x22*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x23*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x24*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x25*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x26*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x27*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x28*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x29*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x2A*/ DECLARE_EX_INSN("cvtsi2ss","cvtsi2ss",bridge_simd_cpu,bridge_simd_cpu,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x2B*/ DECLARE_EX_INSN("movntss","movntss",arg_simd,arg_simd,IX86_P7|INSN_SSE|INSN_STORE,K64_FAM10|INSN_SSE|INSN_STORE),
  /*0x2C*/ DECLARE_EX_INSN("cvttss2si","cvttss2si",bridge_simd_cpu,bridge_simd_cpu,IX86_P3|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE),
  /*0x2D*/ DECLARE_EX_INSN("cvtss2si","cvtss2si",bridge_simd_cpu,bridge_simd_cpu,IX86_P3|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE),
  /*0x2E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x2F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x30*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x31*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x32*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x33*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x34*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x35*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x36*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x37*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x38*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x39*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x40*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x41*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x42*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x43*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x44*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x45*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x46*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x47*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x48*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x49*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x50*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x51*/ DECLARE_EX_INSN("sqrtss","sqrtss",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x52*/ DECLARE_EX_INSN("rsqrtss","rsqrtss",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x53*/ DECLARE_EX_INSN("rcpss","rcpss",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x54*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x55*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x56*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x57*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x58*/ DECLARE_EX_INSN("addss","addss",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x59*/ DECLARE_EX_INSN("mulss","mulss",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5A*/ DECLARE_EX_INSN("cvtss2sd","cvtss2sd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5B*/ DECLARE_EX_INSN("cvttps2dq","cvttps2dq",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5C*/ DECLARE_EX_INSN("subss","subss",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5D*/ DECLARE_EX_INSN("minss","minss",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5E*/ DECLARE_EX_INSN("divss","divss",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5F*/ DECLARE_EX_INSN("maxss","maxss",arg_simd,arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x60*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x61*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x62*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x63*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x64*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x65*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x66*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x67*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x68*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x69*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6F*/ DECLARE_EX_INSN("movdqu","movdqu",arg_simd,arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x70*/ DECLARE_EX_INSN("pshufhw","pshufhw",arg_simd_imm8,arg_simd_imm8,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x71*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x72*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x73*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x74*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x75*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x76*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x77*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x78*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x79*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7E*/ DECLARE_EX_INSN("movq","movq",arg_simd,arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x7F*/ DECLARE_EX_INSN("movdqu","movdqu",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x80*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x81*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x82*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x83*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x84*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x85*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x86*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x87*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x88*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x89*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x90*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x91*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x92*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x93*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x94*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x95*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x96*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x97*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x98*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x99*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB8*/ DECLARE_EX_INSN("popcnt","popcnt",arg_cpu_modregrm,arg_cpu_modregrm,IX86_P7,K64_FAM10),
  /*0xB9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBD*/ DECLARE_EX_INSN("lzcnt","lzcnt",arg_cpu_modregrm,arg_cpu_modregrm,IX86_P7,K64_FAM10),
  /*0xBE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC2*/ DECLARE_EX_INSN("cmpss","cmpss",ix86_ArgXMMCmp,ix86_ArgXMMCmp,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC7*/ DECLARE_EX_INSN("vmxon","vmxon",arg_cpu_mod_rm,arg_cpu_mod_rm,IX86_P6,K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD6*/ DECLARE_EX_INSN("movq2dq","movq2dq",bridge_sse_mmx,bridge_sse_mmx,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0xD7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE6*/ DECLARE_EX_INSN("cvtdq2pd","cvtdq2pd",arg_simd,arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xED*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|INSN_SSE)
};
#if 0
const ix86_Opcodes ix86_NullTable[256] =
{
  /*0x00*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x01*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x02*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x03*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x04*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x05*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x06*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x07*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x08*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x09*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x0A*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x0B*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x0C*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x0D*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x0E*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x0F*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x10*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x11*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x12*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x13*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x14*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x15*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x16*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x17*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x18*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x19*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x1A*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x1B*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x1C*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x1D*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x1E*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x1F*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x20*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x21*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x22*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x23*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x24*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x25*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x26*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x27*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x28*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x29*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x2A*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x2B*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x2C*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x2D*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x2E*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x2F*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x30*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x31*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x32*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x33*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x34*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x35*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x36*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x37*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x38*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x39*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x3A*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x3B*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x3C*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x3D*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x3E*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x3F*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x40*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x41*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x42*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x43*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x44*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x45*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x46*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x47*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x48*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x49*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x4A*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x4B*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x4C*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x4D*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x4E*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x4F*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x50*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x51*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x52*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x53*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x54*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x55*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x56*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x57*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x58*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x59*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x5A*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x5B*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x5C*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x5D*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x5E*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x5F*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x60*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x61*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x62*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x63*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x64*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x65*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x66*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x67*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x68*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x69*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x6A*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x6B*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x6C*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x6D*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x6E*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x6F*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x70*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x71*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x72*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x73*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x74*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x75*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x76*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x77*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x78*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x79*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x7A*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x7B*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x7C*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x7D*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x7E*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x7F*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x80*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x81*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x82*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x83*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x84*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x85*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x86*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x87*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x88*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x89*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x8A*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x8B*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x8C*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x8D*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x8E*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x8F*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x90*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x91*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x92*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x93*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x94*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x95*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x96*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x97*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x98*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x99*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x9A*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x9B*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x9C*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x9D*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x9E*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0x9F*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xA0*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xA1*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xA2*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xA3*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xA4*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xA5*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xA6*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xA7*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xA8*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xA9*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xAA*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xAB*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xAC*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xAD*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xAE*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xAF*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xB0*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xB1*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xB2*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xB3*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xB4*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xB5*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xB6*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xB7*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xB8*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xB9*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xBA*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xBB*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xBC*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xBD*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xBE*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xBF*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xC0*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xC1*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xC2*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xC3*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xC4*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xC5*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xC6*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xC7*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xC8*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xC9*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xCA*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xCB*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xCC*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xCD*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xCE*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xCF*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xD0*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xD1*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xD2*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xD3*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xD4*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xD5*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xD6*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xD7*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xD8*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xD9*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xDA*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xDB*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xDC*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xDD*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xDE*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xDF*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xE0*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xE1*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xE2*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xE3*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xE4*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xE5*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xE6*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xE7*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xE8*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xE9*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xEA*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xEB*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xEC*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xED*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xEE*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xEF*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xF0*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xF1*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xF2*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xF3*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xF4*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xF5*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xF6*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xF7*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xF8*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xF9*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xFA*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xFB*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xFC*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xFD*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xFE*/ { "???", "???", NULL, IX86_UNKCPU },
  /*0xFF*/ { "???", "???", NULL, IX86_UNKCPU }
};

const ix86_ExOpcodes ix86_Null64Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON)
};
#endif

const char * ix86_Op1Names[] = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
const char * ix86_ShNames[] = { "rol", "ror", "rcl", "rcr", "shl", "shr", "RESERVED", "sar" };
const char * ix86_Gr1Names[] = { "test", "RESERVED", "not", "neg", "mul", "imul", "div", "idiv" };
const char * ix86_Gr2Names[] = { "inc", "dec", "call", "call", "jmp", "jmp", "push", "???"/** pop - not real */};

const char * ix86_ExGrp0[] = { "sldt", "str", "lldt", "ltr", "verr", "verw", "???", "???" };

const char * ix86_BitGrpNames[] = { "???", "???", "???", "???", "bt", "bts", "btr", "btc" };

const char *ix86_MMXGr1[] = { "???", "???", "psrlw", "???", "psraw", "???", "psllw", "???" };
const char *ix86_MMXGr2[] = { "???", "???", "psrld", "???", "psrad", "???", "pslld", "???" };
const char *ix86_MMXGr3[] = { "???", "???", "psrlq", "???", "???",   "???", "psllq", "???" };

const char *ix86_XMMXGr1[] = { "???", "???", "psrlw", "???", "psraw", "???", "psllw", "???" };
const char *ix86_XMMXGr2[] = { "???", "???", "psrld", "???", "psrad", "???", "pslld", "???" };
const char *ix86_XMMXGr3[] = { "???", "???", "psrlq", "psrldq", "???",   "???", "psllq", "pslldq" };

const char *ix86_3dPrefetchGrp[] = { "prefetch", "prefetchw", "prefetch", "prefetch", "prefetch", "prefetch", "prefetch", "prefetch" };

const char *ix86_KatmaiGr2Names[] = { "prefetchnta", "prefetcht0", "prefetcht1", "prefetcht2", "???", "???", "???", "???" };
const char *ix86_KatmaiCmpSuffixes[] = { "eq", "lt", "le", "unord", "neq", "nlt", "nle", "ord" };


unsigned x86_Bitness = DAB_AUTO;
static unsigned BITNESS = DAB_AUTO;

char *ix86_voidstr;
char *ix86_da_out;

char ix86_segpref[4] = "";

const unsigned char leave_insns[] = { 0x07, 0x17, 0x1F, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x61, 0x90, 0xC9 };


static tBool is_listed(unsigned char insn,const unsigned char *list,size_t listsize)
{
  size_t i;
  for(i = 0;i < listsize;i++) if(insn == list[i]) return True;
  return False;
}

/*
  For long 64 mode:
  [legacy prefix] [REX] [Opcode] ...
  legacy prefixes: 26 2E 36 3E 64 65 66 67 F0 F3 F2
*/
static MBuffer parse_REX_type(MBuffer insn,char *up,ix86Param* DisP)
{
    if(x86_Bitness == DAB_USE64 && !(DisP->pfx&PFX_REX)) {
	DisP->pfx|=PFX_REX;
	DisP->REX = insn[0];
    }
    (*up)++;
    return &insn[1];
}

static void ix86_gettype(DisasmRet *dret,ix86Param *_DisP)
{
 MBuffer insn;
 char ua,ud,up,has_lock,has_rep,has_seg;
 tBool has_vex;
 insn = &_DisP->RealCmd[0];
 dret->pro_clone = __INSNT_ORDINAL;
 has_vex = has_lock = has_rep = has_seg = 0;
 up = ua = ud = 0;
 RepeateByPrefix:
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64)
 {
   if(ua+ud+has_seg+has_rep+has_lock+has_vex>4) goto get_type;
 }
 else
#endif
 if(has_lock + has_rep > 1 || has_seg > 1 || ua > 1 || ud > 1 || has_vex > 0) goto get_type;
 /** do prefixes loop */
 switch(insn[0])
 {
#ifdef IX86_64
   case 0x40:
   case 0x41:
   case 0x42:
   case 0x43:
   case 0x44:
   case 0x45:
   case 0x46:
   case 0x47:
   case 0x48:
   case 0x49:
   case 0x4A:
   case 0x4B:
   case 0x4C:
   case 0x4D:
   case 0x4E:
   case 0x4F:
		if(x86_Bitness == DAB_USE64) {
		    insn=parse_REX_type(insn,&up,_DisP);
		    goto RepeateByPrefix;
		}
		else goto MakePref;
                break;
#endif
   case 0x26:
   case 0x2E:
   case 0x36:
   case 0x3E:
	      do_seg:
              if(has_seg) break;
              has_seg++;
              goto MakePref;
   case 0x64:
   case 0x65:
		if(x86_Bitness != DAB_USE64) goto do_seg;
		goto MakePref;
   case 0x66:
		_DisP->pfx|=PFX_66;
              ud++;
              goto MakePref;
   case 0x67:
		_DisP->pfx|=PFX_67;
              ua++;
              goto MakePref;
   case 0xF0:
		_DisP->pfx|=PFX_LOCK;
              if(has_lock + has_rep) break;
              has_lock++;
              goto MakePref;
   case 0x8F:
   case 0xC4:
	      if(x86_Bitness == DAB_USE64) has_vex++;
	      else if((insn[1]&0xC0)==0xC0) has_vex++;
	      insn=&insn[2];
   case 0xC5:
	      if(x86_Bitness == DAB_USE64) has_vex++;
	      else if((insn[1]&0xC0)==0xC0) has_vex++;
	      insn=&insn[1];
	      goto MakePref;
   case 0xF2:
		_DisP->pfx|=PFX_F2_REPNE;
              if(has_rep + has_lock)  break;
              has_rep++;
              goto MakePref;
   case 0xF3:
              if(insn[1] == 0x90) goto get_type;
		_DisP->pfx|=PFX_F3_REP;
              if(has_rep + has_lock) break;
              has_rep++;
              goto MakePref;
   default:   break;
   MakePref:
              insn = &insn[1];
              up++;
              goto RepeateByPrefix;
 }
  /** First check for SSE extensions */
  if(insn[0] == 0x0F && (_DisP->pfx&(PFX_66|PFX_F2_REPNE|PFX_F3_REP))) {
    const ix86_ExOpcodes *SSE2_ext=ix86_extable;
    if(_DisP->pfx&PFX_F2_REPNE) SSE2_ext=ix86_F20F_PentiumTable;
    else
    if(_DisP->pfx&PFX_F3_REP)   SSE2_ext=ix86_F30F_PentiumTable;
    else
    if(_DisP->pfx&PFX_66)       SSE2_ext=ix86_660F_PentiumTable;
    if(!(
#ifdef IX86_64
		x86_Bitness==DAB_USE64?
		SSE2_ext[insn[1]].name64:
#endif
	        SSE2_ext[insn[1]].name
		))  return;
  }
 get_type:
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    _DisP->mode|= MOD_WIDE_ADDR;
    if(_DisP->pfx&PFX_REX && ((_DisP->REX & 0x0F)>>3) != 0) _DisP->mode|=MOD_WIDE_DATA;
  }
#endif
   if((insn[0] & 0xF6) == 0xC2)
   {
     dret->pro_clone = __INSNT_RET;
     make_RET:
     if((insn[0] & 0xC7) == 0xC3)
     {
       dret->codelen = 0;
       dret->field   = 0;
     }
     else
     {
       dret->codelen = 2;
       dret->field   = 2;
     }
     return; /* Return here immediatly, because we have 'goto' operator. */
   }
#ifdef IX86_64
     else
     if(insn[0] == 0xFF && (insn[1]==0x15 || insn[1] == 0x25) && x86_Bitness == DAB_USE64)
     {
        dret->pro_clone = __INSNT_JMPRIP;
        dret->codelen = 4;
        dret->field = 2;
     }
#endif
     else
     if(insn[0] == 0xFF &&
        (((insn[1] & 0x27) == 0x25 && (_DisP->mode&MOD_WIDE_ADDR)) ||
        ((insn[1] & 0x27) == 0x26 && !(_DisP->mode&MOD_WIDE_ADDR))))
     {
        dret->pro_clone = __INSNT_JMPVVT;
        dret->codelen = (insn[1] & 0x27) == 0x25 ? 4 : 2;
        dret->field = 2;
     }
     else
     if(insn[0] == 0xFF && insn[1] == 0xA3 && (_DisP->mode&MOD_WIDE_ADDR))
     {
        dret->pro_clone = __INSNT_JMPPIC;
        dret->codelen = 4;
        dret->field = 2;
     }
     else
     {
        /* Attempt determine leave insn */
        size_t i;
        tBool leave_cond = False, ret_reached = False;
        for(i = 0;i < (PREDICT_DEPTH-1)*MAX_IX86_INSN_LEN;i++)
        {
          /*
             RETURN insn can not be reached first because it directly checked
             above. Therefore 'leave_cond' variable will computed correctly.
          */
          if((insn[i] & 0xF6) == 0xC2) { ret_reached = True; break; }
          leave_cond = is_listed(insn[i],leave_insns,sizeof(leave_insns));
          if(!leave_cond)
          {
            if((insn[i] == 0x89 && insn[i+1] == 0xC9) || /* mov esp, ebp */
               (insn[i] == 0x8B && insn[i+1] == 0xE5) || /* mov esp, ebp */
               (insn[i] == 0xD9 && insn[i+1] == 0xD0) || /* fnop */
               (insn[i] == 0xDD && insn[i+1] == 0xD8) || /* fstp st(0), st(0) */
               (insn[i] == 0x0F && insn[i+1] == 0x77)    /* emms */
              )
            {
              leave_cond = True;
              i++;
            }
            else
            if(insn[i] == 0x83 && (insn[i+1] == 0xC4 || insn[i+1] == 0xEC))
            { /* sub/add esp, short_num */
              leave_cond = True;
              i += 2;
            }
            else
            if(insn[i] == 0x81 && (insn[i+1] == 0xC4 || insn[i+1] == 0xEC))
            { /* sub/add esp, long_num */
              leave_cond = True;
              i += (_DisP->mode&MOD_WIDE_DATA) ? 5 : 3;
            }
          }
          if(!leave_cond) break;
        }
        if(leave_cond && ret_reached)
        {
          dret->pro_clone = __INSNT_LEAVE;
          insn = &insn[i];
          goto make_RET;
        }
     }
}

static unsigned char parse_REX(unsigned char code,ix86Param* DisP)
{
#ifdef IX86_64
    if(x86_Bitness == DAB_USE64 && !(DisP->pfx&PFX_REX))
    {
        DisP->pfx|=PFX_REX;
        DisP->REX = code;
    }
    DisP->CodeAddress++;
    DisP->RealCmd = &DisP->RealCmd[1];
#endif
    return DisP->RealCmd[0];
}

static void parse_VEX_pp(ix86Param* DisP) {
    if((DisP->VEX_vlp & 0x03) == 0x01) DisP->pfx|=PFX_66;
    else
    if((DisP->VEX_vlp & 0x03) == 0x02) DisP->pfx|=PFX_F3_REP;
    else
    if((DisP->VEX_vlp & 0x03) == 0x03) DisP->pfx|=PFX_F2_REPNE;
}

static void parse_VEX_C4(ix86Param* DisP) {
  unsigned char code;
  DisP->pfx|=PFX_VEX;
  DisP->pfx|=PFX_REX;
  code=DisP->RealCmd[1];
  DisP->REX=0x40;
  DisP->VEX_m = code&0x1F;
  DisP->REX|=((code>>7)&0x01)<<2; /* make R */
  DisP->REX|=((code>>6)&0x01)<<1; /* make X */
  DisP->REX|=((code>>5)&0x01);    /* make B */
  code=DisP->RealCmd[2];
  DisP->REX|=((code>>7)&0x01)<<3; /* make W */
  DisP->REX^=0x0F;                /* complenent it */
  DisP->VEX_vlp=code&0x7F;
  parse_VEX_pp(DisP);
}

static void parse_VEX_C5(ix86Param* DisP) {
  unsigned char code;
  DisP->pfx|=PFX_VEX;
  DisP->pfx|=PFX_REX;
  DisP->REX=0x4F;
  code=DisP->RealCmd[1];
  DisP->REX|=((code>>7)&0x01)<<2; /* make R */
  DisP->REX^=0x0F;                /* complenent it */
  DisP->VEX_m = 0x01; /* Fake 0Fh opcode*/
  DisP->VEX_vlp=code&0x7F;
  parse_VEX_pp(DisP);
}

static void parse_XOP_8F(ix86Param* DisP) {
    parse_VEX_C4(DisP);
    DisP->pfx|=PFX_XOP;
    DisP->XOP_m = DisP->VEX_m;
    DisP->VEX_m &= 0x1F;
    DisP->VEX_m = 0x01; /* Fake 0Fh opcode*/
}

static DisasmRet __FASTCALL__ ix86Disassembler(__filesize_t ulShift,
                                               MBuffer buffer,
                                               unsigned flags)
{
 unsigned char code;
 DisasmRet Ret;
 ix86Param DisP;
 unsigned char ua,ud,up;
 char has_lock,has_rep,has_seg;
 tBool has_vex,has_rex,has_xop;

 memset(&DisP,0,sizeof(DisP));
 memset(&Ret,0,sizeof(Ret));
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64) DisP.pro_clone=K64_ATHLON;
 else
#endif
 DisP.pro_clone = IX86_CPU086;
 DisP.codelen = 1;
 DisP.DisasmPrefAddr = ulShift;
 DisP.CodeAddress = ulShift;
 DisP.CodeBuffer = buffer;
 DisP.RealCmd = buffer;
 DisP.flags = flags;
 /*
    __DISF_GETTYPE must be reentrance. Must do not modify any internal
    disassembler variables and flags and e.t.c.
    It is calling from disassembler recursively.
 */
 if(flags & __DISF_GETTYPE)
 {
   ix86_gettype(&Ret,&DisP);
   goto Bye;
 }

 ix86_segpref[0] = 0;
 has_xop = has_rex = has_vex = has_lock = has_rep = has_seg = 0;
 up = ua = ud = 0;
 ix86_da_out[0] = 0;

 if(BITNESS == DAB_AUTO && detectedFormat->query_bitness) x86_Bitness = detectedFormat->query_bitness(ulShift);
 else x86_Bitness = BITNESS;

 if(x86_Bitness > DAB_USE16)
 {
    DisP.mode|=MOD_WIDE_DATA;
    DisP.mode|=MOD_WIDE_ADDR;
 }

 Ret.str = ix86_voidstr;
 Ret.str[0] = 0;
 RepeateByPrefix:
 code = DisP.RealCmd[0];
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64)
 {
   if(ua+ud+has_seg+has_rep+has_lock+has_vex+has_xop>4) goto bad_prefixes;
 }
 else
#endif
 if(has_lock + has_rep > 1 || has_seg > 1 || ua > 1 || ud > 1 || has_vex > 1 || has_rex > 1 || has_xop > 1)
 {
   bad_prefixes:
   DisP.codelen = ud;
   strcpy(ix86_voidstr,"???");
   goto ExitDisAsm;
 }
 if(has_vex || has_xop) goto end_of_prefixes;
 /** do prefixes loop */
 switch(code)
 {
#ifdef IX86_64
   case 0x40:
   case 0x41:
   case 0x42:
   case 0x43:
   case 0x44:
   case 0x45:
   case 0x46:
   case 0x47:
   case 0x48:
   case 0x49:
   case 0x4A:
   case 0x4B:
   case 0x4C:
   case 0x4D:
   case 0x4E:
   case 0x4F: if(x86_Bitness == DAB_USE64 && !(DisP.pfx&PFX_REX))
              {
		has_rex=1;
		code=parse_REX(code,&DisP);
		up++;
		goto RepeateByPrefix;
              }
              break;
#endif
   case 0x26:
		if(has_seg) break;
		/*in 64-bit mode, the CS,DS,ES,SS segment overrides are ignored but may be specified */
		DisP.pfx|=PFX_SEG_ES;
		strcpy(ix86_segpref,"es:");
		has_seg++;
		goto MakePref;
   case 0x2E:
		if(has_seg) break;
		/*in 64-bit mode, the CS,DS,ES,SS segment overrides are ignored but may be specified */
		DisP.pfx|=PFX_SEG_CS;
		strcpy(ix86_segpref,"cs:");
		has_seg++;
		goto MakePref;
   case 0x36:
		if(has_seg) break;
		/*in 64-bit mode, the CS,DS,ES,SS segment overrides are ignored but may be specified */
		DisP.pfx|=PFX_SEG_SS;
		strcpy(ix86_segpref,"ss:");
		has_seg++;
		goto MakePref;
   case 0x3E:
		if(has_seg) break;
		/*in 64-bit mode, the CS,DS,ES,SS segment overrides are ignored but may be specified */
		DisP.pfx|=PFX_SEG_DS;
		strcpy(ix86_segpref,"ds:");
		has_seg++;
		goto MakePref;
   case 0x64:
		if(has_seg) break;
		DisP.pfx|=PFX_SEG_FS;
		strcpy(ix86_segpref,"fs:");
		has_seg++;
		DisP.pro_clone = IX86_CPU386;
		goto MakePref;
   case 0x65:
		if(has_seg) break;
		DisP.pfx|=PFX_SEG_GS;
		strcpy(ix86_segpref,"gs:");
		has_seg++;
		DisP.pro_clone = IX86_CPU386;
		goto MakePref;
   case 0x66:
		ud++;
		DisP.pfx |= PFX_66;
		DisP.mode ^= MOD_WIDE_DATA;
		DisP.pro_clone = IX86_CPU386;
		goto MakePref;
   case 0x67:
		ua++;
		DisP.pfx |= PFX_67;
		DisP.mode ^= MOD_WIDE_ADDR;
		goto MakePref;
   case 0x8F:
		if(x86_Bitness == DAB_USE64) has_xop++;
		else if((DisP.RealCmd[1]&0xC0)==0xC0) has_xop++;
		if(has_xop) {
		    parse_XOP_8F(&DisP);
		    DisP.CodeAddress+=2;
		    up+=2;
		    DisP.RealCmd = &DisP.RealCmd[2];
		    goto MakePref;
		}
		break;
   case 0xC4:
		if(x86_Bitness == DAB_USE64) has_vex++;
		else if((DisP.RealCmd[1]&0xC0)==0xC0) has_vex++;
		if(has_vex) {
		    parse_VEX_C4(&DisP);
		    DisP.CodeAddress+=2;
		    up+=2;
		    DisP.RealCmd = &DisP.RealCmd[2];
		    goto MakePref;
		}
		break;
   case 0xC5:
		if(x86_Bitness == DAB_USE64) has_vex++;
		else if((DisP.RealCmd[1]&0xC0)==0xC0) has_vex++;
		if(has_vex) {
		    parse_VEX_C5(&DisP);
		    DisP.CodeAddress++;
		    up++;
		    DisP.RealCmd = &DisP.RealCmd[1];
		    goto MakePref;
		}
		break;
   case 0xF0:
		if(has_lock + has_rep) break;
		has_lock++;
		DisP.pfx|=PFX_LOCK;
		strcat(ix86_da_out,"lock; ");
		goto MakePref;
   case 0xF2:
		DisP.pfx|=PFX_F2_REPNE;
		if(has_rep + has_lock) break;
		has_rep++;
		strcat(ix86_da_out,"repne; ");
		goto MakePref;
   case 0xF3:
		DisP.pfx|=PFX_F3_REP;
		if(DisP.RealCmd[1] == 0x90) {
		/* this is pause insns */
		    strcpy(Ret.str,x86_Bitness == DAB_USE64?"pause":"pause");
		    DisP.codelen++;
		    DisP.pro_clone = DAB_USE64?K64_ATHLON:IX86_P4;
		    goto ExitDisAsm;
		}
		if(has_rep + has_lock) break;
		has_rep++;
		strcat(ix86_da_out,"rep; ");
		goto MakePref;
   default:   break;
   MakePref:
              DisP.CodeAddress++;
              DisP.RealCmd = &DisP.RealCmd[1];
              code = DisP.RealCmd[0];
              up++;
              goto RepeateByPrefix;
 }
 end_of_prefixes:
#ifdef IX86_64
 /* Let it be overwritten later */
 if(x86_Bitness == DAB_USE64) DisP.insn_flags = ix86_table[code].flags64;
 else
#endif
 DisP.insn_flags = ix86_table[code].pro_clone;
 if((DisP.pfx&PFX_VEX) && DisP.VEX_m>0 && DisP.VEX_m<4 && !(DisP.pfx&PFX_XOP)) goto fake_0F_opcode;
 if(code==0x0F && (DisP.pfx&(PFX_F2_REPNE|PFX_F3_REP|PFX_66))) {
    const ix86_ExOpcodes *SSE2_ext=ix86_extable;
    unsigned char ecode;
    const char *nam;
    ix86_method mthd;
    /* for continuing  */
    unsigned char up_saved = up;
    ix86Param DisP_saved=DisP;
    fake_0F_opcode:
    if(DisP.pfx&PFX_F2_REPNE) SSE2_ext=ix86_F20F_PentiumTable;
    else
    if(DisP.pfx&PFX_F3_REP)   SSE2_ext=ix86_F30F_PentiumTable;
    else
    if(DisP.pfx&PFX_66)       SSE2_ext=ix86_660F_PentiumTable;
    ecode = (DisP.pfx&PFX_VEX)?DisP.RealCmd[0]:DisP.RealCmd[1];
    if((DisP.pfx&PFX_VEX) && DisP.VEX_m==0x02) ecode = 0x38;
    else
    if((DisP.pfx&PFX_VEX) && DisP.VEX_m==0x03) ecode = 0x3A;

    SSE2_ext=ix86_prepare_flags(SSE2_ext,&DisP,&ecode,&up);

    if((DisP.pfx&PFX_VEX) && DisP.VEX_m>1) {
	DisP.RealCmd=&DisP.RealCmd[-1];
	up--;
    }
    if(DisP.pfx&PFX_VEX)	ecode=DisP.RealCmd[0];
    else			ecode=DisP.RealCmd[1];

    nam=((x86_Bitness==DAB_USE64)?SSE2_ext[ecode].name64:SSE2_ext[ecode].name);
    if(nam) {
	if(DisP.pfx&PFX_66&&
	    (SSE2_ext==ix86_660F_PentiumTable||
	     SSE2_ext==ix86_660F38_Table||
	     SSE2_ext==ix86_660F3A_Table
	    )) DisP.mode^=MOD_WIDE_DATA;
	if(DisP.pfx&PFX_VEX && nam[0]!='v') {
	    strcpy(Ret.str,"v");
	    strcat(Ret.str,nam);
	}
	else strcpy(Ret.str,nam);
	ix86_da_out[0]='\0'; /* disable rep; lock; prefixes */
	if(!(DisP.pfx&PFX_VEX) && (DisP.pfx&(PFX_F2_REPNE|PFX_F3_REP|PFX_66))) {
	    DisP.RealCmd = &DisP.RealCmd[1];
	    up++;
	}
	if(x86_Bitness == DAB_USE64)	DisP.insn_flags = SSE2_ext[ecode].flags64;
	else				DisP.insn_flags = SSE2_ext[ecode].pro_clone;
	mthd=((x86_Bitness==DAB_USE64)?SSE2_ext[ecode].method64:SSE2_ext[ecode].method);
	if(mthd) {
		TabSpace(Ret.str,TAB_POS);
		mthd(Ret.str,&DisP);
	}
	goto ExitDisAsm;
    }
    /* continue ordinal execution */
    DisP=DisP_saved;
    up=up_saved;
 }
 if(DisP.pfx&PFX_XOP) {
    const ix86_ExOpcodes* _this = &K64_XOP_Table[code];
    if(DisP.XOP_m==0x08)	strcpy(Ret.str,_this->name); /* emulate 8F.08 */
    else			strcpy(Ret.str,_this->name64);
    if(DisP.XOP_m==0x08 && _this->method) { /* emulate 8F.08 */
	TabSpace(Ret.str,TAB_POS);
	DisP.insn_flags = _this->flags64;
	_this->method(Ret.str,&DisP);
    }
    else if(DisP.XOP_m==0x09 &&_this->method64) {
	TabSpace(Ret.str,TAB_POS);
	DisP.insn_flags = _this->pro_clone;
	_this->method64(Ret.str,&DisP);
    }
 }
 else {
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64)
 {
   if(REX_W(DisP.REX)) DisP.mode|=MOD_WIDE_DATA; /* 66h prefix is ignored if REX prefix is present*/
   if(ix86_table[code].flags64 & K64_DEF32)
   {
     if(REX_W(DisP.REX)) strcpy(Ret.str,ix86_table[code].name64);
     else
     if(!(DisP.mode&MOD_WIDE_DATA))	strcpy(Ret.str,ix86_table[code].name16);
     else				strcpy(Ret.str,ix86_table[code].name32);
   }
   else
   if((DisP.mode&MOD_WIDE_DATA) || (ix86_table[code].flags64 & K64_NOCOMPAT))
		strcpy(Ret.str,ix86_table[code].name64);
   else		strcpy(Ret.str,ix86_table[code].name32);
 }
 else
#endif
 strcpy(Ret.str,(DisP.mode&MOD_WIDE_DATA) ? ix86_table[code].name32 : ix86_table[code].name16);
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64)
 {
   if(ix86_table[code].method64)
   {
	DisP.insn_flags = ix86_table[code].flags64;
	TabSpace(Ret.str,TAB_POS);
	ix86_table[code].method64(Ret.str,&DisP);
   }
 }
 else
#endif
 if(ix86_table[code].method)
 {
	DisP.insn_flags = ix86_table[code].pro_clone;
	TabSpace(Ret.str,TAB_POS);
	ix86_table[code].method(Ret.str,&DisP);
 }
 /** Special case for jmp call ret modify table name */
 switch(code)
 {
   /** retx, jmpx case */
     case 0xC2:
     case 0xC3:
     case 0xCA:
     case 0xCB:
     case 0xE9:
     case 0xEA:
                if(!ud && !ua) Ret.str[4] = Ret.str[5] = ' ';
                break;
   /** popax, popfx case */
     case 0x61:
     case 0x9D:
                if(!ud && !ua && x86_Bitness < DAB_USE64) Ret.str[4] = 0;
                break;
   /** callx case */
     case 0xE8:
     case 0x9A:
                if(!ud && !ua) Ret.str[5] = Ret.str[6] = ' ';
                break;
   /** pushax, pushfx case */
     case 0x60:
     case 0x9C:
                if(!ud && !ua && x86_Bitness < DAB_USE64) Ret.str[5] = 0;
                break;
     default:   break;
 }
 } /* ifelse XOP */
 ExitDisAsm:
 if(up) DisP.codelen+=up;
 /* control jumps here after SSE2 opcodes */
 if(ix86_segpref[0])
 {
    strcat(ix86_da_out,"seg ");
    ix86_segpref[2] = ';';
    strcat(ix86_da_out,ix86_segpref);
    strcat(ix86_da_out," ");
 }
 if(ix86_da_out[0]) TabSpace(ix86_da_out,TAB_POS);
 strcat(ix86_da_out,Ret.str);
 Ret.str = ix86_da_out;
#ifdef IX86_64
 if(x86_Bitness < DAB_USE64)
#endif
 if((DisP.pfx&PFX_66) || (DisP.pfx&PFX_67) || x86_Bitness == DAB_USE32)
 {
    if((DisP.pro_clone & IX86_CPUMASK) < IX86_CPU386)
    {
	DisP.pro_clone &= ~IX86_CPUMASK;
	DisP.pro_clone |= IX86_CPU386;
    }
 }
 Ret.pro_clone = DisP.insn_flags;
 Ret.codelen = DisP.codelen;
 Bye:
 return Ret;
}

static const char * CPUNames[] =
{
  " 8086 ",
  "80186 ",
  "80286 ",
  "80386 ",
  "80486 ",
  "80586 ",
  "80686/PII ",
  "PIII   ",
  "P4   ",
  "Prescott "
};

static const char * FPUNames[] =
{
  " 8087 ",
  "      ",
  "80287 ",
  "80387 ",
  "80487 ",
  "      ",
  "80687     ",
  "PIII   ",
  "P4   ",
  "Prescott "
};

static const char * MMXNames[] =
{
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "P1MMX ",
  "K6-2 3dNow",
  "PIII/K7",
  "P4   ",
  "Prescott "
};

#ifdef IX86_64
static const char * CPU64Names[] =
{
  " K86-64 CPU ",
  " K86-64 FPU/MMX ",
  " K86-64 SSE/AVX ",
  "       ",
  "       ",
  "       ",
  "       ",
  "      ",
  "      ",
  "      "
};
#endif
static const char * altPipesNames[] =
{
  " CPU ALU ",
  " FPU/MMX ALU ",
  " SSE/AVX ALU ",
  "       ",
  "       ",
  "       ",
  "       ",
  "      ",
  "      ",
  "      "
};

static int color_mode=0;

static ColorAttr  __FASTCALL__ ix86GetAsmColor(unsigned long clone)
{
    color_mode=0;
#ifdef IX86_64
     if(x86_Bitness == DAB_USE64)
     {
	if((clone & INSN_SSE) == INSN_SSE) return disasm_cset.cpu_cset[2].clone[0];
	else
	if(clone & (INSN_FPU|INSN_MMX)) return disasm_cset.cpu_cset[1].clone[0];
	else
	return disasm_cset.cpu_cset[0].clone[0];
     }
     else
#endif
     if((clone&INSN_MMX)|(clone&INSN_SSE)) return disasm_cset.cpu_cset[2].clone[clone & IX86_CPUMASK];
     else
       if((clone&INSN_FPU))	return disasm_cset.cpu_cset[1].clone[clone & IX86_CPUMASK];
       else			return disasm_cset.cpu_cset[0].clone[clone & IX86_CPUMASK];
}

static ColorAttr  __FASTCALL__ ix86GetOpcodeColor(unsigned long clone)
{
#ifdef IX86_64
   if(x86_Bitness == DAB_USE64)
	return ((clone & INSN_CPL0) == INSN_CPL0)?disasm_cset.opcodes0:disasm_cset.opcodes;
   else
#endif
  return ((clone & INSN_CPL0) == INSN_CPL0)?disasm_cset.opcodes0:disasm_cset.opcodes;
}

static ColorAttr  __FASTCALL__ ix86altGetAsmColor(unsigned long clone)
{
    color_mode=1;
#ifdef IX86_64
     if(x86_Bitness == DAB_USE64)
     {
	if(clone & INSN_SSE) return disasm_cset.cpu_cset[2].clone[clone & K64_CLONEMASK];
	else
	if(clone & (INSN_FPU|INSN_MMX)) return disasm_cset.cpu_cset[1].clone[clone & K64_CLONEMASK];
	else
	return disasm_cset.cpu_cset[0].clone[clone & K64_CLONEMASK];
     }
     else
#endif
     {
	if(clone & INSN_SSE) return disasm_cset.cpu_cset[2].clone[0];
	else
	if(clone & (INSN_FPU|INSN_MMX)) return disasm_cset.cpu_cset[1].clone[0];
	else
	return disasm_cset.cpu_cset[0].clone[0];
     }
}

static ColorAttr  __FASTCALL__ ix86altGetOpcodeColor(unsigned long clone)
{
#ifdef IX86_64
   if(x86_Bitness == DAB_USE64)
	return ((clone & INSN_CPL0) == INSN_CPL0)?disasm_cset.opcodes0:disasm_cset.opcodes;
   else
#endif
  return ((clone & INSN_CPL0) == INSN_CPL0)?disasm_cset.opcodes0:disasm_cset.opcodes;
}

static tBool __FASTCALL__ x86AsmRef( void )
{
  hlpDisplay(20000);
  return False;
}

static void __FASTCALL__ ix86HelpAsm( void )
{
 char *msgAsmText,*title;
 char **strs;
 unsigned size,i,evt;
 unsigned long nstrs;
 TWindow * hwnd;
 if(!hlpOpen(True)) return;
#ifdef IX86_64
 size = (unsigned)hlpGetItemSize(x86_Bitness == DAB_USE64 ? 20002:20001);
#else
 size = (unsigned)hlpGetItemSize(20001);
#endif
 if(!size) goto ix86hlp_bye;
 msgAsmText = PMalloc(size+1);
 if(!msgAsmText)
 {
   mem_off:
   MemOutBox(" Help Display ");
   goto ix86hlp_bye;
 }
#ifdef IX86_64
 if(!hlpLoadItem(x86_Bitness == DAB_USE64 ? 20002:20001,msgAsmText))
#else
 if(!hlpLoadItem(20001,msgAsmText))
#endif
 {
   PFree(msgAsmText);
   goto ix86hlp_bye;
 }
 msgAsmText[size] = 0;
 if(!(strs = hlpPointStrings(msgAsmText,size,&nstrs))) goto mem_off;
 title = msgAsmText;
 hwnd = CrtHlpWndnls(title,72,21);
 twUseWin(hwnd);
 for(i = 0;i < nstrs;i++)
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
   rlen = hlpFillBuffer(&it,__TVIO_MAXSCREENWIDTH,strs[i],rlen,0,NULL,0);
   twWriteBuffer(hwnd,2,i+2,&it,rlen);
 }
 PFree(msgAsmText);
 twGotoXY(5,3);
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64 || color_mode==1)
 {
   twGotoXY(5,3);
   i=0;
//   for(i = 0;i < 10;i++)
   {
     twSetColorAttr(disasm_cset.cpu_cset[0].clone[i]);
     twPutS((x86_Bitness == DAB_USE64)?CPU64Names[0]:altPipesNames[0]);
     twClrEOL();
   }
   twGotoXY(5,4);
//   for(i = 0;i < 10;i++)
   {
     twSetColorAttr(disasm_cset.cpu_cset[1].clone[i]);
     twPutS((x86_Bitness == DAB_USE64)?CPU64Names[1]:altPipesNames[1]);
     twClrEOL();
   }
   twGotoXY(5,5);
//   for(i = 0;i < 10;i++)
   {
     twSetColorAttr(disasm_cset.cpu_cset[2].clone[i]);
     twPutS((x86_Bitness == DAB_USE64)?CPU64Names[2]:altPipesNames[2]);
     twClrEOL();
   }
 }
 else
#endif
 {
 for(i = 0;i < 10;i++)
 {
   twSetColorAttr(disasm_cset.cpu_cset[0].clone[i]);
   twPutS(CPUNames[i]);
 }
 twGotoXY(5,4);
 for(i = 0;i < 10;i++)
 {
   twSetColorAttr(disasm_cset.cpu_cset[1].clone[i]);
   twPutS(FPUNames[i]);
 }
 twGotoXY(5,5);
 for(i = 0;i < 10;i++)
 {
   twSetColorAttr(disasm_cset.cpu_cset[2].clone[i]);
   twPutS(MMXNames[i]);
 }
 }
 do
 {
   evt = GetEvent(drawEmptyPrompt,NULL,hwnd);
 }
 while(!(evt == KE_ESCAPE || evt == KE_F(10)));
 CloseWnd(hwnd);
 ix86hlp_bye:
 hlpClose();
}

static const char *use_names[] =
{
   "Use1~6",
   "Use~32",
#ifdef IX86_64
   "Use6~4",
#endif
   "~Auto"
};

static tBool __FASTCALL__ x86Select_Bitness( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(use_names)/sizeof(char *);
  if(BITNESS == DAB_AUTO) BITNESS = 3;
  i = SelBoxA(use_names,nModes," Select bitness mode: ",BITNESS);
  if(i != -1)
  {
    if(i == 3) i = DAB_AUTO;
    BITNESS = x86_Bitness = i;
    return True;
  }
  else if(BITNESS == 3) BITNESS = x86_Bitness = DAB_AUTO;
  return False;
}

static int __FASTCALL__ ix86MaxInsnLen( void ) { return MAX_IX86_INSN_LEN; }
static int __FASTCALL__ ix86GetBitness( void ) { return BITNESS; }
static char __FASTCALL__ ix86GetClone(unsigned long clone) { if(x86_Bitness == DAB_USE64) return 'a'; else return ix86CloneSNames[(clone >> 24) & 0x0F]; }

extern char *ix86_Katmai_buff;
extern char *ix86_appstr;
extern char *ix86_dtile;
extern char *ix86_appbuffer;
extern char *ix86_apistr;
extern char *ix86_modrm_ret;

struct assembler_t
{
  const char *run_command;
  const char *detect_command;
};

/*
  x86 disassemblers
  The run_command is expected to be used in sprintf to generate the actual command line.
  The argument is a single string containing the home directory. The temporary files
  created are:
    tmp0: input file
    tmp1: output file
    tmp2: output (stdout) / error (stderr) messages (note the "-s" option)
  The detect_command is any command which prints something to stdout as long as the assembler
  is available.
  Yasm is preferred over Nasm only because at the time of writing only Yasm has 64-bit
  support in its stable release.
  The last element must be a structure containing only NULL pointers.
*/
struct assembler_t assemblers[] = {
  {
    "yasm -f bin -s -o %1$stmp1 %1$stmp0 >%1$stmp2",
#if defined(__MSDOS__) || defined(__OS2__) || defined(__WIN32__)
    "yasm -s --version 2>NUL",
#else
    "yasm -s --version 2>/dev/null",
#endif
  },
  {
    "nasm -f bin -s -o %1$stmp1 %1$stmp0 >%1$stmp2",
#if defined(__MSDOS__) || defined(__OS2__) || defined(__WIN32__)
    "nasm -s -version 2>NUL",
#else
    "nasm -s -version 2>/dev/null",
#endif
  },
  {
    NULL,
    NULL,
  }
};

static signed int active_assembler = -1;

#ifndef HAVE_PCLOSE
#define pclose(fp) fclose(fp)
#endif
extern tBool iniUseExtProgs;
static void __FASTCALL__ ix86Init( void )
{
  ix86_voidstr = PMalloc(1000);
  ix86_da_out  = PMalloc(1000);
  ix86_Katmai_buff = PMalloc(1000);
  ix86_appstr = PMalloc(1000);
  ix86_dtile = PMalloc(1000);
  ix86_appbuffer = PMalloc(1000);
  ix86_apistr = PMalloc(1000);
  ix86_modrm_ret = PMalloc(1000);
  if((!ix86_voidstr) || (!ix86_da_out) || !(ix86_Katmai_buff) ||
     (!ix86_appstr) || (!ix86_dtile) || (!ix86_appbuffer) ||
     (!ix86_apistr) || (!ix86_modrm_ret)
     )
  {
    MemOutBox("ix86 disassembler initialization");
    exit(EXIT_FAILURE);
  }
#ifdef HAVE_POPEN
  //Assembler initialization
  //Look for an available assembler
  if (active_assembler == -1 && iniUseExtProgs==True) //Execute this only once
  {
    int i;
    for (i = 0; assemblers[i].detect_command; i++)
    {
      //Assume that the assembler is available if the detect_command prints something to stdout.
      //Note: using the return value of "system()" does not work, at least here.
      FILE *fp;
      fp = popen(assemblers[i].detect_command, "r");
      if (fp == NULL) continue;
      if (fgetc(fp) != EOF) {
        pclose(fp);
	active_assembler=i;
        break;
      }
      pclose(fp);
    }
  }
#endif
}

static void __FASTCALL__ ix86Term( void )
{
   PFREE(ix86_voidstr);
   PFREE(ix86_da_out);
   PFREE(ix86_Katmai_buff);
   PFREE(ix86_appstr);
   PFREE(ix86_dtile);
   PFREE(ix86_appbuffer);
   PFREE(ix86_apistr);
   PFREE(ix86_modrm_ret);
}

static void __FASTCALL__ ix86ReadIni( hIniProfile *ini )
{
  char tmps[10];
  if(isValidIniArgs())
  {
    biewReadProfileString(ini,"Biew","Browser","SubSubMode3","1",tmps,sizeof(tmps));
    BITNESS = (unsigned)strtoul(tmps,NULL,10);
    if(BITNESS > 2 && BITNESS != DAB_AUTO) BITNESS = 0;
    x86_Bitness = BITNESS;
  }
}

static void __FASTCALL__ ix86WriteIni( hIniProfile *ini )
{
  char tmps[10];
  sprintf(tmps,"%u",BITNESS);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode3",tmps);
}

#define CODEBUFFER_LEN 64
char codebuffer[CODEBUFFER_LEN];

/*
  Assemble "code".
  On success, it returns:
    .insn       = assembled bytes
    .insn_len   = length of assembled code
    .error_code = 0
  On error, it returns:
    .insn       = an NULL-terminated error message
    .insn_len   = undefined
    .error_code = non-zero
*/
AsmRet __FASTCALL__ ix86Asm(const char *code)
{
  AsmRet result;
  FILE *asmf, *bin, *err;
  int i,c;
  char commandbuffer[FILENAME_MAX+1];
  char *home=NULL;

  //Check assembler availability
  memset(&result,0,sizeof(AsmRet));
  if (active_assembler<0) goto noassemblererror;
  if (!assemblers[active_assembler].run_command) goto noassemblererror;

  home = __get_home_dir("biew");

  //File cleanup
  sprintf(commandbuffer, "%stmp0", home);
  for (i=0; i<3; i++)
  {
    remove(commandbuffer);
    commandbuffer[strlen(commandbuffer)-1]++;
  }

  //Generate NASM input file
  sprintf(commandbuffer, "%stmp0", home);
  asmf = fopen(commandbuffer, "w");
  if (!asmf) goto tmperror;
  if (ix86GetBitness() == DAB_USE16)
  {
    fprintf(asmf, "BITS 16");
  }
#ifdef IX86_64
  else if (ix86GetBitness() == DAB_USE64)
  {
    fprintf(asmf, "BITS 64");
  }
#endif
  else
  {
    fprintf(asmf, "BITS 32");
  }
  fprintf(asmf, "\n%s",code);
  fclose(asmf);

  //Build command line
  i=sprintf(commandbuffer, assemblers[active_assembler].run_command, home);
  if ((i >= FILENAME_MAX) || (i < 0)) goto commandtoolongerror;

  //Run external assembler
  //It happens way too often that system() fails with EAGAIN with
  //no apparent reason. So, don't give up immediately
  i=0;
  do
  {
    errno=0;
    system(commandbuffer);
    i++;
  }
  while ((errno == EAGAIN) && (i < 10));

  //Read result
  sprintf(commandbuffer, "%stmp1", home);
  bin = fopen(commandbuffer, "r");
  if (!bin) goto asmerror;
  i=0;
  while (((c = fgetc(bin)) != EOF) && (i<CODEBUFFER_LEN))
  {
    codebuffer[i++]=(char)c;
  }
  fclose(bin);

  if (i==CODEBUFFER_LEN) goto codetoolongerror;
  if (i==0) goto asmerror;

  result.err_code=0;
  result.insn_len=i;
  result.insn=codebuffer;
  goto done;

asmerror:
  //Read error message
  sprintf(commandbuffer, "%stmp2", home);
  err = fopen(commandbuffer, "r");
  if (!err) goto tmperror;
  i=0;
  while (((c = fgetc(err)) != EOF) && (i<CODEBUFFER_LEN-1))
  {
    //Only get the 1st error line (usually the most informative)
    if ((((char)c) == '\r') || (((char)c) == '\n')) break;

    codebuffer[i++]=(char)c;
    /*
    Most error messages are in the form:
      [something]: [error description]
    Discarding everything before ':' we obtain the [error description] alone
    */
    if ((char)c == ':')
    {
      i=0;
    }
  }
  fclose(err);
  codebuffer[i]='\0';
  result.insn = codebuffer;
  while (result.insn[0] == ' ') //Drop leading spaces
    result.insn++;
  result.err_code=1;
  goto doneerror;

codetoolongerror:
  result.insn="Internal error (assembly code too long)";
  result.err_code=2;
  goto doneerror;

tmperror:
  result.insn="Can't write temporary files";
  result.err_code=3;
  goto doneerror;

noassemblererror:
  result.insn="No assembler available/usable";
  result.err_code=4;
  goto doneerror;

commandtoolongerror:
  result.insn="Internal error (command too long)";
  result.err_code=5;
  goto doneerror;

doneerror:
done:
  //Final cleanup
  sprintf(commandbuffer, "%stmp0", home);
  for (i=0; i<3; i++)
  {
    remove(commandbuffer);
    commandbuffer[strlen(commandbuffer)-1]++;
  }
  return result;
}

REGISTRY_DISASM ix86_Disasm =
{
  DISASM_CPU_IX86,
  "Intel ~ix86 / x86_64",
  { "x86Hlp", "Bitnes", NULL, NULL },
  { x86AsmRef, x86Select_Bitness, NULL, NULL },
  ix86Disassembler,
  ix86Asm,
  ix86HelpAsm,
  ix86MaxInsnLen,
  ix86GetAsmColor,
  ix86GetOpcodeColor,
  ix86altGetAsmColor,
  ix86altGetOpcodeColor,
  ix86GetBitness,
  ix86GetClone,
  ix86Init,
  ix86Term,
  ix86ReadIni,
  ix86WriteIni
};








