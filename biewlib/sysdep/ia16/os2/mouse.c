/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/os2/mouse.c
 * @brief       This file contains implementation of mouse handles for OS/2.
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
#define INCL_DOS
#define INCL_MOU
#include <os2.h>
#include <limits.h>

#include "biewlib/biewlib.h"

static HMOU mouHandle;
static int mou_btns;
static tBool ms_visible = False;
static USHORT mouStatus;

int __FASTCALL__ __init_mouse( void )
{
   USHORT rc,nbtn,status;
   rc = MouOpen(NULL,&mouHandle);
   if(rc) { mouHandle = 0; return INT_MAX; }
   MouGetNumButtons(&nbtn,mouHandle);
   MouGetDevStatus(&mouStatus,mouHandle);
   status = 0x200;
   MouSetDevStatus(&status,mouHandle);
   mou_btns = nbtn;
   MouDrawPtr(mouHandle);
   return nbtn;
}

void __FASTCALL__ __term_mouse( void )
{
  __MsSetState(False);
  MouSetDevStatus(&mouStatus,mouHandle);
  if(mouHandle) MouClose(mouHandle);
  mouHandle = 0;
}

tBool __FASTCALL__ __MsGetState( void )
{
  return ms_visible;
}

void __FASTCALL__ __MsSetState( tBool state )
{
  switch(state)
  {
    case True:
     if(!ms_visible)
     {
       MouDrawPtr(mouHandle);
       ms_visible=True;
     }
     break;
    default:
    case False:
    {
      NOPTRRECT mrect;
      if(ms_visible)
      {
        mrect.row = mrect.col = 0;
        mrect.cCol = tvioWidth - 1;
        mrect.cRow = tvioHeight - 1;
        MouRemovePtr(&mrect,mouHandle);
        ms_visible=False;
      }
    }
  }
}

#define PIXEL_PER_CHAR 8

void __FASTCALL__ __MsGetPos(tAbsCoord *mx,tAbsCoord *my )
{
  USHORT devstatus;
  PTRLOC ploc;
  *mx = *my = 0;
  if(mouHandle)
  {
    MouGetPtrPos(&ploc,mouHandle);
    MouGetDevStatus(&devstatus,mouHandle);
    *mx = ploc.col;
    *my = ploc.row;
    if(!(devstatus & 0x0200))
    {
      *mx = ploc.col/PIXEL_PER_CHAR;
      *my = ploc.row/PIXEL_PER_CHAR;
    }
  }
}

int __FASTCALL__ __MsGetBtns( void )
{
  static int ret;
  USHORT fwait;
  MOUQUEINFO mqinfo;
  static MOUEVENTINFO minfo;
  ret = 0;
  if(mouHandle)
  {
    fwait = 0;
    while(1)
    {
      MouGetNumQueEl(&mqinfo,mouHandle);
      if(mqinfo.cEvents < 1) break;
      MouReadEventQue(&minfo,&fwait,mouHandle);
    }
    if((minfo.fs & 0x04) == 0x04 || (minfo.fs & 0x02) == 0x02) ret |= MS_LEFTPRESS;
    if((minfo.fs & 0x10) == 0x10 || (minfo.fs & 0x08) == 0x08)
    {
       if(mou_btns == 2) ret |= MS_RIGHTPRESS;
       else              ret |= MS_MIDDLEPRESS;
    }
    if((minfo.fs & 0x40) == 0x40 || (minfo.fs & 0x20) == 0x20) ret |= MS_RIGHTPRESS;
  }
  return ret;
}
