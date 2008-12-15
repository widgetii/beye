/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/jpeg.c
 * @brief       This file contains implementation of decoder for jpeg
 *              file format.
 * @version     -
 * @remark      this source file is part of jpegary vIEW project (BIEW).
 *              The jpegary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
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
#include "biewlib/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"

static tBool  __FASTCALL__ jpeg_check_fmt( void )
{
    unsigned long val;
    unsigned char id[4];
    val=bmReadDWordEx(0,BM_SEEK_SET);
    bmReadBufferEx(id,4,6,BM_SEEK_SET);
    if(val==0xE0FFD8FF && memcmp(id,"JFIF",4)==0) return True;
    return False;
}

static void __FASTCALL__ jpeg_init_fmt( void ) {}
static void __FASTCALL__ jpeg_destroy_fmt(void) {}
static int  __FASTCALL__ jpeg_platform( void) { return DISASM_DEFAULT; }

static __filesize_t __FASTCALL__ Show_JPEG_Header( void )
{
    ErrMessageBox("Not implemented yet!","JPEG format");
    return BMGetCurrFilePos();
}

REGISTRY_BIN jpegTable =
{
  "JPEG file format",
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  jpeg_check_fmt,
  jpeg_init_fmt,
  jpeg_destroy_fmt,
  Show_JPEG_Header,
  NULL,
  NULL,
  jpeg_platform,
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
