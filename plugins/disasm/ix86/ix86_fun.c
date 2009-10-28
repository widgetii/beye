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
#define REX_B(rex) (((rex)&0x01)>>0)
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
 if((DisP->mode&MOD_SSE)) return x86_Bitness==DAB_USE64?k86_XMMXRegs[REX_reg(rex,reg)]:ix86_XMMXRegs[reg];
 else
   if((DisP->mode&MOD_MMX))  return ix86_MMXRegs[reg];
   else
     if(!w) return (x86_Bitness==DAB_USE64&&HAS_REX)?k86_ByteRegs[REX_reg(rex,reg)]:ix86_ByteRegs[reg];
     else
     if(x86_Bitness==DAB_USE64)
      return use64?k86_QWordRegs[REX_reg(rex,reg)]:USE_WIDE_DATA?k86_DWordRegs[REX_reg(rex,reg)]:k86_WordRegs[REX_reg(rex,reg)];
     else return USE_WIDE_DATA ? ix86_DWordRegs[reg] : ix86_WordRegs[reg];
#else
 if((DisP->mode&MOD_SSE)) return ix86_XMMXRegs[reg];
 else
   if((DisP->mode&MOD_MMX))  return ix86_MMXRegs[reg];
   else
     if(!w) return ix86_ByteRegs[reg];
     else   return USE_WIDE_DATA ? ix86_DWordRegs[reg] : ix86_WordRegs[reg];
 rex=rex;
 use64 = use64;
#endif
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

void __FASTCALL__ ix86_ArgByte(char *str,ix86Param *DisP)
{
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(str,DisP->CodeAddress + DisP->codelen,
                 APREF_USE_TYPE,1,&DisP->RealCmd[DisP->codelen],DISARG_BYTE);
  DisP->codelen++;
}

void __FASTCALL__ ix86_ArgWord(char *str,ix86Param *DisP)
{
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(str,DisP->CodeAddress + DisP->codelen,
                 APREF_USE_TYPE,2,&DisP->RealCmd[DisP->codelen],DISARG_WORD | DISARG_IMM);
  DisP->codelen+=2;
}

void __FASTCALL__ ix86_ArgWord_Byte(char *str,ix86Param *DisP)
{
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(str,DisP->CodeAddress + DisP->codelen,
                 APREF_USE_TYPE,2,&DisP->RealCmd[DisP->codelen],DISARG_WORD);
  DisP->codelen+=2;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    strcat(str,",");
    strcat(str,Get2DigitApp(DisP->codelen,DisP));
  }
  DisP->codelen++;
}

void __FASTCALL__ ix86_ArgDWord(char *str,ix86Param *DisP)
{
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(str,DisP->CodeAddress + DisP->codelen,
                 APREF_USE_TYPE,4,&DisP->RealCmd[DisP->codelen],DISARG_DWORD | DISARG_IMM);
  DisP->codelen+=4;
}

static void __FASTCALL__ ix86_ArgQWord(char *str,ix86Param *DisP)
{
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(str,DisP->CodeAddress + DisP->codelen,
                 APREF_USE_TYPE,8,&DisP->RealCmd[DisP->codelen],DISARG_QWORD | DISARG_IMM);
  DisP->codelen+=8;
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

void __FASTCALL__ ix86_ArgFar(char * str,ix86Param *DisP)
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

void __FASTCALL__ ix86_ArgNear(char * str,ix86Param *DisP)
{
  long lshift = 0L;
  unsigned long newpos;
  unsigned modifier;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    lshift = USE_WIDE_DATA ?(long)(*((tInt32  *)(&DisP->RealCmd[1]))) :
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
    DisP->codelen += USE_WIDE_DATA ? 4 : 2;
    modifier = USE_WIDE_DATA ? DISADR_NEAR32 : DISADR_NEAR16;
  }
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    newpos = DisP->DisasmPrefAddr + lshift + DisP->codelen;
    disAppendFAddr(str,DisP->CodeAddress + 1,lshift,newpos,modifier,0,DisP->codelen-1);
  }
}

void __FASTCALL__ ix86_ArgShort(char * str,ix86Param *DisP)
{
  unsigned long r_sh;
  signed char distin;
  distin = DisP->RealCmd[DisP->codelen++];
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    r_sh = DisP->CodeAddress + distin + 2; /** 2 byte it self code jmp<c> addr */
    disAppendFAddr(str,DisP->CodeAddress + 1,(long)distin,r_sh,
                   DISADR_SHORT,0,1);
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
	if(x86_Bitness == DAB_USE64 && ((K86_REX&2)>>1))
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
         brex = (K86_REX&2)>>1;
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
         brex = (K86_REX&1)>>0;
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
      cptr = (HAS_67_IN64==False) ? k86_QWordRegs[REX_reg(K86_REX&1,rm)] : k86_DWordRegs[REX_reg(K86_REX&1,rm)];
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
                brex = K86_REX&1;
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
 char reg = (DisP->RealCmd[1] & 0x38) >> 3;
 char mod = (DisP->RealCmd[1] & 0xC0) >> 6;
 char rm  = (DisP->RealCmd[1] & 0x07);
 const char *regs,* modrm;
 tBool brex, use64;
 brex = use64 = 0;
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64)
 {
   brex = (K86_REX&4)>>2;
   use64 = Use64;
 }
#endif
 regs = k86_getREG(DisP,reg,w,brex,use64);
#ifdef IX86_64
 if(x86_Bitness == DAB_USE64) brex = K86_REX&1;
#endif
 if(mod == 3) modrm = k86_getREG(DisP,rm,w,brex,use64);
 else         modrm = ix86_getModRM(w,mod,rm,DisP);
 ix86_dtile[0] = 0;
 strcat(ix86_dtile,d ? regs : modrm);
 ix86_CStile(ix86_dtile,d ? modrm : regs);
 return ix86_dtile;
}

static char * __FASTCALL__ ix86_getTileWD( ix86Param *DisP )
{
  return __getTile(DisP,True,True);
}

