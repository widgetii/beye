/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/dos/misc.c
 * @brief       Misc. functions for DOS (optional!!!)
 * @version     -
 * @author      Nick Kurshev
 * @date        2003
**/
#include <bios.h>
#include <stdio.h>
#include <string.h>

#include "biewlib/biewlib.h"

int __FASTCALL__ __inputRawInfo(char *head, char *text)
{
  int avail_kbd;
  int rval, rval2;
  const char *type;
  avail_kbd = _bios_keybrd(9);
  if(avail_kbd&0x40)
  {
    rval = _bios_keybrd(0x20);
    rval2 = _bios_keybrd(0x22);
    type = "122-kbd";
  }
  else
  if(avail_kbd&0x20)
  {
    rval = _bios_keybrd(0x10);
    rval2 = _bios_keybrd(0x12);
    type = "enh-kbd";
  }
  else
  {
    rval = _bios_keybrd(0x0);
    rval2 = _bios_keybrd(0x2);
    type = "std-kbd";
  }
  strcpy(head,"Type    Keycode Shifts");
  sprintf(text,"%s %04X      %04X", type, rval, rval2);
  return rval==0x011B?0:1;
}
