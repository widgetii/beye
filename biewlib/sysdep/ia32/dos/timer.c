/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/dos/timer.c
 * @brief       This file contains implementation of timer depended part for DOS-32.
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
#include <go32.h>
#include <dpmi.h>
#include <stddef.h>

#include "biewlib/biewlib.h"

static timer_callback *user_callback = NULL;

static void timer_handler( void )
{
  if(user_callback) (*user_callback)();
}

unsigned __FASTCALL__ __OsSetTimerCallBack(unsigned ms,timer_callback func)
{
   _go32_dpmi_seginfo si;
   si.pm_offset=(unsigned)&timer_handler;
   user_callback = func;
   _go32_dpmi_chain_protected_mode_interrupt_vector(0x1C,&si);
   return 54;
}

                             /* Restore time callback function to original
                                state */
void __FASTCALL__ __OsRestoreTimer(void)
{
  user_callback = NULL;
}


