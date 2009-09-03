/**
 * @namespace   biew_plugins_I
 * @file        plugins/disasm.h
 * @brief       This file contains function prototypes for disassembler interface.
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
#ifndef __DISASM_H
#define __DISASM_H

#ifndef __COLORSET__H
#include "colorset.h"
#endif

#ifndef __TWIN_H
#include "biewlib/twin.h"
#endif

#ifndef __FILE_INI_RUNTIME_SUPPORT_SYSTEM__
#include "biewlib/file_ini.h"
#endif

/** List of CPU platform. */

#define DISASM_DATA		0  /**< indicates data disassembler */
#define DISASM_CPU_IX86		1  /**< indicates Intel-x86 disassembler */
#define DISASM_CPU_AVR		2  /**< indicates Atmel-AVR disassembler */
#define DISASM_JAVA		3  /**< indicates Java disassembler */
#define DISASM_CPU_ARM		4  /**< indicates ARM disassembler */
#define DISASM_CPU_PPC		5  /**< indicates PowerPC disassembler */
                                    /* ... here may placed other constants!!! ... */
#define DISASM_CPU_IA64		6  /**< indicates Itanium disassembler */
#define DISASM_CPU_ALPHA	7  /**< indicates DEC Alpha disassembler */
#define DISASM_CPU_MIPS		8  /**< indicates MIPS disassembler */
#define DISASM_CPU_SPARC	9  /**< indicates SUN Sparc disassembler */
#define DISASM_CPU_SH		10 /**< indicates Hitachi SH disassembler */
#define DISASM_CPU_CRAY		11 /**< indicates Cray disassembler */
                                    /* ... here may placed other constants!!! ... */
#define DISASM_DEFAULT		0  /**< indicates unspecified disassembler: format default */


typedef tBool (__FASTCALL__ *DisasmAction)( void );
typedef unsigned char * MBuffer;

/*
   This struct is ordered as it documented in Athlon manual
   Publication # 22007 Rev: D
*/
typedef struct tagDisasmRet
{
  unsigned long pro_clone; /**< contains processor clone when normal disassembling; instruction type on __DISF_GETTYPE */
  char         *str;       /**< contains disassembler output */
  unsigned long field;     /**< on __DISF_GETTYPE contains offset to field for binding from begin of insn, if present. */
  unsigned char codelen;   /**< contains length of instruction when normal disassembling; length of field for binding on __DISF_GETTYPE */
}DisasmRet;

#define ASM_NOERR  0
#define ASM_SYNTAX 1

/*
   This struct is ordered as it documented in Athlon manual
   Publication # 22007 Rev: D
*/
typedef struct tagAsmRet
{
  MBuffer       insn;
  int           err_code;
  unsigned char insn_len;
}AsmRet;

/* New features: commentaries */
#define DISCOM_SIZE 256       /**< Size of disassembler commentaries */
extern char *   dis_comments; /**< Pointer to disassembler commentaries */

#define DISCOMSEV_NONE    0 /**< means no comments were appended */
#define DISCOMSEV_STRING  1 /**< means comment is representation of multibyte immediate operand */
#define DISCOMSEV_INSNREF 2 /**< means comment is instruction reference (like ; RETURN) */
#define DISCOMSEV_STRPTR  3 /**< means comment is pointer to the string */
#define DISCOMSEV_FUNC    4 /**< means comment is function name */
extern unsigned dis_severity; /**< severity of disassembler commentaries */


#define PREDICT_DEPTH 15 /**< means depth of prediction is 15 insns which have max_insn_len */

/** Flags of disassembler */
#define __DISF_NORMAL   0x0000 /**< Performs normal disassembling */
#define __DISF_GETTYPE  0x0001 /**< Tells to disassembler that field pro_clone must to contain type of instruction */
#define __DISF_SIZEONLY 0x8000 /**< Performs computing size of insns */

