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

#define ARM_USE_SP	0x00100000UL
#define ARM_USE_PC	0x00200000UL
#define ARM_HAS_RN	0x00400000UL

typedef struct tag_arm_opcode16
{
    const char *name;
    const char *mask;
    const long flags;
    unsigned   bmsk;
    unsigned   bits;
}arm_opcode16;

/*
    c    - conditional codes
    a	 - address mode
    d	 - destinition register
    D	 - indicates high of destinition register
    b	 - field-bit mask
    s	 - source register
    S	 - indicates high of source register
    N	 - indicates high of Rn register
    n	 - Rn register
    M	 - indicates high of Rm register
    m	 - Rm register
    R	 - register list
    t	 - source fpu register
    T	 - destinition fpu register
    y	 - third fpu register
    x	 - number of fpu
    L    - L suffix of insn
    H    - indicates Half-word offset
    i    - immediate value
    2    - immediate*2 value
    4    - immediate*4 value
    o    - code offset value
    O    - offset value
*/
static arm_opcode16 opcode_table[]=
{
    { "ADC", "0100000101mmmddd", ARM_V4|ARM_INTEGER },
    { "ADD", "0001110iiisssddd", ARM_V4|ARM_INTEGER },
    { "ADD", "00110dddiiiiiiii", ARM_V4|ARM_INTEGER },
    { "ADD", "0001100mmmsssddd", ARM_V4|ARM_INTEGER },
    { "ADD", "01000100SDsssddd", ARM_V4|ARM_INTEGER },
    { "ADD", "10100ddd44444444", ARM_V4|ARM_INTEGER|ARM_USE_PC },
    { "ADD", "10101ddd44444444", ARM_V4|ARM_INTEGER|ARM_USE_SP },
    { "ADD", "1011000004444444", ARM_V4|ARM_INTEGER },
    { "AND", "0100000000sssddd", ARM_V4|ARM_INTEGER },
    { "ASR", "00010iiiiisssddd", ARM_V4|ARM_INTEGER },
    { "ASR", "0100000100sssddd", ARM_V4|ARM_INTEGER },
    { "B",   "1101ccccoooooooo", ARM_V4|ARM_INTEGER },
    { "B",   "11100ooooooooooo", ARM_V4|ARM_INTEGER },
    { "BIC", "0100001110sssddd", ARM_V4|ARM_INTEGER },
    { "BKPT","10111110iiiiiiii", ARM_V5|ARM_INTEGER },
    { "BL",  "111HHooooooooooo", ARM_V5|ARM_INTEGER },
    { "BLX", "010001111Dddd000", ARM_V5|ARM_INTEGER },
    { "BX",  "010001110Dddd000", ARM_V4|ARM_INTEGER },
    { "CMN", "0100001011sssmmm", ARM_V4|ARM_INTEGER },
    { "CMP", "00101sssiiiiiiii", ARM_V4|ARM_INTEGER },
    { "CMP", "0100001010mmmsss", ARM_V4|ARM_INTEGER },
    { "CMP", "01000101MSmmmsss", ARM_V4|ARM_INTEGER },
    { "EOR", "0100000001mmmddd", ARM_V4|ARM_INTEGER },
    {"LDMIA","11001sssRRRRRRRR", ARM_V4|ARM_INTEGER },
    { "LDR", "0110144444nnnddd", ARM_V4|ARM_INTEGER },
    { "LDR", "0101100mmmnnnddd", ARM_V4|ARM_INTEGER },
    { "LDR", "01001ddd44444444", ARM_V4|ARM_INTEGER|ARM_USE_PC|ARM_HAS_RN },
    { "LDR", "10011ddd44444444", ARM_V4|ARM_INTEGER|ARM_USE_SP|ARM_HAS_RN },
    { "LDRB","01111iiiiinnnddd", ARM_V4|ARM_INTEGER },
    { "LDRB","0101110mmmnnnddd", ARM_V4|ARM_INTEGER },
    { "LDRH","1000122222nnnddd", ARM_V4|ARM_INTEGER },
    { "LDRH","0101101mmmnnnddd", ARM_V4|ARM_INTEGER },
    {"LDRSB","0101011mmmnnnddd", ARM_V4|ARM_INTEGER },
    {"LDRSH","0101111mmmnnnddd", ARM_V4|ARM_INTEGER },
    { "LSL", "00000iiiiisssddd", ARM_V4|ARM_INTEGER },
    { "LSL", "0100000010sssddd", ARM_V4|ARM_INTEGER },
    { "LSR", "00001iiiiisssddd", ARM_V4|ARM_INTEGER },
    { "LSR", "0100000011sssddd", ARM_V4|ARM_INTEGER },
    { "MOV", "00100dddiiiiiiii", ARM_V4|ARM_INTEGER },
    { "MOV", "0001110000sssddd", ARM_V4|ARM_INTEGER },
    { "MOV", "01000110SDsssddd", ARM_V4|ARM_INTEGER },
    { "MUL", "0100001101sssddd", ARM_V4|ARM_INTEGER },
    { "MVN", "0100001111sssddd", ARM_V4|ARM_INTEGER },
    { "NEG", "0100001001sssddd", ARM_V4|ARM_INTEGER },
    { "ORR", "0100001100sssddd", ARM_V4|ARM_INTEGER },
    { "POP", "1011110RRRRRRRRR", ARM_V4|ARM_INTEGER },
    { "PUSH","1011010RRRRRRRRR", ARM_V4|ARM_INTEGER },
    { "ROR", "0100000111sssddd", ARM_V4|ARM_INTEGER },
    { "SBC", "0100000110sssddd", ARM_V4|ARM_INTEGER },
    {"STMIA","11000sssRRRRRRRR", ARM_V4|ARM_INTEGER },
    { "STR", "0110044444nnnddd", ARM_V4|ARM_INTEGER },
    { "STR", "0101000mmmnnnddd", ARM_V4|ARM_INTEGER },
    { "STR", "10010ddd44444444", ARM_V4|ARM_INTEGER },
    { "STRB","01110iiiiinnnddd", ARM_V4|ARM_INTEGER },
    { "STRB","0101010mmmnnnddd", ARM_V4|ARM_INTEGER },
    { "STRH","1000022222nnnddd", ARM_V4|ARM_INTEGER },
    { "STRH","0101001mmmnnnddd", ARM_V4|ARM_INTEGER },
    { "SUB", "0001111iiisssddd", ARM_V4|ARM_INTEGER },
    { "SUB", "00111dddiiiiiiii", ARM_V4|ARM_INTEGER },
    { "SUB", "0001101mmmsssddd", ARM_V4|ARM_INTEGER },
    { "SUB", "101100001iiiiiii", ARM_V4|ARM_INTEGER },
    { "SWI", "11011111iiiiiiii", ARM_V4|ARM_INTEGER },
    { "TST", "0100001000sssmmm", ARM_V4|ARM_INTEGER }
};

