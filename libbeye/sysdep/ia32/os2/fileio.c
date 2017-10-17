/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/os2/fileio.c
 * @brief       This file contains implementation of file i/o routines for OS/2-32.
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
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include "biewlib/biewlib.h"

#ifdef __EMX__
extern void _sys_set_errno(unsigned long rc);
#endif

static char f_buff[FILENAME_MAX+1];

static char * __FASTCALL__ fnUnix2Dos(const char *fn)
{
  char *ptr;
  strncpy(f_buff,fn,sizeof(f_buff)-1);
  ptr = f_buff;
  while((ptr = strchr(ptr,'/')) != NULL) *ptr = '\\';
  return f_buff;
}

void __FASTCALL__ __OsClose(bhandle_t handle)
{
  unsigned long rc;
  rc = DosClose(handle);
#ifdef __EMX__
     _sys_set_errno(rc);
#else
     errno = rc;
#endif
}

int __FASTCALL__ __OsDelete(const char *name)
{
  unsigned long rc;
  char *dosname;
  dosname = fnUnix2Dos(name);
  rc = DosDelete((PSZ)dosname);
#ifdef __EMX__
     _sys_set_errno(rc);
#else
     errno = rc;
#endif
  return rc;
}

int __FASTCALL__ __OsRename(const char *oldname,const char *newname)
{
  unsigned long rc;
  char *newdosname;
  char olddosname[FILENAME_MAX+1];
  newdosname = fnUnix2Dos(oldname);
  strncpy(olddosname,newdosname,sizeof(olddosname)-1);
  newdosname = fnUnix2Dos(newname);
  rc = DosMove((PSZ)olddosname,(PSZ)newdosname);
#ifdef __EMX__
     _sys_set_errno(rc);
#else
     errno = rc;
#endif
  return rc;
}

bhandle_t __FASTCALL__ __OsDupHandle(bhandle_t handle)
{
  unsigned long rc;
  HFILE ret;
  ret = -1; /* need by OS/2 SDK */
  rc = DosDupHandle(handle,&ret);
  if(rc) ret = -1;
#ifdef __EMX__
     _sys_set_errno(rc);
#else
     errno = rc;
#endif
  return ret;
}

int __FASTCALL__ __OsChSize(bhandle_t handle, __fileoff_t size)
{
  unsigned long rc;
  rc = DosSetFileSize(handle,size);
#ifdef __EMX__
     _sys_set_errno(rc);
#else
     errno = rc;
#endif
  return rc ? -1 : 0;
}

bhandle_t __FASTCALL__ __OsCreate(const char *fname)
{
   ULONG action,rc;
   char *dosname;
   HFILE fhandle;
   dosname = fnUnix2Dos(fname);
   rc = DosOpen((PSZ)dosname,&fhandle,&action,0L,0,
                OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_FAIL_IF_EXISTS,
                OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYREADWRITE,0L);
   if(rc)
   {
     fhandle = -1;
#ifdef __EMX__
     _sys_set_errno(rc);
#else
     errno = rc;
#endif
   }
   return fhandle;
}

bhandle_t __FASTCALL__ __OsOpen(const char *fname,int mode)
{
   ULONG action,rc;
   char *dosname;
   HFILE fhandle;
   dosname = fnUnix2Dos(fname);
   mode |= OPEN_FLAGS_FAIL_ON_ERROR;
          /* report media i/o error trough retcode but not
             through critical error */
   rc = DosOpen((PSZ)dosname,&fhandle,&action,0L,0,OPEN_ACTION_OPEN_IF_EXISTS,mode,0L);
   if(rc)
   {
     fhandle = -1;
#ifdef __EMX__
     _sys_set_errno(rc);
#else
     errno = rc;
#endif
   }
   return fhandle;
}

__fileoff_t __FASTCALL__ __OsSeek(bhandle_t handle, __fileoff_t offset, int origin)
{
  unsigned long ret,rc;
  rc = DosSetFilePtr(handle,offset,origin,&ret);
  if(rc) ret = -1;
#ifdef __EMX__
     _sys_set_errno(rc);
#else
     errno = rc;
#endif
  return ret;
}

int __FASTCALL__ __OsTruncFile(bhandle_t handle, __filesize_t size)
{
  unsigned long rc;
  rc = DosSetFileSize(handle,size);
#ifdef __EMX__
     _sys_set_errno(rc);
#else
     errno = rc;
#endif
  return rc;
}

int __FASTCALL__ __OsRead(bhandle_t handle, void *buff,unsigned size)
{
  ULONG ret;
  unsigned long rc;
  rc = DosRead(handle,buff,size,&ret);
#ifdef __EMX__
     _sys_set_errno(rc);
#else
     errno = rc;
#endif
  return ret;
}

int __FASTCALL__ __OsWrite(bhandle_t handle,const void *buff,unsigned size)
{
  ULONG ret;
  unsigned long rc;
  rc = DosWrite(handle,(PVOID)buff,size,&ret);
#ifdef __EMX__
     _sys_set_errno(rc);
#else
     errno = rc;
#endif
  return ret;
}

__fileoff_t __FASTCALL__ __FileLength(bhandle_t handle)
{
  long ret,spos;
  spos = __OsTell(handle);
  ret = __OsSeek(handle,0L,SEEKF_END);
  __OsSeek(handle,spos,SEEKF_START);
  return ret;
}

__fileoff_t __FASTCALL__ __OsTell(bhandle_t handle) { return __OsSeek(handle,0L,SEEKF_CUR); }

tBool __FASTCALL__ __IsFileExists(const char *name)
{
   bhandle_t handle = __OsOpen(name,FO_READONLY | SO_DENYNONE);
   if(handle != NULL_HANDLE) __OsClose(handle);
   return handle != NULL_HANDLE;
}

tBool      __FASTCALL__ __OsGetFTime(const char *name,FTime *data)
{
  tBool ret = False;
  bhandle_t handle;
  handle = __OsOpen(name,FO_READONLY);
  if(handle == NULL_HANDLE) handle = __OsOpen(name,FO_READONLY | SO_DENYNONE);
  if(handle != NULL_HANDLE)
  {
    FILESTATUS3 fs;
    struct tm tm;
    unsigned long rc;
    rc = DosQueryFileInfo(handle,FIL_STANDARD,&fs,sizeof(FILESTATUS3));
#ifdef __EMX__
     _sys_set_errno(rc);
#else
     errno = rc;
#endif
    if(!rc)
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
  bhandle_t handle;
  handle = __OsOpen(name,FO_READWRITE);
  if(handle == NULL_HANDLE) handle = __OsOpen(name,FO_READWRITE | SO_DENYNONE);
  if(handle != NULL_HANDLE)
  {
    unsigned long rc;
    FILESTATUS3 fs;
    struct tm *tm;
    time_t tim;
    tim = data->acctime;
    DosQueryFileInfo(handle,FIL_STANDARD,&fs,sizeof(FILESTATUS3));
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
    rc = DosSetFileInfo(handle,FIL_STANDARD,&fs,sizeof(FILESTATUS3));
#ifdef __EMX__
     _sys_set_errno(rc);
#else
     errno = rc;
#endif
    if(!rc) ret = True;
    __OsClose(handle);
  }
  return ret;
}
