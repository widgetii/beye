/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/win32/vio.c
 * @brief       This file contains implementation of video subsystem handles for Win32s.
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
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "biewlib/biewlib.h"

tAbsCoord tvioWidth,tvioHeight;
unsigned tvioNumColors;
static HANDLE hOut;
static unsigned long vio_flags;
extern tBool  win32_use_ansi;
static tBool is_winnt;
extern OSVERSIONINFO win32_verinfo;

void __FASTCALL__ __init_vio( unsigned long flg )
{
  hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  is_winnt = win32_verinfo.dwPlatformId == VER_PLATFORM_WIN32_NT;
  vio_flags = flg;
  __vioRereadState();
}

void __FASTCALL__ __term_vio( void )
{
}

void __FASTCALL__ __vioRereadState( void )
{
  CONSOLE_SCREEN_BUFFER_INFO csbinfo;
  win32_use_ansi = False;
  switch(GetConsoleOutputCP())
  {
     case 1251:  win32_use_ansi = True; break;
     default:    break;
  }
  GetConsoleScreenBufferInfo(hOut,&csbinfo);
  if((vio_flags & __TVIO_FLG_DIRECT_CONSOLE_ACCESS) == __TVIO_FLG_DIRECT_CONSOLE_ACCESS)
  {
    tvioWidth  = csbinfo.dwSize.X;
    tvioHeight = csbinfo.dwSize.Y;
  }
  else
  {
    tvioWidth  = csbinfo.srWindow.Right-csbinfo.srWindow.Left+1;
    tvioHeight = csbinfo.srWindow.Bottom-csbinfo.srWindow.Top+1;
  }
  tvioNumColors = 16;
}

int __FASTCALL__ __vioGetCursorType( void )
{
  CONSOLE_CURSOR_INFO cci;
  GetConsoleCursorInfo(hOut,&cci);
  if(cci.bVisible == FALSE) return __TVIO_CUR_OFF;
  if(cci.dwSize == 100) return __TVIO_CUR_SOLID;
  return __TVIO_CUR_NORM;
}

/* type: 0 - off  1 - normal  2 - solid */
void __FASTCALL__ __vioSetCursorType(int type)
{
  CONSOLE_CURSOR_INFO cci;
  GetConsoleCursorInfo(hOut,&cci);
  cci.bVisible = TRUE;
  switch(type)
  {
    case __TVIO_CUR_OFF: cci.bVisible = FALSE;
                  break;
    case __TVIO_CUR_SOLID:
                  cci.dwSize = 100;
                  break;
    default:
                  cci.dwSize = 20;
  }
  SetConsoleCursorInfo(hOut,&cci);
}

void __FASTCALL__ __vioGetCursorPos(tAbsCoord *x,tAbsCoord *y)
{
  CONSOLE_SCREEN_BUFFER_INFO csbinfo;
  GetConsoleScreenBufferInfo(hOut,&csbinfo);
  *x = csbinfo.dwCursorPosition.X;
  *y = csbinfo.dwCursorPosition.Y;
}

void __FASTCALL__ __vioSetCursorPos(tAbsCoord x,tAbsCoord y)
{
  COORD cc;
  cc.X = x;
  cc.Y = y;
  SetConsoleCursorPosition(hOut,cc);
}

/*
 * Here we support two versions of console i/o.
 * Because of some Win95 releases have bug in
 * - (Read)WriteConsoleOutput
 * But Win2000 (without SP) has bug in
 * - (Read)WriteConsoleOutputCharacter
 * - (Read)WriteConsoleOutputAttribute
 * functions. Sorry! Life ain't easy with MS.
*/
void __FASTCALL__ __vioWriteBuff(tAbsCoord x,tAbsCoord y,const tvioBuff *buff,unsigned len)
{
  size_t i;
  if(is_winnt)
  {
    COORD pos,size;
    SMALL_RECT sr;
    CHAR_INFO *obuff, small_buffer[__TVIO_MAXSCREENWIDTH];
    if(len > tvioWidth)
    {
      if(!(obuff = malloc(sizeof(CHAR_INFO)*len)))
      {
	printm("Memory allocation failed: %s\nExiting..", strerror(errno));
	exit(EXIT_FAILURE);
      } 
    }
    else obuff = small_buffer;
    for(i = 0;i < len;i++)
    {
/*
      unsigned char attr;
      unsigned newattr;
*/
      obuff[i].Char.AsciiChar = buff->chars[i];
      obuff[i].Attributes = buff->attrs[i];
/*
      This code is useful for generic platform
      ========================================
      attr = buff->attrs[i];
      newattr = 0;
      if(attr & 0x01) newattr |= FOREGROUND_BLUE;
      if(attr & 0x02) newattr |= FOREGROUND_GREEN;
      if(attr & 0x04) newattr |= FOREGROUND_RED;
      if(attr & 0x08) newattr |= FOREGROUND_INTENSITY;
      if(attr & 0x10) newattr |= BACKGROUND_BLUE;
      if(attr & 0x20) newattr |= BACKGROUND_GREEN;
      if(attr & 0x40) newattr |= BACKGROUND_RED;
      if(attr & 0x80) newattr |= BACKGROUND_INTENSITY;
      obuff[i].Attributes = newattr;
*/
    }
    pos.X = pos.Y = 0;
    size.X = min(len,tvioWidth);
    size.Y = (len/tvioWidth)+(len%tvioWidth?1:0);
    sr.Left = x;
    sr.Top = y;
    sr.Bottom = y+size.Y;
    sr.Right = x + size.X;
    WriteConsoleOutput(hOut,obuff,size,pos,&sr);
    if(obuff != small_buffer) free(obuff);
  }
  else
  {
    unsigned long w, ii;
    COORD cc;
    unsigned short *attr, *attr2, small_buffer[__TVIO_MAXSCREENWIDTH];
    if(len > tvioWidth)
    {
      if(!(attr = malloc(sizeof(unsigned short)*len)))
      {
	printm("Memory allocation failed: %s\nExiting..", strerror(errno));
	exit(EXIT_FAILURE);
      } 
    }
    else attr = small_buffer;
    cc.X = x;
    cc.Y = y;
    WriteConsoleOutputCharacter(hOut,buff->chars,len,cc,(LPDWORD)&w);
    __CHARS_TO_SHORTS(len, attr, buff->attrs);
    /* added by Kostya Nosov: */
    attr2=attr;
    ii=min(len, tvioWidth);
    for (i=0; i<len; i+=tvioWidth)
    {
        WriteConsoleOutputAttribute(hOut, attr2, ii, cc, (LPDWORD)&w);
        attr2 += ii;
        cc.Y++;
    } /* end of addition */
    if(attr != small_buffer) free(attr);
  }
}

