/**
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/ix86/ix86_fpu.c
 * @brief       This file contains implementation of Intel x86 disassembler for
 *              FPU instructions set.
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
**/
#include <stdlib.h>
#include <string.h>

#include "biewutil.h"
#include "reg_form.h"
#include "plugins/disasm/ix86/ix86.h"

extern char *ix86_appstr;

static char * __NEAR__ __FASTCALL__ SetNameTab(char * str,const char * name)
{
 strcpy(str,name);
 TabSpace(str,TAB_POS);
 return str;
}

static char * __NEAR__ __FASTCALL__ SC(const char * name1,const char * name2)
{
 SetNameTab(ix86_appstr,name1);
 strcat(ix86_appstr,name2);
 return ix86_appstr;
}

static char * __NEAR__ __FASTCALL__ SetNameTabD(char * str,const char * name,unsigned char size,ix86Param *DisP)
{
 strcpy(str,name);
 strcat(str," ");
 if(size < 7 && ((DisP->RealCmd[1] >> 6) & 0x03) != 3) strcat(str,ix86_sizes[size]);
 TabSpace(str,TAB_POS);
 return str;
}

typedef char * (__NEAR__ __FASTCALL__ * FPUroutine)(char *,const char *,ix86Param *);
typedef struct tagFPUcall
{
  FPUroutine   f;
  const char * c;
}FPUcall;

typedef struct tgDualStr
{
 const char * c1;
 const char * c2;
}DualStr;

static char stx[] = "st(x)";
#define MakeST(str,num) { stx[3] = (num)+'0'; strcat(str,stx); }

static char * __NEAR__ __FASTCALL__ __UniFPUfunc(char * str,const char * cmd,char opsize,char direct,ix86Param *DisP)
{
 char mod = ( DisP->RealCmd[1] & 0xC0 ) >> 6;
 char reg = DisP->RealCmd[1] & 0x07;
 char *modrm;
 modrm = ix86_getModRM(True,mod,reg,DisP);
 SetNameTabD(str,cmd,opsize,DisP);
 if(!direct)  MakeST(str,0)
 else         strcat(str,modrm);
 strcat(str,",");
 if(direct)  MakeST(str,0)
 else        strcat(str,modrm);
 return str;
}

static char * __NEAR__ __FASTCALL__ __MemFPUfunc(char * str,const char * cmd,char opsize,ix86Param *DisP)
{
 char mod = ( DisP->RealCmd[1] & 0xC0 ) >> 6;
 char reg = DisP->RealCmd[1] & 0x07;
 char *modrm;
 modrm = ix86_getModRM(True,mod,reg,DisP);
 SetNameTabD(str,cmd,opsize,DisP);
 strcat(str,modrm);
 return str;
}

static char * __NEAR__ __FASTCALL__ FPUmem(char * str,const char * cmd,ix86Param *DisP)
{
  return __MemFPUfunc(str,cmd,DUMMY_PTR,DisP);
}

static char * __NEAR__ __FASTCALL__ FPUmem64mem32(char * str,const char * cmd,ix86Param *DisP)
{
 return __UniFPUfunc(str,cmd,DisP->RealCmd[0] & 0x04 ? QWORD_PTR : DWORD_PTR,0,DisP);
}

static char * __NEAR__ __FASTCALL__ FPUmem64mem32st(char * str,const char * cmd,ix86Param *DisP)
{
 return __UniFPUfunc(str,cmd,DisP->RealCmd[0] & 0x04 ? QWORD_PTR : DWORD_PTR,1,DisP);
}

static char * __NEAR__ __FASTCALL__ FPUint16int32(char * str,const char * cmd,ix86Param *DisP)
{
 return __UniFPUfunc(str,cmd,DisP->RealCmd[0] & 0x04 ? WORD_PTR : DWORD_PTR,0,DisP);
}

static char * __NEAR__ __FASTCALL__ FPUint16int32st(char * str,const char * cmd,ix86Param *DisP)
{
 return __UniFPUfunc(str,cmd,DisP->RealCmd[0] & 0x04 ? WORD_PTR : DWORD_PTR,1,DisP);
}

static char * __NEAR__ __FASTCALL__ FPUint64(char * str,const char * cmd,ix86Param *DisP)
{
 return __UniFPUfunc(str,cmd,QWORD_PTR,0,DisP);
}

static char * __NEAR__ __FASTCALL__ FPUint64st(char * str,const char * cmd,ix86Param *DisP)
{
 return __UniFPUfunc(str,cmd,QWORD_PTR,1,DisP);
}

