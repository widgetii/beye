/**
 * @namespace   biew
 * @file        addendum.c
 * @brief       This file contains interface to Addons functions.
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
#include <stddef.h>
#include "bconsole.h"
#include "biewutil.h"
#include "reg_form.h"

extern REGISTRY_TOOL DigitalConvertor;
extern REGISTRY_TOOL Calculator;

static REGISTRY_TOOL *toolTable[] =
{
  &DigitalConvertor,
  &Calculator
};

static size_t defToolSel = 0;

void SelectTool( void )
{
  const char *toolName[sizeof(toolTable)/sizeof(REGISTRY_TOOL *)];
  size_t i,nTools;
  int retval;

  nTools = sizeof(toolTable)/sizeof(REGISTRY_TOOL *);
  for(i = 0;i < nTools;i++) toolName[i] = toolTable[i]->name;
  retval = SelBoxA(toolName,nTools," Select tool: ",defToolSel);
  if(retval != -1)
  {
    toolTable[retval]->tool();
    defToolSel = retval;
  }
}

void init_addons( void )
{
  size_t i;
  for(i = 0;i < sizeof(toolTable)/sizeof(REGISTRY_TOOL *);i++)
  {
    if(toolTable[i]->read_ini) toolTable[i]->read_ini();
  }
}

void term_addons( void )
{
  size_t i;
  for(i = 0;i < sizeof(toolTable)/sizeof(REGISTRY_TOOL *);i++)
  {
    if(toolTable[i]->save_ini) toolTable[i]->save_ini();
  }
}