void __FASTCALL__ __vioReadBuff(tAbsCoord x,tAbsCoord y,tvioBuff *buff,unsigned len)
{
  size_t i;
  if(is_winnt)
  {
    COORD pos,size;
    SMALL_RECT sr;
    CHAR_INFO *obuff, small_buffer[__TVIO_MAXSCREENWIDTH];
    if(len > tvioWidth)
    {
      if(!(obuff = malloc(sizeof(CHAR_INFO)*len)))
      {
	printm("Memory allocation failed: %s\nExiting..", strerror(errno));
	exit(EXIT_FAILURE);
      } 
    }
    else obuff = small_buffer;
    pos.X = pos.Y = 0;
    size.X = min(len,tvioWidth);
    size.Y = (len/tvioWidth)+(len%tvioWidth?1:0);
    sr.Left = x;
    sr.Top = y;
    sr.Bottom = y+size.Y;
    sr.Right = x + size.X;
    ReadConsoleOutput(hOut,obuff,size,pos,&sr);
    for(i = 0;i < len;i++)
    {
/*
      unsigned char attr;
      unsigned newattr;
*/
      buff->chars[i] = obuff[i].Char.AsciiChar;
      buff->attrs[i] = obuff[i].Attributes;
/*
      This code is useful for generic platform
      ========================================
      attr = obuff[i].Attributes;
      newattr = 0;
      if((attr & FOREGROUND_BLUE) == FOREGROUND_BLUE) newattr |= 0x01;
      if((attr & FOREGROUND_GREEN) == FOREGROUND_GREEN) newattr |= 0x02;
      if((attr & FOREGROUND_RED) == FOREGROUND_RED) newattr |= 0x04;
      if((attr & FOREGROUND_INTENSITY) == FOREGROUND_INTENSITY) newattr |= 0x08;
      if((attr & BACKGROUND_BLUE) == BACKGROUND_BLUE) newattr |= 0x10;
      if((attr & BACKGROUND_GREEN) == BACKGROUND_GREEN) newattr |= 0x20;
      if((attr & BACKGROUND_RED) == BACKGROUND_RED) newattr |= 0x40;
      if((attr & BACKGROUND_INTENSITY) == BACKGROUND_INTENSITY) newattr |= 0x80;
      buff->attrs[i] = newattr;
*/
    }
    if(obuff != small_buffer) free(obuff);
  }
  else
  {
    unsigned long r, ii;
    COORD cc;
    unsigned short *attr, *attr2, small_buffer[__TVIO_MAXSCREENWIDTH];
    if(len > tvioWidth)
    {
      if(!(attr = malloc(sizeof(unsigned short)*len)))
      {
	printm("Memory allocation failed: %s\nExiting..", strerror(errno));
	exit(EXIT_FAILURE);
      } 
    }
    else attr = small_buffer;
    cc.X = x;
    cc.Y = y;
    ReadConsoleOutputCharacter(hOut,buff->chars,len,cc,(LPDWORD)&r);
    /* added by Kostya Nosov: */
    attr2 = attr;
    ii = min(len, tvioWidth);
    for (i=0; i<len; i+=tvioWidth)
    {
        ReadConsoleOutputAttribute(hOut, attr2, ii, cc, (LPDWORD)&r);
        attr2 += ii;
        cc.Y++;
    } /* end of addition */
    __SHORTS_TO_CHARS(len, buff->attrs, attr);
    if(attr != small_buffer) free(attr);
  }
}

void __FASTCALL__ __vioSetTransparentColor(unsigned char value)
{
  UNUSED(value);
}
