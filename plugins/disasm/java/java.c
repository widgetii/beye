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
#include "bmfile.h"
#include "reg_form.h"
#include "biewlib/file_ini.h"
#include "biewlib/pmalloc.h"

#define TAB_POS 10

typedef struct java_codes_s
{
    char * name;
#define JVM_TAIL	0x000000FFUL /* n-bytes of insn's tail */
#define JVM_OBJREF1	0x00000100UL /* insns refers object as 1-byte idx */
#define JVM_OBJREF2	0x00000200UL /* insns refers object as 2-byte idx */
#define JVM_OBJREF4	0x00000300UL /* insns refers object as 4-byte idx */
#define JVM_OBJREFMASK	0x00000300UL
#define JVM_CONST1	0x00000400UL /* insns hass const as idx,1-byte const */
#define JVM_CONST2	0x00000800UL /* insns hass const as idx,2-byte const */
#define JVM_CONST4	0x00000C00UL /* insns hass const as idx,4-byte const */
#define JVM_CONSTMASK	0x00000C00UL
#define JVM_ATYPE	0x00001000UL
#define JVM_CODEREF	0x20000000UL /* insns refers relative jump */
#define JVM_LOOKUPSWITCH 0x01000000UL /* special case : lookupswitch insn */
#define JVM_TABLESWITCH	0x02000000UL /* special case : tableswitch insn */
    unsigned long flags;
}java_codes_t;


