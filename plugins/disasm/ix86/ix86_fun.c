/**
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/ix86/ix86_fun.c
 * @brief       This file contains implementation common function and utility
 *              for Intel x86 disassembler.
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
 * @author      Kostya Nosov <k-nosov@yandex.ru>
 * @date        12.09.2000
 * @note        Adding navigation for indirect jumps
**/
#include <string.h>
#include <limits.h>
#include <stdio.h>

#include "plugins/disasm.h"
#include "plugins/disasm/ix86/ix86.h"
#include "biewutil.h"
#include "bmfile.h"
#include "codeguid.h"
#include "reg_form.h"

#define MODRM_REG(X) (((X)&0x38)>>3)
#define MODRM_COP(X) MODRM_REG(X)
#define MODRM_MOD(X) (((X)&0xC0)>>6)
#define MODRM_RM(X)  ((X)&0x07)

char *ix86_appstr;
char *ix86_dtile;
char *ix86_appbuffer;
char *ix86_apistr;
char *ix86_modrm_ret;
char *ix86_Katmai_buff;

#define ix86_setModifier(str,modf) disSetModifier(str,modf)
#define getSREG(sreg) (ix86_SegRegs[(unsigned char)sreg])
#define getTile(DisP) __getTile(DisP,DisP->RealCmd[0] & 0x01,DisP->RealCmd[0] & 0x02)

#ifdef IX86_64
#define REX_reg(rex,reg) (reg|((rex&1)<<3))
#endif

#define HAS_REX (DisP->pfx&PFX_REX)
#define K86_REX (DisP->REX)

#define HAS_REX (DisP->pfx&PFX_REX)
#define K86_REX (DisP->REX)
#define USE_WIDE_DATA (DisP->mode&MOD_WIDE_DATA)
#define USE_WIDE_ADDR (DisP->mode&MOD_WIDE_ADDR)
#define HAS_67_IN64 ((x86_Bitness == DAB_USE64)&&(DisP->pfx&PFX_67))

#define REX_W(rex) (((rex)&0x08)>>3)
#define REX_R(rex) (((rex)&0x04)>>2)
#define REX_X(rex) (((rex)&0x02)>>1)
#define REX_B(rex) ((rex)&0x01)
#define REX_w(rex) ((rex)&0x08)
#define REX_r(rex) ((rex)&0x04)
#define REX_x(rex) ((rex)&0x02)
#define REX_b(rex) ((rex)&0x01)

static inline unsigned ix86_calcModifier(ix86Param* DisP,unsigned w) {
  unsigned sizptr;
     if(!w) sizptr = BYTE_PTR;
     else
#ifdef IX86_64
     if(x86_Bitness == DAB_USE64)
	sizptr = REX_w(DisP->REX)?QWORD_PTR:USE_WIDE_DATA?DWORD_PTR:WORD_PTR;
     else
#endif
	sizptr = USE_WIDE_DATA ? DWORD_PTR : WORD_PTR;
   return sizptr;
}

static const char * __FASTCALL__ k86_getREG(ix86Param*DisP,unsigned char reg,tBool w,tBool rex, tBool use64)
{
#ifdef IX86_64
 if((DisP->mode&MOD_SSE)) {
    const char **k86_xmm_regs;
    unsigned ridx;
    if((DisP->pfx&PFX_VEX) && (DisP->VEX_vlp&0x04))	k86_xmm_regs=k86_YMMXRegs; /* 256-bit vectors */
    else						k86_xmm_regs=k86_XMMXRegs; /* 128-bit vectors */
    ridx = x86_Bitness==DAB_USE64?REX_reg(rex,reg):(reg&0x07);
    return  k86_xmm_regs[ridx];
 }
 else
   if((DisP->mode&MOD_MMX))  return ix86_MMXRegs[reg];
   else
     if(!w) return (x86_Bitness==DAB_USE64&&HAS_REX)?k86_ByteRegs[REX_reg(rex,reg)]:ix86_ByteRegs[reg];
     else
     if(x86_Bitness==DAB_USE64)
      return use64?k86_QWordRegs[REX_reg(rex,reg)]:USE_WIDE_DATA?k86_DWordRegs[REX_reg(rex,reg)]:k86_WordRegs[REX_reg(rex,reg)];
     else return USE_WIDE_DATA ? ix86_DWordRegs[reg] : ix86_WordRegs[reg];
#else
 if((DisP->mode&MOD_SSE)) {
    const char **ix86_xmm_regs;
    if((DisP->pfx&PFX_VEX) && (DisP->VEX_vlp&0x04))	ix86_xmm_regs=ix86_YMMXRegs; /* 256-bit vectors */
    else						ix86_xmm_regs=ix86_XMMXRegs; /* 128-bit vectors */
    return ix86_xmm_regs[reg];
 }
 else
   if((DisP->mode&MOD_MMX))  return ix86_MMXRegs[reg];
   else
     if(!w) return ix86_ByteRegs[reg];
     else   return USE_WIDE_DATA ? ix86_DWordRegs[reg] : ix86_WordRegs[reg];
 rex=rex;
 use64 = use64;
#endif
}

static const char *get_VEX_reg(ix86Param* DisP)
{
    unsigned rg,brex,use64;
    const char *rval=NULL;
    if(x86_Bitness == DAB_USE64) {
	use64 = Use64;
	rg = ((DisP->VEX_vlp>>3)&0x0F)^0x0F;
	brex = (rg>>3)&0x01;
	if(DisP->insn_flags&K64_VEX_V) {
	    rval=k86_getREG(DisP,rg,1,brex,use64);
	}
    }
    else {
	rg = ((DisP->VEX_vlp>>3)&0x07)^0x07;
	if(DisP->insn_flags&IX86_VEX_V)
	    rval=k86_getREG(DisP,rg,1,0,0);
    }
    return rval;
}

static char * __NEAR__ __FASTCALL__ GetDigitsApp(unsigned char loc_off,
                                                 ix86Param *DisP,
                                                 char codelen,int type)
{
  ix86_appbuffer[0] = 0;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(ix86_appbuffer,DisP->CodeAddress + loc_off,
                    APREF_USE_TYPE,codelen,&DisP->RealCmd[loc_off],type);
  return ix86_appbuffer;
}

#define Get2DigitApp(loc_off,DisP)   GetDigitsApp(loc_off,DisP,1,DISARG_BYTE)
#define Get2SignDigApp(loc_off,DisP) GetDigitsApp(loc_off,DisP,1,DISARG_CHAR)
#define Get4DigitApp(loc_off,DisP)   GetDigitsApp(loc_off,DisP,2,DISARG_WORD)
#define Get4SignDigApp(loc_off,DisP) GetDigitsApp(loc_off,DisP,2,DISARG_SHORT)
#define Get8DigitApp(loc_off,DisP)   GetDigitsApp(loc_off,DisP,4,DISARG_DWORD)
#define Get8SignDigApp(loc_off,DisP) GetDigitsApp(loc_off,DisP,4,DISARG_LONG)
#ifdef IX86_64
#define Get16DigitApp(loc_off,DisP)   GetDigitsApp(loc_off,DisP,8,DISARG_QWORD)
#define Get16SignDigApp(loc_off,DisP) GetDigitsApp(loc_off,DisP,8,DISARG_LLONG)
#endif
static char * __NEAR__ __FASTCALL__ Get2SquareDig(unsigned char loc_off,ix86Param *DisP,tBool as_sign)
{
  char *ptr = ix86_apistr;
  char *rets;
  *ptr = 0;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    rets = GetDigitsApp(loc_off,DisP,1,as_sign ? DISARG_CHAR : DISARG_BYTE);
    if(!(rets[0] == '+' || rets[0] == '-')) *ptr++ = '+';
    strcpy(ptr,rets);
  }
  return ix86_apistr;
}

