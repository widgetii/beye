/**
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/ix86/ix86_fun.c
 * @brief       This file contains implementation common function and utility
 *              for Intel x86 disassembler.
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

#ifdef INT64_C
#define REX_reg(rex,reg) (reg|((rex&1)<<3))
#endif

static const char * __FASTCALL__ k86_getREG(unsigned char reg,tBool w,tBool rex, tBool use64)
{
#ifdef INT64_C
 if(UseXMMXSet) return x86_Bitness==DAB_USE64?k86_XMMXRegs[REX_reg(rex,reg)]:ix86_XMMXRegs[reg];
 else
   if(UseMMXSet)  return ix86_MMXRegs[reg];
   else
     if(!w) return (x86_Bitness==DAB_USE64&&has_REX)?k86_ByteRegs[REX_reg(rex,reg)]:ix86_ByteRegs[reg];
     else 
     if(x86_Bitness==DAB_USE64)
      return use64?k86_QWordRegs[REX_reg(rex,reg)]:Use32Data?k86_DWordRegs[REX_reg(rex,reg)]:k86_WordRegs[REX_reg(rex,reg)];
     else return Use32Data ? ix86_DWordRegs[reg] : ix86_WordRegs[reg];
#else
 if(UseXMMXSet) return ix86_XMMXRegs[reg];
 else
   if(UseMMXSet)  return ix86_MMXRegs[reg];
   else
     if(!w) return ix86_ByteRegs[reg];
     else   return Use32Data ? ix86_DWordRegs[reg] : ix86_WordRegs[reg];
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
#ifdef INT64_C
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

static char * __NEAR__ __FASTCALL__ Get8SquareDig(unsigned char loc_off,ix86Param *DisP,tBool as_sign,tBool is_disponly)
{
  char *ptr = ix86_apistr;
  char *rets;
  unsigned type;
  *ptr = 0;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    type = as_sign ? DISARG_LONG : DISARG_DWORD;
    type |= is_disponly ? DISARG_DISP : DISARG_IDXDISP;
    rets = GetDigitsApp(loc_off,DisP,4,type);
    if(!(rets[0] == '+' || rets[0] == '-')) *ptr++ = '+';
    strcpy(ptr,rets);
  }
  return ix86_apistr;
}

#ifdef INT64_C
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
#ifdef INT64_C
  if(x86_Bitness == DAB_USE64 && wrd == -1) /* special case for */
    do_64 = 1;
#endif
  cl = do_64 ? 8 : wrd ? ( Use32Data ? 4 : 2 ) : 1;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
#ifdef INT64_C
    if(do_64)
      type = sgn ? DISARG_LLONG : DISARG_QWORD;
    else
#endif
    type = sgn ? wrd ? ( Use32Data ? DISARG_LONG : DISARG_SHORT ) : DISARG_CHAR:
                 wrd ? ( Use32Data ? DISARG_DWORD : DISARG_WORD ) : DISARG_BYTE;
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
    seg = Use32Data ? *((short  *)(&DisP->RealCmd[5])) : *((short  *)(&DisP->RealCmd[3]));
    off = Use32Data ? *((long  *)(&DisP->RealCmd[1])) : (long)(*((short  *)(&DisP->RealCmd[1])));
    newpos = ((long)(seg) << 4) + off; /* It is computing of x86 real-mode address */
  }
  DisP->codelen += Use32Data ? 6 : 4;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendFAddr(str,DisP->CodeAddress + 1,off,newpos,
                   Use32Data ? DISADR_FAR32 : DISADR_FAR16,seg,DisP->codelen-1);
}

