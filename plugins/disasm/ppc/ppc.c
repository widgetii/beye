/**
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/ppc.c
 * @brief       This file contains implementation of PowerPC disassembler.
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "bswap.h"

#include "reg_form.h"
#include "plugins/disasm.h"
#include "bconsole.h"
#include "biewhelp.h"
#include "biewutil.h"
#include "plugins/disasm/ppc/ppc.h"
#include "biewlib/kbd_code.h"
#include "biewlib/file_ini.h"
#include "biewlib/pmalloc.h"

#include "ppc.h"

static char *outstr;

static int ppcBitness=DAB_USE32;
static int ppcBigEndian=1;
static int ppcDialect=0;

static const char *gprs[] =
{
  "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
  "r8", "r9", "r10","r11","r12","r13","r14","r15",
  "r16","r17","r18","r19","r20","r21","r22","r23",
  "r24","r25","r26","r27","r28","r29","r30","r31"
};

static const char *fprs[] =
{
  "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
  "f8", "f9", "f10","f11","f12","f13","f14","f15",
  "f16","f17","f18","f19","f20","f21","f22","f23",
  "f24","f25","f26","f27","f28","f29","f30","f31"
};

static const char *vprs[] =
{
  "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
  "v8", "v9", "v10","v11","v12","v13","v14","v15",
  "v16","v17","v18","v19","v20","v21","v22","v23",
  "v24","v25","v26","v27","v28","v29","v30","v31"
};

static void ppc_Encode_args(char *ostr,tUInt32 opcode,
			__fileoff_t ulShift,
			unsigned long flags,
			const ppc_arg *args) {
    __fileoff_t dig_off;
    unsigned long dig_flg;
    unsigned i,dig_sz;
    int use_ea=0;
    TabSpace(ostr,TAB_POS);
    for(i=0;i<6;i++) {
	unsigned value,len;
	if(!args[i].type) break;
	dig_off=-1;
	len=args[i].len;
	value=PPC_GET_BITS(opcode,args[i].off,len);
	if((args[i].flg&PPC_LSHIFT_MASK)) value<<=(args[i].flg&PPC_LSHIFT_MASK);
	if(!(args[i].flg&PPC_EA) && use_ea) { use_ea=0; strcat(ostr,"]"); }
	if(i) strcat(ostr,use_ea?args[i].type=='-'?"":"+":",");
	if((args[i].flg&PPC_EA)) { if(!use_ea) strcat(ostr,"["); use_ea=1; }
	switch(args[i].type) {
	    case 'r': strcat(ostr,gprs[value&0x3F]); break;
	    case 'f': strcat(ostr,fprs[value&0x3F]); break;
	    case 'v': strcat(ostr,vprs[value&0x3F]); break;
	    case '-': dig_off = ulShift+(args[i].off+7)/8;
			dig_sz = (len+7)/8;
			dig_flg =	len<9?  DISARG_CHAR:
					len<17? DISARG_SHORT:
						DISARG_LONG;
			break;
	    case '+': dig_off = ulShift+(args[i].off+7)/8;
			dig_sz = (len+7)/8;
			dig_flg =	len<9?  DISARG_BYTE:
					len<17? DISARG_WORD:
						DISARG_DWORD;
			break;
	    default: /* internal disassembler error */
		break;
	}
	if(dig_off>0) {
	    if(flags&PPC_BRANCH_INSN) {
		if(len>6) {
		    int aa = PPC_GET_BITS(opcode,30,1);
		    unsigned long distin = (value<<2) + (aa?0:ulShift);
		    disAppendFAddr(ostr,dig_off,value,distin,
				DISADR_NEAR32,0,dig_sz);
		}
		else goto do_digs;
	    }
	    else {
	        do_digs:
		disAppendDigits(ostr,dig_off,APREF_USE_TYPE,dig_sz,&opcode,dig_flg);
	    }
	}
    }
    if(use_ea) strcat(ostr,"]");
}

