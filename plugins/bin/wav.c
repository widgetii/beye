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

wTagNames wtagNames[] =
{
 { 0x0000, "MS-Unknown" },
 { 0x0001, "Raw PCM" },
 { 0x0002, "ADPCM" },
 { 0x0003, "IEEE-Float" },
 { 0x0004, "VSELP" },
 { 0x0005, "IBM-CVSD" },
 { 0x0006, "ALAW" },
 { 0x0007, "MuLAW" },
 { 0x0008, "DTS" },
 { 0x0010, "OKI-ADPCM" },
 { 0x0011, "DVI-ADPCM" },
 { 0x0012, "MediaSpace-ADPCM" },
 { 0x0013, "Sierra-ADPCM" },
 { 0x0014, "Antex G723-ADPCM" },
 { 0x0015, "DigiSTD" },
 { 0x0016, "DigiFix" },
 { 0x0017, "Dialogic-OKI-ADPCM" },
 { 0x0018, "MediaVision-ADPCM" },
 { 0x0019, "CU-Codec" },
 { 0x0020, "YAMAHA-ADPCM" },
 { 0x0021, "SONARC" },
 { 0x0022, "DSPGroup-TRUESPEECH" },
 { 0x0023, "ECHOSC1" },
 { 0x0024, "AudioFile-AF36" },
 { 0x0025, "APTX" },
 { 0x0026, "AudioFile-AF10" },
 { 0x0027, "PROSODY-1612" },
 { 0x0028, "LRC" },
 { 0x0030, "Dolby-AC2" },
 { 0x0031, "GSM610" },
 { 0x0032, "MSNAudio" },
 { 0x0033, "ANTEX-ADPCME" },
 { 0x0034, "CONTROL-RES-VQLPC" },
 { 0x0035, "DigiReal" },
 { 0x0036, "DigiADPCM" },
 { 0x0037, "CONTROL-RES-CR10" },
 { 0x0038, "NMS-VBXADPCM" },
 { 0x0039, "CS-IMAADPCM" },
 { 0x003A, "ECHOSC3" },
 { 0x003B, "ROCKWELL-ADPCM" },
 { 0x003C, "ROCKWELL-DIGITALK" },
 { 0x003D, "XEBEC" },
 { 0x0040, "G721-ADPCM" },
 { 0x0041, "G728-CELP" },
 { 0x0042, "MSG723" },
 { 0x0050, "MP2" },
 { 0x0052, "RT24" },
 { 0x0053, "PAC" },
 { 0x0055, "MP3" },
 { 0x0059, "Lucent-G723" },
 { 0x0060, "Cirrus" },
 { 0x0061, "ESPCM" },
 { 0x0062, "VoxWare" },
 { 0x0063, "Canopus-ATRAC" },
 { 0x0064, "G726-ADPCM" },
 { 0x0065, "G722-ADPCM" },
 { 0x0067, "DSAT-Display" },
 { 0x0069, "VoxWare-ByteAligned" },
 { 0x0070, "VoxWare-AC8" },
 { 0x0071, "VoxWare-AC10" },
 { 0x0072, "VoxWare-AC16" },
 { 0x0073, "VoxWare-AC20" },
 { 0x0074, "VoxWare-RT24" },
 { 0x0075, "VoxWare-RT29" },
 { 0x0076, "VoxWare-RT29HW" },
 { 0x0077, "VoxWare-VR12" },
 { 0x0078, "VoxWare-VR18" },
 { 0x0079, "VoxWare-TQ40" },
 { 0x0080, "SoftSound" },
 { 0x0081, "VoxWare-TQ60" },
 { 0x0082, "MSRT24" },
 { 0x0083, "G729A" },
 { 0x0084, "MVI-MVI2" },
 { 0x0085, "DF-G726" },
 { 0x0086, "DF-GSM610" },
 { 0x0088, "ISIAudio" },
 { 0x0089, "ONLive" },
 { 0x0091, "SBC24" },
 { 0x0092, "DOLBY-AC3-SPDIF" },
 { 0x0093, "MediaSonic-G723" },
 { 0x0094, "PROSODY-8KBPS" },
 { 0x0097, "ZyXEL-ADPCM" },
 { 0x0098, "Philips-LPCBB" },
 { 0x0099, "AG-PACKED" },
 { 0x00A0, "MALDEN-PhonyTalk" },
 { 0x0100, "Rhetorex-ADPCM" },
 { 0x0101, "IRAT" },
 { 0x0111, "VIVO-G723" },
 { 0x0112, "VIVO-SIREN" },
 { 0x0123, "Digital-G723" },
 { 0x0125, "Sanyo-LD-ADPCM" },
 { 0x0130, "SiproLab-ACEPLNET" },
 { 0x0131, "SiproLab-ACELP4800" },
 { 0x0132, "SiproLab-ACELP8V3" },
 { 0x0133, "SiproLab-G729" },
 { 0x0134, "SiproLab-G729A" },
 { 0x0135, "SiproLab-KELVIN" },
 { 0x0140, "G726ADPCM" },
 { 0x0150, "QualComm-PUREVOICE" },
 { 0x0151, "QualComm-HALFRATE" },
 { 0x0155, "TUBGSM" },
 { 0x0160, "MSAUDIO1" },
 { 0x0161, "MSAUDIO2" },
 { 0x0162, "MSAUDIO3" },
 { 0x0200, "Creative-ADPCM" },
 { 0x0202, "Creative-FASTSPEECH8" },
 { 0x0203, "Creative-FASTSPEECH10" },
 { 0x0210, "UHER-ADPCM" },
 { 0x0220, "Quarterdeck" },
 { 0x0230, "I-Link-VC" },
 { 0x0240, "Aureal RAW-Sport" },
 { 0x0250, "IPI-HSX" },
 { 0x0251, "IPI-RPELP" },
 { 0x0260, "CS2" },
 { 0x0270, "Sony-SCX" },
 { 0x0300, "FM-Towns-SND" },
 { 0x0400, "BTV-DIGITAL" },
 { 0x0450, "QDesign-MUSIC" },
 { 0x0680, "VME-VMPCM" },
 { 0x0681, "TPC" },
 { 0x1000, "Olivetti-GSM" },
 { 0x1001, "Olivetti-ADPCM" },
 { 0x1002, "Olivetti-CELP" },
 { 0x1003, "Olivetti-SBC" },
 { 0x1004, "Olivetti-OPR" },
 { 0x1100, "Lernout & Hauspie Codec" },
 { 0x1400, "Norris" },
 { 0x1500, "SoundSpace-MusiCompress" },
 { 0x2000, "DVM" },
 { 0xFFFE, "Microsoft Extensible" },
 { 0xFFFF, "Development" },
};

const char *wtag_find_name(unsigned short wtag)
{
    unsigned i;
    for(i=0;i<sizeof(wtagNames)/sizeof(wTagNames);i++)
    {
	if(wtagNames[i].wTag==wtag) return wtagNames[i].name;
    }
    return "Unknown";
}

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
 twPrintF("FormatTag            = 0x%04X (%s)\n"
          "ChannelsxSampleSecs  = %ux%u\n"
          "AvgBytesSecs         = %lu\n"
          "BlockAlign           = %u\n"
          "BitsPerSample        = %u\n"
	  ,wavf.wFormatTag,wtag_find_name(wavf.wFormatTag)
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