static char * __NEAR__ __FASTCALL__ Get4SquareDig(unsigned char loc_off,ix86Param *DisP,tBool as_sign, tBool is_disponly)
{
  char *ptr = ix86_apistr;
  char *rets;
  unsigned type;
  *ptr = 0;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    type = as_sign ? DISARG_SHORT : DISARG_WORD;
    type |= is_disponly ? DISARG_DISP : DISARG_IDXDISP;
    rets = GetDigitsApp(loc_off,DisP,2,type);
    if(!(rets[0] == '+' || rets[0] == '-')) *ptr++ = '+';
    strcpy(ptr,rets);
  }
  return ix86_apistr;
}

static char * __NEAR__ __FASTCALL__ Get8SquareDig(unsigned char loc_off,ix86Param *DisP,tBool as_sign,tBool is_disponly,tBool as_rip)
{
  char *ptr = ix86_apistr;
  char *rets;
  unsigned type;
  *ptr = 0;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    type = as_sign ? DISARG_LONG : DISARG_DWORD;
    type |= is_disponly ? DISARG_DISP : DISARG_IDXDISP;
    if(as_rip) type |= DISARG_RIP;
    rets = GetDigitsApp(loc_off,DisP,4,type);
    if(!(rets[0] == '+' || rets[0] == '-')) *ptr++ = '+';
    strcpy(ptr,rets);
  }
  return ix86_apistr;
}

#ifdef IX86_64
static char * __NEAR__ __FASTCALL__ Get16SquareDig(unsigned char loc_off,ix86Param *DisP,tBool as_sign,tBool is_disponly)
{
  char *ptr = ix86_apistr;
  char *rets;
  unsigned type;
  *ptr = 0;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    type = as_sign ? DISARG_LLONG : DISARG_QWORD;
    type |= is_disponly ? DISARG_DISP : DISARG_IDXDISP;
    rets = GetDigitsApp(loc_off,DisP,8,type);
    if(!(rets[0] == '+' || rets[0] == '-')) *ptr++ = '+';
    strcpy(ptr,rets);
  }
  return ix86_apistr;
}
#endif

void __FASTCALL__ ix86_ArgES(char *str,ix86Param *param)
{
  UNUSED(param);
  strcat(str,"es");
}

void __FASTCALL__ ix86_ArgDS(char *str,ix86Param *param)
{
  UNUSED(param);
  strcat(str,"ds");
}

void __FASTCALL__ ix86_ArgSS(char *str,ix86Param *param)
{
  UNUSED(param);
  strcat(str,"ss");
}

void __FASTCALL__ ix86_ArgCS(char *str,ix86Param *param)
{
  UNUSED(param);
  strcat(str,"cs");
}

void __FASTCALL__ ix86_ArgFS(char *str,ix86Param *param)
{
  UNUSED(param);
  strcat(str,"fs");
}

void __FASTCALL__ ix86_ArgGS(char *str,ix86Param *param)
{
  UNUSED(param);
  strcat(str,"gs");
}


static char * __FASTCALL__ ix86_GetDigitTile(ix86Param* DisP,char wrd,char sgn,unsigned char loc_off)
{
  int cl,type;
  int do_64;
  do_64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64 && wrd == -1) /* special case for */
    do_64 = 1;
#endif
  cl = do_64 ? 8 : wrd ? ( USE_WIDE_DATA ? 4 : 2 ) : 1;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
#ifdef IX86_64
    if(do_64)
      type = sgn ? DISARG_LLONG : DISARG_QWORD;
    else
#endif
    type = sgn ? wrd ? ( USE_WIDE_DATA ? DISARG_LONG : DISARG_SHORT ) : DISARG_CHAR:
                 wrd ? ( USE_WIDE_DATA ? DISARG_DWORD : DISARG_WORD ) : DISARG_BYTE;
    type |= DISARG_IMM;
    ix86_dtile[0] = 0;
    disAppendDigits(ix86_dtile,
                    DisP->CodeAddress + loc_off,
                    APREF_USE_TYPE,cl,&DisP->RealCmd[loc_off],type);
  }
  DisP->codelen += cl;
  return ix86_dtile;
}

void __FASTCALL__ arg_segoff(char * str,ix86Param *DisP)
{
  unsigned long newpos = 0L;
  long off = 0L;
  unsigned short seg = 0;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    seg = USE_WIDE_DATA ? *((tInt16  *)(&DisP->RealCmd[5])) : *((tInt16  *)(&DisP->RealCmd[3]));
    off = USE_WIDE_DATA ? *((tInt32  *)(&DisP->RealCmd[1])) : (tInt32)(*((tInt16  *)(&DisP->RealCmd[1])));
    newpos = ((long)(seg) << 4) + off; /* It is computing of x86 real-mode address */
  }
  DisP->codelen += USE_WIDE_DATA ? 6 : 4;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendFAddr(str,DisP->CodeAddress + 1,off,newpos,
                   USE_WIDE_DATA ? DISADR_FAR32 : DISADR_FAR16,seg,DisP->codelen-1);
}

void __FASTCALL__ arg_offset(char * str,ix86Param *DisP)
{
  long lshift = 0L;
  unsigned long newpos;
  unsigned modifier,off8;
  off8 = (DisP->insn_flags&IMM_BYTE)?1:0;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    lshift =	off8 ? (long)(*((tInt8  *)(&DisP->RealCmd[1]))):
		USE_WIDE_DATA ?(long)(*((tInt32  *)(&DisP->RealCmd[1]))) :
					(long)(*((tInt16  *)(&DisP->RealCmd[1])));
#if 0
/*
In 64-bit mode, the operand size defaults to 64 bits. The processor sign-extends
the 8-bit or 32-bit displacement value to 64 bits before adding it to the RIP.
*/
  if(x86_Bitness == DAB_USE64) {
    DisP->codelen += 8;
    modifier=DISADR_NEAR64;
  }
  else
#endif
  {
    DisP->codelen += off8 ? 1 : USE_WIDE_DATA ? 4 : 2;
    modifier = USE_WIDE_DATA ? off8 ? DISARG_SHORT : DISADR_NEAR32 : DISADR_NEAR16;
  }
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    newpos = DisP->CodeAddress + lshift + DisP->codelen;
    disAppendFAddr(str,DisP->CodeAddress + 1,lshift,newpos,modifier,0,DisP->codelen);
  }
}

static void __NEAR__ __FASTCALL__ getSIBRegs(ix86Param*DisP,char * base,char * scale,char * _index,char *mod,char code)
{
  char scl = (code & 0xC0) >> 6;
  char bas = code & 0x07;
  char ind = (code & 0x38) >> 3;
  unsigned long mode = DisP->mode;
  tBool brex, use64;
  DisP->mode|=MOD_WIDE_DATA;
  DisP->mode&=~MOD_MMX;
  DisP->mode&=~MOD_SSE;
  brex = use64 = 0;
  if(scl)
  {
    scale[0] = '*';
    scale[1] = ( 1 << scl ) + 0x30;
    scale[2] = 0;
  }
  else scale[0] = 0;
  if(ind == 4)
  {
#ifdef IX86_64
	if(x86_Bitness == DAB_USE64 && REX_X(K86_REX))
	    /*	AMD 24594.pdf rev 3.02 aug 2002:
		REX adds fourth bit (X) which is decoded
		that allows to use R12 as index.
	     */
	     goto do_r12;
	else
#endif
	_index[0] = 0;
  }
  else
  {
#ifdef IX86_64
     do_r12:
     if(x86_Bitness == DAB_USE64)
     {
         use64 = HAS_67_IN64?0:1;
         brex = REX_X(K86_REX);
     }
#endif
     strcpy(_index,k86_getREG(DisP,ind,True,brex,use64));
  }
  if(bas == 5 && *mod == 0) { base[0] = 0; *mod = 2; }
  else
  {
#ifdef IX86_64
     if(x86_Bitness == DAB_USE64)
     {
         use64 = HAS_67_IN64?0:1;
         brex = REX_B(K86_REX);
     }
#endif
    strcpy(base,k86_getREG(DisP,bas,True,brex,use64));
  }
  DisP->mode = mode;
#if 0
  return ind == 4 || ind == 5 || bas == 4 || bas == 5; /** return is_stack */
#endif
}