static const ppc_opcode ppc_table[] =
{
  { "add",     XO_FORM(31,266,0,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "add.",    XO_FORM(31,266,0,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "addo",    XO_FORM(31,266,1,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "addo.",   XO_FORM(31,266,1,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "adc",     XO_FORM(31,10,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "adc.",    XO_FORM(31,10,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "adco",    XO_FORM(31,10,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "adco.",   XO_FORM(31,10,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "ade",     XO_FORM(31,138,0,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "ade.",    XO_FORM(31,138,0,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "adeo",    XO_FORM(31,138,1,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "adeo.",   XO_FORM(31,138,1,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "addg6s",  XO_SHRT_FORM(31,74), XO_SHRT_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "addi",    D_FORM(14),            D_MASK, PPC_CPU, {D_RT,D_RA,D_SI,PPC_0} },
  { "addic",   D_FORM(12),            D_MASK, PPC_CPU, {D_RT,D_RA,D_SI,PPC_0} },
  { "addic.",  D_FORM(13),            D_MASK, PPC_CPU, {D_RT,D_RA,D_SI,PPC_0} },
  { "addis",   D_FORM(15),            D_MASK, PPC_CPU, {D_RT,D_RA,D_SI,PPC_0} },
  { "adme",    XO_FORM(31,234,0,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "adme.",   XO_FORM(31,234,0,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "admeo",   XO_FORM(31,234,1,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "admeo.",  XO_FORM(31,234,1,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "adze",    XO_FORM(31,202,0,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "adze.",   XO_FORM(31,202,0,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "adzeo",   XO_FORM(31,202,1,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "adzeo.",  XO_FORM(31,202,1,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "and",     X_FORM(31,28,0),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  { "and.",    X_FORM(31,28,1),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  { "andc",    X_FORM(31,60,0),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  { "andc.",   X_FORM(31,60,1),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  { "andi.",   D_FORM(28),            D_MASK, PPC_CPU, {D_RA,D_RS,D_UI,PPC_0} },
  { "andis.",  D_FORM(29),            D_MASK, PPC_CPU, {D_RA,D_RS,D_UI,PPC_0} },
  { "b",       I_FORM(18,0,0),        I_MASK, PPC_CPU|PPC_BRANCH_INSN, {I_LI,PPC_0} },
  { "ba",      I_FORM(18,1,0),        I_MASK, PPC_CPU|PPC_BRANCH_INSN, {I_LI,PPC_0} },
  { "bl",      I_FORM(18,0,1),        I_MASK, PPC_CPU|PPC_BRANCH_INSN, {I_LI,PPC_0} },
  { "bla",     I_FORM(18,1,1),        I_MASK, PPC_CPU|PPC_BRANCH_INSN, {I_LI,PPC_0} },
  { "bc",      B_FORM(16,0,0),        B_MASK, PPC_CPU|PPC_BRANCH_INSN, {B_BO,B_BI,B_BD,PPC_0} },
  { "bca",     B_FORM(16,1,0),        B_MASK, PPC_CPU|PPC_BRANCH_INSN, {B_BO,B_BI,B_BD,PPC_0} },
  { "bcl",     B_FORM(16,0,1),        B_MASK, PPC_CPU|PPC_BRANCH_INSN, {B_BO,B_BI,B_BD,PPC_0} },
  { "bcla",    B_FORM(16,1,1),        B_MASK, PPC_CPU|PPC_BRANCH_INSN, {B_BO,B_BI,B_BD,PPC_0} },
  { "bclr",    XL_FORM(18,16,0),     XL_MASK, PPC_CPU|PPC_BRANCH_INSN, {XL_BO,XL_BI,XL_BH,PPC_0} },
  { "bclrl",   XL_FORM(18,16,1),     XL_MASK, PPC_CPU|PPC_BRANCH_INSN, {XL_BO,XL_BI,XL_BH,PPC_0} },
  { "bcctr",   XL_FORM(18,528,0),    XL_MASK, PPC_CPU|PPC_BRANCH_INSN, {XL_BO,XL_BI,XL_BH,PPC_0} },
  { "bcctrl",  XL_FORM(18,528,1),    XL_MASK, PPC_CPU|PPC_BRANCH_INSN, {XL_BO,XL_BI,XL_BH,PPC_0} },
  { "brinc",   EVX_FORM(4,527),     EVX_MASK, PPC_CPU, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "cbcdtd",  X_FORM(31,314,0),      X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  { "cbcdtd",  X_FORM(31,314,1),      X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  { "cdtbcd",  X_FORM(31,282,0),      X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  { "cdtbcd",  X_FORM(31,282,1),      X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  { "cmp",     X_FORM(31,0,0),        X_MASK, PPC_CPU, {X_BF,X_L,X_RA,X_RB,PPC_0} },
  { "cmp",     X_FORM(31,0,1),        X_MASK, PPC_CPU, {X_BF,X_L,X_RA,X_RB,PPC_0} },
  { "cmpb",    X_FORM(31,0,0),        X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  { "cmpb",    X_FORM(31,0,1),        X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  { "cmpi",    D_FORM(11),            D_MASK, PPC_CPU, {D_BF,D_L,D_RA,D_SI,PPC_0} },
  { "cmpl",    X_FORM(31,32,0),       X_MASK, PPC_CPU, {X_BF,X_L,X_RA,X_RB,PPC_0} },
  { "cmpl",    X_FORM(31,32,1),       X_MASK, PPC_CPU, {X_BF,X_L,X_RA,X_RB,PPC_0} },
  { "cmpli",   D_FORM(10),            D_MASK, PPC_CPU, {D_BF,D_L,D_RA,D_UI,PPC_0} },
  { "cntlzd",  X_FORM(31,58,0),       X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  { "cntlzd.", X_FORM(31,58,1),       X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  { "cntlzw",  X_FORM(31,26,0),       X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  { "cntlzw.", X_FORM(31,26,1),       X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  { "crand",  XL_FORM(19,257,0),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "crand",  XL_FORM(19,257,1),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "crandc", XL_FORM(19,129,0),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "crandc", XL_FORM(19,129,1),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "creqv",  XL_FORM(19,289,0),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "creqv",  XL_FORM(19,289,1),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "crnand", XL_FORM(19,225,0),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "crnand", XL_FORM(19,225,1),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "crnor",  XL_FORM(19,33,0),      XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "crnor",  XL_FORM(19,33,1),      XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "cror",   XL_FORM(19,449,0),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "cror",   XL_FORM(19,449,1),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "crorc",  XL_FORM(19,129,0),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "crorc",  XL_FORM(19,129,1),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "crxor",  XL_FORM(19,193,0),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "crxor",  XL_FORM(19,193,1),     XL_MASK, PPC_CPU, {XL_BT,XL_BA,XL_BB,PPC_0} },
  { "dadd",   X_FORM(59,2,0),         X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "dadd.",  X_FORM(59,2,1),         X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "daddq",  X_FORM(63,2,0),         X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "daddq.", X_FORM(63,2,1),         X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "dcba",   X_FORM(31,758,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcba",   X_FORM(31,758,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbf",   X_FORM(31,86,0),        X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_L2,PPC_0} },
  { "dcbf",   X_FORM(31,86,1),        X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_L2,PPC_0} },
  { "dcbfep", X_FORM(31,127,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbfep", X_FORM(31,127,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbi",   X_FORM(31,470,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbi",   X_FORM(31,470,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcblc",  X_FORM(31,390,0),       X_MASK, PPC_CPU, {X_CT,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcblc",  X_FORM(31,390,1),       X_MASK, PPC_CPU, {X_CT,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbst",  X_FORM(31,54,0),        X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbst",  X_FORM(31,54,1),        X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbstep",X_FORM(31,63,0),        X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbstep",X_FORM(31,63,1),        X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbt",   X_FORM(31,278,0),       X_MASK, PPC_CPU, {X_TH,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbt",   X_FORM(31,278,1),       X_MASK, PPC_CPU, {X_TH,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbtep", X_FORM(31,319,0),       X_MASK, PPC_CPU, {X_TH,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbtep", X_FORM(31,319,1),       X_MASK, PPC_CPU, {X_TH,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbtls", X_FORM(31,166,0),       X_MASK, PPC_CPU, {X_CT,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbtls", X_FORM(31,166,1),       X_MASK, PPC_CPU, {X_CT,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbtst", X_FORM(31,246,0),       X_MASK, PPC_CPU, {X_TH,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbtst", X_FORM(31,246,1),       X_MASK, PPC_CPU, {X_TH,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbtstep",X_FORM(31,255,0),      X_MASK, PPC_CPU, {X_TH,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbtstep",X_FORM(31,255,1),      X_MASK, PPC_CPU, {X_TH,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbtstls",X_FORM(31,134,0),      X_MASK, PPC_CPU, {X_CT,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbtstls",X_FORM(31,134,1),      X_MASK, PPC_CPU, {X_CT,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbz",   X_FORM(31,1014,0),      X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbz",   X_FORM(31,1014,1),      X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbzep", X_FORM(31,1023,0),      X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcbzep", X_FORM(31,1023,1),      X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "dcffixq",X_FORM(63,802,0),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dcffixq.",X_FORM(63,802,1),      X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dci",    X_FORM(31,454,0),       X_MASK, PPC_CPU, {X_CT,PPC_0} },
  { "dci",    X_FORM(31,454,1),       X_MASK, PPC_CPU, {X_CT,PPC_0} },
  { "dcmpo",  X_FORM(59,130,0),       X_MASK, PPC_FPU, {X_BF,X_FRA,X_FRB,PPC_0} },
  { "dcmpo",  X_FORM(59,130,1),       X_MASK, PPC_FPU, {X_BF,X_FRA,X_FRB,PPC_0} },
  { "dcmpoq", X_FORM(63,130,0),       X_MASK, PPC_FPU, {X_BF,X_FRA,X_FRB,PPC_0} },
  { "dcmpoq", X_FORM(63,130,1),       X_MASK, PPC_FPU, {X_BF,X_FRA,X_FRB,PPC_0} },
  { "dcmpu",  X_FORM(59,642,0),       X_MASK, PPC_FPU, {X_BF,X_FRA,X_FRB,PPC_0} },
  { "dcmpu",  X_FORM(59,642,1),       X_MASK, PPC_FPU, {X_BF,X_FRA,X_FRB,PPC_0} },
  { "dcmpuq", X_FORM(63,642,0),       X_MASK, PPC_FPU, {X_BF,X_FRA,X_FRB,PPC_0} },
  { "dcmpuq", X_FORM(63,642,1),       X_MASK, PPC_FPU, {X_BF,X_FRA,X_FRB,PPC_0} },
  { "dcread", X_FORM(31,486,0),       X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcread", X_FORM(31,486,1),       X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcread", X_FORM(31,326,0),       X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcread", X_FORM(31,326,1),       X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  { "dcffixq",X_FORM(63,802,0),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dcffixq.",X_FORM(63,802,1),      X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dctdp",  X_FORM(59,258,0),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dctdp.", X_FORM(59,258,1),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dctfix", X_FORM(59,290,0),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dctfix.",X_FORM(59,290,1),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dctfixq",X_FORM(63,290,0),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dctfixq.",X_FORM(63,290,1),      X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dctqpq", X_FORM(63,258,0),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dctqpq.",X_FORM(63,258,1),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "ddedpd", X_FORM(59,322,0),       X_MASK, PPC_FPU, {X_SP,X_FRT,X_FRB,PPC_0} },
  { "ddedpd.",X_FORM(59,322,1),       X_MASK, PPC_FPU, {X_SP,X_FRT,X_FRB,PPC_0} },
  { "ddedpdq",X_FORM(63,322,0),       X_MASK, PPC_FPU, {X_SP,X_FRT,X_FRB,PPC_0} },
  { "ddedpdq.",X_FORM(63,322,1),      X_MASK, PPC_FPU, {X_SP,X_FRT,X_FRB,PPC_0} },
  { "ddiv",   X_FORM(59,546,0),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "ddiv.",  X_FORM(59,546,1),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "ddivq",  X_FORM(63,546,0),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "ddivq.", X_FORM(63,546,1),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "denbcd", X_FORM(59,834,0),       X_MASK, PPC_FPU, {X_SP,X_FRT,X_FRB,PPC_0} },
  { "denbcd.",X_FORM(59,834,1),       X_MASK, PPC_FPU, {X_SP,X_FRT,X_FRB,PPC_0} },
  { "denbcdq",X_FORM(63,834,0),       X_MASK, PPC_FPU, {X_SP,X_FRT,X_FRB,PPC_0} },
  { "denbcdq.",X_FORM(63,834,1),      X_MASK, PPC_FPU, {X_SP,X_FRT,X_FRB,PPC_0} },
  { "diex",   X_FORM(59,866,0),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "diex.",  X_FORM(59,866,1),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "diexq",  X_FORM(63,866,0),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "diexq.", X_FORM(63,866,1),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "divd",   XO_FORM(31,489,0,0),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divd.",  XO_FORM(31,489,0,1),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divdo",  XO_FORM(31,489,1,0),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divdo.", XO_FORM(31,489,1,1),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divdu",  XO_FORM(31,457,0,0),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divdu.", XO_FORM(31,457,0,1),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divduo", XO_FORM(31,457,1,0),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divduo.",XO_FORM(31,457,1,1),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divw",   XO_FORM(31,491,0,0),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divw.",  XO_FORM(31,491,0,1),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divwo",  XO_FORM(31,491,1,0),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divwo.", XO_FORM(31,491,1,1),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divwu",  XO_FORM(31,459,0,0),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divwu.", XO_FORM(31,459,0,1),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divwuo", XO_FORM(31,459,1,0),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "divwuo.",XO_FORM(31,459,1,1),   XO_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "dlmzb",   X_FORM(31,78,0),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  { "dlmzb.",  X_FORM(31,78,1),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  { "dmul",    X_FORM(59,34,0),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "dmul.",   X_FORM(59,34,1),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "dmulq",   X_FORM(63,34,0),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "dmulq.",  X_FORM(63,34,1),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "dnh",    XFX_FORM(19,0,198,0), XFX_MASK, PPC_CPU, {XFX_DUI,XFX_DUIS,PPC_0} },
  { "dnh.",   XFX_FORM(19,0,198,1), XFX_MASK, PPC_CPU, {XFX_DUI,XFX_DUIS,PPC_0} },
  { "dnh",    XFX_FORM(19,1,198,0), XFX_MASK, PPC_CPU, {XFX_DUI,XFX_DUIS,PPC_0} },
  { "dnh.",   XFX_FORM(19,1,198,1), XFX_MASK, PPC_CPU, {XFX_DUI,XFX_DUIS,PPC_0} },
  { "doze",    XL_FORM(19,402,0),    XL_MASK, PPC_CPU, {PPC_0} },
  { "doze",    XL_FORM(19,402,1),    XL_MASK, PPC_CPU, {PPC_0} },
  { "dqua",   Z23_FORM(59,3,0),     Z23_MASK, PPC_FPU, {Z23_FRT,Z23_FRA,Z23_FRB,Z23_RMC,PPC_0} },
  { "dqua.",  Z23_FORM(59,3,1),     Z23_MASK, PPC_FPU, {Z23_FRT,Z23_FRA,Z23_FRB,Z23_RMC,PPC_0} },
  { "dquai",  Z23_FORM(59,67,0),    Z23_MASK, PPC_FPU, {Z23_TE,Z23_FRT,Z23_FRB,Z23_RMC,PPC_0} },
  { "dquai.", Z23_FORM(59,67,1),    Z23_MASK, PPC_FPU, {Z23_TE,Z23_FRT,Z23_FRB,Z23_RMC,PPC_0} },
  { "dquaiq", Z23_FORM(63,67,0),    Z23_MASK, PPC_FPU, {Z23_TE,Z23_FRT,Z23_FRB,Z23_RMC,PPC_0} },
  { "dquaiq.",Z23_FORM(63,67,1),    Z23_MASK, PPC_FPU, {Z23_TE,Z23_FRT,Z23_FRB,Z23_RMC,PPC_0} },
  { "dquaq",  Z23_FORM(63,3,0),     Z23_MASK, PPC_FPU, {Z23_FRT,Z23_FRA,Z23_FRB,Z23_RMC,PPC_0} },
  { "dquaq.", Z23_FORM(63,3,1),     Z23_MASK, PPC_FPU, {Z23_FRT,Z23_FRA,Z23_FRB,Z23_RMC,PPC_0} },
  { "drdpq",  X_FORM(63,770,0),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "drdpq.", X_FORM(63,770,1),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "drintn", Z23_FORM(59,227,0),   Z23_MASK, PPC_FPU, {Z23_R,Z23_FRT,Z23_FRB,Z23_RMC,PPC_0} },
  { "drintn.",Z23_FORM(59,227,1),   Z23_MASK, PPC_FPU, {Z23_R,Z23_FRT,Z23_FRB,Z23_RMC,PPC_0} },
  { "drintnq",Z23_FORM(63,227,0),   Z23_MASK, PPC_FPU, {Z23_R,Z23_FRT,Z23_FRB,Z23_RMC,PPC_0} },
  { "drintnq.",Z23_FORM(63,227,1),  Z23_MASK, PPC_FPU, {Z23_R,Z23_FRT,Z23_FRB,Z23_RMC,PPC_0} },
  { "drintx", Z23_FORM(59,99,0),    Z23_MASK, PPC_FPU, {Z23_R,Z23_FRT,Z23_FRB,Z23_RMC,PPC_0} },
  { "drintx.",Z23_FORM(59,99,1),    Z23_MASK, PPC_FPU, {Z23_R,Z23_FRT,Z23_FRB,Z23_RMC,PPC_0} },
  { "drintxq",Z23_FORM(63,99,0),    Z23_MASK, PPC_FPU, {Z23_R,Z23_FRT,Z23_FRB,Z23_RMC,PPC_0} },
  { "drintxq.",Z23_FORM(63,99,1),   Z23_MASK, PPC_FPU, {Z23_R,Z23_FRT,Z23_FRB,Z23_RMC,PPC_0} },
  { "drrnd",  Z23_FORM(59,35,0),    Z23_MASK, PPC_FPU, {Z23_FRT,Z23_FRA,Z23_FRB,Z23_RMC,PPC_0} },
  { "drrnd.", Z23_FORM(59,35,1),    Z23_MASK, PPC_FPU, {Z23_FRT,Z23_FRA,Z23_FRB,Z23_RMC,PPC_0} },
  { "drrndq", Z23_FORM(63,35,0),    Z23_MASK, PPC_FPU, {Z23_FRT,Z23_FRA,Z23_FRB,Z23_RMC,PPC_0} },
  { "drrndq.",Z23_FORM(63,35,1),    Z23_MASK, PPC_FPU, {Z23_FRT,Z23_FRA,Z23_FRB,Z23_RMC,PPC_0} },
  { "drsp",   X_FORM(59,770,0),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "drsp.",  X_FORM(59,770,1),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dscli", Z22_FORM(59,66,0),     Z22_MASK, PPC_FPU, {Z22_FRT,Z22_FRA,Z22_SH,PPC_0} },
  { "dscli.",Z22_FORM(59,66,1),     Z22_MASK, PPC_FPU, {Z22_FRT,Z22_FRA,Z22_SH,PPC_0} },
  { "dscliq",Z22_FORM(63,66,0),     Z22_MASK, PPC_FPU, {Z22_FRT,Z22_FRA,Z22_SH,PPC_0} },
  { "dscliq.",Z22_FORM(63,66,1),    Z22_MASK, PPC_FPU, {Z22_FRT,Z22_FRA,Z22_SH,PPC_0} },
  { "dscri", Z22_FORM(59,98,0),     Z22_MASK, PPC_FPU, {Z22_FRT,Z22_FRA,Z22_SH,PPC_0} },
  { "dscri.",Z22_FORM(59,98,1),     Z22_MASK, PPC_FPU, {Z22_FRT,Z22_FRA,Z22_SH,PPC_0} },
  { "dscriq",Z22_FORM(63,98,0),     Z22_MASK, PPC_FPU, {Z22_FRT,Z22_FRA,Z22_SH,PPC_0} },
  { "dscriq.",Z22_FORM(63,98,1),    Z22_MASK, PPC_FPU, {Z22_FRT,Z22_FRA,Z22_SH,PPC_0} },
  { "dsub",   X_FORM(59,514,0),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "dsub.",  X_FORM(59,514,1),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "dsubq",  X_FORM(63,514,0),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "dsubq.", X_FORM(63,514,1),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  { "dtstdc", Z22_FORM(59,194,0),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_DCM,PPC_0} },
  { "dtstdc.",Z22_FORM(59,194,1),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_DCM,PPC_0} },
  { "dtstdcq",Z22_FORM(63,194,0),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_DCM,PPC_0} },
  { "dtstdcq.",Z22_FORM(63,194,1),  Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_DCM,PPC_0} },
  { "dtstdg", Z22_FORM(59,226,0),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_DCM,PPC_0} },
  { "dtstdg.",Z22_FORM(59,226,1),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_DCM,PPC_0} },
  { "dtstdgq",Z22_FORM(63,226,0),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_DCM,PPC_0} },
  { "dtstdgq.",Z22_FORM(63,226,1),  Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_DCM,PPC_0} },
  { "dtstex",   X_FORM(59,162,0),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_FRB,PPC_0} },
  { "dtstex.",  X_FORM(59,162,1),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_FRB,PPC_0} },
  { "dtstexq",  X_FORM(63,162,0),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_FRB,PPC_0} },
  { "dtstexq.", X_FORM(63,162,1),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_FRB,PPC_0} },
  { "dtstsf",   X_FORM(59,674,0),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_FRB,PPC_0} },
  { "dtstsf.",  X_FORM(59,674,1),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_FRB,PPC_0} },
  { "dtstsfq",  X_FORM(63,674,0),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_FRB,PPC_0} },
  { "dtstsfq.", X_FORM(63,674,1),   Z22_MASK, PPC_FPU, {Z22_BF,Z22_FRA,Z22_FRB,PPC_0} },
  { "dxex",   X_FORM(59,354,0),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dxex.",  X_FORM(59,354,1),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dxexq",  X_FORM(63,354,0),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "dxexq.", X_FORM(63,354,1),       X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  { "eciwx",  X_FORM(31,310,0),       X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  { "eciwx.", X_FORM(31,310,1),       X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  { "ecowx",  X_FORM(31,438,0),       X_MASK, PPC_CPU, {X_RS,X_RA_EA,X_RB_EA,PPC_0} },
  { "ecowx.", X_FORM(31,438,1),       X_MASK, PPC_CPU, {X_RS,X_RA_EA,X_RB_EA,PPC_0} },
  { "efdabs",EVX_FORM(4,740),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "efdadd",EVX_FORM(4,736),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "efdcfs",EVX_FORM(4,751),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdcfsi",EVX_FORM(4,753),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdcfsid",EVX_FORM(4,739),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdcfsf",EVX_FORM(4,755),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdcfui",EVX_FORM(4,752),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdcfuid",EVX_FORM(4,738),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdcfuf",EVX_FORM(4,754),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdcmpeq",EVX_FORM(4,750),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  {"efdcmpgt",EVX_FORM(4,748),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  {"efdcmplt",EVX_FORM(4,749),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  {"efdctfs",EVX_FORM(4,759),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdctsi",EVX_FORM(4,757),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdctsidz",EVX_FORM(4,747),     EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdctsiz",EVX_FORM(4,762),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdctuf",EVX_FORM(4,758),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdctui",EVX_FORM(4,756),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdctuidz",EVX_FORM(4,746),     EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efdctuiz",EVX_FORM(4,760),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  { "efddiv",EVX_FORM(4,745),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "efdmul",EVX_FORM(4,744),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"efdnabs",EVX_FORM(4,741),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "efdneg",EVX_FORM(4,742),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "efdsub",EVX_FORM(4,737),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"efdtsteq",EVX_FORM(4,766),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  {"efdtstgt",EVX_FORM(4,764),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  {"efdtstlt",EVX_FORM(4,765),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  { "efsabs",EVX_FORM(4,708),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "efsadd",EVX_FORM(4,704),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"efscfsf",EVX_FORM(4,723),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efscfsi",EVX_FORM(4,721),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efscfui",EVX_FORM(4,720),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efscfuf",EVX_FORM(4,722),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efscmpeq",EVX_FORM(4,718),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  {"efscmpgt",EVX_FORM(4,716),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  {"efscmplt",EVX_FORM(4,717),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  {"efsctsf",EVX_FORM(4,727),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efsctsi",EVX_FORM(4,725),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efsctsiz",EVX_FORM(4,730),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efsctuf",EVX_FORM(4,726),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efsctui",EVX_FORM(4,724),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  {"efsctuiz",EVX_FORM(4,728),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  { "efsdiv",EVX_FORM(4,713),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "efsmul",EVX_FORM(4,712),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"efsnabs",EVX_FORM(4,709),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "efsneg",EVX_FORM(4,710),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "efssub",EVX_FORM(4,705),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"efststeq",EVX_FORM(4,734),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  {"efststgt",EVX_FORM(4,732),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  {"efststlt",EVX_FORM(4,733),      EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  { "eieio",  X_FORM(31,854,0),       X_MASK, PPC_CPU, {PPC_0} },
  { "eieio",  X_FORM(31,854,1),       X_MASK, PPC_CPU, {PPC_0} },
  {  "eqv",   X_FORM(31,284,0),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {  "eqv.",  X_FORM(31,284,1),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  { "evabs", EVX_FORM(4,520),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  {"evaddiw",EVX_FORM(4,514),       EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,EVX_UI,PPC_0} },
  {"evaddsmiaaw",EVX_FORM(4,1225),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  {"evaddssiaaw",EVX_FORM(4,1217),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  {"evaddumiaaw",EVX_FORM(4,1224),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  {"evaddusiaaw",EVX_FORM(4,1216),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "evaddw",    EVX_FORM(4,512),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evand",     EVX_FORM(4,529),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evandc",    EVX_FORM(4,530),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evcmpeq",   EVX_FORM(4,564),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  { "evcmpgts",  EVX_FORM(4,561),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  { "evcmpgtu",  EVX_FORM(4,560),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  { "evcmplts",  EVX_FORM(4,563),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  { "evcmpltu",  EVX_FORM(4,562),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  { "evcntlsw",  EVX_FORM(4,526),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "evcntlzw",  EVX_FORM(4,525),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "evdivws",   EVX_FORM(4,1222),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evdivwu",   EVX_FORM(4,1223),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "eveqv",     EVX_FORM(4,537),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evextsb",   EVX_FORM(4,522),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "evextsh",   EVX_FORM(4,523),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "evfsabs",   EVX_FORM(4,644),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "evfsadd",   EVX_FORM(4,640),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evfscfsf",  EVX_FORM(4,659),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  { "evfscfsi",  EVX_FORM(4,657),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  { "evfscfuf",  EVX_FORM(4,658),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  { "evfscfui",  EVX_FORM(4,656),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  { "evfscmpeq", EVX_FORM(4,654),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  { "evfscmpgt", EVX_FORM(4,652),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  { "evfscmplt", EVX_FORM(4,653),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  { "evfsctsf",  EVX_FORM(4,663),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  { "evfsctsi",  EVX_FORM(4,661),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  { "evfsctsiz", EVX_FORM(4,666),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  { "evfsctuf",  EVX_FORM(4,662),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  { "evfsctui",  EVX_FORM(4,660),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  { "evfsctuiz", EVX_FORM(4,664),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RB,PPC_0} },
  { "evfsdiv",   EVX_FORM(4,649),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evfsmul",   EVX_FORM(4,648),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evfsnabs",  EVX_FORM(4,645),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "evfsneg",   EVX_FORM(4,646),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  { "evfssub",   EVX_FORM(4,641),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evfststeq", EVX_FORM(4,670),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  { "evfststgt", EVX_FORM(4,668),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  { "evfststlt", EVX_FORM(4,669),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_BF,EVX_RA,EVX_RB,PPC_0} },
  { "evldd",     EVX_FORM(4,769),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_UI_EA,PPC_0} },
  { "evlddx",    EVX_FORM(4,768),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_RB_EA,PPC_0} },
  { "evldh",     EVX_FORM(4,773),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_UI_EA,PPC_0} },
  { "evldhx",    EVX_FORM(4,772),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_RB_EA,PPC_0} },
  { "evldw",     EVX_FORM(4,771),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_UI_EA,PPC_0} },
  { "evldwx",    EVX_FORM(4,770),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_RB_EA,PPC_0} },
  {"evlhhesplat",EVX_FORM(4,777),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_UI_EA,PPC_0} },
  {"evlhhesplatx",EVX_FORM(4,776),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_RB_EA,PPC_0} },
  {"evlhhosplat",EVX_FORM(4,783),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_UI_EA,PPC_0} },
  {"evlhhosplatx",EVX_FORM(4,782),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_RB_EA,PPC_0} },
  {"evlhhousplat",EVX_FORM(4,781),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_UI_EA,PPC_0} },
  {"evlhhousplatx",EVX_FORM(4,780), EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_RB_EA,PPC_0} },
  { "evlwhe",    EVX_FORM(4,785),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_UI_EA,PPC_0} },
  { "evlwhex",   EVX_FORM(4,784),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_RB_EA,PPC_0} },
  { "evlwhos",   EVX_FORM(4,791),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_UI_EA,PPC_0} },
  { "evlwhosx",  EVX_FORM(4,790),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_RB_EA,PPC_0} },
  { "evlwhou",   EVX_FORM(4,789),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_UI_EA,PPC_0} },
  { "evlwhoux",  EVX_FORM(4,788),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_RB_EA,PPC_0} },
  { "evlwhsplat",EVX_FORM(4,797),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_UI_EA,PPC_0} },
  {"evlwhsplatx",EVX_FORM(4,796),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_RB_EA,PPC_0} },
  { "evlwwsplat",EVX_FORM(4,793),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_UI_EA,PPC_0} },
  {"evlwwsplatx",EVX_FORM(4,792),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA_EA,EVX_RB_EA,PPC_0} },
  { "evmergehi", EVX_FORM(4,556),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmergehilo",EVX_FORM(4,558),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evmergelo", EVX_FORM(4,557),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmergelohi",EVX_FORM(4,559),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhegsmfaa",EVX_FORM(4,1323),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhegsmfan",EVX_FORM(4,1451),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhegsmiaa",EVX_FORM(4,1321),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhegsmian",EVX_FORM(4,1449),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhegumiaa",EVX_FORM(4,1320),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhegumian",EVX_FORM(4,1448),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhesmf", EVX_FORM(4,1035),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhesmfa",EVX_FORM(4,1067),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhesmfaaw",EVX_FORM(4,1291),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhesmfanw",EVX_FORM(4,1419),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhesmi", EVX_FORM(4,1033),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhesmia",EVX_FORM(4,1065),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhesmiaaw",EVX_FORM(4,1289),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhesmianw",EVX_FORM(4,1417),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhessf", EVX_FORM(4,1027),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhessfa",EVX_FORM(4,1059),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhessfaaw",EVX_FORM(4,1283),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhessfanw",EVX_FORM(4,1411),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhessiaaw",EVX_FORM(4,1281),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhessianw",EVX_FORM(4,1409),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmheumi", EVX_FORM(4,1032),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmheumia",EVX_FORM(4,1064),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmheumiaaw",EVX_FORM(4,1288),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmheumianw",EVX_FORM(4,1416),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmheusiaaw",EVX_FORM(4,1280),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmheusianw",EVX_FORM(4,1408),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhogsmfaa",EVX_FORM(4,1327),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhogsmfan",EVX_FORM(4,1455),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhogsmiaa",EVX_FORM(4,1325),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhogsmian",EVX_FORM(4,1453),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhogumiaa",EVX_FORM(4,1324),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhogumian",EVX_FORM(4,1452),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhosmf", EVX_FORM(4,1039),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhosmfa",EVX_FORM(4,1071),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhosmfaaw",EVX_FORM(4,1295),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhosmfanw",EVX_FORM(4,1423),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhosmi", EVX_FORM(4,1037),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhosmia",EVX_FORM(4,1069),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhosmiaaw",EVX_FORM(4,1293),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhosmianw",EVX_FORM(4,1421),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhossf", EVX_FORM(4,1031),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhossfa",EVX_FORM(4,1063),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhossfaaw",EVX_FORM(4,1287),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhossfanw",EVX_FORM(4,1415),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhossiaaw",EVX_FORM(4,1285),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhossianw",EVX_FORM(4,1413),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhoumi", EVX_FORM(4,1036),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmhoumia",EVX_FORM(4,1068),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhoumiaaw",EVX_FORM(4,1292),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhoumianw",EVX_FORM(4,1420),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhousiaaw",EVX_FORM(4,1284),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmhousianw",EVX_FORM(4,1412),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {     "evmra", EVX_FORM(4,1220),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  {  "evmwhsmf", EVX_FORM(4,1103),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwhsmfa",EVX_FORM(4,1135),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwhsmi", EVX_FORM(4,1101),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwhsmia",EVX_FORM(4,1133),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwhssf", EVX_FORM(4,1095),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwhssfa",EVX_FORM(4,1127),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwhumi", EVX_FORM(4,1100),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwhumia",EVX_FORM(4,1132),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmwlsmiaaw",EVX_FORM(4,1353),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmwlsmianw",EVX_FORM(4,1481),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmwlssiaaw",EVX_FORM(4,1345),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmwlssianw",EVX_FORM(4,1473),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwlumi", EVX_FORM(4,1096),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwlumia",EVX_FORM(4,1128),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmwlumiaaw",EVX_FORM(4,1352),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmwlumianw",EVX_FORM(4,1480),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmwlusiaaw",EVX_FORM(4,1344),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {"evmwlusianw",EVX_FORM(4,1472),  EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwsmf", EVX_FORM(4,1115),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwsmfa",EVX_FORM(4,1147),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evmwsmfaa",EVX_FORM(4,1371),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evmwsmfan",EVX_FORM(4,1499),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwsmi", EVX_FORM(4,1113),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwsmia",EVX_FORM(4,1145),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evmwsmiaa",EVX_FORM(4,1369),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evmwsmian",EVX_FORM(4,1497),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwssf", EVX_FORM(4,1107),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwssfa",EVX_FORM(4,1139),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evmwssfaa",EVX_FORM(4,1363),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evmwssfan",EVX_FORM(4,1491),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwumi", EVX_FORM(4,1112),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evmwumia",EVX_FORM(4,1144),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evmwumiaa",EVX_FORM(4,1368),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  { "evmwumian",EVX_FORM(4,1496),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {    "evnand",EVX_FORM(4,542),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {     "evneg",EVX_FORM(4,521),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  {     "evnor",EVX_FORM(4,536),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {      "evor",EVX_FORM(4,535),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {     "evorc",EVX_FORM(4,539),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {     "evrlw",EVX_FORM(4,552),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {    "evrlwi",EVX_FORM(4,554),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_UI,PPC_0} },
  {    "evrndw",EVX_FORM(4,524),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  {    "evsel", EVS_FORM(4,79),     EVS_MASK, PPC_CPU|PPC_SPE, {EVS_RT,EVS_RA,EVS_RB,EVS_BFA,PPC_0} },
  {    "evslw", EVX_FORM(4,548),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {    "evslwi",EVX_FORM(4,550),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_UI,PPC_0} },
  { "evsplatfi",EVX_FORM(4,555),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_SI,PPC_0} },
  {  "evsplati",EVX_FORM(4,553),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_SI,PPC_0} },
  {   "evsrwis",EVX_FORM(4,547),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_UI,PPC_0} },
  {   "evsrwiu",EVX_FORM(4,546),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_UI,PPC_0} },
  {    "evsrws",EVX_FORM(4,545),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {    "evsrwu",EVX_FORM(4,544),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {    "evstdd",EVX_FORM(4,801),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_UI_EA,EVX_RS,PPC_0} },
  {   "evstddx",EVX_FORM(4,800),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_RB_EA,EVX_RS,PPC_0} },
  { "evstddepx",EVX_FORM(31,413),   EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_RB_EA,EVX_RS,PPC_0} },
  {    "evstdh",EVX_FORM(4,805),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_UI_EA,EVX_RS,PPC_0} },
  {   "evstdhx",EVX_FORM(4,804),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_RB_EA,EVX_RS,PPC_0} },
  {    "evstdw",EVX_FORM(4,803),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_UI_EA,EVX_RS,PPC_0} },
  {   "evstdwx",EVX_FORM(4,802),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_RB_EA,EVX_RS,PPC_0} },
  {   "evstwhe",EVX_FORM(4,817),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_UI_EA,EVX_RS,PPC_0} },
  {  "evstwhex",EVX_FORM(4,816),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_RB_EA,EVX_RS,PPC_0} },
  {   "evstwho",EVX_FORM(4,821),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_UI_EA,EVX_RS,PPC_0} },
  {  "evstwhox",EVX_FORM(4,820),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_RB_EA,EVX_RS,PPC_0} },
  {   "evstwwe",EVX_FORM(4,821),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_UI_EA,EVX_RS,PPC_0} },
  {  "evstwwex",EVX_FORM(4,820),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_RB_EA,EVX_RS,PPC_0} },
  {   "evstwwo",EVX_FORM(4,829),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_UI_EA,EVX_RS,PPC_0} },
  {  "evstwwox",EVX_FORM(4,828),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RA_EA,EVX_RB_EA,EVX_RS,PPC_0} },
  {"evsubfsmiaaw",EVX_FORM(4,1227), EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  {"evsubfssiaaw",EVX_FORM(4,1219), EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  {"evsubfumiaaw",EVX_FORM(4,1226), EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  {"evsubfusiaaw",EVX_FORM(4,1218), EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,PPC_0} },
  {   "evsubfw",EVX_FORM(4,516),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {  "evsubifw",EVX_FORM(4,518),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_UI,EVX_RB,PPC_0} },
  {     "evxor",EVX_FORM(4,534),    EVX_MASK, PPC_CPU|PPC_SPE, {EVX_RT,EVX_RA,EVX_RB,PPC_0} },
  {    "extsb", X_FORM(31,954,0),     X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  {    "extsb.",X_FORM(31,954,1),     X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  {    "extsh", X_FORM(31,922,0),     X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  {    "extsh.",X_FORM(31,922,1),     X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  {    "extsw", X_FORM(31,986,0),     X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  {    "extsw.",X_FORM(31,986,1),     X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  {    "fabs",  X_FORM(63,264,0),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "fabs.", X_FORM(63,264,1),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "fadd",  A_FORM(63,21,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,PPC_0} },
  {    "fadd.", A_FORM(63,21,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,PPC_0} },
  {    "fadds", A_FORM(59,21,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,PPC_0} },
  {    "fadds.",A_FORM(59,21,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,PPC_0} },
  {    "fcfid", X_FORM(63,846,0),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "fcfid.",X_FORM(63,846,1),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "fcmpo", X_FORM(63,32,0),      X_MASK, PPC_FPU, {X_BF,X_FRA,X_FRB,PPC_0} },
  {    "fcmpo.",X_FORM(63,32,1),      X_MASK, PPC_FPU, {X_BF,X_FRA,X_FRB,PPC_0} },
  {    "fcmpu", X_FORM(63,0,0),       X_MASK, PPC_FPU, {X_BF,X_FRA,X_FRB,PPC_0} },
  {    "fcmpu.",X_FORM(63,0,1),       X_MASK, PPC_FPU, {X_BF,X_FRA,X_FRB,PPC_0} },
  {   "fcpsgn", X_FORM(63,8,0),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  {   "fcpsgn.",X_FORM(63,8,1),       X_MASK, PPC_FPU, {X_FRT,X_FRA,X_FRB,PPC_0} },
  {    "fctid", X_FORM(63,814,0),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "fctid.",X_FORM(63,814,1),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {   "fctidz", X_FORM(63,815,0),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {   "fctidz.",X_FORM(63,815,1),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "fctiw", X_FORM(63,14,0),      X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "fctiw.",X_FORM(63,14,1),      X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {   "fctiwz", X_FORM(63,15,0),      X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {   "fctiwz.",X_FORM(63,15,1),      X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "fdiv",  A_FORM(63,18,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,PPC_0} },
  {    "fdiv.", A_FORM(63,18,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,PPC_0} },
  {    "fdivs", A_FORM(59,18,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,PPC_0} },
  {    "fdivs.",A_FORM(59,18,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,PPC_0} },
  {    "fmadd", A_FORM(63,29,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {    "fmadd.",A_FORM(63,29,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {   "fmadds", A_FORM(59,29,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {   "fmadds.",A_FORM(59,29,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {      "fmr", X_FORM(63,72,0),      X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {      "fmr.",X_FORM(63,72,1),      X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "fmsub", A_FORM(63,28,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {    "fmsub.",A_FORM(63,28,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {   "fmsubs", A_FORM(59,28,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {   "fmsubs.",A_FORM(59,28,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {    "fmul",  A_FORM(63,25,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRC,PPC_0} },
  {    "fmul.", A_FORM(63,25,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRC,PPC_0} },
  {    "fmuls", A_FORM(59,25,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRC,PPC_0} },
  {    "fmuls.",A_FORM(59,25,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRC,PPC_0} },
  {   "fnabs",  X_FORM(63,136,0),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {   "fnabs.", X_FORM(63,136,1),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {     "fneg", X_FORM(63,40,0),      X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {     "fneg.",X_FORM(63,40,1),      X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {   "fnmadd", A_FORM(63,31,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {   "fnmadd.",A_FORM(63,31,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {  "fnmadds", A_FORM(59,31,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {  "fnmadds.",A_FORM(59,31,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {   "fnmsub", A_FORM(63,30,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {   "fnmsub.",A_FORM(63,30,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {  "fnmsubs", A_FORM(59,30,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {  "fnmsubs.",A_FORM(59,30,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {    "fre",   A_FORM(63,24,0),      A_MASK, PPC_FPU, {A_FRT,A_FRB,A_L,PPC_0} },
  {    "fre.",  A_FORM(63,24,1),      A_MASK, PPC_FPU, {A_FRT,A_FRB,A_L,PPC_0} },
  {    "fres",  A_FORM(59,24,0),      A_MASK, PPC_FPU, {A_FRT,A_FRB,A_L,PPC_0} },
  {    "fres.", A_FORM(59,24,1),      A_MASK, PPC_FPU, {A_FRT,A_FRB,A_L,PPC_0} },
  {    "frim",  X_FORM(63,488,0),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "frim.", X_FORM(63,488,1),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "frin",  X_FORM(63,392,0),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "frin.", X_FORM(63,392,1),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "frip",  X_FORM(63,456,0),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "frip.", X_FORM(63,456,1),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "friz",  X_FORM(63,424,0),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {    "friz.", X_FORM(63,424,1),     X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {     "frsp", X_FORM(63,12,0),      X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {     "frsp.",X_FORM(63,12,1),      X_MASK, PPC_FPU, {X_FRT,X_FRB,PPC_0} },
  {  "frsqrte", A_FORM(63,26,0),      A_MASK, PPC_FPU, {A_FRT,A_FRB,A_L,PPC_0} },
  {  "frsqrte.",A_FORM(63,26,1),      A_MASK, PPC_FPU, {A_FRT,A_FRB,A_L,PPC_0} },
  { "frsqrtes", A_FORM(59,26,0),      A_MASK, PPC_FPU, {A_FRT,A_FRB,A_L,PPC_0} },
  { "frsqrtes.",A_FORM(59,26,1),      A_MASK, PPC_FPU, {A_FRT,A_FRB,A_L,PPC_0} },
  {    "fsel",  A_FORM(63,23,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {    "fsel.", A_FORM(63,23,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,A_FRC,PPC_0} },
  {    "fsqrt", A_FORM(63,22,0),      A_MASK, PPC_FPU, {A_FRT,A_FRB,PPC_0} },
  {    "fsqrt.",A_FORM(63,22,1),      A_MASK, PPC_FPU, {A_FRT,A_FRB,PPC_0} },
  {   "fsqrts", A_FORM(59,22,0),      A_MASK, PPC_FPU, {A_FRT,A_FRB,PPC_0} },
  {   "fsqrts.",A_FORM(59,22,1),      A_MASK, PPC_FPU, {A_FRT,A_FRB,PPC_0} },
  {    "fsub",  A_FORM(63,20,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,PPC_0} },
  {    "fsub.", A_FORM(63,20,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,PPC_0} },
  {    "fsubs", A_FORM(59,20,0),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,PPC_0} },
  {    "fsubs.",A_FORM(59,20,1),      A_MASK, PPC_FPU, {A_FRT,A_FRA,A_FRB,PPC_0} },
  {   "hrfid", XL_FORM(19,274,0),    XL_MASK, PPC_CPU, {PPC_0} },
  {   "hrfid.",XL_FORM(19,274,1),    XL_MASK, PPC_CPU, {PPC_0} },
  {    "icbi",  X_FORM(31,982,0),     X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  {   "icbi.",  X_FORM(31,982,1),     X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  {  "icbiep",  X_FORM(31,991,0),     X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "icbiep.",  X_FORM(31,991,1),     X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  {   "icblc",  X_FORM(31,230,0),     X_MASK, PPC_CPU, {X_CT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "icblc.",  X_FORM(31,230,1),     X_MASK, PPC_CPU, {X_CT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "icbt",    X_FORM(31,22,0),      X_MASK, PPC_CPU, {X_CT,X_RA_EA,X_RB_EA,PPC_0} },
  { "icbt.",    X_FORM(31,22,1),      X_MASK, PPC_CPU, {X_CT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "icbtls",  X_FORM(31,486,0),     X_MASK, PPC_CPU, {X_CT,X_RA_EA,X_RB_EA,PPC_0} },
  { "icbtls.",  X_FORM(31,486,1),     X_MASK, PPC_CPU, {X_CT,X_RA_EA,X_RB_EA,PPC_0} },
  {     "ici",  X_FORM(31,966,0),     X_MASK, PPC_CPU, {X_CT,PPC_0} },
  {    "ici.",  X_FORM(31,966,1),     X_MASK, PPC_CPU, {X_CT,PPC_0} },
  {  "icread",  X_FORM(31,998,0),     X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "icread.",  X_FORM(31,998,1),     X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  {    "isel",  A_FORM(31,15,0),      A_MASK, PPC_CPU, {A_RT,A_RA,A_RB,A_BC,PPC_0} },
  {   "isel.",  A_FORM(31,15,1),      A_MASK, PPC_CPU, {A_RT,A_RA,A_RB,A_BC,PPC_0} },
  {   "isync", XL_FORM(19,150,0),    XL_MASK, PPC_CPU, {PPC_0} },
  {  "isync.", XL_FORM(19,150,1),    XL_MASK, PPC_CPU, {PPC_0} },
  {   "lbepx",  X_FORM(31,95,0),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lbepx.",  X_FORM(31,95,1),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {     "lbz",  D_FORM(34),           D_MASK, PPC_CPU, {D_RT,D_RA_EA,D_D_EA,PPC_0} },
  {  "lbzcix",  X_FORM(31,853,0),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  { "lbzcix.",  X_FORM(31,853,1),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lbzu",  D_FORM(35),           D_MASK, PPC_CPU, {D_RT,D_RA_EA,D_D_EA,PPC_0} },
  {   "lbzux",  X_FORM(31,119,0),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lbzux.",  X_FORM(31,119,1),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lbzx",  X_FORM(31,87,0),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lbzx.",  X_FORM(31,87,1),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {      "ld", DS_FORM(58,0),        DS_MASK, PPC_CPU, {DS_RT,DS_RA_EA,DS_DS_EA,PPC_0} },
  {   "ldarx",  X_FORM(31,84,0),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,X_EH,PPC_0} },
  {   "ldarx",  X_FORM(31,84,1),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,X_EH,PPC_0} },
  {   "ldcix",  X_FORM(31,885,0),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "ldcix.",  X_FORM(31,885,1),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "ldepx",  X_FORM(31,29,0),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "ldepx.",  X_FORM(31,29,1),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {     "ldu", DS_FORM(58,1),        DS_MASK, PPC_CPU, {DS_RT,DS_RA_EA,DS_DS_EA,PPC_0} },
  {    "ldux",  X_FORM(31,53,0),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "ldux.",  X_FORM(31,53,1),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {     "ldx",  X_FORM(31,21,0),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "ldx.",  X_FORM(31,21,1),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {     "lfd",  D_FORM(50),           D_MASK, PPC_FPU, {D_FRT,D_RA_EA,D_D_EA,PPC_0} },
  {  "lfdepx",  X_FORM(31,607,0),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lfdepx.", X_FORM(31,607,1),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lfdp", DS_FORM(57,0),        DS_MASK, PPC_FPU, {DS_FRT,DS_RA_EA,DS_DS_EA,PPC_0} },
  {   "lfdpx",  X_FORM(31,791,0),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lfdpx.", X_FORM(31,791,1),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lfdu",  D_FORM(51),           D_MASK, PPC_FPU, {D_FRT,D_RA_EA,D_D_EA,PPC_0} },
  {   "lfdux",  X_FORM(31,631,0),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lfdux.", X_FORM(31,631,1),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lfdx",  X_FORM(31,599,0),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lfdx.", X_FORM(31,599,1),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lfiwax",  X_FORM(31,855,0),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lfiwax.", X_FORM(31,855,1),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {     "lfs",  D_FORM(48),           D_MASK, PPC_FPU, {D_FRT,D_RA_EA,D_D_EA,PPC_0} },
  {    "lfsu",  D_FORM(49),           D_MASK, PPC_FPU, {D_FRT,D_RA_EA,D_D_EA,PPC_0} },
  {   "lfsux",  X_FORM(31,567,0),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lfsux.", X_FORM(31,567,1),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lfsx",  X_FORM(31,535,0),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lfsx.", X_FORM(31,535,1),     X_MASK, PPC_FPU, {X_FRT,X_RA_EA,X_RB_EA,PPC_0} },
  {     "lha",  D_FORM(42),           D_MASK, PPC_CPU, {D_RT,D_RA_EA,D_D_EA,PPC_0} },
  {    "lhau",  D_FORM(43),           D_MASK, PPC_CPU, {D_RT,D_RA_EA,D_D_EA,PPC_0} },
  {   "lhaux",  X_FORM(31,375,0),     X_MASK, PPC_FPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lhaux.", X_FORM(31,375,1),     X_MASK, PPC_FPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lhax",  X_FORM(31,343,0),     X_MASK, PPC_FPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lhax.", X_FORM(31,343,1),     X_MASK, PPC_FPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lhbrx",  X_FORM(31,790,0),     X_MASK, PPC_FPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lhbrx.", X_FORM(31,790,1),     X_MASK, PPC_FPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lhepx",  X_FORM(31,287,0),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lhepx.",  X_FORM(31,287,1),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {     "lhz",  D_FORM(40),           D_MASK, PPC_CPU, {D_RT,D_RA_EA,D_D_EA,PPC_0} },
  {  "lhzcix",  X_FORM(31,821,0),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  { "lhzcix.",  X_FORM(31,821,1),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lhzu",  D_FORM(41),           D_MASK, PPC_CPU, {D_RT,D_RA_EA,D_D_EA,PPC_0} },
  {   "lhzux",  X_FORM(31,311,0),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lhzux.",  X_FORM(31,311,1),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lhzx",  X_FORM(31,279,0),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lhzx.",  X_FORM(31,279,1),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lmw",   D_FORM(46),           D_MASK, PPC_CPU, {D_RT,D_RA_EA,D_D_EA,PPC_0} },
  {     "lq",  DQ_FORM(56),          DQ_MASK, PPC_CPU, {DQ_RT,DQ_RA_EA,DQ_DQ_EA,PPC_0} },
  {    "lswi",  X_FORM(31,597,0),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_NB,PPC_0} },
  {   "lswi.",  X_FORM(31,597,1),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_NB,PPC_0} },
  {    "lswx",  X_FORM(31,533,0),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lswx.",  X_FORM(31,533,1),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lvebx",  X_FORM(31,7,0),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lvebx.",  X_FORM(31,7,1),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lvehx",  X_FORM(31,39,0),      X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lvehx.",  X_FORM(31,39,1),      X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lvepx",  X_FORM(31,295,0),     X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lvepx.",  X_FORM(31,295,1),     X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lvepxl",  X_FORM(31,263,0),     X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  { "lvepxl.",  X_FORM(31,263,1),     X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lvewx",  X_FORM(31,71,0),      X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lvewx.",  X_FORM(31,71,1),      X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lvsl",   X_FORM(31,6,0),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lvsl.",   X_FORM(31,6,1),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lvsr",   X_FORM(31,38,0),      X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lvsr.",   X_FORM(31,38,1),      X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lvx",   X_FORM(31,103,0),     X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lvx.",   X_FORM(31,103,1),     X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lvxl",   X_FORM(31,359,0),     X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lvxl.",   X_FORM(31,359,1),     X_MASK, PPC_ALTIVEC|PPC_VEC, {X_VRT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lwa",  DS_FORM(58,2),        DS_MASK, PPC_CPU, {DS_RT,DS_RA_EA,DS_DS_EA,PPC_0} },
  {   "lwarx",  X_FORM(31,20,0),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,X_EH,PPC_0} },
  {   "lwarx.", X_FORM(31,20,1),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,X_EH,PPC_0} },
  {   "lwaux",  X_FORM(31,373,0),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lwaux.", X_FORM(31,373,1),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lwax",  X_FORM(31,341,0),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lwax.", X_FORM(31,341,1),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lwbrx",  X_FORM(31,534,0),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lwbrx.", X_FORM(31,534,1),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lwepx",  X_FORM(31,31,0),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lwepx.",  X_FORM(31,31,1),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {     "lwz",  D_FORM(32),           D_MASK, PPC_CPU, {D_RT,D_RA_EA,D_D_EA,PPC_0} },
  {  "lwzcix",  X_FORM(31,789,0),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  { "lwzcix.",  X_FORM(31,789,1),     X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lwzu",  D_FORM(33),           D_MASK, PPC_CPU, {D_RT,D_RA_EA,D_D_EA,PPC_0} },
  {   "lwzux",  X_FORM(31,55,0),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "lwzux.",  X_FORM(31,55,1),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {    "lwzx",  X_FORM(31,23,0),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {   "lwzx.",  X_FORM(31,23,1),      X_MASK, PPC_CPU, {X_RT,X_RA_EA,X_RB_EA,PPC_0} },
  {  "macchw", XO_FORM(4,172,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "macchw.", XO_FORM(4,172,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "macchwo", XO_FORM(4,172,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"macchwo.", XO_FORM(4,172,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "macchws", XO_FORM(4,236,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"macchws.", XO_FORM(4,236,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"macchwso", XO_FORM(4,236,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"macchwso.",XO_FORM(4,236,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "macchwsu",XO_FORM(4,204,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"macchwsu.",XO_FORM(4,204,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"macchwsuo",XO_FORM(4,204,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
 {"macchwsuo.",XO_FORM(4,204,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "macchwu", XO_FORM(4,140,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"macchwu.", XO_FORM(4,140,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"macchwuo", XO_FORM(4,140,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"macchwuo.",XO_FORM(4,140,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {  "machhw", XO_FORM(4,44,0,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "machhw.", XO_FORM(4,44,0,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "machhwo", XO_FORM(4,44,1,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"machhwo.", XO_FORM(4,44,1,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "machhws", XO_FORM(4,108,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"machhws.", XO_FORM(4,108,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"machhwso", XO_FORM(4,108,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"machhwso.",XO_FORM(4,108,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "machhwsu",XO_FORM(4,76,0,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"machhwsu.",XO_FORM(4,76,0,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"machhwsuo",XO_FORM(4,76,1,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
 {"machhwsuo.",XO_FORM(4,76,1,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "machhwu", XO_FORM(4,12,0,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"machhwu.", XO_FORM(4,12,0,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"machhwuo", XO_FORM(4,12,1,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"machhwuo.",XO_FORM(4,12,1,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {  "maclhw", XO_FORM(4,428,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "maclhw.", XO_FORM(4,428,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "maclhwo", XO_FORM(4,428,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"maclhwo.", XO_FORM(4,428,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "maclhws", XO_FORM(4,492,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"maclhws.", XO_FORM(4,492,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"maclhwso", XO_FORM(4,492,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"maclhwso.",XO_FORM(4,492,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "maclhwsu",XO_FORM(4,460,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"maclhwsu.",XO_FORM(4,460,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"maclhwsuo",XO_FORM(4,460,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
 {"maclhwsuo.",XO_FORM(4,460,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "maclhwu", XO_FORM(4,396,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"maclhwu.", XO_FORM(4,396,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"maclhwuo", XO_FORM(4,396,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"maclhwuo.",XO_FORM(4,396,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {  "mbar",    X_FORM(31,854,0),     X_MASK, PPC_CPU, {X_MO,PPC_0} },
  {  "mbar.",   X_FORM(31,854,1),     X_MASK, PPC_CPU, {X_MO,PPC_0} },
  {  "mcrf",   XL_FORM(19,0,0),      XL_MASK, PPC_CPU, {XL_BF,XL_BFA,PPC_0} },
  {  "mcrf.",  XL_FORM(19,0,1),      XL_MASK, PPC_CPU, {XL_BF,XL_BFA,PPC_0} },
  {  "mcrfs",   X_FORM(63,64,0),      X_MASK, PPC_FPU, {X_BF,X_BFA,PPC_0} },
  {  "mcrfs.",  X_FORM(63,64,1),      X_MASK, PPC_FPU, {X_BF,X_BFA,PPC_0} },
  {  "mcrxr",   X_FORM(31,512,0),     X_MASK, PPC_CPU, {X_BF,PPC_0} },
  {  "mcrxr.",  X_FORM(31,512,1),     X_MASK, PPC_CPU, {X_BF,PPC_0} },
  {  "mfapidi", X_FORM(31,275,0),     X_MASK, PPC_CPU, {X_RT,X_RA,PPC_0} },
  {  "mfapidi.",X_FORM(31,275,1),     X_MASK, PPC_CPU, {X_RT,X_RA,PPC_0} },
  {  "mfcr",  XFX_FORM(31,0,19,0),  XFX_MASK, PPC_CPU, {XFX_RT,PPC_0} },
  {  "mfcr.", XFX_FORM(31,0,19,1),  XFX_MASK, PPC_CPU, {XFX_RT,PPC_0} },
  {  "mfdcr", XFX_FORM(31,0,323,0), XFX_MASK, PPC_CPU, {XFX_RT,XFX_DCRN,PPC_0} },
  {  "mfdcr.",XFX_FORM(31,0,323,1), XFX_MASK, PPC_CPU, {XFX_RT,XFX_DCRN,PPC_0} },
  {  "mfdcr", XFX_FORM(31,1,323,0), XFX_MASK, PPC_CPU, {XFX_RT,XFX_DCRN,PPC_0} },
  {  "mfdcr.",XFX_FORM(31,1,323,1), XFX_MASK, PPC_CPU, {XFX_RT,XFX_DCRN,PPC_0} },
  {"mfdcrux",   X_FORM(31,291,0),     X_MASK, PPC_CPU, {X_RT,X_RA,PPC_0} },
  {"mfdcrux.",  X_FORM(31,291,1),     X_MASK, PPC_CPU, {X_RT,X_RA,PPC_0} },
  { "mfdcrx",   X_FORM(31,259,0),     X_MASK, PPC_CPU, {X_RT,X_RA,PPC_0} },
  { "mfdcrx.",  X_FORM(31,259,1),     X_MASK, PPC_CPU, {X_RT,X_RA,PPC_0} },
  { "mffs",     X_FORM(63,583,0),     X_MASK, PPC_FPU, {X_FRT,PPC_0} },
  { "mffs.",    X_FORM(63,583,1),     X_MASK, PPC_FPU, {X_FRT,PPC_0} },
  { "mfmsr",    X_FORM(31,83,0),      X_MASK, PPC_CPU, {X_RT,PPC_0} },
  { "mfmsr.",   X_FORM(31,83,1),      X_MASK, PPC_CPU, {X_RT,PPC_0} },
  { "mfocrf", XFX_FORM(31,1,19,0),  XFX_MASK, PPC_CPU, {XFX_RT,XFX_FXM,PPC_0} },
  { "mfocrf.",XFX_FORM(31,1,19,1),  XFX_MASK, PPC_CPU, {XFX_RT,XFX_FXM,PPC_0} },
  { "mfpmr",  XFX_FORM(31,0,334,0), XFX_MASK, PPC_CPU, {XFX_RT,XFX_PMRN,PPC_0} },
  { "mfpmr.", XFX_FORM(31,0,334,1), XFX_MASK, PPC_CPU, {XFX_RT,XFX_PMRN,PPC_0} },
  { "mfpmr",  XFX_FORM(31,1,334,0), XFX_MASK, PPC_CPU, {XFX_RT,XFX_PMRN,PPC_0} },
  { "mfpmr.", XFX_FORM(31,1,334,1), XFX_MASK, PPC_CPU, {XFX_RT,XFX_PMRN,PPC_0} },
  { "mfspr",  XFX_FORM(31,0,339,0), XFX_MASK, PPC_CPU, {XFX_RT,XFX_SPR,PPC_0} },
  { "mfspr.", XFX_FORM(31,0,339,1), XFX_MASK, PPC_CPU, {XFX_RT,XFX_SPR,PPC_0} },
  { "mfspr",  XFX_FORM(31,1,339,0), XFX_MASK, PPC_CPU, {XFX_RT,XFX_SPR,PPC_0} },
  { "mfspr.", XFX_FORM(31,1,339,1), XFX_MASK, PPC_CPU, {XFX_RT,XFX_SPR,PPC_0} },
  { "mfsr",     X_FORM(31,595,0),     X_MASK, PPC_CPU, {X_RT,X_SP,PPC_0} },
  { "mfsr.",    X_FORM(31,595,1),     X_MASK, PPC_CPU, {X_RT,X_SP,PPC_0} },
  { "mfsrin",   X_FORM(31,659,0),     X_MASK, PPC_CPU, {X_RT,X_RB,PPC_0} },
  { "mfsrin.",  X_FORM(31,659,1),     X_MASK, PPC_CPU, {X_RT,X_RB,PPC_0} },
  { "mftb",   XFX_FORM(31,0,371,0), XFX_MASK, PPC_CPU, {XFX_RT,XFX_TBR,PPC_0} },
  { "mftb.",  XFX_FORM(31,0,371,1), XFX_MASK, PPC_CPU, {XFX_RT,XFX_TBR,PPC_0} },
  { "mftb",   XFX_FORM(31,1,371,0), XFX_MASK, PPC_CPU, {XFX_RT,XFX_TBR,PPC_0} },
  { "mftb.",  XFX_FORM(31,1,371,1), XFX_MASK, PPC_CPU, {XFX_RT,XFX_TBR,PPC_0} },
  { "mfvcsr",  VX_FORM(4,1540),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,PPC_0} },
  { "msgclr",   X_FORM(31,238,0),     X_MASK, PPC_CPU, {X_RB,PPC_0} },
  { "msgclr.",  X_FORM(31,238,1),     X_MASK, PPC_CPU, {X_RB,PPC_0} },
  { "msgsnd",   X_FORM(31,206,0),     X_MASK, PPC_CPU, {X_RB,PPC_0} },
  { "msgsnd.",  X_FORM(31,206,1),     X_MASK, PPC_CPU, {X_RB,PPC_0} },
  {  "mtcrf", XFX_FORM(31,0,144,0), XFX_MASK, PPC_CPU, {XFX_FXM,XFX_RS,PPC_0} },
  {  "mtcrf.",XFX_FORM(31,0,144,1), XFX_MASK, PPC_CPU, {XFX_FXM,XFX_RS,PPC_0} },
  {  "mtdcr", XFX_FORM(31,0,451,0), XFX_MASK, PPC_CPU, {XFX_DCRN,XFX_RS,PPC_0} },
  {  "mtdcr.",XFX_FORM(31,0,451,1), XFX_MASK, PPC_CPU, {XFX_DCRN,XFX_RS,PPC_0} },
  {  "mtdcr", XFX_FORM(31,1,451,0), XFX_MASK, PPC_CPU, {XFX_DCRN,XFX_RS,PPC_0} },
  {  "mtdcr.",XFX_FORM(31,1,451,1), XFX_MASK, PPC_CPU, {XFX_DCRN,XFX_RS,PPC_0} },
  {"mtdcrux",   X_FORM(31,419,0),     X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  {"mtdcrux.",  X_FORM(31,419,1),     X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  { "mtdcrx",   X_FORM(31,387,0),     X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  { "mtdcrx.",  X_FORM(31,387,1),     X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  { "mtfsb0",   X_FORM(63,70,0),      X_MASK, PPC_FPU, {X_BT,PPC_0} },
  { "mtfsb0.",  X_FORM(63,70,1),      X_MASK, PPC_FPU, {X_BT,PPC_0} },
  { "mtfsb1",   X_FORM(63,38,0),      X_MASK, PPC_FPU, {X_BT,PPC_0} },
  { "mtfsb1.",  X_FORM(63,38,1),      X_MASK, PPC_FPU, {X_BT,PPC_0} },
  { "mtfsf",    X_FORM(63,711,0),     X_MASK, PPC_FPU, {X_FLM,X_FRB,X_L3,X_W,PPC_0} },
  { "mtfsf.",   X_FORM(63,711,1),     X_MASK, PPC_FPU, {X_FLM,X_FRB,X_L3,X_W,PPC_0} },
  { "mtfsfi",   X_FORM(63,134,0),     X_MASK, PPC_FPU, {X_BF,X_U,X_W,PPC_0} },
  { "mtfsfi.",  X_FORM(63,134,1),     X_MASK, PPC_FPU, {X_BF,X_U,X_W,PPC_0} },
  { "mtmsr",    X_FORM(31,146,0),     X_MASK, PPC_CPU, {X_RS,X_L4,PPC_0} },
  { "mtmsr.",   X_FORM(31,146,1),     X_MASK, PPC_CPU, {X_RS,X_L4,PPC_0} },
  { "mtmsrd",   X_FORM(31,178,0),     X_MASK, PPC_CPU, {X_RS,X_L4,PPC_0} },
  { "mtmsrd.",  X_FORM(31,178,1),     X_MASK, PPC_CPU, {X_RS,X_L4,PPC_0} },
  { "mtocrf", XFX_FORM(31,1,144,0), XFX_MASK, PPC_CPU, {XFX_FXM,XFX_RS,PPC_0} },
  { "mtocrf.",XFX_FORM(31,1,144,1), XFX_MASK, PPC_CPU, {XFX_FXM,XFX_RS,PPC_0} },
  { "mtpmr",  XFX_FORM(31,0,462,0),  XFX_MASK, PPC_CPU, {XFX_PMRN,XFX_RS,PPC_0} },
  { "mtpmr.", XFX_FORM(31,0,462,1),  XFX_MASK, PPC_CPU, {XFX_PMRN,XFX_RS,PPC_0} },
  { "mtpmr",  XFX_FORM(31,1,462,0),  XFX_MASK, PPC_CPU, {XFX_PMRN,XFX_RS,PPC_0} },
  { "mtpmr.", XFX_FORM(31,1,462,1),  XFX_MASK, PPC_CPU, {XFX_PMRN,XFX_RS,PPC_0} },
  { "mtspr",  XFX_FORM(31,0,467,0),  XFX_MASK, PPC_CPU, {XFX_RS,XFX_SPR,PPC_0} },
  { "mtspr.", XFX_FORM(31,0,467,1),  XFX_MASK, PPC_CPU, {XFX_RS,XFX_SPR,PPC_0} },
  { "mtspr",  XFX_FORM(31,1,467,0),  XFX_MASK, PPC_CPU, {XFX_RS,XFX_SPR,PPC_0} },
  { "mtspr.", XFX_FORM(31,1,467,1),  XFX_MASK, PPC_CPU, {XFX_RS,XFX_SPR,PPC_0} },
  { "mtsr",     X_FORM(31,210,0),      X_MASK, PPC_CPU, {X_SP,X_RS,PPC_0} },
  { "mtsr.",    X_FORM(31,210,1),      X_MASK, PPC_CPU, {X_SP,X_RS,PPC_0} },
  { "mtsrin",   X_FORM(31,242,0),      X_MASK, PPC_CPU, {X_RS,X_RB,PPC_0} },
  { "mtsrin.",  X_FORM(31,242,1),      X_MASK, PPC_CPU, {X_RS,X_RB,PPC_0} },
  { "mtvcsr",  VX_FORM(4,1604),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRB,PPC_0} },
  { "mulchw",   X_FORM(4,168,0),       X_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "mulchw.",  X_FORM(4,168,1),       X_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "mulchwu",  X_FORM(4,136,0),       X_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "mulchwu.", X_FORM(4,136,1),       X_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "mulhd",   XO_FORM(31,73,0,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhd.",  XO_FORM(31,73,0,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhd",   XO_FORM(31,73,1,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhd.",  XO_FORM(31,73,1,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhdu",  XO_FORM(31,9,0,0),     XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhdu.", XO_FORM(31,9,0,1),     XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhdu",  XO_FORM(31,9,1,0),     XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhdu.", XO_FORM(31,9,1,1),     XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhhw",   X_FORM(4,40,0),        X_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "mulhhw.",  X_FORM(4,40,1),        X_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "mulhhwu",  X_FORM(4,8,0),         X_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "mulhhwu.", X_FORM(4,8,1),         X_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "mulhw",   XO_FORM(31,75,0,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhw.",  XO_FORM(31,75,0,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhw",   XO_FORM(31,75,1,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhw.",  XO_FORM(31,75,1,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhwu",  XO_FORM(31,11,0,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhwu.", XO_FORM(31,11,0,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhwu",  XO_FORM(31,11,1,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulhwu.", XO_FORM(31,11,1,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulld",  XO_FORM(31,233,0,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulld.", XO_FORM(31,233,0,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulldo", XO_FORM(31,233,1,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mulldo.",XO_FORM(31,233,1,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mullhw",   X_FORM(4,424,0),       X_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "mullhw.",  X_FORM(4,424,1),       X_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "mullhwu",  X_FORM(4,392,0),       X_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "mullhwu.", X_FORM(4,392,1),       X_MASK, PPC_CPU, {X_RT,X_RA,X_RB,PPC_0} },
  { "mulli",    D_FORM(7),             D_MASK, PPC_CPU, {D_RT,D_RA,D_SI,PPC_0} },
  { "mullw",   XO_FORM(31,235,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mullw." , XO_FORM(31,235,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mullwo" , XO_FORM(31,235,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "mullwo.", XO_FORM(31,235,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "nand",     X_FORM(31,476,0),      X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  { "nand.",    X_FORM(31,476,1),      X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  { "nap",     XL_FORM(19,434,0),     XL_MASK, PPC_CPU, {PPC_0} },
  { "nap",     XL_FORM(19,434,1),     XL_MASK, PPC_CPU, {PPC_0} },
  { "neg",     XO_FORM(31,104,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "neg.",    XO_FORM(31,104,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "nego",    XO_FORM(31,104,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "nego.",   XO_FORM(31,104,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "nmacchw", XO_FORM(4,174,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmacchw.", XO_FORM(4,174,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmacchwo", XO_FORM(4,174,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmacchwo.",XO_FORM(4,174,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmacchws", XO_FORM(4,238,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmacchws.",XO_FORM(4,238,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmacchwso",XO_FORM(4,238,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
 {"nmacchwso.",XO_FORM(4,238,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "nmachhw", XO_FORM(4,46,0,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmachhw.", XO_FORM(4,46,0,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmachhwo", XO_FORM(4,46,1,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmachhwo.",XO_FORM(4,46,1,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmachhws", XO_FORM(4,110,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmachhws.",XO_FORM(4,110,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmachhwso",XO_FORM(4,110,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
 {"nmachhwso.",XO_FORM(4,110,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "nmaclhw", XO_FORM(4,430,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmaclhw.", XO_FORM(4,430,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmaclhwo", XO_FORM(4,430,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmaclhwo.",XO_FORM(4,430,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmaclhws", XO_FORM(4,494,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmaclhws.",XO_FORM(4,494,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {"nmaclhwso",XO_FORM(4,494,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
 {"nmaclhwso.",XO_FORM(4,494,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  {  "nor",   X_FORM(31,124,0),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {  "nor.",  X_FORM(31,124,1),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {   "or",   X_FORM(31,444,0),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {   "or.",  X_FORM(31,444,1),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {  "orc",   X_FORM(31,412,0),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {  "orc.",  X_FORM(31,412,1),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {   "ori",  D_FORM(24),             D_MASK, PPC_CPU, {D_RA,D_RS,D_UI,PPC_0} },
  {   "oris", D_FORM(25),             D_MASK, PPC_CPU, {D_RA,D_RS,D_UI,PPC_0} },
  {"popcntb", X_FORM(31,122,0),       X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  {"popcntb.",X_FORM(31,122,1),       X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  {  "prtyd", X_FORM(31,186,0),       X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  {  "prtyd.",X_FORM(31,186,1),       X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  {  "prtyw", X_FORM(31,154,0),       X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  {  "prtyw.",X_FORM(31,154,1),       X_MASK, PPC_CPU, {X_RA,X_RS,PPC_0} },
  {  "rfci", XL_FORM(19,51,0),       XL_MASK, PPC_CPU, {PPC_0} },
  {  "rfci.",XL_FORM(19,51,1),       XL_MASK, PPC_CPU, {PPC_0} },
  {  "rfdi", XL_FORM(19,39,0),       XL_MASK, PPC_CPU, {PPC_0} },
  {  "rfdi.",XL_FORM(19,39,1),       XL_MASK, PPC_CPU, {PPC_0} },
  {  "rfi",  XL_FORM(19,50,0),       XL_MASK, PPC_CPU, {PPC_0} },
  {  "rfi." ,XL_FORM(19,50,1),       XL_MASK, PPC_CPU, {PPC_0} },
  {  "rfid", XL_FORM(19,18,0),       XL_MASK, PPC_CPU, {PPC_0} },
  {  "rfid.",XL_FORM(19,18,1),       XL_MASK, PPC_CPU, {PPC_0} },
  { "rfmci", XL_FORM(19,38,0),       XL_MASK, PPC_CPU, {PPC_0} },
  { "rfmci.",XL_FORM(19,38,1),       XL_MASK, PPC_CPU, {PPC_0} },
  {"rldcl", MDS_FORM(30,8,0),       MDS_MASK, PPC_CPU, {MDS_RA,MDS_RS,MDS_RB,MDS_MB,PPC_0} },
  {"rldcl.",MDS_FORM(30,8,1),       MDS_MASK, PPC_CPU, {MDS_RA,MDS_RS,MDS_RB,MDS_MB,PPC_0} },
  {"rldcr", MDS_FORM(30,9,0),       MDS_MASK, PPC_CPU, {MDS_RA,MDS_RS,MDS_RB,MDS_MB,PPC_0} },
  {"rldcr.",MDS_FORM(30,9,1),       MDS_MASK, PPC_CPU, {MDS_RA,MDS_RS,MDS_RB,MDS_MB,PPC_0} },
  {"rldic",  MD_FORM(30,2,0),        MD_MASK, PPC_CPU, {MD_RA,MD_RS,MD_SHH,MD_SH,MD_MB,PPC_0} },
  {"rldic.", MD_FORM(30,2,1),        MD_MASK, PPC_CPU, {MD_RA,MD_RS,MD_SHH,MD_SH,MD_MB,PPC_0} },
  {"rldicl", MD_FORM(30,0,0),        MD_MASK, PPC_CPU, {MD_RA,MD_RS,MD_SHH,MD_SH,MD_MB,PPC_0} },
  {"rldicl.",MD_FORM(30,0,1),        MD_MASK, PPC_CPU, {MD_RA,MD_RS,MD_SHH,MD_SH,MD_MB,PPC_0} },
  {"rldicr", MD_FORM(30,1,0),        MD_MASK, PPC_CPU, {MD_RA,MD_RS,MD_SHH,MD_SH,MD_MB,PPC_0} },
  {"rldicr.",MD_FORM(30,1,1),        MD_MASK, PPC_CPU, {MD_RA,MD_RS,MD_SHH,MD_SH,MD_MB,PPC_0} },
  {"rldimi", MD_FORM(30,3,0),        MD_MASK, PPC_CPU, {MD_RA,MD_RS,MD_SHH,MD_SH,MD_MB,PPC_0} },
  {"rldimi.",MD_FORM(30,3,1),        MD_MASK, PPC_CPU, {MD_RA,MD_RS,MD_SHH,MD_SH,MD_MB,PPC_0} },
  {"rlwimi",  M_FORM(20,0),           M_MASK, PPC_CPU, {M_RA,M_RS,M_SH,M_MB,M_ME,PPC_0} },
  {"rlwimi.", M_FORM(20,1),           M_MASK, PPC_CPU, {M_RA,M_RS,M_SH,M_MB,M_ME,PPC_0} },
  {"rlwinm",  M_FORM(21,0),           M_MASK, PPC_CPU, {M_RA,M_RS,M_SH,M_MB,M_ME,PPC_0} },
  {"rlwinm.", M_FORM(21,1),           M_MASK, PPC_CPU, {M_RA,M_RS,M_SH,M_MB,M_ME,PPC_0} },
  {"rlwnm",   M_FORM(23,0),           M_MASK, PPC_CPU, {M_RA,M_RS,M_SH,M_MB,M_ME,PPC_0} },
  {"rlwnm.",  M_FORM(23,1),           M_MASK, PPC_CPU, {M_RA,M_RS,M_SH,M_MB,M_ME,PPC_0} },
{"rvwinkle", XL_FORM(19,498,0),      XL_MASK, PPC_CPU, {PPC_0} },
{"rvwinkle.",XL_FORM(19,498,1),      XL_MASK, PPC_CPU, {PPC_0} },
  {   "sc",  SC_FORM(17,1),          SC_MASK, PPC_CPU, {SC_LEV,PPC_0} },
  { "slbfee", X_FORM(31,979,0),       X_MASK, PPC_CPU, {X_RT,X_RB,PPC_0} },
  { "slbfee.",X_FORM(31,979,1),       X_MASK, PPC_CPU, {X_RT,X_RB,PPC_0} },
  { "slbia",  X_FORM(31,498,0),       X_MASK, PPC_CPU, {X_IH,PPC_0} },
  { "slbia.", X_FORM(31,498,1),       X_MASK, PPC_CPU, {X_IH,PPC_0} },
  { "slbie",  X_FORM(31,434,0),       X_MASK, PPC_CPU, {X_RB,PPC_0} },
  { "slbie.", X_FORM(31,434,1),       X_MASK, PPC_CPU, {X_RB,PPC_0} },
  {"slbmfee", X_FORM(31,915,0),       X_MASK, PPC_CPU, {X_RT,X_RB,PPC_0} },
  {"slbmfee.",X_FORM(31,915,1),       X_MASK, PPC_CPU, {X_RT,X_RB,PPC_0} },
  {"slbmfev", X_FORM(31,851,0),       X_MASK, PPC_CPU, {X_RT,X_RB,PPC_0} },
  {"slbmfev.",X_FORM(31,851,1),       X_MASK, PPC_CPU, {X_RT,X_RB,PPC_0} },
  { "slbmte", X_FORM(31,402,0),       X_MASK, PPC_CPU, {X_RS,X_RB,PPC_0} },
  { "slbmte.",X_FORM(31,402,1),       X_MASK, PPC_CPU, {X_RS,X_RB,PPC_0} },
  {   "sld",  X_FORM(31,27,0),        X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {   "sld.", X_FORM(31,27,1),        X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {"sleep",  XL_FORM(19,466,0),      XL_MASK, PPC_CPU, {PPC_0} },
  {"sleep.", XL_FORM(19,466,1),      XL_MASK, PPC_CPU, {PPC_0} },
  {   "slw",  X_FORM(31,24,0),        X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {   "slw.", X_FORM(31,24,1),        X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {  "srad",  X_FORM(31,794,0),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {  "srad.", X_FORM(31,794,1),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {  "sradi", X_FORM(31,413,0),       X_MASK, PPC_CPU, {X_RA,X_RS,X_SH,PPC_0} },
  {  "sradi.",X_FORM(31,413,1),       X_MASK, PPC_CPU, {X_RA,X_RS,X_SH,PPC_0} },
  {  "sraw",  X_FORM(31,792,0),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {  "sraw.", X_FORM(31,792,1),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {  "srawi", X_FORM(31,824,0),       X_MASK, PPC_CPU, {X_RA,X_RS,X_SH,PPC_0} },
  {  "srawi.",X_FORM(31,824,1),       X_MASK, PPC_CPU, {X_RA,X_RS,X_SH,PPC_0} },
  {   "srd",  X_FORM(31,539,0),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {   "srd.", X_FORM(31,539,1),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {   "srw",  X_FORM(31,536,0),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {   "srw.", X_FORM(31,536,1),       X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {   "stb",  D_FORM(38),             D_MASK, PPC_CPU, {D_RA_EA,D_D_EA,D_RS,PPC_0} },
  {  "stbcix",X_FORM(31,981,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "stbcix",X_FORM(31,981,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "stbepx",X_FORM(31,223,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "stbepx",X_FORM(31,223,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stbu", D_FORM(39),             D_MASK, PPC_CPU, {D_RA_EA,D_D_EA,D_RS,PPC_0} },
  {   "stbux",X_FORM(31,247,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stbux",X_FORM(31,247,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stbx", X_FORM(31,215,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stbx", X_FORM(31,215,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "std", DS_FORM(62,0),          DS_MASK, PPC_CPU, {DS_RA_EA,DS_DS_EA,DS_RS,PPC_0} },
  {  "stdcix",X_FORM(31,1013,0),      X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "stdcix",X_FORM(31,1013,1),      X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "stdcx", X_FORM(31,214,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "stdcx.",X_FORM(31,214,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "stdepx",X_FORM(31,157,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "stdepx",X_FORM(31,157,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stdu",DS_FORM(62,1),          DS_MASK, PPC_CPU, {DS_RA_EA,DS_DS_EA,DS_RS,PPC_0} },
  {   "stdux",X_FORM(31,181,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stdux",X_FORM(31,181,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stdx", X_FORM(31,149,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stdx", X_FORM(31,149,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stfd", D_FORM(54),             D_MASK, PPC_FPU, {D_RA_EA,D_D_EA,D_FRS,PPC_0} },
  {"stfdepx", X_FORM(31,735,0),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  {"stfdepx.",X_FORM(31,735,1),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  { "stfdp", DS_FORM(61,0),          DS_MASK, PPC_FPU, {DS_RA_EA,DS_DS_EA,DS_FRS,PPC_0} },
  { "stfdpx", X_FORM(31,919,0),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  { "stfdpx.",X_FORM(31,919,1),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  {  "stfdu", D_FORM(55),             D_MASK, PPC_FPU, {D_RA_EA,D_D_EA,D_FRS,PPC_0} },
  {  "stfdux",X_FORM(31,759,0),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  {  "stfdux",X_FORM(31,759,1),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  {  "stfdx", X_FORM(31,727,0),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  {  "stfdx", X_FORM(31,727,1),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  {  "stfiwx",X_FORM(31,983,0),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  {  "stfiwx",X_FORM(31,983,1),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  {   "stfs", D_FORM(52),             D_MASK, PPC_FPU, {D_RA_EA,D_D_EA,D_FRS,PPC_0} },
  {  "stfsu", D_FORM(53),             D_MASK, PPC_FPU, {D_RA_EA,D_D_EA,D_FRS,PPC_0} },
  {  "stfsux",X_FORM(31,695,0),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  {  "stfsux",X_FORM(31,695,1),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  {  "stfsx", X_FORM(31,663,0),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  {  "stfsx", X_FORM(31,663,1),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_FRS,PPC_0} },
  {   "sth",  D_FORM(44),             D_MASK, PPC_CPU, {D_RA_EA,D_D_EA,D_RS,PPC_0} },
  {"sthbrx",  X_FORM(31,918,0),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {"sthbrx.", X_FORM(31,918,1),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "sthcix",X_FORM(31,949,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "sthcix",X_FORM(31,949,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "sthepx",X_FORM(31,415,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "sthepx",X_FORM(31,415,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "sthu", D_FORM(45),             D_MASK, PPC_CPU, {D_RA_EA,D_D_EA,D_RS,PPC_0} },
  {   "sthux",X_FORM(31,439,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "sthux",X_FORM(31,439,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "sthx", X_FORM(31,407,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "sthx", X_FORM(31,407,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stmw", D_FORM(47),             D_MASK, PPC_CPU, {D_RA_EA,D_D_EA,D_RS,PPC_0} },
  {   "stq", DS_FORM(62,2),          DS_MASK, PPC_CPU, {DS_RA_EA,DS_DS_EA,DS_RS,PPC_0} },
  {   "stswi",X_FORM(31,725,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stswi",X_FORM(31,725,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stswx",X_FORM(31,661,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stswx",X_FORM(31,661,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "stvebx",X_FORM(31,135,0),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  {  "stvebx",X_FORM(31,135,1),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  {  "stvehx",X_FORM(31,167,0),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  {  "stvehx",X_FORM(31,167,1),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  {  "stvepx",X_FORM(31,807,0),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  {  "stvepx",X_FORM(31,807,1),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  { "stvepxl",X_FORM(31,775,0),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  { "stvepxl",X_FORM(31,775,1),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  {  "stvewx",X_FORM(31,199,0),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  {  "stvewx",X_FORM(31,199,1),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  {  "stvx",  X_FORM(31,231,0),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  {  "stvx",  X_FORM(31,231,1),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  {  "stvxl", X_FORM(31,487,0),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  {  "stvxl", X_FORM(31,487,1),       X_MASK, PPC_ALTIVEC|PPC_VEC, {X_RA_EA,X_RB_EA,X_VRS,PPC_0} },
  {   "stw",  D_FORM(36),             D_MASK, PPC_CPU, {D_RA_EA,D_D_EA,D_RS,PPC_0} },
  {"stwbrx",  X_FORM(31,662,0),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {"stwbrx.", X_FORM(31,662,1),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "stwcix",X_FORM(31,917,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "stwcix",X_FORM(31,917,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  { "stwcx",  X_FORM(31,150,0),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  { "stwcx.", X_FORM(31,150,1),       X_MASK, PPC_FPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "stwepx",X_FORM(31,159,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {  "stwepx",X_FORM(31,159,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stwu", D_FORM(37),             D_MASK, PPC_CPU, {D_RA_EA,D_D_EA,D_RS,PPC_0} },
  {   "stwux",X_FORM(31,183,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stwux",X_FORM(31,183,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stwx", X_FORM(31,151,0),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  {   "stwx", X_FORM(31,151,1),       X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,X_RS,PPC_0} },
  { "subf",    XO_FORM(31,40,0,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "subf.",   XO_FORM(31,40,0,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "subfo",   XO_FORM(31,40,1,0),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "subfo.",  XO_FORM(31,40,1,1),   XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "subfc",   XO_FORM(31,8,0,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "subfc.",  XO_FORM(31,8,0,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "subfco",  XO_FORM(31,8,1,0),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "subfco.", XO_FORM(31,8,1,1),    XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "subfe",   XO_FORM(31,136,0,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "subfe.",  XO_FORM(31,136,0,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "subfeo",  XO_FORM(31,136,1,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "subfeo.", XO_FORM(31,136,1,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,XO_RB,PPC_0} },
  { "subfic",  D_FORM(8),             D_MASK, PPC_CPU, {D_RT,D_RA,D_SI,PPC_0} },
  { "subfme",  XO_FORM(31,232,0,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "subfme.", XO_FORM(31,232,0,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "subfmeo", XO_FORM(31,232,1,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "subfmeo.",XO_FORM(31,232,1,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "subfze",  XO_FORM(31,200,0,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "subfze.", XO_FORM(31,200,0,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "subfzeo", XO_FORM(31,200,1,0),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  { "subfzeo.",XO_FORM(31,200,1,1),  XO_MASK, PPC_CPU, {XO_RT,XO_RA,PPC_0} },
  {  "sync",    X_FORM(31,598,0),     X_MASK, PPC_CPU, {X_L2,PPC_0} },
  {  "sync",    X_FORM(31,598,1),     X_MASK, PPC_CPU, {X_L2,PPC_0} },
  {  "td",      X_FORM(31,68,0),      X_MASK, PPC_CPU, {X_TO,X_RA,X_RB,PPC_0} },
  {  "td",      X_FORM(31,68,1),      X_MASK, PPC_CPU, {X_TO,X_RA,X_RB,PPC_0} },
  {  "tdi",     D_FORM(2),            D_MASK, PPC_CPU, {D_TO,D_RA,D_SI,PPC_0} },
  {  "tlbia",   X_FORM(31,370,0),     X_MASK, PPC_CPU, {PPC_0} },
  {  "tlbia",   X_FORM(31,370,1),     X_MASK, PPC_CPU, {PPC_0} },
  {  "tlbie",   X_FORM(31,306,0),     X_MASK, PPC_CPU, {X_RB,X_L,PPC_0} },
  {  "tlbie",   X_FORM(31,306,1),     X_MASK, PPC_CPU, {X_RB,X_L,PPC_0} },
  {  "tlbiel",  X_FORM(31,274,0),     X_MASK, PPC_CPU, {X_RB,X_L,PPC_0} },
  {  "tlbiel",  X_FORM(31,274,1),     X_MASK, PPC_CPU, {X_RB,X_L,PPC_0} },
  { "tlbivax",  X_FORM(31,786,0),     X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  { "tlbivax",  X_FORM(31,786,1),     X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  {  "tlbre",   X_FORM(31,946,0),     X_MASK, PPC_CPU, {PPC_0} },
  {  "tlbre",   X_FORM(31,946,1),     X_MASK, PPC_CPU, {PPC_0} },
  {  "tlbsx",   X_FORM(31,914,0),     X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  {  "tlbsx",   X_FORM(31,914,1),     X_MASK, PPC_CPU, {X_RA_EA,X_RB_EA,PPC_0} },
  {  "tlbsync", X_FORM(31,566,0),     X_MASK, PPC_CPU, {PPC_0} },
  {  "tlbsync", X_FORM(31,566,1),     X_MASK, PPC_CPU, {PPC_0} },
  {  "tlbwe",   X_FORM(31,978,0),     X_MASK, PPC_CPU, {PPC_0} },
  {  "tlbwe",   X_FORM(31,978,1),     X_MASK, PPC_CPU, {PPC_0} },
  {  "tw",      X_FORM(31,4,0),       X_MASK, PPC_CPU, {X_TO,X_RA,X_RB,PPC_0} },
  {  "tw",      X_FORM(31,4,1),       X_MASK, PPC_CPU, {X_TO,X_RA,X_RB,PPC_0} },
  {  "twi",     D_FORM(3),            D_MASK, PPC_CPU, {D_TO,D_RA,D_SI,PPC_0} },
/* FreeScale AltiVec */
  {  "vaddcuw",VX_FORM(4,384),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vaddfp", VX_FORM(4,10),        VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vaddsbs",VX_FORM(4,768),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vaddshs",VX_FORM(4,832),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vaddsws",VX_FORM(4,896),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vaddubm",VX_FORM(4,0),         VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vaddubs",VX_FORM(4,512),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vadduhm",VX_FORM(4,64),        VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vadduhs",VX_FORM(4,576),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vadduwm",VX_FORM(4,128),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vadduws",VX_FORM(4,640),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vand",   VX_FORM(4,1028),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vandc",  VX_FORM(4,1092),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vavgsb", VX_FORM(4,1282),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vavgsh", VX_FORM(4,1346),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vavgsw", VX_FORM(4,1410),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vavgub", VX_FORM(4,1026),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vavguh", VX_FORM(4,1090),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vavguw", VX_FORM(4,1154),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vcfsx",  VX_FORM(4,842),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,VX_UIM,PPC_0} },
  {  "vcfux",  VX_FORM(4,778),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,VX_UIM,PPC_0} },
  { "vcmpbfp", VC_FORM(4,0,966),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  { "vcmpbfp.",VC_FORM(4,1,966),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpeqfp", VC_FORM(4,0,198),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpeqfp.",VC_FORM(4,1,198),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpequb", VC_FORM(4,0,6),       VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpequb.",VC_FORM(4,1,6),       VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpequh", VC_FORM(4,0,70),      VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpequh.",VC_FORM(4,1,70),      VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpequw", VC_FORM(4,0,134),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpequw.",VC_FORM(4,1,134),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgefp", VC_FORM(4,0,454),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgefp.",VC_FORM(4,1,454),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtfp", VC_FORM(4,0,710),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtfp.",VC_FORM(4,1,710),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtsb", VC_FORM(4,0,774),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtsb.",VC_FORM(4,1,774),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtsh", VC_FORM(4,0,838),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtsh.",VC_FORM(4,1,838),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtsw", VC_FORM(4,0,902),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtsw.",VC_FORM(4,1,902),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtub", VC_FORM(4,0,518),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtub.",VC_FORM(4,1,518),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtuh", VC_FORM(4,0,582),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtuh.",VC_FORM(4,1,582),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtuw", VC_FORM(4,0,646),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  {"vcmpgtuw.",VC_FORM(4,1,646),     VC_MASK, PPC_ALTIVEC|PPC_VEC, {VC_VRT,VC_VRA,VC_VRB,PPC_0} },
  { "vctsxs",  VX_FORM(4,970),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,VX_UIM,PPC_0} },
  { "vctuxs",  VX_FORM(4,906),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,VX_UIM,PPC_0} },
  { "vexptefp",VX_FORM(4,394),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  { "vlogefp", VX_FORM(4,458),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  { "vmaddfp", VA_FORM(4,46),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRC,VA_VRB,PPC_0} },
  { "vmaxfp",  VX_FORM(4,1034),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmaxsb",  VX_FORM(4,258),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmaxsh",  VX_FORM(4,322),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmaxsw",  VX_FORM(4,386),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmaxub",  VX_FORM(4,2),         VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmaxuh",  VX_FORM(4,66),        VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmaxuw",  VX_FORM(4,130),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {"vmhaddshs",VA_FORM(4,32),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRB,VA_VRC,PPC_0} },
 {"vmhraddshs",VA_FORM(4,33),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRB,VA_VRC,PPC_0} },
  { "vminfp",  VX_FORM(4,1098),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vminsb",  VX_FORM(4,770),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vminsh",  VX_FORM(4,834),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vminsw",  VX_FORM(4,898),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vminub",  VX_FORM(4,540),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vminuh",  VX_FORM(4,578),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vminuw",  VX_FORM(4,642),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {"vmladduhm",VA_FORM(4,34),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRB,VA_VRC,PPC_0} },
  { "vmrghb",  VX_FORM(4,12),        VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmrghh",  VX_FORM(4,76),        VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmrghw",  VX_FORM(4,140),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmrglb",  VX_FORM(4,268),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmrglh",  VX_FORM(4,332),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmrglw",  VX_FORM(4,396),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {"vmsummbm", VA_FORM(4,37),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRB,VA_VRC,PPC_0} },
  {"vmsumshm", VA_FORM(4,40),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRB,VA_VRC,PPC_0} },
  {"vmsumshs", VA_FORM(4,41),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRB,VA_VRC,PPC_0} },
  {"vmsumubm", VA_FORM(4,36),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRB,VA_VRC,PPC_0} },
  {"vmsumuhm", VA_FORM(4,38),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRB,VA_VRC,PPC_0} },
  {"vmsumuhs", VA_FORM(4,39),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRB,VA_VRC,PPC_0} },
  { "vmulesb", VX_FORM(4,776),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmulesh", VX_FORM(4,840),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmuleub", VX_FORM(4,520),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmuleuh", VX_FORM(4,584),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmulosb", VX_FORM(4,264),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmulosh", VX_FORM(4,328),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmuloub", VX_FORM(4,8),         VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vmulouh", VX_FORM(4,72),        VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vnmsubfp",VA_FORM(4,47),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRC,VA_VRB,PPC_0} },
  {  "vnor",   VX_FORM(4,1284),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vor",    VX_FORM(4,1156),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vperm",  VA_FORM(4,43),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRC,VA_VRB,PPC_0} },
  {  "vpkpx",  VX_FORM(4,782),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vpkshss",VX_FORM(4,398),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vpkshus",VX_FORM(4,270),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vpkswss",VX_FORM(4,462),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vpkswus",VX_FORM(4,334),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vpkuhum",VX_FORM(4,14),        VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vpkuhus",VX_FORM(4,142),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vpkuwum",VX_FORM(4,78),        VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vpkuwus",VX_FORM(4,206),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vrefp",  VX_FORM(4,266),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  {  "vrfim",  VX_FORM(4,714),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  {  "vrfin",  VX_FORM(4,522),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  {  "vrfip",  VX_FORM(4,650),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  {  "vrfiz",  VX_FORM(4,586),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  {  "vrlb",   VX_FORM(4,4),         VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vrlh",   VX_FORM(4,68),        VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vrlw",   VX_FORM(4,132),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {"vrsqrtefp",VX_FORM(4,330),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  {  "vsel",   VA_FORM(4,42),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRC,VA_VRB,PPC_0} },
  {  "vsl",    VX_FORM(4,452),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vslb",   VX_FORM(4,260),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsldoi", VA_FORM(4,44),        VA_MASK, PPC_ALTIVEC|PPC_VEC, {VA_VRT,VA_VRA,VA_VRB,VA_SHB,PPC_0} },
  {  "vslh",   VX_FORM(4,324),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vslo",   VX_FORM(4,1036),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vslw",   VX_FORM(4,388),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vspltb", VX_FORM(4,524),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,VX_UIM2,PPC_0} },
  {  "vsplth", VX_FORM(4,588),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,VX_UIM3,PPC_0} },
  { "vspltsib",VX_FORM(4,780),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_SIM,PPC_0} },
  { "vspltsih",VX_FORM(4,844),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_SIM,PPC_0} },
  { "vspltsiw",VX_FORM(4,908),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_SIM,PPC_0} },
  {  "vspltw", VX_FORM(4,652),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,VX_UIM4,PPC_0} },
  {  "vsr",    VX_FORM(4,708),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsrab",  VX_FORM(4,772),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsrah",  VX_FORM(4,836),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsraw",  VX_FORM(4,900),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsrb",   VX_FORM(4,516),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsrh",   VX_FORM(4,580),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsro",   VX_FORM(4,1100),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsrw",   VX_FORM(4,644),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsubcuw",VX_FORM(4,1480),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsubfp", VX_FORM(4,74),        VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsubsbs",VX_FORM(4,1792),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsubshs",VX_FORM(4,1856),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsubsws",VX_FORM(4,1920),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsububm",VX_FORM(4,1024),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsububs",VX_FORM(4,1536),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsubuhm",VX_FORM(4,1088),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsubuhs",VX_FORM(4,1600),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsubuwm",VX_FORM(4,1152),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "vsubuws",VX_FORM(4,1664),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vsum2sws",VX_FORM(4,1672),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vsum4sbs",VX_FORM(4,1800),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vsum4shs",VX_FORM(4,1608),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vsum4ubs",VX_FORM(4,1544),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vsumsws", VX_FORM(4,1928),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  { "vupkhpx", VX_FORM(4,846),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  { "vupkhsb", VX_FORM(4,526),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  { "vupkhsh", VX_FORM(4,590),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  { "vupklpx", VX_FORM(4,974),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  { "vupklsb", VX_FORM(4,654),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  { "vupklsh", VX_FORM(4,718),       VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRB,PPC_0} },
  {  "vxor",   VX_FORM(4,1220),      VX_MASK, PPC_ALTIVEC|PPC_VEC, {VX_VRT,VX_VRA,VX_VRB,PPC_0} },
  {  "wait",    X_FORM(31,62,0),      X_MASK, PPC_CPU, {PPC_0} },
  {  "wait",    X_FORM(31,62,1),      X_MASK, PPC_CPU, {PPC_0} },
  {  "wrtee",   X_FORM(31,131,0),     X_MASK, PPC_CPU, {X_RS,PPC_0} },
  {  "wrtee",   X_FORM(31,131,1),     X_MASK, PPC_CPU, {X_RS,PPC_0} },
  {  "wrteei",  X_FORM(31,163,0),     X_MASK, PPC_CPU, {X_E,PPC_0} },
  {  "wrteei",  X_FORM(31,163,1),     X_MASK, PPC_CPU, {X_E,PPC_0} },
  {   "xor",    X_FORM(31,316,0),     X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {  "xor.",    X_FORM(31,316,1),     X_MASK, PPC_CPU, {X_RA,X_RS,X_RB,PPC_0} },
  {  "xori",    D_FORM(26),           D_MASK, PPC_CPU, {D_RA,D_RS,D_UI,PPC_0} },
  {  "xoris",   D_FORM(27),           D_MASK, PPC_CPU, {D_RA,D_RS,D_UI,PPC_0} },
};

#if 0
static tUInt32 __FASTCALL__ biswap_32(tUInt32 code) {
   unsigned i;
   tUInt32 rval=0x00;
   for(i=0;i<32;i++) if((code&(1<<i))==(1<<i)) rval = rval | (1<<(31-i));
   return rval;
}
#if __BYTE_ORDER != __LITTLE_ENDIAN
#define bie2mie_32(a) (a)
#define lie2mie_32(a) (biswap_32(a))
#else
#define bie2mie_32(a) (biswap_32(a))
#define lie2mie_32(a) (a)
#endif
#endif
static DisasmRet __FASTCALL__ ppcDisassembler(__filesize_t ulShift,
                                              MBuffer buffer,
                                              unsigned flags)
{
    DisasmRet dret;
    tInt32 tbuff;
    int done;
    unsigned i,ix,n,idx,val;
    char *p;
    tUInt32 opcode;
    if(detectedFormat->query_bitness) ppcBitness = detectedFormat->query_bitness(ulShift);
    if(detectedFormat->query_endian) ppcBigEndian = detectedFormat->query_endian(ulShift)==DAE_BIG?1:0;
    opcode=ppcBigEndian?be2me_32(*((tUInt32 *)buffer)):le2me_32(*((tUInt32 *)buffer));
    n = sizeof(ppc_table)/sizeof(ppc_opcode);
    done=0;
    dret.str = outstr;
    dret.codelen = 4;
    if(flags == __DISF_NORMAL) {
    tUInt32 ppc_fmask;
    ppc_fmask=ppcDialect?PPC_SPE:PPC_VEC;
    for(ix=0;ix<n;ix++)
    {
	tUInt32 mask,bits,ppc_flgs;
	mask=ppc_table[ix].mask;
	bits=ppc_table[ix].bits;
	ppc_flgs=ppc_table[ix].flags;
	if((opcode&mask)==bits)
	{
	    int legal_insn=0;
	    if(!(ppc_flgs&PPC_DIALECT)) legal_insn=1;
	    else if((ppc_flgs&PPC_DIALECT)==ppc_fmask) legal_insn=1;
	    if( legal_insn ) {
		strcpy(dret.str,ppc_table[ix].name);
		ppc_Encode_args(dret.str,opcode,ulShift,ppc_table[ix].flags,ppc_table[ix].args);
		done=1;
		dret.pro_clone=ppc_table[ix].flags & PPC_CLONE_MSK;
	    }
	}
    }
    if(!done)
    {
	{
		strcpy(dret.str,"db");
		TabSpace(dret.str,TAB_POS);
		disAppendDigits(dret.str,ulShift,APREF_USE_TYPE,4,&opcode,DISARG_DWORD);
	}
	dret.pro_clone=0;
    }
    }
    else {
	if(flags & __DISF_GETTYPE) dret.pro_clone = __INSNT_ORDINAL;
	else dret.codelen = 4;
    }
    return dret;
}

static tBool __FASTCALL__ ppcAsmRef( void )
{
  hlpDisplay(20050);
  return False;
}

static void __FASTCALL__ ppcHelpAsm( void )
{
 char *msgAsmText,*title;
 char **strs;
 unsigned size,i,evt;
 unsigned long nstrs;
 TWindow * hwnd;
 if(!hlpOpen(True)) return;
 size = (unsigned)hlpGetItemSize(20051);
 if(!size) goto ppchlp_bye;
 msgAsmText = PMalloc(size+1);
 if(!msgAsmText)
 {
   mem_off:
   MemOutBox(" Help Display ");
   goto ppchlp_bye;
 }
 if(!hlpLoadItem(20051,msgAsmText))
 {
   PFree(msgAsmText);
   goto ppchlp_bye;
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
 twGotoXY(2,3);
 {
   twGotoXY(2,3);
   i=0;
   {
     twSetColorAttr(disasm_cset.cpu_cset[0].clone[i]);
     twPutS("PPC CPU");
     twClrEOL();
   }
   twGotoXY(2,4);
   {
     twSetColorAttr(disasm_cset.cpu_cset[1].clone[i]);
     twPutS("PPC FPU");
     twClrEOL();
   }
   twGotoXY(2,5);
   {
     twSetColorAttr(disasm_cset.cpu_cset[2].clone[i]);
     twPutS("AltiVec");
     twClrEOL();
   }
 }
 do
 {
   evt = GetEvent(drawEmptyPrompt,NULL,hwnd);
 }
 while(!(evt == KE_ESCAPE || evt == KE_F(10)));
 CloseWnd(hwnd);
 ppchlp_bye:
 hlpClose();
}

static int    __FASTCALL__ ppcMaxInsnLen( void ) { return 8; }
static ColorAttr __FASTCALL__ ppcGetAsmColor( unsigned long clone )
{
  if((clone & PPC_CLONE_MSK)==PPC_ALTIVEC) return disasm_cset.cpu_cset[2].clone[0];
  else
  if((clone & PPC_CLONE_MSK)==PPC_FPU) return disasm_cset.cpu_cset[1].clone[0];
  else
	return disasm_cset.cpu_cset[0].clone[0];
}

static int       __FASTCALL__ ppcGetBitness( void ) { return ppcBitness; }
static char      __FASTCALL__ ppcGetClone( unsigned long clone )
{
  UNUSED(clone);
  return ' ';
}

static const char *ppc_bitness_names[] =
{
   "Use~32",
   "Use~64"
};

static tBool __FASTCALL__ ppcSelect_bitness( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(ppc_bitness_names)/sizeof(char *);
  i = SelBoxA(ppc_bitness_names,nModes," Select bitness mode: ",ppcBitness);
  if(i != -1)
  {
    ppcBitness = ((i==0)?DAB_USE32:DAB_USE64);
    return True;
  }
  return False;
}

static const char *ppc_endian_names[] =
{
   "~Little endian",
   "~Big endian"
};

static tBool __FASTCALL__ ppcSelect_endian( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(ppc_endian_names)/sizeof(char *);
  i = SelBoxA(ppc_endian_names,nModes," Select endian mode: ",ppcBigEndian);
  if(i != -1)
  {
    ppcBigEndian = i;
    return True;
  }
  return False;
}

static const char *ppc_dialect_names[] =
{
   "~Altivec",
   "~SPE (embedded cpu)"
};

static tBool __FASTCALL__ ppcSelectDialect( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(ppc_dialect_names)/sizeof(char *);
  i = SelBoxA(ppc_dialect_names,nModes," Select PPC dialect: ",ppcDialect);
  if(i != -1)
  {
    ppcDialect = i;
    return True;
  }
  return False;
}

static void      __FASTCALL__ ppcInit( void )
{
  unsigned i,n,j;
  outstr = PMalloc(1000);
  if(!outstr)
  {
    MemOutBox("Data disassembler initialization");
    exit(EXIT_FAILURE);
  }
}

static void  __FASTCALL__ ppcTerm( void )
{
   PFREE(outstr);
}

static void __FASTCALL__ ppcReadIni( hIniProfile *ini )
{
  char tmps[10];
  if(isValidIniArgs())
  {
    biewReadProfileString(ini,"Biew","Browser","SubSubMode3","1",tmps,sizeof(tmps));
    ppcBitness = (int)strtoul(tmps,NULL,10);
    if(ppcBitness > 1 && ppcBitness != DAB_AUTO) ppcBitness = 0;
    biewReadProfileString(ini,"Biew","Browser","SubSubMode4","1",tmps,sizeof(tmps));
    ppcBigEndian = (int)strtoul(tmps,NULL,10);
    if(ppcBigEndian > 1) ppcBigEndian = 0;
    biewReadProfileString(ini,"Biew","Browser","SubSubMode5","0",tmps,sizeof(tmps));
    ppcDialect = (int)strtoul(tmps,NULL,10);
    if(ppcDialect > 1) ppcDialect = 0;
  }
}

static void __FASTCALL__ ppcWriteIni( hIniProfile *ini )
{
  char tmps[10];
  sprintf(tmps,"%i",ppcBitness);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode3",tmps);
  sprintf(tmps,"%i",ppcBigEndian);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode4",tmps);
  sprintf(tmps,"%i",ppcDialect);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode5",tmps);
}

REGISTRY_DISASM PPC_Disasm =
{
  DISASM_CPU_PPC,
  "AIM Power5+ ISA             [New,Experimental]",
  { "PpcHlp", "Bitnes", "Endian", "Dialec" },
  { ppcAsmRef, ppcSelect_bitness, ppcSelect_endian, ppcSelectDialect },
  ppcDisassembler,
  NULL,
  ppcHelpAsm,
  ppcMaxInsnLen,
  ppcGetAsmColor,
  NULL,
  ppcGetAsmColor,
  NULL,
  ppcGetBitness,
  ppcGetClone,
  ppcInit,
  ppcTerm,
  ppcReadIni,
  ppcWriteIni
};




