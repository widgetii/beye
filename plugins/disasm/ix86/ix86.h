/**
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/ix86/ix86.h
 * @brief       This file contains declaration of internal Intel x86 disassembler functions.
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
**/
#ifndef ____DISASM_H
#define ____DISASM_H

#define IX86_64 1 /* enable athlon64 disassembler by default */

#include "plugins/disasm.h"

#ifndef __BIEWLIB_H
#include "biewlib/biewlib.h"
#endif

#define TAB_POS 10
#define TILE_SAFE 4000

#define DUMMY_PTR 0 /**< "" */
#define BYTE_PTR  1 /**<" <b>"*/
#define WORD_PTR  2 /**<" <w>"*/
#define DWORD_PTR 3 /**<" <d>"*/
#define PWORD_PTR 4 /**<" <p>"*/
#define QWORD_PTR 5 /**<" <q>"*/
#define TWORD_PTR 6 /**<" <t>"*/

/*
   This struct is ordered as it documented in Athlon manual
   Publication # 22007 Rev: D
*/
typedef struct tagix86Param
{
  unsigned long pro_clone; /**< processor family */
  __filesize_t  DisasmPrefAddr; /**< address of instruction with prefixes */
  __filesize_t  CodeAddress; /**< without prefixes */
  MBuffer       CodeBuffer; /**< buffer with source code */
  MBuffer       RealCmd; /**< buffer without prefixes */
  unsigned      flags; /**< refer to disasm.h header */
  unsigned char codelen;
#define PFX_SEGMASK		0x00000007
#define  PFX_SEG_CS		0x00000000
#define  PFX_SEG_DS		0x00000001
#define  PFX_SEG_ES		0x00000002
#define  PFX_SEG_SS		0x00000003
#define  PFX_SEG_FS		0x00000004
#define  PFX_SEG_GS		0x00000005
#define  PFX_SEG_US		0x00000006
#define  PFX_SEG_XS		0x00000007
#define PFX_LOCK		0x00000008
#define PFX_F2_REPNE		0x00000010
#define PFX_F3_REP		0x00000020
#define PFX_66			0x00000040
#define PFX_67			0x00000080
#define PFX_OF			0x00000100 /* for VEX compatibility */
#define PFX_REX			0x01000000
#define PFX_VEX			0x02000000
  unsigned long pfx;
#define MOD_WIDE_DATA		0x00000001
#define MOD_WIDE_ADDR		0x00000002
#define MOD_MMX			0x00000010
#define MOD_SSE			0x00000020
  unsigned long mode;
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
  unsigned char REX;
/*
  VEX is C4, C5 opcodes
                   Byte 0         Byte 1             Byte 2
  (Bit Position) 7        0   7 6 5 4        0   7 6    3 2 1 0
		+----------+ +-----+----------+ +-+------+-+---+
  3-byte VEX C4 | 11000100 | |R X B|  m-mmmm  | |W| vvvv |L| pp|
                +----------+ +-----+----------+ +-+------+-+---+
                 7        0   7 6   3 2 1 0
		+----------+ +-+-----+-+---+
  2-byte VEX C5 | 11000101 | |R| vvvv|L| pp|
                +----------+ +-+-----+-+---+

   R: REX.R in 1’s complement (inverted) form
      1: Same as REX.R=0 (must be 1 in 32-bit mode)
      0: Same as REX.R=1 (64-bit mode only)
   X: REX.X in 1’s complement (inverted) form
      1: Same as REX.X=0 (must be 1 in 32-bit mode)
      0: Same as REX.X=1 (64-bit mode only)
   B: REX.B in 1’s complement (inverted) form
      1: Same as REX.B=0 (Ignored in 32-bit mode).
      0: Same as REX.B=1 (64-bit mode only)
   W: opcode specific (use like REX.W, or used for memory operand
      select on 4-operand instructions, or ignored, depending on the opcode)
   m-mmmm:
     00000: Reserved for future use (will #UD)
     00001: implied 0F leading opcode byte
     00010: implied 0F 38 leading opcode bytes
     00011: implied 0F 3A leading opcode bytes
     00100-11111: Reserved for future use (will #UD)
   vvvv: a register specifier (in 1’s complement form) or 1111 if unused.			   					      
   L: Vector Length
        0: scalar or 128-bit vector
        1: 256-bit vector
   pp: opcode extension providing equivalent functionality of a SIMD prefix
        00: None
        01: 66
        10: F3
        11: F2
*/
  unsigned char VEX_m;
  unsigned char VEX_vlp;
}ix86Param;

extern tBool Use64;
extern char * SJump[];
typedef void (__FASTCALL__*ix86_method)(char *encode_str,ix86Param *);

