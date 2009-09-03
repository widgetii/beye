/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/rdoff2.c
 * @brief       This file contains implementation of decoder for RDOFF v2 file format.
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

#include "plugins/disasm.h"
#include "plugins/bin/rdoff2.h"
#include "reg_form.h"
#include "bin_util.h"
#include "bmfile.h"
#include "bconsole.h"
#include "biewhelp.h"
#include "biewlib/kbd_code.h"

static __filesize_t __FASTCALL__ rdoff2_ShowHeader( void )
{
  int endian;
  __filesize_t fpos;
  unsigned long hs_len,im_len;
  TWindow *w;
  fpos = BMGetCurrFilePos();
  endian = bmReadByteEx(5,BM_SEEK_SET);
  im_len = bmReadDWord();
  hs_len = bmReadDWord();
  w = CrtDlgWndnls(endian == 0x02 ? " RDOFFv2 big endian " : " RDOFFv2 little endian ",54,5);
  twGotoXY(1,1);
  twPrintF("Image length                = %08lXH\n"
           "Header length               = %08lXH\n"
           ,im_len
           ,hs_len);
  while(1)
  {
    int keycode;
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    switch(keycode)
    {
/*
      case KE_ENTER:      fpos = cs_start; goto exit;
      case KE_F(5):
      case KE_CTL_ENTER:  fpos = ds_start; goto exit;
*/
      case KE_F(10):
      case KE_ESCAPE:     goto exit;
      default:            break;
    };
  }
  exit:
  CloseWnd(w);
  return fpos;
}

static __filesize_t __FASTCALL__ rdoff2_Help( void )
{
  hlpDisplay(10012);
  return BMGetCurrFilePos();
}

static tBool __FASTCALL__ rdoff2_check_fmt( void )
{
  char rbuff[6];
  bmReadBufferEx(rbuff,sizeof(rbuff),0L,BM_SEEK_SET);
  return memcmp(rbuff,"RDOFF2",sizeof(rbuff)) == 0 ||
         memcmp(rbuff,"RDOFF\x2",sizeof(rbuff)) == 0;
}

static void __FASTCALL__ rdoff2_init_fmt( void ) {}
static void __FASTCALL__ rdoff2_destroy_fmt( void ) {}

static int __FASTCALL__ rdoff2_platform( void ) { return DISASM_CPU_IX86; }

REGISTRY_BIN rdoff2Table =
{
  "RDOFF v2 (Relocatable Dynamic Object File Format)",
  { "RdHelp", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { rdoff2_Help, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  rdoff2_check_fmt,
  rdoff2_init_fmt,
  rdoff2_destroy_fmt,
  rdoff2_ShowHeader,
  NULL,
  NULL,
  rdoff2_platform,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