const char * arm_reg_name[] =
{
   "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7",
   "R8", "R9", "R10", "R11", "R12", "R13", "R14", "PC",
};

extern const char *armCCnames[16];

#define READ_IMM(chr)\
{\
	val=0;\
	idx=0;\
	idx=strrchr(msk,chr)-msk;\
	for(i=0;i<16;i++)\
	    if(msk[i]==chr)\
		val |= (opcode&(1<<(15-i)));\
	val>>=(15-idx);\
}

#define PARSE_IMM(chr,smul)\
    p=strchr(msk,chr);\
    if(p)\
    {\
	READ_IMM(chr);\
	if(prev) strcat(dret->str,",");\
	strcat(dret->str,"#");\
	disAppendDigits(dret->str,ulShift,APREF_USE_TYPE,2,&val,DISARG_WORD);\
	if(smul) strcat(dret->str,smul);\
	prev=1;\
    }

static void __FASTCALL__ arm16EncodeTail(DisasmRet *dret,tUInt16 opcode,__filesize_t ulShift,const char *msk,long flags)
{
    unsigned i,idx,val,prev,bracket;
    int s,d,m,n;
    char *p; 
    s=d=m=n=0;
    p=strchr(msk,'S'); if(p) { idx=p-msk; s=(opcode>>(15-idx))&0x1; }
    p=strchr(msk,'D'); if(p) { idx=p-msk; d=(opcode>>(15-idx))&0x1; }
    p=strchr(msk,'M'); if(p) { idx=p-msk; m=(opcode>>(15-idx))&0x1; }
    p=strchr(msk,'N'); if(p) { idx=p-msk; n=(opcode>>(15-idx))&0x1; }
    prev=0;
    bracket=0;
    p=strchr(msk,'d');
    if(p)
    {
	READ_IMM('d');
	strcat(dret->str,arm_reg_name[(val&0xF)+(d?8:0)]);
	prev=1;
    }
    p=strchr(msk,'s');
    if(p)
    {
	READ_IMM('s');
	if(prev) strcat(dret->str,","); prev=1;
	strcat(dret->str,arm_reg_name[(val&0xF)+(s?8:0)]);
    }
    p=strchr(msk,'n');
    if(p)
    {
	READ_IMM('n');
	if(prev) strcat(dret->str,",["); prev=1;
	strcat(dret->str,arm_reg_name[(val&0xF)+(n?8:0)]);
	bracket=1;
    }
    if(!bracket && (flags & ARM_HAS_RN))
    {
	if(prev) strcat(dret->str,",["); prev=1;
	bracket=1;
    }
    if(flags & ARM_USE_PC)
    {
	if(prev && !bracket) strcat(dret->str,","); prev=1;
	strcat(dret->str, "PC");
    }
    if(flags & ARM_USE_SP)
    {
	if(prev && !bracket) strcat(dret->str,","); prev=1;
	strcat(dret->str, "SP");
    }
    p=strchr(msk,'m');
    if(p)
    {
	READ_IMM('m');
	if(prev) strcat(dret->str,",");
	strcat(dret->str,arm_reg_name[(val&0xF)+(m?8:0)]);
	prev=1;
    }
    PARSE_IMM('i',NULL);
    PARSE_IMM('2',"*2");
    PARSE_IMM('4',"*4");
    p=strchr(msk,'o');
    if(p)
    {
	tUInt32 tbuff;
	unsigned hh=0;
	p=strchr(msk,'H');
	if(p)
	{
	    READ_IMM('H');
	    hh=val;
	}
	READ_IMM('o');
	tbuff=val;
	if(prev) strcat(dret->str,",");
	if(hh==3)
	    tbuff=tbuff<<1;
	else if(hh==2)
	    tbuff=tbuff<<12;
	else if(hh==1)
	    tbuff=(tbuff<<1)&0xfffffffc;
	disAppendFAddr(dret->str,ulShift+1,(long)tbuff,ulShift+tbuff,
			DISADR_NEAR32,0,2);
	prev=1;
    }
    p=strchr(msk,'R');
    if(p)
    {
	int prevv;
	if(prev) strcat(dret->str,"!,{");
	READ_IMM('R');
	prevv=0;
	for(i=0;i<16;i++)
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
    if(bracket) strcat(dret->str,"]");
}

void __FASTCALL__ arm16Disassembler(DisasmRet *dret,__filesize_t ulShift,
					tUInt16 opcode, unsigned flags)
{
    int done;
    unsigned i,n;
    n = sizeof(opcode_table)/sizeof(arm_opcode16);
    done=0;
    for(i=0;i<n;i++)
    {
	if((opcode & opcode_table[i].bmsk) == opcode_table[i].bits)
	{
	    unsigned idx,val;
	    char *p;
	    strcpy(dret->str,opcode_table[i].name);
	    p=strchr(opcode_table[i].mask,'c');
	    if(p)
	    {
		idx=p-opcode_table[i].mask;
		val=(opcode>>(11-idx))&0xF;
		strcat(dret->str,armCCnames[val]);	
	    }
	    TabSpace(dret->str,TAB_POS);
	    arm16EncodeTail(dret,opcode,ulShift,opcode_table[i].mask,opcode_table[i].flags);
	    dret->pro_clone=opcode_table[i].flags;
	    done=1;
	    break;
	}
    }
    if(!done)
    {
	strcpy(dret->str,"???");
	TabSpace(dret->str,TAB_POS);
	disAppendDigits(dret->str,ulShift,APREF_USE_TYPE,2,&opcode,DISARG_WORD);
    }
}

void __FASTCALL__ arm16Init(void)
{
    unsigned i,n,j;
    n = sizeof(opcode_table)/sizeof(arm_opcode16);
    for(i=0;i<n;i++)
    {
	opcode_table[i].bmsk=0;
	opcode_table[i].bits=0;
	for(j=0;j<16;j++)
	{
	    if(opcode_table[i].mask[j]=='0' || opcode_table[i].mask[j]=='1')
	    {
		opcode_table[i].bmsk|=(1<<(15-j));
		opcode_table[i].bits|=(opcode_table[i].mask[j]=='1'?1<<(15-j):0<<(15-j));
	    }
	}
    }
}

void __FASTCALL__ arm16Term(void) {}