static char * __NEAR__ __FASTCALL__ ConstrSibMod(ix86Param*DisP,char * store,char * scale,char * _index,char * base,char code,char *mod)
{
   getSIBRegs(DisP,base,scale,_index,mod,code);
   store[0] = 0;
   if(_index[0])
   {
     strcat(store,_index);
     strcat(store,scale);
   }
   if(base[0])
   {
     if(_index[0]) strcat(store,"+");
     strcat(store,base);
   }
   return store;
}

char * __FASTCALL__ ix86_getModRM(tBool w,unsigned char mod,unsigned char rm,ix86Param *DisP)
{
 const char *cptr;
 char square[50];
 char ret1[50];
 char base[8];
 char _index[8];
 char scale[4];
 char new_mode = mod;
 unsigned char clen,tmp;
 tBool as_sign,is_disponly,as_rip;
 clen = 2;
 if(rm == 4 && USE_WIDE_ADDR) cptr = ConstrSibMod(DisP,ret1,base,_index,scale,DisP->RealCmd[2],&new_mode);
 else
 {
#ifdef IX86_64
   if(x86_Bitness == DAB_USE64)
   {
      cptr = (HAS_67_IN64==False) ? k86_QWordRegs[REX_reg(REX_b(K86_REX),rm)] : k86_DWordRegs[REX_reg(REX_B(K86_REX),rm)];
   }
   else
#endif
   cptr = USE_WIDE_ADDR ? ix86_DWordRegs[rm] : ix86_A16[rm];
#if 0
   is_stack = USE_WIDE_ADDR ? rm == 4 || rm == 5 : rm == 2 || rm == 3 || rm == 6;
#endif
 }
 strcpy(square,cptr);
 mod = new_mode;
 ix86_modrm_ret[0] = 0;
 if(mod != 3)
 {
   strcpy(ix86_modrm_ret,"[");
   strcat(ix86_modrm_ret,ix86_segpref);
   ix86_segpref[0] = 0;
 }
 is_disponly = False;
 as_sign = True;
 as_rip=False;
 switch(mod)
 {
       case 0:
             {
               if((rm != 6 && !USE_WIDE_ADDR) || (rm != 5 && USE_WIDE_ADDR)) /* i.e. combine address */
               {
                 clen = 0;
                 if(USE_WIDE_ADDR && rm == 4) clen = 1;
                 strcat(ix86_modrm_ret,square);
               }
               else /* i.e. displacement only. fake mod = 2 */
               {
                  square[0] = 0;
                  is_disponly = True;
                  as_sign = False;
                  goto disp_long;
               }
             }
             break;
       case 1:
             {
               if(!USE_WIDE_ADDR)
               {
                 strcat(ix86_modrm_ret,ix86_segpref);
                 ix86_segpref[0] = 0;
                 tmp = 2;
                 clen = 1;
               }
               else
               {
                 if(rm != 4) { tmp = 2; clen = 1; }
                 else        { tmp = 3; clen = 2; }
               }
               strcat(ix86_modrm_ret,square);
               /* The "disp8" nomenclature denotes an 8-bit displacement
                  following the ModR/M or SIB byte, to be sign-extended
                  and added to the index. */
               strcat(ix86_modrm_ret,Get2SquareDig(tmp,DisP,True));
             }
             break;
       case 2:
            {
              disp_long:
#ifdef IX86_64
		 if(x86_Bitness == DAB_USE64 && is_disponly)
		 {
		   strcat(ix86_modrm_ret,"rip");
		   as_sign = True;
		   is_disponly = False;
		   as_rip=True;
		 }
#endif
              if(!USE_WIDE_ADDR)
              {
                strcat(ix86_modrm_ret,ix86_segpref);
                ix86_segpref[0] = 0;
                strcat(ix86_modrm_ret,square);
                strcat(ix86_modrm_ret,Get4SquareDig(2,DisP,as_sign,is_disponly));
                clen = 2;
              }
              else
              {
                if(rm != 4) { tmp = 2; clen = 4; }
                else        { tmp = 3; clen = 5; }
                strcat(ix86_modrm_ret,square);
                strcat(ix86_modrm_ret,Get8SquareDig(tmp,DisP,as_sign,is_disponly,as_rip));
              }
            }
            break;
      default:
            {
              tBool brex, use64;
              clen = 0;
              brex = use64 = 0;
#ifdef IX86_64
	      if(x86_Bitness == DAB_USE64)
              {
                brex = REX_B(K86_REX);
                use64 = Use64;
              }
#endif
              strcat(ix86_modrm_ret,k86_getREG(DisP,rm,w,brex,use64));
            }
            break;
   }
   if(mod != 3)
   {
     DisP->codelen += clen;
     strcat(ix86_modrm_ret,"]");
   }
   return ix86_modrm_ret;
}

char * __FASTCALL__ ix86_CStile(char *str,const char *arg2)
{
  strcat(str,",");
  strcat(str,arg2);
  return str;
}

static char * __NEAR__ __FASTCALL__ __getTile(ix86Param *DisP,tBool w,tBool d)
{
 char reg = MODRM_REG(DisP->RealCmd[1]);
 char mod = MODRM_MOD(DisP->RealCmd[1]);
 char rm  = MODRM_RM(DisP->RealCmd[1]);
 const char *regs,* modrm;
 tBool brex, use64;
 brex = use64 = 0;
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64)
 {
   brex = REX_R(K86_REX);
   use64 = Use64;
 }
#endif
 regs = k86_getREG(DisP,reg,w,brex,use64);
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64) brex = REX_B(K86_REX);
#endif
 if(mod == 3) modrm = k86_getREG(DisP,rm,w,brex,use64);
 else         modrm = ix86_getModRM(w,mod,rm,DisP);
 ix86_dtile[0] = 0;
 strcat(ix86_dtile,d ? regs : modrm);
 /* add VEX.vvvv field as first source operand */
 if((DisP->pfx&PFX_VEX) && (DisP->insn_flags&K64_VEX_V)) ix86_CStile(ix86_dtile,get_VEX_reg(DisP));
 ix86_CStile(ix86_dtile,d ? modrm : regs);
 return ix86_dtile;
}

void __FASTCALL__ arg_cpu_modregrm(char * str,ix86Param *DisP)
{
 DisP->codelen++;
 strcat(str,__getTile(DisP,(DisP->insn_flags&IX86_OP_BYTE)?False:True,(DisP->insn_flags&IX86_STORE)?False:True));
}

void __FASTCALL__ arg_cpu_modREGrm(char * str,ix86Param *DisP)
{
  tBool brex,use64;
  char reg = MODRM_REG(DisP->RealCmd[1]);
  char mod = MODRM_MOD(DisP->RealCmd[1]);
  char rm = MODRM_RM(DisP->RealCmd[1]);
  brex = use64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    brex = REX_B(K86_REX);
    use64 = Use64;
  }
#endif
  strcat(str,k86_getREG(DisP,reg,True,brex,use64));
  DisP->codelen++;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY)) strcat(str,",");
  strcat(str,ix86_getModRM((DisP->insn_flags&IX86_OP_BYTE)?False:True,mod,rm,DisP));
}

void __FASTCALL__ arg_cpu_mod_rm(char* str,ix86Param *DisP)
{
  char mod = MODRM_MOD(DisP->RealCmd[1]);
  char rm = MODRM_RM(DisP->RealCmd[1]);
  tBool w = (DisP->insn_flags&IX86_OP_BYTE)?False:True;
  if(mod<3) ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,w)]);
  DisP->codelen++;
  strcat(str,ix86_getModRM(w,mod,rm,DisP));
}

void  __FASTCALL__ arg_cpu_mod_rm_imm(char *str,ix86Param *DisP)
{
  arg_cpu_mod_rm(str,DisP);
  ix86_CStile(str,ix86_GetDigitTile(DisP,(DisP->insn_flags&IMM_BYTE)?False:True,0,DisP->codelen));
}

