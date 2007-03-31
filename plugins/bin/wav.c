/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/wav.c
 * @brief       This file contains implementation of decoder for wav multimedia
 *              streams.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
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

static tBool  __FASTCALL__ wav_check_fmt( void )
{
    unsigned long id;
    id=bmReadDWordEx(8,SEEKF_START);
    if(	bmReadDWordEx(0,SEEKF_START)==mmioFOURCC('R','I','F','F') &&
	id==mmioFOURCC('W','A','V','E'))
	return True;
    return False;
}

static void __FASTCALL__ wav_init_fmt( void ) {}
static void __FASTCALL__ wav_destroy_fmt(void) {}
static int  __FASTCALL__ wav_platform( void) { return DISASM_DEFAULT; }

static __filesize_t __FASTCALL__ wav_find_chunk(__filesize_t off,unsigned long id)
{
    unsigned long ids,size,type,fpos;
    bmSeek(off,BM_SEEK_SET);
    while(!bmEOF())
    {
/*	fpos=bmGetCurrFilePos();*/
	ids=bmReadDWord();
	if(ids==id) return bmGetCurrFilePos();
	size=bmReadDWord();
	size=(size+1)&(~1);
/*fprintf(stderr,"%08X:%4s %08X\n",fpos,(char *)&ids,size);*/
	if(ids==mmioFOURCC('L','I','S','T'))
	{
	    type=bmReadDWord();
	    if(type==id) return bmGetCurrFilePos();
	    continue;
	}
	bmSeek(size,BM_SEEK_CUR);
    }
    return -1;
}

static __filesize_t __FASTCALL__ Show_WAV_Header( void )
{
 unsigned keycode;
 TWindow * hwnd;
 WAVEFORMATEX wavf;
 __filesize_t newcpos,fpos,fpos2;
 unsigned long FPageCnt;
 const char * addinfo;
 fpos = BMGetCurrFilePos();
 fpos2 = wav_find_chunk(12,mmioFOURCC('f','m','t',' '));
 if(fpos2==-1) { ErrMessageBox("Main WAV Header not found",NULL); return fpos; }
 bmSeek(fpos2,BM_SEEK_SET);
 bmReadDWord(); /* skip section size */
 bmReadBuffer(&wavf,sizeof(WAVEFORMATEX));
 fpos2 = wav_find_chunk(12,mmioFOURCC('d','a','t','a'));
 if(fpos2!=-1) fpos2-=4;
 hwnd = CrtDlgWndnls(" WAV File Header ",43,5);
 twUseWin(hwnd);
 twGotoXY(1,1);
 twPrintF("FormatTag            = 0x%04X\n"
          "ChannelsxSampleSecs  = %ux%u\n"
          "AvgBytesSecs         = %lu\n"
          "BlockAlign           = %u\n"
          "BitsPerSample        = %u\n"
	  ,wavf.wFormatTag
	  ,wavf.nChannels,wavf.nSamplesPerSec
	  ,wavf.nAvgBytesPerSec
	  ,wavf.nBlockAlign
	  ,wavf.wBitsPerSample);
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

REGISTRY_BIN wavTable =
{
  "RIFF WAVE format",
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  wav_check_fmt,
  wav_init_fmt,
  wav_destroy_fmt,
  Show_WAV_Header,
  NULL,
  NULL,
  wav_platform,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
