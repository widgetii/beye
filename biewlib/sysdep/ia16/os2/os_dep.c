/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/os2/os_dep.c
 * @brief       This file contains implementation of OS depended part for OS/2.
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
#define INCL_SUB
#define INCL_DOSSEMAPHORES
#define INCL_DOSSIGNALS
#define INCL_DOSPROCESS
#define INCL_DOSMODULEMGR
#define INCL_DOSINFOSEG
#define INCL_DOSERRORS
#include <os2.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "biewlib/biewlib.h"

HSYSSEM biewSem = 0;

static tBool __c__break = False;

static VOID PASCAL FAR myCBreak( USHORT sigNum, USHORT sigArg)
{
  if(__c__break) exit(EXIT_FAILURE);
  __c__break = True;
}

tBool __FASTCALL__ __OsGetCBreak( void )
{
  return __c__break;
}

void  __FASTCALL__ __OsSetCBreak( tBool state )
{
  __c__break = state;
}

void __FASTCALL__ __init_sys( void )
{
  USHORT rc;
  rc = DosCreateSem(CSEM_PUBLIC,&biewSem,(PSZ)"\\SEM\\BIEW.SEM");
  if(rc == ERROR_ALREADY_EXISTS) /* means still one copy of biew already runing */
  {
     rc = DosOpenSem(&biewSem,(PSZ)"\\SEM\\BIEW.SEM");
     if(rc)
     {
       fprintf(stderr,"Can not create semaphore: error = %u\n",rc);
       exit(EXIT_FAILURE);
     }
  }
  DosSetSigHandler(&myCBreak,NULL,NULL,SIGA_ACCEPT,SIG_CTRLBREAK);
}

void __FASTCALL__ __term_sys( void )
{
  DosCloseSem(biewSem);
  DosSetSigHandler(NULL,NULL,NULL,SIGA_KILL,SIG_CTRLBREAK);
}

void __FASTCALL__ __OsYield( void ) { DosSleep(1); }

static char rbuff[FILENAME_MAX+1];
extern char **ArgVector;

static void __NEAR__ __FASTCALL__ getStartupFolder(char *to,unsigned size)
{
   SEL gsel;
   SEL lsel;
   PLINFOSEG plis;
   DosGetInfoSeg((PSEL)&gsel,(PSEL)&lsel);
   plis = MAKEPLINFOSEG(lsel);
   DosGetModName(plis->hmod,size,to);
}

char * __FASTCALL__ __get_ini_name( const char *progname )
{
   int len;
   getStartupFolder(rbuff,sizeof(rbuff));
   len = strlen(rbuff);
   if(stricmp(&rbuff[len-4],".exe") == 0) strcpy(&rbuff[len-4],".ini");
   else                                   strcat(rbuff,".ini");
   return rbuff;
}

static char rbuff2[FILENAME_MAX+1];
char * __FASTCALL__ __get_rc_dir( const char *progname )
{
   char *p1,*p2,last;
   getStartupFolder(rbuff2,sizeof(rbuff2));
   p1 = strrchr(rbuff2,'\\');
   p2 = strrchr(rbuff2,'/');
   p1 = max(p1,p2);
   if(p1) p1[1] = '\0';
   last = p1[strlen(p1)-1];
   if(!(last == '\\' || last == '/')) strcat(rbuff2,"\\");
   return rbuff2;
}

