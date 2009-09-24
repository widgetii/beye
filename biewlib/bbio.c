/**
 * @namespace   biewlib
 * @file        biewlib/bbio.c
 * @brief       This file contains implementation of Buffering binary input/ouput routines.
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
 * @todo        Support for preemptive memory allocation and multiple buffers for
 *              one opened stream
**/
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "biewlib/bbio.h"
#include "biewlib/pmalloc.h"

#define MK_FPTR(x) (x)

#define IS_CACHE_VALID(obj) ((obj)->b.vfb.MBuffer && !((obj)->optimize & BIO_OPT_NOCACHE))
#define IS_WRITEABLE(openmode) (((openmode) & FO_READWRITE) || ((openmode) & FO_WRITEONLY))

struct tagBFILE bNull =
{
  0L,
  0L,
  NULL,
  0,
  0,
  False,
  False,
  {
   {
    -2,
     0L,
     NULL,
     0,
     0,
     True
   }
  },
  False
};

/* notes: all function with prefix=>__ assume, that buffer present */

#define __isOutOfBuffer(obj,pos)\
        (int)(((__filesize_t)pos < ((BFILE *)obj)->b.vfb.FBufStart ||\
               (__filesize_t)pos >= ((BFILE *)obj)->b.vfb.FBufStart +\
               ((BFILE *)obj)->b.vfb.MBufSize) && !((BFILE *)obj)->is_mmf)

#define __isOutOfContents(obj,pos)\
        (int)(((__filesize_t)pos < ((BFILE *)obj)->b.vfb.FBufStart ||\
               (__filesize_t)pos >= ((BFILE *)obj)->b.vfb.FBufStart +\
               ((BFILE *)obj)->b.vfb.MBufLen) && !((BFILE *)obj)->is_mmf)

#define MMF_HANDLE (-2)

static tBool __NEAR__ __FASTCALL__ __fill(BFILE  *obj,__fileoff_t pos)
{
  void * mbuff;
  __filesize_t remaind;
  tBool ret;
  if(pos < 0) pos = 0;
  if((__filesize_t)pos > obj->FLength)
  {
     pos = obj->FLength;
     obj->b.vfb.MBufLen = 0;
     ret = False;
  }
  else
  {
    obj->b.vfb.FBufStart = pos;
    remaind = obj->FLength - pos;
    obj->b.vfb.MBufLen = (__filesize_t)obj->b.vfb.MBufSize < remaind ?
                                     obj->b.vfb.MBufSize : (unsigned)remaind;
    mbuff = MK_FPTR(obj->b.vfb.MBuffer);
    __OsSeek(obj->b.vfb.handle,pos,SEEKF_START);
    ret = (unsigned)__OsRead(obj->b.vfb.handle,mbuff,obj->b.vfb.MBufLen) == obj->b.vfb.MBufLen;
  }
  return ret;
}

static tBool __NEAR__ __FASTCALL__ __flush(BFILE  *obj)
{
  void * mbuff;
  tBool ret;
  ret = True;
  if(!obj->b.vfb.updated)
  {
    mbuff = MK_FPTR(obj->b.vfb.MBuffer);
    __OsSeek(obj->b.vfb.handle,obj->b.vfb.FBufStart,SEEKF_START);
    if(obj->b.vfb.MBufLen) ret = (unsigned)__OsWrite(obj->b.vfb.handle,mbuff,obj->b.vfb.MBufLen) == obj->b.vfb.MBufLen;
    if(ret)                obj->b.vfb.updated = True;
  }
  return ret;
}

#define CHK_EOF(obj,pos)\
{\
 ((BFILE *)obj)->is_eof = False;\
  /* Accessing memory after mmf[FLength-1] causes GPF in MMF mode */\
  /* so we must add special checks for it but no for read-write mode */\
  /* Special case: FLength == 0. When file is being created pos == FLength.*/\
 if(obj->FLength && !IS_WRITEABLE(obj->openmode)\
    && (__filesize_t)pos >= (__filesize_t)obj->FLength)\
 {\
    pos = ((BFILE *)obj)->FLength-1;\
    ret = False;\
    ((BFILE *)obj)->is_eof = True;\
 }\
}

