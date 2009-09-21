/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/win32/timer.c
 * @brief       This file contains implementation of timer depended part for Win32
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
**/
/* for cygwin - remove unnecessary includes */
#define _OLE_H
#define _OLE2_H
#include <windows.h>
#include <stddef.h>
#include <stdlib.h>

#include "biewlib/biewlib.h"

#if defined(__GNUC__) && !defined(_MMSYSTEM_H) && __MACHINE__!=x86_64
/****************************************************************\
* Cygnus GNU C/C++ v0.20b does not have 'mmsystem.h' header file *
\****************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/** timer device capabilities data structure */
typedef struct timecaps_tag {
    UINT    wPeriodMin;     /**< minimum period supported  */
    UINT    wPeriodMax;     /**< maximum period supported  */
    } TIMECAPS;
typedef TIMECAPS       *PTIMECAPS;
typedef TIMECAPS       *LPTIMECAPS;

UINT WINAPI timeGetDevCaps(TIMECAPS FAR* lpTimeCaps, UINT uSize);

/** timer data types */
typedef void (CALLBACK TIMECALLBACK) (UINT uTimerID,
                                      UINT uMessage,
                                      DWORD dwUser,
                                      DWORD dw1,
                                      DWORD dw2);

typedef TIMECALLBACK FAR *LPTIMECALLBACK;

/* flags for wFlags parameter of timeSetEvent() function */
#define TIME_ONESHOT    0   /**< program timer for single event */
#define TIME_PERIODIC   1   /**< program for continuous periodic event */

extern UINT WINAPI timeSetEvent( UINT uDelay,
                                 UINT uResolution,
                                 LPTIMECALLBACK lpTimeProc,
                                 DWORD dwUser,
                                 UINT fuEvent );

extern UINT WINAPI timeKillEvent( UINT uTimerID );
extern UINT WINAPI timeBeginPeriod(UINT uPeriod);
extern UINT WINAPI timeEndPeriod(UINT uPeriod);

#ifdef __cplusplus
}
#endif
#endif

static UINT uTimerID = 0;
static UINT uPeriod = 0;
static timer_callback *user_callback = NULL;

static VOID CALLBACK my_callback( UINT _uTimerID, UINT uMessage, DWORD dwUser, DWORD dw1, DWORD dw2 )
{
  UNUSED(_uTimerID);
  UNUSED(uMessage);
  UNUSED(dwUser);
  UNUSED(dw1);
  UNUSED(dw2);
  if(user_callback) (*user_callback)();
}

unsigned   __FASTCALL__ __OsSetTimerCallBack(unsigned ms,timer_callback *func)
{
  UINT cur_period;
  TIMECAPS tc;
   if(!uTimerID)
   {
     user_callback = func;
     if(timeGetDevCaps(&tc,sizeof(TIMECAPS)) == 0)
     {
       cur_period = tc.wPeriodMin;
       uPeriod = ms / cur_period;
       if(uPeriod < ms) uPeriod += cur_period-1;
       if(uPeriod > tc.wPeriodMax) uPeriod = tc.wPeriodMax-1;
       if(timeBeginPeriod(uPeriod) == 0)
       {
         uTimerID = timeSetEvent(uPeriod,0,my_callback,0L,TIME_PERIODIC);
         return uPeriod;
       }
     }
   }
   return 0;
}
                             /* Restore time callback function to original
                                state */
void   __FASTCALL__ __OsRestoreTimer(void)
{
  if(uTimerID)
  {
    timeEndPeriod(uPeriod);
    timeKillEvent(uTimerID);
    uTimerID = 0;
  }
}
