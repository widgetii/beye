/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/win32/mmfio.c
 * @brief       Memory Mapped Files Interface for Win32
 * @version     -
 * @remark      this source file is modified version of OS/2 one.
                For detail see biewlib/sysdep/ia32/os2/mmfio.c
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @date        02/03/2001
 * @bug         Program can be destroyed if operates with 0 size of file.
**/
/* for cygwin - remove unnecessary includes */
#define _OLE_H
#define _OLE2_H
#ifdef __DISABLE_MMF
#include "biewlib/sysdep/ia16/dos/mmfio.c"
#else
#ifndef __DISABLE_LOWLEVEL_MMF
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <windows.h>
#include "biewlib/pmalloc.h"
#include "biewlib/biewlib.h"

/*
   Using standard file-mapping technique of Win32 (CreateFileMapping, 
   MapViewOfFile and so on) is not compatible with program logic of biew.
   (IMHO it also is incompatible with logic of mremap function from advanced
   UNIX standards).

   Cause:
   WinNT 4.0 SDK says:
   SetEndOfFile.
    ...
   Remarks:
    ...
    If you called CreateFileMapping to create a file-mapping object for hFile,
    you must first call UnmapViewOfFile to unmap all views and call CloseHandle
    to close the file-mapping object before you can call SetEndOfFile.

    But implementation of same logic with using VirtualAlloc - VirtualFree
    and SetUnhandledExceptionFilter avoids problems described above.
*/

/*
**  Constants
*/

#define MMF_USEDENTRY           0x80000000  /* internal flag */
#define MMF_MAX                 255         /* maximal number of concurently */
                                            /* active MMF areas */

/*
**  Error codes
*/

#define ERROR_MMF_TOO_MANY_MMF_OPEN -1
#define ERROR_MMF_ZERO_FILE         -2
#define ERROR_MMF_INVALID_REGION    -3


/* Internal structures and constants */

#define PAG_SIZE    4096
#if __WORDSIZE == 32
#define PAG_MASK    0xFFFFF000
typedef unsigned long ADRSIZE;
#else
/* Hack: MinGW64 (unlike linux64) assumes sizeof(long)==4 :( */
#define PAG_MASK    0xFFFFFFFFFFFFF000
typedef unsigned long long ADRSIZE;
#endif

typedef struct
{
    tUInt32   ulFlags;
    HANDLE    fhandle;
    void *    pData;
    ADRSIZE   ulSize;
} MMFENTRY;

typedef MMFENTRY* PMMF;

/* Static data */

MMFENTRY mmfTable[MMF_MAX];

/* Local functions implementation */

static PMMF __FASTCALL__ Locate(void *addr)
{
    unsigned i;
    for(i = 0; i < MMF_MAX; i++)
    {
        if(mmfTable[i].ulFlags & MMF_USEDENTRY)
        {
            if(   (ADRSIZE)mmfTable[i].pData <= (ADRSIZE)addr
               && ((ADRSIZE)mmfTable[i].pData+mmfTable[i].ulSize) >= (ADRSIZE)addr)
            {
                return &mmfTable[i];
            }
        }
    }
    return 0;
}

static PMMF __FASTCALL__ LocateFree(void)
{
    unsigned i;
    for(i = 0; i < MMF_MAX; i++)
    {
        if(!(mmfTable[i].ulFlags & MMF_USEDENTRY))
        {
            return &mmfTable[i];
        }
    }
    return 0;
}