static char * __FASTCALL__ ix86_getTileWnD( ix86Param *DisP )
{
  return __getTile(DisP,True,False);
}

static char * __FASTCALL__ ix86_getTilenWD( ix86Param *DisP )
{
  return __getTile(DisP,False,True);
}

static char * __FASTCALL__ ix86_getTilenWnD( ix86Param *DisP )
{
  return __getTile(DisP,False,False);
}

void __FASTCALL__ ix86_ArgModRM(char * str,ix86Param *DisP)
{
 DisP->codelen++;
 strcat(str,getTile(DisP));
}

void __FASTCALL__ ix86_ArgModRMDW(char *str,ix86Param *DisP)
{
 DisP->codelen++;
 strcat(str,ix86_getTileWD(DisP));
}

void __FASTCALL__ ix86_ArgModRMnDW(char *str,ix86Param *DisP)
{
 DisP->codelen++;
 strcat(str,ix86_getTileWnD(DisP));
}

void __FASTCALL__ ix86_ArgModRMDnW(char *str,ix86Param *DisP)
{
 DisP->codelen++;
 strcat(str,ix86_getTilenWD(DisP));
}

void __FASTCALL__ ix86_ArgModRMnDnW(char *str,ix86Param *DisP)
{
 DisP->codelen++;
 strcat(str,ix86_getTilenWnD(DisP));
}

void __FASTCALL__ ix86_ArgMod(char *str,ix86Param *DisP)
{
  char mod = (DisP->RealCmd[1] >> 6) & 0x03;
  char rm = (DisP->RealCmd[1] & 0x07);
  ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,1)]);
  DisP->codelen++;
  strcat(str,ix86_getModRM(True,mod,rm,DisP));
}

void __FASTCALL__ ix86_ArgModB(char *str,ix86Param *DisP)
{
  char mod = (DisP->RealCmd[1] >> 6) & 0x03;
  char rm = (DisP->RealCmd[1] & 0x07);
  if(mod<3) ix86_setModifier(str,ix86_sizes[BYTE_PTR]);
  DisP->codelen++;
  strcat(str,ix86_getModRM(False,mod,rm,DisP));
}

void __FASTCALL__ ix86_SMov(char * str,ix86Param *DisP)
{
  char direct = ( DisP->RealCmd[0] & 0x02 ) >> 1;
  char sreg  = (DisP->RealCmd[1] & 0x38) >> 3;
  char reg   = DisP->RealCmd[1] & 0x07;
  char mod   = (DisP->RealCmd[1] & 0xC0) >> 6;
  const char *tileptr,*sregptr;
  unsigned long mode = DisP->mode;
  DisP->mode&= ~MOD_WIDE_DATA;
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

void __FASTCALL__ ix86_ArgSegRM(char * str,ix86Param *DisP)
{
  char sreg  = (DisP->RealCmd[1] & 0x38) >> 3;
  char reg   = DisP->RealCmd[1] & 0x07;
  char mod   = (DisP->RealCmd[1] & 0xC0) >> 6;
  const char *tileptr,*sregptr;
  unsigned long mode = DisP->mode;
  DisP->mode&= ~MOD_WIDE_DATA;
  sregptr = getSREG(sreg);
  strcat(str,sregptr);
  DisP->codelen++;
  tileptr = ix86_getModRM(True,mod,reg,DisP);
  if(sreg > 3)
    if((DisP->pro_clone & IX86_CPUMASK) < 3)
#ifdef IX86_64
	if(x86_Bitness != DAB_USE64)
#endif
      DisP->pro_clone = IX86_CPU386;
  DisP->mode = mode;
  ix86_CStile(str,tileptr);
}

void __FASTCALL__ ix86_ArgAXDigit(char * str,ix86Param *DisP)
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

void __FASTCALL__ ix86_ArgRMDigit(char * str,ix86Param *DisP)
{
  char w = DisP->RealCmd[0] & 0x01;
  char mod = (DisP->RealCmd[1] & 0xC0) >> 6;
  char rm  = (DisP->RealCmd[1] & 0x07);
  char *a1,*a2;
  ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,w)]);
  DisP->codelen++;
  a1 = ix86_getModRM(w,mod,rm,DisP);
  strcat(str,a1);
  a2 = ix86_GetDigitTile(DisP,w,0,DisP->codelen);
  ix86_CStile(str,a2);
}

void __FASTCALL__ ix86_ArgIRegDigit(char * str,ix86Param *DisP)
{
  char w = (DisP->RealCmd[0] >> 3 ) & 0x01;
  char reg = (DisP->RealCmd[0]) & 0x07;
  tBool brex,use64;
  brex = use64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    brex = K86_REX&1;
    use64 = Use64;
  }
#endif
  strcat(str,k86_getREG(DisP,reg,w,brex,use64));
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64 && Use64 && w) w = -1;
#endif
  ix86_CStile(str,ix86_GetDigitTile(DisP,w,0,1));
}

void __FASTCALL__ ix86_ArgIReg(char *str,ix86Param *DisP)
{
  tBool brex,use64;
  brex = use64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    brex = K86_REX&1;
    use64 = Use64;
  }
#endif
  strcat(str,k86_getREG(DisP,DisP->RealCmd[0] & 0x07,True,brex,use64));
}

void __FASTCALL__ ix86_ArgIReg64(char *str,ix86Param *DisP)
{
  tBool brex,use64;
  brex = use64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    brex = K86_REX&1;
    use64 = Use64;
  }
#endif
  strcat(str,k86_getREG(DisP,DisP->RealCmd[0] & 0x07,True,brex,1));
}

void __FASTCALL__ ix86_ArgAXIReg(char *str,ix86Param *DisP)
{
  tBool brex,use64;
  brex = use64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    brex = K86_REX&1;
    use64 = Use64;
  }
#endif
  strcat(str,k86_getREG(DisP,0,True,0,use64));
  ix86_CStile(str,k86_getREG(DisP,DisP->RealCmd[0] & 0x07,True,brex,use64));
}


