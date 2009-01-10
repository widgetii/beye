/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/dos4gw/vio.c
 * @brief       This file contains implementation of video subsystem handles for DOS4GW.
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
#include <dos.h>
#include <string.h>
#include "biewlib/biewlib.h"

tAbsCoord tvioWidth,tvioHeight;
unsigned tvioNumColors;
static char * viomem;
static unsigned long vio_flags;

void __FASTCALL__ __init_vio(const char *user_cp, unsigned long flg )
{
  union REGS reg;
  unsigned char i;
  unsigned char palettes[16] = { 0,1,2,3,4,5,20,7,56,57,58,59,60,61,62,63 };
  reg.x.eax = 0x1003;
  reg.x.ebx = 0;
  /* Background intensity enabled */
  int386(0x10,&reg,&reg);
  for(i = 0;i < 16;i++)
  {
    reg.x.eax = 0x1000;
    reg.h.bl = i;
    reg.h.bh = palettes[i];
    /* Set all palette registers to default values */
    int386(0x10,&reg,&reg);
  }
  vio_flags = flg;
  __vioRereadState();
}

void __FASTCALL__ __term_vio( void )
{
  union REGS reg;
  reg.x.eax = 0x1003;
  reg.x.ebx = 0x0001;
  /* Blinking enabled */
  int386(0x10,&reg,&reg);
}


/*
   Watcom manual says:
       Under DOS/4GW, the first megabyte of physical memory
       (real-mode memory) is mapped as a shared linear address
       space. This allows your application to access video RAM
       using its linear address.  The DOS segment:offset of
       B800:0000 corresponds to a linear address of B8000.
*/

void __FASTCALL__ __vioRereadState( void )
{
  unsigned short *tvw;
  unsigned char *tvh;
  union REGS reg;
  reg.h.ah = 0x0F;
  int386(0x10,&reg,&reg);
  tvioNumColors = 16;
  tvw = (void *)0x044A;
  tvh = (void *)0x0484;
  tvioWidth = *tvw;
  tvioHeight = (*tvh)+1;
  switch(reg.h.al)
  {
     case 7: viomem = (char *)0xB0000;
             tvioNumColors = 2;
             break;
     default: viomem = (char *)0xB8000;
  }
}

int __FASTCALL__ __vioGetCursorType( void )
{
  int ret;
  union REGS reg;
  unsigned char mode;
  memset(&reg,0,sizeof(union REGS));
  reg.h.ah = 0x0F;
  int386(0x10,&reg,&reg);
  mode = reg.h.al;
  reg.h.ah = 3;
  int386(0x10,&reg,&reg);
  if(reg.x.ecx == 0x2000) ret = __TVIO_CUR_OFF;
  else
    if(tvioHeight >= 43 && reg.x.ecx == 0x08) ret = __TVIO_CUR_SOLID;
    else
      if(tvioHeight < 43 && mode == 7 && reg.x.ecx == 0x0C) ret = __TVIO_CUR_SOLID;
      else
        if(tvioHeight < 43 && mode != 7 && reg.x.ecx == 0x07) ret = __TVIO_CUR_SOLID;
        else ret = __TVIO_CUR_NORM;
  return ret;
}

/* type: 0 - off  1 - normal  2 - solid */
static int c_type;
void __FASTCALL__ __vioSetCursorType(int type)
{
  unsigned int mode;
  union REGS inreg,outreg;
  memset(&inreg,0,sizeof(inreg));
  inreg.h.ah = 0x0F;
  int386(0x10,&inreg,&outreg);
  mode = outreg.h.al;
  c_type = type;
  switch(type)
  {
    case __TVIO_CUR_OFF:
            inreg.x.ecx = 0x2000;
            break;
    case __TVIO_CUR_SOLID:
            if(tvioHeight >= 43) inreg.x.ecx = 0x08;
            else
            {
              inreg.h.ch = 0;
              inreg.h.cl = mode == 7 ? 0x0C : 0x07;
            }
            break;
     default:
            if(mode == 7) inreg.x.ecx = 0x0B0C;
            else
            {
              inreg.h.ch = 0x06;
              inreg.h.cl = tvioHeight >= 43 ? 0 : 7;
            }
  }
  inreg.h.ah = 1;
  int386(0x10,&inreg,&outreg);
}

