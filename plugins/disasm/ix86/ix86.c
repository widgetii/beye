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
**/
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

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

ix86_Opcodes ix86_table[256] =
{
  /*0x00*/ { "add", "add", ix86_ArgModRMnDnW, IX86_CPU086 },
  /*0x01*/ { "add", "add", ix86_ArgModRMnDW, IX86_CPU086 },
  /*0x02*/ { "add", "add", ix86_ArgModRMDnW, IX86_CPU086 },
  /*0x03*/ { "add", "add", ix86_ArgModRMDW, IX86_CPU086 },
  /*0x04*/ { "add", "add", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x05*/ { "add", "add", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x06*/ { "push", "push", ix86_ArgES, IX86_CPU086 },
  /*0x07*/ { "pop", "pop", ix86_ArgES, IX86_CPU086 },
  /*0x08*/ { "or", "or", ix86_ArgModRMnDnW, IX86_CPU086 },
  /*0x09*/ { "or", "or", ix86_ArgModRMnDW, IX86_CPU086 },
  /*0x0A*/ { "or", "or", ix86_ArgModRMDnW, IX86_CPU086 },
  /*0x0B*/ { "or", "or", ix86_ArgModRMDW, IX86_CPU086 },
  /*0x0C*/ { "or", "or", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x0D*/ { "or", "or", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x0E*/ { "push", "push", ix86_ArgCS, IX86_CPU086 },
  /*0x0F*/ { "!!!", "!!!", ix86_ExOpCodes, IX86_CPU086 },
  /*0x10*/ { "adc", "adc", ix86_ArgModRMnDnW, IX86_CPU086 },
  /*0x11*/ { "adc", "adc", ix86_ArgModRMnDW, IX86_CPU086 },
  /*0x12*/ { "adc", "adc", ix86_ArgModRMDnW, IX86_CPU086 },
  /*0x13*/ { "adc", "adc", ix86_ArgModRMDW, IX86_CPU086 },
  /*0x14*/ { "adc", "adc", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x15*/ { "adc", "adc", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x16*/ { "push", "push", ix86_ArgSS, IX86_CPU086 },
  /*0x17*/ { "pop", "pop", ix86_ArgSS, IX86_CPU086 },
  /*0x18*/ { "sbb", "sbb", ix86_ArgModRMnDnW, IX86_CPU086 },
  /*0x19*/ { "sbb", "sbb", ix86_ArgModRMnDW, IX86_CPU086 },
  /*0x1A*/ { "sbb", "sbb", ix86_ArgModRMDnW, IX86_CPU086 },
  /*0x1B*/ { "sbb", "sbb", ix86_ArgModRMDW, IX86_CPU086 },
  /*0x1C*/ { "sbb", "sbb", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x1D*/ { "sbb", "sbb", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x1E*/ { "push", "push", ix86_ArgDS, IX86_CPU086 },
  /*0x1F*/ { "pop", "pop", ix86_ArgDS, IX86_CPU086 },
  /*0x20*/ { "and", "and", ix86_ArgModRMnDnW, IX86_CPU086 },
  /*0x21*/ { "and", "and", ix86_ArgModRMnDW, IX86_CPU086 },
  /*0x22*/ { "and", "and", ix86_ArgModRMDnW, IX86_CPU086 },
  /*0x23*/ { "and", "and", ix86_ArgModRMDW, IX86_CPU086 },
  /*0x24*/ { "and", "and", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x25*/ { "and", "and", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x26*/ { "seg", "seg", ix86_ArgES, IX86_CPU086 },
  /*0x27*/ { "daa", "daa", NULL, IX86_CPU086 },
  /*0x28*/ { "sub", "sub", ix86_ArgModRMnDnW, IX86_CPU086 },
  /*0x29*/ { "sub", "sub", ix86_ArgModRMnDW, IX86_CPU086 },
  /*0x2A*/ { "sub", "sub", ix86_ArgModRMDnW, IX86_CPU086 },
  /*0x2B*/ { "sub", "sub", ix86_ArgModRMDW, IX86_CPU086 },
  /*0x2C*/ { "sub", "sub", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x2D*/ { "sub", "sub", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x2E*/ { "seg", "seg", ix86_ArgCS, IX86_CPU086 },
  /*0x2F*/ { "das", "das", NULL, IX86_CPU086 },
  /*0x30*/ { "xor", "xor", ix86_ArgModRMnDnW, IX86_CPU086 },
  /*0x31*/ { "xor", "xor", ix86_ArgModRMnDW, IX86_CPU086 },
  /*0x32*/ { "xor", "xor", ix86_ArgModRMDnW, IX86_CPU086 },
  /*0x33*/ { "xor", "xor", ix86_ArgModRMDW, IX86_CPU086 },
  /*0x34*/ { "xor", "xor", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x35*/ { "xor", "xor", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x36*/ { "seg", "seg", ix86_ArgSS, IX86_CPU086 },
  /*0x37*/ { "aaa", "aaa", NULL, IX86_CPU086 },
  /*0x38*/ { "cmp", "cmp", ix86_ArgModRMnDnW, IX86_CPU086 },
  /*0x39*/ { "cmp", "cmp", ix86_ArgModRMnDW, IX86_CPU086 },
  /*0x3A*/ { "cmp", "cmp", ix86_ArgModRMDnW, IX86_CPU086 },
  /*0x3B*/ { "cmp", "cmp", ix86_ArgModRMDW, IX86_CPU086 },
  /*0x3C*/ { "cmp", "cmp", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x3D*/ { "cmp", "cmp", ix86_ArgAXDigit, IX86_CPU086 },
  /*0x3E*/ { "seg", "seg", ix86_ArgDS, IX86_CPU086 },
  /*0x3F*/ { "aas", "aas", NULL, IX86_CPU086 },
  /*0x40*/ { "inc", "inc", ix86_ArgIReg, IX86_CPU086 },
  /*0x41*/ { "inc", "inc", ix86_ArgIReg, IX86_CPU086 },
  /*0x42*/ { "inc", "inc", ix86_ArgIReg, IX86_CPU086 },
  /*0x43*/ { "inc", "inc", ix86_ArgIReg, IX86_CPU086 },
  /*0x44*/ { "inc", "inc", ix86_ArgIReg, IX86_CPU086 },
  /*0x45*/ { "inc", "inc", ix86_ArgIReg, IX86_CPU086 },
  /*0x46*/ { "inc", "inc", ix86_ArgIReg, IX86_CPU086 },
  /*0x47*/ { "inc", "inc", ix86_ArgIReg, IX86_CPU086 },
  /*0x48*/ { "dec", "dec", ix86_ArgIReg, IX86_CPU086 },
  /*0x49*/ { "dec", "dec", ix86_ArgIReg, IX86_CPU086 },
  /*0x4A*/ { "dec", "dec", ix86_ArgIReg, IX86_CPU086 },
  /*0x4B*/ { "dec", "dec", ix86_ArgIReg, IX86_CPU086 },
  /*0x4C*/ { "dec", "dec", ix86_ArgIReg, IX86_CPU086 },
  /*0x4D*/ { "dec", "dec", ix86_ArgIReg, IX86_CPU086 },
  /*0x4E*/ { "dec", "dec", ix86_ArgIReg, IX86_CPU086 },
  /*0x4F*/ { "dec", "dec", ix86_ArgIReg, IX86_CPU086 },
  /*0x50*/ { "push", "push", ix86_ArgIReg, IX86_CPU086 },
  /*0x51*/ { "push", "push", ix86_ArgIReg, IX86_CPU086 },
  /*0x52*/ { "push", "push", ix86_ArgIReg, IX86_CPU086 },
  /*0x53*/ { "push", "push", ix86_ArgIReg, IX86_CPU086 },
  /*0x54*/ { "push", "push", ix86_ArgIReg, IX86_CPU086 },
  /*0x55*/ { "push", "push", ix86_ArgIReg, IX86_CPU086 },
  /*0x56*/ { "push", "push", ix86_ArgIReg, IX86_CPU086 },
  /*0x57*/ { "push", "push", ix86_ArgIReg, IX86_CPU086 },
  /*0x58*/ { "pop", "pop", ix86_ArgIReg, IX86_CPU086 },
  /*0x59*/ { "pop", "pop", ix86_ArgIReg, IX86_CPU086 },
  /*0x5A*/ { "pop", "pop", ix86_ArgIReg, IX86_CPU086 },
  /*0x5B*/ { "pop", "pop", ix86_ArgIReg, IX86_CPU086 },
  /*0x5C*/ { "pop", "pop", ix86_ArgIReg, IX86_CPU086 },
  /*0x5D*/ { "pop", "pop", ix86_ArgIReg, IX86_CPU086 },
  /*0x5E*/ { "pop", "pop", ix86_ArgIReg, IX86_CPU086 },
  /*0x5F*/ { "pop", "pop", ix86_ArgIReg, IX86_CPU086 },
  /*0x60*/ { "pushaw", "pushad", NULL, IX86_CPU186 },
  /*0x61*/ { "popaw", "popad", NULL, IX86_CPU186 },
  /*0x62*/ { "bound", "bound", ix86_ArgModRMDW, IX86_CPU286 },
  /*0x63*/ { "arpl", "arpl", ix86_ArgModRMDW, IX86_CPU286 },
  /*0x64*/ { "seg", "seg", ix86_ArgFS, IX86_CPU386 },
  /*0x65*/ { "seg", "seg", ix86_ArgGS, IX86_CPU386 },
  /*0x66*/ { "???", "???", NULL, IX86_CPU386 },
  /*0x67*/ { "???", "???", NULL, IX86_CPU386 },
  /*0x68*/ { "push", "push", ix86_ArgInt, IX86_CPU186 },
  /*0x69*/ { "imul", "imul", ix86_ArgRegRMDigit, IX86_CPU186 },
  /*0x6A*/ { "push", "push", ix86_ArgSInt, IX86_CPU186 },
  /*0x6B*/ { "imul", "imul", ix86_ArgRegRMDigit, IX86_CPU186 },
  /*0x6C*/ { "insb", "insb", NULL, IX86_CPU186 },
  /*0x6D*/ { "insw", "insd", NULL, IX86_CPU186 },
  /*0x6E*/ { "outsb", "outsb", NULL, IX86_CPU186 },
  /*0x6F*/ { "outsw", "outsd", NULL, IX86_CPU186 },
  /*0x70*/ { "jo", "jo", ix86_ArgShort, IX86_CPU086 },
  /*0x71*/ { "jno", "jno", ix86_ArgShort, IX86_CPU086 },
  /*0x72*/ { "jc", "jc", ix86_ArgShort, IX86_CPU086 },
  /*0x73*/ { "jnc", "jnc", ix86_ArgShort, IX86_CPU086 },
  /*0x74*/ { "je", "je", ix86_ArgShort, IX86_CPU086 },
  /*0x75*/ { "jne", "jne", ix86_ArgShort, IX86_CPU086 },
  /*0x76*/ { "jna", "jna", ix86_ArgShort, IX86_CPU086 },
  /*0x77*/ { "ja", "ja", ix86_ArgShort, IX86_CPU086 },
  /*0x78*/ { "js", "js", ix86_ArgShort, IX86_CPU086 },
  /*0x79*/ { "jns", "jns", ix86_ArgShort, IX86_CPU086 },
  /*0x7A*/ { "jp", "jp", ix86_ArgShort, IX86_CPU086 },
  /*0x7B*/ { "jnp", "jnp", ix86_ArgShort, IX86_CPU086 },
  /*0x7C*/ { "jl", "jl", ix86_ArgShort, IX86_CPU086 },
  /*0x7D*/ { "jnl", "jnl", ix86_ArgShort, IX86_CPU086 },
  /*0x7E*/ { "jle", "jle", ix86_ArgShort, IX86_CPU086 },
  /*0x7F*/ { "jg", "jg", ix86_ArgShort, IX86_CPU086 },
  /*0x80*/ { "!!!", "!!!", ix86_ArgOp1, IX86_CPU086 },
  /*0x81*/ { "!!!", "!!!", ix86_ArgOp1, IX86_CPU086 },
  /*0x82*/ { "!!!", "!!!", ix86_ArgOp2, IX86_CPU086 },
  /*0x83*/ { "!!!", "!!!", ix86_ArgOp2, IX86_CPU086 },
  /*0x84*/ { "test", "test", ix86_ArgModRMnDnW, IX86_CPU086 },
  /*0x85*/ { "test", "test", ix86_ArgModRMnDW, IX86_CPU086 },
  /*0x86*/ { "xchg", "xchg", ix86_ArgModRMDnW, IX86_CPU086 },
  /*0x87*/ { "xchg", "xchg", ix86_ArgModRMDW, IX86_CPU086 },
  /*0x88*/ { "mov", "mov", ix86_ArgModRMnDnW, IX86_CPU086 },
  /*0x89*/ { "mov", "mov", ix86_ArgModRMnDW, IX86_CPU086 },
  /*0x8A*/ { "mov", "mov", ix86_ArgModRMDnW, IX86_CPU086 },
  /*0x8B*/ { "mov", "mov", ix86_ArgModRMDW, IX86_CPU086 },
  /*0x8C*/ { "mov", "mov", ix86_SMov, IX86_CPU086 },
  /*0x8D*/ { "lea", "lea", ix86_ArgModRMDW, IX86_CPU086 },
  /*0x8E*/ { "mov", "mov", ix86_SMov, IX86_CPU086 },
  /*0x8F*/ { "pop", "pop", ix86_ArgMod, IX86_CPU086 },
  /*0x90*/ { "nop", "nop", NULL, IX86_CPU086 },
  /*0x91*/ { "xchg", "xchg", ix86_ArgAXIReg, IX86_CPU086 },
  /*0x92*/ { "xchg", "xchg", ix86_ArgAXIReg, IX86_CPU086 },
  /*0x93*/ { "xchg", "xchg", ix86_ArgAXIReg, IX86_CPU086 },
  /*0x94*/ { "xchg", "xchg", ix86_ArgAXIReg, IX86_CPU086 },
  /*0x95*/ { "xchg", "xchg", ix86_ArgAXIReg, IX86_CPU086 },
  /*0x96*/ { "xchg", "xchg", ix86_ArgAXIReg, IX86_CPU086 },
  /*0x97*/ { "xchg", "xchg", ix86_ArgAXIReg, IX86_CPU086 },
  /*0x98*/ { "cbw", "cwde", NULL, IX86_CPU086 },
  /*0x99*/ { "cwd", "cdq", NULL, IX86_CPU086 },
  /*0x9A*/ { "callf16", "callf32", ix86_ArgFar, IX86_CPU086 },
  /*0x9B*/ { "wait", "wait", NULL, IX86_CPU086 },
  /*0x9C*/ { "pushfw", "pushfd", NULL, IX86_CPU086 },
  /*0x9D*/ { "popfw", "popfd", NULL, IX86_CPU086 },
  /*0x9E*/ { "sahf", "sahf", NULL, IX86_CPU086 },
  /*0x9F*/ { "lahf", "lahf", NULL, IX86_CPU086 },
  /*0xA0*/ { "mov", "mov", ix86_ArgAXMem, IX86_CPU086 },
  /*0xA1*/ { "mov", "mov", ix86_ArgAXMem, IX86_CPU086 },
  /*0xA2*/ { "mov", "mov", ix86_ArgAXMem, IX86_CPU086 },
  /*0xA3*/ { "mov", "mov", ix86_ArgAXMem, IX86_CPU086 },
  /*0xA4*/ { "movsb", "movsb", NULL, IX86_CPU086 },
  /*0xA5*/ { "movsw", "movsd", NULL, IX86_CPU086 },
  /*0xA6*/ { "cmpsb", "cmpsb", NULL, IX86_CPU086 },
  /*0xA7*/ { "cmpsw", "cmpsd", NULL, IX86_CPU086 },
  /*0xA8*/ { "test", "test", ix86_ArgAXDigit, IX86_CPU086 },
  /*0xA9*/ { "test", "test", ix86_ArgAXDigit, IX86_CPU086 },
  /*0xAA*/ { "stosb", "stosb", NULL, IX86_CPU086 },
  /*0xAB*/ { "stosw", "stosd", NULL, IX86_CPU086 },
  /*0xAC*/ { "lodsb", "lodsb", NULL, IX86_CPU086 },
  /*0xAD*/ { "lodsw", "lodsd", NULL, IX86_CPU086 },
  /*0xAE*/ { "scasb", "scasb", NULL, IX86_CPU086 },
  /*0xAF*/ { "scasw", "scasd", NULL, IX86_CPU086 },
  /*0xB0*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xB1*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xB2*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xB3*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xB4*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xB5*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xB6*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xB7*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xB8*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xB9*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xBA*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xBB*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xBC*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xBD*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xBE*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xBF*/ { "mov", "mov", ix86_ArgIRegDigit, IX86_CPU086 },
  /*0xC0*/ { "!!!", "!!!", ix86_ShOp2, IX86_CPU186 },
  /*0xC1*/ { "!!!", "!!!", ix86_ShOp2, IX86_CPU186 },
  /*0xC2*/ { "retn16", "retn32", ix86_ArgWord, IX86_CPU086 },
  /*0xC3*/ { "retn16", "retn32", NULL, IX86_CPU086 },
  /*0xC4*/ { "les", "les", ix86_ArgModRMDW, IX86_CPU086 },
  /*0xC5*/ { "lds", "lds", ix86_ArgModRMDW, IX86_CPU086 },
  /*0xC6*/ { "mov", "mov", ix86_ArgRMDigit, IX86_CPU086 },
  /*0xC7*/ { "mov", "mov", ix86_ArgRMDigit, IX86_CPU086 },
  /*0xC8*/ { "enter", "enter", ix86_ArgWord_Byte, IX86_CPU186 },
  /*0xC9*/ { "leave", "leave", NULL, IX86_CPU186 },
  /*0xCA*/ { "retf16", "retf32", ix86_ArgWord, IX86_CPU086 },
  /*0xCB*/ { "retf16", "retf32", NULL, IX86_CPU086 },
  /*0xCC*/ { "int3", "int3", NULL, IX86_CPU086 },
  /*0xCD*/ { "int", "int", ix86_ArgByte, IX86_CPU086 },
  /*0xCE*/ { "into16", "into32", NULL, IX86_CPU086 },
  /*0xCF*/ { "iret16", "iret32", NULL, IX86_CPU086 },
  /*0xD0*/ { "!!!", "!!!", ix86_ShOp1, IX86_CPU086 },
  /*0xD1*/ { "!!!", "!!!", ix86_ShOp1, IX86_CPU086 },
  /*0xD2*/ { "!!!", "!!!", ix86_ShOpCL, IX86_CPU086 },
  /*0xD3*/ { "!!!", "!!!", ix86_ShOpCL, IX86_CPU086 },
  /*0xD4*/ { "aam on", "aam on", ix86_ArgByte, IX86_CPU086 },
  /*0xD5*/ { "aad on", "aad on", ix86_ArgByte, IX86_CPU086 },
  /*0xD6*/ { "salc", "salc", NULL, IX86_CPU086 },
  /*0xD7*/ { "xlat", "xlat", NULL, IX86_CPU086 },
  /*0xD8*/ { "esc", "esc", ix86_FPUCmd, IX86_CPU086 },
  /*0xD9*/ { "esc", "esc", ix86_FPUCmd, IX86_CPU086 },
  /*0xDA*/ { "esc", "esc", ix86_FPUCmd, IX86_CPU086 },
  /*0xDB*/ { "esc", "esc", ix86_FPUCmd, IX86_CPU086 },
  /*0xDC*/ { "esc", "esc", ix86_FPUCmd, IX86_CPU086 },
  /*0xDD*/ { "esc", "esc", ix86_FPUCmd, IX86_CPU086 },
  /*0xDE*/ { "esc", "esc", ix86_FPUCmd, IX86_CPU086 },
  /*0xDF*/ { "esc", "esc", ix86_FPUCmd, IX86_CPU086 },
  /*0xE0*/ { "loopnz", "loopnz", ix86_ArgShort, IX86_CPU086 },
  /*0xE1*/ { "loopz", "loopz", ix86_ArgShort, IX86_CPU086 },
  /*0xE2*/ { "loop", "loop", ix86_ArgShort, IX86_CPU086 },
  /*0xE3*/ { "jcxz", "jecxz", ix86_ArgShort, IX86_CPU086 },
  /*0xE4*/ { "in", "in", ix86_InOut, IX86_CPU086 },
  /*0xE5*/ { "in", "in", ix86_InOut, IX86_CPU086 },
  /*0xE6*/ { "out", "out", ix86_InOut, IX86_CPU086 },
  /*0xE7*/ { "out", "out", ix86_InOut, IX86_CPU086 },
  /*0xE8*/ { "calln16", "calln32", ix86_ArgNear, IX86_CPU086 },
  /*0xE9*/ { "jmpn16", "jmpn32", ix86_ArgNear, IX86_CPU086 },
  /*0xEA*/ { "jmpf16", "jmpf32", ix86_ArgFar, IX86_CPU086 },
  /*0xEB*/ { "jmps", "jmps", ix86_ArgShort, IX86_CPU086 },
  /*0xEC*/ { "in", "in", ix86_InOut, IX86_CPU086 },
  /*0xED*/ { "in", "in", ix86_InOut, IX86_CPU086 },
  /*0xEE*/ { "out", "out", ix86_InOut, IX86_CPU086 },
  /*0xEF*/ { "out", "out", ix86_InOut, IX86_CPU086 },
  /*0xF0*/ { "lock", "lock", NULL, IX86_CPU086 },
  /*0xF1*/ { "icebp", "icebp", NULL, IX86_CPU386 },
  /*0xF2*/ { "repne", "repne", NULL, IX86_CPU086 },
  /*0xF3*/ { "rep", "rep", NULL, IX86_CPU086 },
  /*0xF4*/ { "hlt", "hlt", NULL, IX86_CPU086 },
  /*0xF5*/ { "cmc", "cmc", NULL, IX86_CPU086 },
  /*0xF6*/ { "!!!", "!!!", ix86_ArgGrp1, IX86_CPU086 },
  /*0xF7*/ { "!!!", "!!!", ix86_ArgGrp1, IX86_CPU086 },
  /*0xF8*/ { "clc", "clc", NULL, IX86_CPU086 },
  /*0xF9*/ { "stc", "stc", NULL, IX86_CPU086 },
  /*0xFA*/ { "cli", "cli", NULL, IX86_CPU086 },
  /*0xFB*/ { "sti", "sti", NULL, IX86_CPU086 },
  /*0xFC*/ { "cld", "cld", NULL, IX86_CPU086 },
  /*0xFD*/ { "std", "std", NULL, IX86_CPU086 },
  /*0xFE*/ { "!!!", "!!!", ix86_ArgGrp2, IX86_CPU086 },
  /*0xFF*/ { "!!!", "!!!", ix86_ArgGrp2, IX86_CPU086 }
};

ix86_ExOpcodes ix86_extable[256] = /* for 0FH leading */
{
  /*0x00*/ { "!!!", ix86_ArgExGr0, IX86_CPU286 },
  /*0x01*/ { "!!!", ix86_ArgExGr1, IX86_CPU286 },
  /*0x02*/ { "lar", ix86_ArgModRMDW, IX86_CPU286 },
  /*0x03*/ { "lsl", ix86_ArgModRMDW, IX86_CPU286 },
  /*0x04*/ { "???", NULL, IX86_UNKCPU },
  /*0x05*/ { "syscall", NULL, IX86_K6 },
  /*0x06*/ { "clts", NULL, IX86_CPU286 },
  /*0x07*/ { "sysret", NULL, IX86_K6 },
  /*0x08*/ { "invd", NULL, IX86_CPU486 },
  /*0x09*/ { "wbinvd", NULL, IX86_CPU486 },
  /*0x0A*/ { "???", NULL, IX86_UNKCPU },
  /*0x0B*/ { "ud", NULL, IX86_CPU686 },
  /*0x0C*/ { "???", NULL, IX86_UNKCPU },
  /*0x0D*/ { "!!!", ix86_3dNowPrefetchGrp, IX86_3DNOW },
  /*0x0E*/ { "femms", NULL, IX86_3DNOW },
  /*0x0F*/ { "!!!", ix86_3dNowOpCodes, IX86_3DNOW },
  /*0x10*/ { "movups", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x11*/ { "movups", ix86_ArgXMMXD, IX86_P3MMX },
  /*0x12*/ { "movlps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x13*/ { "movlps", ix86_ArgXMMXD, IX86_P3MMX },
  /*0x14*/ { "unpcklps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x15*/ { "unpckhps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x16*/ { "movhps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x17*/ { "movhps", ix86_ArgXMMXD, IX86_P3MMX },
  /*0x18*/ { "!!!", ix86_ArgKatmaiGrp2, IX86_P3 },
  /*0x19*/ { "???", NULL, IX86_UNKCPU },
  /*0x1A*/ { "???", NULL, IX86_UNKCPU },
  /*0x1B*/ { "???", NULL, IX86_UNKCPU },
  /*0x1C*/ { "???", NULL, IX86_UNKCPU },
  /*0x1D*/ { "???", NULL, IX86_UNKCPU },
  /*0x1E*/ { "???", NULL, IX86_UNKCPU },
  /*0x1F*/ { "???", NULL, IX86_UNKCPU },
  /*0x20*/ { "mov", ix86_ArgMovXRY, IX86_CPU386 },
  /*0x21*/ { "mov", ix86_ArgMovXRY, IX86_CPU386 },
  /*0x22*/ { "mov", ix86_ArgMovXRY, IX86_CPU386 },
  /*0x23*/ { "mov", ix86_ArgMovXRY, IX86_CPU386 },
  /*0x24*/ { "mov", ix86_ArgMovXRY, IX86_CPU386 },
  /*0x25*/ { "mov", ix86_ArgMovXRY, IX86_CPU386 },
  /*0x26*/ { "mov", ix86_ArgMovXRY, IX86_CPU386 },
  /*0x27*/ { "mov", ix86_ArgMovXRY, IX86_CPU386 },
  /*0x28*/ { "movaps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x29*/ { "movaps", ix86_ArgXMMXD, IX86_P3MMX },
  /*0x2A*/ { "cvtpi2ps", ix86_ArgXMMXMMnD, IX86_P3MMX },
  /*0x2B*/ { "movntps", ix86_ArgXMMXD, IX86_P3MMX },
  /*0x2C*/ { "cvttps2pi", ix86_ArgMMXMMXnD, IX86_P3MMX },
  /*0x2D*/ { "cvtps2pi", ix86_ArgMMXMMXnD, IX86_P3MMX },
  /*0x2E*/ { "ucomiss", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x2F*/ { "comiss", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x30*/ { "wrmsr", NULL, IX86_CPU586 },
  /*0x31*/ { "rdtsc", NULL, IX86_CPU586 },
  /*0x32*/ { "rdmsr", NULL, IX86_CPU586 },
  /*0x33*/ { "rdpmc", NULL, IX86_CPU686 },
  /*0x34*/ { "sysenter", NULL, IX86_P2 },
  /*0x35*/ { "sysexit", NULL, IX86_P2 },
  /*0x36*/ { "rdshr", NULL, IX86_CYRIX686 },
  /*0x37*/ { "wrshr", NULL, IX86_CYRIX686 },
  /*0x38*/ { "smint", NULL, IX86_CYRIX686 },
  /*0x39*/ { "???", NULL, IX86_UNKCPU },
  /*0x3A*/ { "???", NULL, IX86_UNKCPU },
  /*0x3B*/ { "???", NULL, IX86_UNKCPU },
  /*0x3C*/ { "???", NULL, IX86_UNKCPU },
  /*0x3D*/ { "???", NULL, IX86_UNKCPU },
  /*0x3E*/ { "???", NULL, IX86_UNKCPU },
  /*0x3F*/ { "???", NULL, IX86_UNKCPU },
  /*0x40*/ { "cmovo", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x41*/ { "cmovno", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x42*/ { "cmovc", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x43*/ { "cmovnc", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x44*/ { "cmove", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x45*/ { "cmovne", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x46*/ { "cmovna", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x47*/ { "cmova", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x48*/ { "cmovs", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x49*/ { "cmovns", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x4A*/ { "cmovp", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x4B*/ { "cmovnp", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x4C*/ { "cmovl", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x4D*/ { "cmovnl", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x4E*/ { "cmovle", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x4F*/ { "cmovg", ix86_ArgModRMDW, IX86_CPU686 },
  /*0x50*/ { "movmskps", ix86_ArgXMMXRD, IX86_P3MMX },
  /*0x51*/ { "sqrtps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x52*/ { "rsqrtps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x53*/ { "rcpps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x54*/ { "andps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x55*/ { "andnps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x56*/ { "orps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x57*/ { "xorps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x58*/ { "addps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x59*/ { "mulps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x5A*/ { "cvtps2pd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5B*/ { "cvtdq2ps", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5C*/ { "subps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x5D*/ { "minps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x5E*/ { "divps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x5F*/ { "maxps", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x60*/ { "punpcklbw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x61*/ { "punpcklwd", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x62*/ { "punpckldq", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x63*/ { "packsswb", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x64*/ { "pcmpgtb", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x65*/ { "pcmpgtw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x66*/ { "pcmpgtd", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x67*/ { "packuswb", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x68*/ { "punpckhbw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x69*/ { "punpckhwd", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x6A*/ { "punpckhdq", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x6B*/ { "packssdw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x6C*/ { "???", NULL, IX86_UNKMMX },
  /*0x6D*/ { "???", NULL, IX86_UNKMMX },
  /*0x6E*/ { "movd", ix86_ArgMMXRnD, IX86_MMX586 },
  /*0x6F*/ { "movq", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x70*/ { "pshufw", ix86_ArgMMRMDigit, IX86_P3MMX },
  /*0x71*/ { "!!!", ix86_ArgMMXGr1, IX86_MMX586 },
  /*0x72*/ { "!!!", ix86_ArgMMXGr2, IX86_MMX586 },
  /*0x73*/ { "!!!", ix86_ArgMMXGr3, IX86_MMX586 },
  /*0x74*/ { "pcmpeqb", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x75*/ { "pcmpeqw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x76*/ { "pcmpeqd", ix86_ArgMMXnD, IX86_MMX586 },
  /*0x77*/ { "emms", NULL, IX86_MMX586 },
  /*0x78*/ { "svdc", ix86_ArgSegRM, IX86_CYRIX486 },
  /*0x79*/ { "rsdc", ix86_ArgSegRM, IX86_CYRIX486 },
  /*0x7A*/ { "svldt", ix86_ArgMod, IX86_CYRIX486 },
  /*0x7B*/ { "rsldt", ix86_ArgMod, IX86_CYRIX486 },
  /*0x7C*/ { "svts", ix86_ArgMod, IX86_CYRIX486 },
  /*0x7D*/ { "rsts", ix86_ArgMod, IX86_CYRIX486 },
  /*0x7E*/ { "movd", ix86_ArgMMXRD, IX86_MMX586 },
  /*0x7F*/ { "movq", ix86_ArgMMXD, IX86_MMX586 },
  /*0x80*/ { "jo", ix86_ArgNear, IX86_CPU386 },
  /*0x81*/ { "jno", ix86_ArgNear, IX86_CPU386 },
  /*0x82*/ { "jc", ix86_ArgNear, IX86_CPU386 },
  /*0x83*/ { "jnc", ix86_ArgNear, IX86_CPU386 },
  /*0x84*/ { "je", ix86_ArgNear, IX86_CPU386 },
  /*0x85*/ { "jne", ix86_ArgNear, IX86_CPU386 },
  /*0x86*/ { "jna", ix86_ArgNear, IX86_CPU386 },
  /*0x87*/ { "ja", ix86_ArgNear, IX86_CPU386 },
  /*0x88*/ { "js", ix86_ArgNear, IX86_CPU386 },
  /*0x89*/ { "jns", ix86_ArgNear, IX86_CPU386 },
  /*0x8A*/ { "jp", ix86_ArgNear, IX86_CPU386 },
  /*0x8B*/ { "jnp", ix86_ArgNear, IX86_CPU386 },
  /*0x8C*/ { "jl", ix86_ArgNear, IX86_CPU386 },
  /*0x8D*/ { "jnl", ix86_ArgNear, IX86_CPU386 },
  /*0x8E*/ { "jle", ix86_ArgNear, IX86_CPU386 },
  /*0x8F*/ { "jg", ix86_ArgNear, IX86_CPU386 },
  /*0x90*/ { "seto", ix86_ArgModB, IX86_CPU386 },
  /*0x91*/ { "setno", ix86_ArgModB, IX86_CPU386 },
  /*0x92*/ { "setc", ix86_ArgModB, IX86_CPU386 },
  /*0x93*/ { "setnc", ix86_ArgModB, IX86_CPU386 },
  /*0x94*/ { "sete", ix86_ArgModB, IX86_CPU386 },
  /*0x95*/ { "setne", ix86_ArgModB, IX86_CPU386 },
  /*0x96*/ { "setna", ix86_ArgModB, IX86_CPU386 },
  /*0x97*/ { "seta", ix86_ArgModB, IX86_CPU386 },
  /*0x98*/ { "sets", ix86_ArgModB, IX86_CPU386 },
  /*0x99*/ { "setns", ix86_ArgModB, IX86_CPU386 },
  /*0x9A*/ { "setp", ix86_ArgModB, IX86_CPU386 },
  /*0x9B*/ { "setnp", ix86_ArgModB, IX86_CPU386 },
  /*0x9C*/ { "setl", ix86_ArgModB, IX86_CPU386 },
  /*0x9D*/ { "setnl", ix86_ArgModB, IX86_CPU386 },
  /*0x9E*/ { "setle", ix86_ArgModB, IX86_CPU386 },
  /*0x9F*/ { "setg", ix86_ArgModB, IX86_CPU386 },
  /*0xA0*/ { "push", ix86_ArgFS, IX86_CPU386 },
  /*0xA1*/ { "pop", ix86_ArgFS, IX86_CPU386 },
  /*0xA2*/ { "cpuid", NULL, IX86_CPU486 },
  /*0xA3*/ { "bt", ix86_ArgModRMnDW, IX86_CPU386 },
  /*0xA4*/ { "shld", ix86_DblShift, IX86_CPU386 },
  /*0xA5*/ { "shld", ix86_DblShift, IX86_CPU386 },
  /*0xA6*/ { "???", NULL, IX86_UNKCPU },
  /*0xA7*/ { "???", NULL, IX86_UNKCPU },
  /*0xA8*/ { "push", ix86_ArgGS, IX86_CPU386 },
  /*0xA9*/ { "pop", ix86_ArgGS, IX86_CPU386 },
  /*0xAA*/ { "rsm", NULL, IX86_CPU586 },
  /*0xAB*/ { "bts", ix86_ArgModRMnDW, IX86_CPU386 },
  /*0xAC*/ { "shrd", ix86_DblShift, IX86_CPU386 },
  /*0xAD*/ { "shrd", ix86_DblShift, IX86_CPU386 },
  /*0xAE*/ { "!!!", ix86_ArgKatmaiGrp1, IX86_P3FPU },
  /*0xAF*/ { "imul", ix86_ArgModRMDW, IX86_CPU386 },
  /*0xB0*/ { "cmpxchg", ix86_ArgModRMnDnW, IX86_CPU486 },
  /*0xB1*/ { "cmpxchg", ix86_ArgModRMnDW, IX86_CPU486 },
  /*0xB2*/ { "lss", ix86_ArgModRMDW, IX86_CPU386 },
  /*0xB3*/ { "btr", ix86_ArgModRMnDW, IX86_CPU386 },
  /*0xB4*/ { "lfs", ix86_ArgModRMDW, IX86_CPU386 },
  /*0xB5*/ { "lgs", ix86_ArgModRMDW, IX86_CPU386 },
  /*0xB6*/ { "movzx", ix86_ArgMovYX, IX86_CPU386 },
  /*0xB7*/ { "movzx", ix86_ArgMovYX, IX86_CPU386 },
  /*0xB8*/ { "???", NULL, IX86_UNKCPU },
  /*0xB9*/ { "ud2", NULL, IX86_CPU686 },
  /*0xBA*/ { "!!!", ix86_BitGrp, IX86_CPU386 },
  /*0xBB*/ { "btc", ix86_ArgModRMnDW, IX86_CPU386 },
  /*0xBC*/ { "bsf", ix86_ArgModRMDW, IX86_CPU386 },
  /*0xBD*/ { "bsr", ix86_ArgModRMDW, IX86_CPU386 },
  /*0xBE*/ { "movsx", ix86_ArgMovYX, IX86_CPU386 },
  /*0xBF*/ { "movsx", ix86_ArgMovYX, IX86_CPU386 },
  /*0xC0*/ { "xadd", ix86_ArgModRMnDnW, IX86_CPU486 },
  /*0xC1*/ { "xadd", ix86_ArgModRMnDW, IX86_CPU486 },
  /*0xC2*/ { "cmpps", ix86_ArgXMMCmp, IX86_P3MMX },
  /*0xC3*/ { "movnti", ix86_ArgModRMnDW, IX86_P4 },
  /*0xC4*/ { "pinsrw", ix86_ArgXMMRegDigit, IX86_P3MMX },
  /*0xC5*/ { "pextrw", ix86_ArgRegXMMDigit, IX86_P3MMX },
  /*0xC6*/ { "shufps", ix86_ArgXMMRMDigit, IX86_P3MMX },
  /*0xC7*/ { "cmpxchg8b", ix86_ArgMod, IX86_CPU586 },
  /*0xC8*/ { "bswap", ix86_ArgIReg, IX86_CPU486 },
  /*0xC9*/ { "bswap", ix86_ArgIReg, IX86_CPU486 },
  /*0xCA*/ { "bswap", ix86_ArgIReg, IX86_CPU486 },
  /*0xCB*/ { "bswap", ix86_ArgIReg, IX86_CPU486 },
  /*0xCC*/ { "bswap", ix86_ArgIReg, IX86_CPU486 },
  /*0xCD*/ { "bswap", ix86_ArgIReg, IX86_CPU486 },
  /*0xCE*/ { "bswap", ix86_ArgIReg, IX86_CPU486 },
  /*0xCF*/ { "bswap", ix86_ArgIReg, IX86_CPU486 },
  /*0xD0*/ { "???", ix86_ArgMMXnD, IX86_UNKMMX },
  /*0xD1*/ { "psrlw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xD2*/ { "psrld", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xD3*/ { "psrlq", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xD4*/ { "paddq", ix86_ArgMMXnD, IX86_P4MMX },
  /*0xD5*/ { "pmullw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xD6*/ { "???", ix86_ArgMMXnD, IX86_UNKMMX },
  /*0xD7*/ { "pmovmskb", ix86_ArgRMMXRevnD, IX86_P3MMX },
  /*0xD8*/ { "psubusb", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xD9*/ { "psubusw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xDA*/ { "pminub", ix86_ArgMMXnD, IX86_P3MMX },
  /*0xDB*/ { "pand", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xDC*/ { "paddusb", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xDD*/ { "paddusw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xDE*/ { "pmaxub", ix86_ArgMMXnD, IX86_P3MMX },
  /*0xDF*/ { "pandn", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xE0*/ { "pavgb", ix86_ArgMMXnD, IX86_P3MMX },
  /*0xE1*/ { "psraw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xE2*/ { "psrad", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xE3*/ { "pavgw", ix86_ArgMMXnD, IX86_P3MMX },
  /*0xE4*/ { "pmulhuw", ix86_ArgMMXnD, IX86_P3MMX },
  /*0xE5*/ { "pmulhw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xE6*/ { "???", ix86_ArgMMXnD, IX86_UNKMMX },
  /*0xE7*/ { "movntq", ix86_ArgMMXD, IX86_P3MMX },
  /*0xE8*/ { "psubsb", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xE9*/ { "psubsw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xEA*/ { "pminsw", ix86_ArgMMXnD, IX86_P3MMX },
  /*0xEB*/ { "por", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xEC*/ { "paddsb", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xED*/ { "paddsw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xEE*/ { "pmaxsw", ix86_ArgMMXnD, IX86_P3MMX },
  /*0xEF*/ { "pxor", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xF0*/ { "???", ix86_ArgMMXnD, IX86_UNKMMX },
  /*0xF1*/ { "psllw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xF2*/ { "pslld", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xF3*/ { "psllq", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xF4*/ { "pmuludq", ix86_ArgMMXnD, IX86_P4MMX },
  /*0xF5*/ { "pmaddwd", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xF6*/ { "psadbw", ix86_ArgMMXnD, IX86_P3MMX },
  /*0xF7*/ { "maskmovq", ix86_ArgMMXnD, IX86_P3MMX },
  /*0xF8*/ { "psubb", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xF9*/ { "psubw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xFA*/ { "psubd", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xFB*/ { "psubq", ix86_ArgMMXnD, IX86_P4MMX },
  /*0xFC*/ { "paddb", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xFD*/ { "paddw", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xFE*/ { "paddd", ix86_ArgMMXnD, IX86_MMX586 },
  /*0xFF*/ { "???", ix86_ArgMMXnD, IX86_UNKMMX }
};

ix86_3dNowopcodes ix86_3dNowtable[256] =
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

ix86_ExOpcodes ix86_660F_PentiumTable[256] =
{
  /*0x00*/ { NULL, NULL, IX86_UNKMMX },
  /*0x01*/ { NULL, NULL, IX86_UNKMMX },
  /*0x02*/ { NULL, NULL, IX86_UNKMMX },
  /*0x03*/ { NULL, NULL, IX86_UNKMMX },
  /*0x04*/ { NULL, NULL, IX86_UNKMMX },
  /*0x05*/ { NULL, NULL, IX86_UNKMMX },
  /*0x06*/ { NULL, NULL, IX86_UNKMMX },
  /*0x07*/ { NULL, NULL, IX86_UNKMMX },
  /*0x08*/ { NULL, NULL, IX86_UNKMMX },
  /*0x09*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x10*/ { "movupd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x11*/ { "movupd", ix86_ArgXMMXD, IX86_P4MMX },
  /*0x12*/ { "movlpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x13*/ { "movlpd", ix86_ArgXMMXD, IX86_P4MMX },
  /*0x14*/ { "unpcklpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x15*/ { "unpckhpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x16*/ { "movhpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x17*/ { "movhpd", ix86_ArgXMMXD, IX86_P4MMX },
  /*0x18*/ { NULL, NULL, IX86_UNKMMX },
  /*0x19*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x20*/ { NULL, NULL, IX86_UNKMMX },
  /*0x21*/ { NULL, NULL, IX86_UNKMMX },
  /*0x22*/ { NULL, NULL, IX86_UNKMMX },
  /*0x23*/ { NULL, NULL, IX86_UNKMMX },
  /*0x24*/ { NULL, NULL, IX86_UNKMMX },
  /*0x25*/ { NULL, NULL, IX86_UNKMMX },
  /*0x26*/ { NULL, NULL, IX86_UNKMMX },
  /*0x27*/ { NULL, NULL, IX86_UNKMMX },
  /*0x28*/ { "movapd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x29*/ { "movapd", ix86_ArgXMMXD, IX86_P4MMX },
  /*0x2A*/ { "cvtpi2pd", ix86_ArgXMMXMMnD, IX86_P4MMX },
  /*0x2B*/ { "movntpd", ix86_ArgXMMXD, IX86_P4MMX },
  /*0x2C*/ { "cvttpd2pi", ix86_ArgMMXMMXnD, IX86_P4MMX },
  /*0x2D*/ { "cvtpd2pi", ix86_ArgMMXMMXnD, IX86_P4MMX },
  /*0x2E*/ { "ucomisd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x2F*/ { "comisd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x30*/ { NULL, NULL, IX86_UNKMMX },
  /*0x31*/ { NULL, NULL, IX86_UNKMMX },
  /*0x32*/ { NULL, NULL, IX86_UNKMMX },
  /*0x33*/ { NULL, NULL, IX86_UNKMMX },
  /*0x34*/ { NULL, NULL, IX86_UNKMMX },
  /*0x35*/ { NULL, NULL, IX86_UNKMMX },
  /*0x36*/ { NULL, NULL, IX86_UNKMMX },
  /*0x37*/ { NULL, NULL, IX86_UNKMMX },
  /*0x38*/ { NULL, NULL, IX86_UNKMMX },
  /*0x39*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x40*/ { NULL, NULL, IX86_UNKMMX },
  /*0x41*/ { NULL, NULL, IX86_UNKMMX },
  /*0x42*/ { NULL, NULL, IX86_UNKMMX },
  /*0x43*/ { NULL, NULL, IX86_UNKMMX },
  /*0x44*/ { NULL, NULL, IX86_UNKMMX },
  /*0x45*/ { NULL, NULL, IX86_UNKMMX },
  /*0x46*/ { NULL, NULL, IX86_UNKMMX },
  /*0x47*/ { NULL, NULL, IX86_UNKMMX },
  /*0x48*/ { NULL, NULL, IX86_UNKMMX },
  /*0x49*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x50*/ { "movmskpd", ix86_ArgXMMXRD, IX86_P4MMX },
  /*0x51*/ { "sqrtpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x52*/ { NULL, NULL, IX86_UNKMMX },
  /*0x53*/ { NULL, NULL, IX86_UNKMMX },
  /*0x54*/ { "andpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x55*/ { "andnpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x56*/ { "orpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x57*/ { "xorpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x58*/ { "addpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x59*/ { "mulpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5A*/ { "cvtpd2ps", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5B*/ { "cvtps2dq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5C*/ { "subpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5D*/ { "minpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5E*/ { "divpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5F*/ { "maxpd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x60*/ { "punpcklbw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x61*/ { "punpcklwd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x62*/ { "punpckldq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x63*/ { "packsswb", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x64*/ { "pcmpgtb", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x65*/ { "pcmpgtw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x66*/ { "pcmpgtd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x67*/ { "packuswb", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x68*/ { "punpckhbw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x69*/ { "punpckhwd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x6A*/ { "punpckhdq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x6B*/ { "packssdw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x6C*/ { "punpcklqdq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x6D*/ { "punpckhqdq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x6E*/ { "movd", ix86_ArgXMMXRnD, IX86_P4MMX },
  /*0x6F*/ { "movdqa", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x70*/ { "pshufd", ix86_ArgXMMRMDigit, IX86_P4MMX },
  /*0x71*/ { "!!!", ix86_ArgXMMXGr1, IX86_P4MMX },
  /*0x72*/ { "!!!", ix86_ArgXMMXGr2, IX86_P4MMX },
  /*0x73*/ { "!!!", ix86_ArgXMMXGr3, IX86_P4MMX },
  /*0x74*/ { "pcmpeqb", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x75*/ { "pcmpeqw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x76*/ { "pcmpeqd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x77*/ { NULL, NULL, IX86_UNKMMX },
  /*0x78*/ { NULL, NULL, IX86_UNKMMX },
  /*0x79*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7E*/ { "movd", ix86_ArgXMMXRD, IX86_P4MMX },
  /*0x7F*/ { "movdqa", ix86_ArgXMMXD, IX86_P4MMX },
  /*0x80*/ { NULL, NULL, IX86_UNKMMX },
  /*0x81*/ { NULL, NULL, IX86_UNKMMX },
  /*0x82*/ { NULL, NULL, IX86_UNKMMX },
  /*0x83*/ { NULL, NULL, IX86_UNKMMX },
  /*0x84*/ { NULL, NULL, IX86_UNKMMX },
  /*0x85*/ { NULL, NULL, IX86_UNKMMX },
  /*0x86*/ { NULL, NULL, IX86_UNKMMX },
  /*0x87*/ { NULL, NULL, IX86_UNKMMX },
  /*0x88*/ { NULL, NULL, IX86_UNKMMX },
  /*0x89*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x90*/ { NULL, NULL, IX86_UNKMMX },
  /*0x91*/ { NULL, NULL, IX86_UNKMMX },
  /*0x92*/ { NULL, NULL, IX86_UNKMMX },
  /*0x93*/ { NULL, NULL, IX86_UNKMMX },
  /*0x94*/ { NULL, NULL, IX86_UNKMMX },
  /*0x95*/ { NULL, NULL, IX86_UNKMMX },
  /*0x96*/ { NULL, NULL, IX86_UNKMMX },
  /*0x97*/ { NULL, NULL, IX86_UNKMMX },
  /*0x98*/ { NULL, NULL, IX86_UNKMMX },
  /*0x99*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9F*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA2*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA6*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAD*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAF*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB2*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB6*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBD*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBF*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC2*/ { "cmppd", ix86_ArgXMMCmp, IX86_P4MMX },
  /*0xC3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC4*/ { "pinsrw", ix86_ArgXMMXRegDigit, IX86_P4MMX },
  /*0xC5*/ { "pextrw", ix86_ArgRegXMMXDigit, IX86_P4MMX },
  /*0xC6*/ { "shufpd", ix86_ArgXMMRMDigit, IX86_P4MMX },
  /*0xC7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCD*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCF*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD1*/ { "psrlw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xD2*/ { "psrld", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xD3*/ { "psrlq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xD4*/ { "paddq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xD5*/ { "pmullw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xD6*/ { "movq", ix86_ArgXMMXD, IX86_P4MMX },
  /*0xD7*/ { "pmovmskb", ix86_ArgRXMMXRevnD, IX86_P4MMX },
  /*0xD8*/ { "psubusb", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xD9*/ { "psubusw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xDA*/ { "pminub", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xDB*/ { "pand", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xDC*/ { "paddusb", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xDD*/ { "paddusw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xDE*/ { "pmaxub", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xDF*/ { "pandn", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xE0*/ { "pavgb", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xE1*/ { "psraw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xE2*/ { "psrad", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xE3*/ { "pavgw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xE4*/ { "pmulhuw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xE5*/ { "pmulhw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xE6*/ { "cvttpd2dq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xE7*/ { "movntdq", ix86_ArgXMMXD, IX86_P4MMX },
  /*0xE8*/ { "psubsb", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xE9*/ { "psubsw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xEA*/ { "pminsw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xEB*/ { "por", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xEC*/ { "paddsb", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xED*/ { "paddsw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xEE*/ { "pmaxsw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xEF*/ { "pxor", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xF0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF1*/ { "psllw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xF2*/ { "pslld", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xF3*/ { "psllq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xF4*/ { "pmuludq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xF5*/ { "pmaddwd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xF6*/ { "psadbw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xF7*/ { "maskmovdqu", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xF8*/ { "psubb", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xF9*/ { "psubw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xFA*/ { "psubd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xFB*/ { "psubq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xFC*/ { "paddb", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xFD*/ { "paddw", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xFE*/ { "paddd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xFF*/ { NULL, NULL, IX86_UNKMMX }
};

ix86_ExOpcodes ix86_F20F_PentiumTable[256] =
{
  /*0x00*/ { NULL, NULL, IX86_UNKMMX },
  /*0x01*/ { NULL, NULL, IX86_UNKMMX },
  /*0x02*/ { NULL, NULL, IX86_UNKMMX },
  /*0x03*/ { NULL, NULL, IX86_UNKMMX },
  /*0x04*/ { NULL, NULL, IX86_UNKMMX },
  /*0x05*/ { NULL, NULL, IX86_UNKMMX },
  /*0x06*/ { NULL, NULL, IX86_UNKMMX },
  /*0x07*/ { NULL, NULL, IX86_UNKMMX },
  /*0x08*/ { NULL, NULL, IX86_UNKMMX },
  /*0x09*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x10*/ { "movsd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x11*/ { "movsd", ix86_ArgXMMXD, IX86_P4MMX },
  /*0x12*/ { NULL, NULL, IX86_UNKMMX },
  /*0x13*/ { NULL, NULL, IX86_UNKMMX },
  /*0x14*/ { NULL, NULL, IX86_UNKMMX },
  /*0x15*/ { NULL, NULL, IX86_UNKMMX },
  /*0x16*/ { NULL, NULL, IX86_UNKMMX },
  /*0x17*/ { NULL, NULL, IX86_UNKMMX },
  /*0x18*/ { NULL, NULL, IX86_UNKMMX },
  /*0x19*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x20*/ { NULL, NULL, IX86_UNKMMX },
  /*0x21*/ { NULL, NULL, IX86_UNKMMX },
  /*0x22*/ { NULL, NULL, IX86_UNKMMX },
  /*0x23*/ { NULL, NULL, IX86_UNKMMX },
  /*0x24*/ { NULL, NULL, IX86_UNKMMX },
  /*0x25*/ { NULL, NULL, IX86_UNKMMX },
  /*0x26*/ { NULL, NULL, IX86_UNKMMX },
  /*0x27*/ { NULL, NULL, IX86_UNKMMX },
  /*0x28*/ { NULL, NULL, IX86_UNKMMX },
  /*0x29*/ { NULL, NULL, IX86_UNKMMX },
  /*0x2A*/ { "cvtsi2sd", ix86_ArgRXMMXnD, IX86_P4MMX },
  /*0x2B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x2C*/ { "cvttsd2si", ix86_ArgRXMMXRevnD, IX86_P4MMX },
  /*0x2D*/ { "cvtsd2si", ix86_ArgRXMMXRevnD, IX86_P4MMX },
  /*0x2E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x2F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x30*/ { NULL, NULL, IX86_UNKMMX },
  /*0x31*/ { NULL, NULL, IX86_UNKMMX },
  /*0x32*/ { NULL, NULL, IX86_UNKMMX },
  /*0x33*/ { NULL, NULL, IX86_UNKMMX },
  /*0x34*/ { NULL, NULL, IX86_UNKMMX },
  /*0x35*/ { NULL, NULL, IX86_UNKMMX },
  /*0x36*/ { NULL, NULL, IX86_UNKMMX },
  /*0x37*/ { NULL, NULL, IX86_UNKMMX },
  /*0x38*/ { NULL, NULL, IX86_UNKMMX },
  /*0x39*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x40*/ { NULL, NULL, IX86_UNKMMX },
  /*0x41*/ { NULL, NULL, IX86_UNKMMX },
  /*0x42*/ { NULL, NULL, IX86_UNKMMX },
  /*0x43*/ { NULL, NULL, IX86_UNKMMX },
  /*0x44*/ { NULL, NULL, IX86_UNKMMX },
  /*0x45*/ { NULL, NULL, IX86_UNKMMX },
  /*0x46*/ { NULL, NULL, IX86_UNKMMX },
  /*0x47*/ { NULL, NULL, IX86_UNKMMX },
  /*0x48*/ { NULL, NULL, IX86_UNKMMX },
  /*0x49*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x50*/ { NULL, NULL, IX86_UNKMMX },
  /*0x51*/ { "sqrtsd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x52*/ { NULL, NULL, IX86_UNKMMX },
  /*0x53*/ { NULL, NULL, IX86_UNKMMX },
  /*0x54*/ { NULL, NULL, IX86_UNKMMX },
  /*0x55*/ { NULL, NULL, IX86_UNKMMX },
  /*0x56*/ { NULL, NULL, IX86_UNKMMX },
  /*0x57*/ { NULL, NULL, IX86_UNKMMX },
  /*0x58*/ { "addsd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x59*/ { "mulsd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5A*/ { "cvtsd2ss", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x5C*/ { "subsd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5D*/ { "minsd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5E*/ { "divsd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5F*/ { "maxsd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x60*/ { NULL, NULL, IX86_UNKMMX },
  /*0x61*/ { NULL, NULL, IX86_UNKMMX },
  /*0x62*/ { NULL, NULL, IX86_UNKMMX },
  /*0x63*/ { NULL, NULL, IX86_UNKMMX },
  /*0x64*/ { NULL, NULL, IX86_UNKMMX },
  /*0x65*/ { NULL, NULL, IX86_UNKMMX },
  /*0x66*/ { NULL, NULL, IX86_UNKMMX },
  /*0x67*/ { NULL, NULL, IX86_UNKMMX },
  /*0x68*/ { NULL, NULL, IX86_UNKMMX },
  /*0x69*/ { NULL, NULL, IX86_UNKMMX },
  /*0x6A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x6B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x6C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x6D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x6E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x6F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x70*/ { "pshuflw", ix86_ArgXMMRMDigit, IX86_P4MMX },
  /*0x71*/ { NULL, NULL, IX86_UNKMMX },
  /*0x72*/ { NULL, NULL, IX86_UNKMMX },
  /*0x73*/ { NULL, NULL, IX86_UNKMMX },
  /*0x74*/ { NULL, NULL, IX86_UNKMMX },
  /*0x75*/ { NULL, NULL, IX86_UNKMMX },
  /*0x76*/ { NULL, NULL, IX86_UNKMMX },
  /*0x77*/ { NULL, NULL, IX86_UNKMMX },
  /*0x78*/ { NULL, NULL, IX86_UNKMMX },
  /*0x79*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x80*/ { NULL, NULL, IX86_UNKMMX },
  /*0x81*/ { NULL, NULL, IX86_UNKMMX },
  /*0x82*/ { NULL, NULL, IX86_UNKMMX },
  /*0x83*/ { NULL, NULL, IX86_UNKMMX },
  /*0x84*/ { NULL, NULL, IX86_UNKMMX },
  /*0x85*/ { NULL, NULL, IX86_UNKMMX },
  /*0x86*/ { NULL, NULL, IX86_UNKMMX },
  /*0x87*/ { NULL, NULL, IX86_UNKMMX },
  /*0x88*/ { NULL, NULL, IX86_UNKMMX },
  /*0x89*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x90*/ { NULL, NULL, IX86_UNKMMX },
  /*0x91*/ { NULL, NULL, IX86_UNKMMX },
  /*0x92*/ { NULL, NULL, IX86_UNKMMX },
  /*0x93*/ { NULL, NULL, IX86_UNKMMX },
  /*0x94*/ { NULL, NULL, IX86_UNKMMX },
  /*0x95*/ { NULL, NULL, IX86_UNKMMX },
  /*0x96*/ { NULL, NULL, IX86_UNKMMX },
  /*0x97*/ { NULL, NULL, IX86_UNKMMX },
  /*0x98*/ { NULL, NULL, IX86_UNKMMX },
  /*0x99*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9F*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA2*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA6*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAD*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAF*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB2*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB6*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBD*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBF*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC2*/ { "cmpsd", ix86_ArgXMMCmp, IX86_P4MMX },
  /*0xC3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC6*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCD*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCF*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD2*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD6*/ { "movdq2q", ix86_ArgMMXMMXnD, IX86_P4MMX },
  /*0xD7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xDA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xDB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xDC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xDD*/ { NULL, NULL, IX86_UNKMMX },
  /*0xDE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xDF*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE2*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE6*/ { "cvtpd2dq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xE7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xEA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xEB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xEC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xED*/ { NULL, NULL, IX86_UNKMMX },
  /*0xEE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xEF*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF2*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF6*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xFA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xFB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xFC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xFD*/ { NULL, NULL, IX86_UNKMMX },
  /*0xFE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xFF*/ { NULL, NULL, IX86_UNKMMX }
};

ix86_ExOpcodes ix86_F30F_PentiumTable[256] =
{
  /*0x00*/ { NULL, NULL, IX86_UNKMMX },
  /*0x01*/ { NULL, NULL, IX86_UNKMMX },
  /*0x02*/ { NULL, NULL, IX86_UNKMMX },
  /*0x03*/ { NULL, NULL, IX86_UNKMMX },
  /*0x04*/ { NULL, NULL, IX86_UNKMMX },
  /*0x05*/ { NULL, NULL, IX86_UNKMMX },
  /*0x06*/ { NULL, NULL, IX86_UNKMMX },
  /*0x07*/ { NULL, NULL, IX86_UNKMMX },
  /*0x08*/ { NULL, NULL, IX86_UNKMMX },
  /*0x09*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x0F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x10*/ { "movss", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x11*/ { "movss", ix86_ArgXMMXD, IX86_P3MMX },
  /*0x12*/ { NULL, NULL, IX86_UNKMMX },
  /*0x13*/ { NULL, NULL, IX86_UNKMMX },
  /*0x14*/ { NULL, NULL, IX86_UNKMMX },
  /*0x15*/ { NULL, NULL, IX86_UNKMMX },
  /*0x16*/ { NULL, NULL, IX86_UNKMMX },
  /*0x17*/ { NULL, NULL, IX86_UNKMMX },
  /*0x18*/ { NULL, NULL, IX86_UNKMMX },
  /*0x19*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x1F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x20*/ { NULL, NULL, IX86_UNKMMX },
  /*0x21*/ { NULL, NULL, IX86_UNKMMX },
  /*0x22*/ { NULL, NULL, IX86_UNKMMX },
  /*0x23*/ { NULL, NULL, IX86_UNKMMX },
  /*0x24*/ { NULL, NULL, IX86_UNKMMX },
  /*0x25*/ { NULL, NULL, IX86_UNKMMX },
  /*0x26*/ { NULL, NULL, IX86_UNKMMX },
  /*0x27*/ { NULL, NULL, IX86_UNKMMX },
  /*0x28*/ { NULL, NULL, IX86_UNKMMX },
  /*0x29*/ { NULL, NULL, IX86_UNKMMX },
  /*0x2A*/ { "cvtsi2ss", ix86_ArgRXMMXnD, IX86_P3MMX },
  /*0x2B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x2C*/ { "cvttss2si", ix86_ArgRXMMXRevnD, IX86_P3MMX },
  /*0x2D*/ { "cvtss2si", ix86_ArgRXMMXRevnD, IX86_P3MMX },
  /*0x2E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x2F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x30*/ { NULL, NULL, IX86_UNKMMX },
  /*0x31*/ { NULL, NULL, IX86_UNKMMX },
  /*0x32*/ { NULL, NULL, IX86_UNKMMX },
  /*0x33*/ { NULL, NULL, IX86_UNKMMX },
  /*0x34*/ { NULL, NULL, IX86_UNKMMX },
  /*0x35*/ { NULL, NULL, IX86_UNKMMX },
  /*0x36*/ { NULL, NULL, IX86_UNKMMX },
  /*0x37*/ { NULL, NULL, IX86_UNKMMX },
  /*0x38*/ { NULL, NULL, IX86_UNKMMX },
  /*0x39*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x3F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x40*/ { NULL, NULL, IX86_UNKMMX },
  /*0x41*/ { NULL, NULL, IX86_UNKMMX },
  /*0x42*/ { NULL, NULL, IX86_UNKMMX },
  /*0x43*/ { NULL, NULL, IX86_UNKMMX },
  /*0x44*/ { NULL, NULL, IX86_UNKMMX },
  /*0x45*/ { NULL, NULL, IX86_UNKMMX },
  /*0x46*/ { NULL, NULL, IX86_UNKMMX },
  /*0x47*/ { NULL, NULL, IX86_UNKMMX },
  /*0x48*/ { NULL, NULL, IX86_UNKMMX },
  /*0x49*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x4F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x50*/ { NULL, NULL, IX86_UNKMMX },
  /*0x51*/ { "sqrtss", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x52*/ { "rsqrtss", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x53*/ { "rcpss", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x54*/ { NULL, NULL, IX86_UNKMMX },
  /*0x55*/ { NULL, NULL, IX86_UNKMMX },
  /*0x56*/ { NULL, NULL, IX86_UNKMMX },
  /*0x57*/ { NULL, NULL, IX86_UNKMMX },
  /*0x58*/ { "addss", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x59*/ { "mulss", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x5A*/ { "cvtss2sd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5B*/ { "cvttps2dq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x5C*/ { "subss", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x5D*/ { "minss", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x5E*/ { "divss", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x5F*/ { "maxss", ix86_ArgXMMXnD, IX86_P3MMX },
  /*0x60*/ { NULL, NULL, IX86_UNKMMX },
  /*0x61*/ { NULL, NULL, IX86_UNKMMX },
  /*0x62*/ { NULL, NULL, IX86_UNKMMX },
  /*0x63*/ { NULL, NULL, IX86_UNKMMX },
  /*0x64*/ { NULL, NULL, IX86_UNKMMX },
  /*0x65*/ { NULL, NULL, IX86_UNKMMX },
  /*0x66*/ { NULL, NULL, IX86_UNKMMX },
  /*0x67*/ { NULL, NULL, IX86_UNKMMX },
  /*0x68*/ { NULL, NULL, IX86_UNKMMX },
  /*0x69*/ { NULL, NULL, IX86_UNKMMX },
  /*0x6A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x6B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x6C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x6D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x6E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x6F*/ { "movdqu", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x70*/ { "pshufhw", ix86_ArgXMMRMDigit, IX86_P4MMX },
  /*0x71*/ { NULL, NULL, IX86_UNKMMX },
  /*0x72*/ { NULL, NULL, IX86_UNKMMX },
  /*0x73*/ { NULL, NULL, IX86_UNKMMX },
  /*0x74*/ { NULL, NULL, IX86_UNKMMX },
  /*0x75*/ { NULL, NULL, IX86_UNKMMX },
  /*0x76*/ { NULL, NULL, IX86_UNKMMX },
  /*0x77*/ { NULL, NULL, IX86_UNKMMX },
  /*0x78*/ { NULL, NULL, IX86_UNKMMX },
  /*0x79*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x7E*/ { "movq", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0x7F*/ { "movdqu", ix86_ArgXMMXD, IX86_P4MMX },
  /*0x80*/ { NULL, NULL, IX86_UNKMMX },
  /*0x81*/ { NULL, NULL, IX86_UNKMMX },
  /*0x82*/ { NULL, NULL, IX86_UNKMMX },
  /*0x83*/ { NULL, NULL, IX86_UNKMMX },
  /*0x84*/ { NULL, NULL, IX86_UNKMMX },
  /*0x85*/ { NULL, NULL, IX86_UNKMMX },
  /*0x86*/ { NULL, NULL, IX86_UNKMMX },
  /*0x87*/ { NULL, NULL, IX86_UNKMMX },
  /*0x88*/ { NULL, NULL, IX86_UNKMMX },
  /*0x89*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x8F*/ { NULL, NULL, IX86_UNKMMX },
  /*0x90*/ { NULL, NULL, IX86_UNKMMX },
  /*0x91*/ { NULL, NULL, IX86_UNKMMX },
  /*0x92*/ { NULL, NULL, IX86_UNKMMX },
  /*0x93*/ { NULL, NULL, IX86_UNKMMX },
  /*0x94*/ { NULL, NULL, IX86_UNKMMX },
  /*0x95*/ { NULL, NULL, IX86_UNKMMX },
  /*0x96*/ { NULL, NULL, IX86_UNKMMX },
  /*0x97*/ { NULL, NULL, IX86_UNKMMX },
  /*0x98*/ { NULL, NULL, IX86_UNKMMX },
  /*0x99*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9A*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9B*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9C*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9D*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9E*/ { NULL, NULL, IX86_UNKMMX },
  /*0x9F*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA2*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA6*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xA9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAD*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xAF*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB2*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB6*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xB9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBD*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xBF*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC2*/ { "cmpss", ix86_ArgXMMCmp, IX86_P3MMX },
  /*0xC3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC6*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xC9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCD*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xCF*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD2*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD6*/ { "movq2dq", ix86_ArgMMXMMXD, IX86_P4MMX },
  /*0xD7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xD9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xDA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xDB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xDC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xDD*/ { NULL, NULL, IX86_UNKMMX },
  /*0xDE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xDF*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE2*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE6*/ { "cvtdq2pd", ix86_ArgXMMXnD, IX86_P4MMX },
  /*0xE7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xE9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xEA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xEB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xEC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xED*/ { NULL, NULL, IX86_UNKMMX },
  /*0xEE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xEF*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF0*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF1*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF2*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF3*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF4*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF5*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF6*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF7*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF8*/ { NULL, NULL, IX86_UNKMMX },
  /*0xF9*/ { NULL, NULL, IX86_UNKMMX },
  /*0xFA*/ { NULL, NULL, IX86_UNKMMX },
  /*0xFB*/ { NULL, NULL, IX86_UNKMMX },
  /*0xFC*/ { NULL, NULL, IX86_UNKMMX },
  /*0xFD*/ { NULL, NULL, IX86_UNKMMX },
  /*0xFE*/ { NULL, NULL, IX86_UNKMMX },
  /*0xFF*/ { NULL, NULL, IX86_UNKMMX }
};
#if 0
ix86_Opcodes ix86_NullTable[256] =
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


static unsigned x86_Bitness = DAB_AUTO;
static unsigned BITNESS = DAB_AUTO;

char *ix86_voidstr;
char *ix86_da_out;

char ix86_segpref[4] = "";

tBool Use32Addr,Use32Data,UseMMXSet,UseXMMXSet;

const unsigned char leave_insns[] = { 0x07, 0x17, 0x1F, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x61, 0x90, 0xC9 };

static tBool is_listed(unsigned char insn,const unsigned char *list,size_t listsize)
{
  size_t i;
  for(i = 0;i < listsize;i++) if(insn == list[i]) return True;
  return False;
}

static void ix86_gettype(DisasmRet *dret,ix86Param *_DisP)
{
 MBuffer insn;
 char ua,ud,up,has_lock,has_rep,has_seg;
 insn = &_DisP->RealCmd[0];
 dret->pro_clone = __INSNT_ORDINAL;
 has_lock = has_rep = has_seg = 0;
 up = ua = ud = 0;
 RepeateByPrefix:
 if(has_lock + has_rep > 1 || has_seg > 1 || ua > 1 || ud > 1) goto get_type;
 /** do prefixes loop */
 switch(insn[0])
 {
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
              /** First check for SSE2 extensions */
              if(insn[1] == 0x0F && ix86_660F_PentiumTable[insn[2]].name)
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
              /** First check for SSE2 extensions */
              if(insn[1] == 0x0F && ix86_F20F_PentiumTable[insn[2]].name)
                goto get_type;
              if(has_rep + has_lock)  break;
              has_rep++;
              goto MakePref;
   case 0xF3:
              /** First check for SSE extensions */
              if(insn[1] == 0x0F && ix86_F30F_PentiumTable[insn[2]].name)
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

static DisasmRet __FASTCALL__ ix86Disassembler(unsigned long ulShift,
                                               MBuffer buffer,
                                               unsigned flags)
{
 unsigned char code;
 DisasmRet Ret;
 ix86Param DisP;
 char ua,ud,up,has_lock,has_rep,has_seg;

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

 if(x86_Bitness == DAB_USE32)
 {
    Use32Data = Use32Data ? False : True;
    Use32Addr = Use32Addr ? False : True;
 }
 Ret.str = ix86_voidstr;
 Ret.str[0] = 0;
 RepeateByPrefix:
 if(has_lock + has_rep > 1 || has_seg > 1 || ua > 1 || ud > 1)
 {
   DisP.codelen = 0;
   strcpy(ix86_voidstr,"???");
   goto ExitDisAsm;
 }
 /** do prefixes loop */
 switch(code)
 {
   case 0x26:
              if(has_seg) break;
              strcpy(ix86_segpref,"es:");
              has_seg++;
              goto MakePref;
   case 0x2E:
              if(has_seg) break;
              strcpy(ix86_segpref,"cs:");
              has_seg++;
              goto MakePref;
   case 0x36:
              if(has_seg) break;
              strcpy(ix86_segpref,"ss:");
              has_seg++;
              goto MakePref;
   case 0x3E:
              if(has_seg) break;
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
              /** First check for SSE2 extensions */
              if(DisP.RealCmd[1] == 0x0F && ix86_660F_PentiumTable[DisP.RealCmd[2]].name)
              {
                /** therefore it non rep prefix, but instruction of SSE2 */
                DisP.RealCmd = &DisP.RealCmd[2];
                code = DisP.RealCmd[0];
                DisP.codelen++;
                DisP.pro_clone = ix86_660F_PentiumTable[code].pro_clone;
                strcpy(Ret.str,ix86_660F_PentiumTable[code].name);
                if(ix86_660F_PentiumTable[code].method)
                {
                  TabSpace(Ret.str,TAB_POS);
                  ix86_660F_PentiumTable[code].method(Ret.str,&DisP);
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
              /** First check for SSE2 extensions */
              if(DisP.RealCmd[1] == 0x0F && ix86_F20F_PentiumTable[DisP.RealCmd[2]].name)
              {
                /** therefore it non rep prefix, but instruction of SSE2 */
                DisP.RealCmd = &DisP.RealCmd[2];
                code = DisP.RealCmd[0];
                DisP.codelen++;
                DisP.pro_clone = ix86_F20F_PentiumTable[code].pro_clone;
                strcpy(Ret.str,ix86_F20F_PentiumTable[code].name);
                if(ix86_F20F_PentiumTable[code].method)
                {
                  TabSpace(Ret.str,TAB_POS);
                  ix86_F20F_PentiumTable[code].method(Ret.str,&DisP);
                }
                DisP.codelen++;
                goto ExitDisAsm;
              }
              if(has_rep + has_lock) break;
              has_rep++;
              strcat(ix86_da_out,"repne; ");
              goto MakePref;
   case 0xF3:
              /** First check for SSE extensions */
              if(DisP.RealCmd[1] == 0x0F && ix86_F30F_PentiumTable[DisP.RealCmd[2]].name)
              {
                /** therefore it non rep prefix, but instruction of SSE */
                DisP.RealCmd = &DisP.RealCmd[2];
                code = DisP.RealCmd[0];
                DisP.codelen++;
                DisP.pro_clone = ix86_F30F_PentiumTable[code].pro_clone;
                strcpy(Ret.str,ix86_F30F_PentiumTable[code].name);
                if(ix86_F30F_PentiumTable[code].method)
                {
                  TabSpace(Ret.str,TAB_POS);
                  ix86_F30F_PentiumTable[code].method(Ret.str,&DisP);
                }
                DisP.codelen++;
                goto ExitDisAsm;
              }
              else
                if(DisP.RealCmd[1] == 0x90)
                {
                  /* this is pause insns */
                  strcpy(Ret.str,"pause");
                  DisP.codelen++;
                  DisP.pro_clone = IX86_P4;
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
 strcpy(Ret.str,Use32Data ? ix86_table[code].name32 : ix86_table[code].name16);
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
                if(!ud && !ua) Ret.str[4] = 0;
                break;
   /** callx case */
     case 0xE8:
     case 0x9A:
                if(!ud && !ua) Ret.str[5] = Ret.str[6] = ' ';
                break;
   /** pushax, pushfx case */
     case 0x60:
     case 0x9C:
                if(!ud && !ua) Ret.str[5] = 0;
                break;
     default:   break;
 }
 if((DisP.pro_clone & IX86_CPUMASK) < ix86_table[code].pro_clone) DisP.pro_clone = ix86_table[code].pro_clone;
 if((DisP.pro_clone & IX86_FPUMASK) < (ix86_table[code].pro_clone & IX86_FPUMASK) &&
    (DisP.pro_clone & IX86_CPUMASK) == IX86_CPUMASK) DisP.pro_clone = ix86_table[code].pro_clone;
 if((DisP.pro_clone & IX86_MMXMASK) < (ix86_table[code].pro_clone & IX86_MMXMASK) &&
    (DisP.pro_clone & IX86_CPUMASK) == IX86_CPUMASK &&
    (DisP.pro_clone & IX86_FPUMASK) == IX86_FPUMASK ) DisP.pro_clone = ix86_table[code].pro_clone;
 ExitDisAsm:
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
 if(Use32Data || Use32Addr || x86_Bitness == DAB_USE32)
 {
     if((DisP.pro_clone & IX86_CPUMASK) < IX86_CPU386) DisP.pro_clone |= IX86_CPU386;
     if((DisP.pro_clone & IX86_FPUMASK) < IX86_FPU387 &&
        (DisP.pro_clone & IX86_CPUMASK) == IX86_CPUMASK) DisP.pro_clone |= IX86_FPU387;
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
  "80686/PII  ",
  "PIII    ",
  "P4    ",
  " ???  "
};

static const char * FPUNames[] =
{
  " 8087 ",
  "      ",
  "80287 ",
  "80387 ",
  "80487 ",
  "      ",
  "80687      ",
  "PIII    ",
  "P4    ",
  " ???  "
};

static const char * MMXNames[] =
{
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "P1MMX ",
  "K6-2 3dNow ",
  "PIII/K7 ",
  "P4    ",
  " ???  "
};

static ColorAttr  __FASTCALL__ ix86GetAsmColor(unsigned long clone)
{
     if(clone & IX86_MMXMASK) return disasm_cset.cpu_cset[2].clone[(clone >> 16) & 0xFF];
     else
       if((clone & IX86_FPUMASK) || clone == IX86_FPU087) return disasm_cset.cpu_cset[1].clone[(clone >> 8) & 0xFF];
       else                      return disasm_cset.cpu_cset[0].clone[clone & 0xFF];
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
 size = (unsigned)hlpGetItemSize(20001);
 if(!size) goto ix86hlp_bye;
 msgAsmText = PMalloc(size+1);
 if(!msgAsmText)
 {
   mem_off:
   MemOutBox(" Help Display ");
   goto ix86hlp_bye;
 }
 if(!hlpLoadItem(20001,msgAsmText))
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
 do
 {
   evt = GetEvent(drawEmptyPrompt,hwnd);
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
   "~Auto"
};

static tBool __FASTCALL__ x86Select_Bitness( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(use_names)/sizeof(char *);
  if(BITNESS == DAB_AUTO) BITNESS = 2;
  i = SelBoxA(use_names,nModes," Select bitness mode: ",BITNESS);
  if(i != -1)
  {
    if(i == 2) i = DAB_AUTO;
    BITNESS = x86_Bitness = i;
    return True;
  }
  else if(BITNESS == 2) BITNESS = x86_Bitness = DAB_AUTO;
  return False;
}

static int __FASTCALL__ ix86MaxInsnLen( void ) { return MAX_IX86_INSN_LEN; }
static int __FASTCALL__ ix86GetBitness( void ) { return BITNESS; }
static char __FASTCALL__ ix86GetClone(unsigned long clone) { return ix86CloneSNames[(clone >> 24) & 0xFF]; }

extern char *ix86_Katmai_buff;
extern char *ix86_appstr;
extern char *ix86_dtile;
extern char *ix86_appbuffer;
extern char *ix86_apistr;
extern char *ix86_modrm_ret;

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
    if(BITNESS > 1 && BITNESS != DAB_AUTO) BITNESS = 0;
    x86_Bitness = BITNESS;
  }
}

static void __FASTCALL__ ix86WriteIni( hIniProfile *ini )
{
  char tmps[10];
  sprintf(tmps,"%u",BITNESS);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode3",tmps);
}


REGISTRY_DISASM ix86_Disasm =
{
  "Intel ~ix86",
  { "x86Hlp", "Bitnes", NULL, NULL, NULL },
  { x86AsmRef, x86Select_Bitness, NULL, NULL, NULL },
  ix86Disassembler,
  NULL,
  ix86HelpAsm,
  ix86MaxInsnLen,
  ix86GetAsmColor,
  ix86GetBitness,
  ix86GetClone,
  ix86Init,
  ix86Term,
  ix86ReadIni,
  ix86WriteIni
};








