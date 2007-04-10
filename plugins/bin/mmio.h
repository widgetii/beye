#ifndef __MMIO
#define __MMIO 1

typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )				\
		( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |	\
		( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

#define FOURCC DWORD

typedef struct
{
    WORD  left;
    WORD  top;
    WORD  right;
    WORD  bottom;
} RECT, LPRECT;

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
typedef struct __attribute__((__packed__)) _WAVEFORMATEX {
  WORD   wFormatTag;
  WORD   nChannels;
  DWORD  nSamplesPerSec;
  DWORD  nAvgBytesPerSec;
  WORD   nBlockAlign;
  WORD   wBitsPerSample;
  WORD   cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *NPWAVEFORMATEX, *LPWAVEFORMATEX;
#endif /* _WAVEFORMATEX_ */

#ifndef _BITMAPINFOHEADER_
#define _BITMAPINFOHEADER_
typedef struct __attribute__((__packed__))
{
    int 	biSize;
    int  	biWidth;
    int  	biHeight;
    short 	biPlanes;
    short 	biBitCount;
    int 	biCompression;
    int 	biSizeImage;
    int  	biXPelsPerMeter;
    int  	biYPelsPerMeter;
    int 	biClrUsed;
    int 	biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER, *LPBITMAPINFOHEADER;
#endif

/* this macro helps print FOURCC */
#define INT_2_CHAR_ARG(i)\
,((char *)&i)[0]?((char *)&i)[0]:' '\
,((char *)&i)[1]?((char *)&i)[1]:' '\
,((char *)&i)[2]?((char *)&i)[2]:' '\
,((char *)&i)[3]?((char *)&i)[3]:' '

typedef struct
{
    unsigned short wTag;
    const char *name;
}wTagNames;
extern wTagNames wtagNames[];
extern const char *wtag_find_name(unsigned short wtag);

#endif
