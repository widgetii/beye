/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/dos/os_dep.c
 * @brief       This file contains implementation of OS depended part for DOS-32.
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
#include <dpmi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <go32.h>

#include "biewlib/biewlib.h"

void __FASTCALL__ __init_sys( void )
{
  _go32_want_ctrl_break(0);
}

void __FASTCALL__ __term_sys( void )
{
  _go32_want_ctrl_break(1);
}

void __FASTCALL__ __OsYield( void )
{
  __dpmi_yield();
};

static tBool __c__break = 0;
static int   __c_hits = 0;

tBool __FASTCALL__ __OsGetCBreak( void )
{
  if(!__c__break)
  {
    __c_hits += _go32_was_ctrl_break_hit();
    if(__c_hits > 1) exit(EXIT_FAILURE);
    else
     if(__c_hits > 0) __c__break = True;
  }
  return __c__break;
}

void __FASTCALL__ __OsSetCBreak( tBool state )
{
  __c__break = state;
  __c_hits = 0;
}

static char rbuff[FILENAME_MAX+1];
extern char **ArgVector;

char * __FASTCALL__ __get_ini_name( const char *progname )
{
   int len;
   strcpy(rbuff,ArgVector[0]);
   len = strlen(rbuff);
   if(stricmp(&rbuff[len-4],".exe") == 0) strcpy(&rbuff[len-4],".ini");
   else                                   strcat(rbuff,".ini");
   return rbuff;
}

static char rbuff2[FILENAME_MAX+1];
char * __FASTCALL__ __get_rc_dir( const char *progname )
{
   char *p1,*p2,last;
   strcpy(rbuff2,ArgVector[0]);
   p1 = strrchr(rbuff2,'\\');
   p2 = strrchr(rbuff2,'/');
   p1 = max(p1,p2);
   if(p1) p1[1] = '\0';
   last = p1[strlen(p1)-1];
   if(!(last == '\\' || last == '/')) strcat(rbuff2,"\\");
   return rbuff2;
}


