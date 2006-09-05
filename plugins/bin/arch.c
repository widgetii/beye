/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/arch.c
 * @brief       This file contains implementation of Archive file format decoder.
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
#include <time.h>

#include "bmfile.h"
#include "bin_util.h"
#include "biewhelp.h"
#include "bconsole.h"
#include "biewutil.h"
#include "reg_form.h"
#include "tstrings.h"
#include "plugins/bin/arch.h"
#include "plugins/disasm.h"
#include "biewlib/pmalloc.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

ar_hdr arch;

static __filesize_t __FASTCALL__ ShowARCHHeader( void )
{
  __filesize_t fpos,ldat;
  unsigned evt;
  TWindow * w;
  struct tm * tm;
  char sout[50];
  fpos = BMGetCurrFilePos();
  w = CrtDlgWndnls(" This is COFF or a.out archive ",54,6);
  twGotoXY(1,1);
  strncpy(sout,(char *)arch.ar_name,16);
  sout[16] = 0;
  twPrintF("Name           = %s\n",sout);
  strncpy(sout,(char *)arch.ar_date,12);
  sout[12] = 0;
  ldat = atol(sout);
  tm = localtime((time_t *)&ldat);
  strftime(sout,sizeof(sout),"%X %x",tm);
  twPrintF("Date           = %s\n",sout);
  strncpy(sout,(char *)arch.ar_uid,6);
  sout[6] = 0;
  twPrintF("Owner UID      = %s\n",sout);
  strncpy(sout,(char *)arch.ar_gid,6);
  sout[6] = 0;
  twPrintF("Owner GID      = %s\n",sout);
  strncpy(sout,(char *)arch.ar_mode,8);
  sout[8] = 0;
  twPrintF("File mode      = %s\n",sout);
  strncpy(sout,(char *)arch.ar_size,10);
  sout[10] = 0;
  twPrintF("File size      = %s bytes",sout);
  do
  {
    evt = GetEvent(drawEmptyPrompt,NULL,w);
  }
  while(!(evt == KE_ESCAPE || evt == KE_F(10)));
  CloseWnd(w);
  return fpos;
}

static tBool __NEAR__ __FASTCALL__ archReadModList(memArray *obj,unsigned nnames,__filesize_t *addr)
{
  __filesize_t foff,flen;
  unsigned i;
  char stmp[80];
  flen = bmGetFLength();
  for(i = 0;i < nnames;i++)
  {
    tBool is_eof;
    /**
       Some archives sometimes have big and sometimes little endian.
       Here is a horrible attempt to determine it.
    */
    foff = addr[i];
    if(foff > flen)  foff = ByteSwapL(foff);
    if(IsKbdTerminate()) break;
    bmReadBufferEx(stmp,sizeof(ar_sub_hdr),foff,BM_SEEK_SET);
    is_eof = bmEOF();
    stmp[sizeof(ar_sub_hdr)-2] = 0;
    if(!ma_AddString(obj,is_eof ? CORRUPT_BIN_MSG : stmp,True)) break;
    if(is_eof) break;
  }
  return True;
}

static __filesize_t __FASTCALL__ archModLst( void )
{
   memArray *obj;
   __filesize_t *addr;
   unsigned long rnames,bnames;
   unsigned nnames;
   __filesize_t fpos,flen;
   fpos = BMGetCurrFilePos();
   flen = bmGetFLength();
   rnames = bmReadDWordEx(sizeof(ar_hdr),BM_SEEK_SET);
   bnames = ByteSwapL(rnames);
   /**
      Some archives sometimes have big and sometimes little endian.
      Here is a horrible attempt to determine it.
   */
   if(!(nnames = (unsigned)min(rnames,bnames))) { NotifyBox(NOT_ENTRY,"Archive modules list"); return fpos; }
   /**
      Some archives sometimes have length and sometimes number of entries
      Here is a horrible attempt to determine it.
   */
   if(!(nnames%4)) nnames/=sizeof(unsigned long);
   if(!(obj = ma_Build(nnames,True))) return fpos;
   if(!(addr = PMalloc(sizeof(unsigned long)*nnames))) goto exit;
   bmReadBufferEx(addr,sizeof(unsigned long)*nnames,sizeof(ar_hdr)+sizeof(unsigned long),BM_SEEK_SET);
   if(archReadModList(obj,nnames,addr))
   {
     int ret;
     ret = ma_Display(obj," Archive modules list ",LB_SELECTIVE,-1);
     if(ret != -1)
     {
       /**
          Some archives sometimes have big and sometimes little endian.
          Here is a horrible attempt to determine it.
       */
       fpos = addr[ret];
       if(fpos > flen) fpos = ByteSwapL(fpos);
       fpos += sizeof(ar_sub_hdr);
     }
   }
   free(addr);
   exit:
   ma_Destroy(obj);
   return fpos;
}

static tBool __FASTCALL__ IsArch( void )
{
  char str[16];
  bmReadBufferEx(str,sizeof(str),0,BM_SEEK_SET);
  return strncmp(str,"!<arch>\012",8) == 0;
}

static void __FASTCALL__ ArchInit( void )
{
  bmReadBufferEx(&arch,sizeof(arch),0,BM_SEEK_SET);
}

static void __FASTCALL__ ArchDestroy( void )
{
}

static tBool __FASTCALL__ archAddrResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  tBool bret = True;
  if(cfpos < sizeof(ar_hdr))
  {
    strcpy(addr,"arch.h:");
    strcpy(&addr[7],Get2Digit(cfpos));
  }
  else bret = False;
  return bret;
}

static __filesize_t __FASTCALL__ archHelp( void )
{
  hlpDisplay(10001);
  return BMGetCurrFilePos();
}

static int __FASTCALL__ arch_platform( void ) { return DISASM_DEFAULT; }

REGISTRY_BIN archTable =
{
  "arch (Archive)",
  { "ArcHlp", NULL, "ModLst", NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { archHelp, NULL, archModLst, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  IsArch, ArchInit, ArchDestroy,
  ShowARCHHeader,
  NULL,
  NULL,
  arch_platform,
  NULL,
  archAddrResolv,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
