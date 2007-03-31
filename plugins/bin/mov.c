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
#include "biewlib/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"

static tBool  __FASTCALL__ mov_check_fmt( void )
{
    return False;
}

static void __FASTCALL__ mov_init_fmt( void ) {}
static void __FASTCALL__ mov_destroy_fmt(void) {}
static int  __FASTCALL__ mov_platform( void) { return DISASM_DEFAULT; }

REGISTRY_BIN movTable =
{
  "MOV file format",
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  mov_check_fmt,
  mov_init_fmt,
  mov_destroy_fmt,
  NULL,
  NULL,
  NULL,
  mov_platform,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