#define SEEK_FPTR(ret,obj,pos,origin)\
{\
 ret = True;\
 switch((int)origin)\
 {\
    case BIO_SEEK_SET: break;\
    case BIO_SEEK_CUR: pos += ((BFILE *)obj)->FilePos; break;\
    default:           pos += ((BFILE *)obj)->FLength;\
 }\
 CHK_EOF(obj,pos)\
}

static tBool __NEAR__ __FASTCALL__ __seek(BFILE  *obj,__fileoff_t pos,int origin)
{
 tBool ret,rret;
 SEEK_FPTR(ret,obj,pos,origin);
 obj->FilePos = pos;
 if(__isOutOfBuffer(obj,pos))
 {
    __flush(obj);
    switch(obj->optimize & BIO_OPT_DIRMASK)
    {
      default:
      case BIO_OPT_DB:         break;
      case BIO_OPT_RANDOM:     pos -= obj->b.vfb.MBufSize / 2;
                               break;
      case BIO_OPT_BACKSCAN:   pos -= (obj->b.vfb.MBufSize - 1);
                               break;
      case BIO_OPT_RFORWARD:   pos -= obj->b.vfb.MBufSize / 10;
                               break;
      case BIO_OPT_RBACKSCAN:  pos -= (obj->b.vfb.MBufSize/10)*9;
                               break;
    }
    if(obj->FilePos < obj->FLength)
    {
      rret = __fill(obj,pos);
      if(ret) ret = rret;
    }
    else
    {
      obj->b.vfb.FBufStart = obj->FilePos;
      obj->b.vfb.MBufLen = 0;
    }
 }
 return ret;
}

static tBool founderr;

static unsigned char __NEAR__ __FASTCALL__ __getc(BFILE  *obj)
{
  char * buff = MK_FPTR(obj->b.vfb.MBuffer);
  unsigned buffpos;
  tBool ret;
  unsigned char ch;
  ret = True;
  founderr = False;
  if(__isOutOfContents(obj,obj->FilePos)) ret = __seek(obj,obj->FilePos,SEEKF_START);
  ch = -1;
  if(obj->b.vfb.MBufLen && ret && obj->FilePos <= obj->FLength)
  {
    buffpos = (unsigned)(obj->FilePos - obj->b.vfb.FBufStart);
    if(buffpos < obj->b.vfb.MBufLen)  ch = buff[buffpos];
    if(obj->FilePos < obj->FLength) obj->FilePos++;
    CHK_EOF(obj,obj->FilePos);
  }
  else 
  {
    errno = EACCES;
    ret = False; 
  }
  if(ret == False) founderr = True;
  return ch;
}

static tBool __NEAR__ __FASTCALL__ __putc(BFILE  *obj,unsigned char ch)
{
  char *  buff;
  unsigned buffpos;
  tBool ret;
  if(IS_WRITEABLE(obj->openmode))
  {
    ret = True;
    buff = MK_FPTR(obj->b.vfb.MBuffer);
    if(__isOutOfBuffer(obj,obj->FilePos)) __seek(obj,obj->FilePos,SEEKF_START);
    buffpos = (unsigned)(obj->FilePos - obj->b.vfb.FBufStart);
    buff[buffpos++] = ch;
    obj->b.vfb.updated = False;
    if(obj->b.vfb.MBufLen < buffpos)
      if(obj->b.vfb.MBufLen < obj->b.vfb.MBufSize) obj->b.vfb.MBufLen = buffpos;
    if(obj->FLength <= obj->b.vfb.FBufStart + buffpos)
      obj->FLength = obj->b.vfb.FBufStart + buffpos;
    obj->FilePos++;
  }
  else { errno = EACCES; ret = False; }
  return ret;
}

