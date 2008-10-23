/**
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/ix86/ix86.c
 * @brief       This file contains implementation of Intel x86 disassembler (main module).
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

const char * ix86_ByteRegs[] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
const char * ix86_WordRegs[] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
const char * ix86_DWordRegs[]= { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" };
const char * ix86_MMXRegs[]  = { "mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm7" };
const char * ix86_XMMXRegs[]  = { "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7" };
const char * ix86_SegRegs[]  = { "es", "cs", "ss", "ds", "fs", "gs", "?s", "?s" };
const char * ix86_CrxRegs[]  = { "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7" };
const char * ix86_DrxRegs[]  = { "dr0", "dr1", "dr2", "dr3", "dr4", "dr5", "dr6", "dr7" };
const char * ix86_TrxRegs[]  = { "tr0", "tr1", "tr2", "tr3", "tr4", "tr5", "tr6", "tr7" };
const char * ix86_XrxRegs[]  = { "?r0", "?r1", "?r2", "?r3", "?r4", "?r5", "?r6", "?r7" };
#ifdef IX86_64
const char * k86_ByteRegs[] = { "al",  "cl",  "dl",  "bl",  "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b" };
const char * k86_WordRegs[] = { "ax",  "cx",  "dx",  "bx",  "sp",  "bp",  "si",  "di",  "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w" };
const char * k86_DWordRegs[]= { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };
const char * k86_QWordRegs[]= { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8",  "r9",  "r10",  "r11",  "r12",  "r13",  "r14",  "r15" };
const char * k86_XMMXRegs[] = { "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7","xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15" };
const char * k86_CrxRegs[]  = { "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7", "cr8", "cr9", "cr10", "cr11", "cr12", "cr13", "cr14", "cr15" };
const char * k86_DrxRegs[]  = { "dr0", "dr1", "dr2", "dr3", "dr4", "dr5", "dr6", "dr7", "dr8", "dr9", "dr10", "dr11", "dr12", "dr13", "dr14", "dr15" };
const char * k86_TrxRegs[]  = { "tr0", "tr1", "tr2", "tr3", "tr4", "tr5", "tr6", "tr7", "tr8", "tr9", "tr10", "tr11", "tr12", "tr13", "tr14", "tr15" };
const char * k86_XrxRegs[]  = { "?r0", "?r1", "?r2", "?r3", "?r4", "?r5", "?r6", "?r7", "?r8", "?r9", "?r10", "?r11", "?r12", "?r13", "?r14", "?r15" };
/*
  REX is 0x4? opcodes 
  bits  meaning
  0     rex.b (extension to the Base)
  1     rex.x (extsnsion to the SIB indeX)
  2     rex.r (extension to the ModRM/REG)
  3     rex.w (extension to the operand Width)
  7-4   0100
  DEFAULT operand size:
    if rex.w then 64
    else 66_pref then 16
    else 32
  DEFAULT address size:
    if 67_pref then 32
    else 64
    (note: address displasement always has 8, 16 or 32-bit)
*/
unsigned char k86_REX;
int has_REX; /* is required for accessing to ah-dh registers */
tBool has67_in64;
#endif

#ifdef IX86_64
#define DECLARE_BASE_INSN(n16, n32, n64, func, func64, flags, flags64)\
{ n16, n32, n64, func, flags, func64, flags64 }
#else
#define DECLARE_BASE_INSN(n16, n32, n64, func, func64, flags, flags64)\
{ n16, n32, func, flags }
#endif

