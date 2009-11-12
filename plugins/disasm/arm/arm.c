/**
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/arm.c
 * @brief       This file contains implementation of ARM disassembler.
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
#include "plugins/disasm/arm/arm.h"
#include "biewlib/kbd_code.h"
#include "biewlib/file_ini.h"
#include "biewlib/pmalloc.h"

static char *outstr;

static int armBitness=DAB_USE32;
static int armBigEndian=1;

static DisasmRet __FASTCALL__ armDisassembler(__filesize_t ulShift,
                                              MBuffer buffer,
                                              unsigned flags)
{
  DisasmRet ret;
  if(detectedFormat->query_endian) armBigEndian = detectedFormat->query_endian(ulShift)==DAE_BIG?1:0;
  if(flags == __DISF_NORMAL)
  {
    memset(&ret,0,sizeof(ret));
    ret.str = outstr;
    ret.codelen = armBitness==DAB_USE32?4:2;
    if(armBitness==DAB_USE32)
    {
	tUInt32 opcode32;
	opcode32=armBigEndian?be2me_32(*((tUInt32 *)buffer)):le2me_32(*((tUInt32 *)buffer));
	arm32Disassembler(&ret,ulShift,opcode32,flags);
    }
    else
    {
	tUInt16 opcode16;
	opcode16=armBigEndian?be2me_16(*((tUInt16 *)buffer)):le2me_16(*((tUInt16 *)buffer));
	arm16Disassembler(&ret,ulShift,opcode16,flags);
    }
  }
  else
  {
    if(flags & __DISF_GETTYPE) ret.pro_clone = __INSNT_ORDINAL;
    else ret.codelen = armBitness==DAB_USE32?4:2;
  }
  return ret;
}

static tBool __FASTCALL__ armAsmRef( void )
{
  hlpDisplay(20040);
  return False;
}

static void __FASTCALL__ armHelpAsm( void )
{
 char *msgAsmText,*title;
 char **strs;
 unsigned size,i,evt;
 unsigned long nstrs;
 TWindow * hwnd;
 if(!hlpOpen(True)) return;
 size = (unsigned)hlpGetItemSize(20041);
 if(!size) goto armhlp_bye;
 msgAsmText = PMalloc(size+1);
 if(!msgAsmText)
 {
   mem_off:
   MemOutBox(" Help Display ");
   goto armhlp_bye;
 }
 if(!hlpLoadItem(20041,msgAsmText))
 {
   PFree(msgAsmText);
   goto armhlp_bye;
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
     twSetColorAttr(disasm_cset.engine[0].engine);
     twPutS("ARM CPU");
     twClrEOL();
   }
   twGotoXY(2,4);
   {
     twSetColorAttr(disasm_cset.engine[1].engine);
     twPutS("VFP extension");
     twClrEOL();
   }
   twGotoXY(2,5);
   {
     twSetColorAttr(disasm_cset.engine[2].engine);
     twPutS("XScale extensions");
     twClrEOL();
   }
 }
 do
 {
   evt = GetEvent(drawEmptyPrompt,NULL,hwnd);
 }
 while(!(evt == KE_ESCAPE || evt == KE_F(10)));
 CloseWnd(hwnd);
 armhlp_bye:
 hlpClose();
}

static int    __FASTCALL__ armMaxInsnLen( void ) { return 8; }
static ColorAttr __FASTCALL__ armGetAsmColor( unsigned long clone )
{
  if((clone & ARM_XSCALE)==ARM_XSCALE) return disasm_cset.engine[2].engine;
  else
  if((clone & ARM_FPU)==ARM_FPU) return disasm_cset.engine[1].engine;
  else
	return disasm_cset.engine[0].engine;
}

static int       __FASTCALL__ armGetBitness( void ) { return armBitness; }
static char      __FASTCALL__ armGetClone( unsigned long clone )
{
  UNUSED(clone);
  return ' ';
}
static void      __FASTCALL__ armInit( void )
{
  outstr = PMalloc(1000);
  if(!outstr)
  {
    MemOutBox("Data disassembler initialization");
    exit(EXIT_FAILURE);
  }
  arm16Init();
  arm32Init();
}

static void  __FASTCALL__ armTerm( void )
{
   arm32Term();
   arm16Term();
   PFREE(outstr);
}

static void __FASTCALL__ armReadIni( hIniProfile *ini )
{
  char tmps[10];
  if(isValidIniArgs())
  {
    biewReadProfileString(ini,"Biew","Browser","SubSubMode3","1",tmps,sizeof(tmps));
    armBitness = (int)strtoul(tmps,NULL,10);
    if(armBitness > 1 && armBitness != DAB_AUTO) armBitness = 0;
    biewReadProfileString(ini,"Biew","Browser","SubSubMode4","1",tmps,sizeof(tmps));
    armBigEndian = (int)strtoul(tmps,NULL,10);
    if(armBigEndian > 1) armBigEndian = 0;
  }
}

static void __FASTCALL__ armWriteIni( hIniProfile *ini )
{
  char tmps[10];
  sprintf(tmps,"%i",armBitness);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode3",tmps);
  sprintf(tmps,"%i",armBigEndian);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode4",tmps);
}

static const char *arm_bitness_names[] =
{
   "~Thumb-16",
   "~Full-32"
};

static tBool __FASTCALL__ armSelect_bitness( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(arm_bitness_names)/sizeof(char *);
  i = SelBoxA(arm_bitness_names,nModes," Select bitness mode: ",armBitness);
  if(i != -1)
  {
    armBitness = ((i==0)?DAB_USE16:DAB_USE32);
    return True;
  }
  return False;
}

static const char *arm_endian_names[] =
{
   "~Little endian",
   "~Big endian"
};

static tBool __FASTCALL__ armSelect_endian( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(arm_endian_names)/sizeof(char *);
  i = SelBoxA(arm_endian_names,nModes," Select endian mode: ",armBigEndian);
  if(i != -1)
  {
    armBigEndian = i;
    return True;
  }
  return False;
}

REGISTRY_DISASM ARM_Disasm =
{
  DISASM_CPU_ARM,
  "A~RMv5TE/XScale",
  { "ARMHlp", "Bitnes", "Endian", NULL },
  { armAsmRef, armSelect_bitness, armSelect_endian, NULL },
  armDisassembler,
  NULL,
  armHelpAsm,
  armMaxInsnLen,
  armGetAsmColor,
  NULL,
  armGetAsmColor,
  NULL,
  armGetBitness,
  armGetClone,
  armInit,
  armTerm,
  armReadIni,
  armWriteIni
};




