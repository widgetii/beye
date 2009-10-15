/**
 * @namespace   biew
 * @file        bmfile.c
 * @brief       This file has developed as Buffered stream Manager and presents
 *              front end interface to bbio library.
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
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "bmfile.h"
#include "bconsole.h"
#include "tstrings.h"
#include "biewlib/bbio.h"

unsigned BMFileFlags=0;
extern tBool fioUseMMF;
static BGLOBAL file = NULL,sc_file = NULL;
static char FName[FILENAME_MAX+1];

BGLOBAL __FASTCALL__ biewOpenRO(char *fname,unsigned cache_size)
{
  BGLOBAL fret;
  fret = bioOpen(fname,FO_READONLY | SO_DENYNONE,cache_size,fioUseMMF ? BIO_OPT_USEMMF : BIO_OPT_DB);
  if(fret == &bNull)
    fret = bioOpen(fname,FO_READONLY | SO_COMPAT,cache_size,fioUseMMF ? BIO_OPT_USEMMF : BIO_OPT_DB);
  return fret;
}

BGLOBAL __FASTCALL__ biewOpenRW(char *fname,unsigned cache_size)
{
  BGLOBAL fret;
  fret = bioOpen(fname,FO_READWRITE | SO_DENYNONE,cache_size,fioUseMMF ? BIO_OPT_USEMMF : BIO_OPT_DB);
  if(fret == &bNull)
    fret = bioOpen(fname,FO_READWRITE | SO_COMPAT,cache_size,fioUseMMF ? BIO_OPT_USEMMF : BIO_OPT_DB);
  return fret;
}

int __FASTCALL__ BMOpen(char * fname)
{
  strcpy(FName,fname);
  file = biewOpenRO(fname,BBIO_CACHE_SIZE);
  if(file == &bNull)
  {
    errnoMessageBox(OPEN_FAIL,NULL,errno);
    return -1;
  }
  sc_file = bioDupEx(file,BBIO_SMALL_CACHE_SIZE);
  if(sc_file == &bNull)
  {
    errnoMessageBox(DUP_FAIL,NULL,errno);
    return -1;
  }
  bioSetOptimization(file,BIO_OPT_RANDOM);
  bioSetOptimization(sc_file,BIO_OPT_RANDOM);
  if(BMGetFLength() > ULONG_MAX) BMFileFlags |= BMFF_USE64;
  return 0;
}

bhandle_t __FASTCALL__ BMHandle( void )
{
  return bioHandle(file);
}

BGLOBAL __FASTCALL__ BMbioHandle( void )
{
  return file;
}

void __FASTCALL__ BMClose( void )
{
  if(file != &bNull) bioClose(file);
  file = &bNull;
  if(sc_file != &bNull) bioClose(sc_file);
  sc_file = &bNull;
}

void __FASTCALL__ BMSeek(__fileoff_t pos,int RELATION)
{
  bioSeek(file,pos,RELATION);
}

void __FASTCALL__ BMReRead( void )
{
  bioReRead(file);
}

tUInt8  __FASTCALL__ BMReadByte( void )
{
  return bioReadByte(file);
}

tUInt16 __FASTCALL__ BMReadWord( void )
{
 return bioReadWord(file);
}

tUInt32  __FASTCALL__ BMReadDWord( void )
{
  return bioReadDWord(file);
}

tUInt64  __FASTCALL__ BMReadQWord( void )
{
  return bioReadQWord(file);
}

__filesize_t  __FASTCALL__ BMGetCurrFilePos( void )
{
  return bioTell(file);
}

__filesize_t __FASTCALL__ BMGetFLength( void )
{
  return bioFLength(file);
}

tBool __FASTCALL__ BMReadBuffer(void * buffer,unsigned len)
{
  return bioReadBuffer(file,buffer,len);
}

tUInt8  __FASTCALL__ BMReadByteEx(__fileoff_t pos,int RELATION)
{
 bioSeek(file,pos,RELATION);
 return bioReadByte(file);
}

tUInt16 __FASTCALL__ BMReadWordEx(__fileoff_t pos,int RELATION)
{
 bioSeek(file,pos,RELATION);
 return bioReadWord(file);
}

tUInt32  __FASTCALL__ BMReadDWordEx(__fileoff_t pos,int RELATION)
{
 bioSeek(file,pos,RELATION);
 return bioReadDWord(file);
}

tUInt64  __FASTCALL__ BMReadQWordEx(__fileoff_t pos,int RELATION)
{
 bioSeek(file,pos,RELATION);
 return bioReadQWord(file);
}

tBool __FASTCALL__ BMReadBufferEx(void * buffer,unsigned len,__fileoff_t pos,int RELATION)
{
 bioSeek(file,pos,RELATION);
 return bioReadBuffer(file,buffer,len);
}

tBool __FASTCALL__ BMWriteByte(tUInt8 byte)
{
  return bioWriteByte(file,byte);
}

tBool __FASTCALL__ BMWriteWord(tUInt16 word)
{
  return bioWriteWord(file,word);
}

tBool __FASTCALL__ BMWriteDWord(tUInt32 dword)
{
  return bioWriteDWord(file,dword);
}

tBool __FASTCALL__ BMWriteQWord(tUInt64 dword)
{
  return bioWriteQWord(file,dword);
}

tBool __FASTCALL__ BMWriteBuff(void * buff,unsigned len)
{
  tBool ret;
  ret = bioWriteBuffer(file,buff,len);
  bioFlush(file);
  return ret;
}

tBool __FASTCALL__ BMWriteByteEx(__fileoff_t pos,int RELATION,tUInt8 byte)
{
  bioSeek(file,pos,RELATION);
  return bioWriteByte(file,byte);
}

tBool __FASTCALL__ BMWriteWordEx(__fileoff_t pos,int RELATION,tUInt16 word)
{
  bioSeek(file,pos,RELATION);
  return bioWriteWord(file,word);
}

tBool __FASTCALL__ BMWriteDWordEx(__fileoff_t pos,int RELATION,tUInt32 dword)
{
  bioSeek(file,pos,RELATION);
  return bioWriteDWord(file,dword);
}

tBool __FASTCALL__ BMWriteQWordEx(__fileoff_t pos,int RELATION,tUInt64 dword)
{
  bioSeek(file,pos,RELATION);
  return bioWriteQWord(file,dword);
}

tBool  __FASTCALL__ BMWriteBuffEx(__fileoff_t pos,int RELATION,void * buff,unsigned len)
{
  bioSeek(file,pos,RELATION);
  return bioWriteBuffer(file,buff,len);
}

char * __FASTCALL__ BMName( void )
{
  return bioFileName(file);
}

tBool __FASTCALL__ BMEOF( void )
{
  return bioEOF(file);
}

bhandle_t __FASTCALL__ bmHandle( void )
{
  return bioHandle(sc_file);
}

BGLOBAL __FASTCALL__ bmbioHandle( void )
{
  return sc_file;
}

void __FASTCALL__ bmSeek(__fileoff_t pos,int RELATION)
{
  bioSeek(sc_file,pos,RELATION);
}

void __FASTCALL__ bmReRead( void )
{
  bioReRead(sc_file);
}

tUInt8  __FASTCALL__ bmReadByte( void )
{
  return bioReadByte(sc_file);
}

tUInt16 __FASTCALL__ bmReadWord( void )
{
 return bioReadWord(sc_file);
}

tUInt32  __FASTCALL__ bmReadDWord( void )
{
  return bioReadDWord(sc_file);
}

tUInt64  __FASTCALL__ bmReadQWord( void )
{
  return bioReadQWord(sc_file);
}

__filesize_t  __FASTCALL__ bmGetCurrFilePos( void )
{
  return bioTell(sc_file);
}

__filesize_t __FASTCALL__ bmGetFLength( void )
{
  return bioFLength(sc_file);
}

tBool __FASTCALL__ bmReadBuffer(void * buffer,unsigned len)
{
  return bioReadBuffer(sc_file,buffer,len);
}

tUInt8 __FASTCALL__ bmReadByteEx(__fileoff_t pos,int RELATION)
{
 bioSeek(sc_file,pos,RELATION);
 return bioReadByte(sc_file);
}

tUInt16 __FASTCALL__ bmReadWordEx(__fileoff_t pos,int RELATION)
{
 bioSeek(sc_file,pos,RELATION);
 return bioReadWord(sc_file);
}

tUInt32 __FASTCALL__ bmReadDWordEx(__fileoff_t pos,int RELATION)
{
 bioSeek(sc_file,pos,RELATION);
 return bioReadDWord(sc_file);
}

tUInt64 __FASTCALL__ bmReadQWordEx(__fileoff_t pos,int RELATION)
{
 bioSeek(sc_file,pos,RELATION);
 return bioReadQWord(sc_file);
}

tBool __FASTCALL__ bmReadBufferEx(void * buffer,unsigned len,__fileoff_t pos,int RELATION)
{
 bioSeek(sc_file,pos,RELATION);
 return bioReadBuffer(sc_file,buffer,len);
}

tBool __FASTCALL__ bmWriteByte(tUInt8 byte)
{
  return bioWriteByte(sc_file,byte);
}

tBool __FASTCALL__ bmWriteWord(tUInt16 word)
{
  return bioWriteWord(sc_file,word);
}

tBool __FASTCALL__ bmWriteDWord(tUInt32 dword)
{
  return bioWriteDWord(sc_file,dword);
}

tBool __FASTCALL__ bmWriteQWord(tUInt64 dword)
{
  return bioWriteQWord(sc_file,dword);
}

tBool __FASTCALL__ bmWriteBuff(void * buff,unsigned len)
{
  tBool ret;
  ret = bioWriteBuffer(sc_file,buff,len);
  bioFlush(sc_file);
  return ret;
}

tBool __FASTCALL__ bmWriteByteEx(__fileoff_t pos,int RELATION,tUInt8 byte)
{
  bioSeek(sc_file,pos,RELATION);
  return bioWriteByte(sc_file,byte);
}

tBool __FASTCALL__ bmWriteWordEx(__fileoff_t pos,int RELATION,tUInt16 word)
{
  bioSeek(sc_file,pos,RELATION);
  return bioWriteWord(sc_file,word);
}

tBool __FASTCALL__ bmWriteDWordEx(__fileoff_t pos,int RELATION,tUInt32 dword)
{
  bioSeek(sc_file,pos,RELATION);
  return bioWriteDWord(sc_file,dword);
}

tBool __FASTCALL__ bmWriteQWordEx(__fileoff_t pos,int RELATION,tUInt64 dword)
{
  bioSeek(sc_file,pos,RELATION);
  return bioWriteQWord(sc_file,dword);
}

tBool  __FASTCALL__ bmWriteBuffEx(__fileoff_t pos,int RELATION,void * buff,unsigned len)
{
  bioSeek(sc_file,pos,RELATION);
  return bioWriteBuffer(sc_file,buff,len);
}

char * __FASTCALL__ bmName( void )
{
  return bioFileName(sc_file);
}

tBool __FASTCALL__ bmEOF( void )
{
  return bioEOF(sc_file);
}
