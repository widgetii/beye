/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/generic/cpu_info.c
 * @brief       This file contains generic function emplementation for retrieving
 *              CPU information.
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
#include <stdio.h>

void __FillCPUInfo(char *buff,unsigned cbBuff,void (*func)(int))
{
  (*func)(100);
  sprintf(buff,"\n\n\n\n\n\n\n\nCan not work on generic platform\n");
  buff[cbBuff-1] = '\0';
}
