/**
   Test of .ini file library.
   Written by Nick Kurshev 2000
   Use, distribute, and modify this program freely.
   This subset version is hereby donated to the public domain
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "biewlib/file_ini.h"

char **ArgVector;
unsigned ArgCount;

static tBool __FASTCALL__ MyProc(IniInfo * info)
{
  printf("[ %s ] < %s > %s = %s\n"
         ,info->section
         ,info->subsection
         ,info->item
         ,info->value);
  return 0;
}

int main( int argc, char * argv[] )
{
  if(argc < 2)
  {
     printf("Too few\n\r");
     return EXIT_FAILURE;
  }
  __init_sys();
  FiProgress(argv[1],MyProc);
  __term_sys();
  return EXIT_SUCCESS;
}