static char * __NEAR__ __FASTCALL__ FPUstint32(char * str,const char * cmd,ix86Param *DisP)
{
 return __UniFPUfunc(str,cmd,DWORD_PTR,0,DisP);
}

static char * __NEAR__ __FASTCALL__ FPUld(char * str,const char * cmd,ix86Param *DisP)
{
 return __UniFPUfunc(str,cmd,DUMMY_PTR,0,DisP);
}

static char * __NEAR__ __FASTCALL__ FPUstisti(char * str,const char * cmd,char code1,char code2)
{
 SetNameTab(str,cmd);
 MakeST(str,code1 & 0x07);
 strcat(str,",");
 MakeST(str,code2 & 0x07);
 return str;
}

static char * __NEAR__ __FASTCALL__ FPUst0sti(char * str,const char * cmd,char code1)
{
 return FPUstisti(str,cmd,0,code1);
}

static char * __NEAR__ __FASTCALL__ FPUstist0(char * str,const char * cmd,char code1)
{
 return FPUstisti(str,cmd,code1,0);
}

static char * __NEAR__ __FASTCALL__ FPUldtword(char * str,const char * cmd,ix86Param *DisP)
{
 return __UniFPUfunc(str,cmd,TWORD_PTR,0,DisP);
}

static char * __NEAR__ __FASTCALL__ FPUsttword(char * str,const char * cmd,ix86Param *DisP)
{
  return __UniFPUfunc(str,cmd,TWORD_PTR,1,DisP);
}

static char * __NEAR__ __FASTCALL__ FPUcmdsti(char * str,const char * name,char code)
{
  SetNameTab(str,name);
  MakeST(str,code & 0x07);
  return str;
}

static char * __NEAR__ __FASTCALL__ FPUcmdst0(char * str,const char * name)
{
  return FPUcmdsti(str,name,0);
}

static char * __NEAR__ __FASTCALL__ FPUcmdsti_2(char * str,const char * name1,const char * name2,char code)
{
  return FPUcmdsti(str,code & 0x08 ? name2 : name1,code);
}

static char * __NEAR__ __FASTCALL__ FPUst0sti_2(char * str,const char * name1,const char * name2,char code)
{
 return FPUst0sti(str,code & 0x08 ? name2 : name1,code);
}

static char * __NEAR__ __FASTCALL__ FPUstist0_2(char * str,const char * name1,const char * name2,char code)
{
 return FPUstist0(str,code & 0x08 ? name2 : name1,code);
}

const char * mem64mem32[] =
{
  "fadd", "fmul", "fcom", "fcomp", "fsub", "fsubr", "fdiv", "fdivr"
};

const char * int16int32[] =
{
 "fiadd", "fimul", "ficom", "ficomp", "fisub", "fisubr", "fidiv", "fidivr"
};

const char * DBEx[] = { "feni", "fdisi", "fclex", "finit", "fsetpm" };
const char * D9Ex[] = { "fchs", "fabs", "f???", "f???", "ftst", "fxam", "f???", "f???",
                        "fld1", "fldl2t", "fldl2e", "fldpi", "fldlg2", "fldln2", "fldz", "f???" };
const char * D9Fx[] = { "f2xm1", "fyl2x", "fptan", "fpatan", "fxtract", "fprem1", "fdecstp", "fincstp",
                        "fprem", "fyl2xp1", "fsqrt", "fsincos", "frndint", "fscale", "fsin", "fcos" };

FPUcall D9rm[8] =
{
  { FPUstint32,      "fld" },
  { FPUld,           "f???" },
  { FPUmem64mem32st, "fst" },
  { FPUmem64mem32st, "fstp" },
  { FPUmem,          "fldenv" },
  { FPUmem,          "fldcw" },
  { FPUmem,          "fstenv" },
  { FPUmem,          "fstcw" }
};

FPUcall DBrm[8] =
{
  { FPUint16int32,   "fild" },
  { FPUld,           "fi???" },
  { FPUint16int32st, "fist" },
  { FPUsttword,      "fistp" },
  { FPUld,           "f???" },
  { FPUldtword,      "fld" },
  { FPUld,           "f???" },
  { FPUint64st,      "fstp" }
};

FPUcall DDrm[8] =
{
  { FPUint64,        "fld" },
  { FPUld,           "f???" },
  { FPUmem64mem32st, "fst" },
  { FPUmem64mem32st, "fstp" },
  { FPUld,           "frstor" },
  { FPUld,           "f???" },
  { FPUld,           "fsave" },
  { FPUmem,          "fstsw" }
};

