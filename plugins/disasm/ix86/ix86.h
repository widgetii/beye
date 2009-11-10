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
  unsigned long insn_flags; /**< contains copy of flags32/flags64 field from INSN_TABLE */
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
#define PFX_XOP			0x04000000
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

   R: REX.R in 1's complement (inverted) form
      1: Same as REX.R=0 (must be 1 in 32-bit mode)
      0: Same as REX.R=1 (64-bit mode only)
   X: REX.X in 1's complement (inverted) form
      1: Same as REX.X=0 (must be 1 in 32-bit mode)
      0: Same as REX.X=1 (64-bit mode only)
   B: REX.B in 1's complement (inverted) form
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
   vvvv: a register specifier (in 1's complement form) or 1111 if unused.			   					      
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
  unsigned char XOP_m;
}ix86Param;

#define K86_REX (DisP->REX)
#define REX_W(rex) (((rex)&0x08)>>3)
#define REX_R(rex) (((rex)&0x04)>>2)
#define REX_X(rex) (((rex)&0x02)>>1)
#define REX_B(rex) ((rex)&0x01)
#define REX_w(rex) ((rex)&0x08)
#define REX_r(rex) ((rex)&0x04)
#define REX_x(rex) ((rex)&0x02)
#define REX_b(rex) ((rex)&0x01)

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
#define IX86_CPUMASK	0x000000FFUL

#define IX86_P2		IX86_CPU686
#define IX86_P3		IX86_CPU786
#define IX86_P4		IX86_CPU886
/* Prescott processor (SSE3) */
#define IX86_P5		IX86_CPU986
/* Xeon5100 processor (SSSE3)*/
#define IX86_P6		IX86_CPU1086
/* Xeon5200 processor (SSE4)*/
#define IX86_P7		IX86_CPU1186
#define IX86_P8		IX86_CPU1286

#define K64_ATHLON	0x00000000UL
#define K64_FAM9	0x00000001UL
#define K64_FAM10	0x00000002UL
#define K64_FAM11	0x00000003UL
#define K64_CLONEMASK	0x000000FFUL

#define IX86_CLONEMASK	0x00000700UL
#define IX86_INTEL	0x00000000UL
#define IX86_AMD	0x00000100UL
#define IX86_CYRIX	0x00000200UL
#define IX86_VIA	0x00000400UL

#define INSN_SYSTEMMASK	0x00000800UL
#define INSN_CPL0	0x00000800UL

#define INSN_REGGROUP	0x0000F000UL
#define INSN_GPR	0x00000000UL /* insn works with general purpose registers */
#define INSN_FPU	0x00001000UL /* insn works with fpu registers */
#define INSN_MMX	0x00002000UL /* insn works with mmx registers */
#define INSN_SSE	0x00004000UL /* insn works with sse registers */
#define INSN_AVX	0x0000C000UL /* insn works with avx registers */

#define INSN_VEXMASK		0x000F0000UL
#define INSN_VEX_V		0x00010000UL /* means insns use VVVV register extension from VEX prefix*/
#define INSN_VEXW_AS_SWAP	0x00020000UL /* means use VEX.W register to swap sources */

#define INSN_FLAGS_MASK	0xFFF00000UL
#define INSN_LOAD	0x00000000UL /* means direction: OPCODE reg,[mem] */
#define INSN_STORE	0x00100000UL /* means direction: OPCODE [mem],reg */
#define INSN_OP_BYTE	0x00200000UL /* means operand size is 1 byte */
#define INSN_OP_WORD	0x00000000UL /* means operand size is word (16,32 or 64) depends on mode */
#define K64_NOCOMPAT	0x01000000UL /* means insns has no 16 or 32 bit forms */
#define K64_DEF32	0x02000000UL /* means insns size depends on default data size but not address size */
#define INSN_USERBIT	0x40000000UL /* overloaded for special purposes */

#define BRIDGE_MMX_SSE	INSN_USERBIT
#define BRIDGE_SSE_MMX	0x00000000UL
#define BRIDGE_CPU_SSE	INSN_USERBIT
#define BRIDGE_SSE_CPU	0x00000000UL
#define IMM_BYTE	INSN_USERBIT
#define IMM_WORD	0x00000000UL
#define K64_FORCE64	INSN_USERBIT

/* Special features flags */
#define TABDESC_MASK		0x80000000UL
#define TAB_NAME_IS_TABLE	0x80000000UL

/* Furter processors */
#define IX86_UNKCPU	IX86_CPU1286
#define IX86_UNKFPU	(IX86_UNKCPU|INSN_FPU)
#define IX86_UNKMMX	(IX86_UNKCPU|INSN_MMX)
#define IX86_UNKSSE	(IX86_UNKCPU|INSN_SSE)
#define IX86_UNKAVX	(IX86_UNKCPU|INSN_AVX)

#define IX86_K6		(IX86_AMD|IX86_CPU586)
#define IX86_3DNOW	(IX86_AMD|IX86_CPU686|INSN_MMX)
#define IX86_ATHLON	(IX86_AMD|IX86_CPU786|INSN_MMX)
#define IX86_UNKAMD	(IX86_AMD|IX86_CPU886|INSN_MMX)

#define IX86_CYRIX486		(IX86_CYRIX|IX86_CPU486)
#define IX86_CYRIX686		(IX86_CYRIX|IX86_CPU586)
#define IX86_CYRIX686MMX	(IX86_CYRIX|IX86_CPU586|INSN_MMX)
#define IX86_UNKCYRIX		(IX86_CYRIX|IX86_CPU686)

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
extern const char * i8086_ByteRegs[];
extern const char * k64_ByteRegs[];