static tBool __NEAR__ __FASTCALL__ __getbuff(BFILE  *obj,char * buff,unsigned cbBuff)
{
  unsigned diffsize;
  unsigned MBufStart,MBufRem;
  int optimize = obj->optimize;
  tBool ret = True;
  if(obj->is_mmf)
  {
    tUInt32 size = min(cbBuff,obj->FLength - obj->FilePos);
    CHK_EOF(obj,obj->FilePos);
    if(size && ret)
    {
      memcpy(buff,((tUInt8 *)obj->b.mmb->mmf_addr) + obj->FilePos,size);
      obj->FilePos += size;
    }
  }
  else
  {
    obj->optimize = (optimize & ~BIO_OPT_DIRMASK) | BIO_OPT_DB;
    while(cbBuff)
    {
      MBufStart = (unsigned)(obj->FilePos - obj->b.vfb.FBufStart);
      MBufRem = obj->b.vfb.MBufLen - MBufStart;
      if(!MBufRem || __isOutOfContents(obj,obj->FilePos))
      {
       ret = __seek(obj,obj->FilePos,SEEKF_START);
       if(!ret) break;
       if(obj->FilePos >= obj->FLength) break;
       MBufStart = (unsigned)(obj->FilePos - obj->b.vfb.FBufStart);
       MBufRem = obj->b.vfb.MBufLen - MBufStart;
      }
      if(!MBufRem)
      {
         errno = E2BIG; /* fault: internal error */
         ret = False;
         break;
      }
      diffsize = min(MBufRem,cbBuff);
      memcpy(buff,&obj->b.vfb.MBuffer[MBufStart],diffsize);
      obj->FilePos += diffsize;
      if(obj->FilePos > obj->FLength) obj->FilePos = obj->FLength;
      cbBuff -= diffsize;
      buff += diffsize;
    }
    obj->optimize = optimize;
  }
  return ret;
}

static tBool __NEAR__ __FASTCALL__ __putbuff(BFILE  *obj,const char * buff,unsigned cbBuff)
{
  unsigned diffsize;
  unsigned MBufStart,MBufRem;
  int optimize = obj->optimize;
  tBool ret = True;
  if(IS_WRITEABLE(obj->openmode))
  {
    if(obj->is_mmf)
    {
      tUInt32 size = min(cbBuff,obj->FLength - obj->FilePos);
      if(size)
      {
        memcpy(((tUInt8 *)obj->b.mmb->mmf_addr) + obj->FilePos,buff,size);
        obj->FilePos += size;
      }
    }
    else
    {
      obj->optimize = (optimize & ~BIO_OPT_DIRMASK) | BIO_OPT_DB;
      ret = True;
      while(cbBuff)
      {
        MBufStart = (unsigned)(obj->FilePos - obj->b.vfb.FBufStart);
        MBufRem = obj->b.vfb.MBufSize - MBufStart;
        if(!MBufRem || __isOutOfBuffer(obj,obj->FilePos))
        {
         ret = __seek(obj,obj->FilePos,SEEKF_START);
         if(!ret) break;
         MBufStart = (unsigned)(obj->FilePos - obj->b.vfb.FBufStart);
         MBufRem = obj->b.vfb.MBufSize - MBufStart;
        }
        if(!MBufRem)
        {
           errno = E2BIG; /* fault: internal error */
           ret = False;
           break;
        }
        diffsize = min(MBufRem,cbBuff);
        memcpy(&obj->b.vfb.MBuffer[MBufStart],buff,diffsize);
        obj->b.vfb.updated = False;
        if(obj->b.vfb.MBufLen < MBufStart + diffsize) obj->b.vfb.MBufLen = MBufStart + diffsize;
        if(obj->FLength < obj->FilePos + diffsize) obj->FLength = obj->FilePos + diffsize;
        obj->FilePos += diffsize;
        cbBuff -= diffsize;
        buff += diffsize;
      }
      obj->optimize = optimize;
    }
  }
  else { errno = EACCES; ret = False; }
  return ret;
}