void __FASTCALL__ ix86_ArgSInt(char *str,ix86Param *DisP)
{
  ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,1)]);
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(str,DisP->CodeAddress + DisP->codelen,
                 APREF_USE_TYPE,1,&DisP->RealCmd[DisP->codelen],DISARG_CHAR | DISARG_IMM);
  DisP->codelen++;
}

void __FASTCALL__ ix86_ArgInt(char *str,ix86Param *DisP)
{
  tBool use64;
  use64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64) use64 = Use64;
#endif
  use64 ? ix86_ArgQWord(str,DisP) : USE_WIDE_DATA ? ix86_ArgDWord(str,DisP) : ix86_ArgWord(str,DisP);
}

void __FASTCALL__ ix86_ArgRegRMDigit(char *str,ix86Param *DisP)
{
   char * a1,*a2;
   char nw = DisP->RealCmd[0] & 0x02;
   DisP->codelen++;
   a1 = ix86_getTileWD(DisP);
   strcat(str,a1);
   a2 = ix86_GetDigitTile(DisP,!nw,0,DisP->codelen);
   ix86_CStile(str,a2);
}

static void __FASTCALL__ ix86_ArgRmDigit(char *str,ix86Param *DisP,char w,char s)
{
   char mod = (DisP->RealCmd[1] >> 6) & 0x03;
   char rm = (DisP->RealCmd[1] & 0x07);
   DisP->codelen++;
   strcat(str,ix86_getModRM(w == 2 ? True : w,mod,rm,DisP));
   strcat(str,",");
   strcat(str,ix86_GetDigitTile(DisP,w == 2 ? 0 : w,s,DisP->codelen));
}

void __FASTCALL__ ix86_ArgOp1(char *str,ix86Param *DisP)
{
   unsigned char w;
   unsigned char idx;
   w = DisP->RealCmd[0] & 0x01;
   idx = (DisP->RealCmd[1] & 0x38) >> 3;
   strcpy(str,ix86_Op1Names[idx]);
   TabSpace(str,TAB_POS);
   ix86_ArgRmDigit(str,DisP,w,idx == 0 || idx == 2 || idx == 3 || idx == 5 ? 1 : 0);
}

void __FASTCALL__ ix86_ArgOp2(char *str,ix86Param *DisP)
{
   unsigned char w;
   unsigned char idx;
   w = DisP->RealCmd[0] & 0x01;
   idx = (DisP->RealCmd[1] & 0x38) >> 3;
   strcpy(str,ix86_Op1Names[idx]);
   TabSpace(str,TAB_POS);
   if(w) ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,w)]);
   ix86_ArgRmDigit(str,DisP,w ? 2 : w, w ? 1 : 0);
}

void __FASTCALL__ ix86_ShOp2(char *str,ix86Param *DisP)
{
   char *a1,*a2;
   unsigned char code2 = ( DisP->RealCmd[1] & 0x38 ) >> 3;
   unsigned char mod = (DisP->RealCmd[1] >> 6) & 0x03;
   unsigned char rm = (DisP->RealCmd[1] & 0x07);
   unsigned char w = DisP->RealCmd[0] & 0x01;
   DisP->codelen++;
   strcpy(str,ix86_ShNames[code2]);
   TabSpace(str,TAB_POS);
   ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,w)]);
   a1 = ix86_getModRM(w,mod,rm,DisP);
   strcat(str,a1);
   a2 = ix86_GetDigitTile(DisP,0,0,DisP->codelen);
   ix86_CStile(str,a2);
}

void __FASTCALL__ ix86_ShOp1(char *str,ix86Param *DisP)
{
   char *a1;
   unsigned char code2 = ( DisP->RealCmd[1] & 0x38 ) >> 3;
   unsigned char mod = (DisP->RealCmd[1] >> 6) & 0x03;
   unsigned char rm = (DisP->RealCmd[1] & 0x07);
   unsigned char w = DisP->RealCmd[0] & 0x01;
   DisP->codelen++;
   a1 = ix86_getModRM(w,mod,rm,DisP);
   strcpy(str,ix86_ShNames[code2]);
   TabSpace(str,TAB_POS);
   ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,w)]);
   strcat(str,a1);
   ix86_CStile(str,"1");
}

void __FASTCALL__ ix86_ShOpCL(char *str,ix86Param *DisP)
{
   char *a1;
   unsigned char code2 = ( DisP->RealCmd[1] & 0x38 ) >> 3;
   unsigned char mod = (DisP->RealCmd[1] >> 6) & 0x03;
   unsigned char rm = (DisP->RealCmd[1] & 0x07);
   unsigned char w = DisP->RealCmd[0] & 0x01;
   DisP->codelen++;
   a1 = ix86_getModRM(w,mod,rm,DisP);
   strcpy(str,ix86_ShNames[code2]);
   TabSpace(str,TAB_POS);
   ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,w)]);
   strcat(str,a1);
   ix86_CStile(str,"cl");
}

void __FASTCALL__ ix86_ArgGrp1(char *str,ix86Param *DisP)
{
   unsigned char code2 = ( DisP->RealCmd[1] & 0x38 ) >> 3;
   unsigned char mod = (DisP->RealCmd[1] >> 6) & 0x03;
   unsigned char rm = (DisP->RealCmd[1] & 0x07);
   unsigned char w = DisP->RealCmd[0] & 0x01;
   strcpy(str,ix86_Gr1Names[code2]);
   TabSpace(str,TAB_POS);
   DisP->codelen++;
   ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,w)]);
   if(!code2) /** test only */
   {
     strcat(str,ix86_getModRM(w,mod,rm,DisP));
     ix86_CStile(str,ix86_GetDigitTile(DisP,w == 2 ? 0 : w,0,DisP->codelen));
   }
   else
   strcat(str,ix86_getModRM(w,mod,rm,DisP));
}

