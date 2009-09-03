/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/os2/fileio.c
 * @brief       This file contains implementation of file i/o routines for OS/2.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <os2.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#include "biewlib/biewlib.h"

static char f_buff[FILENAME_MAX+1];

static char * __NEAR__ __FASTCALL__ fnUnix2Dos(const char *fn)
{
  char *ptr;
  strncpy(f_buff,fn,sizeof(f_buff)-1);
  ptr = f_buff;
  while((ptr = strchr(ptr,'/')) != NULL) *ptr = '\\';
  return f_buff;
}

void __FASTCALL__ __OsClose(int handle)
{
  errno = DosClose(handle);
}

int __FASTCALL__ __OsDelete(const char *name)
{
  char *dosname;
  dosname = fnUnix2Dos(name);
  return errno = DosDelete((PSZ)dosname,0);
}

int __FASTCALL__ __OsRename(const char *oldname,const char *newname)
{
  char olddosname[FILENAME_MAX+1],*newdosname;
  newdosname = fnUnix2Dos(oldname);
  strncpy(olddosname,newdosname,sizeof(olddosname)-1);
  newdosname = fnUnix2Dos(newname);
  return errno = DosMove((PSZ)olddosname,(PSZ)newdosname,0);
}

int  __FASTCALL__ __OsDupHandle(int handle)
{
  USHORT ret;
  ret = -1; /* need by OS/2 SDK */
  errno = DosDupHandle(handle,&ret);
  if(errno) ret = -1;
  return ret;
}

int __FASTCALL__ __OsChSize(int handle, __fileoff_t size)
{
  errno = DosNewSize(handle,size);
  return errno ? -1 : 0;
}

int  __FASTCALL__ __OsCreate(const char *fname)
{
   char *dosname;
   USHORT fhandle,action,rc;
   dosname = fnUnix2Dos(fname);
   rc = DosOpen((PSZ)dosname,&fhandle,&action,0L,0,
                OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_FAIL_IF_EXISTS,
                OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYREADWRITE,0L);
   if(rc) { fhandle = -1; errno = rc; }
   return fhandle;
}

int  __FASTCALL__ __OsOpen(const char *fname,int mode)
{
   char *dosname;
   USHORT fhandle,action,rc;
   dosname = fnUnix2Dos(fname);
   mode |= OPEN_FLAGS_FAIL_ON_ERROR;
           /* report media i/o error trough retcode but not
              through critical error */
   rc = DosOpen((PSZ)dosname,&fhandle,&action,0L,0,
                OPEN_ACTION_OPEN_IF_EXISTS,mode,0L);
   if(rc) { fhandle = -1; errno = rc; }
   return fhandle;
}

__fileoff_t __FASTCALL__ __OsSeek( int handle, __fileoff_t offset, int origin)
{
  unsigned long ret;
  errno = DosChgFilePtr(handle,offset,origin,&ret);
  if(errno) ret = -1;
  return ret;
}

int __FASTCALL__ __OsTruncFile( int handle, __filesize_t size)
{
  return DosNewSize(handle,size);
}

int __FASTCALL__ __OsRead(int handle, void *buff,unsigned size)
{
  USHORT ret;
  errno = DosRead(handle,buff,size,&ret);
  return ret;
}

int __FASTCALL__ __OsWrite(int handle,const void *buff,unsigned size)
{
  USHORT ret;
  errno = DosWrite(handle,buff,size,&ret);
  return ret;
}

__fileoff_t __FASTCALL__ __FileLength(int handle)
{
  long ret,spos;
  spos = __OsTell(handle);
  ret = __OsSeek(handle,0L,SEEKF_END);
  __OsSeek(handle,spos,SEEKF_START);
  return ret;
}

__fileoff_t __FASTCALL__ __OsTell(int handle) { return __OsSeek(handle,0L,SEEKF_CUR); }

tBool __FASTCALL__ __IsFileExists(const char *name)
{
   int handle = __OsOpen(name,FO_READONLY | SO_DENYNONE);
   if(handle != -1) __OsClose(handle);
   return handle != -1;
}

tBool      __FASTCALL__ __OsGetFTime(const char *name,FTime *data)
{
  tBool ret = False;
  int handle;
  handle = __OsOpen(name,FO_READONLY);
  if(handle == -1) handle = __OsOpen(name,FO_READONLY | SO_DENYNONE);
  if(handle != -1)
  {
    FILESTATUS fs;
    struct tm tm;
    errno = DosQFileInfo(handle,FIL_STANDARD,&fs,sizeof(FILESTATUS));
    if(!errno)
    {
      tm.tm_sec  = fs.ftimeLastAccess.twosecs*2;
      tm.tm_min  = fs.ftimeLastAccess.minutes;
      tm.tm_hour = fs.ftimeLastAccess.hours;
      tm.tm_mday = fs.fdateLastAccess.day;
      tm.tm_mon  = fs.fdateLastAccess.month-1;
      tm.tm_year = fs.fdateLastAccess.year+80;
      data->acctime = mktime(&tm);
      tm.tm_sec  = fs.ftimeLastWrite.twosecs*2;
      tm.tm_min  = fs.ftimeLastWrite.minutes;
      tm.tm_hour = fs.ftimeLastWrite.hours;
      tm.tm_mday = fs.fdateLastWrite.day;
      tm.tm_mon  = fs.fdateLastWrite.month-1;
      tm.tm_year = fs.fdateLastWrite.year+80;
      data->modtime = mktime(&tm);
      ret = True;
    }
    __OsClose(handle);
  }
  return ret;
}

tBool      __FASTCALL__ __OsSetFTime(const char *name,const FTime *data)
{
  tBool ret = False;
  int handle;
  handle = __OsOpen(name,FO_READWRITE);
  if(handle == -1) handle = __OsOpen(name,FO_READWRITE | SO_DENYNONE);
  if(handle != -1)
  {
    FILESTATUS fs;
    struct tm *tm;
    time_t tim;
    tim = data->acctime;
    DosQFileInfo(handle,FIL_STANDARD,&fs,sizeof(FILESTATUS));
    tm = localtime(&tim);
    fs.ftimeLastAccess.twosecs = tm->tm_sec/2;
    fs.ftimeLastAccess.minutes = tm->tm_min;
    fs.ftimeLastAccess.hours = tm->tm_hour;
    fs.fdateLastAccess.day = tm->tm_mday;
    fs.fdateLastAccess.month = tm->tm_mon+1;
    fs.fdateLastAccess.year = tm->tm_year-80;
    tim = data->modtime;
    tm = localtime(&tim);
    fs.ftimeLastWrite.twosecs = tm->tm_sec/2;
    fs.ftimeLastWrite.minutes = tm->tm_min;
    fs.ftimeLastWrite.hours = tm->tm_hour;
    fs.fdateLastWrite.day = tm->tm_mday;
    fs.fdateLastWrite.month = tm->tm_mon+1;
    fs.fdateLastWrite.year = tm->tm_year-80;
    errno = DosSetFileInfo(handle,FIL_STANDARD,&fs,sizeof(FILESTATUS));
    if(!errno) ret = True;
    __OsClose(handle);
  }
  return ret;
}