void  __FASTCALL__ arg_cpu_modregrm_imm(char *str,ix86Param *DisP)
{
  arg_cpu_modregrm(str,DisP);
  ix86_CStile(str,ix86_GetDigitTile(DisP,(DisP->insn_flags&IMM_BYTE)?False:True,0,DisP->codelen));
}

void __FASTCALL__ arg_cpu_modsegrm(char * str,ix86Param *DisP)
{
  char direct = ( DisP->RealCmd[0] & 0x02 ) >> 1;
  char sreg  = MODRM_REG(DisP->RealCmd[1]);
  char reg   = MODRM_RM(DisP->RealCmd[1]);
  char mod   = MODRM_MOD(DisP->RealCmd[1]);
  const char *tileptr,*sregptr;
  unsigned long mode = DisP->mode;
  DisP->mode&= ~MOD_WIDE_DATA; /* disable: 'mov eax, ds' constructions */
  sregptr = getSREG(sreg);
  DisP->codelen = 2;
  tileptr = ix86_getModRM(True,mod,reg,DisP);
  if(sreg > 3)
    if((DisP->pro_clone & IX86_CPUMASK) < 3)
#ifdef IX86_64
	if(x86_Bitness != DAB_USE64)
#endif
      DisP->pro_clone = IX86_CPU386;
  DisP->mode = mode;
  strcat(str,direct ? sregptr : tileptr);
  ix86_CStile(str,direct ? tileptr : sregptr);
}

void __FASTCALL__ arg_r0_imm(char * str,ix86Param *DisP)
{
  char w = DisP->RealCmd[0] & 0x01;
  tBool use64=0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64) use64 = Use64;
  /* I guess that K9 will support "OP rax, imm64" forms
     same as "OP rrx, imm64" since they are not longer
     than 15 bytes in length
     (example: "adc r13, 123456789ABCDEF0").
     TODO: if(use64) w = -1; here */
#endif
  strcat(str,k86_getREG(DisP,0,w,0,use64));
  ix86_CStile(str,ix86_GetDigitTile(DisP,w,0,1));
}

void __FASTCALL__ arg_r0rm(char *str,ix86Param *DisP)
{
  tBool brex,use64;
  brex = use64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    brex = REX_B(K86_REX);
    use64 = Use64;
  }
#endif
  strcat(str,k86_getREG(DisP,0,True,0,use64));
  ix86_CStile(str,k86_getREG(DisP,MODRM_RM(DisP->RealCmd[0]),True,brex,use64));
}

void __FASTCALL__ arg_r0mem(char *str,ix86Param *DisP)
{
  unsigned char d = DisP->RealCmd[0] & 0x02;
  const char *mem,*reg;
  tBool use64;
  use64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64 && Use64)
  {
    use64 = Use64;
  }
  if(x86_Bitness == DAB_USE64 && !HAS_67_IN64)
  /* Opcodes A0 through A3 are address sized. In 64-bit mode, memory offset default to 64-bit. */
  {
    mem = Get16SquareDig(1,DisP,False,True);
  }
  else
#endif
    mem = USE_WIDE_ADDR ?   Get8SquareDig(1,DisP,False,True,False) :
			    Get4SquareDig(1,DisP,False,True);
  strcpy(ix86_appstr,ix86_segpref);
  ix86_segpref[0] = 0;
  strcat(ix86_appstr,"[");
  strcat(ix86_appstr,mem);
  strcat(ix86_appstr,"]");
  reg = k86_getREG(DisP,0,DisP->RealCmd[0] & 0x01,0,use64);
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64 && !HAS_67_IN64)
    DisP->codelen = 9;
  else
#endif
  DisP->codelen = USE_WIDE_ADDR ? 5 : 3;
  strcat(str,d ? ix86_appstr : reg);
  ix86_CStile(str,d ? reg : ix86_appstr);
}

void __FASTCALL__ arg_insnreg(char *str,ix86Param *DisP)
{
  tBool brex,use64;
  brex = use64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    brex = REX_B(K86_REX);
    use64 = Use64;
  }
#endif
  strcat(str,k86_getREG(DisP,MODRM_RM(DisP->RealCmd[0]),(DisP->insn_flags&IX86_OP_BYTE)?False:True,brex,(DisP->insn_flags&K64_FORCE64)?1:use64));
}

void __FASTCALL__ arg_insnreg_imm(char * str,ix86Param *DisP)
{
  char w = (DisP->insn_flags&IX86_OP_BYTE)?0:1;
  arg_insnreg(str,DisP);
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64 && Use64 && w) w = -1; /* special case for do64 */
#endif
  ix86_CStile(str,ix86_GetDigitTile(DisP,w,0,1));
}

void __FASTCALL__ arg_imm(char *str,ix86Param *DisP)
{
  char wrd = 1,sgn = 0;
  if(DisP->insn_flags&IMM_BYTE) {
    ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,1)]);
    wrd = 0;
    sgn = 1;
  }
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64 && Use64) wrd=-1;
#endif
  strcat(str,ix86_GetDigitTile(DisP,wrd,sgn,1));
}

void __FASTCALL__ arg_imm8(char *str,ix86Param *DisP)
{
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(str,DisP->CodeAddress + DisP->codelen,
                 APREF_USE_TYPE,1,&DisP->RealCmd[DisP->codelen],DISARG_BYTE);
  DisP->codelen++;
}

void __FASTCALL__ arg_imm16(char *str,ix86Param *DisP)
{
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(str,DisP->CodeAddress + DisP->codelen,
                 APREF_USE_TYPE,2,&DisP->RealCmd[DisP->codelen],DISARG_WORD | DISARG_IMM);
  DisP->codelen+=2;
}

void __FASTCALL__ arg_imm16_imm8(char *str,ix86Param *DisP)
{
  arg_imm16(str,DisP);
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY)) strcat(str,",");
  arg_imm8(str,DisP);
}

static void __FASTCALL__ ix86_ArgRmDigit(char *str,ix86Param *DisP,char w,char s)
{
   char mod = MODRM_MOD(DisP->RealCmd[1]);
   char rm = MODRM_RM(DisP->RealCmd[1]);
   strcat(str,ix86_getModRM(w == 2 ? True : w,mod,rm,DisP));
   strcat(str,",");
   DisP->codelen++;
   strcat(str,ix86_GetDigitTile(DisP,w == 2 ? 0 : w,s,DisP->codelen));
}

void __FASTCALL__ ix86_ArgOp1(char *str,ix86Param *DisP)
{
   unsigned char w = DisP->RealCmd[0] & 0x01;
   unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
   strcpy(str,ix86_Op1Names[cop]);
   TabSpace(str,TAB_POS);
   ix86_ArgRmDigit(str,DisP,w,cop == 0 || cop == 2 || cop == 3 || cop == 5 ? 1 : 0);
}

void __FASTCALL__ ix86_ArgOp2(char *str,ix86Param *DisP)
{
   unsigned char w = DisP->RealCmd[0] & 0x01;
   unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
   strcpy(str,ix86_Op1Names[cop]);
   TabSpace(str,TAB_POS);
   if(w) ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,w)]);
   ix86_ArgRmDigit(str,DisP,w ? 2 : w, w ? 1 : 0);
}

void __FASTCALL__ ix86_ShOp2(char *str,ix86Param *DisP)
{
   char *a1,*a2;
   unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
   unsigned char mod = MODRM_MOD(DisP->RealCmd[1]);
   unsigned char rm = MODRM_RM(DisP->RealCmd[1]);
   unsigned char w = DisP->RealCmd[0] & 0x01;
   DisP->codelen++;
   strcpy(str,ix86_ShNames[cop]);
   TabSpace(str,TAB_POS);
   ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,w)]);
   a1 = ix86_getModRM(w,mod,rm,DisP);
   strcat(str,a1);
   a2 = ix86_GetDigitTile(DisP,0,0,DisP->codelen);
   ix86_CStile(str,a2);
}

