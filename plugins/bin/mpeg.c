/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/mpeg.c
 * @brief       This file contains implementation of decoder for MPEG-PES
 *              file format.
 * @version     -
 * @remark      this source file is part of mpegary vIEW project (BIEW).
 *              The mpegary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
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

/*
 headers:
 0x00000100		- picture_start_code
 0x00000101-0x000001AF	- slice_start_code
 0x000001B0-0x000001B1	- reserved
 0x000001B2		- user_data_start_code
 0x000001B3		- sequence_header_code
 0x000001B4		- sequence_error_code
 0x000001B5		- extension_start_code
 0x000001B6		- reserved
 0x000001B7		- sequence_end_code
 0x000001B8		- group_start_code
 0x000001B9-0x000001FF	- system_start_code
  0x000001B9		- ISO_111172_end_code
  0x000001BA		- pack_start_code
  0x000001BB		- system_header_start_code
  0x000001BC		- program_stream_map
  0x000001BD		- private_stream_1
  0x000001BE		- padding_stream
  0x000001BF		- private_stream_2
  0x000001C0-0x000001DF - audio_stream_prefixes
  0x000001E0-0x000001EF - video_stream_prefixes
  0x000001F0		- ECM_stream
  0x000001F1		- EMM_stream
  0x000001F2		- DSM_CC_stream
  0x000001F3		- ISO_13522_stream
  0x000001FF		- prog_stream_dir
*/

static tBool  __FASTCALL__ mpeg_check_fmt( void )
{
    return False;
}

static void __FASTCALL__ mpeg_init_fmt( void ) {}
static void __FASTCALL__ mpeg_destroy_fmt(void) {}
static int  __FASTCALL__ mpeg_platform( void) { return DISASM_DEFAULT; }

REGISTRY_BIN mpegTable =
{
  "MPEG-PES file format",
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  mpeg_check_fmt,
  mpeg_init_fmt,
  mpeg_destroy_fmt,
  NULL,
  NULL,
  NULL,
  mpeg_platform,
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