void __FASTCALL__ ix86_ArgGrp2(char *str,ix86Param *DisP)
{
   unsigned char code2 = ( DisP->RealCmd[1] & 0x38 ) >> 3;
   unsigned char mod = (DisP->RealCmd[1] >> 6) & 0x03;
   unsigned char rm = (DisP->RealCmd[1] & 0x07);
   unsigned char w = DisP->RealCmd[0] & 0x01;
   unsigned char wrd = code2 & 0x01;
   unsigned sizptr;
   unsigned oldDisNeedRef;
   tBool prev_use64;
   DisP->codelen++;
   prev_use64 = Use64;
   strcpy(str,ix86_Gr2Names[code2]);
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
   if(code2 >=2 && code2 <= 5) Use64=True;
   if(code2 == 0 || code2 == 1) { strcat(str,ix86_getModRM(w,mod,rm,DisP)); } /** inc dec */
   else
   {
     wrd = code2 == 0x06 || code2 == 0x07 ? 0 : wrd; /** push / pop; */
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

void __FASTCALL__ ix86_ArgAXMem(char *str,ix86Param *DisP)
{
  unsigned char d = DisP->RealCmd[0] & 0x02;
  const char *mem,*reg;
  tBool use64;
#ifdef IX86_64
  unsigned char sav_REX;
  sav_REX = K86_REX;
#endif
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
    mem = USE_WIDE_ADDR ? Get8SquareDig(1,DisP,False,True,False) : Get4SquareDig(1,DisP,False,True);
  strcpy(ix86_appstr,ix86_segpref);
  ix86_segpref[0] = 0;
  strcat(ix86_appstr,"[");
  strcat(ix86_appstr,mem);
  strcat(ix86_appstr,"]");
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64 && Use64) K86_REX=0;
#endif
  reg = k86_getREG(DisP,0,DisP->RealCmd[0] & 0x01,0,use64);
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64 && Use64) K86_REX=sav_REX;
#endif
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64 && !HAS_67_IN64)
    DisP->codelen = 9;
  else
#endif
  DisP->codelen = USE_WIDE_ADDR ? 5 : 3;
  strcat(str,d ? ix86_appstr : reg);
  ix86_CStile(str,d ? reg : ix86_appstr);
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

const ix86_ExOpcodes* __FASTCALL__ ix86_prepare_flags(const ix86_ExOpcodes *extable,ix86Param *DisP,unsigned char *code)
{
 int in_chain=1;
 while(in_chain)
 {
#ifdef IX86_64
    if(x86_Bitness == DAB_USE64)
    {
	if(extable[*code].flags64&TAB_NAME_IS_TABLE)
	{
	    extable=(ix86_ExOpcodes*)(extable[*code].name64);
	    DisP->RealCmd = &DisP->RealCmd[1];
	    *code = DisP->RealCmd[0];
	    DisP->codelen++;
	}
	else in_chain=0;
    }
    else
#endif
    if(extable[*code].pro_clone&TAB_NAME_IS_TABLE)
    {
	extable=(ix86_ExOpcodes*)(extable[*code].name);
	DisP->RealCmd = &DisP->RealCmd[1];
	*code = DisP->RealCmd[0];
	DisP->codelen++;
    }
    else in_chain=0;
 }
 return extable;
}

/** 0f xx opcodes */

void  __FASTCALL__ ix86_ExOpCodes(char *str,ix86Param *DisP)
{
 unsigned char code;
 const ix86_ExOpcodes *extable=ix86_extable;
 DisP->RealCmd = &DisP->RealCmd[1];
 DisP->CodeAddress++;
 code = DisP->RealCmd[0];
 DisP->codelen++;
 extable=ix86_prepare_flags(extable,DisP,&code);
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
 if(x86_Bitness == DAB_USE64) DisP->pro_clone = extable[code].flags64;
 else
#endif
 if((DisP->pro_clone&IX86_CPUMASK) < (extable[code].pro_clone&IX86_CPUMASK))
 {
	DisP->pro_clone &= ~IX86_CPUMASK;
	DisP->pro_clone |= extable[code].pro_clone;
 }
}

void  __FASTCALL__ ix86_ArgExGr0(char *str,ix86Param *DisP)
{
    unsigned char rm = (DisP->RealCmd[1] & 0x38) >> 3;
    unsigned char reg = (DisP->RealCmd[1] & 0x07);
    unsigned char mod = (DisP->RealCmd[1] & 0xC0) >> 6;
    strcpy(str,ix86_ExGrp0[rm]);
    TabSpace(str,TAB_POS);
    DisP->codelen++;
    ix86_setModifier(str,ix86_sizes[WORD_PTR]);
    strcat(str,ix86_getModRM(True,mod,reg,DisP));
#ifdef IX86_64
 if(x86_Bitness != DAB_USE64)
#endif
 if(rm < 4) DisP->pro_clone |= IX86_CPL0;
}

static char *ix86_vmxname[]={"???","???","???","???","???","???","vmclear","???",};
void   __FASTCALL__ ix86_VMX(char *str,ix86Param *DisP)
{
    unsigned char rm = (DisP->RealCmd[1] & 0x38) >> 3;
    unsigned char reg = (DisP->RealCmd[1] & 0x07);
    unsigned char mod = (DisP->RealCmd[1] & 0xC0) >> 6;
    strcpy(str,ix86_vmxname[reg]);
    TabSpace(str,TAB_POS);
    DisP->codelen++;
    strcat(str,ix86_getModRM(True,mod,rm,DisP));
    DisP->pro_clone &= ~IX86_CPUMASK;
    DisP->pro_clone |= IX86_P6;
}

