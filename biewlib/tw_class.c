/**
 * @namespace   biewlib
 * @file        biewlib/tw_class.c
 * @brief       This file contains implementation of classes for Text Window manager.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       2000
 * @note        Development, fixes and improvements
**/
#include <string.h>
#include <stdlib.h>
#include "biewlib/twin.h"

static linearArray *class_set = NULL;

static tCompare __FASTCALL__ comp_class(const void __HUGE__ *e1,const void __HUGE__ *e2)
{
  const TwClass __HUGE__ *t1, __HUGE__ *t2;
  t1 = (const TwClass __HUGE__ *)e1;
  t2 = (const TwClass __HUGE__ *)e2;
  return stricmp(t1->name, t2->name);
}

tBool __FASTCALL__ twcRegisterClass(const char *name, unsigned flags, twClassFunc method)
{
  TwClass newest;
  TwClass *exists = twcFindClass(name);
  if(!exists)
  {
     if(!class_set) class_set = la_Build(0,sizeof(TwClass),NULL);
     if(class_set)
     {
        newest.name = malloc(strlen(name)+1);
        if(newest.name)
        {
          strcpy(newest.name,name);
          newest.flags = flags;
          newest.method= method;
          if(!la_AddData(class_set,&newest, NULL))
          {
            free(newest.name);
            return False;
          }
          la_Sort(class_set, comp_class);
          return True;
        }
     }
  }
  return False;
}

static void __FASTCALL__ del_class(void __HUGE__ *it)
{
  const TwClass __HUGE__ *t1;
  t1 = (const TwClass __HUGE__ *)it;
  free(t1->name);
}

void __FASTCALL__ twcDestroyClassSet(void)
{
  if(class_set) la_IterDestroy(class_set, del_class);
}

TwClass * __FASTCALL__ twcFindClass(const char *name)
{
 TwClass key;
 key.name = name;
 return (TwClass *)la_Find(class_set,&key, comp_class);
}