BGLOBAL  __FASTCALL__ bioOpen(const char * fname,unsigned openmode,unsigned bSize,unsigned optimization)
{
 BFILE  * bFile;
 BGLOBAL ret = NULL;
 unsigned len;
 ret = PMalloc(sizeof(BFILE));
 if(ret)
 {
   bFile = MK_FPTR(ret);
   bFile->FilePos = 0;
   bFile->is_eof = False;
   bFile->openmode = openmode;
   if(!(bFile->FileName = PMalloc(strlen(fname)+1)))
   {
     PFREE(bFile);
     return &bNull;
   }
   strcpy(bFile->FileName,fname);
   bFile->is_mmf = False;
   /* Attempt open as MMF */
   if(!IS_WRITEABLE(openmode) && optimization == BIO_OPT_USEMMF)
   {
      if((bFile->b.mmb = PMalloc(sizeof(mmb))))
      {
        if((bFile->b.mmb->mmf = __mmfOpen(fname,openmode)) != NULL)
        {
          bFile->b.mmb->mmf_addr = __mmfAddress(bFile->b.mmb->mmf);
          bFile->FLength = __mmfSize(bFile->b.mmb->mmf);
          bFile->primary_mmf = True;
          bFile->is_mmf = True;
	  bFile->b.vfb.MBuffer = NULL; /* [dBorca] be consistent with IS_CACHE_VALID */
        }
        else PFREE(bFile->b.mmb);
      }
   }
   if(!bFile->is_mmf)
   {
     bhandle_t handle = __OsOpen(fname,openmode);
     optimization = BIO_OPT_DB;
     if(handle == -1)
     {
       PFREE(bFile->FileName);
       PFREE(ret);
       return &bNull;
     }
     bFile->b.vfb.handle = handle;
     bFile->optimize = optimization;
     bFile->b.vfb.FBufStart = 0L;
     bFile->b.vfb.updated  = True;
     bFile->is_mmf = False;
     bFile->FLength = __FileLength(bFile->b.vfb.handle);
     len = bSize;
     if(bSize == UINT_MAX) len = (unsigned)bFile->FLength;
     if(len)
     {
       if((bFile->b.vfb.MBuffer = PMalloc(len)) != NULL)
       {
         bFile->b.vfb.MBufLen = 0;
         bFile->b.vfb.MBufSize = len;
       }
       else
       {
         PFREE(bFile->FileName);
         __OsClose(bFile->b.vfb.handle);
         PFREE(ret);
         return &bNull;
       }
       __fill(bFile,0L);
     }
     else bFile->b.vfb.MBuffer = NULL;
   }
 }
 else ret = &bNull;
 return ret;
}

tBool  __FASTCALL__ bioClose(BGLOBAL handle)
{
  BFILE * bFile = MK_FPTR(handle);
  if(bFile->is_mmf)
  {
    if(bFile->primary_mmf)
    {
      __mmfClose(bFile->b.mmb->mmf);
      PFREE(bFile->b.mmb);
    }
  }
  else
  {
    if(bFile->b.vfb.handle != -1)
    {
      if(IS_WRITEABLE(bFile->openmode)) __flush(bFile);
      if(bFile->b.vfb.handle != 2) __OsClose(bFile->b.vfb.handle);
    }
    if(bFile->b.vfb.MBuffer) PFREE(bFile->b.vfb.MBuffer);
  }
  PFREE(bFile->FileName);
  PFREE(handle);
  return True;
}

tBool   __FASTCALL__ bioSeek(BGLOBAL bioFile,__fileoff_t pos,int orig)
{
 BFILE  *obj =MK_FPTR(bioFile);
 tBool ret;
 if(IS_CACHE_VALID(obj)) ret = __seek(obj,pos,orig);
 else
 {
   if(obj->is_mmf)
   {
     SEEK_FPTR(ret,obj,pos,orig);
     obj->FilePos = pos;
   }
   else
   {
     ret = __OsSeek(obj->b.vfb.handle,pos,orig) != 0;
     if(pos == 0L && orig == SEEKF_START) ret = True;
   }
 }
 return ret;
}

__filesize_t  __FASTCALL__ bioTell(BGLOBAL bioFile)
{
  BFILE  *obj = MK_FPTR(bioFile);
  return (IS_CACHE_VALID(obj) || obj->is_mmf) ? obj->FilePos : (__filesize_t)__OsTell(obj->b.vfb.handle);
}

tUInt8 __FASTCALL__ bioReadByte(BGLOBAL bioFile)
{
  BFILE  *obj = MK_FPTR(bioFile);
  tUInt8 ret;
  if(IS_CACHE_VALID(obj) && !obj->is_mmf) ret = __getc(obj);
  else
    if(obj->is_mmf)
    {
      tUInt8 rval;
      rval = ret = ((tUInt8 *)obj->b.mmb->mmf_addr)[obj->FilePos];
      if(obj->FilePos < obj->FLength) obj->FilePos++;
      CHK_EOF(obj,obj->FilePos);
      ret = rval;
    }
    else
      if(__OsRead(obj->b.vfb.handle,&ret,sizeof(tUInt8)) != sizeof(tUInt8)) ret = -1;
  return ret;
}