FPUcall DFrm[8] =
{
  { FPUint16int32,   "fild" },
  { FPUld,           "fi???" },
  { FPUint16int32st, "fist" },
  { FPUint16int32st, "fistp" },
  { FPUldtword,      "fbld" },
  { FPUint64,        "fild" },
  { FPUsttword,      "fbstp" },
  { FPUint64st,      "fistp" }
};

DualStr D8str[4] =
{
  { "fadd" , "fmul" },
  { "fcom" , "fcomp" },
  { "fsub" , "fsubr" },
  { "fdiv" , "fdivr" }
};

DualStr DEstr[4] =
{
  { "faddp" , "fmulp" },
  { "fcomp" , "fcompp" },
  { "fsubrp", "fsubp" },
  { "fdivrp", "fdivp" }
};

const char * FCMOVc[] = { "fcmovl", "fcmove", "fcmovle", "fcmovu", "fcmov?", "fcmov?", "fcmov?", "fcmov?" };
const char * FCMOVnc[] = { "fcmovge", "fcmovne", "fcmovg", "fcmovnu", "fcmov?", "fucomi", "fcomi", "f?comi" };
const char * FxCOMIP[] = { "f???", "f???", "f???", "f???", "f???", "fucomip", "fcomip", "f???" };

void __FASTCALL__ ix86_FPUCmd(char * str,ix86Param *DisP)
{
 unsigned char code = DisP->RealCmd[0],code1 = DisP->RealCmd[1];
 unsigned char rm = ( code1 & 0x38 ) >> 3;
 DisP->codelen = 2;
#ifdef INT64_C
 DisP->pro_clone = x86_Bitness==DAB_USE64?K64_ATHLON|K64_FPU:IX86_FPU087;
#else
 DisP->pro_clone = IX86_FPU087;
#endif
 SetNameTab(str,"f???");
 switch(code)
 {
   case 0xD8 :
            if((code1 & 0xF0) >= 0xC0)
            {
              unsigned char _index = (code1 & 0x30) >> 4;
              FPUst0sti_2(str,D8str[_index].c1,D8str[_index].c2,code1);
            }
            else     FPUmem64mem32(str,mem64mem32[rm],DisP);
            break;
   case 0xD9 :
            if((code1 & 0xF0) == 0xE0) FPUcmdst0(str,D9Ex[code1 & 0x0F]);
            else
              if((code1 & 0xF0) == 0xF0)
              {
                if(code1 == 0xF6 || code1 == 0xF7) strcpy(str,D9Fx[code1 & 0x0F]);
                else    FPUcmdst0(str,D9Fx[code1 & 0x0F]);
                if(code1 == 0xF5 || code1 == 0xFB || code1 == 0xFE || code1 == 0xFF)
#ifdef INT64_C
		if(x86_Bitness != DAB_USE64)
#endif
			DisP->pro_clone = IX86_FPU387;
              }
              else
                if(code1 == 0xD0) strcpy(str,"fnop");
                else
                  if((code1 & 0xF0) == 0xC0) FPUcmdsti_2(str,"fld","fxch",code1);
                  else                       (*D9rm[rm].f)(str,D9rm[rm].c,DisP);
            break;
   case 0xDA :
            if(code1 == 0xE9) 
	    {
#ifdef INT64_C
		if(x86_Bitness != DAB_USE64)
#endif
              DisP->pro_clone = IX86_FPU387;
              strcpy(str,SC("fucompp","st(1)"));
	    }
            else
              if((code1 & 0xC0) == 0xC0)
              {
#ifdef INT64_C
		if(x86_Bitness != DAB_USE64)
#endif
                 DisP->pro_clone = IX86_FPU687;
                 FPUst0sti(str,FCMOVc[(code1 >> 3) & 0x07],code1);
              }
              else
                FPUint16int32(str,int16int32[rm],DisP);
            break;
   case 0xDB :
           switch(code1)
           {
             case 0xFC:  
#ifdef INT64_C
		if(x86_Bitness != DAB_USE64)
		{
#endif
                          strcpy(str,SC("frint2","st(0)"));
                          DisP->pro_clone = IX86_FPU487 | IX86_CYRIX;
#ifdef INT64_C
		}
		else strcpy(str,"f???");
#endif
                break;
             default:
             if((code1 & 0xF0) == 0xE0)
             {
               if((code1 & 0x0F) <= 0x04)
               {
                  unsigned char _index = code1 & 0x07;
                  strcpy(str,DBEx[_index]);
#ifdef INT64_C
		  if(x86_Bitness != DAB_USE64)
#endif
                  if(_index == 4) DisP->pro_clone = IX86_FPU287;
               }
               else
                 if((code1 & 0x0F) >= 0x08) goto XC0;
                 else strcpy(str,"f???");
             }
             else
               if((code1 & 0xC0) == 0xC0)
               {
                  XC0:
#ifdef INT64_C
		  if(x86_Bitness != DAB_USE64)
#endif
                  DisP->pro_clone = IX86_FPU387;
                  FPUst0sti(str,FCMOVnc[(code1 >> 3) & 0x07],code1);
               }
               else   (*DBrm[rm].f)(str,DBrm[rm].c,DisP);
            }
            break;
   case 0xDC:
            if((code1 & 0xF0) >= 0xC0)
            {
              unsigned char _index = (code1 & 0x30) >> 4;
              FPUstist0_2(str,_index > 1 ? D8str[_index].c2 : D8str[_index].c1,_index > 1 ? D8str[_index].c1 : D8str[_index].c2,code1);
            }
            else          FPUmem64mem32(str,mem64mem32[rm],DisP);
            break;
   case 0xDD:
            switch(code1)
            {
              case 0xFC:  
#ifdef INT64_C
		if(x86_Bitness != DAB_USE64)
                {
#endif
                          strcpy(str,SC("frichop","st(0)"));
                          DisP->pro_clone = IX86_FPU487 | IX86_CYRIX;
#ifdef INT64_C
		}
                else strcpy(str, "f???");
#endif
                break;
              default:
              if((code1 & 0xF0) == 0xC0) FPUcmdsti_2(str,"ffree","f???",code1);
              else
                if((code1 & 0xF0) == 0xD0) FPUstist0_2(str,"fst","fstp",code1);
                else
                  if((code1 & 0xF0) == 0xE0) 
                  {
#ifdef INT64_C
		    if(x86_Bitness != DAB_USE64)
#endif
                     DisP->pro_clone = IX86_FPU387;
                     FPUcmdsti_2(str,"fucom","fucomp",code1);
                  }
                  else     (*DDrm[rm].f)(str,DDrm[rm].c,DisP);
              break;
            }
            break;
   case 0xDE:
            if((code1 & 0xF0) >= 0xC0)
            {
              unsigned char _index = (code1 & 0x30) >> 4;
              FPUstist0_2(str,DEstr[_index].c1,DEstr[_index].c2,code1);
            }
            else    FPUint16int32(str,int16int32[rm],DisP);
            break;
   default:
   case 0xDF:
           switch(code1)
           {
             case 0xE0:  strcpy(str,SC("fstsw","ax"));
                         break;
             case 0xE1:  strcpy(str,SC("fnstdw","ax"));
#ifdef INT64_C
		         if(x86_Bitness != DAB_USE64)
#endif
                         DisP->pro_clone = IX86_FPU387;
                         break;
             case 0xE2:  strcpy(str,SC("fnstsg","ax"));
#ifdef INT64_C
			 if(x86_Bitness != DAB_USE64)
#endif
                         DisP->pro_clone = IX86_FPU387;
                         break;
             case 0xFC:  
#ifdef INT64_C
			 if(x86_Bitness != DAB_USE64)
			 {
#endif
                         strcpy(str,SC("frinear","st(0)"));
                         DisP->pro_clone = IX86_FPU487 | IX86_CYRIX;
#ifdef INT64_C
			 }
			 else strcpy(str,"f???");
#endif
                         break;
             default:
              {
                if((code1 & 0xE0) == 0xE0)
                {
#ifdef INT64_C
		  if(x86_Bitness != DAB_USE64)
#endif
                  DisP->pro_clone = IX86_FPU687;
                  FPUst0sti(str,FxCOMIP[(code1 >> 3) & 0x07],code1);
                }
                else
                if((code1 & 0xC0) == 0xC0)
                {
#ifdef INT64_C
		 if(x86_Bitness != DAB_USE64)
#endif
                  DisP->pro_clone = IX86_FPU387;
                  FPUcmdsti_2(str,"ffreep","f???",code1);
                }
                else  (*DFrm[rm].f)(str,DFrm[rm].c,DisP);
              }
              break;
           }
           break;
 }
}

