/**
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/java.c
 * @brief       This file contains implementation of Java disassembler.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       2004
 * @note        Development, fixes and improvements
**/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "reg_form.h"
#include "plugins/disasm.h"
#include "bconsole.h"
#include "biewhelp.h"
#include "biewutil.h"
#include "bin_util.h"
#include "reg_form.h"
#include "biewlib/file_ini.h"
#include "biewlib/pmalloc.h"

#define TAB_POS 10

typedef struct java_codes_s
{
    char * name;
    void * decoder;
#define JVM_TAIL	0x000000FFUL /* n-bytes of insn's tail */
#define JVM_OBJREF1	0x00000100UL /* insns refers object as 1-byte idx */
#define JVM_OBJREF2	0x00000200UL /* insns refers object as 2-byte idx */
#define JVM_OBJREF4	0x00000300UL /* insns refers object as 4-byte idx */
#define JVM_OBJREFMASK	0x00000300UL
#define JVM_CONST1	0x00000400UL /* insns hass const as idx,1-byte const */
#define JVM_CONST2	0x00000800UL /* insns hass const as idx,2-byte const */
#define JVM_CONST4	0x00000C00UL /* insns hass const as idx,4-byte const */
#define JVM_CONSTMASK	0x00000C00UL
#define JVM_CODEREF	0x20000000UL /* insns refers relative jump */
    unsigned long flags;
}java_codes_t;