tUInt16 __FASTCALL__ bioReadWord(BGLOBAL bioFile)
{
  BFILE  *obj = MK_FPTR(bioFile);
  tUInt16 ret;
  if(IS_CACHE_VALID(obj) || obj->is_mmf)
  {
    if(!__getbuff(obj,(void *)&ret,sizeof(tUInt16))) ret = -1;
  }
  else
    if(__OsRead(obj->b.vfb.handle,&ret,sizeof(tUInt16)) != sizeof(tUInt16)) ret = -1;
  return ret;
}

tUInt32 __FASTCALL__ bioReadDWord(BGLOBAL bioFile)
{
  BFILE  *obj = MK_FPTR(bioFile);
  tUInt32 ret;
  if(IS_CACHE_VALID(obj) || obj->is_mmf)
  {
    if(!__getbuff(obj,(void *)&ret,sizeof(tUInt32))) ret = -1;
  }
  else
    if(__OsRead(obj->b.vfb.handle,&ret,sizeof(tUInt32)) != sizeof(tUInt32)) ret = -1;
  return ret;
}

tUInt64 __FASTCALL__ bioReadQWord(BGLOBAL bioFile)
{
  BFILE  *obj = MK_FPTR(bioFile);
  tUInt64 ret;
  if(IS_CACHE_VALID(obj) || obj->is_mmf)
  {
    if(!__getbuff(obj,(void *)&ret,sizeof(tUInt64))) ret = -1;
  }
  else
    if(__OsRead(obj->b.vfb.handle,&ret,sizeof(tUInt64)) != sizeof(tUInt64)) ret = -1;
  return ret;
}

tBool __FASTCALL__  bioReadBuffer(BGLOBAL bioFile,void * buffer,unsigned cbBuffer)
{
  BFILE  *obj = MK_FPTR(bioFile);
  return IS_CACHE_VALID(obj) || obj->is_mmf ?
            __getbuff(obj,buffer,cbBuffer) :
            ((unsigned)__OsRead(obj->b.vfb.handle,buffer,cbBuffer)) == cbBuffer;
}

tBool __FASTCALL__  bioWriteByte(BGLOBAL bioFile,tUInt8 bVal)
{
  BFILE  *obj = MK_FPTR(bioFile);
  tBool ret = True;
  if(IS_CACHE_VALID(obj)) ret = __putc(obj,bVal);
  else
    ret = __OsWrite(obj->b.vfb.handle,&bVal,sizeof(tUInt8)) == sizeof(tUInt8);
  return ret;
}

tBool __FASTCALL__  bioWriteWord(BGLOBAL bioFile,tUInt16 wVal)
{
  BFILE  *obj = MK_FPTR(bioFile);
  return IS_CACHE_VALID(obj) ?
             __putbuff(obj,(void *)&wVal,sizeof(tUInt16)) :
             __OsWrite(obj->b.vfb.handle,&wVal,sizeof(tUInt16)) == sizeof(tUInt16);
}

tBool __FASTCALL__  bioWriteDWord(BGLOBAL bioFile,tUInt32 dwVal)
{
  BFILE  *obj = MK_FPTR(bioFile);
  return IS_CACHE_VALID(obj) ?
             __putbuff(obj,(void *)&dwVal,sizeof(tUInt32)) :
             __OsWrite(obj->b.vfb.handle,&dwVal,sizeof(tUInt32)) == sizeof(tUInt32);
}

tBool __FASTCALL__  bioWriteQWord(BGLOBAL bioFile,tUInt64 dwVal)
{
  BFILE  *obj = MK_FPTR(bioFile);
  return IS_CACHE_VALID(obj) ?
             __putbuff(obj,(void *)&dwVal,sizeof(tUInt64)) :
             __OsWrite(obj->b.vfb.handle,&dwVal,sizeof(tUInt64)) == sizeof(tUInt64);
}

tBool __FASTCALL__  bioWriteBuffer(BGLOBAL bioFile,const void * buffer,unsigned cbBuffer)
{
  BFILE  *obj = MK_FPTR(bioFile);
  return IS_CACHE_VALID(obj) ?
             __putbuff(obj,buffer,cbBuffer) :
             ((unsigned)__OsWrite(obj->b.vfb.handle,buffer,cbBuffer)) == cbBuffer;
}