void  __FASTCALL__ ix86_DblShift(char *str,ix86Param *DisP)
{
    unsigned char code = DisP->RealCmd[0];
    arg_cpu_modregrm(str,DisP);
    if(code&0x01) ix86_CStile(str,"cl");
    else { strcat(str,","); arg_imm8(str,DisP); }
}

void __FASTCALL__ ix86_ShOp1(char *str,ix86Param *DisP)
{
   char *a1;
   unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
   unsigned char mod = MODRM_MOD(DisP->RealCmd[1]);
   unsigned char rm = MODRM_RM(DisP->RealCmd[1]);
   unsigned char w = DisP->RealCmd[0] & 0x01;
   DisP->codelen++;
   a1 = ix86_getModRM(w,mod,rm,DisP);
   strcpy(str,ix86_ShNames[cop]);
   TabSpace(str,TAB_POS);
   ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,w)]);
   strcat(str,a1);
   ix86_CStile(str,"1");
}

void __FASTCALL__ ix86_ShOpCL(char *str,ix86Param *DisP)
{
   char *a1;
   unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
   unsigned char mod = MODRM_MOD(DisP->RealCmd[1]);
   unsigned char rm = MODRM_RM(DisP->RealCmd[1]);
   unsigned char w = DisP->RealCmd[0] & 0x01;
   DisP->codelen++;
   a1 = ix86_getModRM(w,mod,rm,DisP);
   strcpy(str,ix86_ShNames[cop]);
   TabSpace(str,TAB_POS);
   ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,w)]);
   strcat(str,a1);
   ix86_CStile(str,"cl");
}

void __FASTCALL__ ix86_ArgGrp1(char *str,ix86Param *DisP)
{
   unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
   unsigned char mod = MODRM_MOD(DisP->RealCmd[1]);
   unsigned char rm = MODRM_RM(DisP->RealCmd[1]);
   unsigned char w = DisP->RealCmd[0] & 0x01;
   strcpy(str,ix86_Gr1Names[cop]);
   TabSpace(str,TAB_POS);
   DisP->codelen++;
   ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,w)]);
   if(!cop) /** test only */
   {
     strcat(str,ix86_getModRM(w,mod,rm,DisP));
     ix86_CStile(str,ix86_GetDigitTile(DisP,w == 2 ? 0 : w,0,DisP->codelen));
   }
   else
   strcat(str,ix86_getModRM(w,mod,rm,DisP));
}

void __FASTCALL__ ix86_ArgGrp2(char *str,ix86Param *DisP)
{
   unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
   unsigned char mod = MODRM_MOD(DisP->RealCmd[1]);
   unsigned char rm = MODRM_RM(DisP->RealCmd[1]);
   unsigned char w = DisP->RealCmd[0] & 0x01;
   unsigned char wrd = cop & 0x01;
   unsigned sizptr;
   unsigned oldDisNeedRef;
   tBool prev_use64;
   DisP->codelen++;
   prev_use64 = Use64;
   strcpy(str,ix86_Gr2Names[cop]);
   /*
      Added by "Kostya Nosov" <k-nosov@yandex.ru>:
      make NEEDREF_ALL for indirect "jmp" and "call"
   */
   oldDisNeedRef = disNeedRef;
/*
   if(code2 >= 2 && code2 <= 5 && disNeedRef != NEEDREF_NONE)
                                             disNeedRef = NEEDREF_ALL;
*/
   TabSpace(str,TAB_POS);
   if(cop >=2 && cop <= 5) Use64=True;
   if(cop == 0 || cop == 1) { strcat(str,ix86_getModRM(w,mod,rm,DisP)); } /** inc dec */
   else
   {
     wrd = cop == 0x06 || cop == 0x07 ? 0 : wrd; /** push / pop; */
#ifdef IX86_64
     if(x86_Bitness == DAB_USE64)
	sizptr = HAS_REX?REX_w(K86_REX)?QWORD_PTR:DWORD_PTR:QWORD_PTR;
     else
#endif
     if(!wrd) sizptr = USE_WIDE_DATA?DWORD_PTR:WORD_PTR;
     else     sizptr = USE_WIDE_DATA?PWORD_PTR:DWORD_PTR;
     ix86_setModifier(str,ix86_sizes[sizptr]);
     strcat(str,ix86_getModRM(True,mod,rm,DisP));
   }
   disNeedRef = oldDisNeedRef;
   Use64 = prev_use64;
}


void __FASTCALL__ ix86_InOut(char * str,ix86Param *DisP)
{
  const char *arg,*reg1,*reg2,*regptr,*dig;
  char i;
  regptr = k86_getREG(DisP,0,DisP->RealCmd[0] & 0x01,0,0);
  dig = Get2Digit(DisP->RealCmd[1]);
  i = DisP->RealCmd[0] & 0x08;
  if(!i) DisP->codelen++;
  arg = i ? "dx" : dig;
  if(DisP->RealCmd[0] & 0x02)
  {
      reg1  = arg;
      reg2  = regptr;
  }
  else
  {
      reg1  = regptr;
      reg2  = arg;
  }
  strcat(str,reg1);
  ix86_CStile(str,reg2);
}

const ix86_ExOpcodes* __FASTCALL__ ix86_prepare_flags(const ix86_ExOpcodes *extable,ix86Param *DisP,unsigned char *code,unsigned char* codelen)
{
 int in_chain=1;
 while(in_chain)
 {
#ifdef IX86_64
    if(x86_Bitness == DAB_USE64)
    {
	if(extable[*code].flags64&TAB_NAME_IS_TABLE)
		extable=(ix86_ExOpcodes*)(extable[*code].name64);
	else in_chain=0;
    }
    else
#endif
    if(extable[*code].pro_clone&TAB_NAME_IS_TABLE)
	extable=(ix86_ExOpcodes*)(extable[*code].name);
    else in_chain = 0;

    if(in_chain) {
	DisP->RealCmd = &DisP->RealCmd[1];
	*code = DisP->RealCmd[0];
	(*codelen)++;
    }
    else {
	if(x86_Bitness == DAB_USE64) {
		DisP->pro_clone = extable[*code].flags64&(~K64_FLAGS_MASK);
		DisP->insn_flags = extable[*code].flags64;
	}
	else {
		DisP->pro_clone = extable[*code].pro_clone&(~IX86_FLAGS_MASK);
		DisP->insn_flags = extable[*code].pro_clone;
	}
	in_chain=0;
    }
 }
 return extable;
}

/** 0f xx opcodes */

void  __FASTCALL__ ix86_ExOpCodes(char *str,ix86Param *DisP)
{
 unsigned char code,up;
 const ix86_ExOpcodes *extable=ix86_extable;
 DisP->RealCmd = &DisP->RealCmd[1];
 DisP->CodeAddress++;
 code = DisP->RealCmd[0];
 up = 0;
 extable=ix86_prepare_flags(extable,DisP,&code,&up);
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64)
 {
    strcpy(str,extable[code].name64?extable[code].name64:"???");
 }
 else
#endif
 strcpy(str,extable[code].name?extable[code].name:"???");
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64)
 {
   if(extable[code].method64)
   {
     TabSpace(str,TAB_POS);
     extable[code].method64(str,DisP);
   }
 }
 else
#endif
 if(extable[code].method)
 {
   TabSpace(str,TAB_POS);
   extable[code].method(str,DisP);
 }
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64) DisP->pro_clone = extable[code].flags64&K64_CLONEMASK;
 else
#endif
 if((DisP->pro_clone&IX86_CPUMASK) < (extable[code].pro_clone&IX86_CPUMASK))
 {
	DisP->pro_clone &= ~IX86_CPUMASK;
	DisP->pro_clone |= extable[code].pro_clone;
 }
 DisP->codelen+=up+1;
}