/** Types of instruction (for __DISF_GETTYPE) in future must grow */
#define __INSNT_ORDINAL 0x00000000L /**< Any unspecified instruction */
#define __INSNT_RET     0x00000001L /**< Instruction of return class */
#define __INSNT_LEAVE   0x00000002L /**< Instruction of leave class: Example: pop reg1; pop reg2; mov esp, ebp; pop ebp; retx or similar */
#define __INSNT_JMPVVT  0x00000003L /**< Jump via virtual table */
#define __INSNT_JMPPIC  0x00000004L /**< Jump via PIC. Like: .i386: jmp name@GOT(ebx) */
#define __INSNT_JMPRIP  0x00000005L /**< Jump via RIP. Like: .i386: jmp [rip+name@GOT(rip)] */

typedef DisasmRet (__FASTCALL__ *DisasmFunc)(__filesize_t shift,
                                             MBuffer insn_buff,
                                             unsigned flags);
typedef AsmRet    (__FASTCALL__ *AsmFunc)(const char *str);

typedef struct tag_REGISTRY_DISASM
{
  unsigned     type;		/**< DISASM_XXX constant */
  const char * name;		/**< disassembler name */
  const char * prompt[4];	/**< prompt on Ctrl-(F1,F3-F5) */
  DisasmAction action[4];	/**< actions on Ctrl-(F1,F3-F5) */
  DisasmFunc   disasm;		/**< main function of disasm */
  AsmFunc      asm_f;		/**< assembler (vice versa of disasm) */
  void         (__FASTCALL__ *ShowShortHelp)(void); /**< displays short help */
  int          (__FASTCALL__ *max_insn_len)(void); /**< Max length of 1 disasm instruction */
  ColorAttr    (__FASTCALL__ *GetInsnColor)(unsigned long clone); /**< returns color of instruction */
  ColorAttr    (__FASTCALL__ *GetOpcodeColor)(unsigned long clone); /**< returns color of instruction */
  ColorAttr    (__FASTCALL__ *altGetInsnColor)(unsigned long clone); /**< returns color of instruction in alternative mode */
  ColorAttr    (__FASTCALL__ *altGetOpcodeColor)(unsigned long clone); /**< returns color of instruction in alternative mode */
  int          (__FASTCALL__ *GetDefBitness)(void);               /**< returns currently used bitness */
  char         (__FASTCALL__ *CloneShortName)(unsigned long clone); /**< returns short clone name of instruction */
  void         (__FASTCALL__ *init)(void);     /**< initializing of plugin */
  void         (__FASTCALL__ *term)(void);     /**< terminating of plugin */
  void         (__FASTCALL__ *read_ini)(hIniProfile *);  /**< reads settings of plugin from .ini file */
  void         (__FASTCALL__ *save_ini)(hIniProfile *);  /**< stores settings of plugin into .ini file */
}REGISTRY_DISASM;

extern REGISTRY_DISASM *activeDisasm; /**< currently selected active disassembler */
#define PANMOD_FULL   2       /**< full mode of panel: address + instruction bytes + instruction */
#define PANMOD_MEDIUM 1       /**< medium mode of panel: address + instruction */
#define PANMOD_WIDE   0       /**< full mode of panel: instruction only */
extern unsigned disPanelMode; /**< contains select mode of panel */

/* references resolution */
#define NEEDREF_PREDICT 3    /**< resolves all instructions and uses prediction mechanism */
#define NEEDREF_ALL     2    /**< resolves all instructions that has binding */
#define NEEDREF_JMP     1    /**< resolves only jump and call instructions */
#define NEEDREF_NONE    0    /**< do not resolve references */
extern unsigned disNeedRef;  /**< contains selected references resolution */

extern DisasmRet Disassembler(__filesize_t ulShift,MBuffer buffer,unsigned flags);

/** Common disassembler utility */

extern char * __FASTCALL__ TabSpace(char * str,unsigned nSpace);
extern void   __FASTCALL__ disSetModifier(char *str,const char *modf);

