/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/os2/timer.c
 * @brief       This file contains implementation of timer depended part for OS/2.
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
#define INCL_DOSSEMAPHORES
#define INCL_DOSINFOSEG
#define INCL_DOSDATETIME
#define INCL_DOSPROCESS
#include <os2.h>
#include <stddef.h>
#include <stdlib.h>

#include "biewlib/biewlib.h"

extern HSYSSEM biewSem;
static HTIMER  timerID = 0;
static TID     timerThread = 0;
static         timer_callback *user_callback = NULL;
static BYTE *thread_stack = 0;

static void thread_callback( void )
{
  while(1)
  {
    DosSemSetWait(biewSem,SEM_INDEFINITE_WAIT);
    if(user_callback) (*user_callback)();
    else DosExit(EXIT_THREAD,0);
  }
}

unsigned __FASTCALL__  __OsSetTimerCallBack(unsigned ms,timer_callback func)
{
   SEL gsel;
   SEL lsel;
   int rc;
   USHORT real_interval;
   PGINFOSEG pgis;
   if(!(thread_stack = malloc(0x1000))) return 0;
   DosGetInfoSeg((PSEL)&gsel,(PSEL)&lsel);
   pgis = MAKEPGINFOSEG(gsel);
   real_interval = pgis->cusecTimerInterval;
   user_callback = func;
   rc = DosCreateThread(&thread_callback,&timerThread,thread_stack + 0xFFC);
   if(!rc) rc = DosSetPrty(PRTYS_THREAD,PRTYC_TIMECRITICAL,PRTYD_MINIMUM,timerThread);
   if(!rc)
   {
       if(DosTimerStart(real_interval-1,biewSem,&timerID) == 0)
       {
          return real_interval;
       }
   }
   if(thread_stack) { free(thread_stack); thread_stack = 0; }
   return 0;
}
                             /* Restore time callback function to original
                                state */
void  __FASTCALL__ __OsRestoreTimer(void)
{
  RESULTCODES rcs;
  PID pid;
  if(timerID) { DosTimerStop(timerID); timerID = 0; }
  user_callback = 0;
  DosCwait(DCWA_PROCESS,DCWW_WAIT,&rcs,&pid,timerThread);
  if(thread_stack)
  {
    free(thread_stack);
    thread_stack = 0;
  }
}