void  __FASTCALL__ ix86_ArgExGr0(char *str,ix86Param *DisP)
{
    unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
    unsigned char reg = MODRM_RM(DisP->RealCmd[1]);
    unsigned char mod = MODRM_MOD(DisP->RealCmd[1]);
    strcpy(str,ix86_ExGrp0[cop]);
    TabSpace(str,TAB_POS);
    DisP->codelen++;
    ix86_setModifier(str,ix86_sizes[WORD_PTR]);
    strcat(str,ix86_getModRM(True,mod,reg,DisP));
#ifdef IX86_64
    if(x86_Bitness != DAB_USE64)
#endif
    if(cop < 4) DisP->pro_clone |= IX86_CPL0;
}

static char *ix86_vmxname[]={"???","???","???","???","???","???","vmclear","???",};
void   __FASTCALL__ ix86_VMX(char *str,ix86Param *DisP)
{
    unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
    unsigned char rm  = MODRM_RM(DisP->RealCmd[1]);
    unsigned char mod = MODRM_MOD(DisP->RealCmd[1]);
    strcpy(str,ix86_vmxname[cop]);
    TabSpace(str,TAB_POS);
    DisP->codelen++;
    strcat(str,ix86_getModRM(True,mod,rm,DisP));
    DisP->pro_clone &= ~IX86_CPUMASK;
    DisP->pro_clone |= IX86_P6;
}

static char *ix86_0Fvmxname[]={"???","cmpxchg8b","???","???","???","???","vmptrld","vmptrst",};
void   __FASTCALL__ ix86_0FVMX(char *str,ix86Param *DisP)
{
    unsigned char rm = MODRM_RM(DisP->RealCmd[1]);
    unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
    unsigned char mod = MODRM_MOD(DisP->RealCmd[1]);
#ifdef IX86_64
    if(x86_Bitness == DAB_USE64 && USE_WIDE_DATA && cop==1)
	strcpy(str,"cmpxchg16b");
    else
#endif
    strcpy(str,ix86_0Fvmxname[cop]);
    TabSpace(str,TAB_POS);
    DisP->codelen++;
    strcat(str,ix86_getModRM(True,mod,rm,DisP));
    if(rm > 1) {
	DisP->pro_clone &= ~IX86_CPUMASK;
	DisP->pro_clone |= IX86_P6;
    }
}

void  __FASTCALL__ ix86_ArgExGr1(char *str,ix86Param *DisP)
{
    unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
    unsigned char reg = MODRM_RM(DisP->RealCmd[1]);
    unsigned char mod = MODRM_MOD(DisP->RealCmd[1]);
    unsigned char buf,ptr;
    tBool word;
#ifdef IX86_64
    if(x86_Bitness == DAB_USE64)
    {
      if(DisP->RealCmd[1] == 0xF8)
      {
        strcpy(str,"swapgs");
        DisP->codelen++;
	DisP->pro_clone |= K64_CPL0;
        return;
      }
    }
#endif
    {
      if(DisP->RealCmd[1] == 0xC1)
      {
        strcpy(str,"vmcall");
        DisP->codelen++;
	DisP->pro_clone &= ~IX86_CPUMASK;
	DisP->pro_clone |= IX86_P6;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xC2)
      {
        strcpy(str,"vmlaunch");
        DisP->codelen++;
	DisP->pro_clone &= ~IX86_CPUMASK;
	DisP->pro_clone |= IX86_P6;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xC3)
      {
        strcpy(str,"vmresume");
        DisP->codelen++;
	DisP->pro_clone &= ~IX86_CPUMASK;
	DisP->pro_clone |= IX86_P6;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xC4)
      {
        strcpy(str,"vmxoff");
        DisP->codelen++;
	DisP->pro_clone &= ~IX86_CPUMASK;
	DisP->pro_clone |= IX86_P6;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xC8)
      {
        strcpy(str,"monitor");
        DisP->codelen++;
	DisP->pro_clone &= ~(IX86_CPUMASK|IX86_CPL0);
	DisP->pro_clone |= IX86_P5;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xC9)
      {
        strcpy(str,"mwait");
        DisP->codelen++;
	DisP->pro_clone &= ~(IX86_CPUMASK|IX86_CPL0);
	DisP->pro_clone |= IX86_P5;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xD0)
      {
        strcpy(str,"xgetbv");
        DisP->codelen++;
	DisP->pro_clone &= ~(IX86_CPUMASK|IX86_CPL0);
	DisP->pro_clone |= IX86_P8;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xD1)
      {
        strcpy(str,"xsetbv");
        DisP->codelen++;
	DisP->pro_clone &= ~(IX86_CPUMASK|IX86_CPL0);
	DisP->pro_clone |= IX86_P8;
        return;
      }
#ifdef IX86_64
      else
      if(DisP->RealCmd[1] == 0xD8)
      {
        strcpy(str,"vmrun");
        DisP->codelen++;
	DisP->pro_clone = K64_FAM10|K64_CPL0;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xD9)
      {
        strcpy(str,"vmmcall");
        DisP->codelen++;
	DisP->pro_clone = K64_FAM10|K64_CPL0;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xDA)
      {
        strcpy(str,"vmload");
        DisP->codelen++;
	DisP->pro_clone = K64_FAM10|K64_CPL0;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xDB)
      {
        strcpy(str,"vmsave");
        DisP->codelen++;
	DisP->pro_clone = K64_FAM10|K64_CPL0;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xDC)
      {
        strcpy(str,"stgi");
        DisP->codelen++;
	DisP->pro_clone = K64_FAM10|K64_CPL0;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xDD)
      {
        strcpy(str,"clgi");
        DisP->codelen++;
	DisP->pro_clone = K64_FAM10|K64_CPL0;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xDE)
      {
        strcpy(str,"skinit");
        DisP->codelen++;
	DisP->pro_clone = K64_FAM10|K64_CPL0;
        return;
      }
      else
      if(DisP->RealCmd[1] == 0xDF)
      {
        strcpy(str,"invlpga");
        DisP->codelen++;
	DisP->pro_clone = K64_FAM10|K64_CPL0;
        return;
      }
#endif
    }
    strcpy(str,ix86_ExGrp1[cop]);
    TabSpace(str,TAB_POS);
    DisP->codelen++;
    buf = DisP->RealCmd[1];
    ptr = PWORD_PTR;
    word = True;
    if(cop == 7)
    {
      word = False;
#ifdef IX86_64
      if(x86_Bitness != DAB_USE64)
#endif
      DisP->pro_clone = IX86_CPU486;
      buf = 1;
      ptr = DUMMY_PTR;
    }
    else  if(cop == 4 || cop == 6) ptr = WORD_PTR;
    ix86_setModifier(str,ix86_sizes[ptr]);
    strcat(str,ix86_getModRM(word ? True : buf & 0x01,mod,reg,DisP));
}

static const char ** xry[] = { ix86_CrxRegs, ix86_DrxRegs, ix86_TrxRegs, ix86_XrxRegs };
#ifdef IX86_64
static const char ** k86_xry[] = { k86_CrxRegs, k86_DrxRegs, k86_TrxRegs, k86_XrxRegs };
#endif

void  __FASTCALL__ ix86_ArgMovXRY(char *str,ix86Param *DisP)
{
    unsigned char code = DisP->RealCmd[0];
    unsigned char xreg = MODRM_REG(DisP->RealCmd[1]);
    unsigned char reg = MODRM_RM(DisP->RealCmd[1]);
    unsigned char ridx = (code & 0x01) + ((code >> 1) & 0x02);
    unsigned char d = (code >> 1) & 0x01;
    unsigned long mode = DisP->mode;
    tBool brex,use64;
    brex = use64 = 0;
#ifdef IX86_64
    if(x86_Bitness == DAB_USE64)
    {
       brex = K86_REX & 1;
       use64 = Use64;
    }
#endif
    DisP->codelen++;
    DisP->mode|=MOD_WIDE_DATA;
#ifdef IX86_64
    if(x86_Bitness == DAB_USE64)
    {
	/* according on intel 30083401.pdf p# 1.4.3 to access cr8-tr15 is used REX.r */
	strcat(str,d ? k86_xry[ridx][xreg|(REX_R(K86_REX)<<3)] : k86_getREG(DisP,reg,True,brex,1));
	ix86_CStile(str,d ? k86_getREG(DisP,reg,True,brex,1) : k86_xry[ridx][xreg|(REX_R(K86_REX)<<3)]);
    }
    else
#endif
    {
	strcat(str,d ? xry[ridx][xreg] : k86_getREG(DisP,reg,True,brex,use64));
	ix86_CStile(str,d ? k86_getREG(DisP,reg,True,brex,use64) : xry[ridx][xreg]);
    }
    DisP->mode=mode;
}