extern const char * ix86_MMXRegs[];

extern const char * k64_WordRegs[];
extern const char * k64_DWordRegs[];
extern const char * k64_QWordRegs[];
extern const char * k64_XMMXRegs[];
extern const char * k64_YMMXRegs[];
extern const char * k64_CrxRegs[];
extern const char * k64_DrxRegs[];
extern const char * k64_TrxRegs[];
extern const char * k64_XrxRegs[];

extern const char * ix86_SegRegs[];

extern const char * ix86_Op1Names[];
extern const char * ix86_ShNames[];
extern const char * ix86_Gr1Names[];
extern const char * ix86_Gr2Names[];

extern const char * ix86_ExGrp0[];
extern const char * ix86_BitGrpNames[];

extern const char * ix86_MMXGr1[];
extern const char * ix86_MMXGr2[];
extern const char * ix86_MMXGr3[];
extern const char * ix86_XMMXGr1[];
extern const char * ix86_XMMXGr2[];
extern const char * ix86_XMMXGr3[];

extern const char * ix86_3dPrefetchGrp[];

extern const char * ix86_KatmaiGr2Names[];
extern const char * ix86_KatmaiCmpSuffixes[];

extern const ix86_ExOpcodes* __FASTCALL__ ix86_prepare_flags(const ix86_ExOpcodes *extable,ix86Param *DisP,unsigned char *code,unsigned char *codelen);
extern char * __FASTCALL__ ix86_getModRM(tBool w,unsigned char mod,unsigned char rm,ix86Param *DisP);
extern void   __FASTCALL__ ix86_setModifier(char *str,const char *modf);
extern char * __FASTCALL__ ix86_CStile(char *str,const char *arg2);

/* methods */

extern void   __FASTCALL__ ix86_ArgES(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgDS(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgSS(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgCS(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgFS(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgGS(char *str,ix86Param *);

extern void   __FASTCALL__ arg_cpu_modregrm(char * str,ix86Param *DisP);
extern void   __FASTCALL__ arg_cpu_modREGrm(char * str,ix86Param *DisP); /* CRC32 */
extern void   __FASTCALL__ arg_cpu_mod_rm(char* str,ix86Param *DisP);
extern void   __FASTCALL__ arg_cpu_mod_rm_imm(char *str,ix86Param *DisP);
extern void   __FASTCALL__ arg_cpu_modregrm_imm(char *str,ix86Param *DisP);
extern void   __FASTCALL__ arg_offset(char *str,ix86Param *);
extern void   __FASTCALL__ arg_segoff(char *str,ix86Param *);
extern void   __FASTCALL__ arg_insnreg(char *str,ix86Param *); /* reg is part of insn */
extern void   __FASTCALL__ arg_insnreg_imm(char *str,ix86Param *);
extern void   __FASTCALL__ arg_cpu_modsegrm(char * str,ix86Param *DisP);
extern void   __FASTCALL__ arg_r0_imm(char *str,ix86Param *);
extern void   __FASTCALL__ arg_r0rm(char *str,ix86Param *);
extern void   __FASTCALL__ arg_r0mem(char *str,ix86Param *DisP);
extern void   __FASTCALL__ arg_imm(char *str,ix86Param *);
extern void   __FASTCALL__ arg_imm8(char *str,ix86Param *);
extern void   __FASTCALL__ arg_imm16(char *str,ix86Param *);
extern void   __FASTCALL__ arg_imm16_imm8(char *str,ix86Param *);

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

extern void   __FASTCALL__ arg_emms(char *str,ix86Param *);
extern void   __FASTCALL__ arg_simd(char *str,ix86Param *);
extern void   __FASTCALL__ arg_simd_imm8(char *str,ix86Param *);
extern void   __FASTCALL__ arg_simd_xmm0(char *str,ix86Param *);
extern void   __FASTCALL__ arg_simd_regrm(char *str,ix86Param *);
extern void   __FASTCALL__ arg_simd_regrm_imm8_imm8(char *str,ix86Param *);
extern void   __FASTCALL__ arg_simd_rm_imm8_imm8(char *str,ix86Param *);
extern void   __FASTCALL__ bridge_sse_mmx(char *str,ix86Param* DisP);
extern void   __FASTCALL__ bridge_simd_cpu(char *str,ix86Param* DisP);
extern void   __FASTCALL__ bridge_simd_cpu_imm8(char *str,ix86Param* DisP);
extern void   __FASTCALL__ arg_fma(char *str,ix86Param *);
extern void   __FASTCALL__ arg_fma4(char *str,ix86Param *);
extern void   __FASTCALL__ arg_fma4_imm8(char *str,ix86Param *DisP);

extern void   __FASTCALL__ ix86_ArgMMXGr1(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMMXGr2(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMMXGr3(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMXGr1(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMXGr2(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMXGr3(char *str,ix86Param *);

extern void   __FASTCALL__ ix86_ArgKatmaiGrp1(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgKatmaiGrp2(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgXMMCmp(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_ArgMovYX(char *str,ix86Param *);

extern void   __FASTCALL__ ix86_VMX(char *str,ix86Param *);
extern void   __FASTCALL__ ix86_0FVMX(char *str,ix86Param *DisP);
extern void   __FASTCALL__ ix86_660FVMX(char *str,ix86Param *DisP);

#endif


