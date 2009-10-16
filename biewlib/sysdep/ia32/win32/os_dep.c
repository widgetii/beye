/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/win32/os_dep.c
 * @brief       This file contains implementation of OS depended part for Win32s.
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
 *
 * @author      Mauro Giachero
 * @since       11.2007
 * @note        Added __get_home_dir() and some optimizations
**/
/* for cygwin - remove unnecessary includes */
#define _OLE_H
#define _OLE2_H
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#include "biewlib/biewlib.h"

#if __WORDSIZE > 32
/* Note: Vista-64 requires porting for MMF file handling */
#define __DISABLE_MMF 1
#endif

OSVERSIONINFO win32_verinfo;

static char rbuff[FILENAME_MAX+1];
static char rbuff2[FILENAME_MAX+1];
static char _home_dir_name[FILENAME_MAX + 1];

static tBool __c__break = False;

#if defined( _MSC_VER ) || __GNUC_MINOR__ >= 95
static BOOL WINAPI MyHandler( DWORD type )
#else
static BOOL MyHandler( DWORD type )
#endif
{
  switch(type)
  {
     case CTRL_C_EVENT:
     case CTRL_BREAK_EVENT:
                         if(__c__break)  exit(EXIT_FAILURE);
                         else __c__break = True;
                         return True;
     default:
                         return FALSE;
  }
}

tBool __FASTCALL__ __OsGetCBreak( void )
{
  return __c__break;
}

void __FASTCALL__ __OsSetCBreak( tBool state )
{
  __c__break = state;
}

extern HANDLE hIn;
extern tBool hInputTrigger;
#if !(defined( __DISABLE_MMF ) || defined( __DISABLE_LOWLEVEL_MMF))
extern LONG CALLBACK PageFaultHandler(LPEXCEPTION_POINTERS);
LPTOP_LEVEL_EXCEPTION_FILTER PrevPageFaultHandler;
#endif

void __FASTCALL__ __init_sys( void )
{
  win32_verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&win32_verinfo);
  SetConsoleCtrlHandler(MyHandler,TRUE);
#if !(defined( __DISABLE_MMF ) || defined( __DISABLE_LOWLEVEL_MMF))
  PrevPageFaultHandler = SetUnhandledExceptionFilter(PageFaultHandler);
#endif

  rbuff[0] = '\0';
  rbuff2[0] = '\0';
  _home_dir_name[0] = '\0';
}

void __FASTCALL__ __term_sys( void )
{
#if !(defined( __DISABLE_MMF ) || defined( __DISABLE_LOWLEVEL_MMF))
  SetUnhandledExceptionFilter(PrevPageFaultHandler);
#endif
  SetConsoleCtrlHandler(MyHandler,FALSE);
}

void __FASTCALL__ __OsYield( void )
{
       WaitForSingleObject(hIn,INFINITE);
       hInputTrigger++;
}

static void __FASTCALL__ getStartupFolder(char *to,unsigned cblen)
{
   GetModuleFileName(0,to,cblen);
}

char * __FASTCALL__ __get_ini_name( const char *progname )
{
   int len;

   if (rbuff[0]) return rbuff; //Already computed

   UNUSED(progname);
   getStartupFolder(rbuff,sizeof(rbuff));
   len = strlen(rbuff);
   if(stricmp(&rbuff[len-4],".exe") == 0) strcpy(&rbuff[len-4],".ini");
   else                                   strcat(rbuff,".ini");
   return rbuff;
}

char * __FASTCALL__ __get_rc_dir( const char *progname )
{
   char *p1,*p2,last;

   if (rbuff2[0]) return rbuff2; //Already computed

   UNUSED(progname);
   getStartupFolder(rbuff2,sizeof(rbuff2));
   p1 = strrchr(rbuff2,'\\');
   p2 = strrchr(rbuff2,'/');
   p1 = max(p1,p2);
   if(p1) p1[1] = '\0';
   last = p1[strlen(p1)-1];
   if(!(last == '\\' || last == '/')) strcat(rbuff2,"\\");
   return rbuff2;
}

/*
The home directory is a good place for configuration
and temporary files.
The trailing '\\' is included in the returned string.
*/
char * __FASTCALL__ __get_home_dir(const char *progname)
{
   char *p1,*p2,last;

   if (_home_dir_name[0]) return _home_dir_name; //Already computed

   UNUSED(progname);
   getStartupFolder(_home_dir_name,sizeof(_home_dir_name));
   p1 = strrchr(_home_dir_name,'\\');
   p2 = strrchr(_home_dir_name,'/');
   p1 = max(p1,p2);
   if(p1) p1[1] = '\0';
   last = p1[strlen(p1)-1];
   if(!(last == '\\' || last == '/')) strcat(_home_dir_name,"\\");
   return _home_dir_name;
}