/* MMX and SSE(2) opcodes */

static void __NEAR__ __FASTCALL__ ix86_bridge_cpu_simd(char *str,ix86Param *DisP,tBool direct,tBool as_xmmx)
{
   unsigned long mode=DisP->mode;
   if(as_xmmx) DisP->mode|=MOD_SSE;
   else        DisP->mode|=MOD_MMX;
   if((DisP->RealCmd[1] & 0xC0) != 0xC0) {
	if(direct) DisP->mode=mode;
	arg_cpu_modregrm(str,DisP);
   }
   else
   {
      const char *mmx,*exx,*vxx=NULL;
      tBool brex,use64;
      brex = use64 = 0;
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64)
      {
         brex = direct?REX_B(K86_REX):REX_R(K86_REX);
         use64 = Use64;
         if(brex && !as_xmmx) brex = 0; /* Note: there are only 8 mmx registers */
      }
#endif
      mmx = k86_getREG(DisP,direct?MODRM_RM(DisP->RealCmd[1]):MODRM_REG(DisP->RealCmd[1]),True,brex,use64);
      if(DisP->pfx&PFX_VEX) vxx = get_VEX_reg(DisP);
      else {
	if(as_xmmx) DisP->mode&=~MOD_SSE;
	else        DisP->mode&=~MOD_MMX;
      }
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64) brex = direct?REX_R(K86_REX):REX_B(K86_REX);
#endif
      exx = k86_getREG(DisP,direct?MODRM_REG(DisP->RealCmd[1]):MODRM_RM(DisP->RealCmd[1]),True,brex,use64);
      strcat(str,direct ? exx : mmx);
      strcat(str,",");
      if(vxx) { strcat(str, vxx); strcat(str,","); }
      strcat(str,direct ? mmx : exx);
      DisP->codelen++;
   }
   DisP->mode=mode;
}

void   __FASTCALL__ bridge_simd_cpu(char *str,ix86Param* DisP)
{
    ix86_bridge_cpu_simd(str,DisP,(DisP->insn_flags&BRIDGE_CPU_SSE)?True:False,(DisP->insn_flags&IX86_SSE)?True:False);
}

void __FASTCALL__  bridge_simd_cpu_imm8(char *str,ix86Param *DisP)
{
    bridge_simd_cpu(str,DisP);
    if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY)) strcat(str,",");
    arg_imm8(str,DisP);
}

static void __NEAR__ __FASTCALL__ ix86_bridge_sse_mmx(char *str,ix86Param *DisP,tBool xmmx_first)
{
   unsigned long mode=DisP->mode;
   if(xmmx_first) DisP->mode|=MOD_SSE;
   else           DisP->mode|=MOD_MMX;
   if((DisP->RealCmd[1] & 0xC0) != 0xC0)
			arg_cpu_modregrm(str,DisP);
   else
   {
      const char *mmx,*exx;
      tBool brex,use64;
      brex = use64 = 0;
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64)
      {
        brex = REX_R(K86_REX);
        use64 = Use64;
        if(brex && !xmmx_first) brex = 0; /* Note: there are only 8 mmx registers */
      }
#endif
      mmx = k86_getREG(DisP,MODRM_REG(DisP->RealCmd[1]),True,brex,use64);
      if(xmmx_first)
      {
        DisP->mode&=~MOD_SSE;
        DisP->mode|=MOD_MMX;
      }
      else
      {
        DisP->mode|=MOD_SSE;
        DisP->mode&=~MOD_MMX;
      }
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64) brex = REX_B(K86_REX);
#endif
      exx = k86_getREG(DisP,MODRM_RM(DisP->RealCmd[1]),True,brex,use64);
      strcat(str,mmx);
      strcat(str,",");
      strcat(str,exx);
      DisP->codelen++;
   }
   DisP->mode=mode;
}

void __FASTCALL__ bridge_sse_mmx(char *str,ix86Param* DisP)
{
   ix86_bridge_sse_mmx(str,DisP,(DisP->insn_flags&BRIDGE_MMX_SSE)?False:True);
}

void __FASTCALL__ arg_simd(char *str,ix86Param *DisP)
{
    unsigned long mode=DisP->mode;
    if(DisP->insn_flags&IX86_MMX)	DisP->mode|=MOD_MMX;
    else				DisP->mode|=MOD_SSE;
    arg_cpu_modregrm(str,DisP);
    DisP->mode=mode;
}

static void __NEAR__ __FASTCALL__ ix86_ArgxMMXGroup(char *str,const char *name,ix86Param *DisP,tBool as_xmmx)
{
  unsigned long mode=DisP->mode;
  tBool brex,use64;
  brex = use64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    brex = REX_B(K86_REX);
    use64 = Use64;
    if(brex && !as_xmmx) brex = 0; /* Note: there are only 8 mmx registers */
  }
#endif
  DisP->codelen++;
  strcpy(str,name);
  TabSpace(str,TAB_POS);
  if(as_xmmx) DisP->mode|=MOD_SSE;
  else        DisP->mode|=MOD_MMX;
  strcat(str,k86_getREG(DisP,MODRM_RM(DisP->RealCmd[1]),True,brex,use64));
  if(as_xmmx) DisP->mode&=~MOD_SSE;
  else        DisP->mode&=~MOD_MMX;
  strcat(str,",");
  strcat(str,Get2Digit(DisP->RealCmd[2]));
  DisP->codelen++;
  DisP->mode=mode;
}

void __FASTCALL__ ix86_ArgMMXGr1(char *str,ix86Param *DisP)
{
  ix86_ArgxMMXGroup(str,ix86_MMXGr1[MODRM_COP(DisP->RealCmd[1])],DisP,False);
}

void __FASTCALL__ ix86_ArgMMXGr2(char *str,ix86Param *DisP)
{
  ix86_ArgxMMXGroup(str,ix86_MMXGr2[MODRM_COP(DisP->RealCmd[1])],DisP,False);
}

void __FASTCALL__ ix86_ArgMMXGr3(char *str,ix86Param *DisP)
{
  ix86_ArgxMMXGroup(str,ix86_MMXGr3[MODRM_COP(DisP->RealCmd[1])],DisP,False);
}

void __FASTCALL__ ix86_ArgXMMXGr1(char *str,ix86Param *DisP)
{
  ix86_ArgxMMXGroup(str,ix86_XMMXGr1[MODRM_COP(DisP->RealCmd[1])],DisP,True);
}

void __FASTCALL__ ix86_ArgXMMXGr2(char *str,ix86Param *DisP)
{
  ix86_ArgxMMXGroup(str,ix86_XMMXGr2[MODRM_COP(DisP->RealCmd[1])],DisP,True);
}

void __FASTCALL__ ix86_ArgXMMXGr3(char *str,ix86Param *DisP)
{
  ix86_ArgxMMXGroup(str,ix86_XMMXGr3[MODRM_COP(DisP->RealCmd[1])],DisP,True);
}

void __FASTCALL__ ix86_ArgMovYX(char *str,ix86Param *DisP)
{
      const char *dst,*src;
      unsigned char mod;
      unsigned long mode=DisP->mode;
      tBool brex,use64;
      brex = use64 = 0;
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64)
      {
        brex = REX_R(K86_REX);
        use64 = Use64;
      }
#endif
      mod = MODRM_MOD(DisP->RealCmd[1]);
      /*ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,1)]);*/
      dst = k86_getREG(DisP,MODRM_REG(DisP->RealCmd[1]),True,brex,use64);
      DisP->mode&=~MOD_WIDE_DATA;
      src = ix86_getModRM(DisP->RealCmd[0] & 0x01,mod,MODRM_RM(DisP->RealCmd[1]),DisP);
      DisP->mode=mode;
      strcat(str,dst);
      strcat(str,",");
      strcat(str,src);
      DisP->codelen++;
}

