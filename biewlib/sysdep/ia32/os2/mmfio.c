/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/os2/mmfio.c
 * @brief       Memory Mapped Files Interface for OS/2
 * @version     -
 * @remark      this source file is distributed under FreeBSD-like licence.
                Atricles about this technology are located at:
                http://www.edm2.com/0610/memorymap.html
                Original sources can be found at:
                http://www.edm2.com/0610/memorymap.zip
 * @note        Requires POSIX compatible development system
 *
 * @author      Sergey I. Yevtushenko <evsi@naverex.kiev.ua>
 * @since       03/09/1997
 * @note        Original code implementation
 * @author      Nickols_K
 * @date        23/10/2000
 * @note        Adapted for using with BIEW; expanding possibilities.
 * @bug         Program can be destroyed if operates with 0 size of file.
**/
#if defined ( __DISABLE_MMF ) || defined ( __DISABLE_LOWLEVEL_MMF )
/* Sorry! High level MMF support is not implemented under OS/2 SDK */
#include "biewlib/sysdep/ia16/dos/mmfio.c"
#else
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#define INCL_DOSPROCESS
#define INCL_DOSEXCEPTIONS
#define INCL_ERRORS
#include <os2.h>
#include "biewlib/pmalloc.h"
#include "biewlib/biewlib.h"

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
#define PAG_MASK    0xFFFFF000

typedef struct
{
    ULONG ulFlags;
    int   fhandle;
    PVOID pData;
    ULONG ulSize;
} MMFENTRY;

typedef MMFENTRY* PMMF;

/* Static data */

MMFENTRY mmfTable[MMF_MAX];

/* Local functions implementation */

static PMMF __FASTCALL__ Locate(void *addr)
{
    int i;
    for(i = 0; i < MMF_MAX; i++)
    {
        if(mmfTable[i].ulFlags & MMF_USEDENTRY)
        {
            if(   (ULONG)mmfTable[i].pData <= (ULONG)addr
               && ((ULONG)mmfTable[i].pData+mmfTable[i].ulSize) >= (ULONG)addr)
            {
                return &mmfTable[i];
            }
        }
    }
    return 0;
}

static PMMF __FASTCALL__ LocateFree(void)
{
    int i;
    for(i = 0; i < MMF_MAX; i++)
    {
        if(!(mmfTable[i].ulFlags & MMF_USEDENTRY))
        {
            return &mmfTable[i];
        }
    }
    return 0;
}

/* Main worker :) */
extern ULONG __FASTCALL__ PageFaultHandler(PEXCEPTIONREPORTRECORD p1,
				           PEXCEPTIONREGISTRATIONRECORD p2,
				           PCONTEXTRECORD p3,
				           PVOID pv);

ULONG __FASTCALL__ PageFaultHandler(PEXCEPTIONREPORTRECORD p1,
			            PEXCEPTIONREGISTRATIONRECORD p2,
			            PCONTEXTRECORD p3,
			            PVOID pv)
{
  UNUSED(p2);
  UNUSED(p3);
  UNUSED(pv);
    if( p1->ExceptionNum == XCPT_ACCESS_VIOLATION
      && ( p1->ExceptionInfo[0] == XCPT_WRITE_ACCESS
        || p1->ExceptionInfo[0] == XCPT_READ_ACCESS))
    {
        PMMF   pMMF   = 0;
        PVOID  pPage  = 0;
        ULONG  ulFlag = 0;
        ULONG  ulSize = PAG_SIZE;

        if(!(pMMF = Locate((void *)p1->ExceptionInfo[1])))
            return XCPT_CONTINUE_SEARCH;

        pPage = (PVOID)(p1->ExceptionInfo[1] & PAG_MASK);

        /* Query affected page flags */
        if(DosQueryMem(pPage, &ulSize, &ulFlag) != 0)
            return XCPT_CONTINUE_SEARCH;

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
        if(p1->ExceptionInfo[0] == XCPT_WRITE_ACCESS &&
           !(pMMF->ulFlags & FO_READWRITE))
              return XCPT_CONTINUE_SEARCH;

/* if page not committed, commit it and mark as readonly */

        if(!(ulFlag & PAG_COMMIT))
        {
            ULONG ulTemp = 0;
            /* We must temporary enable write access even for read-only pages
               for reading it into memory */
            if(DosSetMem(pPage, ulSize, PAG_READ | PAG_WRITE | PAG_COMMIT) != 0)
                                                    return XCPT_CONTINUE_SEARCH;

            /* set position & read page from disk */
            if(!DosSetFilePtr(pMMF->fhandle,
                              (ULONG)pPage - (ULONG)pMMF->pData,
                              FILE_BEGIN,
                              &ulTemp))
                              /* Actually ignore errors here */
                              DosRead(pMMF->fhandle,pPage,ulSize,&ulTemp);
            /* Mark it as read-only to be sure that page is clean. */
            if(DosSetMem(pPage, ulSize, PAG_READ) != 0)
                                                return XCPT_CONTINUE_SEARCH;
        }

/* if page already committed, and accessed for writing - mark them writable,
   for writting it back to the file at close. */
        if(p1->ExceptionInfo[0] == XCPT_WRITE_ACCESS)
        {
            DosSetMem(pPage, PAG_SIZE, PAG_READ | PAG_WRITE);
        }
        return XCPT_CONTINUE_EXECUTION; /* exception is fully working. Success exit */
    }
    return XCPT_CONTINUE_SEARCH;
}

