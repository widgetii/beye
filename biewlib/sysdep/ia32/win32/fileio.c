/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/win32/fileio.c
 * @brief       This file contains implementation of file i/o routines for Win32s.
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
 * @warning     WinNT may "die", when performs long file query in network.
**/
/* for cygwin - remove unnecessary includes */
#define _OLE_H
#define _OLE2_H
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include "biewlib/biewlib.h"

static char f_buff[FILENAME_MAX+1];

static void __FASTCALL__ set_errno(int code)
{
  /* Since in win32 used
     _errno()
         instead
     int errno
     we must call it after GetLastError, because it rturn pointer
     to memory, that can change during GetLastError execution.
   */
   errno = code;
}

static char * __FASTCALL__ fnUnix2Dos(const char *fn)
{
  char *ptr;
  strncpy(f_buff,fn,sizeof(f_buff)-1);
  ptr = f_buff;
  while((ptr = strchr(ptr,'/')) != NULL) *ptr = '\\';
  return f_buff;
}

bhandle_t __FASTCALL__ __OsCreate(const char *name)
{
  char *dosname;
  HANDLE handle;
  dosname = fnUnix2Dos(name);
  handle = CreateFile(dosname,
                      GENERIC_WRITE | GENERIC_READ,
                      0,
                      NULL,
                      CREATE_NEW,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);
  if(handle == INVALID_HANDLE_VALUE)
  {
    set_errno(GetLastError());
    handle = (HANDLE)-1;
  }
  return (bhandle_t)handle;
}

int __FASTCALL__ __OsDelete(const char *name)
{
  char *dosname;
  dosname = fnUnix2Dos(name);
  if(!DeleteFile(dosname))
  {
    set_errno(GetLastError());
    return -1;
  }
  else return 0;
}

int __FASTCALL__ __OsRename(const char *oldname,const char *newname)
{
  char *newdosname;
  char olddosname[FILENAME_MAX+1];
  newdosname = fnUnix2Dos(oldname);
  strncpy(olddosname,newdosname,sizeof(olddosname)-1);
  newdosname = fnUnix2Dos(newname);
  if(!MoveFile(olddosname,newdosname))
  {
    set_errno(GetLastError());
    return -1;
  }
  else return 0;
}

void __FASTCALL__ __OsClose(bhandle_t handle)
{
  CloseHandle((HANDLE)handle);
}

bhandle_t __FASTCALL__ __OsDupHandle(bhandle_t handle)
{
  HANDLE hret;
  if(DuplicateHandle(GetCurrentProcess(),
                     (HANDLE)handle,
                     GetCurrentProcess(),
                     &hret,
                     FILE_MAP_ALL_ACCESS,
                     TRUE,
                     DUPLICATE_SAME_ACCESS) == 0)
  {
    set_errno(GetLastError());
    hret = (HANDLE)-1;
  }
  return (bhandle_t)hret;
}

bhandle_t __FASTCALL__ __OsOpen(const char *fname,int mode)
{
  char *dosname;
  HANDLE handle;
  int access,smode;
  access = smode = 0;
  access = GENERIC_READ;
  dosname = fnUnix2Dos(fname);
  if(mode & FO_WRITEONLY) access |= GENERIC_WRITE;
  if(mode & FO_READWRITE) access |= GENERIC_WRITE | GENERIC_READ;
  if(mode & SO_DENYWRITE) smode |= FILE_SHARE_READ;
  if(mode & SO_DENYREAD) smode |= FILE_SHARE_WRITE;
  if(mode & SO_DENYNONE) smode |= FILE_SHARE_READ | FILE_SHARE_WRITE;
  handle = CreateFile(dosname,
                      access,
                      smode,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);
  if(handle == INVALID_HANDLE_VALUE)
  {
    set_errno(GetLastError());
    handle = (HANDLE)-1;
  }
  return (bhandle_t)handle;
}

__fileoff_t __FASTCALL__ __OsSeek(bhandle_t handle,__fileoff_t offset,int origin)
{
  unsigned long hioff=offset>>32;
  return SetFilePointer((HANDLE)handle,offset,&hioff,origin);
}

int __FASTCALL__ __OsTruncFile(bhandle_t handle, __filesize_t newsize)
{
  int ret = 0;
  __OsSeek(handle,newsize,SEEKF_START);
  if(SetEndOfFile((HANDLE)handle) == 0)
  {
    set_errno(GetLastError());
    ret = 1;
  }
  return ret;
}

int __FASTCALL__  __OsRead(bhandle_t handle, void *buff, unsigned size)
{
  DWORD ret = size;
  if(ReadFile((HANDLE)handle,buff,size,&ret,NULL) == 0)
  {
    set_errno(GetLastError());
    ret = 0;
  }
  return ret;
}

