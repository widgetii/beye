/**
 * @namespace   biew
 * @file        refs.c
 * @brief       This file contains basic level routines for resolving references.
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "reg_form.h"
#include "biewutil.h"
#include "bconsole.h"

extern REGISTRY_BIN binTable;
unsigned long __FASTCALL__ AppendAsmRef(char *str,unsigned long ulShift,int mode,char codelen,unsigned long r_sh)
{
  static tBool warn_displayed = False;
  unsigned long ret = RAPREF_NONE;
  if(detectedFormat->bind) ret = detectedFormat->bind(str,ulShift,mode,codelen,r_sh);
  else
  {
    if(detectedFormat != &binTable && !warn_displayed)
    {
      WarnMessageBox("Sorry! Reference resolving for this format is still not supported",NULL);
      warn_displayed = True;
    }
  }
  return ret;
}
