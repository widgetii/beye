/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/mov.c
 * @brief       This file contains implementation of decoder for MOV
 *              file format.
 * @version     -
 * @remark      this source file is part of movary vIEW project (BIEW).
 *              The movary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1995
 * @note        Development, fixes and improvements
**/
#include <stddef.h>

#include "bconsole.h"
#include "biewhelp.h"
#include "colorset.h"
#include "biewutil.h"
#include "reg_form.h"
#include "bmfile.h"
#include "bswap.h"
#include "biewlib/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"

#define MOV_FOURCC(a,b,c,d) ((a<<24)|(b<<16)|(c<<8)|(d))

static __filesize_t __FASTCALL__ mov_find_chunk(__filesize_t off,unsigned long id)
{
    unsigned long ids,size,type,fpos;
    bmSeek(off,BM_SEEK_SET);
    while(!bmEOF())
    {
	size=be2me_32(bmReadDWord());
	if(size < 8) return -1;
	ids=be2me_32(bmReadDWord());
	if(ids==id) return bmGetCurrFilePos()-8;
	bmSeek(size-8,BM_SEEK_CUR);
    }
    return -1;
}


static tBool  __FASTCALL__ mov_check_fmt( void )
{
    __filesize_t moov,mdat;
    moov=mov_find_chunk(0,MOV_FOURCC('m','o','o','v'));
    mdat=mov_find_chunk(0,MOV_FOURCC('m','d','a','t'));
    if(moov != -1 && mdat != -1) return True;
    return False;
}

static void __FASTCALL__ mov_init_fmt( void ) {}
static void __FASTCALL__ mov_destroy_fmt(void) {}
static int  __FASTCALL__ mov_platform( void) { return DISASM_DEFAULT; }
static int  __FASTCALL__ mov_endian(__filesize_t off) { return DAE_LITTLE; }

static __filesize_t __FASTCALL__ Show_MOV_Header( void )
{
    ErrMessageBox("Not implemented yet!","MOV format");
    return BMGetCurrFilePos();
}


REGISTRY_BIN movTable =
{
  "MOV file format",
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  mov_check_fmt,
  mov_init_fmt,
  mov_destroy_fmt,
  Show_MOV_Header,
  NULL,
  NULL,
  mov_platform,
  NULL,
  mov_endian,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
