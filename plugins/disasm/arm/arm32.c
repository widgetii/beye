/**
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/arm.c
 * @brief       This file contains implementation of Data disassembler.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bswap.h"
#include "reg_form.h"
#include "plugins/disasm.h"
#include "biewhelp.h"
#include "biewutil.h"
#include "plugins/disasm/arm/arm.h"

typedef struct tag_arm_opcode32
{
    const char *name;
    const char *mask;
    const long flags;
    unsigned   bmsk;
    unsigned   bits;
}arm_opcode32;

/*
    c    - conditional codes
    a	 - address mode
    d	 - destinition register
    D	 - hipart of destinition register
    b	 - field-bit mask
    s	 - source register
    n	 - Rn register
    m	 - Rm register
    R	 - register list
    t	 - source fpu register
    T	 - destinition fpu register
    y	 - third fpu register
    x	 - number of fpu
    <	 - shifter operand
    f    - fpu opcode 1
    F    - fpu opcode 2
    L    - L suffix of insn
    H    - indicates Half-word offset
    i    - immediate value
    o    - code offset value
    O    - offset value
    XY   - XY values of 'TB' suffixes for DSP<x><y> instructions
    SIPUNW - addresing modes
*/

static arm_opcode32 opcode_table[]=
{
    { "ADC", "cccc00I0101Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "ADD", "cccc00I0100Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "AND", "cccc00I0000Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "B",   "cccc101Loooooooooooooooooooooooo", ARM_V1|ARM_INTEGER },
    { "BIC", "cccc00I1110Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "BKPT","111000010010iiiiiiiiiiii0111iiii", ARM_V5|ARM_INTEGER },
    { "BLX", "1111101Hoooooooooooooooooooooooo", ARM_V5|ARM_INTEGER },
    { "BLX", "cccc000100101111111111110011dddd", ARM_V5|ARM_INTEGER },
    { "BX",  "cccc000100101111111111110001dddd", ARM_V5|ARM_INTEGER },
    { "CDP2","11111110ffffTTTTttttxxxxFFF0yyyy", ARM_V5|ARM_FPU },
    { "CDP", "cccc1110ffffTTTTttttxxxxFFF0yyyy", ARM_V2|ARM_FPU },
    { "CLZ", "cccc000101101111dddd11110001ssss", ARM_V5|ARM_INTEGER },
    { "CMN", "cccc00I10111ssss0000<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "CMP", "cccc00I10101ssss0000<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "EOR", "cccc00I0001Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "LDC2","1111110PUNW1ttttTTTTxxxxOOOOOOOO", ARM_V5|ARM_FPU },
    { "LDC", "cccc110PUNW1ttttTTTTxxxxOOOOOOOO", ARM_V2|ARM_FPU },
    { "LDM", "cccc100PU0W1ssssRRRRRRRRRRRRRRRR", ARM_V1|ARM_INTEGER },
    { "LDM", "cccc100PU101ssss0RRRRRRRRRRRRRRR", ARM_V1|ARM_INTEGER },
    { "LDR", "cccc01IPU0W1ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER },
    { "LDRB","cccc01IPU1W1ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER },
    {"LDRBT","cccc01I0U111ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER },
    { "LDRH","cccc000PUIW1ssssddddaaaa1011aaaa", ARM_V4|ARM_INTEGER },
    { "LDRD","cccc000PUIW0ssssddddaaaa1101aaaa", ARM_V5|ARM_DSP },
    {"LDRSB","cccc000PUIW1ssssddddaaaa1101aaaa", ARM_V4|ARM_INTEGER },
    {"LDRSH","cccc000PUIW1ssssddddaaaa1111aaaa", ARM_V4|ARM_INTEGER },
    { "LDRT","cccc01I0U011ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER },
    { "MCRR","cccc11000100nnnnddddxxxxFFFFyyyy", ARM_V5|ARM_DSP },
    { "MRRC","cccc11000101nnnnddddxxxxFFFFyyyy", ARM_V5|ARM_DSP },
    { "MCR2","11111110fff0TTTTssssxxxxFFF1yyyy", ARM_V5|ARM_FPU },
    { "MCR", "cccc1110fff0TTTTssssxxxxFFF1yyyy", ARM_V2|ARM_FPU },
    { "MLA", "cccc0000001Sddddnnnnssss1001mmmm", ARM_V2|ARM_INTEGER },
    { "MOV", "cccc00I1101S0000dddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "MRC2","11111110fff1ttttddddxxxxFFF1yyyy", ARM_V5|ARM_INTEGER },
    { "MRC", "cccc1110fff1ttttddddxxxxFFF1yyyy", ARM_V2|ARM_INTEGER },
    { "MRS", "cccc00010R000000dddd000000000000", ARM_V3|ARM_INTEGER },
    { "MSR", "cccc00110R10bbbb1111iiiiiiiiiiii", ARM_V3|ARM_INTEGER },
    { "MSR", "cccc00010R10bbbb111100000000mmmm", ARM_V3|ARM_INTEGER },
    { "MUL", "cccc0000000Sdddd0000ssss1001mmmm", ARM_V2|ARM_INTEGER },
    { "MVN", "cccc00I1111S0000dddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "ORR", "cccc00I1100Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "PLD", "111101I1U101ssss1111aaaaaaaaaaaa", ARM_V5|ARM_DSP },
    {"QADD", "cccc00010000ssssdddd00000101mmmm", ARM_V5|ARM_DSP },
    {"QDADD","cccc00010100ssssdddd00000101mmmm", ARM_V5|ARM_DSP },
    {"QDSUB","cccc00010110ssssdddd00000101mmmm", ARM_V5|ARM_DSP },
    { "QSUB","cccc00010010ssssdddd00000101mmmm", ARM_V5|ARM_DSP },
    { "RSB", "cccc00I0011Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "RSC", "cccc00I0111Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "SBC", "cccc00I0110Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "SMLA","cccc00010000ddddnnnnssss1YX0mmmm", ARM_V5|ARM_DSP },
    {"SMLAL","cccc00010100DDDDddddssss1YX0mmmm", ARM_V5|ARM_DSP },
    {"SMLAW","cccc00010010ddddnnnnssss1Y00mmmm", ARM_V5|ARM_DSP },
    {"SMLAL","cccc0000111SDDDDddddssss1001mmmm", ARM_V1|ARM_INTEGER },
    { "SMUL","cccc00010110dddd0000ssss1YX0mmmm", ARM_V5|ARM_DSP },
    {"SMULL","cccc0000110SDDDDddddssss1001mmmm", ARM_V1|ARM_INTEGER },
    {"SMULW","cccc00010010dddd0000ssss1Y10mmmm", ARM_V5|ARM_DSP },
    { "STC2","1111110PUNW0ssssTTTTxxxxiiiiiiii", ARM_V5|ARM_INTEGER },
    { "STC", "cccc110PUNW0ssssTTTTxxxxiiiiiiii", ARM_V2|ARM_INTEGER },
    { "STM", "cccc100PU0W0ssssRRRRRRRRRRRRRRRR", ARM_V1|ARM_INTEGER },
    { "STM", "cccc100PU100ssssRRRRRRRRRRRRRRRR", ARM_V1|ARM_INTEGER },
    { "STRD","cccc000PUIW0nnnnddddaaaa1111aaaa", ARM_V5|ARM_DSP },
    { "STR", "cccc01IPU0W0ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER },
    { "STRB","cccc01IPU1W0ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER },
    {"STRBT","cccc01I0U110ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER },
    { "STRH","cccc000PUIW0ssssddddaaaa1011aaaa", ARM_V4|ARM_INTEGER },
    { "STRT","cccc01I0U010ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER },
    { "SUB", "cccc00I0010Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "SWI", "cccc1111iiiiiiiiiiiiiiiiiiiiiiii", ARM_V1|ARM_INTEGER },
    { "SWP", "cccc00010000ssssdddd00001001mmmm", ARM_V3|ARM_INTEGER },
    { "SWPB","cccc00010100ssssdddd00001001mmmm", ARM_V3|ARM_INTEGER },
    { "TEQ", "cccc00I10011ssss0000<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "TST", "cccc00I10001ssss0000<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    {"UMLAL","cccc0000101SDDDDddddssss1001mmmm", ARM_V1|ARM_INTEGER },
    {"UMULL","cccc0000100SDDDDddddssss1001mmmm", ARM_V1|ARM_INTEGER }

//    { "XYZ", "ccccaaaabbbbddddeeeeffffgggghhhh", ARM_V5|ARM_INTEGER }
};

const char *armCCnames[16] = 
{
    "EQ", /* Equal */
    "NE", /* Not Equal */
    "CS", /* Carry Set */
    "CC", /* Carry Clear */
    "MI", /* MInus/negative */
    "PL", /* PLus/positive or zero */
    "VS", /* V Set = overflow */
    "VC", /* V Clear = non overflow */
    "HI", /* unsigned HIgher [C set and Z clear] */
    "LS", /* unsigned Lower or Same [C clear or Z set] */
    "GE", /* signed Great or Equal */
    "LT", /* signed Less Than */
    "GT", /* signed Great Than */
    "LE", /* signed Less or Equal */
    ""/*AL - Always or unconditional*/,
    "NV" /* NEVER */
};
const char *arm_freg_name[16] = 
{
    "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7",
    "C8", "C9", "C10", "C11", "C12", "C13", "C14", "C15"
};
const char *arm_funit_name[16] = 
{
    "P0", "P1", "P2", "P3", "P4", "P5", "P6", "P7",
    "P8", "P9", "P10", "P11", "P12", "P13", "P14", "P15"
};
extern const char * arm_reg_name[];
#define READ_IMM32(chr)\
{\
	val=0;\
	idx=0;\
	idx=strrchr(msk,chr)-msk;\
	for(i=0;i<32;i++)\
	    if(msk[i]==chr)\
		val |= (opcode&(1<<(31-i)));\
	val>>=(31-idx);\
}

#define PARSE_IMM32(chr,smul)\
    p=strchr(msk,chr);\
    if(p)\
    {\
	READ_IMM32(chr);\
	if(prev) strcat(dret->str,",");\
	strcat(dret->str,"#");\
	disAppendDigits(dret->str,ulShift,APREF_USE_TYPE,4,&val,DISARG_WORD);\
	if(smul) strcat(dret->str,smul);\
	prev=1;\
    }

void __FASTCALL__ arm32EncodeTail(DisasmRet *dret,__filesize_t ulShift,
					tUInt32 opcode, unsigned flags,unsigned index)
{
    unsigned i,idx,val,prev,bracket;
    const char *msk=opcode_table[index].mask;
    char *p;
    prev=0;
    bracket=0;
    p=strchr(msk,'x');
    if(p)
    {
	READ_IMM32('x');
	if(prev) strcat(dret->str,","); prev=1;
	strcat(dret->str,arm_funit_name[val&0xF]);
    }
    p=strchr(msk,'f');
    if(p)
    {
	READ_IMM32('f');
	if(prev) strcat(dret->str,","); prev=1;
	strcat(dret->str,Get2Digit(val&0xF));
    }
    p=strchr(msk,'D');
    if(p)
    {
	READ_IMM32('D');
	if(prev) strcat(dret->str,","); prev=1;
	strcat(dret->str,arm_reg_name[val&0xF]);
    }
    p=strchr(msk,'d');
    if(p)
    {
	READ_IMM32('d');
	if(prev) strcat(dret->str,","); prev=1;
	strcat(dret->str,arm_reg_name[val&0xF]);
    }
    p=strchr(msk,'s');
    if(p)
    {
	READ_IMM32('s');
	if(prev) strcat(dret->str,","); prev=1;
	strcat(dret->str,arm_reg_name[val&0xF]);
    }
    p=strchr(msk,'m');
    if(p)
    {
	READ_IMM32('m');
	if(prev) strcat(dret->str,","); prev=1;
	strcat(dret->str,arm_reg_name[val&0xF]);
    }
    p=strchr(msk,'T');
    if(p)
    {
	READ_IMM32('T');
	if(prev) strcat(dret->str,","); prev=1;
	strcat(dret->str,arm_freg_name[val&0xF]);
    }
    p=strchr(msk,'t');
    if(p)
    {
	READ_IMM32('t');
	if(prev) strcat(dret->str,","); prev=1;
	strcat(dret->str,arm_freg_name[val&0xF]);
    }
    p=strchr(msk,'y');
    if(p)
    {
	READ_IMM32('y');
	if(prev) strcat(dret->str,","); prev=1;
	strcat(dret->str,arm_freg_name[val&0xF]);
    }
    p=strchr(msk,'R');
    if(p)
    {
	int prevv;
	if(prev) strcat(dret->str,"!,{");
	READ_IMM32('R');
	prevv=0;
	for(i=0;i<32;i++)
	{
	    if(val&(1<<i))
	    {
		if(prevv) strcat(dret->str,",");
		strcat(dret->str,arm_reg_name[i]);
		prevv=1;
	    }
	}
	if(prev) strcat(dret->str,"}");
    }
    p=strchr(msk,'o');
    if(p)
    {
	tUInt32 tbuff;
	unsigned hh=0;
	p=strchr(msk,'H');
	if(p)
	{
	    READ_IMM32('H');
	    hh=val;
	}
	READ_IMM32('o');
	tbuff=val<<2;
	if(prev) strcat(dret->str,",");
	if(hh) tbuff+=2;
	disAppendFAddr(dret->str,ulShift+1,(long)tbuff,ulShift+tbuff,
			DISADR_NEAR32,0,3);
	prev=1;
    }
    PARSE_IMM32('i',NULL);

    p=strchr(msk,'F');
    if(p)
    {
	READ_IMM32('F');
	if(prev) strcat(dret->str,","); prev=1;
	strcat(dret->str,Get2Digit(val&0xF));
    }
}

void __FASTCALL__ arm32Disassembler(DisasmRet *dret,__filesize_t ulShift,
					tUInt32 opcode, unsigned flags)
{
    tInt32 tbuff;
    int done;
    unsigned i,ix,n,idx,val;
    char *p;
    const char *msk;
    n = sizeof(opcode_table)/sizeof(arm_opcode32);
    done=0;
    for(ix=0;ix<n;ix++)
    {
	if((opcode & opcode_table[ix].bmsk) == opcode_table[ix].bits)
	{
	    strcpy(dret->str,opcode_table[ix].name);
	    msk=opcode_table[ix].mask;
	    p=strchr(msk,'L');
	    if(p)
	    {
		READ_IMM32('L');
		if(val) strcat(dret->str,"L");
	    }
	    p=strchr(msk,'X');
	    if(p)
	    {
		READ_IMM32('X');
		strcat(dret->str,val?"T":"B");
	    }
	    p=strchr(msk,'Y');
	    if(p)
	    {
		READ_IMM32('Y');
		strcat(dret->str,val?"T":"B");
	    }
	    p=strchr(msk,'c');
	    if(p)
	    {
		READ_IMM32('c');
		strcat(dret->str,armCCnames[val&0xF]);
	    }
	    TabSpace(dret->str,TAB_POS);
	    arm32EncodeTail(dret,ulShift,opcode,flags,ix);
	    done=1;
	    break;
	}
    }
    if(!done)
    {
	    {
		strcpy(dret->str,"???");
		TabSpace(dret->str,TAB_POS);
		disAppendDigits(dret->str,ulShift,APREF_USE_TYPE,4,&opcode,DISARG_DWORD);
	    }
    }
}

void __FASTCALL__ arm32Init(void)
{
    unsigned i,n,j;
    n = sizeof(opcode_table)/sizeof(arm_opcode32);
    for(i=0;i<n;i++)
    {
	opcode_table[i].bmsk=0;
	opcode_table[i].bits=0;
	for(j=0;j<32;j++)
	{
	    if(opcode_table[i].mask[j]=='0' || opcode_table[i].mask[j]=='1')
	    {
		opcode_table[i].bmsk|=(1<<(31-j));
		opcode_table[i].bits|=(opcode_table[i].mask[j]=='1'?1<<(31-j):0<<(31-j));
	    }
	}
    }
}

void __FASTCALL__ arm32Term(void) {}
