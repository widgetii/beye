/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/dos/timer.c
 * @brief       This file contains implementation of timer depended part for DOS.
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
#include <stddef.h>

#include "biewlib/biewlib.h"

static void * original_timer = NULL;
static timer_callback *user_callback = NULL;

static void interrupt lib_callback( void )
{
  if(user_callback) (*user_callback)();
}

unsigned  __FASTCALL__ __OsSetTimerCallBack(unsigned ms,timer_callback func)
{
   if(!original_timer)
      original_timer = (void *)getvect(0x1C);
   user_callback = func;
   setvect(0x1C,lib_callback);
   return ms = 54;
}
                             /* Restore time callback function to original
                                state */
void __FASTCALL__ __OsRestoreTimer(void)
{
  if(original_timer) setvect(0x1C,(void (interrupt *)())original_timer);
}
