/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/os2/vio.c
 * @brief       This file contains implementation of video subsystem handles for OS/2.
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
#define INCL_SUB
#define INCL_DOSSIGNALS
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include "biewlib/biewlib.h"

tAbsCoord tvioWidth,tvioHeight;
unsigned tvioNumColors;
VIOMODEINFO viosaved;
VIOINTENSITY vioIntens;
static unsigned long vio_flags;
static unsigned char palettes[16] = { 0,1,2,3,4,5,20,7,56,57,58,59,60,61,62,63 };

#include <stdio.h>
void __FASTCALL__ __init_vio(const char *user_cp, unsigned long flg )
{
  unsigned i;
  VIOINTENSITY vioi;
  VIOPALSTATE  vpal;
    vioIntens.cb = sizeof(VIOINTENSITY);
    vioIntens.type = 2;
    VioGetState(&vioIntens,0);
    vioi.cb = sizeof(VIOINTENSITY);
    vioi.type = 2;
    vioi.fs = 1;
    VioSetState(&vioi,0);
    vpal.cb = sizeof(VIOPALSTATE);
    vpal.type = 0;
    for(i = 0;i < 16;i++)
    {
      unsigned char pals;
      pals = palettes[i];
      vpal.iFirst = i;
      VioGetState(&vpal,0);
      palettes[i] = vpal.acolor[0];
      vpal.acolor[0] = pals;
      VioSetState(&vpal,0);
    }
    vio_flags = flg;
    __vioRereadState();
}

void __FASTCALL__ __term_vio( void )
{
  unsigned i;
  VIOPALSTATE vpal;
    VioSetState(&vioIntens,0);
    vpal.cb = sizeof(VIOPALSTATE);
    vpal.type = 0;
    for(i = 0;i < 16;i++)
    {
      vpal.iFirst = i;
      vpal.acolor[0] = palettes[i];
      VioSetState(&vpal,0);
    }
}

void __FASTCALL__ __vioRereadState( void )
{
    int i;
    VIOMODEINFO vminfo;
    memset(&vminfo,0,sizeof(VIOMODEINFO));
    viosaved.cb = sizeof(VIOMODEINFO);
    vminfo.cb = sizeof(VIOMODEINFO);
    VioGetMode(&vminfo,0);
    tvioWidth = vminfo.col;
    tvioHeight = vminfo.row;
    tvioNumColors = 1;
    for(i = 0;i < vminfo.color;i++) tvioNumColors *= 2;
}

int __FASTCALL__ __vioGetCursorType( void )
{
  int hcell,ret;
  VIOMODEINFO vminfo;
  VIOCURSORINFO vcinfo;
  vminfo.cb = sizeof(VIOMODEINFO);
  VioGetMode(&vminfo,0);
  hcell = vminfo.vres / vminfo.row;
  VioGetCurType(&vcinfo,0);
  if((vcinfo.yStart == 0 && vcinfo.cEnd == 0) || ((SHORT)vcinfo.attr) == -1) ret = __TVIO_CUR_OFF;
  else
    if(vcinfo.yStart == 0 && vcinfo.cEnd == hcell-1) ret = __TVIO_CUR_SOLID;
    else ret = __TVIO_CUR_NORM;
  return ret;
}

/* type: 0 - off  1 - normal  2 - solid */
void __FASTCALL__ __vioSetCursorType(int type)
{
  int hcell; /* height of cell */
  VIOMODEINFO vminfo;
  VIOCURSORINFO vcinfo;

  vminfo.cb = sizeof(VIOMODEINFO);
  VioGetMode(&vminfo,0);
  hcell = vminfo.vres / vminfo.row;
  VioGetCurType(&vcinfo,0);
  vcinfo.attr = 0;
  switch(type)
  {
    case __TVIO_CUR_OFF:
            vcinfo.yStart = 0;
            vcinfo.cEnd = 0;
            vcinfo.attr = -1;
            vcinfo.cx = 0;
            break;
    case __TVIO_CUR_SOLID:
            vcinfo.yStart = 0;
            vcinfo.cEnd = hcell-1;
            break;
     default:
            vcinfo.yStart = hcell-2;
            vcinfo.cEnd = hcell-1;
  }
  VioSetCurType(&vcinfo,0);
}

void __FASTCALL__ __vioGetCursorPos(tAbsCoord *x,tAbsCoord *y)
{
  VioGetCurPos((USHORT *)&y,(USHORT *)&x,0);
}

void __FASTCALL__ __vioSetCursorPos(tAbsCoord x,tAbsCoord y )
{
  VioSetCurPos(y,x,0);
}

void __FASTCALL__ __vioWriteBuff(tAbsCoord x,tAbsCoord y,const tvioBuff *buff,unsigned len)
{
#if 1
/* I've found bugs in OS/2 on large arrays */
/* But Max Alekseyev <relf@os2.ru> swears that this block is working with Watcom */
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
  VioWrtCellStr((void *)resbuff,len<<1,y,x,0);
  if(resbuff != small_buffer) free(resbuff);
#else
  tUInt16 small_buffer[__TVIO_MAXSCREENWIDTH];
  size_t i,remaind;
  i = 0;
  while(len)
  {
    remaind = min(tvioWidth,len);
    __INTERLEAVE_BUFFERS(remaind, small_buffer, &buff->chars[i], &buff->attrs[i]);
    VioWrtCellStr((void *)small_buffer,remaind<<1,y,x,0);
    len -= remaind;
    i += tvioWidth;
    y++;
  }
#endif
}

void __FASTCALL__ __vioReadBuff(tAbsCoord x,tAbsCoord y,tvioBuff *buff,unsigned len)
{
#if 1
/* I've found bugs in OS/2 on large arrays */
/* But Max Alekseyev <relf@os2.ru> swears that this block is working with Watcom */
  size_t i,rsize;
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
  rsize = len << 1;
  VioReadCellStr((void *)resbuff,(USHORT *)&rsize,y,x,0);
  for(i = 0;i < len;i++)
  {
    buff->attrs[i] = ((tUInt8 *)resbuff)[i+i+1];
    buff->chars[i] = ((tUInt8 *)resbuff)[i+i];
  }
  if(resbuff != small_buffer) free(resbuff);
#else
  size_t i,j,remaind,rsize,idx;
  tUInt16 small_buffer[__TVIO_MAXSCREENWIDTH];
  j = 0;
  while(len)
  {
    remaind = min(tvioWidth,len);
    rsize = remaind << 1;
    VioReadCellStr((void *)small_buffer,(USHORT *)&rsize,y+j,x,0);
    for(i = 0;i < len;i++)
    {
      idx = j*tvioWidth+i;
      buff->attrs[idx] = ((tUInt8 *)small_buffer)[i+i+1];
      buff->chars[idx] = ((tUInt8 *)small_buffer)[i+i];
    }
    len -= remaind;
    j++;
  }
#endif
}

void __FASTCALL__ __vioSetTransparentColor(unsigned char value)
{
   UNUSED(value);
}
