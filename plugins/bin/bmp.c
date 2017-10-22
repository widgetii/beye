/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/bmp.c
 * @brief       This file contains implementation of decoder for BMP
 *              file format.
 * @version     -
 * @remark      this source file is part of bmpary vIEW project (BIEW).
 *              The bmpary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
**/
#include <stddef.h>
#include <stdint.h>

#include "bconsole.h"
#include "beyehelp.h"
#include "colorset.h"
#include "beyeutil.h"
#include "reg_form.h"
#include "bmfile.h"
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"

static tBool  __FASTCALL__ bmp_check_fmt( void )
{
    if(	bmReadByteEx(0,BM_SEEK_SET) == 'B' &&
	bmReadByteEx(1,BM_SEEK_SET) == 'M') return True;
    return False;
}
static void __FASTCALL__ bmp_init_fmt( void ) {}
static void __FASTCALL__ bmp_destroy_fmt(void) {}
static int  __FASTCALL__ bmp_platform( void) { return DISASM_DEFAULT; }

static __filesize_t __FASTCALL__ Show_BMP_Header( void )
{
 unsigned keycode;
 TWindow * hwnd;
 BITMAPINFOHEADER bmph;
 __filesize_t fpos,fpos2,filesize;
 fpos = BMGetCurrFilePos();
 bmSeek(2,BM_SEEK_SET);
 filesize = bmReadDWord();
 bmSeek(4,BM_SEEK_CUR);
 fpos2=bmReadWord(); /* data offset */
 bmSeek(2,BM_SEEK_CUR);
 bmReadBuffer(&bmph,sizeof(BITMAPINFOHEADER));
 hwnd = CrtDlgWndnls(" BMP File Header ",43,6);
 twUseWin(hwnd);
 twGotoXY(1,1);
 twPrintF("WxH                  = %lux%lu\n"
          "PlanesxBitCount      = %ux%u\n"
          "Compression          = %c%c%c%c\n"
          "ImageSize            = %lu\n"
          "XxYPelsPerMeter      = %lux%lu\n"
          "ColorUsedxImportant  = %lux%lu\n"
	  ,bmph.biWidth,bmph.biHeight
	  ,bmph.biPlanes,bmph.biBitCount
	  INT_2_CHAR_ARG(bmph.biCompression)
	  ,bmph.biSizeImage
	  ,bmph.biXPelsPerMeter,bmph.biYPelsPerMeter
	  ,bmph.biClrUsed,bmph.biClrImportant);
 while(1)
 {
   keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
   if(keycode == KE_F(5) || keycode == KE_ENTER) { fpos = fpos2; break; }
   else
     if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
 }
 CloseWnd(hwnd);
 return fpos;
}

REGISTRY_BIN bmpTable =
{
  "BitMaP file format",
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  bmp_check_fmt,
  bmp_init_fmt,
  bmp_destroy_fmt,
  Show_BMP_Header,
  NULL,
  NULL,
  bmp_platform,
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
