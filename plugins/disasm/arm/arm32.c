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
    Q	 - system register 0 - fpsid, 1 - fpscr, 8 - fpexc
    T	 - source fpu register
    t	 - low index of source fpu register
    V	 - destinition fpu register
    v	 - low index of destinition fpu register
    Z	 - third fpu register
    z	 - low index of third fpu register
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
 /* CPU */
    { "ADC", "cccc00I0101Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "ADD", "cccc00I0100Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "AND", "cccc00I0000Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "B",   "cccc101Loooooooooooooooooooooooo", ARM_V1|ARM_INTEGER },
    { "BIC", "cccc00I1110Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "BKPT","111000010010iiiiiiiiiiii0111iiii", ARM_V5|ARM_INTEGER },
    { "BLX", "1111101Hoooooooooooooooooooooooo", ARM_V5|ARM_INTEGER },
    { "BLX", "cccc000100101111111111110011dddd", ARM_V5|ARM_INTEGER },
    { "BX",  "cccc000100101111111111110001dddd", ARM_V5|ARM_INTEGER },
    { "CLZ", "cccc000101101111dddd11110001ssss", ARM_V5|ARM_INTEGER },
    { "CMN", "cccc00I10111ssss0000<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "CMP", "cccc00I10101ssss0000<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "EOR", "cccc00I0001Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "LDC2","1111110PUNW1TTTTVVVVxxxxOOOOOOOO", ARM_V5|ARM_FPU },
    { "LDC", "cccc110PUNW1TTTTVVVVxxxxOOOOOOOO", ARM_V2|ARM_FPU },
    { "LDM", "cccc100PU0W1ssssRRRRRRRRRRRRRRRR", ARM_V1|ARM_INTEGER },
    { "LDM", "cccc100PU101ssss0RRRRRRRRRRRRRRR", ARM_V1|ARM_INTEGER },
    { "LDR", "cccc01IPU0W1ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER },
    { "LDRB","cccc01IPU1W1ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER },
    {"LDRBT","cccc01I0U111ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER },
    { "LDRH","cccc000PUIW1ssssddddaaaa1011aaaa", ARM_V4|ARM_INTEGER },
    {"LDRSB","cccc000PUIW1ssssddddaaaa1101aaaa", ARM_V4|ARM_INTEGER },
    {"LDRSH","cccc000PUIW1ssssddddaaaa1111aaaa", ARM_V4|ARM_INTEGER },
    { "LDRT","cccc01I0U011ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER },
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
    { "RSB", "cccc00I0011Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "RSC", "cccc00I0111Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    { "SBC", "cccc00I0110Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER },
    {"SMLAL","cccc0000111SDDDDddddssss1001mmmm", ARM_V1|ARM_INTEGER },
    {"SMULL","cccc0000110SDDDDddddssss1001mmmm", ARM_V1|ARM_INTEGER },
    { "STC2","1111110PUNW0ssssTTTTxxxxiiiiiiii", ARM_V5|ARM_INTEGER },
    { "STC", "cccc110PUNW0ssssTTTTxxxxiiiiiiii", ARM_V2|ARM_INTEGER },
    { "STM", "cccc100PU0W0ssssRRRRRRRRRRRRRRRR", ARM_V1|ARM_INTEGER },
    { "STM", "cccc100PU100ssssRRRRRRRRRRRRRRRR", ARM_V1|ARM_INTEGER },
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
    {"UMULL","cccc0000100SDDDDddddssss1001mmmm", ARM_V1|ARM_INTEGER },
 /* DSP */
    { "LDRD","cccc000PUIW0ssssddddaaaa1101aaaa", ARM_V5|ARM_DSP },
    { "MCRR","cccc11000100nnnnddddxxxxFFFFyyyy", ARM_V5|ARM_DSP },
    { "MRRC","cccc11000101nnnnddddxxxxFFFFyyyy", ARM_V5|ARM_DSP },
    { "PLD", "111101I1U101ssss1111aaaaaaaaaaaa", ARM_V5|ARM_DSP },
    {"QADD", "cccc00010000ssssdddd00000101mmmm", ARM_V5|ARM_DSP },
    {"QDADD","cccc00010100ssssdddd00000101mmmm", ARM_V5|ARM_DSP },
    {"QDSUB","cccc00010110ssssdddd00000101mmmm", ARM_V5|ARM_DSP },
    { "QSUB","cccc00010010ssssdddd00000101mmmm", ARM_V5|ARM_DSP },
    { "SMLA","cccc00010000ddddnnnnssss1YX0mmmm", ARM_V5|ARM_DSP },
    {"SMLAL","cccc00010100DDDDddddssss1YX0mmmm", ARM_V5|ARM_DSP },
    {"SMLAW","cccc00010010ddddnnnnssss1Y00mmmm", ARM_V5|ARM_DSP },
    { "SMUL","cccc00010110dddd0000ssss1YX0mmmm", ARM_V5|ARM_DSP },
    {"SMULW","cccc00010010dddd0000ssss1Y10mmmm", ARM_V5|ARM_DSP },
    { "STRD","cccc000PUIW0nnnnddddaaaa1111aaaa", ARM_V5|ARM_DSP },
 /* VFP */
    { "CDP2","11111110ffffTTTTttttxxxxFFF0yyyy", ARM_V5|ARM_FPU },
    { "CDP", "cccc1110ffffTTTTttttxxxxFFF0yyyy", ARM_V2|ARM_FPU },

    {"FABSD","cccc111010110000VVVV10111100TTTT", ARM_V5|ARM_FPU },
    {"FABSS","cccc11101v110000VVVV101111t0TTTT", ARM_V5|ARM_FPU },
    {"FADDD","cccc11100011TTTTVVVV10110000ZZZZ", ARM_V5|ARM_FPU },
    {"FADDS","cccc11100T11TTTTVVVV1010t0z0ZZZZ", ARM_V5|ARM_FPU },
    {"FCMPD","cccc111010110100TTTT10110100ZZZZ", ARM_V5|ARM_FPU },
   {"FCMPED","cccc111010110100TTTT10111100ZZZZ", ARM_V5|ARM_FPU },
   {"FCMPES","cccc11101t110100TTTT101011z0ZZZZ", ARM_V5|ARM_FPU },
  {"FCMPEZD","cccc111010110101TTTT101111000000", ARM_V5|ARM_FPU },
  {"FCMPEZS","cccc11101t110101TTTT101111000000", ARM_V5|ARM_FPU },
    {"FCMPS","cccc11101t110100TTTT101001z0ZZZZ", ARM_V5|ARM_FPU },
   {"FCMPZD","cccc111010110101TTTT101101000000", ARM_V5|ARM_FPU },
   {"FCMPZS","cccc11101t110101TTTT101001000000", ARM_V5|ARM_FPU },
    {"FCPYD","cccc111010110000VVVV10110100TTTT", ARM_V5|ARM_FPU },
    {"FCPYS","cccc11101v110000VVVV101001t0TTTT", ARM_V5|ARM_FPU },
   {"FCVTDS","cccc111010110111VVVV101011t0TTTT", ARM_V5|ARM_FPU },
   {"FCVTSD","cccc11101v110111VVVV10111100TTTT", ARM_V5|ARM_FPU },
    {"FDIVD","cccc11101000TTTTVVVV10110000ZZZZ", ARM_V5|ARM_FPU },
    {"FDIVS","cccc11101t00TTTTVVVV1010v0z0ZZZZ", ARM_V5|ARM_FPU },
    {"FLDD" ,"cccc1101U001ssssVVVV1011OOOOOOOO", ARM_V5|ARM_FPU },
    {"FLDND","cccc110PU0W1ssssVVVV1011OOOOOOOO", ARM_V5|ARM_FPU },
    {"FLDNS","cccc110PUvW1ssssVVVV1010OOOOOOOO", ARM_V5|ARM_FPU },
    {"FLDMX","cccc110PU0W1ssssVVVV1011OOOOOOOO", ARM_V5|ARM_FPU },
    {"FLDS" ,"cccc1101Uv01ssssVVVV1010OOOOOOOO", ARM_V5|ARM_FPU },
    {"FMACD","cccc11100000TTTTVVVV10110000ZZZZ", ARM_V5|ARM_FPU },
    {"FMACS","cccc11100v00TTTTVVVV1010t0z0ZZZZ", ARM_V5|ARM_FPU },
    {"FMDHR","cccc11100010VVVVssss101100010000", ARM_V5|ARM_FPU },
    {"FMDLR","cccc11100000VVVVssss101100010000", ARM_V5|ARM_FPU },
    {"FMRDH","cccc11100011TTTTdddd101100010000", ARM_V5|ARM_FPU },
    {"FMRDL","cccc11100001TTTTdddd101100010000", ARM_V5|ARM_FPU },
    {"FMRS", "cccc11100001TTTTdddd1010t0010000", ARM_V5|ARM_FPU },
    {"FMRX", "cccc11101111QQQQdddd101000010000", ARM_V5|ARM_FPU },
    {"FMSCD","cccc11100001TTTTVVVV10110000ZZZZ", ARM_V5|ARM_FPU },
    {"FMSCS","cccc11100v01TTTTVVVV1010t0z0ZZZZ", ARM_V5|ARM_FPU },
    {"FMSR" ,"cccc11100000TTTTdddd1010t0010000", ARM_V5|ARM_FPU },
   {"FMSTAT","cccc1110111100011111101000010000", ARM_V5|ARM_FPU },
    {"FMULD","cccc11100010TTTTVVVV10110000ZZZZ", ARM_V5|ARM_FPU },
    {"FMULS","cccc11100v10TTTTVVVV1010t0z0ZZZZ", ARM_V5|ARM_FPU },
    {"FMXR" ,"cccc11101110QQQQssss101000010000", ARM_V5|ARM_FPU },
    {"FNEGD","cccc111010110001VVVV10110100TTTT", ARM_V5|ARM_FPU },
    {"FNEGS","cccc11101v110001VVVV10100t00TTTT", ARM_V5|ARM_FPU },
   {"FNMACD","cccc11100000TTTTVVVV10110100ZZZZ", ARM_V5|ARM_FPU },
   {"FNMACS","cccc11100v00TTTTVVVV1010t1z0ZZZZ", ARM_V5|ARM_FPU },
   {"FNMSCD","cccc11100001TTTTVVVV10110100ZZZZ", ARM_V5|ARM_FPU },
   {"FNMSCD","cccc11100v01TTTTVVVV1010t1z0ZZZZ", ARM_V5|ARM_FPU },
   {"FNMULD","cccc11100010TTTTVVVV10110100ZZZZ", ARM_V5|ARM_FPU },
   {"FNMULS","cccc11100v10TTTTVVVV1010t1z0ZZZZ", ARM_V5|ARM_FPU },
   {"FSITOD","cccc111010111000VVVV101111t0TTTT", ARM_V5|ARM_FPU },
   {"FSITOS","cccc11101v111000VVVV101011t0TTTT", ARM_V5|ARM_FPU },
   {"FSQRTD","cccc111010110001VVVV10111100TTTT", ARM_V5|ARM_FPU },
   {"FSQRTS","cccc11101v110001VVVV101011t0TTTT", ARM_V5|ARM_FPU },
    {"FSTD", "cccc1101U000ssssVVVV1011OOOOOOOO", ARM_V5|ARM_FPU },
    {"FSTMD","cccc110PU0W0ssssVVVV1011OOOOOOOO", ARM_V5|ARM_FPU },
    {"FSTMS","cccc110PUvW0ssssVVVV1010OOOOOOOO", ARM_V5|ARM_FPU },
    {"FSTMX","cccc110PU0W0ssssVVVV1011OOOOOOOO", ARM_V5|ARM_FPU },
    {"FSTS", "cccc1101Uv00ssssVVVV1010OOOOOOOO", ARM_V5|ARM_FPU },
   { "FSUBD","cccc11100011TTTTVVVV10110100ZZZZ", ARM_V5|ARM_FPU },
   { "FSUBS","cccc11100v11TTTTVVVV1010t1z0ZZZZ", ARM_V5|ARM_FPU },
   {"FTOSID","cccc11101v111101VVVV10110100TTTT", ARM_V5|ARM_FPU },
  {"FTOSIZD","cccc11101v111101VVVV10111100TTTT", ARM_V5|ARM_FPU },
   {"FTOSIS","cccc11101v111101VVVV101001t0TTTT", ARM_V5|ARM_FPU },
  {"FTOSIZS","cccc11101v111101VVVV101011t0TTTT", ARM_V5|ARM_FPU },
   {"FTOUID","cccc11101v111100VVVV10110100TTTT", ARM_V5|ARM_FPU },
  {"FTOUIZD","cccc11101v111100VVVV10111100TTTT", ARM_V5|ARM_FPU },
   {"FTOSIS","cccc11101v111100VVVV101001t0TTTT", ARM_V5|ARM_FPU },
  {"FTOSIZS","cccc11101v111100VVVV101011t0TTTT", ARM_V5|ARM_FPU },
   {"FUITOD","cccc111010111000VVVV101101t0TTTT", ARM_V5|ARM_FPU },
   {"FUITOS","cccc11101v111000VVVV101001t0TTTT", ARM_V5|ARM_FPU },

    { "MCR2","11111110fff0TTTTssssxxxxFFF1yyyy", ARM_V5|ARM_FPU },
    { "MCR", "cccc1110fff0TTTTssssxxxxFFF1yyyy", ARM_V2|ARM_FPU }
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
const char *arm_sysfreg_name[16] =
{
    "fpsid", "fpscr", "fp???", "fp???", "fp???", "fp???", "fp???", "fp???",
    "fpexc", "fp???", "fp???", "fp???", "fp???", "fp???", "fp???", "fp???",
};

const char *arm_freg_name[32] = 
{
    "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7",
    "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15",
    "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23",
    "F24", "F25", "F26", "F27", "F28", "F29", "F30", "F31"
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
    p=strchr(msk,'Q');
    if(p)
    {
	READ_IMM32('Q');
	if(prev) strcat(dret->str,","); prev=1;
	strcat(dret->str,arm_sysfreg_name[val&0xF]);
    }
    p=strchr(msk,'V');
    if(p)
    {
	unsigned val2;
	READ_IMM32('V');
	val2=val;
	if(prev) strcat(dret->str,","); prev=1;
	READ_IMM32('v');
	strcat(dret->str,arm_freg_name[(val2&0x1F<<1)|(val&1)]);
    }
    p=strchr(msk,'T');
    if(p)
    {
	unsigned val2;
	READ_IMM32('T');
	val2=val;
	if(prev) strcat(dret->str,","); prev=1;
	READ_IMM32('t');
	strcat(dret->str,arm_freg_name[(val2&0x1F<<1)|(val&1)]);
    }
    p=strchr(msk,'Z');
    if(p)
    {
	unsigned val2;
	READ_IMM32('Z');
	val2=val;
	if(prev) strcat(dret->str,","); prev=1;
	READ_IMM32('z');
	strcat(dret->str,arm_freg_name[(val2&0x1F<<1)|(val&1)]);
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