static char *ix86_0Fvmxname[]={"???","cmpxchg8b","???","???","???","???","vmptrld","vmptrst",};
void   __FASTCALL__ ix86_0FVMX(char *str,ix86Param *DisP)
{
    unsigned char rm = (DisP->RealCmd[1] & 0x38) >> 3;
    unsigned char reg = (DisP->RealCmd[1] & 0x07);
    unsigned char mod = (DisP->RealCmd[1] & 0xC0) >> 6;
#ifdef IX86_64
    if(x86_Bitness == DAB_USE64 && reg==1)
	strcpy(str,"cmpxchg16b");
    else
#endif
    strcpy(str,ix86_0Fvmxname[reg]);
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
    unsigned char rm = (DisP->RealCmd[1] & 0x38) >> 3;
    unsigned char reg = (DisP->RealCmd[1] & 0x07);
    unsigned char mod = (DisP->RealCmd[1] & 0xC0) >> 6;
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
      if(DisP->RealCmd[1] == 0xC7)
      {
	unsigned char rm = (DisP->RealCmd[1] & 0x38) >> 3;
	unsigned char reg = (DisP->RealCmd[1] & 0x07);
	unsigned char mod = (DisP->RealCmd[1] & 0xC0) >> 6;
	DisP->codelen++;
	if(rm==0x06)
	{
	    strcpy(str,"vmclear");
	    TabSpace(str,TAB_POS);
	    DisP->codelen++;
	    strcat(str,ix86_getModRM(True,mod,reg,DisP));
	    DisP->pro_clone &= ~IX86_CPUMASK;
	    DisP->pro_clone |= IX86_P6;
	}
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
    strcpy(str,ix86_ExGrp1[rm]);
    TabSpace(str,TAB_POS);
    DisP->codelen++;
    buf = DisP->RealCmd[1];
    ptr = PWORD_PTR;
    word = True;
    if(rm == 7)
    {
      word = False;
#ifdef IX86_64
      if(x86_Bitness != DAB_USE64)
#endif
      DisP->pro_clone = IX86_CPU486;
      buf = 1;
      ptr = DUMMY_PTR;
    }
    else  if(rm == 4 || rm == 6) ptr = WORD_PTR;
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
    unsigned char xreg = (DisP->RealCmd[1] & 0x38) >> 3;
    unsigned char reg = DisP->RealCmd[1] & 0x07;
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

void  __FASTCALL__ ix86_DblShift(char *str,ix86Param *DisP)
{
    unsigned char code = DisP->RealCmd[0];
    const char *a1,*a2;
    a1 = ix86_getTileWnD(DisP);
    strcat(str,a1);
    a2 = code & 0x01 ? "cl" : ix86_GetDigitTile(DisP,0,0,DisP->codelen);
    ix86_CStile(str,a2);
    DisP->codelen++;
}

/* MMX and SSE(2) opcodes */

static void __NEAR__ __FASTCALL__ ix86_ArgxMM(char *str,ix86Param *DisP,tBool direct,tBool as_xmmx)
{
   unsigned long mode=DisP->mode;
   if(as_xmmx) DisP->mode|=MOD_SSE;
   else        DisP->mode|=MOD_MMX;
   if((DisP->RealCmd[1] & 0xC0) != 0xC0)
   {
     direct ? ix86_ArgModRMnDW(str,DisP) : ix86_ArgModRMDW(str,DisP);
     if(as_xmmx) DisP->mode&=~MOD_SSE;
     else        DisP->mode&=~MOD_MMX;
   }
   else
   {
      const char *mmx,*exx;
      tBool brex,use64;
      brex = use64 = 0;
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64)
      {
         brex = (K86_REX&4)>>2;
         use64 = Use64;
         if(brex && !as_xmmx) brex = 0; /* Note: there are only 8 mmx registers */
      }
#endif
      mmx = k86_getREG(DisP,(DisP->RealCmd[1] >> 3) & 0x07,True,brex,use64);
      if(as_xmmx) DisP->mode&=~MOD_SSE;
      else        DisP->mode&=~MOD_MMX;
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64) brex = K86_REX&1;
#endif
      exx = k86_getREG(DisP,DisP->RealCmd[1] & 0x07,True,brex,use64);
      strcat(str,direct ? exx : mmx);
      strcat(str,",");
      strcat(str,direct ? mmx : exx);
      DisP->codelen++;
   }
   DisP->mode=mode;
}

static void __NEAR__ __FASTCALL__ ix86_ArgxMMRev(char *str,ix86Param *DisP,tBool direct,tBool as_xmmx)
{
   unsigned long mode=DisP->mode;
   if(as_xmmx) DisP->mode|=MOD_SSE;
   else        DisP->mode|=MOD_MMX;
   if((DisP->RealCmd[1] & 0xC0) != 0xC0)
   {
     direct ? ix86_ArgModRMnDW(str,DisP) : ix86_ArgModRMDW(str,DisP);
     if(as_xmmx) DisP->mode|=MOD_SSE;
     else        DisP->mode&=~MOD_MMX;
   }
   else
   {
      const char *mmx,*exx;
      tBool brex,use64;
      brex = use64 = 0;
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64)
      {
         brex = K86_REX&1;
         use64 = Use64;
         if(brex && !as_xmmx) brex = 0; /* Note: there are only 8 mmx registers */
      }
#endif
      mmx = k86_getREG(DisP,DisP->RealCmd[1] & 0x07,True,brex,use64);
      if(as_xmmx) DisP->mode&=~MOD_SSE;
      else        DisP->mode&=~MOD_MMX;
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64) brex = (K86_REX&4)>>2;
#endif
      exx = k86_getREG(DisP,(DisP->RealCmd[1] >> 3) & 0x07,True,brex,use64);
      strcat(str,direct ? exx : mmx);
      strcat(str,",");
      strcat(str,direct ? mmx : exx);
      DisP->codelen++;
   }
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
    brex = K86_REX&1;
    use64 = Use64;
    if(brex && !as_xmmx) brex = 0; /* Note: there are only 8 mmx registers */
  }
#endif
  DisP->codelen++;
  strcpy(str,name);
  TabSpace(str,TAB_POS);
  if(as_xmmx) DisP->mode|=MOD_SSE;
  else        DisP->mode|=MOD_MMX;
  strcat(str,k86_getREG(DisP,DisP->RealCmd[1] & 0x07,True,brex,use64));
  if(as_xmmx) DisP->mode&=~MOD_SSE;
  else        DisP->mode&=~MOD_MMX;
  strcat(str,",");
  strcat(str,Get2Digit(DisP->RealCmd[2]));
  DisP->codelen++;
  DisP->mode=mode;
}

/* This function does not have analogs from MMX set. SSE(2) only */

