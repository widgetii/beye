/**
 * @namespace   biew
 * @file        bmfile.h
 * @brief       This file contains prototypes of Buffering streams Manager.
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
#ifndef __BMFILE_INC
#define __BMFILE_INC

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __BBIO_H
#include "biewlib/bbio.h"
#endif

#if __WORDSIZE == 16
#define BBIO_CACHE_SIZE        0x4000  /* 16k */
#define BBIO_SMALL_CACHE_SIZE  0x1000  /* 4k */
#else
#define BBIO_CACHE_SIZE        0xFFFF  /* 64k */
#define BBIO_SMALL_CACHE_SIZE  0x4000  /* 16k */
#endif

BGLOBAL        __FASTCALL__ biewOpenRO(char *fname,unsigned cache_size);
BGLOBAL        __FASTCALL__ biewOpenRW(char *fname,unsigned cache_size);

#define BM_SEEK_SET BIO_SEEK_SET
#define BM_SEEK_CUR BIO_SEEK_CUR
#define BM_SEEK_END BIO_SEEK_END

int            __FASTCALL__ BMOpen(char * fname);
void           __FASTCALL__ BMClose( void );
tBool          __FASTCALL__ BMEOF( void );
void           __FASTCALL__ BMSeek(long pos,int RELATION);
void           __FASTCALL__ BMReRead( void );
tUInt8        __FASTCALL__ BMReadByte( void );
tUInt16       __FASTCALL__ BMReadWord( void );
tUInt32       __FASTCALL__ BMReadDWord( void );
tBool          __FASTCALL__ BMReadBuffer(void  * buffer,unsigned len);
tUInt8        __FASTCALL__ BMReadByteEx(long pos,int RELATION);
tUInt16       __FASTCALL__ BMReadWordEx(long pos,int RELATION);
tUInt32       __FASTCALL__ BMReadDWordEx(long pos,int RELATION);
tBool          __FASTCALL__ BMReadBufferEx(void  * buffer,unsigned len,long pos,int RELATION);
int            __FASTCALL__ BMHandle( void );
BGLOBAL        __FASTCALL__ BMbioHandle( void );
char *         __FASTCALL__ BMName( void );

unsigned long  __FASTCALL__ BMGetCurrFilePos( void );
unsigned long  __FASTCALL__ BMGetFLength( void );

tBool          __FASTCALL__ BMWriteByte(tUInt8 byte);
tBool          __FASTCALL__ BMWriteWord(tUInt16 word);
tBool          __FASTCALL__ BMWriteDWord(tUInt32 dword);
tBool          __FASTCALL__ BMWriteBuff(void  * buff,unsigned len);
tBool          __FASTCALL__ BMWriteByteEx(long pos,int RELATION,tUInt8 byte);
tBool          __FASTCALL__ BMWriteWordEx(long pos,int RELATION,tUInt16 word);
tBool          __FASTCALL__ BMWriteDWordEx(long pos,int RELATION,tUInt32 dword);
tBool          __FASTCALL__ BMWriteBuffEx(long pos,int RELATION,void  * buff,unsigned len);

/** Below analogs with using small cache size */

tBool          __FASTCALL__ bmEOF( void );
void           __FASTCALL__ bmSeek(long pos,int RELATION);
void           __FASTCALL__ bmReRead( void );
tUInt8        __FASTCALL__ bmReadByte( void );
tUInt16       __FASTCALL__ bmReadWord( void );
tUInt32       __FASTCALL__ bmReadDWord( void );
tBool          __FASTCALL__ bmReadBuffer(void  * buffer,unsigned len);
tUInt8        __FASTCALL__ bmReadByteEx(long pos,int RELATION);
tUInt16       __FASTCALL__ bmReadWordEx(long pos,int RELATION);
tUInt32       __FASTCALL__ bmReadDWordEx(long pos,int RELATION);
tBool          __FASTCALL__ bmReadBufferEx(void  * buffer,unsigned len,long pos,int RELATION);
int            __FASTCALL__ bmHandle( void );
BGLOBAL        __FASTCALL__ bmbioHandle( void );
char *         __FASTCALL__ bmName( void );

unsigned long  __FASTCALL__ bmGetCurrFilePos( void );
unsigned long  __FASTCALL__ bmGetFLength( void );

tBool          __FASTCALL__ bmWriteByte(tUInt8 byte);
tBool          __FASTCALL__ bmWriteWord(tUInt16 word);
tBool          __FASTCALL__ bmWriteDWord(tUInt32 dword);
tBool          __FASTCALL__ bmWriteBuff(void  * buff,unsigned len);
tBool          __FASTCALL__ bmWriteByteEx(long pos,int RELATION,tUInt8 byte);
tBool          __FASTCALL__ bmWriteWordEx(long pos,int RELATION,tUInt16 word);
tBool          __FASTCALL__ bmWriteDWordEx(long pos,int RELATION,tUInt32 dword);
tBool          __FASTCALL__ bmWriteBuffEx(long pos,int RELATION,void  * buff,unsigned len);

#ifdef __cplusplus
}
#endif

#endif