void __FASTCALL__ __vioGetCursorPos(tAbsCoord *x,tAbsCoord *y)
{
  union REGS reg;
  memset(&reg,0,sizeof(union REGS));
  reg.h.ah = 3;
  int386(0x10,&reg,&reg);
  *x = reg.h.dl;
  *y = reg.h.dh;
}

void __FASTCALL__ __vioSetCursorPos(tAbsCoord x,tAbsCoord y)
{
   union REGS reg;
   memset(&reg,0,sizeof(union REGS));
   reg.h.ah = 2;
   reg.h.dh = y;
   reg.h.dl = x;
   int386(0x10,&reg,&reg);
}

#define calc_vioaddr(x,y) (viomem+(x+y*tvioWidth)*2)

void __FASTCALL__ __vioWriteBuff(tAbsCoord x,tAbsCoord y,const tvioBuff *buff,unsigned len)
{
  size_t i;
  union REGS reg,oreg;
  unsigned char apage;
  if((vio_flags & __TVIO_FLG_DIRECT_CONSOLE_ACCESS) != __TVIO_FLG_DIRECT_CONSOLE_ACCESS)
  {
    tAbsCoord cx,cy;
    int cctype;
    cctype = c_type;
    if(c_type != __TVIO_CUR_OFF)
    {
      __vioGetCursorPos(&cx,&cy);
      __vioSetCursorType(__TVIO_CUR_OFF);
    }
    memset(&reg,0,sizeof(union REGS));
    reg.h.ah = 0x0F;
    int386(0x10,&reg,&reg);
    apage = reg.h.bh;
    reg.h.ah = 0x09;
    reg.h.bh = apage;
    reg.x.ecx = 1;
    for(i = 0;i < len;i++)
    {
      __vioSetCursorPos(x++,y);
      reg.h.al = buff->chars[i];
      reg.h.bl = buff->attrs[i];
      int386(0x10,&reg,&oreg);
      if(x >= tvioWidth) { x = 0; y++; }
    }
    if(cctype != c_type)
    {
      __vioSetCursorPos(cx,cy);
      __vioSetCursorType(cctype);
    }
  }
  else
  {
    char *vioptr = calc_vioaddr(x,y);
    for(i = 0;i < len;i++)
    {
      *vioptr = buff->chars[i];
      vioptr++;
      *vioptr = buff->attrs[i];
      vioptr++;
    }
  }
}

void __FASTCALL__ __vioReadBuff(tAbsCoord x,tAbsCoord y,tvioBuff *buff,unsigned len)
{
  size_t i;
  union REGS reg,oreg;
  unsigned char apage;
  if((vio_flags & __TVIO_FLG_DIRECT_CONSOLE_ACCESS) != __TVIO_FLG_DIRECT_CONSOLE_ACCESS)
  {
    tAbsCoord cx,cy;
    int cctype;
    cctype = c_type;
    if(c_type != __TVIO_CUR_OFF)
    {
      __vioGetCursorPos(&cx,&cy);
      __vioSetCursorType(__TVIO_CUR_OFF);
    }
    memset(&reg,0,sizeof(union REGS));
    reg.h.ah = 0x0F;
    int386(0x10,&reg,&reg);
    apage = reg.h.bh;
    reg.h.ah = 0x08;
    reg.h.bh = apage;
    for(i = 0;i < len;i++)
    {
      __vioSetCursorPos(x++,y);
      int386(0x10,&reg,&oreg);
      buff->chars[i] = oreg.h.al;
      buff->attrs[i] = oreg.h.ah;
      if(x >= tvioWidth) { x = 0; y++; }
    }
    if(cctype != c_type)
    {
      __vioSetCursorPos(cx,cy);
      __vioSetCursorType(cctype);
    }
  }
  else
  {
    char *vioptr = calc_vioaddr(x,y);
    for(i = 0;i < len;i++)
    {
      buff->chars[i] = *vioptr;
      vioptr++;
      buff->attrs[i] = *vioptr;
      vioptr++;
    }
  }
}

void __FASTCALL__ __vioSetTransparentColor(unsigned char value)
{
}