void __FASTCALL__ ix86_BitGrp(char *str,ix86Param *DisP)
{
   unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
   char mod = MODRM_MOD(DisP->RealCmd[1]);
   char rm = MODRM_RM(DisP->RealCmd[1]);
   strcpy(str,ix86_BitGrpNames[cop]);
   TabSpace(str,TAB_POS);
   strcat(str,ix86_getModRM(True,mod,rm,DisP));
   strcat(str,",");
   strcat(str,ix86_GetDigitTile(DisP,0,0,DisP->codelen++));
}

static const char *ix86_KatmaiGr1Names[] = { "fxsave", "fxrstor", "ldmxcsr", "stmxcsr", "xsave", "xrstor", "???", "???" };
static const char *ix86_KatmaiGr1Names11[] = { "???", "???", "???", "???", "???", "lfence", "mfence", "sfence" };
void __FASTCALL__ ix86_ArgKatmaiGrp1(char *str,ix86Param *DisP)
{
   unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
   unsigned char mod = MODRM_MOD(DisP->RealCmd[1]);
   if(mod==3)	strcpy(str,ix86_KatmaiGr1Names11[cop]);
   else		strcpy(str,ix86_KatmaiGr1Names[cop]);
   if(mod < 3)
   {
     TabSpace(str,TAB_POS);
     arg_cpu_mod_rm(str,DisP);
	if(x86_Bitness != DAB_USE64) {
	    DisP->pro_clone &= ~IX86_CPUMASK;
	    DisP->pro_clone |= IX86_P8|IX86_SSE;
	}
	else {
	    DisP->pro_clone &= ~K64_CLONEMASK;
	    DisP->pro_clone |= K64_FAM11|K64_SSE;
	}
   }
   else DisP->codelen++;
   if((cop==5 || cop==6) && mod==3)
	if(x86_Bitness != DAB_USE64) {
	    DisP->pro_clone &= ~IX86_CPUMASK;
	    DisP->pro_clone |= IX86_P4|IX86_MMX;
	}
}

void __FASTCALL__ ix86_ArgKatmaiGrp2(char *str,ix86Param *DisP)
{
   unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
   strcpy(str,ix86_KatmaiGr2Names[cop]);
   TabSpace(str,TAB_POS);
   arg_cpu_mod_rm(str,DisP);
}

void __FASTCALL__  arg_simd_imm8(char *str,ix86Param *DisP)
{
    arg_simd(str,DisP);
    if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY)) strcat(str,",");
    arg_imm8(str,DisP);
}

void __FASTCALL__ arg_simd_regrm(char *str,ix86Param *DisP)
{
  tBool brex,use64;
  unsigned long mode=DisP->mode;
  brex = use64 = 0;
  DisP->codelen++;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    brex = REX_B(K86_REX);
    use64 = Use64;
  }
#endif
  DisP->mode|=MOD_SSE;
  strcat(str,k86_getREG(DisP,MODRM_REG(DisP->RealCmd[1]),True,brex,use64));
  ix86_CStile(str,k86_getREG(DisP,MODRM_RM(DisP->RealCmd[1]),True,brex,use64));
  DisP->mode=mode;
}

void __FASTCALL__ arg_simd_regrm_imm8_imm8(char *str,ix86Param *DisP)
{
  arg_simd_regrm(str,DisP);
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY)) strcat(str,",");
  arg_imm8(str,DisP);
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY)) strcat(str,",");
  arg_imm8(str,DisP);
}

void __FASTCALL__ arg_simd_rm_imm8_imm8(char *str,ix86Param *DisP)
{
  tBool brex,use64;
  unsigned long mode=DisP->mode;
  brex = use64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    brex = REX_B(K86_REX);
    use64 = Use64;
  }
#endif
  DisP->mode|=MOD_SSE;
  strcat(str,k86_getREG(DisP,MODRM_RM(DisP->RealCmd[1]),True,brex,use64));
  DisP->mode=mode;
  DisP->codelen++;

  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY)) strcat(str,",");
  arg_imm8(str,DisP);
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY)) strcat(str,",");
  arg_imm8(str,DisP);
}


void __FASTCALL__  ix86_ArgXMMCmp(char *str,ix86Param *DisP)
{
   char *a;
   unsigned char suffix;
   char name[6], realname[10];
   unsigned long mode=DisP->mode;
   DisP->mode|=MOD_SSE;
   ix86_Katmai_buff[0] = 0;
   arg_cpu_modregrm(ix86_Katmai_buff,DisP);
   suffix = DisP->RealCmd[DisP->codelen];
   if(suffix < 8)
   {
    /*Note: this code suppose that name is cmpXY*/
      strncpy(name, str, 5);
      name[5] = 0;
      strncpy(realname,name,3);
      realname[3] = 0;
      strcat(realname,ix86_KatmaiCmpSuffixes[suffix]);
      strcat(realname, &name[3]);
      strcpy(str, realname);
      TabSpace(str, TAB_POS);
      strcat(str,ix86_Katmai_buff);
   }
   else {
	strcat(str,ix86_Katmai_buff);
	a = ix86_GetDigitTile(DisP,0,0,DisP->codelen);
   }
   DisP->mode = mode;
   DisP->codelen++;
}

void   __FASTCALL__ arg_simd_xmm0(char *str,ix86Param *DisP) {
   arg_simd(str,DisP);
   ix86_CStile(str,"xmm0");
}

/* TODO: fix it !!! */
void   __FASTCALL__ arg_vex(char *str,ix86Param *DisP) {
    unsigned is4,rg,brex,use64;
    unsigned long mod = DisP->mode;
    DisP->mode |= MOD_SSE;
    brex=use64=0;
    arg_simd(str,DisP);
    is4=DisP->RealCmd[DisP->codelen];
    DisP->codelen++;
    if(x86_Bitness == DAB_USE64) {
	brex = (is4>>7)&0x01;
	use64 = HAS_67_IN64?0:1;
	rg = ((is4>>4)&0x07)/*^0x0F*/;
	ix86_CStile(str,k86_getREG(DisP,rg,1,brex,use64));
    }
    else {
	rg = ((is4>>4)&0x07)/*^0x07*/;
	ix86_CStile(str,k86_getREG(DisP,rg,1,brex,use64));
    }
    DisP->mode = mod;
}

void   __FASTCALL__ arg_vex_imm8(char *str,ix86Param *DisP) {
    unsigned is4;
    arg_vex(str,DisP);
    is4=DisP->RealCmd[DisP->codelen-1];
    ix86_CStile(str,Get2Digit(is4&0x0F));
}

void __FASTCALL__ ix86_3dNowOpCodes( char *str,ix86Param *DisP)
{
 unsigned char code;
 unsigned long mode=DisP->mode;
 DisP->mode|=MOD_MMX;
 ix86_Katmai_buff[0] = 0;
 arg_cpu_modregrm(ix86_Katmai_buff,DisP);
 DisP->mode=mode;
 code = DisP->RealCmd[DisP->codelen-1];
 DisP->codelen++;
 strcpy(str,ix86_3dNowtable[code].name);
 TabSpace(str,TAB_POS);
 strcat(str,ix86_Katmai_buff);
#ifdef IX86_64
   if(x86_Bitness == DAB_USE64) DisP->pro_clone = K64_ATHLON|K64_MMX;
   else
#endif
   DisP->pro_clone = ix86_3dNowtable[code].pro_clone;
}

void __FASTCALL__ ix86_3dNowPrefetchGrp(char *str,ix86Param *DisP)
{
   unsigned char cop = MODRM_COP(DisP->RealCmd[1]);
   strcpy(str,ix86_3dPrefetchGrp[cop]);
   TabSpace(str,TAB_POS);
   arg_cpu_mod_rm(str,DisP);
}

