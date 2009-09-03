/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/dos/fileio.c
 * @brief       This file contains implementation of file i/o routines for DOS.
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
#include <dos.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

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

int __FASTCALL__ __OsCreate(const char *name)
{
  char *dosname;
  union REGS reg;
  struct SREGS sreg;
  memset(&reg,0,sizeof(union REGS));
  memset(&sreg,0,sizeof(struct SREGS));
  dosname = fnUnix2Dos(name);
  reg.h.ah = 0x3C;
  reg.x.dx = FP_OFF(dosname);
  sreg.ds = FP_SEG(dosname);
  int86x(0x21,&reg,&reg,&sreg);
  if(reg.x.flags & 0x01) { errno = reg.x.ax; reg.x.ax = -1; }
  return reg.x.ax;
}

int __FASTCALL__ __OsDelete(const char *name)
{
  char *dosname;
  union REGS reg;
  struct SREGS sreg;
  memset(&reg,0,sizeof(union REGS));
  memset(&sreg,0,sizeof(struct SREGS));
  dosname = fnUnix2Dos(name);
  reg.h.ah = 0x41;
  reg.x.dx = FP_OFF(dosname);
  sreg.ds = FP_SEG(dosname);
  int86x(0x21,&reg,&reg,&sreg);
  if(reg.x.flags & 0x01) { errno = reg.x.ax; reg.x.ax = -1; }
  else reg.x.ax = 0;
  return reg.x.ax;
}

int __FASTCALL__ __OsRename(const char *oldname,const char *newname)
{
  union REGS reg;
  struct SREGS sreg;
  char olddosname[FILENAME_MAX+1],*newdosname;
  memset(&reg,0,sizeof(union REGS));
  memset(&sreg,0,sizeof(struct SREGS));
  newdosname = fnUnix2Dos(oldname);
  strncpy(olddosname,newdosname,sizeof(olddosname)-1);
  newdosname = fnUnix2Dos(newname);
  reg.h.ah = 0x56;
  reg.x.dx = FP_OFF(olddosname);
  reg.x.di = FP_OFF(newdosname);
  sreg.ds = FP_SEG(olddosname);
  sreg.es = FP_SEG(newdosname);
  int86x(0x21,&reg,&reg,&sreg);
  if(reg.x.flags & 0x01) { errno = reg.x.ax; reg.x.ax = -1; }
  else reg.x.ax = 0;
  return reg.x.ax;
}

void __FASTCALL__ __OsClose(int handle)
{
  union REGS reg;
  memset(&reg,0,sizeof(union REGS));
  reg.h.ah = 0x3E;
  reg.x.bx = handle;
  int86(0x21,&reg,&reg);
  if(reg.x.flags & 0x01) errno = reg.x.ax;
}

int __FASTCALL__ __OsDupHandle(int handle)
{
  union REGS reg;
  memset(&reg,0,sizeof(union REGS));
  reg.h.ah = 0x45;
  reg.x.bx = handle;
  int86(0x21,&reg,&reg);
  if(reg.x.flags & 0x01) { errno = reg.x.ax; reg.x.ax = -1; }
  return reg.x.ax;
}

int __FASTCALL__ __OsOpen(const char *fname,int mode)
{
  char *dosname;
  union REGS reg;
  struct SREGS sreg;
  memset(&reg,0,sizeof(union REGS));
  memset(&sreg,0,sizeof(struct SREGS));
  dosname = fnUnix2Dos(fname);
  reg.x.ax = 0x6C00;
  reg.h.bl = mode;
  reg.h.bh = 0x20; /* internal possible optimization: not commit on every write
                       return media i/o error through errno but not 24h
                       FAT32 size <= 2GB  */
  reg.h.dl = 0x01; /* fail if not exists, open only if exists */
  sreg.ds = FP_SEG(dosname);
  reg.x.si = FP_OFF(dosname);
  int86x(0x21,&reg,&reg,&sreg);
  if(reg.x.flags & 0x01)
  {
    errno = reg.x.ax;
    reg.x.ax = -1;
  }
  return reg.x.ax;
}

