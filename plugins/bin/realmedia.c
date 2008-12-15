/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/realmdeia.c
 * @brief       This file contains implementation of decoder for jpeg
 *              file format.
 * @version     -
 * @remark      this source file is part of rmary vIEW project (BIEW).
 *              The rmary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
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

#define MKTAG(a, b, c, d) (a | (b << 8) | (c << 16) | (d << 24))

static tBool  __FASTCALL__ rm_check_fmt( void )
{
    if(bmReadDWordEx(0,BM_SEEK_SET)==MKTAG('.', 'R', 'M', 'F')) return True;
    return False;
}

static __filesize_t __FASTCALL__ Show_RM_Header( void )
{
    ErrMessageBox("Not implemented yet!","RM format");
    return BMGetCurrFilePos();
}

static void __FASTCALL__ rm_init_fmt( void ) {}
static void __FASTCALL__ rm_destroy_fmt(void) {}
static int  __FASTCALL__ rm_platform( void) { return DISASM_DEFAULT; }

REGISTRY_BIN rmTable =
{
  "Real Media file format",
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  rm_check_fmt,
  rm_init_fmt,
  rm_destroy_fmt,
  Show_RM_Header,
  NULL,
  NULL,
  rm_platform,
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
