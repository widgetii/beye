/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/dos/os_dep.c
 * @brief       This file contains implementation of OS depended part for DOS.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1995
 * @note        Development, fixes and improvements
**/
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "biewlib/biewlib.h"

static void (interrupt __FAR__ * old1b)( void ) = 0;

static tBool __c__break = False;

static void __FAR__ __INTERRUPT__ MyCBreak( void )
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
  old1b = getvect(0x1B);
  setvect(0x1B, MyCBreak );
}

void __FASTCALL__ __term_sys( void )
{
  if(old1b) setvect(0x1B,old1b);
}

void __FASTCALL__ __OsYield( void )
{
  union REGS reg;
  memset(&reg,0,sizeof(union REGS));
  reg.x.ax = 0x1680;
  int86(0x2F,&reg,&reg);
}

static char rbuff[FILENAME_MAX+1];
extern char **ArgVector;

char * __FASTCALL__ __get_ini_name( const char *progname )
{
   int len;
   UNUSED(progname);
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
   UNUSED(progname);
   strcpy(rbuff2,ArgVector[0]);
   p1 = strrchr(rbuff2,'\\');
   p2 = strrchr(rbuff2,'/');
   p1 = max(p1,p2);
   if(p1) p1[1] = '\0';
   last = p1[strlen(p1)-1];
   if(!(last == '\\' || last == '/')) strcat(rbuff2,"\\");
   return rbuff2;
}