#define DISARG_LLONG    0x0080U /**< signed 8-byte value */
#define DISARG_LONG     0x0040U /**< signed 4-byte value */
#define DISARG_SHORT    0x0020U /**< signed 2-byte value */
#define DISARG_CHAR     0x0010U /**< signed 1-byte value */
#define DISARG_BYTE     0x0001U /**< unsigned 1-byte value */
#define DISARG_WORD     0x0002U /**< unsigned 2-byte value */
#define DISARG_DWORD    0x0004U /**< unsigned 4-byte value */
#define DISARG_QWORD    0x0008U /**< unsigned 8-byte value */
/* Contsants for references predictions */
#define DISARG_IMM      0x1000U /**< Immediate argument */
#define DISARG_DISP     0x2000U /**< Argument is displacement only */
#define DISARG_IDXDISP  0x4000U /**< Argument is displacement which is combined with registers */
#define DISARG_RIP      0x8000U /**< Argument is displacement relatively current Instruction Pointer */
/** Appends symbolic information instead digits to instruction string
    @param str       string to be appended
    @param flags     same as described in reg_form.h (APREF_* family)
    @param ulShift   indicates offset to field for binding
    @param codelen   contains length of field for binding
    @param defval    contains default value if not binding
    @param type      see above (DISARG_LONG - DISARG_DWORD family)
    @return          see RAPREF_* constants in biewutil.h for detail
    @remark
    Examples:
    *** 1 ***
    00005678: B8 34 12       mov   ax, 1234
    after references resolving it can will:
                             mov   ax, off16 KERNEL.@4
    i.e. i must call this function as:
    strcpy(outstr,"mov ax,");
    disAppendDigits(outstr, 0x5679, 1, 2, 0x1234, DISARG_WORD);
    *** 2 ***
    00005678: 8B 80 34 12       mov   ax, [bx+si+1234]
    after references resolving it can will:
                                mov   ax, [bx+si+off16 KERNEL.@4]
    i.e. i must call this function as:
    strcpy(outstr,"mov ax,[bx+si+");
    disAppendDigits(outstr, 0x5680, 1, 2, 0x1234, DISARG_WORD);
    strcat(outstr,"]");
**/
extern int __FASTCALL__  disAppendDigits(char *str,__filesize_t ulShift,int flags,
                              char codelen,void *defval,unsigned type);

#define DISADR_SHORT   0x00
#define DISADR_NEAR16  0x01
#define DISADR_NEAR32  0x02
#define DISADR_NEAR64  0x04
#define DISADR_USESEG  0x10
#define DISADR_FAR16   (DISADR_USESEG | DISADR_NEAR16)
#define DISADR_FAR32   (DISADR_USESEG | DISADR_NEAR32)
/** Appends symbolic information to address field of jump instructions
    @param str       string to be appended
    @param ulShift   indicates offset to field for binding
    @param codelen   contains length of field for binding
    @param distin    contains original value of field (like defval)
    @param r_sh      real shift to begin of insn (for pass to CodeGuider as return addr)
    @param type      see above (DISARG_SHORT - DISARG_FAR32 family)
    @param seg       contains segment value (optional)
    @return          see RAPREF_* constants in biewutil.h for detail
    @remark
    Examples:
    *** 1 ***
    00001002: 9A 78 56 34 12       callf 1234:5678
    after references resolving it may:
                                   callf KERNEL.@90
    i.e. i must call this function as:
    strcpy(outstr,"callf ");
    disAppendFAddr(outstr, 0x1003, 0x5678, 0x1002, DISADR_FAR16, 0x1234, 4);
    *** 2 ***
    00001002: 66 E9 78 56 34 12       calln32 12345678
    after references resolving it may:
                                      calln32 KERNEL32.TerminateApp
    i.e. i must call this function as:
    strcpy(outstr,"calln32 ");
    disAppendFAddr(outstr, 0x1004, 0x12345678, 0x1002, DISADR_NEAR32, 0, 4);
**/
extern int __FASTCALL__  disAppendFAddr(char * str,__fileoff_t ulShift,__fileoff_t distin,
                             __filesize_t r_sh,char type,
                             unsigned seg,char codelen);
#endif