java_codes_t java_codes[256]=
{
  /*0x00*/ { "nop", NULL, 0 },
  /*0x01*/ { "aconst_null", NULL, 0 },
  /*0x02*/ { "iconst_ml", NULL, 0 },
  /*0x03*/ { "iconst_0", NULL, 0 },
  /*0x04*/ { "iconst_1", NULL, 0 },
  /*0x05*/ { "iconst_2", NULL, 0 },
  /*0x06*/ { "iconst_3", NULL, 0 },
  /*0x07*/ { "iconst_4", NULL, 0 },
  /*0x08*/ { "iconst_5", NULL, 0 },
  /*0x09*/ { "lconst_0", NULL, 0 },
  /*0x0A*/ { "lconst_1", NULL, 0 },
  /*0x0B*/ { "fconst_0", NULL, 0 },
  /*0x0C*/ { "fconst_1", NULL, 0 },
  /*0x0D*/ { "fconst_2", NULL, 0 },
  /*0x0E*/ { "dconst_0", NULL, 0 },
  /*0x0F*/ { "dconst_1", NULL, 0 },
  /*0x10*/ { "bipush", NULL, 0x01 },
  /*0x11*/ { "sipush", NULL, 0x02 },
  /*0x12*/ { "ldc", NULL, 0x01 | JVM_OBJREF1 },
  /*0x13*/ { "ldc_w", NULL, 0x02 | JVM_OBJREF2 },
  /*0x14*/ { "ldc2_w", NULL, 0x02 | JVM_OBJREF2 },
  /*0x15*/ { "iload", NULL, 0x01 | JVM_OBJREF1 },
  /*0x16*/ { "lload", NULL, 0x01 | JVM_OBJREF1 },
  /*0x17*/ { "fload", NULL, 0x01 | JVM_OBJREF1 },
  /*0x18*/ { "dload", NULL, 0x01 | JVM_OBJREF1 },
  /*0x19*/ { "aload", NULL, 0x01 | JVM_OBJREF1 },
  /*0x1A*/ { "iload_0", NULL, 0 },
  /*0x1B*/ { "iload_1", NULL, 0 },
  /*0x1C*/ { "iload_2", NULL, 0 },
  /*0x1D*/ { "iload_3", NULL, 0 },
  /*0x1E*/ { "lload_0", NULL, 0 },
  /*0x1F*/ { "lload_1", NULL, 0 },
  /*0x20*/ { "lload_2", NULL, 0 },
  /*0x21*/ { "lload_3", NULL, 0 },
  /*0x22*/ { "fload_0", NULL, 0 },
  /*0x23*/ { "fload_1", NULL, 0 },
  /*0x24*/ { "fload_2", NULL, 0 },
  /*0x25*/ { "fload_3", NULL, 0 },
  /*0x26*/ { "dload_0", NULL, 0 },
  /*0x27*/ { "dload_1", NULL, 0 },
  /*0x28*/ { "dload_2", NULL, 0 },
  /*0x29*/ { "dload_3", NULL, 0 },
  /*0x2A*/ { "aload_0", NULL, 0 },
  /*0x2B*/ { "aload_1", NULL, 0 },
  /*0x2C*/ { "aload_2", NULL, 0 },
  /*0x2D*/ { "aload_3", NULL, 0 },
  /*0x2E*/ { "iaload", NULL, 0 },
  /*0x2F*/ { "laload", NULL, 0 },
  /*0x30*/ { "faload", NULL, 0 },
  /*0x31*/ { "daload", NULL, 0 },
  /*0x32*/ { "aaload", NULL, 0 },
  /*0x33*/ { "baload", NULL, 0 },
  /*0x34*/ { "caload", NULL, 0 },
  /*0x35*/ { "saload", NULL, 0 },
  /*0x36*/ { "istore", NULL, 0x01 | JVM_OBJREF1 },
  /*0x37*/ { "lstore", NULL, 0x01 | JVM_OBJREF1 },
  /*0x38*/ { "fstore", NULL, 0x01 | JVM_OBJREF1 },
  /*0x39*/ { "dstore", NULL, 0x01 | JVM_OBJREF1 },
  /*0x3A*/ { "astore", NULL, 0x01 | JVM_OBJREF1 },
  /*0x3B*/ { "istore_0", NULL, 0 },
  /*0x3C*/ { "istore_1", NULL, 0 },
  /*0x3D*/ { "istore_2", NULL, 0 },
  /*0x3E*/ { "istore_3", NULL, 0 },
  /*0x3F*/ { "lstore_0", NULL, 0 },
  /*0x40*/ { "lstore_1", NULL, 0 },
  /*0x41*/ { "lstore_2", NULL, 0 },
  /*0x42*/ { "lstore_3", NULL, 0 },
  /*0x43*/ { "fstore_0", NULL, 0 },
  /*0x44*/ { "fstore_1", NULL, 0 },
  /*0x45*/ { "fstore_2", NULL, 0 },
  /*0x46*/ { "fstore_3", NULL, 0 },
  /*0x47*/ { "dstore_0", NULL, 0 },
  /*0x48*/ { "dstore_1", NULL, 0 },
  /*0x49*/ { "dstore_2", NULL, 0 },
  /*0x4A*/ { "dstore_3", NULL, 0 },
  /*0x4B*/ { "astore_0", NULL, 0 },
  /*0x4C*/ { "astore_1", NULL, 0 },
  /*0x4D*/ { "astore_2", NULL, 0 },
  /*0x4E*/ { "astore_3", NULL, 0 },
  /*0x4F*/ { "iastore", NULL, 0 },
  /*0x50*/ { "lastore", NULL, 0 },
  /*0x51*/ { "fastore", NULL, 0 },
  /*0x52*/ { "dastore", NULL, 0 },
  /*0x53*/ { "aastore", NULL, 0 },
  /*0x54*/ { "bastore", NULL, 0 },
  /*0x55*/ { "castore", NULL, 0 },
  /*0x56*/ { "sastore", NULL, 0 },
  /*0x57*/ { "pop", NULL, 0 },
  /*0x58*/ { "pop2", NULL, 0 },
  /*0x59*/ { "dup", NULL, 0 },
  /*0x5A*/ { "dup_x1", NULL, 0 },
  /*0x5B*/ { "dup_x2", NULL, 0 },
  /*0x5C*/ { "dup2", NULL, 0 },
  /*0x5D*/ { "dup2_x1", NULL, 0 },
  /*0x5E*/ { "dup2_x2", NULL, 0 },
  /*0x5F*/ { "swap", NULL, 0 },
  /*0x60*/ { "iadd", NULL, 0 },
  /*0x61*/ { "ladd", NULL, 0 },
  /*0x62*/ { "fadd", NULL, 0 },
  /*0x63*/ { "dadd", NULL, 0 },
  /*0x64*/ { "isub", NULL, 0 },
  /*0x65*/ { "lsub", NULL, 0 },
  /*0x66*/ { "fsub", NULL, 0 },
  /*0x67*/ { "dsub", NULL, 0 },
  /*0x68*/ { "imul", NULL, 0 },
  /*0x69*/ { "lmul", NULL, 0 },
  /*0x6A*/ { "fmul", NULL, 0 },
  /*0x6B*/ { "dmul", NULL, 0 },
  /*0x6C*/ { "idiv", NULL, 0 },
  /*0x6D*/ { "ldiv", NULL, 0 },
  /*0x6E*/ { "fdiv", NULL, 0 },
  /*0x6F*/ { "ddiv", NULL, 0 },
  /*0x70*/ { "irem", NULL, 0 },
  /*0x71*/ { "lrem", NULL, 0 },
  /*0x72*/ { "frem", NULL, 0 },
  /*0x73*/ { "drem", NULL, 0 },
  /*0x74*/ { "ineg", NULL, 0 },
  /*0x75*/ { "lneg", NULL, 0 },
  /*0x76*/ { "fneg", NULL, 0 },
  /*0x77*/ { "dneg", NULL, 0 },
  /*0x78*/ { "ishl", NULL, 0 },
  /*0x79*/ { "lshl", NULL, 0 },
  /*0x7A*/ { "ishr", NULL, 0 },
  /*0x7B*/ { "lshr", NULL, 0 },
  /*0x7C*/ { "iushr", NULL, 0 },
  /*0x7D*/ { "lushr", NULL, 0 },
  /*0x7E*/ { "iand", NULL, 0 },
  /*0x7F*/ { "land", NULL, 0 },
  /*0x80*/ { "ior", NULL, 0 },
  /*0x81*/ { "lor", NULL, 0 },
  /*0x82*/ { "ixor", NULL, 0 },
  /*0x83*/ { "lxor", NULL, 0 },
  /*0x84*/ { "iinc", NULL, 0x02 | JVM_OBJREF1 | JVM_CONST1 },
  /*0x85*/ { "i2l", NULL, 0 },
  /*0x86*/ { "i2f", NULL, 0 },
  /*0x87*/ { "i2d", NULL, 0 },
  /*0x88*/ { "l2i", NULL, 0 },
  /*0x89*/ { "l2f", NULL, 0 },
  /*0x8A*/ { "l2d", NULL, 0 },
  /*0x8B*/ { "f2i", NULL, 0 },
  /*0x8C*/ { "f2l", NULL, 0 },
  /*0x8D*/ { "f2d", NULL, 0 },
  /*0x8E*/ { "d2i", NULL, 0 },
  /*0x8F*/ { "d2l", NULL, 0 },
  /*0x90*/ { "d2f", NULL, 0 },
  /*0x91*/ { "i2b", NULL, 0 },
  /*0x92*/ { "i2c", NULL, 0 },
  /*0x93*/ { "i2s", NULL, 0 },
  /*0x94*/ { "lcmp", NULL, 0 },
  /*0x95*/ { "fcmpl", NULL, 0 },
  /*0x96*/ { "fcmpg", NULL, 0 },
  /*0x97*/ { "dcmpl", NULL, 0 },
  /*0x98*/ { "dcmpg", NULL, 0 },
  /*0x99*/ { "ifeq", NULL, 0x02 | JVM_CODEREF },
  /*0x9A*/ { "ifne", NULL, 0x02 | JVM_CODEREF },
  /*0x9B*/ { "iflt", NULL, 0x02 | JVM_CODEREF },
  /*0x9C*/ { "ifge", NULL, 0x02 | JVM_CODEREF },
  /*0x9D*/ { "ifgt", NULL, 0x02 | JVM_CODEREF },
  /*0x9E*/ { "ifle", NULL, 0x02 | JVM_CODEREF },
  /*0x9F*/ { "if_icmpeq", NULL, 0x02 | JVM_CODEREF },
  /*0xA0*/ { "if_icmpne", NULL, 0x02 | JVM_CODEREF },
  /*0xA1*/ { "if_icmplt", NULL, 0x02 | JVM_CODEREF },
  /*0xA2*/ { "if_icmpge", NULL, 0x02 | JVM_CODEREF },
  /*0xA3*/ { "if_icmpgt", NULL, 0x02 | JVM_CODEREF },
  /*0xA4*/ { "if_icmple", NULL, 0x02 | JVM_CODEREF },
  /*0xA5*/ { "if_acmpne", NULL, 0x02 | JVM_CODEREF },
  /*0xA6*/ { "if_acmpeq", NULL, 0x02 | JVM_CODEREF },
  /*0xA7*/ { "goto", NULL, 0x02 | JVM_CODEREF },
  /*0xA8*/ { "jsr", NULL, 0x02 | JVM_CODEREF },
  /*0xA9*/ { "ret", NULL, 0x01 },
  /*0xAA*/ { "tableswitch", NULL, 0x13 /*!!!*/ },
  /*0xAB*/ { "lookupswitch", NULL, 0x09 /*!!!*/ },
  /*0xAC*/ { "ireturn", NULL, 0 },
  /*0xAD*/ { "lreturn", NULL, 0 },
  /*0xAE*/ { "freturn", NULL, 0 },
  /*0xAF*/ { "dreturn", NULL, 0 },
  /*0xB0*/ { "areturn", NULL, 0 },
  /*0xB1*/ { "return", NULL, 0 },
  /*0xB2*/ { "getstatic", NULL, 0x02 | JVM_OBJREF2 },
  /*0xB3*/ { "putstatic", NULL, 0x02 | JVM_OBJREF2 },
  /*0xB4*/ { "getfield", NULL, 0x02 | JVM_OBJREF2 },
  /*0xB5*/ { "putfield", NULL, 0x02 | JVM_OBJREF2 },
  /*0xB6*/ { "invokevirtual", NULL, 0x02 | JVM_OBJREF2 },
  /*0xB7*/ { "invokespecial", NULL, 0x02 | JVM_OBJREF2 },
  /*0xB8*/ { "invokestatic", NULL, 0x02 | JVM_OBJREF2 },
  /*0xB9*/ { "invokeinterface", NULL, 0x04 | JVM_OBJREF2 | JVM_CONST1 },
  /*0xBA*/ { "???", NULL, 0 },
  /*0xBB*/ { "new", NULL, 0x02 | JVM_OBJREF2 },
  /*0xBC*/ { "newarray", NULL, 0x01 /* decode data type !!! */},
  /*0xBD*/ { "anewarray", NULL, 0x02 },
  /*0xBE*/ { "arraylength", NULL, 0 },
  /*0xBF*/ { "athrow", NULL, 0 },
  /*0xC0*/ { "checkcast", NULL, 0x02 | JVM_OBJREF2 },
  /*0xC1*/ { "instanceof", NULL, 0x02 | JVM_OBJREF2 },
  /*0xC2*/ { "monitorenter", NULL, 0 },
  /*0xC3*/ { "monitorexit", NULL, 0 },
  /*0xC4*/ { "wide", NULL, 0/*!!! <- multiplier 2 for operands */ },
  /*0xC5*/ { "multianewarray", NULL, 0x03 | JVM_OBJREF2 | JVM_CONST2 },
  /*0xC6*/ { "ifnull", NULL, 0x02 | JVM_CODEREF },
  /*0xC7*/ { "ifnonnull", NULL, 0x02 | JVM_CODEREF },
  /*0xC8*/ { "goto_w", NULL, 0x04 | JVM_CODEREF },
  /*0xC9*/ { "jsr_w", NULL, 0x04 | JVM_CODEREF },
  /*0xCA*/ { "breakpoint", NULL, 0 },
  /*0xCB*/ { "???", NULL, 0 },
  /*0xCC*/ { "???", NULL, 0 },
  /*0xCD*/ { "???", NULL, 0 },
  /*0xCE*/ { "???", NULL, 0 },
  /*0xCF*/ { "???", NULL, 0 },
  /*0xD0*/ { "???", NULL, 0 },
  /*0xD1*/ { "???", NULL, 0 },
  /*0xD2*/ { "???", NULL, 0 },
  /*0xD3*/ { "???", NULL, 0 },
  /*0xD4*/ { "???", NULL, 0 },
  /*0xD5*/ { "???", NULL, 0 },
  /*0xD6*/ { "???", NULL, 0 },
  /*0xD7*/ { "???", NULL, 0 },
  /*0xD8*/ { "???", NULL, 0 },
  /*0xD9*/ { "???", NULL, 0 },
  /*0xDA*/ { "???", NULL, 0 },
  /*0xDB*/ { "???", NULL, 0 },
  /*0xDC*/ { "???", NULL, 0 },
  /*0xDD*/ { "???", NULL, 0 },
  /*0xDE*/ { "???", NULL, 0 },
  /*0xDF*/ { "???", NULL, 0 },
  /*0xE0*/ { "???", NULL, 0 },
  /*0xE1*/ { "???", NULL, 0 },
  /*0xE2*/ { "???", NULL, 0 },
  /*0xE3*/ { "???", NULL, 0 },
  /*0xE4*/ { "???", NULL, 0 },
  /*0xE5*/ { "???", NULL, 0 },
  /*0xE6*/ { "???", NULL, 0 },
  /*0xE7*/ { "???", NULL, 0 },
  /*0xE8*/ { "???", NULL, 0 },
  /*0xE9*/ { "???", NULL, 0 },
  /*0xEA*/ { "???", NULL, 0 },
  /*0xEB*/ { "???", NULL, 0 },
  /*0xEC*/ { "???", NULL, 0 },
  /*0xED*/ { "???", NULL, 0 },
  /*0xEE*/ { "???", NULL, 0 },
  /*0xEF*/ { "???", NULL, 0 },
  /*0xF0*/ { "???", NULL, 0 },
  /*0xF1*/ { "???", NULL, 0 },
  /*0xF2*/ { "???", NULL, 0 },
  /*0xF3*/ { "???", NULL, 0 },
  /*0xF4*/ { "???", NULL, 0 },
  /*0xF5*/ { "???", NULL, 0 },
  /*0xF6*/ { "???", NULL, 0 },
  /*0xF7*/ { "???", NULL, 0 },
  /*0xF8*/ { "???", NULL, 0 },
  /*0xF9*/ { "???", NULL, 0 },
  /*0xFA*/ { "???", NULL, 0 },
  /*0xFB*/ { "???", NULL, 0 },
  /*0xFC*/ { "???", NULL, 0 },
  /*0xFD*/ { "???", NULL, 0 },
  /*0xFE*/ { "impdep1", NULL, 0 },
  /*0xFF*/ { "impdep2", NULL, 0 }
};