const ix86_Opcodes ix86_table[256] =
{
  /*0x00*/ DECLARE_BASE_INSN("add","add","add",ix86_ArgModRMnDnW,ix86_ArgModRMnDnW,IX86_CPU086,K64_ATHLON),
  /*0x01*/ DECLARE_BASE_INSN("add","add","add",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU086,K64_ATHLON),
  /*0x02*/ DECLARE_BASE_INSN("add","add","add",ix86_ArgModRMDnW,ix86_ArgModRMDnW,IX86_CPU086,K64_ATHLON),
  /*0x03*/ DECLARE_BASE_INSN("add","add","add",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU086,K64_ATHLON),
  /*0x04*/ DECLARE_BASE_INSN("add","add","add",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x05*/ DECLARE_BASE_INSN("add","add","add",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x06*/ DECLARE_BASE_INSN("push","push","???",ix86_ArgES,ix86_ArgES,IX86_CPU086,K64_ATHLON),
  /*0x07*/ DECLARE_BASE_INSN("pop","pop","???",ix86_ArgES,ix86_ArgES,IX86_CPU086,K64_ATHLON),
  /*0x08*/ DECLARE_BASE_INSN("or","or","or",ix86_ArgModRMnDnW,ix86_ArgModRMnDnW,IX86_CPU086,K64_ATHLON),
  /*0x09*/ DECLARE_BASE_INSN("or","or","or",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU086,K64_ATHLON),
  /*0x0A*/ DECLARE_BASE_INSN("or","or","or",ix86_ArgModRMDnW,ix86_ArgModRMDnW,IX86_CPU086,K64_ATHLON),
  /*0x0B*/ DECLARE_BASE_INSN("or","or","or",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU086,K64_ATHLON),
  /*0x0C*/ DECLARE_BASE_INSN("or","or","or",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x0D*/ DECLARE_BASE_INSN("or","or","or",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x0E*/ DECLARE_BASE_INSN("push","push","???", ix86_ArgCS,ix86_ArgCS,IX86_CPU086,K64_ATHLON),
  /*0x0F*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ExOpCodes,ix86_ExOpCodes,IX86_CPU086,K64_ATHLON),
  /*0x10*/ DECLARE_BASE_INSN("adc","adc","adc",ix86_ArgModRMnDnW,ix86_ArgModRMnDnW,IX86_CPU086,K64_ATHLON),
  /*0x11*/ DECLARE_BASE_INSN("adc","adc","adc",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU086,K64_ATHLON),
  /*0x12*/ DECLARE_BASE_INSN("adc","adc","adc",ix86_ArgModRMDnW,ix86_ArgModRMDnW,IX86_CPU086,K64_ATHLON),
  /*0x13*/ DECLARE_BASE_INSN("adc","adc","adc",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU086,K64_ATHLON),
  /*0x14*/ DECLARE_BASE_INSN("adc","adc","adc",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x15*/ DECLARE_BASE_INSN("adc","adc","adc",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x16*/ DECLARE_BASE_INSN("push","push","???",ix86_ArgSS,ix86_ArgSS,IX86_CPU086,K64_ATHLON),
  /*0x17*/ DECLARE_BASE_INSN("pop","pop","???",ix86_ArgSS,ix86_ArgSS,IX86_CPU086,K64_ATHLON),
  /*0x18*/ DECLARE_BASE_INSN("sbb","sbb","sbb",ix86_ArgModRMnDnW,ix86_ArgModRMnDnW,IX86_CPU086,K64_ATHLON),
  /*0x19*/ DECLARE_BASE_INSN("sbb","sbb","sbb",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU086,K64_ATHLON),
  /*0x1A*/ DECLARE_BASE_INSN("sbb","sbb","sbb",ix86_ArgModRMDnW,ix86_ArgModRMDnW,IX86_CPU086,K64_ATHLON),
  /*0x1B*/ DECLARE_BASE_INSN("sbb","sbb","sbb",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU086,K64_ATHLON),
  /*0x1C*/ DECLARE_BASE_INSN("sbb","sbb","sbb",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x1D*/ DECLARE_BASE_INSN("sbb","sbb","sbb",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x1E*/ DECLARE_BASE_INSN("push","push","???",ix86_ArgDS,ix86_ArgDS,IX86_CPU086,K64_ATHLON),
  /*0x1F*/ DECLARE_BASE_INSN("pop","pop","???",ix86_ArgDS,ix86_ArgDS,IX86_CPU086,K64_ATHLON),
  /*0x20*/ DECLARE_BASE_INSN("and","and","and",ix86_ArgModRMnDnW,ix86_ArgModRMnDnW,IX86_CPU086,K64_ATHLON),
  /*0x21*/ DECLARE_BASE_INSN("and","and","and",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU086,K64_ATHLON),
  /*0x22*/ DECLARE_BASE_INSN("and","and","and",ix86_ArgModRMDnW,ix86_ArgModRMDnW,IX86_CPU086,K64_ATHLON),
  /*0x23*/ DECLARE_BASE_INSN("and","and","and",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU086,K64_ATHLON),
  /*0x24*/ DECLARE_BASE_INSN("and","and","and",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x25*/ DECLARE_BASE_INSN("and","and","and",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x26*/ DECLARE_BASE_INSN("seg","seg","seg",ix86_ArgES,ix86_ArgES,IX86_CPU086,K64_ATHLON),
  /*0x27*/ DECLARE_BASE_INSN("daa","daa","???",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0x28*/ DECLARE_BASE_INSN("sub","sub","sub",ix86_ArgModRMnDnW,ix86_ArgModRMnDnW,IX86_CPU086,K64_ATHLON),
  /*0x29*/ DECLARE_BASE_INSN("sub","sub","sub",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU086,K64_ATHLON),
  /*0x2A*/ DECLARE_BASE_INSN("sub","sub","sub",ix86_ArgModRMDnW,ix86_ArgModRMDnW,IX86_CPU086,K64_ATHLON),
  /*0x2B*/ DECLARE_BASE_INSN("sub","sub","sub",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU086,K64_ATHLON),
  /*0x2C*/ DECLARE_BASE_INSN("sub","sub","sub",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x2D*/ DECLARE_BASE_INSN("sub","sub","sub",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x2E*/ DECLARE_BASE_INSN("seg","seg","seg",ix86_ArgCS,ix86_ArgCS,IX86_CPU086,K64_ATHLON),
  /*0x2F*/ DECLARE_BASE_INSN("das","das","???",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0x30*/ DECLARE_BASE_INSN("xor","xor","xor",ix86_ArgModRMnDnW,ix86_ArgModRMnDnW,IX86_CPU086,K64_ATHLON),
  /*0x31*/ DECLARE_BASE_INSN("xor","xor","xor",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU086,K64_ATHLON),
  /*0x32*/ DECLARE_BASE_INSN("xor","xor","xor",ix86_ArgModRMDnW,ix86_ArgModRMDnW,IX86_CPU086,K64_ATHLON),
  /*0x33*/ DECLARE_BASE_INSN("xor","xor","xor",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU086,K64_ATHLON),
  /*0x34*/ DECLARE_BASE_INSN("xor","xor","xor",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x35*/ DECLARE_BASE_INSN("xor","xor","xor",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x36*/ DECLARE_BASE_INSN("seg","seg","seg",ix86_ArgSS,ix86_ArgSS,IX86_CPU086,K64_ATHLON),
  /*0x37*/ DECLARE_BASE_INSN("aaa","aaa","???",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0x38*/ DECLARE_BASE_INSN("cmp","cmp","cmp",ix86_ArgModRMnDnW,ix86_ArgModRMnDnW,IX86_CPU086,K64_ATHLON),
  /*0x39*/ DECLARE_BASE_INSN("cmp","cmp","cmp",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU086,K64_ATHLON),
  /*0x3A*/ DECLARE_BASE_INSN("cmp","cmp","cmp",ix86_ArgModRMDnW,ix86_ArgModRMDnW,IX86_CPU086,K64_ATHLON),
  /*0x3B*/ DECLARE_BASE_INSN("cmp","cmp","cmp",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU086,K64_ATHLON),
  /*0x3C*/ DECLARE_BASE_INSN("cmp","cmp","cmp",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x3D*/ DECLARE_BASE_INSN("cmp","cmp","cmp",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0x3E*/ DECLARE_BASE_INSN("seg","seg","seg",ix86_ArgDS,ix86_ArgDS,IX86_CPU086,K64_ATHLON),
  /*0x3F*/ DECLARE_BASE_INSN("aas","aas","???",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0x40*/ DECLARE_BASE_INSN("inc","inc","inc",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x41*/ DECLARE_BASE_INSN("inc","inc","inc",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x42*/ DECLARE_BASE_INSN("inc","inc","inc",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x43*/ DECLARE_BASE_INSN("inc","inc","inc",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x44*/ DECLARE_BASE_INSN("inc","inc","inc",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x45*/ DECLARE_BASE_INSN("inc","inc","inc",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x46*/ DECLARE_BASE_INSN("inc","inc","inc",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x47*/ DECLARE_BASE_INSN("inc","inc","inc",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x48*/ DECLARE_BASE_INSN("dec","dec","dec",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x49*/ DECLARE_BASE_INSN("dec","dec","dec",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x4A*/ DECLARE_BASE_INSN("dec","dec","dec",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x4B*/ DECLARE_BASE_INSN("dec","dec","dec",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x4C*/ DECLARE_BASE_INSN("dec","dec","dec",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x4D*/ DECLARE_BASE_INSN("dec","dec","dec",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x4E*/ DECLARE_BASE_INSN("dec","dec","dec",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x4F*/ DECLARE_BASE_INSN("dec","dec","dec",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x50*/ DECLARE_BASE_INSN("push","push","push",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x51*/ DECLARE_BASE_INSN("push","push","push",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x52*/ DECLARE_BASE_INSN("push","push","push",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x53*/ DECLARE_BASE_INSN("push","push","push",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x54*/ DECLARE_BASE_INSN("push","push","push",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x55*/ DECLARE_BASE_INSN("push","push","push",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x56*/ DECLARE_BASE_INSN("push","push","push",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x57*/ DECLARE_BASE_INSN("push","push","push",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x58*/ DECLARE_BASE_INSN("pop","pop","pop",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x59*/ DECLARE_BASE_INSN("pop","pop","pop",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x5A*/ DECLARE_BASE_INSN("pop","pop","pop",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x5B*/ DECLARE_BASE_INSN("pop","pop","pop",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x5C*/ DECLARE_BASE_INSN("pop","pop","pop",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x5D*/ DECLARE_BASE_INSN("pop","pop","pop",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x5E*/ DECLARE_BASE_INSN("pop","pop","pop",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x5F*/ DECLARE_BASE_INSN("pop","pop","pop",ix86_ArgIReg,ix86_ArgIReg64,IX86_CPU086,K64_ATHLON),
  /*0x60*/ DECLARE_BASE_INSN("pushaw","pushad","???",NULL,NULL,IX86_CPU186,K64_ATHLON),
  /*0x61*/ DECLARE_BASE_INSN("popaw","popad","???",NULL,NULL,IX86_CPU186,K64_ATHLON),
  /*0x62*/ DECLARE_BASE_INSN("bound","bound","???",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU286,K64_ATHLON),
  /*0x63*/ DECLARE_BASE_INSN("arpl","arpl","movsxd",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU286,K64_ATHLON),
  /*0x64*/ DECLARE_BASE_INSN("seg","seg","seg",ix86_ArgFS,ix86_ArgFS,IX86_CPU386,K64_ATHLON),
  /*0x65*/ DECLARE_BASE_INSN("seg","seg","seg",ix86_ArgGS,ix86_ArgGS,IX86_CPU386,K64_ATHLON),
  /*0x66*/ DECLARE_BASE_INSN("???","???","???",NULL,NULL,IX86_CPU386,K64_ATHLON),
  /*0x67*/ DECLARE_BASE_INSN("???","???","???",NULL,NULL,IX86_CPU386,K64_ATHLON),
  /*0x68*/ DECLARE_BASE_INSN("push","push","push",ix86_ArgInt,ix86_ArgInt,IX86_CPU186,K64_ATHLON),
  /*0x69*/ DECLARE_BASE_INSN("imul","imul","imul",ix86_ArgRegRMDigit,ix86_ArgRegRMDigit,IX86_CPU186,K64_ATHLON),
  /*0x6A*/ DECLARE_BASE_INSN("push","push","push",ix86_ArgSInt,ix86_ArgSInt,IX86_CPU186,K64_ATHLON),
  /*0x6B*/ DECLARE_BASE_INSN("imul","imul","imul",ix86_ArgRegRMDigit,ix86_ArgRegRMDigit,IX86_CPU186,K64_ATHLON),
  /*0x6C*/ DECLARE_BASE_INSN("insb","insb","insb",NULL,NULL,IX86_CPU186|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x6D*/ DECLARE_BASE_INSN("insw","insd","insd",NULL,NULL,IX86_CPU186|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x6E*/ DECLARE_BASE_INSN("outsb","outsb","outsb",NULL,NULL,IX86_CPU186|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x6F*/ DECLARE_BASE_INSN("outsw","outsd","outsd",NULL,NULL,IX86_CPU186|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x70*/ DECLARE_BASE_INSN("jo","jo","jo",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x71*/ DECLARE_BASE_INSN("jno","jno","jno",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x72*/ DECLARE_BASE_INSN("jc","jc","jc",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x73*/ DECLARE_BASE_INSN("jnc","jnc","jnc",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x74*/ DECLARE_BASE_INSN("je","je","je",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x75*/ DECLARE_BASE_INSN("jne","jne","jne",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x76*/ DECLARE_BASE_INSN("jna","jna","jna",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x77*/ DECLARE_BASE_INSN("ja","ja","ja",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x78*/ DECLARE_BASE_INSN("js","js","js",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x79*/ DECLARE_BASE_INSN("jns","jns","jns",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x7A*/ DECLARE_BASE_INSN("jp","jp","jp",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x7B*/ DECLARE_BASE_INSN("jnp","jnp","jnp",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x7C*/ DECLARE_BASE_INSN("jl","jl","jl",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x7D*/ DECLARE_BASE_INSN("jnl","jnl","jnl",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x7E*/ DECLARE_BASE_INSN("jle","jle","jle",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x7F*/ DECLARE_BASE_INSN("jg","jg","jg",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0x80*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgOp1,ix86_ArgOp1,IX86_CPU086,K64_ATHLON),
  /*0x81*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgOp1,ix86_ArgOp1,IX86_CPU086,K64_ATHLON),
  /*0x82*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgOp2,ix86_ArgOp2,IX86_CPU086,K64_ATHLON),
  /*0x83*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgOp2,ix86_ArgOp2,IX86_CPU086,K64_ATHLON),
  /*0x84*/ DECLARE_BASE_INSN("test","test","test",ix86_ArgModRMnDnW,ix86_ArgModRMnDnW,IX86_CPU086,K64_ATHLON),
  /*0x85*/ DECLARE_BASE_INSN("test","test","test",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU086,K64_ATHLON),
  /*0x86*/ DECLARE_BASE_INSN("xchg","xchg","xchg",ix86_ArgModRMDnW,ix86_ArgModRMDnW,IX86_CPU086,K64_ATHLON),
  /*0x87*/ DECLARE_BASE_INSN("xchg","xchg","xchg",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU086,K64_ATHLON),
  /*0x88*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgModRMnDnW,ix86_ArgModRMnDnW,IX86_CPU086,K64_ATHLON),
  /*0x89*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU086,K64_ATHLON),
  /*0x8A*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgModRMDnW,ix86_ArgModRMDnW,IX86_CPU086,K64_ATHLON),
  /*0x8B*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU086,K64_ATHLON),
  /*0x8C*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_SMov,ix86_SMov,IX86_CPU086,K64_ATHLON),
  /*0x8D*/ DECLARE_BASE_INSN("lea","lea","lea",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU086,K64_ATHLON),
  /*0x8E*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_SMov,ix86_SMov,IX86_CPU086,K64_ATHLON),
  /*0x8F*/ DECLARE_BASE_INSN("pop","pop","pop",ix86_ArgMod,ix86_ArgMod,IX86_CPU086,K64_ATHLON),
  /*0x90*/ DECLARE_BASE_INSN("nop","nop","nop",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0x91*/ DECLARE_BASE_INSN("xchg","xchg","xchg",ix86_ArgAXIReg,ix86_ArgAXIReg,IX86_CPU086,K64_ATHLON),
  /*0x92*/ DECLARE_BASE_INSN("xchg","xchg","xchg",ix86_ArgAXIReg,ix86_ArgAXIReg,IX86_CPU086,K64_ATHLON),
  /*0x93*/ DECLARE_BASE_INSN("xchg","xchg","xchg",ix86_ArgAXIReg,ix86_ArgAXIReg,IX86_CPU086,K64_ATHLON),
  /*0x94*/ DECLARE_BASE_INSN("xchg","xchg","xchg",ix86_ArgAXIReg,ix86_ArgAXIReg,IX86_CPU086,K64_ATHLON),
  /*0x95*/ DECLARE_BASE_INSN("xchg","xchg","xchg",ix86_ArgAXIReg,ix86_ArgAXIReg,IX86_CPU086,K64_ATHLON),
  /*0x96*/ DECLARE_BASE_INSN("xchg","xchg","xchg",ix86_ArgAXIReg,ix86_ArgAXIReg,IX86_CPU086,K64_ATHLON),
  /*0x97*/ DECLARE_BASE_INSN("xchg","xchg","xchg",ix86_ArgAXIReg,ix86_ArgAXIReg,IX86_CPU086,K64_ATHLON),
  /*0x98*/ DECLARE_BASE_INSN("cbw","cwde","cdqe",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0x99*/ DECLARE_BASE_INSN("cwd","cdq","cqo",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0x9A*/ DECLARE_BASE_INSN("callf16","callf32","???",ix86_ArgFar,ix86_ArgFar,IX86_CPU086,K64_ATHLON),
  /*0x9B*/ DECLARE_BASE_INSN("wait","wait","wait",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0x9C*/ DECLARE_BASE_INSN("pushfw","pushfd","pushfq",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x9D*/ DECLARE_BASE_INSN("popfw","popfd","popfq",NULL,NULL,IX86_CPU086|IX86_CPL0,K64_ATHLON|K64_NOCOMPAT|K64_CPL0),
  /*0x9E*/ DECLARE_BASE_INSN("sahf","sahf","sahf",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0x9F*/ DECLARE_BASE_INSN("lahf","lahf","lahf",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xA0*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgAXMem,ix86_ArgAXMem,IX86_CPU086,K64_ATHLON),
  /*0xA1*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgAXMem,ix86_ArgAXMem,IX86_CPU086,K64_ATHLON),
  /*0xA2*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgAXMem,ix86_ArgAXMem,IX86_CPU086,K64_ATHLON),
  /*0xA3*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgAXMem,ix86_ArgAXMem,IX86_CPU086,K64_ATHLON),
  /*0xA4*/ DECLARE_BASE_INSN("movsb","movsb","movsb",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xA5*/ DECLARE_BASE_INSN("movsw","movsd","movsq",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xA6*/ DECLARE_BASE_INSN("cmpsb","cmpsb","cmpsb",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xA7*/ DECLARE_BASE_INSN("cmpsw","cmpsd","cmpsq",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xA8*/ DECLARE_BASE_INSN("test","test","test",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0xA9*/ DECLARE_BASE_INSN("test","test","test",ix86_ArgAXDigit,ix86_ArgAXDigit,IX86_CPU086,K64_ATHLON),
  /*0xAA*/ DECLARE_BASE_INSN("stosb","stosb","stosb",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xAB*/ DECLARE_BASE_INSN("stosw","stosd","stosq",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xAC*/ DECLARE_BASE_INSN("lodsb","lodsb","lodsb",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xAD*/ DECLARE_BASE_INSN("lodsw","lodsd","lodsq",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xAE*/ DECLARE_BASE_INSN("scasb","scasb","scasb",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xAF*/ DECLARE_BASE_INSN("scasw","scasd","scasq",NULL,NULL,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xB0*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xB1*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xB2*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xB3*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xB4*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xB5*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xB6*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xB7*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xB8*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xB9*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xBA*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xBB*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xBC*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xBD*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xBE*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xBF*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgIRegDigit,ix86_ArgIRegDigit,IX86_CPU086,K64_ATHLON),
  /*0xC0*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ShOp2,ix86_ShOp2,IX86_CPU186,K64_ATHLON),
  /*0xC1*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ShOp2,ix86_ShOp2,IX86_CPU186,K64_ATHLON),
  /*0xC2*/ DECLARE_BASE_INSN("retn16","retn32","retn32",ix86_ArgWord,ix86_ArgWord,IX86_CPU086,K64_ATHLON),
  /*0xC3*/ DECLARE_BASE_INSN("retn16","retn32","retn32",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xC4*/ DECLARE_BASE_INSN("les","les","???",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU086,K64_ATHLON),
  /*0xC5*/ DECLARE_BASE_INSN("lds","lds","???",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU086,K64_ATHLON),
  /*0xC6*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgRMDigit,ix86_ArgRMDigit,IX86_CPU086,K64_ATHLON),
  /*0xC7*/ DECLARE_BASE_INSN("mov","mov","mov",ix86_ArgRMDigit,ix86_ArgRMDigit,IX86_CPU086,K64_ATHLON),
  /*0xC8*/ DECLARE_BASE_INSN("enter","enter","enter",ix86_ArgWord_Byte,ix86_ArgWord_Byte,IX86_CPU186,K64_ATHLON),
  /*0xC9*/ DECLARE_BASE_INSN("leave","leave","leave",NULL,NULL,IX86_CPU186,K64_ATHLON),
  /*0xCA*/ DECLARE_BASE_INSN("retf16","retf32","retf32",ix86_ArgWord,ix86_ArgWord,IX86_CPU086,K64_ATHLON),
  /*0xCB*/ DECLARE_BASE_INSN("retf16","retf32","retf32",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xCC*/ DECLARE_BASE_INSN("int3","int3","int3",NULL,NULL,IX86_CPU086|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0xCD*/ DECLARE_BASE_INSN("int","int","int",ix86_ArgByte,ix86_ArgByte,IX86_CPU086,K64_ATHLON),
  /*0xCE*/ DECLARE_BASE_INSN("into16","into32","???",NULL,NULL,IX86_CPU086|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0xCF*/ DECLARE_BASE_INSN("iret16","iret32","iretq",NULL,NULL,IX86_CPU086|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0xD0*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ShOp1,ix86_ShOp1,IX86_CPU086,K64_ATHLON),
  /*0xD1*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ShOp1,ix86_ShOp1,IX86_CPU086,K64_ATHLON),
  /*0xD2*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ShOpCL,ix86_ShOpCL,IX86_CPU086,K64_ATHLON),
  /*0xD3*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ShOpCL,ix86_ShOpCL,IX86_CPU086,K64_ATHLON),
  /*0xD4*/ DECLARE_BASE_INSN("aam on","aam on","???",ix86_ArgByte,ix86_ArgByte,IX86_CPU086,K64_ATHLON),
  /*0xD5*/ DECLARE_BASE_INSN("aad on","aad on","???",ix86_ArgByte,ix86_ArgByte,IX86_CPU086,K64_ATHLON),
  /*0xD6*/ DECLARE_BASE_INSN("salc","salc","salc",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xD7*/ DECLARE_BASE_INSN("xlat","xlat","xlat",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xD8*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086,K64_ATHLON|K64_FPU),
  /*0xD9*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086,K64_ATHLON|K64_FPU),
  /*0xDA*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086,K64_ATHLON|K64_FPU),
  /*0xDB*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086,K64_ATHLON|K64_FPU),
  /*0xDC*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086,K64_ATHLON|K64_FPU),
  /*0xDD*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086,K64_ATHLON|K64_FPU),
  /*0xDE*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086,K64_ATHLON|K64_FPU),
  /*0xDF*/ DECLARE_BASE_INSN("esc","esc","esc",ix86_FPUCmd,ix86_FPUCmd,IX86_CPU086,K64_ATHLON|K64_FPU),
  /*0xE0*/ DECLARE_BASE_INSN("loopnz","loopnz","loopnz",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0xE1*/ DECLARE_BASE_INSN("loopz","loopz","loopz",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0xE2*/ DECLARE_BASE_INSN("loop","loop","loop",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0xE3*/ DECLARE_BASE_INSN("jcxz","jecxz","jrcxz",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0xE4*/ DECLARE_BASE_INSN("in","in","in",ix86_InOut,ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xE5*/ DECLARE_BASE_INSN("in","in","in",ix86_InOut,ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xE6*/ DECLARE_BASE_INSN("out","out","out",ix86_InOut,ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xE7*/ DECLARE_BASE_INSN("out","out","out",ix86_InOut,ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xE8*/ DECLARE_BASE_INSN("calln16","calln32","calln32",ix86_ArgNear,ix86_ArgNear,IX86_CPU086,K64_ATHLON),
  /*0xE9*/ DECLARE_BASE_INSN("jmpn16","jmpn32","jmpn32",ix86_ArgNear,ix86_ArgNear,IX86_CPU086,K64_ATHLON),
  /*0xEA*/ DECLARE_BASE_INSN("jmpf16","jmpf32","???",ix86_ArgFar,ix86_ArgFar,IX86_CPU086,K64_ATHLON),
  /*0xEB*/ DECLARE_BASE_INSN("jmps","jmps","jmps",ix86_ArgShort,ix86_ArgShort,IX86_CPU086,K64_ATHLON),
  /*0xEC*/ DECLARE_BASE_INSN("in","in","in",ix86_InOut,ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xED*/ DECLARE_BASE_INSN("in","in","in",ix86_InOut,ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xEE*/ DECLARE_BASE_INSN("out","out","out",ix86_InOut,ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xEF*/ DECLARE_BASE_INSN("out","out","out",ix86_InOut,ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xF0*/ DECLARE_BASE_INSN("lock","lock","lock",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xF1*/ DECLARE_BASE_INSN("icebp","icebp","icebp",NULL,NULL,IX86_CPU386,K64_ATHLON),
  /*0xF2*/ DECLARE_BASE_INSN("repne","repne","repne",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xF3*/ DECLARE_BASE_INSN("rep","rep","rep",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xF4*/ DECLARE_BASE_INSN("hlt","hlt","hlt",NULL,NULL,IX86_CPU086|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0xF5*/ DECLARE_BASE_INSN("cmc","cmc","cmc",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xF6*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgGrp1,ix86_ArgGrp1,IX86_CPU086,K64_ATHLON),
  /*0xF7*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgGrp1,ix86_ArgGrp1,IX86_CPU086,K64_ATHLON),
  /*0xF8*/ DECLARE_BASE_INSN("clc","clc","clc",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xF9*/ DECLARE_BASE_INSN("stc","stc","stc",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xFA*/ DECLARE_BASE_INSN("cli","cli","cli",NULL,NULL,IX86_CPU086|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0xFB*/ DECLARE_BASE_INSN("sti","sti","sti",NULL,NULL,IX86_CPU086|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0xFC*/ DECLARE_BASE_INSN("cld","cld","cld",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xFD*/ DECLARE_BASE_INSN("std","std","std",NULL,NULL,IX86_CPU086,K64_ATHLON),
  /*0xFE*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgGrp2,ix86_ArgGrp2,IX86_CPU086,K64_ATHLON),
  /*0xFF*/ DECLARE_BASE_INSN("!!!","!!!","!!!",ix86_ArgGrp2,ix86_ArgGrp2,IX86_CPU086,K64_ATHLON)
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
  /*0x00*/ DECLARE_EX_INSN("pshufb", "pshufb",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
  /*0x01*/ DECLARE_EX_INSN("phaddw", "phaddw",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
  /*0x02*/ DECLARE_EX_INSN("phaddd", "phaddd",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
  /*0x03*/ DECLARE_EX_INSN("phaddsw", "phaddsw",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
  /*0x04*/ DECLARE_EX_INSN("pmaddubsw", "pmaddubsw",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
  /*0x05*/ DECLARE_EX_INSN("phsubw", "phsubw",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
  /*0x06*/ DECLARE_EX_INSN("phsubd", "phsubd",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
  /*0x07*/ DECLARE_EX_INSN("phsubsw", "phsubsw",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
  /*0x08*/ DECLARE_EX_INSN("psingb", "psignb",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
  /*0x09*/ DECLARE_EX_INSN("psignw", "psignw",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
  /*0x0A*/ DECLARE_EX_INSN("psignd", "psignd",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
  /*0x0B*/ DECLARE_EX_INSN("pmulhrsw", "pmulhrsw",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
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
  /*0x1C*/ DECLARE_EX_INSN("pabsb", "pabsb",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
  /*0x1D*/ DECLARE_EX_INSN("pabsw", "pabsw",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
  /*0x1E*/ DECLARE_EX_INSN("pabsd", "pabsd",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
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
  /*0x0F*/ DECLARE_EX_INSN("palignr", "palignr",ix86_ArgMMXnD,ix86_ArgMMXnD, IX86_P6|IX86_MMX, K64_ATHLON|K64_MMX),
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
  /*0x00*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgExGr0,ix86_ArgExGr0,IX86_CPU286,K64_ATHLON|K64_CPL0),
  /*0x01*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgExGr1,ix86_ArgExGr1,IX86_CPU286|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x02*/ DECLARE_EX_INSN("lar","lar",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU286|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x03*/ DECLARE_EX_INSN("lsl","lsl",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU286|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x04*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN("syscall","syscall",NULL,NULL,IX86_K6|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x06*/ DECLARE_EX_INSN("clts","clts",NULL,NULL,IX86_CPU286|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x07*/ DECLARE_EX_INSN("sysret","sysret",NULL,NULL,IX86_K6|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x08*/ DECLARE_EX_INSN("invd","invd",NULL,NULL,IX86_CPU486|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x09*/ DECLARE_EX_INSN("wbinvd","wbinvd",NULL,NULL,IX86_CPU486|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x0A*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN("ud","ud",NULL,NULL,IX86_CPU686,K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN("!!!","!!!",ix86_3dNowPrefetchGrp,ix86_3dNowPrefetchGrp,IX86_3DNOW,K64_ATHLON|K64_MMX),
  /*0x0E*/ DECLARE_EX_INSN("femms","femms",NULL,NULL,IX86_3DNOW,K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN("!!!","!!!",ix86_3dNowOpCodes,ix86_3dNowOpCodes,IX86_3DNOW,K64_ATHLON|K64_MMX),
  /*0x10*/ DECLARE_EX_INSN("movups","movups",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x11*/ DECLARE_EX_INSN("movups","movups",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x12*/ DECLARE_EX_INSN("movlps","movlps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x13*/ DECLARE_EX_INSN("movlps","movlps",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x14*/ DECLARE_EX_INSN("unpcklps","unpcklps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x15*/ DECLARE_EX_INSN("unpckhps","unpckhps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x16*/ DECLARE_EX_INSN("movhps","movhps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x17*/ DECLARE_EX_INSN("movhps","movhps",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x18*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgKatmaiGrp2,ix86_ArgKatmaiGrp2,IX86_P3|IX86_SSE,K64_ATHLON|IX86_SSE),
  /*0x19*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN("nop","nop",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_P5,K64_FAM9),
  /*0x20*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x21*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x22*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x23*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x24*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x25*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x26*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x27*/ DECLARE_EX_INSN("mov","mov",ix86_ArgMovXRY,ix86_ArgMovXRY,IX86_CPU386|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x28*/ DECLARE_EX_INSN("movaps","movaps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x29*/ DECLARE_EX_INSN("movaps","movaps",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2A*/ DECLARE_EX_INSN("cvtpi2ps","cvtpi2ps",ix86_ArgXMMXMMnD,ix86_ArgXMMXMMnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2B*/ DECLARE_EX_INSN("movntps","movntps",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2C*/ DECLARE_EX_INSN("cvttps2pi","cvttps2pi",ix86_ArgMMXMMXnD,ix86_ArgMMXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2D*/ DECLARE_EX_INSN("cvtps2pi","cvtps2pi",ix86_ArgMMXMMXnD,ix86_ArgMMXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2E*/ DECLARE_EX_INSN("ucomiss","ucomiss",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2F*/ DECLARE_EX_INSN("comiss","comiss",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x30*/ DECLARE_EX_INSN("wrmsr","wrmsr",NULL,NULL,IX86_CPU586|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x31*/ DECLARE_EX_INSN("rdtsc","rdtsc",NULL,NULL,IX86_CPU586|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x32*/ DECLARE_EX_INSN("rdmsr","rdmsr",NULL,NULL,IX86_CPU586|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x33*/ DECLARE_EX_INSN("rdpmc","rdpmc",NULL,NULL,IX86_CPU686|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0x34*/ DECLARE_EX_INSN("sysenter","???",NULL,NULL,IX86_P2|IX86_CPL0,K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN("sysexit","???",NULL,NULL,IX86_P2|IX86_CPL0,K64_ATHLON),
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
  /*0x40*/ DECLARE_EX_INSN("cmovo","cmovo",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN("cmovno","cmovno",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN("cmovc","cmovc",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN("cmovnc","cmovnc",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN("cmove","cmove",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN("cmovne","cmovne",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN("cmovna","cmovna",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN("cmova","cmova",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN("cmovs","cmovs",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN("cmovns","cmovns",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN("cmovp","cmovp",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN("cmovnp","cmovnp",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN("cmovl","cmovl",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN("cmovnl","cmovnl",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN("cmovle","cmovle",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN("cmovg","cmovg",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU686,K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN("movmskps","movmskps",ix86_ArgXMMXRD,ix86_ArgXMMXRD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x51*/ DECLARE_EX_INSN("sqrtps","sqrtps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x52*/ DECLARE_EX_INSN("rsqrtps","rsqrtps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x53*/ DECLARE_EX_INSN("rcpps","rcpps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x54*/ DECLARE_EX_INSN("andps","andps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x55*/ DECLARE_EX_INSN("andnps","andnps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x56*/ DECLARE_EX_INSN("orps","orps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x57*/ DECLARE_EX_INSN("xorps","xorps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x58*/ DECLARE_EX_INSN("addps","addps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x59*/ DECLARE_EX_INSN("mulps","mulps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5A*/ DECLARE_EX_INSN("cvtps2pd","cvtps2pd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5B*/ DECLARE_EX_INSN("cvtdq2ps","cvtdq2ps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5C*/ DECLARE_EX_INSN("subps","subps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5D*/ DECLARE_EX_INSN("minps","minps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5E*/ DECLARE_EX_INSN("divps","divps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5F*/ DECLARE_EX_INSN("maxps","maxps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x60*/ DECLARE_EX_INSN("punpcklbw","punpcklbw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x61*/ DECLARE_EX_INSN("punpcklwd","punpcklwd",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x62*/ DECLARE_EX_INSN("punpckldq","punpckldq",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x63*/ DECLARE_EX_INSN("packsswb","packsswb",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x64*/ DECLARE_EX_INSN("pcmpgtb","pcmpgtb",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x65*/ DECLARE_EX_INSN("pcmpgtw","pcmpgtw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x66*/ DECLARE_EX_INSN("pcmpgtd","pcmpgtd",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x67*/ DECLARE_EX_INSN("packuswb","packuswb",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x68*/ DECLARE_EX_INSN("punpckhbw","punpckhbw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x69*/ DECLARE_EX_INSN("punpckhwd","punpckhwd",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x6A*/ DECLARE_EX_INSN("punpckhdq","punpckhdq",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x6B*/ DECLARE_EX_INSN("packssdw","packssdw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x6C*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKMMX,K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKMMX,K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN("movd","movd",ix86_ArgMMXRnD,ix86_ArgMMXRnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x6F*/ DECLARE_EX_INSN("movq","movq",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x70*/ DECLARE_EX_INSN("pshufw","pshufw",ix86_ArgMMRMDigit,ix86_ArgMMRMDigit,IX86_P3|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x71*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgMMXGr1,ix86_ArgMMXGr1,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x72*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgMMXGr2,ix86_ArgMMXGr2,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x73*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgMMXGr3,ix86_ArgMMXGr3,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x74*/ DECLARE_EX_INSN("pcmpeqb","pcmpeqb",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN("pcmpeqw","pcmpeqw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN("pcmpeqd","pcmpeqd",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN("emms","emms",NULL,NULL,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x78*/ DECLARE_EX_INSN("vmread","vmread",ix86_ArgMod,ix86_ArgMod,IX86_P6,K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN("vmwrite","vmwrite",ix86_ArgMod,ix86_ArgMod,IX86_P6,K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN("svldt","???",ix86_ArgMod,ix86_ArgMod,IX86_CYRIX486,K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN("rsldt","???",ix86_ArgMod,ix86_ArgMod,IX86_CYRIX486,K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN("svts","???",ix86_ArgMod,ix86_ArgMod,IX86_CYRIX486,K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN("rsts","???",ix86_ArgMod,ix86_ArgMod,IX86_CYRIX486,K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN("movd","movd",ix86_ArgMMXRD,ix86_ArgMMXRD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x7F*/ DECLARE_EX_INSN("movq","movq",ix86_ArgMMXD,ix86_ArgMMXD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0x80*/ DECLARE_EX_INSN("jo","jo",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN("jno","jno",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN("jc","jc",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN("jnc","jnc",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN("je","je",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN("jne","jne",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN("jna","jna",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN("ja","ja",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN("js","js",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN("jns","jns",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN("jp","jp",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN("jnp","jnp",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN("jl","jl",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN("jnl","jnl",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN("jle","jle",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN("jg","jg",ix86_ArgNear,ix86_ArgNear,IX86_CPU386,K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN("seto","seto",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN("setno","setno",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN("setc","setc",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN("setnc","setnc",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN("sete","sete",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN("setne","setne",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN("setna","setna",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN("seta","seta",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN("sets","sets",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN("setns","setns",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN("setp","setp",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN("setnp","setnp",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN("setl","setl",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN("setnl","setnl",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN("setle","setle",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN("setg","setg",ix86_ArgModB,ix86_ArgModB,IX86_CPU386,K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN("push","push",ix86_ArgFS,ix86_ArgFS,IX86_CPU386,K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN("pop","pop",ix86_ArgFS,ix86_ArgFS,IX86_CPU386,K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN("cpuid","cpuid",NULL,NULL,IX86_CPU486,K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN("bt","bt",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU386,K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN("shld","shld",ix86_DblShift,ix86_DblShift,IX86_CPU386,K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN("shld","shld",ix86_DblShift,ix86_DblShift,IX86_CPU386,K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN("push","push",ix86_ArgGS,ix86_ArgGS,IX86_CPU386,K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN("pop","pop",ix86_ArgGS,ix86_ArgGS,IX86_CPU386,K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN("rsm","rsm",NULL,NULL,IX86_CPU586|IX86_CPL0,K64_ATHLON|K64_CPL0),
  /*0xAB*/ DECLARE_EX_INSN("bts","bts",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU386,K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN("shrd","shrd",ix86_DblShift,ix86_DblShift,IX86_CPU386,K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN("shrd","shrd",ix86_DblShift,ix86_DblShift,IX86_CPU386,K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgKatmaiGrp1,ix86_ArgKatmaiGrp1,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xAF*/ DECLARE_EX_INSN("imul","imul",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU386,K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN("cmpxchg","cmpxchg",ix86_ArgModRMnDnW,ix86_ArgModRMnDnW,IX86_CPU486,K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN("cmpxchg","cmpxchg",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU486,K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN("lss","lss",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU386,K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN("btr","btr",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU386,K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN("lfs","lfs",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU386,K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN("lgs","lgs",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU386,K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN("movzx","movzx",ix86_ArgMovYX,ix86_ArgMovYX,IX86_CPU386,K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN("movzx","movzx",ix86_ArgMovYX,ix86_ArgMovYX,IX86_CPU386,K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN("???","???",NULL,NULL,IX86_UNKCPU,K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN("ud2","ud2",NULL,NULL,IX86_CPU686,K64_ATHLON|K64_CPL0),
  /*0xBA*/ DECLARE_EX_INSN("!!!","!!!",ix86_BitGrp,ix86_BitGrp,IX86_CPU386,K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN("btc","btc",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU386,K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN("bsf","bsf",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU386,K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN("bsr","bsr",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_CPU386,K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN("movsx","movsx",ix86_ArgMovYX,ix86_ArgMovYX,IX86_CPU386,K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN("movsx","movsx",ix86_ArgMovYX,ix86_ArgMovYX,IX86_CPU386,K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN("xadd","xadd",ix86_ArgModRMnDnW,ix86_ArgModRMnDnW,IX86_CPU486,K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN("xadd","xadd",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_CPU486,K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN("cmpps","cmpps",ix86_ArgXMMCmp,ix86_ArgXMMCmp,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xC3*/ DECLARE_EX_INSN("movnti","movnti",ix86_ArgModRMnDW,ix86_ArgModRMnDW,IX86_P4,K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN("pinsrw","pinsrw",ix86_ArgXMMRegDigit,ix86_ArgXMMRegDigit,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xC5*/ DECLARE_EX_INSN("pextrw","pextrw",ix86_ArgRegXMMDigit,ix86_ArgRegXMMDigit,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xC6*/ DECLARE_EX_INSN("shufps","shufps",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xC7*/ DECLARE_EX_INSN("!!!","!!!",ix86_0FVMX,ix86_0FVMX,IX86_CPU586,K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN("bswap","bswap",ix86_ArgIReg,ix86_ArgIReg,IX86_CPU486,K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN("bswap","bswap",ix86_ArgIReg,ix86_ArgIReg,IX86_CPU486,K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN("bswap","bswap",ix86_ArgIReg,ix86_ArgIReg,IX86_CPU486,K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN("bswap","bswap",ix86_ArgIReg,ix86_ArgIReg,IX86_CPU486,K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN("bswap","bswap",ix86_ArgIReg,ix86_ArgIReg,IX86_CPU486,K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN("bswap","bswap",ix86_ArgIReg,ix86_ArgIReg,IX86_CPU486,K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN("bswap","bswap",ix86_ArgIReg,ix86_ArgIReg,IX86_CPU486,K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN("bswap","bswap",ix86_ArgIReg,ix86_ArgIReg,IX86_CPU486,K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN("???","???",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_UNKMMX,K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN("psrlw","psrlw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xD2*/ DECLARE_EX_INSN("psrld","psrld",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xD3*/ DECLARE_EX_INSN("psrlq","psrlq",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xD4*/ DECLARE_EX_INSN("paddq","paddq",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_P4|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xD5*/ DECLARE_EX_INSN("pmullw","pmullw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xD6*/ DECLARE_EX_INSN("???","???",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_UNKMMX,K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN("pmovmskb","pmovmskb",ix86_ArgRMMXRevnD,ix86_ArgRMMXRevnD,IX86_P3|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xD8*/ DECLARE_EX_INSN("psubusb","psubusb",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xD9*/ DECLARE_EX_INSN("psubusw","psubusw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xDA*/ DECLARE_EX_INSN("pminub","pminub",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_P3|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xDB*/ DECLARE_EX_INSN("pand","pand",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xDC*/ DECLARE_EX_INSN("paddusb","paddusb",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xDD*/ DECLARE_EX_INSN("paddusw","paddusw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xDE*/ DECLARE_EX_INSN("pmaxub","pmaxub",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_P3|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xDF*/ DECLARE_EX_INSN("pandn","pandn",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xE0*/ DECLARE_EX_INSN("pavgb","pavgb",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_P3|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xE1*/ DECLARE_EX_INSN("psraw","psraw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xE2*/ DECLARE_EX_INSN("psrad","psrad",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xE3*/ DECLARE_EX_INSN("pavgw","pavgw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_P3|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xE4*/ DECLARE_EX_INSN("pmulhuw","pmulhuw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_P3|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xE5*/ DECLARE_EX_INSN("pmulhw","pmulhw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xE6*/ DECLARE_EX_INSN("???","???",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_UNKMMX,K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN("movntq","movntq",ix86_ArgMMXD,ix86_ArgMMXD,IX86_P3|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xE8*/ DECLARE_EX_INSN("psubsb","psubsb",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xE9*/ DECLARE_EX_INSN("psubsw","psubsw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xEA*/ DECLARE_EX_INSN("pminsw","pminsw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_P3|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xEB*/ DECLARE_EX_INSN("por","por",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xEC*/ DECLARE_EX_INSN("paddsb","paddsb",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xED*/ DECLARE_EX_INSN("paddsw","paddsw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xEE*/ DECLARE_EX_INSN("pmaxsw","pmaxsw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_P3|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xEF*/ DECLARE_EX_INSN("pxor","pxor",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xF0*/ DECLARE_EX_INSN("???","???",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_UNKMMX,K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN("psllw","psllw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xF2*/ DECLARE_EX_INSN("pslld","pslld",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xF3*/ DECLARE_EX_INSN("psllq","psllq",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xF4*/ DECLARE_EX_INSN("pmuludq","pmuludq",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_P4|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xF5*/ DECLARE_EX_INSN("pmaddwd","pmaddwd",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xF6*/ DECLARE_EX_INSN("psadbw","psadbw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_P3|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xF7*/ DECLARE_EX_INSN("maskmovq","maskmovq",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_P3|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xF8*/ DECLARE_EX_INSN("psubb","psubb",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xF9*/ DECLARE_EX_INSN("psubw","psubw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xFA*/ DECLARE_EX_INSN("psubd","psubd",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xFB*/ DECLARE_EX_INSN("psubq","psubq",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_P4|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xFC*/ DECLARE_EX_INSN("paddb","paddb",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xFD*/ DECLARE_EX_INSN("paddw","paddw",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xFE*/ DECLARE_EX_INSN("paddd","paddd",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_CPU586|IX86_MMX,K64_ATHLON|K64_MMX),
  /*0xFF*/ DECLARE_EX_INSN("???","???",ix86_ArgMMXnD,ix86_ArgMMXnD,IX86_UNKMMX,K64_ATHLON)
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
  /*0x00*/ DECLARE_EX_INSN("pshufb", "pshufb",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x01*/ DECLARE_EX_INSN("phaddw", "phaddw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x02*/ DECLARE_EX_INSN("phaddd", "phaddd",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x03*/ DECLARE_EX_INSN("phaddsw", "phaddsw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x04*/ DECLARE_EX_INSN("pmaddubsw", "pmaddubsw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x05*/ DECLARE_EX_INSN("phsubw", "phsubw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x06*/ DECLARE_EX_INSN("phsubd", "phsubd",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x07*/ DECLARE_EX_INSN("phsubsw", "phsubsw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x08*/ DECLARE_EX_INSN("psingb", "psignb",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x09*/ DECLARE_EX_INSN("psignw", "psignw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x0A*/ DECLARE_EX_INSN("psignd", "psignd",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x0B*/ DECLARE_EX_INSN("pmulhrsw", "pmulhrsw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN("pblendvb", "pblendvb",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN("blendvps", "blendvps",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x15*/ DECLARE_EX_INSN("blendvpd", "blendvpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN("ptest", "ptest",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN("pabsb", "pabsb",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x1D*/ DECLARE_EX_INSN("pabsw", "pabsw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x1E*/ DECLARE_EX_INSN("pabsd", "pabsd",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN("pmovsxbw", "pmovsxbw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x21*/ DECLARE_EX_INSN("pmovsxbd", "pmovsxbd",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x22*/ DECLARE_EX_INSN("pmovsxbq", "pmovsxbq",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x23*/ DECLARE_EX_INSN("pmovsxwd", "pmovsxwd",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x24*/ DECLARE_EX_INSN("pmovsxwq", "pmovsxwq",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x25*/ DECLARE_EX_INSN("pmovsxdq", "pmovsxdq",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN("pmuldq", "pmuldq",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x29*/ DECLARE_EX_INSN("pcmpeqq", "pcmpeqq",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x2A*/ DECLARE_EX_INSN("movntdqa", "movntdqa",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x2B*/ DECLARE_EX_INSN("packusdw", "packusdw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN("pmovzxbw", "pmovzxbw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x31*/ DECLARE_EX_INSN("pmovzxbd", "pmovzxbd",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x32*/ DECLARE_EX_INSN("pmovzxbq", "pmovzxbq",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x33*/ DECLARE_EX_INSN("pmovzxwd", "pmovzxwd",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x34*/ DECLARE_EX_INSN("pmovzxwq", "pmovzxwq",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x35*/ DECLARE_EX_INSN("pmovzxdq", "pmovzxdq",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN("pminsb", "pminb",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x39*/ DECLARE_EX_INSN("pminsd", "pminsd",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x3A*/ DECLARE_EX_INSN("pminuw", "pminuw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x3B*/ DECLARE_EX_INSN("pminud", "pminud",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x3C*/ DECLARE_EX_INSN("pmaxsb", "pmaxsb",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x3D*/ DECLARE_EX_INSN("pmaxsd", "pmaxsd",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x3E*/ DECLARE_EX_INSN("pmaxuw", "pmaxuw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x3F*/ DECLARE_EX_INSN("pmaxud", "pmaxud",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x40*/ DECLARE_EX_INSN("pmulld", "pmulld",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x41*/ DECLARE_EX_INSN("phminposuw", "phminposuw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
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

const ix86_ExOpcodes ix86_660F3A_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN("roundps", "roundps",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x09*/ DECLARE_EX_INSN("roundpd", "roundpd",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x0A*/ DECLARE_EX_INSN("roundss", "roundss",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x0B*/ DECLARE_EX_INSN("roundsd", "roundsd",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x0C*/ DECLARE_EX_INSN("blendps", "blendps",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x0D*/ DECLARE_EX_INSN("blendpd", "blendpd",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x0E*/ DECLARE_EX_INSN("pblendw", "pblendw",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x0F*/ DECLARE_EX_INSN("palignr", "palignr",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P6|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN("pextrb", "pextrb",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN("pextrd", "pextrq",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x17*/ DECLARE_EX_INSN("extractps", "extractps",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, NULL, NULL, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN("pinsrb", "pinsrb",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x21*/ DECLARE_EX_INSN("insertps", "insertps",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x22*/ DECLARE_EX_INSN("pinsrd", "pinsrq",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
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
  /*0x40*/ DECLARE_EX_INSN("dpps", "dpps",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x41*/ DECLARE_EX_INSN("dppd", "dppd",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x42*/ DECLARE_EX_INSN("mpsadbw", "mpsadbw",ix86_ArgXMMXnD,ix86_ArgXMMXnD, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
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
  /*0x60*/ DECLARE_EX_INSN("pcmpestrm", "pcmpestrm",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x61*/ DECLARE_EX_INSN("pcmpestri", "pcmpestri",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x62*/ DECLARE_EX_INSN("pcmpestrm", "pcmpestrm",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
  /*0x63*/ DECLARE_EX_INSN("pcmpistri", "pcmpistri",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit, IX86_P7|IX86_SSE, K64_ATHLON|K64_SSE),
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

const ix86_ExOpcodes ix86_660F_PentiumTable[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x01*/ DECLARE_EX_INSN(ix86_660F01_Table,ix86_660F01_Table,NULL,NULL,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x02*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x03*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x04*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x05*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x06*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x07*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x08*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x09*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x10*/ DECLARE_EX_INSN("movupd","movupd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x11*/ DECLARE_EX_INSN("movupd","movupd",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x12*/ DECLARE_EX_INSN("movlpd","movlpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x13*/ DECLARE_EX_INSN("movlpd","movlpd",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x14*/ DECLARE_EX_INSN("unpcklpd","unpcklpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x15*/ DECLARE_EX_INSN("unpckhpd","unpckhpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x16*/ DECLARE_EX_INSN("movhpd","movhpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x17*/ DECLARE_EX_INSN("movhpd","movhpd",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x18*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x19*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x20*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x21*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x22*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x23*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x24*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x25*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x26*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x27*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x28*/ DECLARE_EX_INSN("movapd","movapd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x29*/ DECLARE_EX_INSN("movapd","movapd",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2A*/ DECLARE_EX_INSN("cvtpi2pd","cvtpi2pd",ix86_ArgXMMXMMnD,ix86_ArgXMMXMMnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2B*/ DECLARE_EX_INSN("movntpd","movntpd",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2C*/ DECLARE_EX_INSN("cvttpd2pi","cvttpd2pi",ix86_ArgMMXMMXnD,ix86_ArgMMXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2D*/ DECLARE_EX_INSN("cvtpd2pi","cvtpd2pi",ix86_ArgMMXMMXnD,ix86_ArgMMXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2E*/ DECLARE_EX_INSN("ucomisd","ucomisd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2F*/ DECLARE_EX_INSN("comisd","comisd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x30*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x31*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x32*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x33*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x34*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x35*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x36*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x37*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x38*/ DECLARE_EX_INSN(ix86_660F38_Table,ix86_660F38_Table,NULL,NULL,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x39*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3A*/ DECLARE_EX_INSN(ix86_660F3A_Table,ix86_660F3A_Table,NULL,NULL,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x3B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x40*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x41*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x42*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x43*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x44*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x45*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x46*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x47*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x48*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x49*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x50*/ DECLARE_EX_INSN("movmskpd","movmskpd",ix86_ArgXMMXRD,ix86_ArgXMMXRD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x51*/ DECLARE_EX_INSN("sqrtpd","sqrtpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x52*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x53*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x54*/ DECLARE_EX_INSN("andpd","andpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x55*/ DECLARE_EX_INSN("andnpd","andnpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x56*/ DECLARE_EX_INSN("orpd","orpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x57*/ DECLARE_EX_INSN("xorpd","xorpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x58*/ DECLARE_EX_INSN("addpd","addpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x59*/ DECLARE_EX_INSN("mulpd","mulpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5A*/ DECLARE_EX_INSN("cvtpd2ps","cvtpd2ps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5B*/ DECLARE_EX_INSN("cvtps2dq","cvtps2dq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5C*/ DECLARE_EX_INSN("subpd","subpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5D*/ DECLARE_EX_INSN("minpd","minpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5E*/ DECLARE_EX_INSN("divpd","divpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5F*/ DECLARE_EX_INSN("maxpd","maxpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x60*/ DECLARE_EX_INSN("punpcklbw","punpcklbw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x61*/ DECLARE_EX_INSN("punpcklwd","punpcklwd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x62*/ DECLARE_EX_INSN("punpckldq","punpckldq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x63*/ DECLARE_EX_INSN("packsswb","packsswb",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x64*/ DECLARE_EX_INSN("pcmpgtb","pcmpgtb",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x65*/ DECLARE_EX_INSN("pcmpgtw","pcmpgtw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x66*/ DECLARE_EX_INSN("pcmpgtd","pcmpgtd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x67*/ DECLARE_EX_INSN("packuswb","packuswb",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x68*/ DECLARE_EX_INSN("punpckhbw","punpckhbw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x69*/ DECLARE_EX_INSN("punpckhwd","punpckhwd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x6A*/ DECLARE_EX_INSN("punpckhdq","punpckhdq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x6B*/ DECLARE_EX_INSN("packssdw","packssdw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x6C*/ DECLARE_EX_INSN("punpcklqdq","punpcklqdq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x6D*/ DECLARE_EX_INSN("punpckhqdq","punpckhqdq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x6E*/ DECLARE_EX_INSN("movd","movd",ix86_ArgXMMXRnD,ix86_ArgXMMXRnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x6F*/ DECLARE_EX_INSN("movdqa","movdqa",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x70*/ DECLARE_EX_INSN("pshufd","pshufd",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x71*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgXMMXGr1,ix86_ArgXMMXGr1,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x72*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgXMMXGr2,ix86_ArgXMMXGr2,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x73*/ DECLARE_EX_INSN("!!!","!!!",ix86_ArgXMMXGr3,ix86_ArgXMMXGr3,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x74*/ DECLARE_EX_INSN("pcmpeqb","pcmpeqb",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x75*/ DECLARE_EX_INSN("pcmpeqw","pcmpeqw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x76*/ DECLARE_EX_INSN("pcmpeqd","pcmpeqd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x77*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x78*/ DECLARE_EX_INSN("extrq","extrq",ix86_ArgXMM1DigDig,ix86_ArgXMM1DigDig,IX86_P7|IX86_SSE,K64_FAM10|K64_SSE),
  /*0x79*/ DECLARE_EX_INSN("extrq","extrq",ix86_ArgXMM1IReg,ix86_ArgXMM1IReg,IX86_P7|IX86_SSE,K64_FAM10|K64_SSE),
  /*0x7A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x7B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x7C*/ DECLARE_EX_INSN("haddpd","haddpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P5|IX86_SSE,K64_FAM9|K64_SSE),
  /*0x7D*/ DECLARE_EX_INSN("hsubpd","hsubpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P5|IX86_SSE,K64_FAM9|K64_SSE),
  /*0x7E*/ DECLARE_EX_INSN("movd","movd",ix86_ArgXMMXRD,ix86_ArgXMMXRD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x7F*/ DECLARE_EX_INSN("movdqa","movdqa",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x80*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x81*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x82*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x83*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x84*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x85*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x86*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x87*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x88*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x89*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x90*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x91*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x92*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x93*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x94*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x95*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x96*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x97*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x98*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x99*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC2*/ DECLARE_EX_INSN("cmppd","cmppd",ix86_ArgXMMCmp,ix86_ArgXMMCmp,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xC3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC4*/ DECLARE_EX_INSN("pinsrw","pinsrw",ix86_ArgXMMXRegDigit,ix86_ArgXMMXRegDigit,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xC5*/ DECLARE_EX_INSN("pextrw","pextrw",ix86_ArgRegXMMXDigit,ix86_ArgRegXMMXDigit,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xC6*/ DECLARE_EX_INSN("shufpd","shufpd",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xC7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD0*/ DECLARE_EX_INSN("addsubpd","addsubpd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P5|IX86_SSE,K64_FAM9|K64_SSE),
  /*0xD1*/ DECLARE_EX_INSN("psrlw","psrlw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xD2*/ DECLARE_EX_INSN("psrld","psrld",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xD3*/ DECLARE_EX_INSN("psrlq","psrlq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xD4*/ DECLARE_EX_INSN("paddq","paddq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xD5*/ DECLARE_EX_INSN("pmullw","pmullw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xD6*/ DECLARE_EX_INSN("movq","movq",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xD7*/ DECLARE_EX_INSN("pmovmskb","pmovmskb",ix86_ArgRXMMXRevnD,ix86_ArgRXMMXRevnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xD8*/ DECLARE_EX_INSN("psubusb","psubusb",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xD9*/ DECLARE_EX_INSN("psubusw","psubusw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xDA*/ DECLARE_EX_INSN("pminub","pminub",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xDB*/ DECLARE_EX_INSN("pand","pand",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xDC*/ DECLARE_EX_INSN("paddusb","paddusb",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xDD*/ DECLARE_EX_INSN("paddusw","paddusw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xDE*/ DECLARE_EX_INSN("pmaxub","pmaxub",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xDF*/ DECLARE_EX_INSN("pandn","pandn",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xE0*/ DECLARE_EX_INSN("pavgb","pavgb",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xE1*/ DECLARE_EX_INSN("psraw","psraw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xE2*/ DECLARE_EX_INSN("psrad","psrad",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xE3*/ DECLARE_EX_INSN("pavgw","pavgw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xE4*/ DECLARE_EX_INSN("pmulhuw","pmulhuw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xE5*/ DECLARE_EX_INSN("pmulhw","pmulhw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xE6*/ DECLARE_EX_INSN("cvttpd2dq","cvttpd2dq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xE7*/ DECLARE_EX_INSN("movntdq","movntdq",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xE8*/ DECLARE_EX_INSN("psubsb","psubsb",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xE9*/ DECLARE_EX_INSN("psubsw","psubsw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xEA*/ DECLARE_EX_INSN("pminsw","pminsw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xEB*/ DECLARE_EX_INSN("por","por",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xEC*/ DECLARE_EX_INSN("paddsb","paddsb",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xED*/ DECLARE_EX_INSN("paddsw","paddsw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xEE*/ DECLARE_EX_INSN("pmaxsw","pmaxsw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xEF*/ DECLARE_EX_INSN("pxor","pxor",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xF0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF1*/ DECLARE_EX_INSN("psllw","psllw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xF2*/ DECLARE_EX_INSN("pslld","pslld",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xF3*/ DECLARE_EX_INSN("psllq","psllq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xF4*/ DECLARE_EX_INSN("pmuludq","pmuludq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xF5*/ DECLARE_EX_INSN("pmaddwd","pmaddwd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xF6*/ DECLARE_EX_INSN("psadbw","psadbw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xF7*/ DECLARE_EX_INSN("maskmovdqu","maskmovdqu",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xF8*/ DECLARE_EX_INSN("psubb","psubb",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xF9*/ DECLARE_EX_INSN("psubw","psubw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xFA*/ DECLARE_EX_INSN("psubd","psubd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xFB*/ DECLARE_EX_INSN("psubq","psubq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xFC*/ DECLARE_EX_INSN("paddb","paddb",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xFD*/ DECLARE_EX_INSN("paddw","paddw",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xFE*/ DECLARE_EX_INSN("paddd","paddd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xFF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE)
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
  /*0xF0*/ DECLARE_EX_INSN("crc32", "crc32",ix86_ArgModRMDnW,ix86_ArgModRMDnW, IX86_P7, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN("crc32", "crc32",ix86_ArgModRMDW,ix86_ArgModRMDW, IX86_P7, K64_ATHLON),
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
  /*0x00*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x01*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x02*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x03*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x04*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x05*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x06*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x07*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x08*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x09*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x10*/ DECLARE_EX_INSN("movsd","movsd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x11*/ DECLARE_EX_INSN("movsd","movsd",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x12*/ DECLARE_EX_INSN("movddup","movddup",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P5|IX86_SSE,K64_FAM9|K64_SSE),
  /*0x13*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x14*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x15*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x16*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x17*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x18*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x19*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x20*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x21*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x22*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x23*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x24*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x25*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x26*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x27*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x28*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x29*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x2A*/ DECLARE_EX_INSN("cvtsi2sd","cvtsi2sd",ix86_ArgRXMMXnD,ix86_ArgRXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2B*/ DECLARE_EX_INSN("movntsd","movntsd",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P7|IX86_SSE,K64_FAM10|K64_SSE),
  /*0x2C*/ DECLARE_EX_INSN("cvttsd2si","cvttsd2si",ix86_ArgRXMMXRevnD,ix86_ArgRXMMXRevnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2D*/ DECLARE_EX_INSN("cvtsd2si","cvtsd2si",ix86_ArgRXMMXRevnD,ix86_ArgRXMMXRevnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x2F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x30*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x31*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x32*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x33*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x34*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x35*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x36*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x37*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x38*/ DECLARE_EX_INSN(ix86_F20F38_Table,ix86_F20F38_Table,NULL,NULL,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x39*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x40*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x41*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x42*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x43*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x44*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x45*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x46*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x47*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x48*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x49*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x50*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x51*/ DECLARE_EX_INSN("sqrtsd","sqrtsd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x52*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x53*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x54*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x55*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x56*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x57*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x58*/ DECLARE_EX_INSN("addsd","addsd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x59*/ DECLARE_EX_INSN("mulsd","mulsd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5A*/ DECLARE_EX_INSN("cvtsd2ss","cvtsd2ss",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x5C*/ DECLARE_EX_INSN("subsd","subsd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5D*/ DECLARE_EX_INSN("minsd","minsd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5E*/ DECLARE_EX_INSN("divsd","divsd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5F*/ DECLARE_EX_INSN("maxsd","maxsd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x60*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x61*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x62*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x63*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x64*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x65*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x66*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x67*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x68*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x69*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x6A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x6B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x6C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x6D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x6E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x6F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x70*/ DECLARE_EX_INSN("pshuflw","pshuflw",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x71*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x72*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x73*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x74*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x75*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x76*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x77*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x78*/ DECLARE_EX_INSN("insertq","insertq",ix86_ArgXMM1RegDigDig,ix86_ArgXMM1RegDigDig,IX86_P7|IX86_SSE,K64_FAM10|K64_SSE),
  /*0x79*/ DECLARE_EX_INSN("insertq","insertq",ix86_ArgXMM1IReg,ix86_ArgXMM1IReg,IX86_P7|IX86_SSE,K64_FAM10|K64_SSE),
  /*0x7A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x7B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x7C*/ DECLARE_EX_INSN("haddps","haddps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P5|IX86_SSE,K64_FAM9|K64_SSE),
  /*0x7D*/ DECLARE_EX_INSN("hsubps","hsubps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P5|IX86_SSE,K64_FAM9|K64_SSE),
  /*0x7E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x7F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x80*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x81*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x82*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x83*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x84*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x85*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x86*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x87*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x88*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x89*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x90*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x91*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x92*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x93*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x94*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x95*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x96*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x97*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x98*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x99*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC2*/ DECLARE_EX_INSN("cmpsd","cmpsd",ix86_ArgXMMCmp,ix86_ArgXMMCmp,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xC3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD0*/ DECLARE_EX_INSN("addsubps","addsubps",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P5|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xD1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD6*/ DECLARE_EX_INSN("movdq2q","movdq2q",ix86_ArgMMXMMXnD,ix86_ArgMMXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xD7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xDA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xDB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xDC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xDD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xDE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xDF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE6*/ DECLARE_EX_INSN("cvtpd2dq","cvtpd2dq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xE7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xEA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xEB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xEC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xED*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xEE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xEF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF0*/ DECLARE_EX_INSN("lddqu","lddqu",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P5|IX86_SSE,K64_FAM9|K64_SSE),
  /*0xF1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xFA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xFB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xFC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xFD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xFE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xFF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE)
};

const ix86_ExOpcodes ix86_F30F_PentiumTable[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x01*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x02*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x03*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x04*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x05*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x06*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x07*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x08*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x09*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x0F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x10*/ DECLARE_EX_INSN("movss","movss",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x11*/ DECLARE_EX_INSN("movss","movss",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x12*/ DECLARE_EX_INSN("movsldup","movsldup",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P5|IX86_SSE,K64_FAM9|K64_SSE),
  /*0x13*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x14*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x15*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x16*/ DECLARE_EX_INSN("movshdup","movshdup",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P5|IX86_SSE,K64_FAM9|K64_SSE),
  /*0x17*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x18*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x19*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x1F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x20*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x21*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x22*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x23*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x24*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x25*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x26*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x27*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x28*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x29*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x2A*/ DECLARE_EX_INSN("cvtsi2ss","cvtsi2ss",ix86_ArgRXMMXnD,ix86_ArgRXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2B*/ DECLARE_EX_INSN("movntss","movntss",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P7|IX86_SSE,K64_FAM10|K64_SSE),
  /*0x2C*/ DECLARE_EX_INSN("cvttss2si","cvttss2si",ix86_ArgRXMMXRevnD,ix86_ArgRXMMXRevnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2D*/ DECLARE_EX_INSN("cvtss2si","cvtss2si",ix86_ArgRXMMXRevnD,ix86_ArgRXMMXRevnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x2E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x2F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x30*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x31*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x32*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x33*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x34*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x35*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x36*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x37*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x38*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x39*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x3F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x40*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x41*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x42*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x43*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x44*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x45*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x46*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x47*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x48*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x49*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x4F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x50*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x51*/ DECLARE_EX_INSN("sqrtss","sqrtss",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x52*/ DECLARE_EX_INSN("rsqrtss","rsqrtss",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x53*/ DECLARE_EX_INSN("rcpss","rcpss",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x54*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x55*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x56*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x57*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x58*/ DECLARE_EX_INSN("addss","addss",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x59*/ DECLARE_EX_INSN("mulss","mulss",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5A*/ DECLARE_EX_INSN("cvtss2sd","cvtss2sd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5B*/ DECLARE_EX_INSN("cvttps2dq","cvttps2dq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5C*/ DECLARE_EX_INSN("subss","subss",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5D*/ DECLARE_EX_INSN("minss","minss",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5E*/ DECLARE_EX_INSN("divss","divss",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x5F*/ DECLARE_EX_INSN("maxss","maxss",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x60*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x61*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x62*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x63*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x64*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x65*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x66*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x67*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x68*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x69*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x6A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x6B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x6C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x6D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x6E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x6F*/ DECLARE_EX_INSN("movdqu","movdqu",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x70*/ DECLARE_EX_INSN("pshufhw","pshufhw",ix86_ArgXMMRMDigit,ix86_ArgXMMRMDigit,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x71*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x72*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x73*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x74*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x75*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x76*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x77*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x78*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x79*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x7A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x7B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x7C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x7D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x7E*/ DECLARE_EX_INSN("movq","movq",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x7F*/ DECLARE_EX_INSN("movdqu","movdqu",ix86_ArgXMMXD,ix86_ArgXMMXD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0x80*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x81*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x82*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x83*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x84*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x85*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x86*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x87*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x88*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x89*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x8F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x90*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x91*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x92*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x93*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x94*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x95*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x96*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x97*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x98*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x99*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9A*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9B*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9C*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9D*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9E*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0x9F*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xA9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xAF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xB8*/ DECLARE_EX_INSN("popcnt","popcnt",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_P7,K64_FAM10),
  /*0xB9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBD*/ DECLARE_EX_INSN("lzcnt","lzcnt",ix86_ArgModRMDW,ix86_ArgModRMDW,IX86_P7,K64_FAM10),
  /*0xBE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xBF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC2*/ DECLARE_EX_INSN("cmpss","cmpss",ix86_ArgXMMCmp,ix86_ArgXMMCmp,IX86_P3|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xC3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC7*/ DECLARE_EX_INSN("vmxon","vmxon",ix86_ArgMod,ix86_ArgMod,IX86_P6,K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xC9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xCF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD6*/ DECLARE_EX_INSN("movq2dq","movq2dq",ix86_ArgMMXMMXD,ix86_ArgMMXMMXD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xD7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xD9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xDA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xDB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xDC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xDD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xDE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xDF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE6*/ DECLARE_EX_INSN("cvtdq2pd","cvtdq2pd",ix86_ArgXMMXnD,ix86_ArgXMMXnD,IX86_P4|IX86_SSE,K64_ATHLON|K64_SSE),
  /*0xE7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xE9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xEA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xEB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xEC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xED*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xEE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xEF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF0*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF1*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF2*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF3*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF4*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF5*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF6*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF7*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF8*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xF9*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xFA*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xFB*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xFC*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xFD*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xFE*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE),
  /*0xFF*/ DECLARE_EX_INSN(NULL,NULL,NULL,NULL,IX86_UNKMMX,K64_ATHLON|K64_SSE)
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
const char * ix86_ExGrp1[] = { "sgdt", "sidt", "lgdt", "lidt", "smsw", "???", "lmsw", "invlpg" };

const char * ix86_BitGrpNames[] = { "???", "???", "???", "???", "bt", "bts", "btr", "btc" };

const char *ix86_MMXGr1[] = { "???", "???", "psrlw", "???", "psraw", "???", "psllw", "???" };
const char *ix86_MMXGr2[] = { "???", "???", "psrld", "???", "psrad", "???", "pslld", "???" };
const char *ix86_MMXGr3[] = { "???", "???", "psrlq", "???", "???",   "???", "psllq", "???" };

const char *ix86_XMMXGr1[] = { "???", "???", "psrlw", "???", "psraw", "???", "psllw", "???" };
const char *ix86_XMMXGr2[] = { "???", "???", "psrld", "???", "psrad", "???", "pslld", "???" };
const char *ix86_XMMXGr3[] = { "???", "???", "psrlq", "psrldq", "???",   "???", "psllq", "pslldq" };

const char *ix86_3dPrefetchGrp[] = { "prefetch", "prefetchw", "prefetch", "prefetch", "prefetch", "prefetch", "prefetch", "prefetch" };

const char *ix86_KatmaiGr1Names[] = { "fxsave", "fxrstor", "ldmxcsr", "stmxcsr", "???", "lfence", "mfence", "sfence" };
const char *ix86_KatmaiGr2Names[] = { "prefetchnta", "prefetcht0", "prefetcht1", "prefetcht2", "???", "???", "???", "???" };
const char *ix86_KatmaiCmpSuffixes[] = { "eq", "lt", "le", "unord", "neq", "nlt", "nle", "ord" };


unsigned x86_Bitness = DAB_AUTO;
static unsigned BITNESS = DAB_AUTO;

char *ix86_voidstr;
char *ix86_da_out;

char ix86_segpref[4] = "";

tBool Use32Addr,Use32Data,UseMMXSet,UseXMMXSet,Use64;

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
static int is_REX(unsigned char code)
{
#ifdef IX86_64
    if((code & 0xF0) == 0x40) return 1;
    else
#endif
    return 0;
}

static MBuffer parse_REX_type(unsigned char code,MBuffer insn,char *up,char *has_rex)
{
    if(x86_Bitness == DAB_USE64 && !has_rex) {
	(*has_rex)++;
	k86_REX = insn[0];
    }
    (*up)++;
    return &insn[1];
}

static void ix86_gettype(DisasmRet *dret,ix86Param *_DisP)
{
 MBuffer insn;
 char ua,ud,up,has_lock,has_rep,has_seg;
#ifdef IX86_64
 char has_rex;
#endif
 insn = &_DisP->RealCmd[0];
 dret->pro_clone = __INSNT_ORDINAL;
 has_lock = has_rep = has_seg = 0;
 up = ua = ud = 0;
#ifdef IX86_64
 has_rex=0;
 k86_REX=0;
#endif
 RepeateByPrefix:
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64)
 {
   if(ua+ud+has_seg+has_rep+has_lock>4) goto get_type;
 } 
 else
#endif
 if(has_lock + has_rep > 1 || has_seg > 1 || ua > 1 || ud > 1) goto get_type;
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
		insn=parse_REX_type(insn[0],insn,&up,&has_rex);
		goto RepeateByPrefix;
                break;
#endif
   case 0x26:
   case 0x2E:
   case 0x36:
   case 0x3E:
   case 0x64:
   case 0x65:
              if(has_seg) break;
              has_seg++;
              goto MakePref;
   case 0x66:
#ifdef IX86_64
		if(is_REX(insn[1]))
		    insn=parse_REX_type(insn[0],insn,&up,&has_rex);
#endif
              /** First check for SSE2 extensions */
              if(insn[1] == 0x0F &&
	        (
#ifdef IX86_64
		x86_Bitness==DAB_USE64?
		ix86_660F_PentiumTable[insn[2]].name64:
#endif
	        ix86_660F_PentiumTable[insn[2]].name
		))
                goto get_type;
              ud++;
              goto MakePref;
   case 0x67:
              ua++;
              goto MakePref;
   case 0xF0:
              if(has_lock + has_rep) break;
              has_lock++;
              goto MakePref;
   case 0xF2:
#ifdef IX86_64
		if(is_REX(insn[1]))
		    insn=parse_REX_type(insn[0],insn,&up,&has_rex);
#endif
              /** First check for SSE2 extensions */
              if(insn[1] == 0x0F &&
	        (
#ifdef IX86_64
		x86_Bitness==DAB_USE64?
		ix86_F20F_PentiumTable[insn[2]].name64:
#endif
	        ix86_F20F_PentiumTable[insn[2]].name
		))
                goto get_type;
              if(has_rep + has_lock)  break;
              has_rep++;
              goto MakePref;
   case 0xF3:
#ifdef IX86_64
		if(is_REX(insn[1]))
		    insn=parse_REX_type(insn[0],insn,&up,&has_rex);
#endif
              /** First check for SSE extensions */
              if(insn[1] == 0x0F &&
	        (
#ifdef IX86_64
		x86_Bitness==DAB_USE64?
		ix86_F30F_PentiumTable[insn[2]].name64:
#endif
	        ix86_F30F_PentiumTable[insn[2]].name
		))
                goto get_type;
              else
               if(insn[1] == 0x90) goto get_type;
              if(has_rep + has_lock) break;
              has_rep++;
              goto MakePref;
   default:   break;
   MakePref:
              insn = &insn[1];
              up++;
              goto RepeateByPrefix;
 }
 get_type:
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    Use32Addr=1;
    if(((k86_REX & 0x0F)>>3) != 0) Use32Data=1;
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
   else
     if(insn[0] == 0xFF &&
        (((insn[1] & 0x27) == 0x25 &&  Use32Addr) ||
        ((insn[1] & 0x27) == 0x26 && !Use32Addr)))
     {
        dret->pro_clone = __INSNT_JMPVVT;
        dret->codelen = (insn[1] & 0x27) == 0x25 ? 4 : 2;
        dret->field = 2;
     }
     else
     if(insn[0] == 0xFF && insn[1] == 0xA3 && Use32Addr)
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
              i += Use32Data ? 5 : 3;
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

static unsigned char parse_REX(unsigned char code,ix86Param* DisP,char *up)
{
#ifdef IX86_64
    if(x86_Bitness == DAB_USE64 && !has_REX) 
    {
        has_REX++;
        k86_REX = code;
        Use64 = (code & 0x0F)>>3;
    }
    DisP->CodeAddress++;
    DisP->RealCmd = &DisP->RealCmd[1];
    (*up)++;
#endif
    return DisP->RealCmd[0];
}


static DisasmRet __FASTCALL__ ix86Disassembler(__filesize_t ulShift,
                                               MBuffer buffer,
                                               unsigned flags)
{
 unsigned char code;
 DisasmRet Ret;
 ix86Param DisP;
 char ua,ud,up,has_lock,has_rep,has_seg;

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
 code = buffer[0];

 ix86_segpref[0] = 0;
 has_lock = has_rep = has_seg = 0;
 up = ua = ud = 0;
 ix86_da_out[0] = 0;
 Use32Data = Use32Addr = UseMMXSet = UseXMMXSet = False;

 if(BITNESS == DAB_AUTO && detectedFormat->query_bitness) x86_Bitness = detectedFormat->query_bitness(ulShift);
 else x86_Bitness = BITNESS;

 if(x86_Bitness == DAB_USE32
#ifdef IX86_64
   || x86_Bitness == DAB_USE64
#endif
   )
 {
    Use32Data = Use32Data ? False : True;
    Use32Addr = Use32Addr ? False : True;
 }
 Ret.str = ix86_voidstr;
 Ret.str[0] = 0;
#ifdef IX86_64
 has_REX=0;
 k86_REX=0;
 Use64 = 0;
#endif
 RepeateByPrefix:
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64)
 {
   if(ua+ud+has_seg+has_rep+has_lock>4) goto bad_prefixes;
 } 
 else
#endif
 if(has_lock + has_rep > 1 || has_seg > 1 || ua > 1 || ud > 1)
 {
   bad_prefixes:
   DisP.codelen = 0;
   strcpy(ix86_voidstr,"???");
   goto ExitDisAsm;
 }
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
   case 0x4F: if(x86_Bitness == DAB_USE64 && !has_REX) 
              {
                 code=parse_REX(code,&DisP,&up);
                 goto RepeateByPrefix;
              }
              break;
#endif
   case 0x26:
              if(has_seg) break;
#ifdef IX86_64
	      if(x86_Bitness < DAB_USE64)
	      /*in 64-bit mode, the CS,DS,ES,SS segment overrides are ignored*/
#endif
              strcpy(ix86_segpref,"es:");
              has_seg++;
              goto MakePref;
   case 0x2E:
              if(has_seg) break;
#ifdef IX86_64
	      if(x86_Bitness < DAB_USE64)
	      /*in 64-bit mode, the CS,DS,ES,SS segment overrides are ignored*/
#endif
              strcpy(ix86_segpref,"cs:");
              has_seg++;
              goto MakePref;
   case 0x36:
              if(has_seg) break;
#ifdef IX86_64
	      if(x86_Bitness < DAB_USE64)
	      /*in 64-bit mode, the CS,DS,ES,SS segment overrides are ignored*/
#endif
              strcpy(ix86_segpref,"ss:");
              has_seg++;
              goto MakePref;
   case 0x3E:
              if(has_seg) break;
#ifdef IX86_64
	      if(x86_Bitness < DAB_USE64)
	      /*in 64-bit mode, the CS,DS,ES,SS segment overrides are ignored*/
#endif
              strcpy(ix86_segpref,"ds:");
              has_seg++;
              goto MakePref;
   case 0x64:
              if(has_seg) break;
              strcpy(ix86_segpref,"fs:");
              has_seg++;
              DisP.pro_clone = IX86_CPU386;
              goto MakePref;
   case 0x65:
              if(has_seg) break;
              strcpy(ix86_segpref,"gs:");
              has_seg++;
              DisP.pro_clone = IX86_CPU386;
              goto MakePref;
   case 0x66:
#ifdef IX86_64
		if(is_REX(DisP.RealCmd[1]))
			code=parse_REX(DisP.RealCmd[1],&DisP,&up);
#endif
              /** First check for SSE2 extensions */
              if(DisP.RealCmd[1] == 0x0F &&
	        (
#ifdef IX86_64
		x86_Bitness==DAB_USE64?
		ix86_660F_PentiumTable[DisP.RealCmd[2]].name64:
#endif
	        ix86_660F_PentiumTable[DisP.RealCmd[2]].name
		))
              {
		unsigned char code = DisP.RealCmd[2];
		const ix86_ExOpcodes *extable=ix86_660F_PentiumTable;
		/** therefore it non rep prefix, but instruction of SSE2 */
		DisP.RealCmd = &DisP.RealCmd[2];
		code = DisP.RealCmd[0];
		DisP.codelen++;
		extable=ix86_prepare_flags(extable,&DisP,&code);
#ifdef IX86_64
		if(x86_Bitness == DAB_USE64) {
		    DisP.pro_clone = extable[code].flags64;
		    strcpy(Ret.str,extable[code].name64?extable[code].name64:"???");
		}
		else
#endif
		{
		    DisP.pro_clone = extable[code].pro_clone;
		    strcpy(Ret.str,extable[code].name?extable[code].name:"???");
		}
		if((
#ifdef IX86_64
		x86_Bitness==DAB_USE64?
		extable[code].method64:
#endif
		extable[code].method
		))
		{
		    TabSpace(Ret.str,TAB_POS);
#ifdef IX86_64
		    x86_Bitness==DAB_USE64?
		    extable[code].method64(Ret.str,&DisP):
#endif
		    extable[code].method(Ret.str,&DisP);
		}
		DisP.codelen++;
		goto ExitDisAsm;
		}
		ud++;
		Use32Data = Use32Data ? False : True;
		DisP.pro_clone = IX86_CPU386;
		goto MakePref;
   case 0x67:
              ua++;
              Use32Addr = Use32Addr ? False : True;
              DisP.pro_clone = IX86_CPU386;
              goto MakePref;
   case 0xF0:
              if(has_lock + has_rep) break;
              has_lock++;
              strcat(ix86_da_out,"lock; ");
              goto MakePref;
   case 0xF2:
#ifdef IX86_64
		if(is_REX(DisP.RealCmd[1]))
			code=parse_REX(DisP.RealCmd[1],&DisP,&up);
#endif
              /** First check for SSE2 extensions */
              if(DisP.RealCmd[1] == 0x0F &&
	        (
#ifdef IX86_64
		x86_Bitness==DAB_USE64?
		ix86_F20F_PentiumTable[DisP.RealCmd[2]].name64:
#endif
	        ix86_F20F_PentiumTable[DisP.RealCmd[2]].name
		))
              {
		unsigned char code = DisP.RealCmd[2];
		const ix86_ExOpcodes *extable=ix86_F20F_PentiumTable;
                /** therefore it non rep prefix, but instruction of SSE2 */
                DisP.RealCmd = &DisP.RealCmd[2];
                code = DisP.RealCmd[0];
                DisP.codelen++;
		extable=ix86_prepare_flags(extable,&DisP,&code);
#ifdef IX86_64
		if(x86_Bitness == DAB_USE64)
		{
		    DisP.pro_clone = extable[code].flags64;
		    strcpy(Ret.str,extable[code].name64?extable[code].name64:"???");
		}
		else
#endif
		{
		    DisP.pro_clone = extable[code].pro_clone;
		    strcpy(Ret.str,extable[code].name?extable[code].name:"???");
		}
                if(
	        (
#ifdef IX86_64
		x86_Bitness==DAB_USE64?
		  extable[code].method64:
#endif
		  extable[code].method
		  ))
                {
                  TabSpace(Ret.str,TAB_POS);
#ifdef IX86_64
		  x86_Bitness==DAB_USE64?
		  extable[code].method64(Ret.str,&DisP):
#endif
                  extable[code].method(Ret.str,&DisP);
                }
                DisP.codelen++;
                goto ExitDisAsm;
              }
              if(has_rep + has_lock) break;
              has_rep++;
              strcat(ix86_da_out,"repne; ");
              goto MakePref;
   case 0xF3:
#ifdef IX86_64
		if(is_REX(DisP.RealCmd[1]))
			code=parse_REX(DisP.RealCmd[1],&DisP,&up);
#endif
              /** First check for SSE extensions */
              if(DisP.RealCmd[1] == 0x0F &&
	        (
#ifdef IX86_64
		x86_Bitness==DAB_USE64?
		ix86_F30F_PentiumTable[DisP.RealCmd[2]].name64:
#endif
	        ix86_F30F_PentiumTable[DisP.RealCmd[2]].name
		))
              {
		unsigned char code = DisP.RealCmd[2];
		const ix86_ExOpcodes *extable=ix86_F30F_PentiumTable;
                /** therefore it non rep prefix, but instruction of SSE */
                DisP.RealCmd = &DisP.RealCmd[2];
                code = DisP.RealCmd[0];
                DisP.codelen++;
		extable=ix86_prepare_flags(extable,&DisP,&code);
#ifdef IX86_64
		if(x86_Bitness == DAB_USE64)
		{
		    DisP.pro_clone = extable[code].flags64;
		    strcpy(Ret.str,extable[code].name64?extable[code].name64:"???");
		}
		else
#endif
		{
		    DisP.pro_clone = extable[code].pro_clone;
		    strcpy(Ret.str,extable[code].name?extable[code].name:"???");
		}
                if(
	        (
#ifdef IX86_64
		x86_Bitness==DAB_USE64?
		  extable[code].method64:
#endif
		  extable[code].method
		  ))
                {
                  TabSpace(Ret.str,TAB_POS);
#ifdef IX86_64
		  x86_Bitness==DAB_USE64?
		  extable[code].method64(Ret.str,&DisP):
#endif
                  extable[code].method(Ret.str,&DisP);
                }
                DisP.codelen++;
                goto ExitDisAsm;
              }
              else
                if(DisP.RealCmd[1] == 0x90)
                {
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
#ifdef IX86_64
 /* Let it be overwritten later */
 if(x86_Bitness == DAB_USE64) DisP.pro_clone = ix86_table[code].flags64;
 else
#endif
 DisP.pro_clone = ix86_table[code].pro_clone;
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64)
 {
   has67_in64 = (Use32Addr == 0);
   Use32Addr = 1; /* there is no way to use 16-bit addresing in 64-bit mode */
   if(Use64) Use32Data=1; /* 66h prefix is ignored if REX prefix is present*/
   if(ix86_table[code].flags64 & K64_DEF32)
   {
     if(Use64) strcpy(Ret.str,ix86_table[code].name64);
     else
     if(!Use32Data) strcpy(Ret.str,ix86_table[code].name16);
     else           strcpy(Ret.str,ix86_table[code].name32);
   }
   else
   if(Use32Addr || (ix86_table[code].flags64 & K64_NOCOMPAT))
                 strcpy(Ret.str,ix86_table[code].name64);
   else          strcpy(Ret.str,ix86_table[code].name32);
 }
 else
#endif
 strcpy(Ret.str,Use32Data ? ix86_table[code].name32 : ix86_table[code].name16);
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64)
 {
   if(ix86_table[code].method64)
   {
     TabSpace(Ret.str,TAB_POS);
     ix86_table[code].method64(Ret.str,&DisP);
   }
 }
 else
#endif
 if(ix86_table[code].method)
 {
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
 ExitDisAsm:
 /* control jumps here after SSE2 opcodes */
 if(up) DisP.codelen+=up;
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
 if(Use32Data || Use32Addr || x86_Bitness == DAB_USE32)
 {
    if((DisP.pro_clone & IX86_CPUMASK) < IX86_CPU386)
    {
	DisP.pro_clone &= ~IX86_CPUMASK;
	DisP.pro_clone |= IX86_CPU386;
    }
 }
 Ret.pro_clone = DisP.pro_clone;
 Ret.codelen = DisP.codelen;
 Bye:
 Use32Data = Use32Addr = False;
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
  " K86-64 SSE ",
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
  " SSE ALU ",
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
	if((clone & K64_SSE) == K64_SSE) return disasm_cset.cpu_cset[2].clone[0];
	else
	if(clone & (K64_FPU|K64_MMX)) return disasm_cset.cpu_cset[1].clone[0];
	else
	return disasm_cset.cpu_cset[0].clone[0];
     }
     else
#endif
     if((clone&IX86_MMX)|(clone&IX86_SSE)) return disasm_cset.cpu_cset[2].clone[clone & IX86_CPUMASK];
     else
       if((clone&IX86_FPU))	return disasm_cset.cpu_cset[1].clone[clone & IX86_CPUMASK];
       else			return disasm_cset.cpu_cset[0].clone[clone & IX86_CPUMASK];
}

static ColorAttr  __FASTCALL__ ix86GetOpcodeColor(unsigned long clone)
{
#ifdef IX86_64
   if(x86_Bitness == DAB_USE64)
	return ((clone & K64_CPL0) == K64_CPL0)?disasm_cset.opcodes0:disasm_cset.opcodes;
   else
#endif
  return ((clone & IX86_CPL0) == IX86_CPL0)?disasm_cset.opcodes0:disasm_cset.opcodes;
}

static ColorAttr  __FASTCALL__ ix86altGetAsmColor(unsigned long clone)
{
    color_mode=1;
#ifdef IX86_64
     if(x86_Bitness == DAB_USE64)
     {
	if((clone & K64_SSE) == K64_SSE) return disasm_cset.cpu_cset[2].clone[clone & K64_CLONEMASK];
	else
	if(clone & (K64_FPU|K64_MMX)) return disasm_cset.cpu_cset[1].clone[clone & K64_CLONEMASK];
	else
	return disasm_cset.cpu_cset[0].clone[clone & K64_CLONEMASK];
     }
     else
#endif
     {
	if((clone & IX86_SSE) == IX86_SSE) return disasm_cset.cpu_cset[2].clone[0];
	else
	if(clone & (IX86_FPU|IX86_MMX)) return disasm_cset.cpu_cset[1].clone[0];
	else
	return disasm_cset.cpu_cset[0].clone[0];
     }
}

static ColorAttr  __FASTCALL__ ix86altGetOpcodeColor(unsigned long clone)
{
#ifdef IX86_64
   if(x86_Bitness == DAB_USE64)
	return ((clone & K64_CPL0) == K64_CPL0)?disasm_cset.opcodes0:disasm_cset.opcodes;
   else
#endif
  return ((clone & IX86_CPL0) == IX86_CPL0)?disasm_cset.opcodes0:disasm_cset.opcodes;
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
    "yasm -s --version",
  },
  {
    "nasm -f bin -s -o %1$stmp1 %1$stmp0 >%1$stmp2",
    "nasm -s -version",
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
    MemOutBox("ix86 dissasembler initialization");
    exit(EXIT_FAILURE);
  }
#ifdef HAVE_POPEN
  //Assembler initialization
  //Look for an available assembler
  if (active_assembler == -1) //Execute this only once
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
  char *home;

  //Check assembler availability
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
  i=snprintf(commandbuffer, FILENAME_MAX+1, assemblers[active_assembler].run_command, home);
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
  "Intel ~ix86",
  { "x86Hlp", "Bitnes", NULL, NULL, NULL },
  { x86AsmRef, x86Select_Bitness, NULL, NULL, NULL },
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