static void __NEAR__ __FASTCALL__ ix86_ArgRXMM(char *str,ix86Param *DisP,tBool direct)
{
   unsigned long mode=DisP->mode;
   DisP->mode|=MOD_SSE;
   if((DisP->RealCmd[1] & 0xC0) != 0xC0)
   {
     direct ? ix86_ArgModRMnDW(str,DisP) : ix86_ArgModRMDW(str,DisP);
     DisP->mode&=~MOD_SSE;
   }
   else
   {
      const char *mmx,*exx;
      tBool brex,use64;
      brex = use64 = 0;
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64)
      {
        brex = (K86_REX&4)>>2;
        use64 = Use64;
      }
#endif
      mmx = k86_getREG(DisP,(DisP->RealCmd[1] >> 3) & 0x07,True,brex,use64);
      DisP->mode&=~MOD_SSE;
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64) brex = K86_REX&1;
#endif
      exx = k86_getREG(DisP,DisP->RealCmd[1] & 0x07,True,brex,use64);
      strcat(str,direct ? exx : mmx);
      strcat(str,",");
      strcat(str,direct ? mmx : exx);
      DisP->codelen++;
   }
   DisP->mode=mode;
}

static void __NEAR__ __FASTCALL__ ix86_ArgRxMMRev(char *str,ix86Param *DisP,tBool direct,tBool as_xmmx)
{
   if((DisP->RealCmd[1] & 0xC0) != 0xC0)
   {
     direct ? ix86_ArgModRMnDW(str,DisP) : ix86_ArgModRMDW(str,DisP);
   }
   else
   {
      const char *mmx,*exx;
      tBool brex,use64;
      brex = use64 = 0;
      unsigned long mode=DisP->mode;
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64)
      {
        brex = K86_REX&1;
        use64 = Use64;
        if(brex && !as_xmmx) brex = 0; /* Note: there are only 8 mmx registers */
      }
#endif
      if(as_xmmx) DisP->mode|=MOD_SSE;
      else        DisP->mode|=MOD_MMX;
      mmx = k86_getREG(DisP,DisP->RealCmd[1] & 0x07,True,brex,True);
      if(as_xmmx) DisP->mode&=~MOD_SSE;
      else        DisP->mode&=~MOD_MMX;
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64) brex = (K86_REX&4)>>2;
#endif
      exx = k86_getREG(DisP,(DisP->RealCmd[1] >> 3) & 0x07,True,brex,use64);
      strcat(str,direct ? mmx : exx);
      strcat(str,",");
      strcat(str,direct ? exx : mmx);
      DisP->codelen++;
      DisP->mode=mode;
   }
}

static void __NEAR__ __FASTCALL__ ix86_ArgxMMxMM(char *str,ix86Param *DisP,tBool direct,tBool xmmx_first)
{
   unsigned long mode=DisP->mode;
   if(xmmx_first) DisP->mode|=MOD_SSE;
   else           DisP->mode|=MOD_MMX;
   if((DisP->RealCmd[1] & 0xC0) != 0xC0)
   {
     direct ? ix86_ArgModRMnDW(str,DisP) : ix86_ArgModRMDW(str,DisP);
     if(xmmx_first) DisP->mode&=~MOD_SSE;
     else           DisP->mode&=~MOD_MMX;
   }
   else
   {
      const char *mmx,*exx;
      tBool brex,use64;
      brex = use64 = 0;
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64)
      {
        brex = (K86_REX&4)>>2;
        use64 = Use64;
        if(brex && !xmmx_first) brex = 0; /* Note: there are only 8 mmx registers */
      }
#endif
      mmx = k86_getREG(DisP,(DisP->RealCmd[1] >> 3) & 0x07,True,brex,use64);
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
      if(x86_Bitness == DAB_USE64) brex = K86_REX&1;
#endif
      exx = k86_getREG(DisP,DisP->RealCmd[1] & 0x07,True,brex,use64);
      if(xmmx_first) DisP->mode&=~MOD_MMX;
      else           DisP->mode&=~MOD_SSE;
      strcat(str,direct ? exx : mmx);
      strcat(str,",");
      strcat(str,direct ? mmx : exx);
      DisP->codelen++;
   }
   DisP->mode=mode;
}

void __FASTCALL__ ix86_ArgMMXD(char *str,ix86Param *DisP)
{
     unsigned long mode=DisP->mode;
     DisP->mode|=MOD_MMX;
     ix86_ArgModRMnDW(str,DisP);
     DisP->mode=mode;
}

void __FASTCALL__ ix86_ArgMMXnD(char *str,ix86Param *DisP)
{
     unsigned long mode=DisP->mode;
     DisP->mode|=MOD_MMX;
     ix86_ArgModRMDW(str,DisP);
     DisP->mode=mode;
}

void __FASTCALL__ ix86_ArgMMXRnD(char *str,ix86Param *DisP)
{
   ix86_ArgxMM(str,DisP,False,False);
}

void __FASTCALL__ ix86_ArgMMXRD(char *str,ix86Param *DisP)
{
   ix86_ArgxMM(str,DisP,True,False);
}

void __FASTCALL__ ix86_ArgMMXGr1(char *str,ix86Param *DisP)
{
  ix86_ArgxMMXGroup(str,ix86_MMXGr1[(DisP->RealCmd[1] >> 3) & 0x07],DisP,False);
}

void __FASTCALL__ ix86_ArgMMXGr2(char *str,ix86Param *DisP)
{
  ix86_ArgxMMXGroup(str,ix86_MMXGr2[(DisP->RealCmd[1] >> 3) & 0x07],DisP,False);
}

void __FASTCALL__ ix86_ArgMMXGr3(char *str,ix86Param *DisP)
{
  ix86_ArgxMMXGroup(str,ix86_MMXGr3[(DisP->RealCmd[1] >> 3) & 0x07],DisP,False);
}

void __FASTCALL__ ix86_ArgXMMXGr1(char *str,ix86Param *DisP)
{
  ix86_ArgxMMXGroup(str,ix86_XMMXGr1[(DisP->RealCmd[1] >> 3) & 0x07],DisP,True);
}

