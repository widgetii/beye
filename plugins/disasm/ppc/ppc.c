/**
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/ppc.c
 * @brief       This file contains implementation of PowerPC disassembler.
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
#include "bconsole.h"
#include "biewhelp.h"
#include "biewutil.h"
#include "plugins/disasm/ppc/ppc.h"
#include "biewlib/kbd_code.h"
#include "biewlib/file_ini.h"
#include "biewlib/pmalloc.h"

static char *outstr;

static int ppcBitness=DAB_USE32;
static int ppcBigEndian=1;

static DisasmRet __FASTCALL__ ppcDisassembler(__filesize_t ulShift,
                                              MBuffer buffer,
                                              unsigned flags)
{
  DisasmRet ret;
  if(!((flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    memset(&ret,0,sizeof(ret));
    ret.str = outstr;
    ret.codelen = 4;
    strcpy(outstr,"dd");
    disAppendDigits(outstr,ulShift,APREF_USE_TYPE,4,buffer,DISARG_DWORD);
  }
  else
  {
    if(flags & __DISF_GETTYPE) ret.pro_clone = __INSNT_ORDINAL;
    else ret.codelen = 4;
  }
  return ret;
}

static tBool __FASTCALL__ ppcAsmRef( void )
{
  hlpDisplay(20050);
  return False;
}

static void __FASTCALL__ ppcHelpAsm( void )
{
 char *msgAsmText,*title;
 char **strs;
 unsigned size,i,evt;
 unsigned long nstrs;
 TWindow * hwnd;
 if(!hlpOpen(True)) return;
 size = (unsigned)hlpGetItemSize(20051);
 if(!size) goto ppchlp_bye;
 msgAsmText = PMalloc(size+1);
 if(!msgAsmText)
 {
   mem_off:
   MemOutBox(" Help Display ");
   goto ppchlp_bye;
 }
 if(!hlpLoadItem(20051,msgAsmText))
 {
   PFree(msgAsmText);
   goto ppchlp_bye;
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
     twSetColorAttr(disasm_cset.cpu_cset[0].clone[i]);
     twPutS("PPC CPU");
     twClrEOL();
   }
   twGotoXY(2,4);
   {
     twSetColorAttr(disasm_cset.cpu_cset[1].clone[i]);
     twPutS("PPC FPU");
     twClrEOL();
   }
   twGotoXY(2,5);
   {
     twSetColorAttr(disasm_cset.cpu_cset[2].clone[i]);
     twPutS("AltiVec");
     twClrEOL();
   }
 }
 do
 {
   evt = GetEvent(drawEmptyPrompt,NULL,hwnd);
 }
 while(!(evt == KE_ESCAPE || evt == KE_F(10)));
 CloseWnd(hwnd);
 ppchlp_bye:
 hlpClose();
}

static int    __FASTCALL__ ppcMaxInsnLen( void ) { return 8; }
static ColorAttr __FASTCALL__ ppcGetAsmColor( unsigned long clone )
{
  if((clone & PPC_FPU)==PPC_ALTIVEC) return disasm_cset.cpu_cset[2].clone[0];
  else
  if((clone & PPC_FPU)==PPC_FPU) return disasm_cset.cpu_cset[1].clone[0];
  else
	return disasm_cset.cpu_cset[0].clone[0];
}

static int       __FASTCALL__ ppcGetBitness( void ) { return ppcBitness; }
static char      __FASTCALL__ ppcGetClone( unsigned long clone )
{
  UNUSED(clone);
  return ' ';
}
static void      __FASTCALL__ ppcInit( void )
{
  outstr = PMalloc(1000);
  if(!outstr)
  {
    MemOutBox("Data disassembler initialization");
    exit(EXIT_FAILURE);
  }
}

static void  __FASTCALL__ ppcTerm( void )
{
   PFREE(outstr);
}

static void __FASTCALL__ ppcReadIni( hIniProfile *ini )
{
  char tmps[10];
  if(isValidIniArgs())
  {
    biewReadProfileString(ini,"Biew","Browser","SubSubMode3","1",tmps,sizeof(tmps));
    ppcBitness = (int)strtoul(tmps,NULL,10);
    if(ppcBitness > 1 && ppcBitness != DAB_AUTO) ppcBitness = 0;
    biewReadProfileString(ini,"Biew","Browser","SubSubMode4","1",tmps,sizeof(tmps));
    ppcBigEndian = (int)strtoul(tmps,NULL,10);
    if(ppcBigEndian > 1) ppcBigEndian = 0;
  }
}

static void __FASTCALL__ ppcWriteIni( hIniProfile *ini )
{
  char tmps[10];
  sprintf(tmps,"%i",ppcBitness);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode3",tmps);
  sprintf(tmps,"%i",ppcBigEndian);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode4",tmps);
}

REGISTRY_DISASM PPC_Disasm =
{
  "PowerPC G5",
  { "PpcHlp", NULL, NULL, NULL, NULL },
  { ppcAsmRef, NULL, NULL, NULL, NULL },
  ppcDisassembler,
  NULL,
  ppcHelpAsm,
  ppcMaxInsnLen,
  ppcGetAsmColor,
  NULL,
  ppcGetAsmColor,
  NULL,
  ppcGetBitness,
  ppcGetClone,
  ppcInit,
  ppcTerm,
  ppcReadIni,
  ppcWriteIni
};