void __FASTCALL__ ix86_ArgNear(char * str,ix86Param *DisP)
{
  long lshift = 0L;
  unsigned long newpos;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    lshift = Use32Data ? *((long  *)(&DisP->RealCmd[1])) :
                         (long)(*((short  *)(&DisP->RealCmd[1])));
  DisP->codelen += Use32Data ? 4 : 2;
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    newpos = DisP->DisasmPrefAddr + lshift + DisP->codelen;
    disAppendFAddr(str,DisP->CodeAddress + 1,lshift,newpos,
                   Use32Data ? DISADR_NEAR32 : DISADR_NEAR16,0,DisP->codelen-1);
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

static void __NEAR__ __FASTCALL__ getSIBRegs(char * base,char * scale,char * _index,char *mod,char code)
{
  char scl = (code & 0xC0) >> 6;
  char bas = code & 0x07;
  char ind = (code & 0x38) >> 3;
  tBool use32data = Use32Data;
  tBool useMMX = UseMMXSet;
  tBool useXMMX = UseXMMXSet;
  tBool brex, use64;
  Use32Data = True;
  UseMMXSet = False;
  UseXMMXSet = False;
  brex = use64 = 0;
  if(scl)
  {
    scale[0] = '*';
    scale[1] = ( 1 << scl ) + 0x30;
    scale[2] = 0;
  }
  else scale[0] = 0;
  if(ind == 4) _index[0] = 0;
  else 
  {

#ifdef INT64_C
     if(x86_Bitness == DAB_USE64)
     {
         /* trickly: use32 is set by default. If it's unset then there is 67 prefix */
         use64 = Use32Addr?1:0;
         brex = (k86_REX&2)>>1;
     }
#endif
     strcpy(_index,k86_getREG(ind,True,brex,use64));
  }
  if(bas == 5 && *mod == 0) { base[0] = 0; *mod = 2; }
  else
  {
#ifdef INT64_C
     if(x86_Bitness == DAB_USE64)
     {
         /* trickly: use32 is set by default. If it's unset then there is 67 prefix */
         use64 = Use32Addr?1:0;
         brex = (k86_REX&1)>>0;
     }
#endif
    strcpy(base,k86_getREG(bas,True,brex,use64));
  }
  Use32Data = use32data;
  UseMMXSet = useMMX;
  UseXMMXSet = useXMMX;
#if 0
  return ind == 4 || ind == 5 || bas == 4 || bas == 5; /** return is_stack */
#endif
}

static char * __NEAR__ __FASTCALL__ ConstrSibMod(char * store,char * scale,char * _index,char * base,char code,char *mod)
{
   getSIBRegs(base,scale,_index,mod,code);
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
 tBool as_sign,is_disponly;
 clen = 2;
 if(rm == 4 && Use32Addr) cptr = ConstrSibMod(ret1,base,_index,scale,DisP->RealCmd[2],&new_mode);
 else
 {
#ifdef INT64_C
   if(x86_Bitness == DAB_USE64)
   {
      cptr = Use32Addr ? k86_QWordRegs[REX_reg(k86_REX&1,rm)] : k86_DWordRegs[REX_reg(k86_REX&1,rm)];
   }
   else
#endif
   cptr = Use32Addr ? ix86_DWordRegs[rm] : ix86_A16[rm];
#if 0
   is_stack = Use32Addr ? rm == 4 || rm == 5 : rm == 2 || rm == 3 || rm == 6;
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
 switch(mod)
 {
       case 0:
             {
               if((rm != 6 && !Use32Addr) || (rm != 5 && Use32Addr)) /* i.e. combine address */
               {
                 clen = 0;
                 if(Use32Addr && rm == 4) clen = 1;
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
               if(!Use32Addr)
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
#ifdef INT64_C
		 if(x86_Bitness == DAB_USE64 && is_disponly) strcat(ix86_modrm_ret,"rip");
#endif
              if(!Use32Addr)
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
                strcat(ix86_modrm_ret,Get8SquareDig(tmp,DisP,as_sign,is_disponly));
              }
            }
            break;
      default:
            {
              tBool brex, use64;
              clen = 0;
              brex = use64 = 0;
#ifdef INT64_C
	      if(x86_Bitness == DAB_USE64)
              {
                brex = k86_REX&1;
                use64 = Use64;
              }
#endif
              strcat(ix86_modrm_ret,k86_getREG(rm,w,brex,use64));
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
#ifdef INT64_C
 if(x86_Bitness == DAB_USE64)
 {
   brex = (k86_REX&4)>>2;
   use64 = Use64;
 }
#endif
 regs = k86_getREG(reg,w,brex,use64);
#ifdef INT64_C
 if(x86_Bitness == DAB_USE64) brex = k86_REX&1;
#endif
 if(mod == 3) modrm = k86_getREG(rm,w,brex,use64);
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
  int sizptr;
  sizptr = Use32Data ? DWORD_PTR : WORD_PTR;
  ix86_setModifier(str,ix86_sizes[sizptr]);
  DisP->codelen++;
  strcat(str,ix86_getModRM(True,mod,rm,DisP));
}

void __FASTCALL__ ix86_ArgModB(char *str,ix86Param *DisP)
{
  char mod = (DisP->RealCmd[1] >> 6) & 0x03;
  char rm = (DisP->RealCmd[1] & 0x07);
  ix86_setModifier(str,ix86_sizes[BYTE_PTR]);
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
  tBool use32data = Use32Data;
  Use32Data = False;
  sregptr = getSREG(sreg);
  DisP->codelen = 2;
  tileptr = ix86_getModRM(True,mod,reg,DisP);
  if(sreg > 3) if((DisP->pro_clone & IX86_CPUMASK) < 3) DisP->pro_clone = IX86_CPU386;
  Use32Data = use32data;
  strcat(str,direct ? sregptr : tileptr);
  ix86_CStile(str,direct ? tileptr : sregptr);
}

void __FASTCALL__ ix86_ArgSegRM(char * str,ix86Param *DisP)
{
  char sreg  = (DisP->RealCmd[1] & 0x38) >> 3;
  char reg   = DisP->RealCmd[1] & 0x07;
  char mod   = (DisP->RealCmd[1] & 0xC0) >> 6;
  const char *tileptr,*sregptr;
  tBool use32data = Use32Data;
  Use32Data = False;
  sregptr = getSREG(sreg);
  strcat(str,sregptr);
  DisP->codelen++;
  tileptr = ix86_getModRM(True,mod,reg,DisP);
  if(sreg > 3) if((DisP->pro_clone & IX86_CPUMASK) < 3) DisP->pro_clone = IX86_CPU386;
  Use32Data = use32data;
  ix86_CStile(str,tileptr);
}

void __FASTCALL__ ix86_ArgAXDigit(char * str,ix86Param *DisP)
{
  char w = DisP->RealCmd[0] & 0x01;
  tBool use64=0;
#ifdef INT64_C
  if(x86_Bitness == DAB_USE64) use64 = Use64;
  /* I guess that K9 will support "OP rax, imm64" forms
     same as "OP rrx, imm64" since they are not longer
     than 15 bytes in length
     (example: "adc r13, 123456789ABCDEF0").
     TODO: if(use64) w = -1; here */
#endif
  strcat(str,k86_getREG(0,w,0,use64));
  ix86_CStile(str,ix86_GetDigitTile(DisP,w,0,1));
}

void __FASTCALL__ ix86_ArgRMDigit(char * str,ix86Param *DisP)
{
  char w = DisP->RealCmd[0] & 0x01;
  char mod = (DisP->RealCmd[1] & 0xC0) >> 6;
  char rm  = (DisP->RealCmd[1] & 0x07);
  char *a1,*a2;
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
#ifdef INT64_C
  if(x86_Bitness == DAB_USE64)
  {
    brex = k86_REX&1;
    use64 = Use64;
  }
#endif
  strcat(str,k86_getREG(reg,w,brex,use64));
#ifdef INT64_C
  if(x86_Bitness == DAB_USE64 && Use64 && w) w = -1;
#endif
  ix86_CStile(str,ix86_GetDigitTile(DisP,w,0,1));
}

void __FASTCALL__ ix86_ArgIReg(char *str,ix86Param *DisP)
{
  tBool brex,use64;
  brex = use64 = 0;
#ifdef INT64_C
  if(x86_Bitness == DAB_USE64)
  {
    brex = k86_REX&1;
    use64 = Use64;
  }
#endif
  strcat(str,k86_getREG(DisP->RealCmd[0] & 0x07,True,brex,use64));
}

void __FASTCALL__ ix86_ArgAXIReg(char *str,ix86Param *DisP)
{
  tBool brex,use64;
  brex = use64 = 0;
#ifdef INT64_C
  if(x86_Bitness == DAB_USE64)
  {
    brex = k86_REX&1;
    use64 = Use64;
  }
#endif
  strcat(str,k86_getREG(0,True,0,use64));
  ix86_CStile(str,k86_getREG(DisP->RealCmd[0] & 0x07,True,brex,use64));
}

void __FASTCALL__ ix86_ArgSInt(char *str,ix86Param *DisP)
{
  ix86_setModifier(str,Use32Data ? ix86_sizes[DWORD_PTR] : ix86_sizes[WORD_PTR]);
  if(!((DisP->flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
    disAppendDigits(str,DisP->CodeAddress + DisP->codelen,
                 APREF_USE_TYPE,1,&DisP->RealCmd[DisP->codelen],DISARG_CHAR | DISARG_IMM);
  DisP->codelen++;
}

void __FASTCALL__ ix86_ArgInt(char *str,ix86Param *DisP)
{
  tBool use64;
  use64 = 0;
#ifdef INT64_C
  if(x86_Bitness == DAB_USE64) use64 = Use64;
#endif
  use64 ? ix86_ArgQWord(str,DisP) : Use32Data ? ix86_ArgDWord(str,DisP) : ix86_ArgWord(str,DisP);
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
   if(w) ix86_setModifier(str,Use32Data ? ix86_sizes[DWORD_PTR] : ix86_sizes[WORD_PTR]);
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
   ix86_setModifier(str,w ? Use32Data ? ix86_sizes[DWORD_PTR] : ix86_sizes[WORD_PTR] : ix86_sizes[BYTE_PTR]);
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
   ix86_setModifier(str,w ? Use32Data ? ix86_sizes[DWORD_PTR] : ix86_sizes[WORD_PTR] : ix86_sizes[BYTE_PTR]);
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
   ix86_setModifier(str,w ? Use32Data ? ix86_sizes[DWORD_PTR] : ix86_sizes[WORD_PTR] : ix86_sizes[BYTE_PTR]);
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
   ix86_setModifier(str,w ? Use32Data ? ix86_sizes[DWORD_PTR] : ix86_sizes[WORD_PTR] : ix86_sizes[BYTE_PTR]);
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
   unsigned char sizptr;
   unsigned oldDisNeedRef;
   DisP->codelen++;
   strcpy(str,ix86_Gr2Names[code2]);
   /*
      Added by "Kostya Nosov" <k-nosov@yandex.ru>:
      make NEEDREF_ALL for indirect "jmp" and "call"
   */
   oldDisNeedRef = disNeedRef;
   if(code2 >= 2 && code2 <= 5 && disNeedRef != NEEDREF_NONE)
                                             disNeedRef = NEEDREF_ALL;
   TabSpace(str,TAB_POS);
   if(code2 == 0 || code2 == 1) { strcat(str,ix86_getModRM(w,mod,rm,DisP)); } /** inc dec */
   else
   {
     wrd = code2 == 0x06 || code2 == 0x07 ? 0 : wrd; /** push / pop; */
     if(!wrd) sizptr = Use32Data ? DWORD_PTR : WORD_PTR;
     else     sizptr = Use32Data ? PWORD_PTR : DWORD_PTR;
     ix86_setModifier(str,ix86_sizes[sizptr]);
     strcat(str,ix86_getModRM(True,mod,rm,DisP));
   }
   disNeedRef = oldDisNeedRef;
}

void __FASTCALL__ ix86_ArgAXMem(char *str,ix86Param *DisP)
{
  unsigned char d = DisP->RealCmd[0] & 0x02;
  const char *mem,*reg;
  tBool use64;
#ifdef INT64_C
  unsigned char sav_REX;
  sav_REX = k86_REX;
#endif
  use64 = 0;
#ifdef INT64_C
  if(x86_Bitness == DAB_USE64 && Use64)
  {
    use64 = Use64;
    mem = Get16SquareDig(1,DisP,False,True);
  }
  else
#endif
    mem = Use32Addr ? Get8SquareDig(1,DisP,False,True) : Get4SquareDig(1,DisP,False,True);
  strcpy(ix86_appstr,ix86_segpref);
  ix86_segpref[0] = 0;
  strcat(ix86_appstr,"[");
  strcat(ix86_appstr,mem);
  strcat(ix86_appstr,"]");
#ifdef INT64_C
  if(x86_Bitness == DAB_USE64 && Use64) k86_REX=0;
#endif
  reg = k86_getREG(0,DisP->RealCmd[0] & 0x01,0,use64);
#ifdef INT64_C
  if(x86_Bitness == DAB_USE64 && Use64) k86_REX=sav_REX;
#endif
#ifdef INT64_C
  if(x86_Bitness == DAB_USE64 && Use64)
    DisP->codelen = 9;
  else
#endif
  DisP->codelen = Use32Addr ? 5 : 3;
  strcat(str,d ? ix86_appstr : reg);
  ix86_CStile(str,d ? reg : ix86_appstr);
}

void __FASTCALL__ ix86_InOut(char * str,ix86Param *DisP)
{
  const char *arg,*reg1,*reg2,*regptr,*dig;
  char i;
  tBool use64;
  use64 = 0;
#ifdef INT64_C
  if(x86_Bitness == DAB_USE64) use64 = Use64;
#endif
  regptr = k86_getREG(0,DisP->RealCmd[0] & 0x01,0,use64);
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

/** 0f xx opcodes */

void  __FASTCALL__ ix86_ExOpCodes(char *str,ix86Param *DisP)
{
 unsigned char code;
 DisP->RealCmd = &DisP->RealCmd[1];
 DisP->CodeAddress++;
 code = DisP->RealCmd[0];
 DisP->codelen++;
#ifdef INT64_C
 if(x86_Bitness == DAB_USE64)
 {
    strcpy(str,ix86_extable[code].name64);
 }
 else
#endif
 strcpy(str,ix86_extable[code].name);
#ifdef INT64_C
 if(x86_Bitness == DAB_USE64)
 {
   if(ix86_extable[code].method64)
   {
     TabSpace(str,TAB_POS);
     ix86_extable[code].method64(str,DisP);
   }
 }
 else
#endif
 if(ix86_extable[code].method)
 {
   TabSpace(str,TAB_POS);
   ix86_extable[code].method(str,DisP);
 }
#ifdef INT64_C
 if(x86_Bitness == DAB_USE64) DisP->pro_clone = ix86_extable[code].flags64 & K64_CLONEMASK;
 else
#endif
 if(DisP->pro_clone < ix86_extable[code].pro_clone) DisP->pro_clone = ix86_extable[code].pro_clone;
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
}

void  __FASTCALL__ ix86_ArgExGr1(char *str,ix86Param *DisP)
{
    unsigned char rm = (DisP->RealCmd[1] & 0x38) >> 3;
    unsigned char reg = (DisP->RealCmd[1] & 0x07);
    unsigned char mod = (DisP->RealCmd[1] & 0xC0) >> 6;
    unsigned char buf,ptr;
    tBool word;
#ifdef INT64_C
    if(x86_Bitness == DAB_USE64)
    {
      if(DisP->RealCmd[1] == 0xF8)
      strcpy(str,"swapgs");
      DisP->codelen++;
      return;
    }
#endif
    strcpy(str,ix86_ExGrp1[rm]);
    TabSpace(str,TAB_POS);
    DisP->codelen++;
    buf = DisP->RealCmd[1];
    ptr = PWORD_PTR;
    word = True;
    if(rm == 7)
    {
      word = False;
      DisP->pro_clone = IX86_CPU486;
      buf = 1;
      ptr = DUMMY_PTR;
    }
    else  if(rm == 4 || rm == 6) ptr = WORD_PTR;
    ix86_setModifier(str,ix86_sizes[ptr]);
    strcat(str,ix86_getModRM(word ? True : buf & 0x01,mod,reg,DisP));
}

static const char ** xry[] = { ix86_CrxRegs, ix86_DrxRegs, ix86_TrxRegs, ix86_XrxRegs };

void  __FASTCALL__ ix86_ArgMovXRY(char *str,ix86Param *DisP)
{
    unsigned char code = DisP->RealCmd[0];
    unsigned char xreg = (DisP->RealCmd[1] & 0x38) >> 3;
    unsigned char reg = DisP->RealCmd[1] & 0x07;
    unsigned char ridx = (code & 0x01) + ((code >> 1) & 0x02);
    unsigned char d = (code >> 1) & 0x01;
    tBool brex,use64;
    brex = use64 = 0;
#ifdef INT64_C
    if(x86_Bitness == DAB_USE64)
    {
       brex = k86_REX & 1;
       use64 = Use64;
    }
#endif
    DisP->codelen++;
    Use32Data = True;
    strcat(str,d ? xry[ridx][xreg] : k86_getREG(reg,True,brex,use64));
    ix86_CStile(str,d ? k86_getREG(reg,True,brex,use64) : xry[ridx][xreg]);
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
   if(as_xmmx) UseXMMXSet = True;
   else        UseMMXSet = True;
   if((DisP->RealCmd[1] & 0xC0) != 0xC0)
   {
     direct ? ix86_ArgModRMnDW(str,DisP) : ix86_ArgModRMDW(str,DisP);
     if(as_xmmx) UseXMMXSet = False;
     else        UseMMXSet = False;
   }
   else
   {
      const char *mmx,*exx;
      tBool brex,use64;
      brex = use64 = 0;
#ifdef INT64_C
      if(x86_Bitness == DAB_USE64)
      {
         brex = (k86_REX&4)>>2;
         use64 = Use64;
         if(brex && !as_xmmx) brex = 0; /* Note: there are only 8 mmx registers */
      }
#endif
      mmx = k86_getREG((DisP->RealCmd[1] >> 3) & 0x07,True,brex,use64);
      if(as_xmmx) UseXMMXSet = False;
      else        UseMMXSet = False;
#ifdef INT64_C
      if(x86_Bitness == DAB_USE64) brex = k86_REX&1;
#endif
      exx = k86_getREG(DisP->RealCmd[1] & 0x07,True,brex,use64);
      strcat(str,direct ? exx : mmx);
      strcat(str,",");
      strcat(str,direct ? mmx : exx);
      DisP->codelen++;
   }
}

static void __NEAR__ __FASTCALL__ ix86_ArgxMMRev(char *str,ix86Param *DisP,tBool direct,tBool as_xmmx)
{
   if(as_xmmx) UseXMMXSet = True;
   else        UseMMXSet = True;
   if((DisP->RealCmd[1] & 0xC0) != 0xC0)
   {
     direct ? ix86_ArgModRMnDW(str,DisP) : ix86_ArgModRMDW(str,DisP);
     if(as_xmmx) UseXMMXSet = True;
     else        UseMMXSet = False;
   }
   else
   {
      const char *mmx,*exx;
      tBool brex,use64;
      brex = use64 = 0;
#ifdef INT64_C
      if(x86_Bitness == DAB_USE64)
      {
         brex = k86_REX&1;
         use64 = Use64;
         if(brex && !as_xmmx) brex = 0; /* Note: there are only 8 mmx registers */
      }
#endif
      mmx = k86_getREG(DisP->RealCmd[1] & 0x07,True,brex,use64);
      if(as_xmmx) UseXMMXSet = False;
      else        UseMMXSet = False;
#ifdef INT64_C
      if(x86_Bitness == DAB_USE64) brex = (k86_REX&4)>>2;
#endif
      exx = k86_getREG((DisP->RealCmd[1] >> 3) & 0x07,True,brex,use64);
      strcat(str,direct ? exx : mmx);
      strcat(str,",");
      strcat(str,direct ? mmx : exx);
      DisP->codelen++;
   }
}

static void __NEAR__ __FASTCALL__ ix86_ArgxMMXGroup(char *str,const char *name,ix86Param *DisP,tBool as_xmmx)
{
  tBool brex,use64;
  brex = use64 = 0;
#ifdef INT64_C
  if(x86_Bitness == DAB_USE64)
  {
    brex = k86_REX&1;
    use64 = Use64;
    if(brex && !as_xmmx) brex = 0; /* Note: there are only 8 mmx registers */
  }
#endif
  DisP->codelen++;
  strcpy(str,name);
  TabSpace(str,TAB_POS);
  if(as_xmmx) UseXMMXSet = True;
  else        UseMMXSet = True;
  strcat(str,k86_getREG(DisP->RealCmd[1] & 0x07,True,brex,use64));
  if(as_xmmx) UseXMMXSet = False;
  else        UseMMXSet = False;
  strcat(str,",");
  strcat(str,Get2Digit(DisP->RealCmd[2]));
  DisP->codelen++;
}

/* This function does not have analogs from MMX set. SSE(2) only */

static void __NEAR__ __FASTCALL__ ix86_ArgRXMM(char *str,ix86Param *DisP,tBool direct)
{
   UseXMMXSet = True;
   if((DisP->RealCmd[1] & 0xC0) != 0xC0)
   {
     direct ? ix86_ArgModRMnDW(str,DisP) : ix86_ArgModRMDW(str,DisP);
     UseXMMXSet = False;
   }
   else
   {
      const char *mmx,*exx;
      tBool brex,use64;
      brex = use64 = 0;
#ifdef INT64_C
      if(x86_Bitness == DAB_USE64)
      {
        brex = (k86_REX&4)>>2;
        use64 = Use64;
      }
#endif
      mmx = k86_getREG((DisP->RealCmd[1] >> 3) & 0x07,True,brex,use64);
      UseXMMXSet = False;
#ifdef INT64_C
      if(x86_Bitness == DAB_USE64) brex = k86_REX&1;
#endif
      exx = k86_getREG(DisP->RealCmd[1] & 0x07,True,brex,use64);
      strcat(str,direct ? exx : mmx);
      strcat(str,",");
      strcat(str,direct ? mmx : exx);
      DisP->codelen++;
   }
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
#ifdef INT64_C
      if(x86_Bitness == DAB_USE64)
      {
        brex = k86_REX&1;
        use64 = Use64;
        if(brex && !as_xmmx) brex = 0; /* Note: there are only 8 mmx registers */
      }
#endif
      if(as_xmmx) UseXMMXSet = True;
      else        UseMMXSet = True;
      mmx = k86_getREG(DisP->RealCmd[1] & 0x07,True,brex,True);
      if(as_xmmx) UseXMMXSet = False;
      else        UseMMXSet = False;
#ifdef INT64_C
      if(x86_Bitness == DAB_USE64) brex = (k86_REX&4)>>2;
#endif
      exx = k86_getREG((DisP->RealCmd[1] >> 3) & 0x07,True,brex,use64);
      strcat(str,direct ? mmx : exx);
      strcat(str,",");
      strcat(str,direct ? exx : mmx);
      DisP->codelen++;
   }
}

static void __NEAR__ __FASTCALL__ ix86_ArgxMMxMM(char *str,ix86Param *DisP,tBool direct,tBool xmmx_first)
{
   if(xmmx_first) UseXMMXSet = True;
   else           UseMMXSet = True;
   if((DisP->RealCmd[1] & 0xC0) != 0xC0)
   {
     direct ? ix86_ArgModRMnDW(str,DisP) : ix86_ArgModRMDW(str,DisP);
     if(xmmx_first) UseXMMXSet = False;
     else           UseMMXSet = False;
   }
   else
   {
      const char *mmx,*exx;
      tBool brex,use64;
      brex = use64 = 0;
#ifdef INT64_C
      if(x86_Bitness == DAB_USE64)
      {
        brex = (k86_REX&4)>>2;
        use64 = Use64;
        if(brex && !xmmx_first) brex = 0; /* Note: there are only 8 mmx registers */
      }
#endif
      mmx = k86_getREG((DisP->RealCmd[1] >> 3) & 0x07,True,brex,use64);
      if(xmmx_first)
      {
        UseXMMXSet = False;
        UseMMXSet = True;
      }
      else
      {
        UseXMMXSet = True;
        UseMMXSet = False;
      }
#ifdef INT64_C
      if(x86_Bitness == DAB_USE64) brex = k86_REX&1;
#endif
      exx = k86_getREG(DisP->RealCmd[1] & 0x07,True,brex,use64);
      if(xmmx_first) UseMMXSet = False;
      else           UseXMMXSet = False;
      strcat(str,direct ? exx : mmx);
      strcat(str,",");
      strcat(str,direct ? mmx : exx);
      DisP->codelen++;
   }
}

void __FASTCALL__ ix86_ArgMMXD(char *str,ix86Param *DisP)
{
     UseMMXSet = True;
     ix86_ArgModRMnDW(str,DisP);
     UseMMXSet = False;
}

void __FASTCALL__ ix86_ArgMMXnD(char *str,ix86Param *DisP)
{
     UseMMXSet = True;
     ix86_ArgModRMDW(str,DisP);
     UseMMXSet = False;
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
     UseXMMXSet = True;
     ix86_ArgModRMnDW(str,DisP);
     UseXMMXSet = False;

}

void  __FASTCALL__ ix86_ArgXMMXnD(char *str,ix86Param *DisP)
{
     UseXMMXSet = True;
     ix86_ArgModRMDW(str,DisP);
     UseXMMXSet = False;
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
      tBool u32data;
      tBool brex,use64;
      brex = use64 = 0;
#ifdef INT64_C
      if(x86_Bitness == DAB_USE64)
      {
        brex = (k86_REX&4)>>2;
        use64 = Use64;
      }
#endif
      mod = (DisP->RealCmd[1] >> 6) & 0x03;
      ix86_setModifier(str,DisP->RealCmd[0] & 0x01 ? Use32Data ? ix86_sizes[DWORD_PTR] : ix86_sizes[WORD_PTR] : ix86_sizes[BYTE_PTR]);
      dst = k86_getREG((DisP->RealCmd[1] >> 3) & 0x07,True,brex,use64);
      u32data = Use32Data;
      Use32Data = False;
      src = ix86_getModRM(DisP->RealCmd[0] & 0x01,mod,DisP->RealCmd[1] & 0x07,DisP);
      Use32Data = u32data;
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
   if(code2 == 5 || code2 == 6) DisP->pro_clone = IX86_P4MMX;
}

void __FASTCALL__ ix86_ArgKatmaiGrp2(char *str,ix86Param *DisP)
{
   unsigned char code2 = ( DisP->RealCmd[1] & 0x38 ) >> 3;
   strcpy(str,ix86_KatmaiGr2Names[code2]);
   TabSpace(str,TAB_POS);
   ix86_ArgMod(str,DisP);
}

void __FASTCALL__  ix86_ArgXMMRMDigit(char *str,ix86Param *DisP)
{
   char * a1,*a2;
   UseXMMXSet = True;
   DisP->codelen++;
   a1 = ix86_getTileWD(DisP);
   strcat(str,a1);
   a2 = ix86_GetDigitTile(DisP,0,0,DisP->codelen-1);
   ix86_CStile(str,a2);
   UseXMMXSet = False;
}

void __FASTCALL__  ix86_ArgXMMCmp(char *str,ix86Param *DisP)
{
   char * a1,*a2;
   unsigned char suffix;
   char name[6], realname[10];
   UseXMMXSet = True;
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
   UseXMMXSet = False;
}

void __FASTCALL__  ix86_ArgMMRMDigit(char *str,ix86Param *DisP)
{
   char * a1,*a2;
   DisP->codelen++;
   UseMMXSet = True;
   a1 = ix86_getTileWD(DisP);
   strcat(str,a1);
   a2 = ix86_GetDigitTile(DisP,0,0,DisP->codelen-1);
   ix86_CStile(str,a2);
   UseMMXSet = False;
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
 UseMMXSet = True;
 ix86_Katmai_buff[0] = 0;
 ix86_ArgModRMDW(ix86_Katmai_buff,DisP);
 UseMMXSet = False;
 code = DisP->RealCmd[DisP->codelen-1];
 DisP->codelen++;
 strcpy(str,ix86_3dNowtable[code].name);
 TabSpace(str,TAB_POS);
 strcat(str,ix86_Katmai_buff);
 DisP->pro_clone = ix86_3dNowtable[code].pro_clone;
}

void __FASTCALL__ ix86_3dNowPrefetchGrp(char *str,ix86Param *DisP)
{
   unsigned char mod = ( DisP->RealCmd[1] & 0x38 ) >> 3;
   strcpy(str,ix86_3dPrefetchGrp[mod]);
   TabSpace(str,TAB_POS);
   ix86_ArgMod(str,DisP);
}


