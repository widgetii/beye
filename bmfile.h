/**
 * @namespace   biew
 * @file        bmfile.h
 * @brief       This file contains prototypes of Buffering streams Manager.
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
#ifndef __BMFILE_INC
#define __BMFILE_INC

#include "libbeye/sysdep/generic/__config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __BBIO_H
#include "libbeye/bbio.h"
#endif

#define HA_LEN ((BMFileFlags&BMFF_USE64)?18:10)

#define BMFF_NONE	0x00000000
#define BMFF_USE64	0x00000001
extern unsigned BMFileFlags;

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
extern BGLOBAL bm_file_handle,sc_bm_file_handle;

int            __FASTCALL__ BMOpen(char * fname);
void           __FASTCALL__ BMClose( void );
static inline bhandle_t		__FASTCALL__ BMHandle( void ) { return bioHandle(bm_file_handle); }
static inline BGLOBAL		__FASTCALL__ BMbioHandle( void ) { return bm_file_handle; }
static inline char *		__FASTCALL__ BMName( void ) { return bioFileName(bm_file_handle); }
static inline __filesize_t	__FASTCALL__ BMGetCurrFilePos( void ) { return bioTell(bm_file_handle); }
static inline __filesize_t	__FASTCALL__ BMGetFLength( void ) { return bioFLength(bm_file_handle); }
static inline tBool	__FASTCALL__ BMEOF( void ) { return bioEOF(bm_file_handle); }
static inline void	__FASTCALL__ BMSeek(__fileoff_t pos,int RELATION) { bioSeek(bm_file_handle,pos,RELATION); } 
static inline void	__FASTCALL__ BMReRead( void )  { bioReRead(bm_file_handle); }
static inline tUInt8	__FASTCALL__ BMReadByte( void ) { return bioReadByte(bm_file_handle); }
static inline tUInt16	__FASTCALL__ BMReadWord( void ) { return bioReadWord(bm_file_handle); }
static inline tUInt32	__FASTCALL__ BMReadDWord( void ) { return bioReadDWord(bm_file_handle); }
static inline tUInt64	__FASTCALL__ BMReadQWord( void ) { return bioReadQWord(bm_file_handle); }
static inline tBool	__FASTCALL__ BMReadBuffer(void  * buffer,unsigned len) { return bioReadBuffer(bm_file_handle,buffer,len); }
tUInt8        __FASTCALL__ BMReadByteEx(__fileoff_t pos,int RELATION);
tUInt16       __FASTCALL__ BMReadWordEx(__fileoff_t pos,int RELATION);
tUInt32       __FASTCALL__ BMReadDWordEx(__fileoff_t pos,int RELATION);
tUInt64       __FASTCALL__ BMReadQWordEx(__fileoff_t pos,int RELATION);
tBool         __FASTCALL__ BMReadBufferEx(void  * buffer,unsigned len,__fileoff_t pos,int RELATION);
static inline tBool	__FASTCALL__ BMWriteByte(tUInt8 byte) { return bioWriteByte(bm_file_handle,byte); }
static inline tBool	__FASTCALL__ BMWriteWord(tUInt16 word) { return bioWriteWord(bm_file_handle,word); }
static inline tBool	__FASTCALL__ BMWriteDWord(tUInt32 dword) { return bioWriteDWord(bm_file_handle,dword); }
static inline tBool	__FASTCALL__ BMWriteQWord(tUInt64 qword) { return bioWriteQWord(bm_file_handle,qword); }
tBool		__FASTCALL__ BMWriteBuff(void* buff,unsigned len);
tBool		__FASTCALL__ BMWriteByteEx(__fileoff_t pos,int RELATION,tUInt8 byte);
tBool		__FASTCALL__ BMWriteWordEx(__fileoff_t pos,int RELATION,tUInt16 word);
tBool		__FASTCALL__ BMWriteDWordEx(__fileoff_t pos,int RELATION,tUInt32 dword);
tBool		__FASTCALL__ BMWriteBuffEx(__fileoff_t pos,int RELATION,void  * buff,unsigned len);

/** Below analogs with using small cache size */

static inline tBool	__FASTCALL__ bmEOF( void ) { return bioEOF(sc_bm_file_handle); }
static inline void	__FASTCALL__ bmSeek(__fileoff_t pos,int RELATION) { bioSeek(sc_bm_file_handle,pos,RELATION); }
static inline void	__FASTCALL__ bmReRead( void ) { bioReRead(sc_bm_file_handle); }
static inline tUInt8	__FASTCALL__ bmReadByte( void ) { return bioReadByte(sc_bm_file_handle); }
static inline tUInt16	__FASTCALL__ bmReadWord( void ) { return bioReadWord(sc_bm_file_handle); }
static inline tUInt32	__FASTCALL__ bmReadDWord( void ) { return bioReadDWord(sc_bm_file_handle); }
static inline tUInt64	__FASTCALL__ bmReadQWord( void ) { return bioReadQWord(sc_bm_file_handle); }
static inline tBool	__FASTCALL__ bmReadBuffer(void  * buffer,unsigned len) { return bioReadBuffer(sc_bm_file_handle,buffer,len); }
tUInt8        __FASTCALL__ bmReadByteEx(__fileoff_t pos,int RELATION);
tUInt16       __FASTCALL__ bmReadWordEx(__fileoff_t pos,int RELATION);
tUInt32       __FASTCALL__ bmReadDWordEx(__fileoff_t pos,int RELATION);
tUInt64       __FASTCALL__ bmReadQWordEx(__fileoff_t pos,int RELATION);
tBool         __FASTCALL__ bmReadBufferEx(void  * buffer,unsigned len,__fileoff_t pos,int RELATION);
static inline bhandle_t	__FASTCALL__ bmHandle( void ) { return bioHandle(sc_bm_file_handle); }
static inline BGLOBAL	__FASTCALL__ bmbioHandle( void ) { return sc_bm_file_handle; }
static inline char *	__FASTCALL__ bmName( void ) { return bioFileName(sc_bm_file_handle); }

static inline __filesize_t	__FASTCALL__ bmGetCurrFilePos( void ) { return bioTell(sc_bm_file_handle); }
static inline __filesize_t	__FASTCALL__ bmGetFLength( void ) {  return bioFLength(sc_bm_file_handle); }

static inline tBool	__FASTCALL__ bmWriteByte(tUInt8 byte) { return bioWriteByte(sc_bm_file_handle,byte); }
static inline tBool	__FASTCALL__ bmWriteWord(tUInt16 word) { return bioWriteWord(sc_bm_file_handle,word); }
static inline tBool	__FASTCALL__ bmWriteDWord(tUInt32 dword) { return bioWriteDWord(sc_bm_file_handle,dword); }
static inline tBool	__FASTCALL__ bmWriteQWord(tUInt64 qword) { return bioWriteQWord(sc_bm_file_handle,qword); }
tBool          __FASTCALL__ bmWriteBuff(void  * buff,unsigned len);
tBool          __FASTCALL__ bmWriteByteEx(__fileoff_t pos,int RELATION,tUInt8 byte);
tBool          __FASTCALL__ bmWriteWordEx(__fileoff_t pos,int RELATION,tUInt16 word);
tBool          __FASTCALL__ bmWriteDWordEx(__fileoff_t pos,int RELATION,tUInt32 dword);
tBool          __FASTCALL__ bmWriteQWordEx(__fileoff_t pos,int RELATION,tUInt64 dword);
tBool          __FASTCALL__ bmWriteBuffEx(__fileoff_t pos,int RELATION,void  * buff,unsigned len);

#ifdef __cplusplus
}
#endif

#endif
