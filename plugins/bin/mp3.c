/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/mp3.c
 * @brief       This file contains implementation of decoder for MP3
 *              file format.
 * @version     -
 * @remark      this source file is part of mp3ary vIEW project (BIEW).
 *              The mp3ary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
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

#include "bswap.h"
#include "bconsole.h"
#include "biewhelp.h"
#include "colorset.h"
#include "biewutil.h"
#include "reg_form.h"
#include "bmfile.h"
#include "biewlib/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"

//----------------------- mp3 audio frame header parser -----------------------

static int tabsel_123[2][3][16] = {
   { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
     {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
     {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

   { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};
static long freqs[9] = { 44100, 48000, 32000, 22050, 24000, 16000 , 11025 , 12000 , 8000 };

int mp_mp3_get_lsf(unsigned char* hbuf){
    unsigned long newhead = 
      hbuf[0] << 24 |
      hbuf[1] << 16 |
      hbuf[2] <<  8 |
      hbuf[3];
    if( newhead & ((long)1<<20) ) {
      return (newhead & ((long)1<<19)) ? 0x0 : 0x1;
    }
    return 1;
}

/*
 * return frame size or -1 (bad frame)
 */
#define MAXFRAMESIZE 1280
int mp_decode_mp3_header(unsigned char* hbuf,int *fmt,int *brate,int *samplerate,int *channels){
    int nch,ssize,crc,lsf,mpeg25,framesize,padding,bitrate_index,sampling_frequency,mp3_fmt;
    unsigned long newhead = 
      hbuf[0] << 24 |
      hbuf[1] << 16 |
      hbuf[2] <<  8 |
      hbuf[3];

    if( (newhead & 0xffe00000) != 0xffe00000 ||  
        (newhead & 0x0000fc00) == 0x0000fc00){
	return -1;
    }

    if(((newhead>>17)&3)==0) return -1;

    mp3_fmt = 4-((newhead>>17)&3);
    if(fmt) *fmt = mp3_fmt;

    if( newhead & ((long)1<<20) ) {
      lsf = (newhead & ((long)1<<19)) ? 0x0 : 0x1;
      mpeg25 = 0;
    } else {
      lsf = 1;
      mpeg25 = 1;
    }

    if(mpeg25)
      sampling_frequency = 6 + ((newhead>>10)&0x3);
    else
      sampling_frequency = ((newhead>>10)&0x3) + (lsf*3);

    if(sampling_frequency>8){
	return -1;  // valid: 0..8
    }

    crc = ((newhead>>16)&0x1)^0x1;
    bitrate_index = ((newhead>>12)&0xf);
    padding   = ((newhead>>9)&0x1);

    nch = ( (((newhead>>6)&0x3)) == 3) ? 1 : 2;
    
    if(channels) *channels=nch;

    if(!bitrate_index){
      return -1;
    }

    if(lsf)
      ssize = (nch == 1) ? 9 : 17;
    else
      ssize = (nch == 1) ? 17 : 32;
    if(crc) ssize += 2;

    switch(mp3_fmt)
    {
	case 0:		return -1;
	case 1:		framesize = (long) tabsel_123[lsf][0][bitrate_index]*12000;
    			framesize /= freqs[sampling_frequency];
			framesize = (framesize + padding)<<2;
			break;
	default:
			framesize  = (long) tabsel_123[lsf][mp3_fmt-1][bitrate_index] * 144000;
			if(mp3_fmt == 3)framesize /= freqs[sampling_frequency]<<lsf;
			else		framesize /= freqs[sampling_frequency];
			framesize += padding;
			break;
    }

    if(framesize<=0 || framesize>MAXFRAMESIZE) return -1;
    if(brate) *brate=(tabsel_123[lsf][mp3_fmt-1][bitrate_index]*1000)/8;
    if(samplerate) *samplerate=freqs[sampling_frequency];
    return framesize;
}

/* id3v2 */
#define FOURCC_TAG mmioFOURCC
#define ID3V22_TAG FOURCC_TAG('I', 'D', '3', 2)  /* id3 v2.2 tags */
#define ID3V23_TAG FOURCC_TAG('I', 'D', '3', 3)  /* id3 v2.3 tags */
#define ID3V24_TAG FOURCC_TAG('I', 'D', '3', 4)  /* id3 v2.4 tags */

/*
 *  ID3 v2.2
 */
/* tag header */
#define ID3V22_UNSYNCH_FLAG               0x80
#define ID3V22_COMPRESS_FLAG              0x40
#define ID3V22_ZERO_FLAG                  0x3F

/* frame header */
#define ID3V22_FRAME_HEADER_SIZE             6
static int read_id3v22_tags(unsigned flags,unsigned hsize)
{
    __filesize_t pos,epos;
    if(	flags==ID3V22_ZERO_FLAG ||
	flags==ID3V22_UNSYNCH_FLAG ||
	flags==ID3V22_COMPRESS_FLAG) return 0;    
    pos=bmGetCurrFilePos();
    epos=pos+hsize;
    while(pos<epos)
    {
	unsigned long id;
	unsigned len;
	unsigned char buf[ID3V22_FRAME_HEADER_SIZE],data[4096];
	bmReadBuffer(buf,ID3V22_FRAME_HEADER_SIZE);
	id=(buf[2] << 16) + (buf[1] << 8) + buf[0];
	len=(buf[3] << 14) + (buf[4] << 7) + buf[5];
	bmReadBuffer(data,min(len,4096));
	data[len]=0;
#if 0
	switch(id)
	{
	    case mmioFOURCC(0,'T','T','1'): if(len>1) demux_info_add(demuxer,INFOT_DESCRIPTION,data+1); break;
	    case mmioFOURCC(0,'T','T','2'): if(len>1) demux_info_add(demuxer,INFOT_NAME,data+1); break;
	    case mmioFOURCC(0,'T','T','3'): if(len>1) demux_info_add(demuxer,INFOT_SUBJECT,data+1); break;
	    case mmioFOURCC(0,'C','O','M'): if(len>4) demux_info_add(demuxer,INFOT_COMMENTS,data+4); break;
	    case mmioFOURCC(0,'T','C','O'): if(len>1) demux_info_add(demuxer,INFOT_GENRE,genres[data[1]]); break;
	    case mmioFOURCC(0,'T','C','R'): if(len>1) demux_info_add(demuxer,INFOT_COPYRIGHT,genres[data[1]]); break;
	    case mmioFOURCC(0,'T','P','1'): if(len>1) demux_info_add(demuxer,INFOT_AUTHOR,data+1); break;
	    case mmioFOURCC(0,'T','A','L'): if(len>1) demux_info_add(demuxer,INFOT_ALBUM,data+1); break;
	    case mmioFOURCC(0,'T','R','K'): if(len>1) demux_info_add(demuxer,INFOT_TRACK,data+1); break;
	    case mmioFOURCC(0,'T','Y','E'): if(len>1) demux_info_add(demuxer,INFOT_DATE,data+1); break;
	    case mmioFOURCC(0,'T','E','N'): if(len>1) demux_info_add(demuxer,INFOT_ENCODER,data+1); break;
	    case mmioFOURCC(0,'T','M','T'): if(len>1) demux_info_add(demuxer,INFOT_SOURCE_MEDIA,data+1); break;
	    case mmioFOURCC(0,'T','F','T'): if(len>1) demux_info_add(demuxer,INFOT_MIME,data+1); break;
	    case mmioFOURCC(0,'P','O','P'): if(len>1) demux_info_add(demuxer,INFOT_RATING,data+1); break;
	    case mmioFOURCC(0,'W','X','X'): if(len>1) demux_info_add(demuxer,INFOT_WWW,data+1); break;
	    case 0: goto end;
	    default: printf("Unhandled frame: %3s\n",buf); break;
	}
#endif
	pos=bmGetCurrFilePos();
    }
    end:
    return 1;
}

/*
 *  ID3 v2.3
 */
/* tag header */
#define ID3V23_UNSYNCH_FLAG               0x80
#define ID3V23_EXT_HEADER_FLAG            0x40
#define ID3V23_EXPERIMENTAL_FLAG          0x20
#define ID3V23_ZERO_FLAG                  0x1F

/* frame header */
#define ID3V23_FRAME_HEADER_SIZE            10
#define ID3V23_FRAME_TAG_PRESERV_FLAG   0x8000
#define ID3V23_FRAME_FILE_PRESERV_FLAG  0x4000
#define ID3V23_FRAME_READ_ONLY_FLAG     0x2000
#define ID3V23_FRAME_COMPRESS_FLAG      0x0080
#define ID3V23_FRAME_ENCRYPT_FLAG       0x0040
#define ID3V23_FRAME_GROUP_ID_FLAG      0x0020
#define ID3V23_FRAME_ZERO_FLAG          0x1F1F

static int read_id3v23_tags(unsigned flags,unsigned hsize)
{
    __filesize_t pos,epos;
    if(	flags==ID3V23_ZERO_FLAG ||
	flags==ID3V23_UNSYNCH_FLAG) return 0;
    if( flags==ID3V23_EXT_HEADER_FLAG )
    {
	char buf[4];
	unsigned ehsize;
	bmReadBuffer(buf,4);
	ehsize=(buf[0] << 21) + (buf[1] << 14) + (buf[2] << 7) + buf[3];
	bmSeek(ehsize,BM_SEEK_CUR);
    }
    pos=bmGetCurrFilePos();
    epos=pos+hsize;
    while(pos<epos)
    {
	unsigned long id;
	unsigned len;
	unsigned char buf[ID3V23_FRAME_HEADER_SIZE],data[4096];
	bmReadBuffer(buf,ID3V23_FRAME_HEADER_SIZE);
	id=*((unsigned long *)buf);
	len=(buf[4] << 21) + (buf[5] << 14) + (buf[6] << 7) + buf[7];
	if((int)len <= 0) return 0;
	bmReadBuffer(data,min(len,4096));
	data[len]=0;
#if 0
	switch(id)
	{
	    case mmioFOURCC('T','I','T','1'): if(len>1) demux_info_add(demuxer,INFOT_DESCRIPTION,data+1); break;
	    case mmioFOURCC('T','I','T','2'): if(len>1) demux_info_add(demuxer,INFOT_NAME,data+1); break;
	    case mmioFOURCC('T','I','T','3'): if(len>1) demux_info_add(demuxer,INFOT_SUBJECT,data+1); break;
	    case mmioFOURCC('C','O','M','M'): if(len>4) demux_info_add(demuxer,INFOT_COMMENTS,data+4); break;
	    case mmioFOURCC('T','C','O','N'): if(len>1) demux_info_add(demuxer,INFOT_GENRE,genres[data[1]]); break;
	    case mmioFOURCC('T','P','E','1'): if(len>1) demux_info_add(demuxer,INFOT_AUTHOR,data+1); break;
	    case mmioFOURCC('T','A','L','B'): if(len>1) demux_info_add(demuxer,INFOT_ALBUM,data+1); break;
	    case mmioFOURCC('T','R','C','K'): if(len>1) demux_info_add(demuxer,INFOT_TRACK,data+1); break;
	    case mmioFOURCC('T','Y','E','R'): if(len>1) demux_info_add(demuxer,INFOT_DATE,data+1); break;
	    case mmioFOURCC('T','E','N','C'): if(len>1) demux_info_add(demuxer,INFOT_ENCODER,data+1); break;
	    case mmioFOURCC('T','C','O','P'): if(len>1) demux_info_add(demuxer,INFOT_COPYRIGHT,data+1); break;
	    case mmioFOURCC('T','M','E','D'): if(len>1) demux_info_add(demuxer,INFOT_SOURCE_MEDIA,data+1); break;
	    case mmioFOURCC('T','F','L','T'): if(len>1) demux_info_add(demuxer,INFOT_MIME,data+1); break;
	    case mmioFOURCC('P','O','P','M'): if(len>1) demux_info_add(demuxer,INFOT_RATING,data+1); break;
	    case mmioFOURCC('W','X','X','X'): if(len>1) demux_info_add(demuxer,INFOT_WWW,data+1); break;
	    case 0: goto end;
	    default: printf("Unhandled frame: %4s\n",buf); break;
	}
#endif
	pos=bmGetCurrFilePos();
    }
    end:
    return 1;
}

/*
 *  ID3 v2.4
 */
/* tag header */
#define ID3V24_UNSYNCH_FLAG               0x80
#define ID3V24_EXT_HEADER_FLAG            0x40
#define ID3V24_EXPERIMENTAL_FLAG          0x20
#define ID3V24_FOOTER_FLAG                0x10
#define ID3V24_ZERO_FLAG                  0x0F

/* frame header */
#define ID3V24_FRAME_HEADER_SIZE            10
#define ID3V24_FRAME_TAG_PRESERV_FLAG   0x4000
#define ID3V24_FRAME_FILE_PRESERV_FLAG  0x2000
#define ID3V24_FRAME_READ_ONLY_FLAG     0x1000
#define ID3V24_FRAME_GROUP_ID_FLAG      0x0040
#define ID3V24_FRAME_COMPRESS_FLAG      0x0008
#define ID3V24_FRAME_ENCRYPT_FLAG       0x0004
#define ID3V24_FRAME_UNSYNCH_FLAG       0x0002
#define ID3V24_FRAME_DATA_LEN_FLAG      0x0001
#define ID3V24_FRAME_ZERO_FLAG          0x8FB0

static int read_id3v24_tags(unsigned flags,unsigned hsize)
{
    __filesize_t pos,epos;
    if(	flags==ID3V24_ZERO_FLAG ||
	flags==ID3V24_UNSYNCH_FLAG) return 0;
    if( flags==ID3V24_EXT_HEADER_FLAG )
    {
	char buf[4];
	unsigned ehsize;
	bmReadBuffer(buf,4);
	ehsize=(buf[0] << 21) + (buf[1] << 14) + (buf[2] << 7) + buf[3];
	bmSeek(ehsize,BM_SEEK_CUR);
    }
    pos=bmGetCurrFilePos();
    epos=pos+hsize;
    while(pos<epos)
    {
	unsigned long id;
	unsigned len;
	unsigned char buf[ID3V23_FRAME_HEADER_SIZE],data[4096];
	bmReadBuffer(buf,ID3V23_FRAME_HEADER_SIZE);
	id=*((unsigned long *)buf);
	len=(buf[4] << 21) + (buf[5] << 14) + (buf[6] << 7) + buf[7];
	bmReadBuffer(data,min(len,4096));
	data[len]=0;
#if 0
	switch(id)
	{
	    /* first byte of data indicates encoding type: 0-ASCII (1-2)-UTF16(LE,BE) 3-UTF8 */
	    case mmioFOURCC('T','I','T','1'): if(len>1) demux_info_add(demuxer,INFOT_DESCRIPTION,data+1); break;
	    case mmioFOURCC('T','I','T','2'): if(len>1) demux_info_add(demuxer,INFOT_NAME,data+1); break;
	    case mmioFOURCC('T','I','T','3'): if(len>1) demux_info_add(demuxer,INFOT_SUBJECT,data+1); break;
	    case mmioFOURCC('C','O','M','M'): if(len>4) demux_info_add(demuxer,INFOT_COMMENTS,data+4); break;
	    case mmioFOURCC('T','C','O','N'): if(len>1) demux_info_add(demuxer,INFOT_GENRE,genres[data[1]]); break;
	    case mmioFOURCC('T','P','E','1'): if(len>1) demux_info_add(demuxer,INFOT_AUTHOR,data+1); break;
	    case mmioFOURCC('T','A','L','B'): if(len>1) demux_info_add(demuxer,INFOT_ALBUM,data+1); break;
	    case mmioFOURCC('T','R','C','K'): if(len>1) demux_info_add(demuxer,INFOT_TRACK,data+1); break;
/*!*/	    case mmioFOURCC('T','D','R','C'): if(len>1) demux_info_add(demuxer,INFOT_DATE,data+1); break;
	    case mmioFOURCC('T','E','N','C'): if(len>1) demux_info_add(demuxer,INFOT_ENCODER,data+1); break;
	    case mmioFOURCC('T','C','O','P'): if(len>1) demux_info_add(demuxer,INFOT_COPYRIGHT,data+1); break;
	    case mmioFOURCC('T','M','E','D'): if(len>1) demux_info_add(demuxer,INFOT_SOURCE_MEDIA,data+1); break;
	    case mmioFOURCC('T','F','L','T'): if(len>1) demux_info_add(demuxer,INFOT_MIME,data+1); break;
	    case mmioFOURCC('P','O','P','M'): if(len>1) demux_info_add(demuxer,INFOT_RATING,data+1); break;
	    case mmioFOURCC('W','X','X','X'): if(len>1) demux_info_add(demuxer,INFOT_WWW,data+1); break;
	    case 0: goto end;
	    default: printf("Unhandled frame: %4s\n",buf); break;
	}
#endif
	pos=bmGetCurrFilePos();
    }
    end:
    return 1;
}

static int read_id3v2_tags(void)
{
    char buf[4];
    unsigned vers,rev,flags,hsize;
    bmSeek(3,BM_SEEK_SET); /* skip 'ID3' */
    vers=bmReadByte();
    rev=bmReadByte();
    flags=bmReadByte();
    bmReadBuffer(buf,4);
    hsize=(buf[0] << 21) + (buf[1] << 14) + (buf[2] << 7) + buf[3];
    if(vers==2) return read_id3v22_tags(flags,hsize);
    else
    if(vers==3) return read_id3v23_tags(flags,hsize);
    else
    if(vers==4) return read_id3v24_tags(flags,hsize);
    else
    return 1;
}

static void find_next_mp3_hdr(unsigned char *hdr) {
  int len;
  __filesize_t spos;
  while(!bmEOF()) {
    spos=bmGetCurrFilePos();
    bmReadBuffer(hdr,4);
    if(bmEOF()) break;
    len = mp_decode_mp3_header(hdr,NULL,NULL,NULL,NULL);
    if(len < 0) {
      bmSeek(-3,BM_SEEK_CUR);
      continue;
    }
    bmSeek(spos,BM_SEEK_SET);
    break;
  }
}

#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008
#define FRAMES_AND_BYTES (FRAMES_FLAG | BYTES_FLAG)
#define MPG_MD_MONO     3

static int Xing_test(char *hdr,int *scale,int *lsf,int *srate,long *nframes,long *nbytes)
{
    __filesize_t fpos;
    unsigned mpeg1, mode, sr_index;
    unsigned off,head_flags;
    char buf[4];
    const int sr_table[4] = { 44100, 48000, 32000, 99999 };
    int is_xing=0;
    *scale=-1;
    mpeg1    = (hdr[1]>>3)&1;
    sr_index = (hdr[2]>>2)&3;
    mode     = (hdr[3]>>6)&3;
    if(mpeg1)	off=mode!=MPG_MD_MONO?32:17;
    else	off=mode!=MPG_MD_MONO?17:9;/* mpeg2 */
    fpos = bmGetCurrFilePos();
    bmSeek(off,BM_SEEK_CUR);
    bmReadBuffer(buf,4);
    if(memcmp(buf,"Xing",4) == 0 || memcmp(buf,"Info",4) == 0)
    {
	is_xing=1;
	*nbytes=*nframes=0;
	*lsf=mpeg1?0:1;
	*srate=sr_table[sr_index&0x3];
	head_flags = be2me_32(bmReadDWord());
	if(head_flags & FRAMES_FLAG)	*nframes=be2me_32(bmReadDWord());
	if(head_flags & BYTES_FLAG)	*nbytes=be2me_32(bmReadDWord());
	if(head_flags & TOC_FLAG)	bmSeek(100,BM_SEEK_CUR); /* skip indexes */
	if(head_flags & VBR_SCALE_FLAG)	*scale = be2me_32(bmReadDWord());
    }
    bmSeek(fpos,BM_SEEK_SET);
    return is_xing;
}
#include <stdio.h>
static tBool  __FASTCALL__ mp3_check_fmt( void )
{
    unsigned i;
    unsigned long off;
    int fmt = 0, n = 0, pos = 0, step,mp3_brate,mp3_samplerate,mp3_channels;
    unsigned char hdr[4];
    bmSeek(0,BM_SEEK_SET);
    bmReadBuffer(hdr,4);
    if( hdr[0] == 'I' && hdr[1] == 'D' && hdr[2] == '3' && (hdr[3] >= 2))
    {
	int len,fmt;
	int scale,lsf,srate;
	long nframes,nbytes;
	bmSeek(2,BM_SEEK_CUR);
	bmReadBuffer(hdr,4);
	len = (hdr[0]<<21) | (hdr[1]<<14) | (hdr[2]<<7) | hdr[3];
	read_id3v2_tags();
	bmSeek(len+10,BM_SEEK_SET);
	find_next_mp3_hdr(hdr);
//	Xing_test(hdr,&scale,&lsf,&srate,&nframes,&nbytes);
	if(mp_decode_mp3_header(hdr,&fmt,&mp3_brate,&mp3_samplerate,&mp3_channels) > 0)	return True;
    }
    else
    {
	if(mp_decode_mp3_header(hdr,NULL,NULL,NULL,NULL) < 0) return False;
	find_next_mp3_hdr(hdr);
	if(bmEOF()) return False;
	for(i=0;i<5;i++)
	{
	    if((off=mp_decode_mp3_header(hdr,&fmt,&mp3_brate,&mp3_samplerate,&mp3_channels)) < 0) return False;
	    bmSeek(off,BM_SEEK_CUR);
	    if(bmEOF()) return False;

	}
    }
    return True;
}

static __filesize_t __FASTCALL__ Show_MP3_Header( void )
{
 unsigned keycode;
 TWindow * hwnd;
 __filesize_t newcpos,fpos,fpos2;
 unsigned long FPageCnt;
 const char * addinfo;
 int fmt = 0, n = 0, pos = 0, step,mp3_brate,mp3_samplerate,mp3_channels;
 unsigned char hdr[4];
 int len;
 int scale,lsf,srate;
 long nframes,nbytes,ave_brate;
 int is_xing=0;
 fpos2 = fpos = BMGetCurrFilePos();
 bmSeek(0,BM_SEEK_SET);
 bmReadBuffer(hdr,4);
 if( hdr[0] == 'I' && hdr[1] == 'D' && hdr[2] == '3' && (hdr[3] >= 2))
 {
	bmSeek(2,BM_SEEK_CUR);
	bmReadBuffer(hdr,4);
	len = (hdr[0]<<21) | (hdr[1]<<14) | (hdr[2]<<7) | hdr[3];
	read_id3v2_tags();
	bmSeek(len+10,BM_SEEK_SET);
	find_next_mp3_hdr(hdr);
	fpos2=bmGetCurrFilePos();
 }
 is_xing=Xing_test(hdr,&scale,&lsf,&srate,&nframes,&nbytes);
 mp_decode_mp3_header(hdr,&fmt,&mp3_brate,&mp3_samplerate,&mp3_channels);
 ave_brate=mp3_brate*8;
 if(is_xing)
 {
    ave_brate=(((float)(nbytes)/nframes)*(float)(srate<<lsf))/144.;
    find_next_mp3_hdr(hdr);
    fpos2=bmGetCurrFilePos();
 }
 hwnd = CrtDlgWndnls(" MP3 File Header ",43,4);
 twUseWin(hwnd);
 twGotoXY(1,1);
 twPrintF("Type                 = MP3v%u:%s\n"
          "Bitrate              = %u\n"
	  "SampleRate           = %u\n"
	  "nChannels            = %u\n"
	  ,fmt
          ,is_xing?"VBR":"CBR"
          ,ave_brate
	  ,is_xing?srate:mp3_samplerate
	  ,mp3_channels
          );
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

static void __FASTCALL__ mp3_init_fmt( void ) {}
static void __FASTCALL__ mp3_destroy_fmt(void) {}
static int  __FASTCALL__ mp3_platform( void) { return DISASM_DEFAULT; }

REGISTRY_BIN mp3Table =
{
  "MP3 file format",
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  mp3_check_fmt,
  mp3_init_fmt,
  mp3_destroy_fmt,
  Show_MP3_Header,
  NULL,
  NULL,
  mp3_platform,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
