/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/avi.c
 * @brief       This file contains implementation of decoder for AVI multimedia
 *              streams.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
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

#include "bconsole.h"
#include "biewhelp.h"
#include "colorset.h"
#include "biewutil.h"
#include "reg_form.h"
#include "bmfile.h"
#include "biewlib/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"

typedef struct
{
    DWORD		dwMicroSecPerFrame;	// frame display rate (or 0L)
    DWORD		dwMaxBytesPerSec;	// max. transfer rate
    DWORD		dwPaddingGranularity;	// pad to multiples of this
                                                // size; normally 2K.
    DWORD		dwFlags;		// the ever-present flags
    DWORD		dwTotalFrames;		// # frames in file
    DWORD		dwInitialFrames;
    DWORD		dwStreams;
    DWORD		dwSuggestedBufferSize;
    
    DWORD		dwWidth;
    DWORD		dwHeight;
    
    DWORD		dwReserved[4];
} MainAVIHeader;

/*
** Stream header
*/

typedef struct {
    FOURCC		fccType;
    FOURCC		fccHandler;
    DWORD		dwFlags;	/* Contains AVITF_* flags */
    WORD		wPriority;
    WORD		wLanguage;
    DWORD		dwInitialFrames;
    DWORD		dwScale;	
    DWORD		dwRate;	/* dwRate / dwScale == samples/second */
    DWORD		dwStart;
    DWORD		dwLength; /* In units above... */
    DWORD		dwSuggestedBufferSize;
    DWORD		dwQuality;
    DWORD		dwSampleSize;
    RECT		rcFrame;
} AVIStreamHeader;

#define formtypeAVI             mmioFOURCC('A', 'V', 'I', ' ')
#define listtypeAVIHEADER       mmioFOURCC('h', 'd', 'r', 'l')
#define ckidAVIMAINHDR          mmioFOURCC('a', 'v', 'i', 'h')
#define listtypeSTREAMHEADER    mmioFOURCC('s', 't', 'r', 'l')
#define ckidSTREAMHEADER        mmioFOURCC('s', 't', 'r', 'h')
#define ckidSTREAMFORMAT        mmioFOURCC('s', 't', 'r', 'f')
#define ckidSTREAMHANDLERDATA   mmioFOURCC('s', 't', 'r', 'd')
#define ckidSTREAMNAME		mmioFOURCC('s', 't', 'r', 'n')

#define listtypeAVIMOVIE        mmioFOURCC('m', 'o', 'v', 'i')
#define listtypeAVIRECORD       mmioFOURCC('r', 'e', 'c', ' ')

#define ckidAVINEWINDEX         mmioFOURCC('i', 'd', 'x', '1')

/*
** Stream types for the <fccType> field of the stream header.
*/
#define streamtypeVIDEO         mmioFOURCC('v', 'i', 'd', 's')
#define streamtypeAUDIO         mmioFOURCC('a', 'u', 'd', 's')
#define streamtypeMIDI		mmioFOURCC('m', 'i', 'd', 's')
#define streamtypeTEXT          mmioFOURCC('t', 'x', 't', 's')

/* Basic chunk types */
#define cktypeDIBbits           aviTWOCC('d', 'b')
#define cktypeDIBcompressed     aviTWOCC('d', 'c')
#define cktypePALchange         aviTWOCC('p', 'c')
#define cktypeWAVEbytes         aviTWOCC('w', 'b')

/* Chunk id to use for extra chunks for padding. */
#define ckidAVIPADDING          mmioFOURCC('J', 'U', 'N', 'K')

static tBool  __FASTCALL__ avi_check_fmt( void )
{
    unsigned long id;
    id=bmReadDWordEx(8,SEEKF_START);
    if(	bmReadDWordEx(0,SEEKF_START)==mmioFOURCC('R','I','F','F') &&
	(id==mmioFOURCC('A','V','I',' ') || id==mmioFOURCC('O','N','2',' ')))
	return True;
    return False;
}

static void __FASTCALL__ avi_init_fmt( void ) {}
static void __FASTCALL__ avi_destroy_fmt(void) {}
static int  __FASTCALL__ avi_platform( void) { return DISASM_DEFAULT; }