void __FASTCALL__ ix86_ArgXMMXGr2(char *str,ix86Param *DisP)
{
  ix86_ArgxMMXGroup(str,ix86_XMMXGr2[(DisP->RealCmd[1] >> 3) & 0x07],DisP,True);
}

void __FASTCALL__ ix86_ArgXMMXGr3(char *str,ix86Param *DisP)
{
  ix86_ArgxMMXGroup(str,ix86_XMMXGr3[(DisP->RealCmd[1] >> 3) & 0x07],DisP,True);
}

void  __FASTCALL__ ix86_ArgXMMXD(char *str,ix86Param *DisP)
{
     unsigned long mode=DisP->mode;
     DisP->mode|=MOD_SSE;
     ix86_ArgModRMnDW(str,DisP);
     DisP->mode=mode;

}

void  __FASTCALL__ ix86_ArgXMMXnD(char *str,ix86Param *DisP)
{
     unsigned long mode=DisP->mode;
     DisP->mode|=MOD_SSE;
     ix86_ArgModRMDW(str,DisP);
     DisP->mode=mode;
}

void  __FASTCALL__ ix86_ArgXMMXRnD(char *str,ix86Param *DisP)
{
   ix86_ArgxMM(str,DisP,False,True);
}

void  __FASTCALL__ ix86_ArgXMMXRD(char *str,ix86Param *DisP)
{
   ix86_ArgxMM(str,DisP,True,True);
}

void  __FASTCALL__ ix86_ArgRXMMXnD(char *str,ix86Param *DisP)
{
   ix86_ArgRXMM(str,DisP,False);
}

void  __FASTCALL__ ix86_ArgRXMMXD(char *str,ix86Param *DisP)
{
   ix86_ArgRXMM(str,DisP,True);
}

void  __FASTCALL__ ix86_ArgRXMMXRevnD(char *str,ix86Param *DisP)
{
   ix86_ArgRxMMRev(str,DisP,False,True);
}

void  __FASTCALL__ ix86_ArgRXMMXRevD(char *str,ix86Param *DisP)
{
   ix86_ArgRxMMRev(str,DisP,True,True);
}

void  __FASTCALL__ ix86_ArgRMMXRevnD(char *str,ix86Param *DisP)
{
   ix86_ArgRxMMRev(str,DisP,False,False);
}

void  __FASTCALL__ ix86_ArgRMMXRevD(char *str,ix86Param *DisP)
{
   ix86_ArgRxMMRev(str,DisP,True,False);
}

void  __FASTCALL__ ix86_ArgXMMXMMnD(char *str,ix86Param *DisP)
{
   ix86_ArgxMMxMM(str,DisP,False,True);
}

void  __FASTCALL__ ix86_ArgXMMXMMD(char *str,ix86Param *DisP)
{
   ix86_ArgxMMxMM(str,DisP,True,True);
}

void  __FASTCALL__ ix86_ArgMMXMMXnD(char *str,ix86Param *DisP)
{
   ix86_ArgxMMxMM(str,DisP,False,False);
}

void  __FASTCALL__ ix86_ArgMMXMMXD(char *str,ix86Param *DisP)
{
   ix86_ArgxMMxMM(str,DisP,True,False);
}

void __FASTCALL__ ix86_ArgMovYX(char *str,ix86Param *DisP)
{
      const char *dst,*src;
      unsigned char mod;
      unsigned char w = DisP->RealCmd[0] & 0x01;
      unsigned long mode=DisP->mode;
      tBool brex,use64;
      brex = use64 = 0;
#ifdef IX86_64
      if(x86_Bitness == DAB_USE64)
      {
        brex = (K86_REX&4)>>2;
        use64 = Use64;
      }
#endif
      mod = (DisP->RealCmd[1] >> 6) & 0x03;
      /*ix86_setModifier(str,ix86_sizes[ix86_calcModifier(DisP,1)]);*/
      dst = k86_getREG(DisP,(DisP->RealCmd[1] >> 3) & 0x07,True,brex,use64);
      DisP->mode&=~MOD_WIDE_DATA;
      src = ix86_getModRM(DisP->RealCmd[0] & 0x01,mod,DisP->RealCmd[1] & 0x07,DisP);
      DisP->mode=mode;
      strcat(str,dst);
      strcat(str,",");
      strcat(str,src);
      DisP->codelen++;
}

void __FASTCALL__ ix86_BitGrp(char *str,ix86Param *DisP)
{
   unsigned char idx;
   char mod = (DisP->RealCmd[1] >> 6) & 0x03;
   char rm = (DisP->RealCmd[1] & 0x07);
   idx = (DisP->RealCmd[1] & 0x38) >> 3;
   strcpy(str,ix86_BitGrpNames[idx]);
   TabSpace(str,TAB_POS);
   strcat(str,ix86_getModRM(True,mod,rm,DisP));
   strcat(str,",");
   strcat(str,ix86_GetDigitTile(DisP,0,0,DisP->codelen++));
}

void __FASTCALL__ ix86_ArgKatmaiGrp1(char *str,ix86Param *DisP)
{
   unsigned char code2 = ( DisP->RealCmd[1] & 0x38 ) >> 3;
   strcpy(str,ix86_KatmaiGr1Names[code2]);
   if(code2 < 5)
   {
     TabSpace(str,TAB_POS);
     ix86_ArgMod(str,DisP);
   }
   else DisP->codelen++;
   if(code2 == 5 || code2 == 6)
#ifdef IX86_64
	if(x86_Bitness != DAB_USE64)
#endif
	DisP->pro_clone &= ~IX86_CPUMASK;
	DisP->pro_clone |= IX86_P4|IX86_MMX;
}

void __FASTCALL__ ix86_ArgKatmaiGrp2(char *str,ix86Param *DisP)
{
   unsigned char code2 = ( DisP->RealCmd[1] & 0x38 ) >> 3;
   strcpy(str,ix86_KatmaiGr2Names[code2]);
   TabSpace(str,TAB_POS);
   ix86_ArgModB(str,DisP);
}