static char *outstr;

static DisasmRet __FASTCALL__ javaDisassembler(unsigned long ulShift,
                                               MBuffer buffer,
                                               unsigned flags)
{
  DisasmRet ret;
  unsigned mult,idx,tail;
  unsigned long jflags;
  memset(&ret,0,sizeof(ret));
  ret.str = outstr;
  idx=0;
  if(buffer[idx]==0xC4) { idx++; mult=2; }
  else mult=1;
  if(!((flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
		    strcpy(outstr,java_codes[buffer[idx]].name);
  jflags=java_codes[buffer[idx]].flags;
  tail=jflags & JVM_TAIL;
  idx++;
  if(tail)
  {
	if(!((flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
		TabSpace(outstr,TAB_POS);
	tail *= mult;
	if(!((flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
	{
	    switch(tail)
	    {
		default:
		case 1:
		    if(jflags & JVM_OBJREFMASK) strcat(outstr,"#");
		    strcat(outstr,Get2Digit(buffer[idx]));
		    break;
		case 2: 
		{
		    unsigned short sval;
		    unsigned long newpos;
		    sval=FMT_WORD(&buffer[idx],1);
		    if(jflags & JVM_CODEREF)
		    {
			newpos = ulShift + (signed short)sval + 3;
			disAppendFAddr(outstr,ulShift + 1,sval,
					newpos,DISADR_NEAR16,0,2);
		    }
		    else
		    if(jflags & JVM_OBJREFMASK)
		    disAppendDigits(outstr,ulShift,
			APREF_USE_TYPE,2,&sval,DISARG_WORD);
		    else strcat(outstr,Get4Digit(sval));
		    break;
		}
		case 4: 
		{
		    unsigned long lval;
		    unsigned long newpos;
		    lval=FMT_DWORD(&buffer[idx],1);
		    if(jflags & JVM_CODEREF)
		    {
			newpos = ulShift + (signed long)lval + 5;
			disAppendFAddr(outstr,ulShift + 1,lval,
					newpos,DISADR_NEAR32,0,4);
		    }
		    else
		    if(jflags & JVM_OBJREFMASK)
		    disAppendDigits(outstr,ulShift,
			APREF_USE_TYPE,4,&lval,DISARG_DWORD);
		    else strcat(outstr,Get8Digit(lval));
		    break;
		}
		case 8: 
		    disAppendDigits(outstr,ulShift,
			APREF_USE_TYPE,8,&buffer[idx],DISARG_QWORD);
		    break;
	    }
	}
  }
  if(flags & __DISF_GETTYPE) ret.pro_clone = __INSNT_ORDINAL;
  ret.codelen=tail+idx;
  return ret;
}

static void  __FASTCALL__ javaHelpAsm( void )
{
  hlpDisplay(20010);
}

static int    __FASTCALL__ javaMaxInsnLen( void ) { return 15; }
static ColorAttr __FASTCALL__ javaGetAsmColor( unsigned long clone )
{
  UNUSED(clone);
  return disasm_cset.cpu_cset[0].clone[0];
}
static int       __FASTCALL__ javaGetBitness( void ) { return DAB_USE16; }
static char      __FASTCALL__ javaGetClone( unsigned long clone )
{
  UNUSED(clone);
  return ' ';
}
static void      __FASTCALL__ javaInit( void )
{
  outstr = PMalloc(1000);
  if(!outstr)
  {
    MemOutBox("Data disassembler initialization");
    exit(EXIT_FAILURE);
  }
}

static void  __FASTCALL__ javaTerm( void )
{
   PFREE(outstr);
}

static void __FASTCALL__ javaReadIni( hIniProfile *ini )
{
    UNUSED(ini);
}

static void __FASTCALL__ javaWriteIni( hIniProfile *ini )
{
    UNUSED(ini);
}

REGISTRY_DISASM Java_Disasm =
{
  "~Java",
  { NULL, "NULL", NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL },
  javaDisassembler,
  NULL,
  javaHelpAsm,
  javaMaxInsnLen,
  javaGetAsmColor,
  NULL, 
  javaGetBitness,
  javaGetClone,
  javaInit,
  javaTerm,
  javaReadIni,
  javaWriteIni
};