java_codes_t java_codes[256]=
{
  /*0x00*/ { "nop", 0 },
  /*0x01*/ { "aconst_null", 0 },
  /*0x02*/ { "iconst_m1", 0 },
  /*0x03*/ { "iconst_0", 0 },
  /*0x04*/ { "iconst_1", 0 },
  /*0x05*/ { "iconst_2", 0 },
  /*0x06*/ { "iconst_3", 0 },
  /*0x07*/ { "iconst_4", 0 },
  /*0x08*/ { "iconst_5", 0 },
  /*0x09*/ { "lconst_0", 0 },
  /*0x0A*/ { "lconst_1", 0 },
  /*0x0B*/ { "fconst_0", 0 },
  /*0x0C*/ { "fconst_1", 0 },
  /*0x0D*/ { "fconst_2", 0 },
  /*0x0E*/ { "dconst_0", 0 },
  /*0x0F*/ { "dconst_1", 0 },
  /*0x10*/ { "bipush", 0x01 },
  /*0x11*/ { "sipush", 0x02 },
  /*0x12*/ { "ldc", 0x01 | JVM_OBJREF1 },
  /*0x13*/ { "ldc_w", 0x02 | JVM_OBJREF2 },
  /*0x14*/ { "ldc2_w", 0x02 | JVM_OBJREF2 },
  /*0x15*/ { "iload", 0x01 | JVM_OBJREF1 },
  /*0x16*/ { "lload", 0x01 | JVM_OBJREF1 },
  /*0x17*/ { "fload", 0x01 | JVM_OBJREF1 },
  /*0x18*/ { "dload", 0x01 | JVM_OBJREF1 },
  /*0x19*/ { "aload", 0x01 | JVM_OBJREF1 },
  /*0x1A*/ { "iload_0", 0 },
  /*0x1B*/ { "iload_1", 0 },
  /*0x1C*/ { "iload_2", 0 },
  /*0x1D*/ { "iload_3", 0 },
  /*0x1E*/ { "lload_0", 0 },
  /*0x1F*/ { "lload_1", 0 },
  /*0x20*/ { "lload_2", 0 },
  /*0x21*/ { "lload_3", 0 },
  /*0x22*/ { "fload_0", 0 },
  /*0x23*/ { "fload_1", 0 },
  /*0x24*/ { "fload_2", 0 },
  /*0x25*/ { "fload_3", 0 },
  /*0x26*/ { "dload_0", 0 },
  /*0x27*/ { "dload_1", 0 },
  /*0x28*/ { "dload_2", 0 },
  /*0x29*/ { "dload_3", 0 },
  /*0x2A*/ { "aload_0", 0 },
  /*0x2B*/ { "aload_1", 0 },
  /*0x2C*/ { "aload_2", 0 },
  /*0x2D*/ { "aload_3", 0 },
  /*0x2E*/ { "iaload", 0 },
  /*0x2F*/ { "laload", 0 },
  /*0x30*/ { "faload", 0 },
  /*0x31*/ { "daload", 0 },
  /*0x32*/ { "aaload", 0 },
  /*0x33*/ { "baload", 0 },
  /*0x34*/ { "caload", 0 },
  /*0x35*/ { "saload", 0 },
  /*0x36*/ { "istore", 0x01 | JVM_OBJREF1 },
  /*0x37*/ { "lstore", 0x01 | JVM_OBJREF1 },
  /*0x38*/ { "fstore", 0x01 | JVM_OBJREF1 },
  /*0x39*/ { "dstore", 0x01 | JVM_OBJREF1 },
  /*0x3A*/ { "astore", 0x01 | JVM_OBJREF1 },
  /*0x3B*/ { "istore_0", 0 },
  /*0x3C*/ { "istore_1", 0 },
  /*0x3D*/ { "istore_2", 0 },
  /*0x3E*/ { "istore_3", 0 },
  /*0x3F*/ { "lstore_0", 0 },
  /*0x40*/ { "lstore_1", 0 },
  /*0x41*/ { "lstore_2", 0 },
  /*0x42*/ { "lstore_3", 0 },
  /*0x43*/ { "fstore_0", 0 },
  /*0x44*/ { "fstore_1", 0 },
  /*0x45*/ { "fstore_2", 0 },
  /*0x46*/ { "fstore_3", 0 },
  /*0x47*/ { "dstore_0", 0 },
  /*0x48*/ { "dstore_1", 0 },
  /*0x49*/ { "dstore_2", 0 },
  /*0x4A*/ { "dstore_3", 0 },
  /*0x4B*/ { "astore_0", 0 },
  /*0x4C*/ { "astore_1", 0 },
  /*0x4D*/ { "astore_2", 0 },
  /*0x4E*/ { "astore_3", 0 },
  /*0x4F*/ { "iastore", 0 },
  /*0x50*/ { "lastore", 0 },
  /*0x51*/ { "fastore", 0 },
  /*0x52*/ { "dastore", 0 },
  /*0x53*/ { "aastore", 0 },
  /*0x54*/ { "bastore", 0 },
  /*0x55*/ { "castore", 0 },
  /*0x56*/ { "sastore", 0 },
  /*0x57*/ { "pop", 0 },
  /*0x58*/ { "pop2", 0 },
  /*0x59*/ { "dup", 0 },
  /*0x5A*/ { "dup_x1", 0 },
  /*0x5B*/ { "dup_x2", 0 },
  /*0x5C*/ { "dup2", 0 },
  /*0x5D*/ { "dup2_x1", 0 },
  /*0x5E*/ { "dup2_x2", 0 },
  /*0x5F*/ { "swap", 0 },
  /*0x60*/ { "iadd", 0 },
  /*0x61*/ { "ladd", 0 },
  /*0x62*/ { "fadd", 0 },
  /*0x63*/ { "dadd", 0 },
  /*0x64*/ { "isub", 0 },
  /*0x65*/ { "lsub", 0 },
  /*0x66*/ { "fsub", 0 },
  /*0x67*/ { "dsub", 0 },
  /*0x68*/ { "imul", 0 },
  /*0x69*/ { "lmul", 0 },
  /*0x6A*/ { "fmul", 0 },
  /*0x6B*/ { "dmul", 0 },
  /*0x6C*/ { "idiv", 0 },
  /*0x6D*/ { "ldiv", 0 },
  /*0x6E*/ { "fdiv", 0 },
  /*0x6F*/ { "ddiv", 0 },
  /*0x70*/ { "irem", 0 },
  /*0x71*/ { "lrem", 0 },
  /*0x72*/ { "frem", 0 },
  /*0x73*/ { "drem", 0 },
  /*0x74*/ { "ineg", 0 },
  /*0x75*/ { "lneg", 0 },
  /*0x76*/ { "fneg", 0 },
  /*0x77*/ { "dneg", 0 },
  /*0x78*/ { "ishl", 0 },
  /*0x79*/ { "lshl", 0 },
  /*0x7A*/ { "ishr", 0 },
  /*0x7B*/ { "lshr", 0 },
  /*0x7C*/ { "iushr", 0 },
  /*0x7D*/ { "lushr", 0 },
  /*0x7E*/ { "iand", 0 },
  /*0x7F*/ { "land", 0 },
  /*0x80*/ { "ior", 0 },
  /*0x81*/ { "lor", 0 },
  /*0x82*/ { "ixor", 0 },
  /*0x83*/ { "lxor", 0 },
  /*0x84*/ { "iinc", 0x02 | JVM_OBJREF1 | JVM_CONST1 },
  /*0x85*/ { "i2l", 0 },
  /*0x86*/ { "i2f", 0 },
  /*0x87*/ { "i2d", 0 },
  /*0x88*/ { "l2i", 0 },
  /*0x89*/ { "l2f", 0 },
  /*0x8A*/ { "l2d", 0 },
  /*0x8B*/ { "f2i", 0 },
  /*0x8C*/ { "f2l", 0 },
  /*0x8D*/ { "f2d", 0 },
  /*0x8E*/ { "d2i", 0 },
  /*0x8F*/ { "d2l", 0 },
  /*0x90*/ { "d2f", 0 },
  /*0x91*/ { "i2b", 0 },
  /*0x92*/ { "i2c", 0 },
  /*0x93*/ { "i2s", 0 },
  /*0x94*/ { "lcmp", 0 },
  /*0x95*/ { "fcmpl", 0 },
  /*0x96*/ { "fcmpg", 0 },
  /*0x97*/ { "dcmpl", 0 },
  /*0x98*/ { "dcmpg", 0 },
  /*0x99*/ { "ifeq", 0x02 | JVM_CODEREF },
  /*0x9A*/ { "ifne", 0x02 | JVM_CODEREF },
  /*0x9B*/ { "iflt", 0x02 | JVM_CODEREF },
  /*0x9C*/ { "ifge", 0x02 | JVM_CODEREF },
  /*0x9D*/ { "ifgt", 0x02 | JVM_CODEREF },
  /*0x9E*/ { "ifle", 0x02 | JVM_CODEREF },
  /*0x9F*/ { "if_icmpeq", 0x02 | JVM_CODEREF },
  /*0xA0*/ { "if_icmpne", 0x02 | JVM_CODEREF },
  /*0xA1*/ { "if_icmplt", 0x02 | JVM_CODEREF },
  /*0xA2*/ { "if_icmpge", 0x02 | JVM_CODEREF },
  /*0xA3*/ { "if_icmpgt", 0x02 | JVM_CODEREF },
  /*0xA4*/ { "if_icmple", 0x02 | JVM_CODEREF },
  /*0xA5*/ { "if_acmpne", 0x02 | JVM_CODEREF },
  /*0xA6*/ { "if_acmpeq", 0x02 | JVM_CODEREF },
  /*0xA7*/ { "goto", 0x02 | JVM_CODEREF },
  /*0xA8*/ { "jsr", 0x02 | JVM_CODEREF },
  /*0xA9*/ { "ret", 0x01 },
  /*0xAA*/ { "tableswitch", JVM_TABLESWITCH },
  /*0xAB*/ { "lookupswitch", JVM_LOOKUPSWITCH },
  /*0xAC*/ { "ireturn", 0 },
  /*0xAD*/ { "lreturn", 0 },
  /*0xAE*/ { "freturn", 0 },
  /*0xAF*/ { "dreturn", 0 },
  /*0xB0*/ { "areturn", 0 },
  /*0xB1*/ { "return", 0 },
  /*0xB2*/ { "getstatic", 0x02 | JVM_OBJREF2 },
  /*0xB3*/ { "putstatic", 0x02 | JVM_OBJREF2 },
  /*0xB4*/ { "getfield", 0x02 | JVM_OBJREF2 },
  /*0xB5*/ { "putfield", 0x02 | JVM_OBJREF2 },
  /*0xB6*/ { "invokevirtual", 0x02 | JVM_OBJREF2 },
  /*0xB7*/ { "invokespecial", 0x02 | JVM_OBJREF2 },
  /*0xB8*/ { "invokestatic", 0x02 | JVM_OBJREF2 },
  /*0xB9*/ { "invokeinterface", 0x04 | JVM_OBJREF2 | JVM_CONST1 },
  /*0xBA*/ { "???", 0 }, /* is not used by historical reasons */
  /*0xBB*/ { "new", 0x02 | JVM_OBJREF2 },
  /*0xBC*/ { "newarray", 0x01 | JVM_ATYPE /* decode data type !!! */},
  /*0xBD*/ { "anewarray", 0x02 },
  /*0xBE*/ { "arraylength", 0 },
  /*0xBF*/ { "athrow", 0 },
  /*0xC0*/ { "checkcast", 0x02 | JVM_OBJREF2 },
  /*0xC1*/ { "instanceof", 0x02 | JVM_OBJREF2 },
  /*0xC2*/ { "monitorenter", 0 },
  /*0xC3*/ { "monitorexit", 0 },
  /*0xC4*/ { "wide", 0/*!!! <- multiplier 2 for operands (prefix) */ },
  /*0xC5*/ { "multianewarray", 0x03 | JVM_OBJREF2 | JVM_CONST1 },
  /*0xC6*/ { "ifnull", 0x02 | JVM_CODEREF },
  /*0xC7*/ { "ifnonnull", 0x02 | JVM_CODEREF },
  /*0xC8*/ { "goto_w", 0x04 | JVM_CODEREF },
  /*0xC9*/ { "jsr_w", 0x04 | JVM_CODEREF },
  /*0xCA*/ { "breakpoint", 0 },
  /*0xCB*/ { "???", 0 },
  /*0xCC*/ { "???", 0 },
  /*0xCD*/ { "???", 0 },
  /*0xCE*/ { "???", 0 },
  /*0xCF*/ { "???", 0 },
  /*0xD0*/ { "???", 0 },
  /*0xD1*/ { "???", 0 },
  /*0xD2*/ { "???", 0 },
  /*0xD3*/ { "???", 0 },
  /*0xD4*/ { "???", 0 },
  /*0xD5*/ { "???", 0 },
  /*0xD6*/ { "???", 0 },
  /*0xD7*/ { "???", 0 },
  /*0xD8*/ { "???", 0 },
  /*0xD9*/ { "???", 0 },
  /*0xDA*/ { "???", 0 },
  /*0xDB*/ { "???", 0 },
  /*0xDC*/ { "???", 0 },
  /*0xDD*/ { "???", 0 },
  /*0xDE*/ { "???", 0 },
  /*0xDF*/ { "???", 0 },
  /*0xE0*/ { "???", 0 },
  /*0xE1*/ { "???", 0 },
  /*0xE2*/ { "???", 0 },
  /*0xE3*/ { "???", 0 },
  /*0xE4*/ { "???", 0 },
  /*0xE5*/ { "???", 0 },
  /*0xE6*/ { "???", 0 },
  /*0xE7*/ { "???", 0 },
  /*0xE8*/ { "???", 0 },
  /*0xE9*/ { "???", 0 },
  /*0xEA*/ { "???", 0 },
  /*0xEB*/ { "???", 0 },
  /*0xEC*/ { "???", 0 },
  /*0xED*/ { "???", 0 },
  /*0xEE*/ { "???", 0 },
  /*0xEF*/ { "???", 0 },
  /*0xF0*/ { "???", 0 },
  /*0xF1*/ { "???", 0 },
  /*0xF2*/ { "???", 0 },
  /*0xF3*/ { "???", 0 },
  /*0xF4*/ { "???", 0 },
  /*0xF5*/ { "???", 0 },
  /*0xF6*/ { "???", 0 },
  /*0xF7*/ { "???", 0 },
  /*0xF8*/ { "???", 0 },
  /*0xF9*/ { "???", 0 },
  /*0xFA*/ { "???", 0 },
  /*0xFB*/ { "???", 0 },
  /*0xFC*/ { "???", 0 },
  /*0xFD*/ { "???", 0 },
  /*0xFE*/ { "impdep1", 0 },
  /*0xFF*/ { "impdep2", 0 }
};