static __filesize_t __FASTCALL__ avi_find_chunk(__filesize_t off,unsigned long id)
{
    unsigned long ids,size,type;
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

static __filesize_t __FASTCALL__ Show_AVI_Header( void )
{
 unsigned keycode;
 TWindow * hwnd;
 MainAVIHeader avih;
 __filesize_t fpos,fpos2;
 fpos = BMGetCurrFilePos();
 fpos2 = avi_find_chunk(12,mmioFOURCC('a','v','i','h'));
 if((__fileoff_t)fpos2==-1) { ErrMessageBox("Main AVI Header not found",NULL); return fpos; }
 bmSeek(fpos2,BM_SEEK_SET);
 bmReadDWord(); /* skip section size */
 bmReadBuffer(&avih,sizeof(MainAVIHeader));
 fpos2 = avi_find_chunk(12,mmioFOURCC('m','o','v','i'));
 if((__fileoff_t)fpos2!=-1) fpos2-=4;
 hwnd = CrtDlgWndnls(" AVI File Header ",43,9);
 twUseWin(hwnd);
 twGotoXY(1,1);
 twPrintF("MicroSecond per Frame= %lu\n"
          "Max bytes per second = %lu\n"
          "Padding granularity  = %lu\n"
          "Flags                = %lu\n"
          "Total frames         = %lu\n"
          "Initial frames       = %lu\n"
          "Number of streams    = %lu\n"
          "Suggested buffer size= %lu\n"
          "Width x Height       = %lux%lu\n"
          ,avih.dwMicroSecPerFrame
          ,avih.dwMaxBytesPerSec
          ,avih.dwPaddingGranularity
          ,avih.dwFlags
          ,avih.dwTotalFrames
          ,avih.dwInitialFrames
          ,avih.dwStreams
          ,avih.dwSuggestedBufferSize
          ,avih.dwWidth
          ,avih.dwHeight);
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

static __filesize_t __FASTCALL__ Show_A_Header( void )
{
 unsigned keycode;
 TWindow * hwnd;
 AVIStreamHeader strh;
 WAVEFORMATEX wavf;
 __filesize_t newcpos,fpos,fpos2;
 fpos = BMGetCurrFilePos();
 memset(&wavf,0,sizeof(wavf));
 fpos2=12;
 do
 {
    fpos2 = avi_find_chunk(fpos2,mmioFOURCC('s','t','r','h'));
    if((__fileoff_t)fpos2==-1) { ErrMessageBox("Audio Stream Header not found",NULL); return fpos; }
    bmSeek(fpos2,BM_SEEK_SET);
    newcpos=bmReadDWord();
    bmReadBuffer(&strh,sizeof(AVIStreamHeader));
    fpos2+=newcpos+4;
 }while(strh.fccType!=streamtypeAUDIO);
 if(bmReadDWord()==mmioFOURCC('s','t','r','f'))
 {
    bmReadDWord(); /* skip header size */
    bmReadBuffer(&wavf,sizeof(WAVEFORMATEX));
 }
 hwnd = CrtDlgWndnls(" Stream File Header ",43,18);
 twUseWin(hwnd);
 twGotoXY(1,1);
 twPrintF("Stream type          = %c%c%c%c\n"
          "FOURCC handler       = %c%c%c%c(%08Xh)\n"
          "Flags                = %lu\n"
          "Priority             = %u\n"
          "Language             = %u\n"
          "Initial frames       = %lu\n"
          "Rate/Scale           = %lu/%lu=%5.3f\n"
          "Start                = %lu\n"
          "Length               = %lu\n"
          "Suggested buffer size= %lu\n"
          "Quality              = %lu\n"
          "SampleSize           = %lu\n"
          "====== WAVE HEADER ====================\n"
          "FormatTag            = 0x%04X (%s)\n"
          "ChannelsxSampleSecs  = %ux%u\n"
          "AvgBytesSecs         = %lu\n"
          "BlockAlign           = %u\n"
          "BitsPerSample        = %u\n"
          INT_2_CHAR_ARG(strh.fccType)
          INT_2_CHAR_ARG(strh.fccHandler),strh.fccHandler
	 ,strh.dwFlags
	 ,strh.wPriority
	 ,strh.wLanguage
	 ,strh.dwInitialFrames
	 ,strh.dwRate
	 ,strh.dwScale
	 ,(float)strh.dwRate/(float)strh.dwScale
	 ,strh.dwStart
	 ,strh.dwLength
	 ,strh.dwSuggestedBufferSize
	 ,strh.dwQuality
	 ,strh.dwSampleSize
	 ,wavf.wFormatTag,wtag_find_name(wavf.wFormatTag)
	 ,wavf.nChannels,wavf.nSamplesPerSec
	 ,wavf.nAvgBytesPerSec
	 ,wavf.nBlockAlign
	 ,wavf.wBitsPerSample);
 while(1)
 {
   keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
   if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
 }
 CloseWnd(hwnd);
 return fpos;
}

static __filesize_t __FASTCALL__ Show_V_Header( void )
{
 unsigned keycode;
 TWindow * hwnd;
 AVIStreamHeader strh;
 BITMAPINFOHEADER bmph;
 __filesize_t newcpos,fpos,fpos2;
 fpos = BMGetCurrFilePos();
 memset(&bmph,0,sizeof(BITMAPINFOHEADER));
 fpos2=12;
 do
 {
    fpos2 = avi_find_chunk(fpos2,mmioFOURCC('s','t','r','h'));
    if((__fileoff_t)fpos2==-1) { ErrMessageBox("Video Stream Header not found",NULL); return fpos; }
    bmSeek(fpos2,BM_SEEK_SET);
    newcpos=bmReadDWord(); /* skip section size */
    bmReadBuffer(&strh,sizeof(AVIStreamHeader));
    fpos2+=newcpos+4;
 }while(strh.fccType!=streamtypeVIDEO);
 if(bmReadDWord()==mmioFOURCC('s','t','r','f'))
 {
    bmReadDWord(); /* skip header size */
    bmReadBuffer(&bmph,sizeof(BITMAPINFOHEADER));
 }
 hwnd = CrtDlgWndnls(" Stream File Header ",43,20);
 twUseWin(hwnd);
 twGotoXY(1,1);
 twPrintF("Stream type          = %c%c%c%c\n"
          "FOURCC handler       = %c%c%c%c(%08Xh)\n"
          "Flags                = %lu\n"
          "Priority             = %u\n"
          "Language             = %u\n"
          "Initial frames       = %lu\n"
          "Rate/Scale           = %lu/%lu=%5.3f\n"
          "Start                = %lu\n"
          "Length               = %lu\n"
          "Suggested buffer size= %lu\n"
          "Quality              = %lu\n"
          "SampleSize           = %lu\n"
          "FrameRect            = %u.%ux%u.%u\n"
          "===== BITMAP INFO HEADER ===============\n"
          "WxH                  = %lux%lu\n"
          "PlanesxBitCount      = %ux%u\n"
          "Compression          = %c%c%c%c\n"
          "ImageSize            = %lu\n"
          "XxYPelsPerMeter      = %lux%lu\n"
          "ColorUsedxImportant  = %lux%lu\n"
          INT_2_CHAR_ARG(strh.fccType)
          INT_2_CHAR_ARG(strh.fccHandler),strh.fccHandler
	 ,strh.dwFlags
	 ,strh.wPriority
	 ,strh.wLanguage
	 ,strh.dwInitialFrames
	 ,strh.dwRate
	 ,strh.dwScale
	 ,(float)strh.dwRate/(float)strh.dwScale
	 ,strh.dwStart
	 ,strh.dwLength
	 ,strh.dwSuggestedBufferSize
	 ,strh.dwQuality
	 ,strh.dwSampleSize
	 ,strh.rcFrame.left,strh.rcFrame.top,strh.rcFrame.right,strh.rcFrame.bottom
	 ,bmph.biWidth,bmph.biHeight
	 ,bmph.biPlanes,bmph.biBitCount
	 INT_2_CHAR_ARG(bmph.biCompression)
	 ,bmph.biSizeImage
	 ,bmph.biXPelsPerMeter,bmph.biYPelsPerMeter
	 ,bmph.biClrUsed,bmph.biClrImportant);
 while(1)
 {
   keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
   if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
 }
 CloseWnd(hwnd);
 return fpos;
}

REGISTRY_BIN aviTable =
{
  "Audio Video Interleaved format",
  { NULL, "Audio", "Video", NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, Show_A_Header, Show_V_Header, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  avi_check_fmt,
  avi_init_fmt,
  avi_destroy_fmt,
  Show_AVI_Header,
  NULL,
  NULL,
  avi_platform,
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