__fileoff_t __FASTCALL__ __OsSeek(int handle,__fileoff_t offset,int origin)
{
  __fileoff_t ret;
  union REGS reg;
  memset(&reg,0,sizeof(union REGS));
  reg.h.ah = 0x42;
  reg.h.al = origin;
  reg.x.bx = handle;
  reg.x.cx = (unsigned short)((offset >> 16) & 0xFFFF);
  reg.x.dx = (unsigned short)(offset & 0xFFFF);
  int86(0x21,&reg,&reg);
  if(reg.x.flags & 0x01)
  {
    errno = reg.x.ax;
    ret = -1;
  }
  else ret = ((long)reg.x.dx << 16) + reg.x.ax;
  return ret;
}

int __FASTCALL__ __OsTruncFile( int handle, __filesize_t newsize)
{
  __OsSeek(handle,newsize,0);
  return __OsWrite(handle,NULL,0);
}

int __FASTCALL__  __OsRead(int handle, void *buff, unsigned size)
{
  union REGS reg;
  struct SREGS sreg;
  memset(&reg,0,sizeof(union REGS));
  memset(&sreg,0,sizeof(struct SREGS));
  reg.h.ah = 0x3F;
  reg.x.bx = handle;
  reg.x.cx = size;
  sreg.ds = FP_SEG(buff);
  reg.x.dx = FP_OFF(buff);
  int86x(0x21,&reg,&reg,&sreg);
  if(reg.x.flags & 0x01)
  {
    errno = reg.x.ax;
    reg.x.ax = 0;
  }
  return reg.x.ax;
}

int __FASTCALL__  __OsWrite(int handle,const void *buff, unsigned size)
{
  union REGS reg;
  struct SREGS sreg;
  memset(&reg,0,sizeof(union REGS));
  memset(&sreg,0,sizeof(struct SREGS));
  reg.h.ah = 0x40;
  reg.x.bx = handle;
  reg.x.cx = size;
  sreg.ds = FP_SEG(buff);
  reg.x.dx = FP_OFF(buff);
  int86x(0x21,&reg,&reg,&sreg);
  if(reg.x.flags & 0x01)
  {
    errno = reg.x.ax;
    reg.x.ax = 0;
  }
  return reg.x.ax;
}

#define BLKSIZE 32767

int __FASTCALL__ __OsChSize(int handle, __fileoff_t size)

{
    __fileoff_t length, fillsize;
    char *buf;
    int  bufsize, numtowrite;

    length=__OsSeek(handle, 0L, SEEKF_END); /* get current length */
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

__fileoff_t __FASTCALL__ __FileLength(int handle)
{
  __fileoff_t ret,spos;
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
  union REGS reg;
  memset(&reg,0,sizeof(union REGS));
  handle = __OsOpen(name,FO_READONLY);
  if(handle == -1) handle = __OsOpen(name,FO_READONLY | SO_DENYNONE);
  if(handle != -1)
  {
    unsigned fd,ft;
    struct tm tm;
    reg.x.ax = 0x5700;
    reg.x.bx = handle;
    int86(0x21,&reg,&reg);
    if(reg.x.flags & 0x01) errno = reg.x.ax;
    else
    {
      ft = reg.x.cx;
      fd = reg.x.dx;
    /* This part of code was taken from DJ Delorie C library */
      tm.tm_sec  = (ft&0x1f)*2;
      tm.tm_min  = (ft>>5)&0x3f;
      tm.tm_hour = (ft>>11)&0x1f;
      tm.tm_mday = fd&0x1f;
      tm.tm_mon  = ((fd>>5)&0x0f)-1; /* 0 = January */
      tm.tm_year = (fd>>9)+80;
      data->modtime = data->acctime = mktime(&tm);
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
  union REGS reg;
  memset(&reg,0,sizeof(union REGS));
  handle = __OsOpen(name,FO_READWRITE);
  if(handle == -1) handle = __OsOpen(name,FO_READWRITE | SO_DENYNONE);
  if(handle != -1)
  {
    unsigned fd,ft;
    struct tm *tm;
    time_t tim = data->acctime;
    tm = localtime(&tim);
    /* This part of code was taken from DJ Delorie C library */
    ft = tm->tm_sec/2+(tm->tm_min<<5)+(tm->tm_hour<<11);
    fd = tm->tm_mday+((tm->tm_mon+1)<<5)+((tm->tm_year-80)<<9);
    reg.x.ax = 0x5701;
    reg.x.bx = handle;
    reg.x.cx = ft;
    reg.x.dx = fd;
    int86(0x21,&reg,&reg);
    if(reg.x.flags & 0x01) errno = reg.x.ax;
    else                   ret = True;
    __OsClose(handle);
  }
  return ret;
}