static char *outstr;
static unsigned vartail=0;
static __filesize_t vartail_base=0, vartail_start=0, vartail_flags, vartail_idx;
static DisasmRet __FASTCALL__ javaDisassembler(__filesize_t ulShift,
                                               MBuffer buffer,
                                               unsigned flags)
{
  DisasmRet ret;
  unsigned mult,idx,tail,npadds;
  __filesize_t prev_pa,next_pa;
  unsigned long jflags;
  memset(&ret,0,sizeof(ret));
  ret.str = outstr;
  idx=0;
  if(buffer[idx]==0xC4)
  {
    idx++;
    mult=2;
  }
  else mult=1;
  jflags=java_codes[buffer[idx]].flags;
  tail=jflags & JVM_TAIL;
  tail *= mult;
  if(flags & __DISF_GETTYPE)
  {
    ret.pro_clone = __INSNT_ORDINAL;
    ret.codelen=ret.field=0;
    if(!(vartail && ulShift>=vartail_start && ulShift<vartail_start+vartail))
    switch(buffer[idx])
    {
	case 0xA9:
	case 0xAC:
	case 0xAD:
	case 0xAE:
	case 0xAF:
	case 0xB0:
	case 0xB1:  ret.pro_clone = __INSNT_RET;
		    ret.codelen = tail;
		    ret.field = tail?idx+1:0;
		    break;
	default: break;
    }
    return ret;
  }
  if(vartail && ulShift>=vartail_start && ulShift<vartail_start+vartail)
  {	
	/* we are in vartail */
	ret.codelen=vartail_flags & JVM_TABLESWITCH ? 4 : 8;
	if(!((flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
	{
		__filesize_t newpos;
		unsigned long cval,lval;
		if(vartail_flags & JVM_TABLESWITCH)
		{
			strcpy(outstr,"case ");
			strcat(outstr,Get8Digit((ulShift-vartail_start)/4+vartail_idx));
			strcat(outstr,":");
			lval=FMT_DWORD(buffer, 1);
			newpos=vartail_base+lval;
			if(lval!=newpos)
				disAppendFAddr(outstr,ulShift,lval,
						newpos,DISADR_NEAR32,0,4);
				else
				    strcat(outstr,Get8Digit(newpos));
		}
		else
		{
			strcpy(outstr,"case ");
			cval=FMT_DWORD(buffer, 1);
			strcat(outstr,Get8Digit(cval));
			strcat(outstr,":");
			lval=FMT_DWORD(&buffer[4], 1);
			newpos=vartail_base+lval;
			if(lval!=newpos)
				disAppendFAddr(outstr,ulShift,lval,
						newpos,DISADR_NEAR32,0,4);
				else
				    strcat(outstr,Get8Digit(newpos));
		}
	}
     return ret;
  }
  if(jflags & JVM_LOOKUPSWITCH || jflags & JVM_TABLESWITCH)
  {
    char prev_func[80];
    unsigned func_class;
    vartail_flags=jflags;
    /* Sorry! We really need to know method offset to compute padding bytes */
    if(detectedFormat->GetPubSym)
    {
	prev_pa = detectedFormat->GetPubSym(prev_func,sizeof(prev_func),
					    &func_class,ulShift,True);
        next_pa = detectedFormat->GetPubSym(prev_func,sizeof(prev_func),
                                            &func_class,ulShift,False);
    }
    else next_pa=prev_pa=0;
    if(!next_pa) next_pa=bmGetFLength();
    if(!(prev_pa%4)) npadds = (ulShift+1-prev_pa)%4; /* align only if method is aligned */
    else npadds=0;
    vartail_base=ulShift;
    if(jflags & JVM_LOOKUPSWITCH)
    {
	tail=8+npadds;
	vartail=FMT_DWORD(&buffer[idx+1+npadds+4],1)*8;
	vartail_start=tail+ulShift;
	if(prev_pa && vartail_start+vartail<=next_pa);
	else vartail=0;
    }
    else
    {
	unsigned long hi,lo;
	tail=12+npadds;
	lo=FMT_DWORD(&buffer[idx+1+npadds+4],1);
	hi=FMT_DWORD(&buffer[idx+1+npadds+8],1);
	vartail=(hi-lo+1)*4;
	vartail_start=tail+ulShift;
	vartail_idx=lo;
	if(hi>lo && prev_pa && vartail_start+vartail<=next_pa);
	else vartail=0;
    }
  }
  if(!((flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
		    strcpy(outstr,java_codes[buffer[idx]].name);
  if(mult==2 && !((flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    if((jflags & JVM_OBJREF1)==JVM_OBJREF1) { jflags &= ~JVM_OBJREF1; jflags |= JVM_OBJREF2; }
    else
    if((jflags & JVM_OBJREF2)==JVM_OBJREF2) { jflags &= ~JVM_OBJREF2; jflags |= JVM_OBJREF4; }
    if((jflags & JVM_CONST1)==JVM_CONST1) { jflags &= ~JVM_CONST1; jflags |= JVM_CONST2; }
    else
    if((jflags & JVM_CONST2)==JVM_CONST2) { jflags &= ~JVM_CONST2; jflags |= JVM_CONST4; }
  }
  idx++;
  if(tail)
  {
	if(!((flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
		TabSpace(outstr,TAB_POS);
	if(!((flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
	{
	    if(jflags & JVM_LOOKUPSWITCH)
	    {
		__filesize_t newpos;
		unsigned long defval,npairs;
		defval=FMT_DWORD(&buffer[idx+npadds],1);
		npairs=FMT_DWORD(&buffer[idx+npadds+4],1);
		if(!vartail) strcat(outstr,"???invalid???");
		else
		{
			strcat(outstr,"L");
			strcat(outstr,Get8Digit(npairs));
			strcat(outstr,",default:");
			newpos=ulShift+(__fileoff_t)defval;
			if(defval)
		    		disAppendFAddr(outstr,ulShift+idx+1+npadds,defval,
						newpos,DISADR_NEAR32,0,4);
			else
				strcat(outstr,Get8Digit(newpos));
		}
	    }
	    else
	    if(jflags & JVM_TABLESWITCH)
	    {
		__filesize_t newpos;
		unsigned long defval;
		defval=FMT_DWORD(&buffer[idx+npadds],1);
		if(!vartail) strcat(outstr,"???invalid???");
		else
		{
			strcat(outstr," default:");
			newpos=ulShift+(__fileoff_t)defval;
			if(defval)
				disAppendFAddr(outstr,ulShift+idx+1+npadds,defval,
						newpos,DISADR_NEAR32,0,4);
			else
				strcat(outstr,Get8Digit(newpos));
		}
	    }
	    else
	    switch(tail)
	    {
		case 1:
		{
		    char *p;
		    if(jflags & JVM_OBJREFMASK) strcat(outstr,"#");
		    p=NULL;
		    if(jflags & JVM_ATYPE) 
		    {
			switch(buffer[idx])
			{
			    case 4: p="T_BOOLEAN"; break;
			    case 5: p="T_CHAR"; break;
			    case 6: p="T_FLOAT"; break;
			    case 7: p="T_DOUBLE"; break;
			    case 8: p="T_BYTE"; break;
			    case 9: p="T_SHORT"; break;
			    case 10: p="T_INT"; break;
			    case 11: p="T_LONG"; break;
			}
		    }
		    if(p) strcat(outstr,p);
		    else
			strcat(outstr,Get2Digit(buffer[idx]));
		    break;
		}
		case 2: 
		{
		    __filesize_t newpos;
		    unsigned short sval;
		    sval=FMT_WORD(&buffer[idx],1);
		    if((jflags & JVM_CODEREF)==JVM_CODEREF && sval)
		    {
			newpos = ulShift + (signed short)sval;
			disAppendFAddr(outstr,ulShift + 1,sval,
					newpos,DISADR_NEAR16,0,2);
		    }
		    else
		    if((jflags & JVM_OBJREF1)==JVM_OBJREF1)
		    {
			strcat(outstr,"#");
			strcat(outstr,Get2Digit(buffer[idx]));
			strcat(outstr,",");
			strcat(outstr,Get2Digit(buffer[idx+1]));
		    }
		    else
		    if(jflags & JVM_OBJREFMASK)
		    disAppendDigits(outstr,ulShift+idx,
			APREF_USE_TYPE,2,&sval,DISARG_WORD);
		    else strcat(outstr,Get4Digit(sval));
		    break;
		}
		default:
		case 4: 
		{
		    __filesize_t newpos;
		    unsigned long lval;
		    lval=FMT_DWORD(&buffer[idx],1);
		    if((jflags & JVM_CODEREF)==JVM_CODEREF && lval)
		    {
			newpos = ulShift + (__fileoff_t)lval;
			disAppendFAddr(outstr,ulShift + 1,lval,
					newpos,DISADR_NEAR32,0,4);
		    }
		    else
		    if((jflags & JVM_OBJREF2)==JVM_OBJREF2)
		    {
			unsigned short sval;
			sval=FMT_WORD(&buffer[idx],1);
			disAppendDigits(outstr,ulShift,
				    APREF_USE_TYPE,2,&sval,DISARG_WORD);
			strcat(outstr,",");
			if((jflags & JVM_CONST1)==JVM_CONST1) strcat(outstr,Get2Digit(buffer[idx+2]));
			else
			{
			    sval=FMT_WORD(&buffer[idx+2],1);
			    strcat(outstr,Get4Digit(sval));
			}
		    }
		    else
		    if(jflags & JVM_OBJREFMASK)
		    disAppendDigits(outstr,ulShift+idx,
			APREF_USE_TYPE,4,&lval,DISARG_DWORD);
		    else strcat(outstr,Get8Digit(lval));
		    break;
		}
		case 8: 
		    disAppendDigits(outstr,ulShift+idx,
			APREF_USE_TYPE,8,&buffer[idx],DISARG_QWORD);
		    break;
	    }
	}
  }
  ret.codelen=tail+idx;
  return ret;
}

static void  __FASTCALL__ javaHelpAsm( void )
{
  hlpDisplay(20030);
}

static tBool  __FASTCALL__ javaAsmHelp( void )
{
  hlpDisplay(20031);
  return False;
}

static int    __FASTCALL__ javaMaxInsnLen( void ) { return 13; }
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
  { "JvmAsm", NULL, NULL, NULL, NULL },
  { javaAsmHelp, NULL, NULL, NULL, NULL },
  javaDisassembler,
  NULL,
  javaHelpAsm,
  javaMaxInsnLen,
  javaGetAsmColor,
  NULL, 
  javaGetAsmColor,
  NULL, 
  javaGetBitness,
  javaGetClone,
  javaInit,
  javaTerm,
  javaReadIni,
  javaWriteIni
};