void __FASTCALL__  ix86_ArgXMMRMDigit(char *str,ix86Param *DisP)
{
   char * a1,*a2;
   unsigned long mode=DisP->mode;
   DisP->mode|=MOD_SSE;
   DisP->codelen++;
   a1 = ix86_getTileWD(DisP);
   strcat(str,a1);
   a2 = ix86_GetDigitTile(DisP,0,0,DisP->codelen-1);
   ix86_CStile(str,a2);
   DisP->mode|=mode;
}

void __FASTCALL__ ix86_ArgXMM1IReg(char *str,ix86Param *DisP)
{
  tBool brex,use64;
  unsigned long mode=DisP->mode;
  brex = use64 = 0;
  DisP->codelen++;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    brex = K86_REX&1;
    use64 = Use64;
  }
#endif
  DisP->mode|=MOD_SSE;
  strcat(str,k86_getREG(DisP,1,True,0,use64));
  ix86_CStile(str,k86_getREG(DisP,DisP->RealCmd[1] & 0x07,True,brex,use64));
  DisP->mode=mode;
}

void __FASTCALL__ ix86_ArgXMM1DigDig(char *str,ix86Param *DisP)
{
  tBool brex,use64;
  unsigned long mode=DisP->mode;
  brex = use64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    brex = K86_REX&1;
    use64 = Use64;
  }
#endif
  DisP->mode|=MOD_SSE;
  strcat(str,k86_getREG(DisP,1,True,0,use64));
  DisP->mode=mode;
  strcat(str,",");
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(str,DisP->CodeAddress+DisP->codelen,
                 APREF_USE_TYPE,1,&DisP->RealCmd[DisP->codelen],DISARG_BYTE);
  DisP->codelen++;
  strcat(str,",");
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(str,DisP->CodeAddress+DisP->codelen,
                 APREF_USE_TYPE,1,&DisP->RealCmd[DisP->codelen],DISARG_BYTE);
  DisP->codelen++;
  DisP->codelen++;
}

void __FASTCALL__ ix86_ArgXMM1RegDigDig(char *str,ix86Param *DisP)
{
  tBool brex,use64;
  unsigned long mode=DisP->mode;
  brex = use64 = 0;
#ifdef IX86_64
  if(x86_Bitness == DAB_USE64)
  {
    brex = K86_REX&1;
    use64 = Use64;
  }
#endif
  DisP->mode|=MOD_SSE;
  strcat(str,k86_getREG(DisP,1,True,0,use64));
  ix86_CStile(str,k86_getREG(DisP,DisP->RealCmd[1] & 0x07,True,brex,use64));
  DisP->mode=mode;
  strcat(str,",");
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(str,DisP->CodeAddress+DisP->codelen,
                 APREF_USE_TYPE,1,&DisP->RealCmd[DisP->codelen],DISARG_BYTE);
  DisP->codelen++;
  strcat(str,",");
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(str,DisP->CodeAddress+DisP->codelen,
                 APREF_USE_TYPE,1,&DisP->RealCmd[DisP->codelen],DISARG_BYTE);
  DisP->codelen++;
  DisP->codelen++;
}

void __FASTCALL__  ix86_ArgXMMCmp(char *str,ix86Param *DisP)
{
   char * a1,*a2;
   unsigned char suffix;
   char name[6], realname[10];
   unsigned long mode=DisP->mode;
   DisP->mode|=MOD_SSE;
   DisP->codelen++;
   a1 = ix86_getTileWD(DisP);
   suffix = DisP->RealCmd[DisP->codelen-1];
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
   }
   strcat(str,a1);
   a2 = ix86_GetDigitTile(DisP,0,0,DisP->codelen-1);
   if(!(suffix < 8)) ix86_CStile(str,a2);
   DisP->mode = mode;
}

void __FASTCALL__  ix86_ArgMMRMDigit(char *str,ix86Param *DisP)
{
   char * a1,*a2;
   unsigned long mode=DisP->mode;
   DisP->codelen++;
   DisP->mode|=MOD_MMX;
   a1 = ix86_getTileWD(DisP);
   strcat(str,a1);
   a2 = ix86_GetDigitTile(DisP,0,0,DisP->codelen-1);
   ix86_CStile(str,a2);
   DisP->mode=mode;
}

void __FASTCALL__  ix86_ArgRegXMMDigit(char *str,ix86Param *DisP)
{
   char * a1;
   ix86_ArgxMMRev(str,DisP,True,False);
   a1 = ix86_GetDigitTile(DisP,0,0,DisP->codelen-1);
   ix86_CStile(str,a1);
}

void __FASTCALL__  ix86_ArgRegXMMXDigit(char *str,ix86Param *DisP)
{
   char * a1;
   ix86_ArgxMMRev(str,DisP,True,True);
   a1 = ix86_GetDigitTile(DisP,0,0,DisP->codelen-1);
   ix86_CStile(str,a1);
}

void __FASTCALL__  ix86_ArgXMMRegDigit(char *str,ix86Param *DisP)
{
   char * a1;
   ix86_ArgMMXRnD(str,DisP);
   a1 = ix86_GetDigitTile(DisP,0,0,DisP->codelen-1);
   ix86_CStile(str,a1);
}

void __FASTCALL__  ix86_ArgXMMXRegDigit(char *str,ix86Param *DisP)
{
   char * a1;
   ix86_ArgRXMMXnD(str,DisP);
   a1 = ix86_GetDigitTile(DisP,0,0,DisP->codelen-1);
   ix86_CStile(str,a1);
}

void __FASTCALL__ ix86_3dNowOpCodes( char *str,ix86Param *DisP)
{
 unsigned char code;
 unsigned long mode=DisP->mode;
 DisP->mode|=MOD_MMX;
 ix86_Katmai_buff[0] = 0;
 ix86_ArgModRMDW(ix86_Katmai_buff,DisP);
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
   unsigned char mod = ( DisP->RealCmd[1] & 0x38 ) >> 3;
   strcpy(str,ix86_3dPrefetchGrp[mod]);
   TabSpace(str,TAB_POS);
   ix86_ArgModB(str,DisP);
}


