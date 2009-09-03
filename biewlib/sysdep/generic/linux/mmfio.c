/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/generic/linux/mmfio.c
 * @brief       This file contains implementation of memory mapped file i/o routines for POSIX compatible OS.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       2000
 * @note        Development, fixes and improvements
**/
#ifdef __DISABLE_MMF
#include "biewlib/sysdep/ia16/dos/mmfio.c"
#else
#include <sys/mman.h>
#include "biewlib/pmalloc.h"
#include "biewlib/biewlib.h"

#ifndef MREMAP_MAYMOVE
#define MREMAP_MAYMOVE 1
#endif

struct mmfRecord
{
  void * addr;
  long   length;
  int    fhandle;
  int    mode;
};

static int __FASTCALL__ mk_prot(int mode)
{
  int pflg;
  pflg = PROT_READ;
  if(mode & FO_WRITEONLY) pflg = PROT_WRITE;
  else
   if(mode & FO_READWRITE) pflg |= PROT_WRITE;
  return pflg;
}

static int __FASTCALL__ mk_flags(int mode)
{
  int pflg;
  pflg = 0;
#ifdef MAP_SHARED
  if((mode & SO_DENYREAD) ||
     (mode & SO_DENYWRITE) ||
     (mode & SO_DENYNONE))          pflg |= MAP_SHARED;
#endif
#ifdef MAP_PRIVATE
  if(mode & SO_PRIVATE) pflg |= MAP_PRIVATE;
#endif
  return pflg;
}

mmfHandle          __FASTCALL__ __mmfOpen(const char *fname,int mode)
{
  struct mmfRecord *mret;
  int fhandle;
  fhandle = __OsOpen(fname,mode);
  if(fhandle != -1)
  {
    mret = PMalloc(sizeof(struct mmfRecord));
    if(mret)
    {
      __fileoff_t length;
      void *addr;
      length = __FileLength(fhandle);
      if(length <= PTRDIFF_MAX)
      {
	addr = mmap(NULL,length,mk_prot(mode),
                  mk_flags(mode),fhandle,0L);
	if(addr != (void *)-1)
	{
	    mret->fhandle = fhandle;
	    mret->addr    = addr;
	    mret->length  = length;
	    mret->mode    = mode;
	    return mret;
	}
     }
     PFree(mret);
    }
    __OsClose(fhandle);
  }
  return NULL;
}

tBool              __FASTCALL__ __mmfFlush(mmfHandle mh)
{
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  return msync(mrec->addr,mrec->length,MS_SYNC) ? False : True;
}

mmfHandle       __FASTCALL__ __mmfSync(mmfHandle mh)
{
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  long length;
  length = __FileLength(mrec->fhandle);
  msync(mrec->addr,min(length,mrec->length),MS_SYNC);
  if((mrec->addr = mremap(mrec->addr,mrec->length,length,MREMAP_MAYMOVE)) != (void *)-1)
  {
     mrec->length = length;
     return mrec;
  }
  __OsClose(mrec->fhandle);
  PFree(mrec);
  return NULL;
}

tBool              __FASTCALL__ __mmfProtect(mmfHandle mh,int flags)
{
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  mrec->mode = flags;
  return mprotect(mrec->addr,mrec->length,mk_prot(flags)) ? False : True;
}

tBool              __FASTCALL__ __mmfResize(mmfHandle mh,long size)
{
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  void *new_addr;
  tBool can_continue = False;
  if(mrec->length > size) /* truncate */
  {
    if((new_addr = mremap(mrec->addr,mrec->length,size,MREMAP_MAYMOVE)) != (void *)-1) can_continue = True;
    if(can_continue)
      can_continue = __OsChSize(mrec->fhandle,size) != -1 ? True : False;
  }
  else /* expand */
  {
    if(__OsChSize(mrec->fhandle,size) != -1) can_continue = True;
    if(can_continue)
      can_continue = (new_addr = mremap(mrec->addr,mrec->length,size,MREMAP_MAYMOVE)) != (void *)-1 ? True : False;
  }
  if(can_continue)
  {
      mrec->addr = new_addr;
      mrec->length = size;
      return True;
  }
  else /* Attempt to unroll transaction back */
    __OsChSize(mrec->fhandle,mrec->length);
  return False;
}

void               __FASTCALL__ __mmfClose(mmfHandle mh)
{
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  munmap(mrec->addr,mrec->length);
  __OsClose(mrec->fhandle);
  PFree(mrec);
}

void *             __FASTCALL__ __mmfAddress(mmfHandle mh)
{
  return ((struct mmfRecord *)mh)->addr;
}

long              __FASTCALL__ __mmfSize(mmfHandle mh)
{
  return ((struct mmfRecord *)mh)->length;
}

tBool             __FASTCALL__ __mmfIsWorkable( void ) { return True; }

#endif