#define IX86_CPU086	0x00000000UL
#define IX86_CPU186	0x00000001UL
#define IX86_CPU286	0x00000002UL
#define IX86_CPU386	0x00000003UL
#define IX86_CPU486	0x00000004UL
#define IX86_CPU586	0x00000005UL
#define IX86_CPU686	0x00000006UL
#define IX86_CPU786	0x00000007UL
#define IX86_CPU886	0x00000008UL
#define IX86_CPU986	0x00000009UL
#define IX86_CPU1086	0x0000000AUL
#define IX86_CPU1186	0x0000000BUL
#define IX86_CPU1286	0x0000000CUL
#define IX86_CPUMASK	0x00000FFFUL

#define IX86_REGGROUP	0x0000F000UL
#define IX86_GPR	0x00000000UL /* insn works with general purpose registers */
#define IX86_FPU	0x00001000UL /* insn works with fpu registers */
#define IX86_MMX	0x00002000UL /* insn works with mmx registers */
#define IX86_SSE	0x00003000UL /* insn works with sse registers */

#define IX86_CLONEMASK	0x000F0000UL
#define IX86_INTEL	0x00000000UL
#define IX86_AMD	0x00010000UL
#define IX86_CYRIX	0x00020000UL

#define IX86_SYSTEMMASK	0x00F00000UL
#define IX86_CPL0	0x00100000UL

/* Furter processors */
#define IX86_UNKCPU	IX86_CPU1286
#define IX86_UNKFPU	(IX86_UNKCPU|IX86_FPU)
#define IX86_UNKMMX	(IX86_UNKCPU|IX86_MMX)
#define IX86_UNKSSE	(IX86_UNKCPU|IX86_SSE)

#define IX86_P2		IX86_CPU686
#define IX86_P3		IX86_CPU786
#define IX86_P4		IX86_CPU886
/* Prescott processor (SSE3) */
#define IX86_P5		IX86_CPU986
/* Xeon5100 processor (SSSE3)*/
#define IX86_P6		IX86_CPU1086
/* Xeon5200 processor (SSE4)*/
#define IX86_P7		IX86_CPU1186

#define IX86_K6		(IX86_AMD|IX86_CPU586)
#define IX86_3DNOW	(IX86_AMD|IX86_CPU686|IX86_MMX)
#define IX86_ATHLON	(IX86_AMD|IX86_CPU786|IX86_MMX)
#define IX86_UNKAMD	(IX86_AMD|IX86_CPU886|IX86_MMX)

#define IX86_CYRIX486		(IX86_CYRIX|IX86_CPU486)
#define IX86_CYRIX686		(IX86_CYRIX|IX86_CPU586)
#define IX86_CYRIX686MMX	(IX86_CYRIX|IX86_CPU586|IX86_MMX)
#define IX86_UNKCYRIX		(IX86_CYRIX|IX86_CPU686)

#define K64_ATHLON	0x00000000UL
#define K64_FAM9	0x00000001UL
#define K64_FAM10	0x00000002UL
#define K64_FAM11	0x00000003UL
#define K64_CLONEMASK	0x00000FFFUL
#define K64_REGGROUP    0x0000F000UL
#define K64_GPR         0x00000000UL /* insn works with general purpose registers */
#define K64_FPU         0x00001000UL /* insn works with fpu registers */
#define K64_MMX         0x00002000UL /* insn works with mmx registers */
#define K64_SSE         0x00003000UL /* insn works with sse registers */
#define K64_NOCOMPAT	0x00004000UL /* means insns has no 16 or 32 bit forms */
#define K64_DEF32	0x00008000UL /* means insns size depends on default data size but not address size */
#define K64_SYSTEMMASK	0x000F0000UL
#define K64_CPL0	0x00010000UL /* means insns requires cpl0 privilegies to be executed */

/* Special features flags */
#define TABDESC_MASK		0xFF000000UL
#define TAB_NAME_IS_TABLE	0x01000000UL

typedef struct tag_ix86opcodes
{
  const char *  name16;
  const char *  name32;
#ifdef IX86_64
  const char *  name64;
#endif
  ix86_method   method;
  unsigned long pro_clone;
#ifdef IX86_64
  ix86_method   method64;
  unsigned long flags64;
#endif
}ix86_Opcodes;

typedef struct tag_ix86ExOpcodes
{
  const char *  name;
#ifdef IX86_64
  const char *  name64;
#endif
  ix86_method   method;
#ifdef IX86_64
  ix86_method   method64;
  unsigned long flags64;
#endif
  unsigned long pro_clone;
}ix86_ExOpcodes;

typedef struct tag_ix3dNowopcodes
{
  const char *  name;
  unsigned long pro_clone;
}ix86_3dNowopcodes;

extern unsigned x86_Bitness;