tBool __FASTCALL__  bioFlush(BGLOBAL bioFile)
{
  BFILE  * obj = MK_FPTR(bioFile);
  return IS_CACHE_VALID(obj) ? __flush(obj) : obj->is_mmf ? __mmfFlush(obj->b.mmb->mmf) : True;
}

tBool __FASTCALL__  bioReRead(BGLOBAL bioFile)
{
  __filesize_t fpos;
  BFILE  * obj = MK_FPTR(bioFile);
  tBool ret = True;
  if(obj->is_mmf)
  {
    obj->b.mmb->mmf = __mmfSync(obj->b.mmb->mmf);
    if(obj->b.mmb->mmf)
    {
      obj->b.mmb->mmf_addr = __mmfAddress(obj->b.mmb->mmf);
      obj->FLength  = __mmfSize(obj->b.mmb->mmf);
    }
    /** @todo Most reliable code */
    else { printm("Internal error occured in __mmfSync\n"); exit(EXIT_FAILURE); }
  }
  else
  {
    fpos = __OsTell(obj->b.vfb.handle);
    __OsSeek(obj->b.vfb.handle,0L,SEEKF_END);
    obj->FLength = __OsTell(obj->b.vfb.handle);
    __OsSeek(obj->b.vfb.handle,fpos,SEEKF_START);
    ret = __fill(obj,obj->b.vfb.FBufStart);
  }
  return ret;
}

__filesize_t  __FASTCALL__  bioFLength(BGLOBAL bioFile)
{
  BFILE  * bFile = MK_FPTR(bioFile);
  return bFile->FLength;
}

tBool __FASTCALL__  bioChSize(BGLOBAL bioFile,__filesize_t newsize)
{
    __filesize_t length, fillsize;
    char * buf;
    BFILE  *obj = MK_FPTR(bioFile);
    unsigned  bufsize, numtowrite;
    tBool ret;
    if(obj->is_mmf)
    {
      if((ret=__mmfResize(obj->b.mmb->mmf,newsize)))
      {
         obj->b.mmb->mmf_addr = __mmfAddress(obj->b.mmb->mmf);
         obj->FLength  = __mmfSize(obj->b.mmb->mmf);
      }
    }
    else
    {
      length = obj->FLength;
      if(length >= newsize) /* truncate size */
      {
        __seek(obj, newsize, SEEKF_START);
        length = newsize;
        ret = __OsTruncFile(obj->b.vfb.handle, length) != 0;
        obj->FLength = newsize;
        if(obj->b.vfb.FBufStart > obj->FLength) { obj->b.vfb.FBufStart = obj->FLength; obj->b.vfb.MBufLen = 0; }
        if(obj->b.vfb.FBufStart + obj->b.vfb.MBufLen > obj->FLength) obj->b.vfb.MBufLen = (unsigned)(obj->FLength - obj->b.vfb.FBufStart);
        ret = ret ? False : True;
      }
      else
      {
        fillsize=newsize-length;  /* increase size */
        ret = False;
        bufsize = obj->b.vfb.MBuffer ? obj->b.vfb.MBufSize : 8192;
        bufsize = (unsigned) min(fillsize,bufsize);
        if((buf = PMalloc(bufsize)) != NULL)
        {
          ret = True;
          memset(buf, 0, bufsize);   /* write zeros to pad file */
          bioSeek(bioFile,0L,SEEKF_END);
          do
          {
            numtowrite = (unsigned)min(bufsize,fillsize);
            if(!bioWriteBuffer(bioFile, (void * )buf, numtowrite)) { ret = False; break; }
            fillsize-=numtowrite;
          } while(fillsize);
          PFREE(buf);
        }
      }
   }
   return ret;
}

unsigned  __FASTCALL__ bioSetOptimization(BGLOBAL bioFile,unsigned flags)
{
  BFILE  *obj = MK_FPTR(bioFile);
  unsigned ret;
  ret = obj->optimize;
  obj->optimize = flags;
  return ret;
  return flags;
}

unsigned  __FASTCALL__ bioGetOptimization(BGLOBAL bioFile)
{
  BFILE  *obj = MK_FPTR(bioFile);
  return obj->optimize;
}

int  __FASTCALL__ bioHandle(BGLOBAL bioFile)
{
   BFILE *obj = MK_FPTR(bioFile);
   return obj->is_mmf ? MMF_HANDLE : obj->b.vfb.handle;
}