static int __FASTCALL__ DosUpdateMMF(PMMF pMMF,unsigned long length)
{
    PBYTE  pArea  = 0;
    ULONG  ulPos  = 0;
    ULONG  ulFlag = 0;
    ULONG  ulSize = PAG_SIZE;
    APIRET rc     = NO_ERROR;

/* locate all regions which needs update, and actually update them */

    for(pArea = (PBYTE)pMMF->pData; ulPos < min(length,pMMF->ulSize); )
    {
        rc = DosQueryMem(pArea, &ulSize, &ulFlag); /* Query info about range */
        if(rc)
            return rc;

        if(ulFlag & PAG_WRITE)
        {
            /* set pointer */
            errno = 0;
            __OsSeek(pMMF->fhandle,(ULONG)pArea - (ULONG)pMMF->pData,SEEKF_START);
            if(!errno)
            {
                unsigned long rem = ulPos - pMMF->ulSize;
                __OsWrite(pMMF->fhandle,pArea,rem >= PAG_SIZE ? PAG_SIZE : rem);
                if(errno)  return errno;
                DosSetMem(pArea,PAG_SIZE,PAG_READ); /* Mark it as clean */
            }
        }
        ulPos += PAG_SIZE;
        pArea += PAG_SIZE;
    }

    return NO_ERROR;
}

static int __FASTCALL__ DosReallocMMF(PMMF pMMF,unsigned long new_length)
{
    PVOID  newData= 0;
    APIRET rc     = NO_ERROR;
    /*
       Since OS/2 does not have DosReallocMem we must perform this stupid
       operation. But it help us to avoid rereading information from disk.
       Anew allocated memory is decommited and will swapped from disk later.
       For flushing memory back to the disk a program must to use mmfFlush
       or DosUpdateMMF directly.
    */
    if(!(rc = DosAllocMem(&newData, new_length, PAG_READ | PAG_WRITE )))
    {
      DosFreeMem(pMMF->pData);
      pMMF->pData = newData;
      pMMF->ulSize = new_length;
    }
    return rc;
}

/* API Implementation */

mmfHandle          __FASTCALL__ __mmfOpen(const char *fname,int mode)
{
  FILESTATUS3 fsts3ConfigInfo;
  ULONG  ulBufSize = sizeof(FILESTATUS3);
  APIRET rc    = NO_ERROR;
  int    fhandle;
  PVOID  pData = 0;
  PMMF   pMMF  = 0;

/* Locate free entry in table */

    pMMF = LocateFree();

    if(!pMMF)
    {
        errno = ERROR_MMF_TOO_MANY_MMF_OPEN;
        return NULL;
    }
/* Open file */

    if((fhandle = __OsOpen(fname,mode)) == -1) return NULL;

/* Query file size */

    rc = DosQueryFileInfo( (HFILE)fhandle,
                           FIL_STANDARD,
                           &fsts3ConfigInfo,
                           ulBufSize);
    if(rc) goto exit_func;
    if(fsts3ConfigInfo.cbFile == 0) fsts3ConfigInfo.cbFile = 1;
/*
   Allocate memory. It is safety here to make read/write access because all
   pages are decommited (i.e. unaccessible). Actual access mode will be set
   later in exception handler.
*/
    rc = DosAllocMem(&pData, fsts3ConfigInfo.cbFile, PAG_READ | PAG_WRITE);

    if(rc)
    {
      exit_func:
        __OsClose(fhandle);
        return NULL;
    }

/* Everything seems ok, fill data structure and return pointer to memory */

    pMMF->ulFlags   = mode | MMF_USEDENTRY;
    pMMF->fhandle   = fhandle;
    pMMF->pData     = pData;
    pMMF->ulSize    = fsts3ConfigInfo.cbFile;
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
  DosUpdateMMF(mrec,min((unsigned long)length,mrec->ulSize));
  if(DosReallocMMF(mrec,length))
  {
    DosFreeMem(mrec->pData);
    __OsClose(mrec->fhandle);
    mrec = NULL;
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
  unsigned long old_length;
  tBool can_continue = False;
  old_length = mrec->ulSize;
  DosUpdateMMF(mrec,min((unsigned long)size,mrec->ulSize));
  if(mrec->ulSize > (unsigned long)size) /* truncate */
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

    DosClose(mrec->fhandle);
    DosFreeMem(mrec->pData);

    mrec->ulFlags   = 0;
    mrec->fhandle   = -1;
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
#endif