LONG CALLBACK apiPageFaultHandler(LPEXCEPTION_POINTERS excpt)
{
    MEMORY_BASIC_INFORMATION mbi;
    ADRSIZE ulTemp;
#if __WORDSIZE > 32
    PEXCEPTION_RECORD64 per=excpt->ExceptionRecord;
#else
    PEXCEPTION_RECORD per=excpt->ExceptionRecord;
#endif
    if(per->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        PMMF     pMMF   = 0;
        void *   pPage  = 0;
        ULONG ulSize = PAG_SIZE;
        if(!(pMMF = Locate((void *)per->ExceptionInformation[1])))
            return EXCEPTION_CONTINUE_SEARCH;
        pPage = (void *)((ADRSIZE)per->ExceptionInformation[1] & PAG_MASK);

        /* Query affected page flags */
        if(VirtualQuery(pPage, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) != 
                                     sizeof(MEMORY_BASIC_INFORMATION))
               return EXCEPTION_CONTINUE_SEARCH;

/*
** There can be three cases:
**
**  1. We trying to read page              - always OK, commit it
**  2. We trying to write committed page   - OK if READ/WRITE mode
**  3. We trying to write uncommitted page - OK if READ/WRITE mode
**                                           but we need to commit it.
*/

/* If file was open for read-only access and system is requiring write access
   we must produce page fault exception */
        if(per->ExceptionInformation[0] != 0 &&
           !(pMMF->ulFlags & FO_READWRITE))
              return EXCEPTION_CONTINUE_SEARCH;

/* if page not committed, commit it and mark as readonly */

        if(!(mbi.State & MEM_COMMIT))
        {
            /* We must temporary enable write access even for read-only pages
               for reading it into memory */
            if(VirtualAlloc(pPage, ulSize, MEM_COMMIT, PAGE_READWRITE) != pPage)
                                           return EXCEPTION_CONTINUE_SEARCH;
            ulTemp = (ADRSIZE)pPage - (ADRSIZE)pMMF->pData;
            /* set position & read page from disk */
            if(SetFilePointer((HANDLE)pMMF->fhandle,
                              ulTemp,
                              NULL, 
                              FILE_BEGIN) == ulTemp)
                              /* Actually ignore errors here */
                              ReadFile((HANDLE)pMMF->fhandle,pPage,ulSize,(DWORD *)&ulTemp,NULL);
            /* Mark it as read-only to be sure that page is clean. */
            if(VirtualProtect(pPage, ulSize, PAGE_READONLY,(ULONG *)&ulTemp) == 0)
                                              return EXCEPTION_CONTINUE_SEARCH;
        }

/* if page already committed, and accessed for writing - mark them writable,
   for writting it back to the file at close. */
        if(per->ExceptionInformation[0] != 0)
            VirtualProtect(pPage, PAG_SIZE, PAGE_READWRITE,(ULONG *)&ulTemp);
        return EXCEPTION_CONTINUE_EXECUTION;
        /* exception is fully working. Success exit. */
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

extern LPTOP_LEVEL_EXCEPTION_FILTER PrevPageFaultHandler;

/* Main worker :) */
LONG CALLBACK PageFaultHandler(LPEXCEPTION_POINTERS excpt)
{
  LONG retval;
  retval = apiPageFaultHandler(excpt);
  if(retval == EXCEPTION_CONTINUE_SEARCH && PrevPageFaultHandler)
    return (*PrevPageFaultHandler)(excpt);
  return retval;
}

static int __FASTCALL__ DosUpdateMMF(PMMF pMMF,ADRSIZE length)
{
    char *  pArea  = 0;
    ADRSIZE ulPos  = 0;
    MEMORY_BASIC_INFORMATION mbi;

/* locate all regions which needs update, and actually update them */

    for(pArea = (PBYTE)pMMF->pData; ulPos < min(length,pMMF->ulSize); )
    {                                                                  
        /* Query info about range */
        if(VirtualQuery(pArea, &mbi, sizeof(MEMORY_BASIC_INFORMATION) != 
                                     sizeof(MEMORY_BASIC_INFORMATION)))
								return -1;

        if(mbi.AllocationProtect & PAGE_READWRITE)
        {
            /* set pointer */
            errno = 0;
            __OsSeek(pMMF->fhandle,(ADRSIZE)pArea - (ADRSIZE)pMMF->pData,SEEKF_START);
            if(!errno)
            {
                ADRSIZE rem = ulPos - pMMF->ulSize;
                __OsWrite(pMMF->fhandle,pArea,rem >= PAG_SIZE ? PAG_SIZE : rem);
                if(errno)  return errno;
                VirtualProtect(pArea,PAG_SIZE,PAGE_READONLY,(DWORD *) &rem); /* Mark it as clean */
            }
        }
        ulPos += PAG_SIZE;
        pArea += PAG_SIZE;
    }

    return 0;
}

static int __FASTCALL__ DosReallocMMF(PMMF pMMF,ADRSIZE new_length)
{
    void *  newData = 0;
    /*
       Since Win32 does not have VirtualRealloc we must perform this stupid
       operation. But it help us to avoid rereading information from disk.
       Anew allocated memory is decommited and will swapped from disk later.
       For flushing memory back to the disk a program must to use mmfFlush
       or DosUpdateMMF directly.
    */
    newData = VirtualAlloc(NULL, new_length, MEM_RESERVE, PAGE_READWRITE);
    if(newData)
    {
      VirtualFree(pMMF->pData, 0L, MEM_RELEASE);
      pMMF->pData = newData;
      pMMF->ulSize = new_length;
      return 0;
    }
    return -1;
}

/* API Implementation */

mmfHandle          __FASTCALL__ __mmfOpen(const char *fname,int mode)
{
  ADRSIZE    flength;
  bhandle_t  fhandle;
  void * pData = 0;
  PMMF   pMMF  = 0;

/* Locate free entry in table */

    pMMF = LocateFree();

    if(!pMMF)
    {
        errno = ERROR_MMF_TOO_MANY_MMF_OPEN;
        return NULL;
    }
/* Open file */

    if((fhandle = __OsOpen(fname,mode)) == NULL_HANDLE) return NULL;

/* Query file size */
    if((flength = GetFileSize((HANDLE)fhandle,NULL)) == 0xFFFFFFFFUL)
         goto exit_func;
    if(!flength) flength = 1;
/*
   Allocate memory. It is safety here to make read/write access because all
   pages are decommited (i.e. unaccessible). Actual access mode will be set
   later in exception handler.
*/
    if(!(pData = VirtualAlloc(NULL, flength, MEM_RESERVE, PAGE_READWRITE)))
    {
      exit_func:
        __OsClose(fhandle);
        return NULL;
    }

/* Everything seems ok, fill data structure and return pointer to memory */

    pMMF->ulFlags   = mode | MMF_USEDENTRY;
    pMMF->fhandle   = fhandle;
    pMMF->pData     = pData;
    pMMF->ulSize    = flength;
    return pMMF;
}

tBool              __FASTCALL__ __mmfFlush(mmfHandle mh)
{
  PMMF mrec = (PMMF)mh;
  return DosUpdateMMF(mrec,mrec->ulSize) ? True : False;
}

mmfHandle     __FASTCALL__ __mmfSync(mmfHandle mh)
{
  PMMF mrec = (PMMF)mh;
  long length;
  length = __FileLength(mrec->fhandle);
  DosUpdateMMF(mrec,min((ADRSIZE)length,mrec->ulSize));
  if(length!=mrec->ulSize) {
    if(DosReallocMMF(mrec,length))
    {
	VirtualFree(mrec->pData, 0L, MEM_RELEASE);
	__OsClose(mrec->fhandle);
	mrec = NULL;
    }
  }
  return mrec;
}

tBool              __FASTCALL__ __mmfProtect(mmfHandle mh,int flags)
{
  PMMF mrec = (PMMF)mh;
  mrec->ulFlags = flags | MMF_USEDENTRY;
  return True;
}

tBool              __FASTCALL__ __mmfResize(mmfHandle mh,long size)
{
  PMMF mrec = (PMMF)mh;
  ADRSIZE old_length;
  tBool can_continue = False;
  old_length = mrec->ulSize;
  DosUpdateMMF(mrec,min((ADRSIZE)size,mrec->ulSize));
  if(mrec->ulSize > (ADRSIZE)size) /* truncate */
  {
    if(!DosReallocMMF(mrec,size)) can_continue = True;
    if(can_continue)
      can_continue = __OsChSize(mrec->fhandle,size) != -1 ? True : False;
  }
  else /* expand */
  {
    if(__OsChSize(mrec->fhandle,size) != -1) can_continue = True;
    if(can_continue)
      can_continue = !DosReallocMMF(mrec,size) ? True : False;
  }
  if(can_continue) return True;
  else /* Attempt to unroll transaction back */
    __OsChSize(mrec->fhandle,old_length);
  return False;
}

void               __FASTCALL__ __mmfClose(mmfHandle mh)
{
  PMMF mrec = (PMMF)mh;

/* No automatic update when area freed */

    if((mrec->ulFlags & FO_READWRITE) || (mrec->ulFlags & FO_WRITEONLY))
        DosUpdateMMF(mrec->pData,mrec->ulSize);

    CloseHandle((HANDLE)mrec->fhandle);
    VirtualFree(mrec->pData, 0L, MEM_RELEASE);

    mrec->ulFlags   = 0;
    mrec->fhandle   = NULL_HANDLE;
    mrec->pData     = 0;
    mrec->ulSize    = 0;
}

void *             __FASTCALL__ __mmfAddress(mmfHandle mh)
{
  return ((PMMF)mh)->pData;
}

long              __FASTCALL__ __mmfSize(mmfHandle mh)
{
  return ((PMMF)mh)->ulSize;
}

tBool             __FASTCALL__ __mmfIsWorkable( void ) { return True; }
#else
/*
   WARNING! This implementation of program logic is not fully compatible
   with win32 MMF.
   WinNT 4.0 SDK says:
   SetEndOfFile.
    ...
   Remarks:
    ...
    If you called CreateFileMapping to create a file-mapping object for hFile,
    you must first call UnmapViewOfFile to unmap all views and call CloseHandle
    to close the file-mapping object before you can call SetEndOfFile.
*/
#include <windows.h>
#include "biewlib/pmalloc.h"
#include "biewlib/biewlib.h"

#ifndef _MSC_VER
#error "You've defined __DISABLE_LOWLEVEL_MMF. You can comment out this message, but in this case resulting code will partially workable."
#endif

struct mmfRecord
{
   void *    addr;
   long      length;
   bhandle_t fhandle;
   int       mode;
   HANDLE    fmapping;
};

static int __FASTCALL__ mk_prot(int mode)
{
  int pflg;
  pflg = PAGE_READONLY;
  if((mode & FO_WRITEONLY) || (mode & FO_READWRITE)) pflg = PAGE_READWRITE;
  pflg |= SEC_COMMIT | SEC_NOCACHE;
  if(mode & SO_PRIVATE) pflg |= PAGE_WRITECOPY;
  return pflg;
}

static int __FASTCALL__ mk_access(int mode)
{
  int pflg;
  pflg = FILE_MAP_READ;
  if((mode & FO_WRITEONLY) || (mode & FO_READWRITE)) pflg = FILE_MAP_WRITE;
  if(mode & SO_PRIVATE) pflg |= FILE_MAP_COPY;
  return pflg;
}

mmfHandle          __FASTCALL__ __mmfOpen(const char *fname,int mode)
{
  struct mmfRecord *mret;
  __fileoff_t length;
  bhandle_t fhandle;
  fhandle = __OsOpen(fname,mode);
  if(fhandle != NULL_HANDLE)
  {
    length = __FileLength(fhandle);
    if(length <= PTRDIFF_MAX)
    {
      mret = PMalloc(sizeof(struct mmfRecord));
      if(mret)
      {
        HANDLE fmapping;
        void *addr;
        fmapping = CreateFileMapping((HANDLE)fhandle,NULL,mk_prot(mode),0L,length,NULL);
        if(fmapping)
        {
          addr = MapViewOfFile(fmapping,mk_access(mode),0L,0L,length);
          if(addr)
          {
            mret->fhandle = fhandle;
            mret->fmapping= fmapping;
            mret->addr    = addr;
            mret->length  = length;
            mret->mode    = mode;
            return mret;
          }
          CloseHandle(fmapping);
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
  return FlushViewOfFile(mrec->addr,mrec->length) ? True : False;
}

mmfHandle     __FASTCALL__ __mmfSync(mmfHandle mh)
{
  /** @bug  This implementation does not change size of file.
            Sorry! It is not possible under win32 without changing logic of
            program.
  */
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  long length;
  length = __FileLength(mrec->fhandle);
  FlushViewOfFile(mrec->addr,min(mrec->length,length));
  UnmapViewOfFile(mrec->addr); /* It restores size of truncated file */
  CloseHandle(mrec->fmapping);
  if(length < mrec->length) __OsChSize(mrec->fhandle,length);
  mrec->length = __FileLength(mrec->fhandle);
  mrec->fmapping = CreateFileMapping((HANDLE)mrec->fhandle,NULL,mk_prot(mrec->mode),0L,mrec->length,NULL);
  if(mrec->fmapping)
  {
    if((mrec->addr = MapViewOfFile(mrec->fmapping,mk_access(mrec->mode),0L,0L,mrec->length)) != NULL)
                                                                    return mh;
    CloseHandle(mrec->fmapping);
  }
  __OsClose(mrec->fhandle);
  PFree(mrec);
  return NULL;
}

tBool              __FASTCALL__ __mmfProtect(mmfHandle mh,int flags)
{
  DWORD oldProt;
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  mrec->mode = flags;
  return VirtualProtect(mrec->addr,mrec->length,mk_prot(flags),&oldProt) ? True : False;
}

tBool              __FASTCALL__ __mmfResize(mmfHandle mh,long size)
{
  /** @bug  This implementation does not change size of file.
            Sorry! It is not possible under win32 without changing logic of
            program.
  */
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  UnmapViewOfFile(mrec->addr);
  CloseHandle(mrec->fmapping);
  __OsChSize(mrec->fhandle,size);
  mrec->length = __FileLength(mrec->fhandle);
  mrec->fmapping = CreateFileMapping((HANDLE)mrec->fhandle,NULL,mk_prot(mrec->mode),0L,mrec->length,NULL);
  if(mrec->fmapping)
  {
    if((mrec->addr = MapViewOfFile(mrec->fmapping,mk_access(mrec->mode),0L,0L,mrec->length)) != NULL)
                                                                    return True;
    CloseHandle(mrec->fmapping);
  }
  __OsClose(mrec->fhandle);
  PFree(mrec);
  return False;
}

void               __FASTCALL__ __mmfClose(mmfHandle mh)
{
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  UnmapViewOfFile(mrec->addr);
  CloseHandle(mrec->fmapping);
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
#endif