extern const ix86_Opcodes ix86_table[];
extern const ix86_ExOpcodes ix86_extable[];
extern const ix86_3dNowopcodes ix86_3dNowtable[];

extern char ix86_segpref[];
extern const char * ix86_sizes[];

extern const char * ix86_A16[];

extern const char * ix86_ByteRegs[];
extern const char * ix86_WordRegs[];
extern const char * ix86_DWordRegs[];
extern const char * ix86_MMXRegs[];
extern const char * ix86_XMMXRegs[];
#ifdef IX86_64
extern const char * k86_ByteRegs[];
extern const char * k86_WordRegs[];
extern const char * k86_DWordRegs[];
extern const char * k86_QWordRegs[];
extern const char * k86_XMMXRegs[];
extern const char * k86_CrxRegs[];
extern const char * k86_DrxRegs[];
extern const char * k86_TrxRegs[];
extern const char * k86_XrxRegs[];
#endif
extern const char * ix86_SegRegs[];
extern const char * ix86_CrxRegs[];
extern const char * ix86_DrxRegs[];
extern const char * ix86_TrxRegs[];
extern const char * ix86_XrxRegs[];

extern const char * ix86_Op1Names[];
extern const char * ix86_ShNames[];
extern const char * ix86_Gr1Names[];
extern const char * ix86_Gr2Names[];

extern const char * ix86_ExGrp0[];
extern const char * ix86_ExGrp1[];
extern const char * ix86_BitGrpNames[];

extern const char * ix86_MMXGr1[];
extern const char * ix86_MMXGr2[];
extern const char * ix86_MMXGr3[];
extern const char * ix86_XMMXGr1[];
extern const char * ix86_XMMXGr2[];
extern const char * ix86_XMMXGr3[];

extern const char * ix86_3dPrefetchGrp[];

extern const char * ix86_KatmaiGr1Names[];
extern const char * ix86_KatmaiGr2Names[];
extern const char * ix86_KatmaiCmpSuffixes[];

extern const ix86_ExOpcodes* __FASTCALL__ ix86_prepare_flags(const ix86_ExOpcodes *extable,ix86Param *DisP,unsigned char *code);
extern char * __FASTCALL__ ix86_getModRM(tBool w,unsigned char mod,unsigned char rm,ix86Param *DisP);
extern void   __FASTCALL__ ix86_setModifier(char *str,const char *modf);
extern char * __FASTCALL__ ix86_CStile(char *str,const char *arg2);

extern void   __FASTCALL__ ix86_ArgES(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgDS(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgSS(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgCS(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgFS(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgGS(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgByte(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgWord(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgWord_Byte(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgDWord(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgShort(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgNear(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgFar(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgModRM(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgModRMDW(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgModRMnDW(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgModRMDnW(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgModRMnDnW(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMod(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgModB(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_SMov(char * str,ix86Param *DisP);
extern void   __FASTCALL__ ix86_ArgSegRM(char * str,ix86Param *DisP);
extern void   __FASTCALL__ ix86_ArgAXMem(char *str,ix86Param *DisP);
extern void   __FASTCALL__ ix86_ArgAXDigit(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgRMDigit(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgIRegDigit(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgIReg(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgIReg64(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgAXIReg(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgSInt(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgInt(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgRegRMDigit(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgOp1(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgOp2(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ShOp2(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ShOp1(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ShOpCL(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgGrp1(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgGrp2(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_InOut(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_FPUCmd(char *str,ix86Param *);

extern void   __FASTCALL__ ix86_ExOpCodes(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_3dNowOpCodes(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_3dNowPrefetchGrp(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_BitGrp(char *str,ix86Param *);

extern void   __FASTCALL__ ix86_ArgExGr0(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgExGr1(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMovXRY(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_DblShift(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMMXD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMMXnD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMMXRD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMMXRnD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMMXGr1(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMMXGr2(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMMXGr3(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMXGr1(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMXGr2(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMXGr3(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMMRMDigit(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgRMMXRevD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgRMMXRevnD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMXD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMXnD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMXRD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMXRnD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgRXMMXD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgRXMMXnD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgRXMMXRevD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgRXMMXRevnD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMXMMD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMXMMnD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMMXMMXD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMMXMMXnD(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgKatmaiGrp1(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgKatmaiGrp2(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMRMDigit(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMCmp(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgRegXMMDigit(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgRegXMMXDigit(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMRegDigit(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMXRegDigit(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMovYX(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_VMX(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_0FVMX(char *str,ix86Param *DisP);

extern void   __FASTCALL__ ix86_ArgXMM1IReg(char *str,ix86Param *DisP);
extern void   __FASTCALL__ ix86_ArgXMM1DigDig(char *str,ix86Param *DisP);
extern void   __FASTCALL__ ix86_ArgXMM1RegDigDig(char *str,ix86Param *DisP);

#endif


