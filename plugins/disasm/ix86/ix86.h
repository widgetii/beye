/**
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/ix86/ix86.h
 * @brief       This file contains declaration of internal Intel x86 disassembler functions.
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
#ifndef ____DISASM_H
#define ____DISASM_H

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
  unsigned long DisasmPrefAddr; /**< address of instruction with prefixes */
  unsigned long CodeAddress; /**< without prefixes */
  MBuffer       CodeBuffer; /**< buffer with source code */
  MBuffer       RealCmd; /**< buffer without prefixes */
  unsigned      flags; /**< refer to disasm.h header */
  unsigned char codelen;
}ix86Param;

extern tBool Use32Addr,Use32Data,UseMMXSet,UseXMMXSet,Use64;
extern char * SJump[];
typedef void (__FASTCALL__*ix86_method)(char *encode_str,ix86Param *);

#define IX86_CPU086  0x00000000UL
#define IX86_CPU186  0x00000001UL
#define IX86_CPU286  0x00000002UL
#define IX86_CPU386  0x00000003UL
#define IX86_CPU486  0x00000004UL
#define IX86_CPU586  0x00000005UL
#define IX86_CPU686  0x00000006UL
#define IX86_CPU786  0x00000007UL
#define IX86_CPU886  0x00000008UL
#define IX86_CPU986  0x00000008UL

#define IX86_CPUMASK 0x000000FFUL

#define IX86_FPU087 0x000000FFUL
#define IX86_FPU187 0x000001FFUL
#define IX86_FPU287 0x000002FFUL
#define IX86_FPU387 0x000003FFUL
#define IX86_FPU487 0x000004FFUL
#define IX86_FPU587 0x000005FFUL
#define IX86_FPU687 0x000006FFUL
#define IX86_FPU787 0x000007FFUL
#define IX86_FPU887 0x000008FFUL
#define IX86_FPU987 0x000008FFUL

#define IX86_FPUMASK 0x0000FF00UL

#define IX86_MMX586 0x0005FFFFUL
#define IX86_MMX686 0x0006FFFFUL
#define IX86_MMX786 0x0007FFFFUL
#define IX86_MMX886 0x0008FFFFUL
#define IX86_MMX986 0x0008FFFFUL

#define IX86_MMXMASK 0x00FF0000UL

#define IX86_INTEL     0x00000000UL
#define IX86_AMD       0x01000000UL
#define IX86_CYRIX     0x02000000UL
#define IX86_CLONEMASK 0xFF000000UL

#define IX86_P2        IX86_CPU686

#define IX86_P3        IX86_CPU786
#define IX86_P3FPU     IX86_FPU787
#define IX86_P3MMX     IX86_MMX786

#define IX86_P4        IX86_CPU886
#define IX86_P4FPU     IX86_FPU887
#define IX86_P4MMX     IX86_MMX886

/* Futured processors */
#define IX86_UNKCPU    IX86_CPU986
#define IX86_UNKFPU    IX86_FPU987
#define IX86_UNKMMX    IX86_MMX986

#define IX86_K6        IX86_AMD | IX86_CPU586
#define IX86_3DNOW     IX86_AMD | IX86_MMX686
#define IX86_ATHLON    IX86_AMD | IX86_MMX786
#define IX86_UNKAMD    IX86_AMD | IX86_CPU886

#define IX86_CYRIX486  IX86_CYRIX | IX86_CPU486
#define IX86_CYRIX686  IX86_CYRIX | IX86_CPU586
#define IX86_CYRIX686MMX IX86_CYRIX | IX86_MMX586
#define IX86_UNKCYRIX  IX86_CYRIX | IX86_CPU686

#define K64_ATHLON	0x00000000UL
#define K64_CLONEMASK	0x00FFFFFFUL
#define K64_NOCOMPAT	0x01000000UL /* means insns has no 16 or 32 bit forms */
#define K64_DEF32	0x02000000UL /* means insns size depends on default data size but not address size */

typedef struct tag_ix86opcodes
{
  const char *  name16;
  const char *  name32;
#ifdef INT64_C
  const char *  name64;
#endif
  ix86_method   method;
  unsigned long pro_clone;
#ifdef INT64_C
  ix86_method   method64;
  unsigned long flags64;
#endif
}ix86_Opcodes;

typedef struct tag_ix86ExOpcodes
{
  const char *  name;
#ifdef INT64_C
  const char *  name64;
#endif
  ix86_method   method;
#ifdef INT64_C
  ix86_method   method64;
  unsigned long flags64;
#endif
  unsigned long pro_clone;
}ix86_ExOpcodes;

typedef struct tag_ix86MMOpcodes
{
  const char *  name;
  ix86_method   method;
  unsigned long pro_clone;
}ix86_MMOpcodes;

typedef struct tag_ix3dNowopcodes
{
  const char *  name;
  unsigned long pro_clone;
}ix86_3dNowopcodes;

extern unsigned x86_Bitness;

extern ix86_Opcodes ix86_table[];
extern ix86_ExOpcodes ix86_extable[];
extern ix86_3dNowopcodes ix86_3dNowtable[];

extern char ix86_segpref[];
extern const char * ix86_sizes[];

extern const char * ix86_A16[];

extern const char * ix86_ByteRegs[];
extern const char * ix86_WordRegs[];
extern const char * ix86_DWordRegs[];
extern const char * ix86_MMXRegs[];
extern const char * ix86_XMMXRegs[];
#ifdef INT64_C
extern const char * k86_ByteRegs[];
extern const char * k86_WordRegs[];
extern const char * k86_DWordRegs[];
extern const char * k86_QWordRegs[];
extern const char * k86_XMMXRegs[];
extern unsigned char k86_REX;
extern int has_REX;
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

#endif