char * __FASTCALL__ bioFileName(BGLOBAL bioFile)
{
  BFILE *obj = MK_FPTR(bioFile);
  return obj->FileName;
}

void * __FASTCALL__ bioBuffer(BGLOBAL bioFile)
{
  BFILE *obj = MK_FPTR(bioFile);
  return obj->is_mmf ? obj->b.mmb->mmf_addr : obj->b.vfb.MBuffer;
}

unsigned __FASTCALL__ bioBuffLen(BGLOBAL bioFile)
{
  BFILE *obj = MK_FPTR(bioFile);
  return obj->is_mmf ? obj->FLength : obj->b.vfb.MBufLen;
}

unsigned __FASTCALL__ bioBuffPos(BGLOBAL bioFile)
{
  BFILE *obj = MK_FPTR(bioFile);
  return obj->is_mmf ? 0L : obj->b.vfb.MBufLen - (unsigned)(obj->FilePos - obj->b.vfb.FBufStart);
}

BGLOBAL __FASTCALL__ bioDupEx(BGLOBAL bioFile,unsigned buff_Size)
{
 BGLOBAL ret = NULL;
 unsigned len;
 ret = PMalloc(sizeof(BFILE));
 if(ret)
 {
   BFILE * bFile,* fromFile;
   bFile = MK_FPTR(ret);
   fromFile = MK_FPTR(bioFile);
   bFile->openmode = fromFile->openmode;
   bFile->optimize = fromFile->optimize;
   bFile->is_eof = fromFile->is_eof;
   if(!(bFile->FileName = PMalloc(strlen(fromFile->FileName)+1)))
   {
     PFREE(bFile);
     return &bNull;
   }
   strcpy(bFile->FileName,fromFile->FileName);

   if(fromFile->is_mmf)
   {
     bFile->b.mmb = fromFile->b.mmb;
     bFile->FilePos = fromFile->FilePos;
     bFile->primary_mmf = False;
   }
   else
   {
     bFile->b.vfb.handle = __OsDupHandle(fromFile->b.vfb.handle);
     bFile->b.vfb.updated  = fromFile->b.vfb.updated;
   }
   bFile->is_mmf = fromFile->is_mmf;
   if(bFile->b.vfb.handle == -1)
   {
     PFREE(bFile->FileName);
     PFREE(ret);
     return &bNull;
   }
   bFile->FLength = fromFile->FLength;
   len = buff_Size;
   if(len && !bFile->is_mmf)
   {
     if((bFile->b.vfb.MBuffer = PMalloc(len)) != NULL)
     {
       if(IS_CACHE_VALID(fromFile))
       {
         bFile->b.vfb.MBufLen = min(len,fromFile->b.vfb.MBufLen);
         bFile->b.vfb.MBufSize = len;
         bFile->FilePos = min(fromFile->FilePos,fromFile->b.vfb.FBufStart+(len/2));
         bFile->b.vfb.FBufStart = fromFile->b.vfb.FBufStart;
         memcpy(bFile->b.vfb.MBuffer,fromFile->b.vfb.MBuffer,bFile->b.vfb.MBufLen);
       }
       else
       {
         bFile->b.vfb.MBufLen = 0;
         bFile->b.vfb.MBufSize = 0;
         bFile->FilePos = 0L;
         bFile->b.vfb.FBufStart = 0;
       }
     }
     else
     {
       PFREE(bFile->FileName);
       __OsClose(bFile->b.vfb.handle);
       PFREE(ret);
       return &bNull;
     }
   }
   else bFile->b.vfb.MBuffer = 0;
 }
 else ret = &bNull;
 return ret;
}

BGLOBAL __FASTCALL__ bioDup(BGLOBAL bHandle)
{
  BFILE *fromFile = MK_FPTR(bHandle);
  return bioDupEx(bHandle,fromFile->b.vfb.MBufSize);
}

tBool __FASTCALL__ bioEOF(BGLOBAL bHandle)
{
  BFILE  *obj = MK_FPTR(bHandle);
  tBool retval;
  if(IS_CACHE_VALID(obj) || obj->is_mmf) retval = obj->is_eof;
  else                                   retval = (__filesize_t)__OsTell(obj->b.vfb.handle) >= obj->FLength;
  return retval;
}