int __FASTCALL__  __OsWrite(bhandle_t handle,const void *buff, unsigned size)
{
  DWORD ret = size;
  if(WriteFile((HANDLE)handle,buff,size,&ret,NULL) == 0)
  {
    set_errno(GetLastError());
    ret = 0;
  }
  return ret;
}

int __FASTCALL__ __OsChSize(bhandle_t handle, __fileoff_t size)
{
  return __OsTruncFile(handle,size);
}

__fileoff_t __FASTCALL__ __FileLength(bhandle_t handle)
{
  DWORD ret, hisize, err;
  ret = GetFileSize((HANDLE)handle,&hisize);
  if(ret == 0xFFFFFFFFUL && (err = GetLastError()) != NO_ERROR)
  {
    set_errno(err);
    ret = 0;
  }
  return ((__fileoff_t)ret&0xFFFFFFFFUL) | (((__fileoff_t)hisize)<<32);
}

__fileoff_t __FASTCALL__ __OsTell(bhandle_t handle) { return __OsSeek(handle,0L,SEEKF_CUR); }

tBool __FASTCALL__ __IsFileExists(const char *name)
{
   bhandle_t handle = __OsOpen(name,FO_READONLY | SO_DENYNONE);
   if(handle != -1) __OsClose(handle);
   return handle != -1;
}

tBool      __FASTCALL__ __OsGetFTime(const char *name,FTime *data)
{
  tBool ret = False;
  FILETIME ct,at,mt;
  bhandle_t handle;
   handle = __OsOpen(name,FO_READONLY);
   if(handle == -1) handle = __OsOpen(name,FO_READONLY | SO_DENYNONE);
   if(handle != -1)
   {
     if(GetFileTime((HANDLE)handle,&ct,&at,&mt))
     {
       WORD ft,fd;
       struct tm tm;

       /* Here we must convert 64-bit win32 filetime to 32-bit unix filetime */

       FileTimeToDosDateTime(&at,&fd,&ft);
       /* This part of code was taken from DJ Delorie C library */
       tm.tm_sec  = (ft&0x1f)*2;
       tm.tm_min  = (ft>>5)&0x3f;
       tm.tm_hour = (ft>>11)&0x1f;
       tm.tm_mday = fd&0x1f;
       tm.tm_mon  = ((fd>>5)&0x0f)-1; /* 0 = January */
       tm.tm_year = (fd>>9)+80;
       data->acctime = mktime(&tm);

       FileTimeToDosDateTime(&mt,&fd,&ft);
       /* This part of code was taken from DJ Delorie C library */
       tm.tm_sec  = (ft&0x1f)*2;
       tm.tm_min  = (ft>>5)&0x3f;
       tm.tm_hour = (ft>>11)&0x1f;
       tm.tm_mday = fd&0x1f;
       tm.tm_mon  = ((fd>>5)&0x0f)-1; /* 0 = January */
       tm.tm_year = (fd>>9)+80;
       data->modtime = mktime(&tm);
       ret = True;
     }
     else set_errno(GetLastError());
     __OsClose(handle);
   }
   return ret;
}

tBool      __FASTCALL__ __OsSetFTime(const char *name,const FTime *data)
{
  tBool ret = False;
  FILETIME at,mt;
  bhandle_t handle;
   handle = __OsOpen(name,FO_READWRITE);
   if(handle == -1) handle = __OsOpen(name,FO_READWRITE | SO_DENYNONE);
   if(handle != -1)
   {
     WORD fd,ft;
     struct tm *tm;
     time_t tim;

     /* Here we must convert 32-bit unix filetime to 64-bit win32 filetime */

     tim = data->acctime;
     tm = localtime(&tim); 
     /* Note: This if-statement was born because Windows can return
        null-pointer. This is probably bug of Win32 since POSIX standard
        guarantees return value to be valid. */
     if(tm) 
     {
       /* This part of code was taken from DJ Delorie C library */
       ft = tm->tm_sec/2+(tm->tm_min<<5)+(tm->tm_hour<<11);
       fd = tm->tm_mday+((tm->tm_mon+1)<<5)+((tm->tm_year-80)<<9);
       DosDateTimeToFileTime(fd,ft,&at);

       tim = data->modtime;
       tm = localtime(&tim);
       if(tm)
       {
         /* This part of code was taken from DJ Delorie C library */
         ft = tm->tm_sec/2+(tm->tm_min<<5)+(tm->tm_hour<<11);
         fd = tm->tm_mday+((tm->tm_mon+1)<<5)+((tm->tm_year-80)<<9);
         DosDateTimeToFileTime(fd,ft,&mt);

         if(SetFileTime((HANDLE)handle,NULL,&at,&mt)) ret = True;
         else set_errno(GetLastError());
       }
     }
     if(!tm) set_errno(EINTR);
     __OsClose(handle);
   }
   return ret;
}
