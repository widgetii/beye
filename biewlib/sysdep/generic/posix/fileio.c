/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/generic/posix/fileio.c
 * @brief       Implementation of file i/o routines for POSIX compatible OS.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#ifndef __UTIME_INCLUDED
#include <utime.h>
#endif
#include <sys/stat.h>

#include "biewlib/biewlib.h"

int __FASTCALL__ __OsCreate(const char *name)
{
  return open(name,O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
}

int __FASTCALL__ __OsDelete(const char *name)
{
  return unlink(name);
}

int __FASTCALL__ __OsRename(const char *oldname,const char *newname)
{
  return rename(oldname,newname);
}

void __FASTCALL__ __OsClose(int handle)
{
  close(handle);
}

int __FASTCALL__ __OsDupHandle(int handle)
{
  return dup(handle);
}

int __FASTCALL__ __OsOpen(const char *fname,int mode)
{
  int flags = O_RDONLY;
  struct stat st;

  if((mode & FO_WRITEONLY) == FO_WRITEONLY) flags |= O_WRONLY;
  if((mode & FO_READWRITE) == FO_READWRITE) flags |= O_RDWR;
#ifdef O_SHLOCK
  if(mode & SO_DENYWRITE) flags |= O_SHLOCK;
#endif
#ifdef O_EXLOCK
  if(mode & (SO_DENYALL | SO_DENYREAD)) flags |= O_EXLOCK;
#endif
#ifdef O_BINARY
  flags |= O_BINARY;
#endif

/* check for directory (KB) */

  if (stat(fname,&st)) return -1;
  if (S_ISDIR(st.st_mode)) { errno = EISDIR; return -1; }

  return open(fname,flags);
}

long __FASTCALL__ __OsSeek(int handle,long offset,int origin)
{
  return lseek(handle,offset,origin);
}

int __FASTCALL__ __OsTruncFile( int handle, unsigned long newsize)
{
  return ftruncate(handle,newsize);
}

int __FASTCALL__ __OsRead(int handle, void *buff, unsigned count)
{
  return read(handle,buff,count);
}

int __FASTCALL__ __OsWrite(int handle,const void *buffer, unsigned count)
{
  return write(handle,buffer,count);
}

#define BLKSIZE 32767

int __FASTCALL__ __OsChSize(int handle, long size)
{
    long length, fillsize;
    char *buf;
    int  bufsize, numtowrite;

    length=__OsSeek(handle, 0L, SEEKF_END);	   /* get current length */
    if(length == -1 || size < 0)           return -1;
    if(length >= size)			   /* truncate size */
    {
	return __OsTruncFile(handle,size);
    }
    fillsize=size-length;		    /* increase size **/
    bufsize= (int) min(fillsize, BLKSIZE);
    buf=malloc(bufsize);
    if(buf == NULL) return -1;
    memset(buf, 0, bufsize);		    /* write zeros to pad file */
    do
    {
	numtowrite= (int) min(fillsize, (long) bufsize);
	if(__OsWrite(handle, buf, numtowrite) != numtowrite)
	{
	    free(buf);
	    return -1;
	}
	fillsize-=numtowrite;

    } while(fillsize);
    free(buf);
    return 0;
}

long __FASTCALL__ __FileLength(int handle)
{
  struct stat statbuf;
  int stat_ret;
  long retval;
  memset(&statbuf,0,sizeof(struct stat));
  stat_ret = fstat(handle,&statbuf);
  retval = 0;
  if(stat_ret == 0)
  {
    if((statbuf.st_mode & S_IFDIR) == S_IFDIR) return 0;
    else retval = statbuf.st_size;
  }
  if(!retval)
  {
        unsigned char ch;
        unsigned long min,max,off,i;
	min = 0UL;
	max = LONG_MAX;
        retval = 0UL;
	while(1)
	{
	   off = min + ((max - min) / 2);
	   lseek(handle,off,SEEK_SET);
	   if(read(handle,&ch,1) == 1) min = off;
	   else                        max = off;
	   if(max - min < 5)
	   {
	     retval = min;
	     lseek(handle,retval,SEEK_SET);
             for(i=0;i < 5;i++)
	     {
	        if(read(handle,&ch,1) == 1) retval++;
		else goto loop_end;
		if(retval > LONG_MAX-2) break;
	     }
	     goto loop_end;
           }
	}
	loop_end:;
  }
  return retval;
}

long __FASTCALL__ __OsTell(int handle)
{
  return __OsSeek(handle,0L,SEEKF_CUR);
}

tBool __FASTCALL__ __IsFileExists(const char *name)
{
  return access(name,F_OK) == 0;
}

tBool      __FASTCALL__ __OsGetFTime(const char *name,FTime *data)
{
  tBool ret = False;
  struct stat statbuf;
  if(!stat(name,&statbuf))
  {
    data->acctime = statbuf.st_atime;
    data->modtime = statbuf.st_mtime;
    ret = True;
  }
  return ret;
}

tBool      __FASTCALL__ __OsSetFTime(const char *name,const FTime *data)
{
  struct utimbuf ubuf;
  ubuf.actime = data->acctime;
  ubuf.modtime= data->modtime;
  return utime(name,&ubuf) ? False : True;
}
