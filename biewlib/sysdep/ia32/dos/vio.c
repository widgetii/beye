/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/dos/vio.c
 * @brief       This file contains implementation of video subsystem handles for DOS-32.
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
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <go32.h>
#include <sys/farptr.h>

#include "biewlib/biewlib.h"

tAbsCoord tvioWidth,tvioHeight;
unsigned tvioNumColors;
static unsigned viomem;
static unsigned long vio_flags;
static unsigned h__my_ds,h__conv_sel;

void __FASTCALL__ __init_vio( unsigned long flg )
{
  union REGS reg;
  unsigned char palettes[16] = { 0,1,2,3,4,5,20,7,56,57,58,59,60,61,62,63 };
  unsigned char i;
  reg.x.ax = 0x1003;
  reg.x.bx = 0;
  /* Background intensity enabled */
  int86(0x10,&reg,&reg);
  reg.x.ax = 0x1000;
  for(i = 0;i < 16;i++)
  {
    reg.h.bl = i;
    reg.h.bh = palettes[i];
    /* Set all palette registers to default values */
    int86(0x10,&reg,&reg);
  }
  h__my_ds = _go32_my_ds();
  h__conv_sel = _go32_conventional_mem_selector();
  vio_flags = flg;
  __vioRereadState();
}

void __FASTCALL__ __term_vio( void )
{
  union REGS reg;
  reg.x.ax = 0x1003;
  reg.x.bx = 0x0001;
  /* Blinking enabled */
  int86(0x10,&reg,&reg);
}

void __FASTCALL__ __vioRereadState( void )
{
  union REGS reg;
  reg.h.ah = 0x0F;
  int86(0x10,&reg,&reg);
  tvioNumColors = 16;
  tvioWidth = _farpeekw(_go32_conventional_mem_selector(),0x044A);
  tvioHeight = _farpeekb(_go32_conventional_mem_selector(),0x0484)+1;
  switch(reg.h.al)
  {
     case 7: viomem = 0xB0000;
             tvioNumColors = 2;
             break;
     default: viomem = 0xB8000;
  }
}

int __FASTCALL__ __vioGetCursorType( void )
{
  int ret;
  union REGS reg;
  unsigned char mode;
  memset(&reg,0,sizeof(union REGS));
  reg.h.ah = 0x0F;
  int86(0x10,&reg,&reg);
  mode = reg.h.al;
  reg.h.ah = 3;
  int86(0x10,&reg,&reg);
  if(reg.x.cx == 0x2000) ret = __TVIO_CUR_OFF;
  else
    if(tvioHeight >= 43 && reg.x.cx == 0x08) ret = __TVIO_CUR_SOLID;
    else
      if(tvioHeight < 43 && mode == 7 && reg.x.cx == 0x0C) ret = __TVIO_CUR_SOLID;
      else
        if(tvioHeight < 43 && mode != 7 && reg.x.cx == 0x07) ret = __TVIO_CUR_SOLID;
        else ret = __TVIO_CUR_NORM;
  return ret;
}

/* type: 0 - off  1 - normal  2 - solid */
static int c_type;
void __FASTCALL__ __vioSetCursorType(int type)
{
  union REGS inreg,outreg;
  unsigned char mode;
  memset(&inreg,0,sizeof(inreg));
  inreg.h.ah = 0x0F;
  int86(0x10,&inreg,&outreg);
  mode = outreg.h.al;
  c_type = type;
  switch(type)
  {
    case __TVIO_CUR_OFF:
            inreg.x.cx = 0x2000;
            break;
    case __TVIO_CUR_SOLID:
            if(tvioHeight >= 43) inreg.x.cx = 0x08;
            else
            {
              inreg.h.ch = 0;
              inreg.h.cl = mode == 7 ? 0x0C : 0x07;
            }
            break;
     default:
            if(mode == 7) inreg.x.cx = 0x0B0C;
            else
            {
              inreg.h.ch = 0x06;
              inreg.h.cl = tvioHeight >= 43 ? 0 : 7;
            }
  }
  inreg.h.ah = 1;
  int86(0x10,&inreg,&outreg);
}

void __FASTCALL__ __vioGetCursorPos(tAbsCoord *x,tAbsCoord *y)
{
  union REGS reg;
  memset(&reg,0,sizeof(union REGS));
  reg.h.ah = 3;
  int86(0x10,&reg,&reg);
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
   int86(0x10,&reg,&reg);
}

#define calc_vioaddr(x,y) (viomem+((x+y*tvioWidth)<<1))

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
    int86(0x10,&reg,&reg);
    apage = reg.h.bh;
    reg.h.ah = 0x09;
    reg.h.bh = apage;
    reg.x.cx = 1;    
    for(i = 0;i < len;i++)
    {
      __vioSetCursorPos(x++,y);
      reg.h.al = buff->chars[i];
      reg.h.bl = buff->attrs[i];
      int86(0x10,&reg,&oreg);
      if(x >= tvioWidth) { x = 0; y++; }
    }
    if(cctype != c_type)
    {
      __vioSetCursorPos(cx,cx);
      __vioSetCursorType(cctype);
    }
  }
  else
  {
    tUInt16 *resbuff, small_buffer[__TVIO_MAXSCREENWIDTH];
    if(len > tvioWidth)
    {
      if(!(resbuff = malloc(sizeof(tUInt16)*len)))
      {
        printm("Memory allocation failed: %s\nExiting..", strerror(errno));
        exit(EXIT_FAILURE);
      } 
    }
    else resbuff = small_buffer;
    __INTERLEAVE_BUFFERS(len, resbuff, buff->chars, buff->attrs);
    movedata(h__my_ds,(unsigned)resbuff,
             h__conv_sel,calc_vioaddr(x,y),
             len<<1);
    if(resbuff != small_buffer) free(resbuff);
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
    int86(0x10,&reg,&reg);
    apage = reg.h.bh;
    reg.h.ah = 0x08;
    reg.h.bh = apage;
    for(i = 0;i < len;i++)
    {
      __vioSetCursorPos(x++,y);
      int86(0x10,&reg,&oreg);
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
    tUInt16 *resbuff, small_buffer[__TVIO_MAXSCREENWIDTH];
    if(len > tvioWidth)
    {
      if(!(resbuff = malloc(sizeof(tUInt16)*len)))
      {
        printm("Memory allocation failed: %s\nExiting..", strerror(errno));
        exit(EXIT_FAILURE);
      } 
    }
    else resbuff = small_buffer;
    movedata(h__conv_sel,calc_vioaddr(x,y),
             h__my_ds,(unsigned)resbuff,
             len<<1);
    for(i = 0;i < len;i++)
    {
      buff->attrs[i] = ((tUInt8 *)resbuff)[i+i+1];
      buff->chars[i] = ((tUInt8 *)resbuff)[i+i];
    }
    if(resbuff != small_buffer) free(resbuff);
  }
}

void __FASTCALL__ __vioSetTransparentColor(unsigned char value)
{
}
