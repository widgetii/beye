/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/dos/timer.c
 * @brief       This file contains implementation of timer depended part for DOS-32.
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
#include <go32.h>
#include <dpmi.h>
#include <stddef.h>

#include "biewlib/biewlib.h"

static __dpmi_paddr original_timer = { 0, 0 };
static timer_callback *user_callback = NULL;

extern unsigned __djgpp_ds_alias;

static void lib_callback( void )
{
  asm(".byte   0x1E"); /* push ds */
  asm(".byte   0x06"); /* push es */
  asm("pushal");
  asm(".byte   0x2E\n" /* cs: prefix */
      "movl    %0, %%eax"::"m"(__djgpp_ds_alias));
  asm(".byte   0x8E, 0xD8"); /* mov ds, ax */
  asm(".byte   0x8E, 0xC0"); /* mov es, ax */
  if(user_callback) (*user_callback)();
  asm("popal");
  asm(".byte   0x07"); /* pop es */
  asm(".byte   0x1F"); /* pop ds */
}

static void timer_handler( void )
{
  lib_callback();
  asm(".byte    0xCF"); /* iret method */
}

unsigned __FASTCALL__ __OsSetTimerCallBack(unsigned ms,timer_callback func)
{
   __dpmi_paddr new_timer;
   if(!original_timer.selector)
      __dpmi_get_protected_mode_interrupt_vector(0x1C,&original_timer);
   user_callback = func;
   new_timer.selector = _my_cs();
   new_timer.offset32 = (unsigned)&timer_handler;
   __dpmi_set_protected_mode_interrupt_vector(0x1C, &new_timer);
   return 54;
}

                             /* Restore time callback function to original
                                state */
void __FASTCALL__ __OsRestoreTimer(void)
{
  if(original_timer.selector)
   __dpmi_set_protected_mode_interrupt_vector(0x1C, &original_timer);
}


