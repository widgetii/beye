/**
 * @namespace   biew
 * @file        codeguid.c
 * @brief       This file contains code navigation routines.
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
 * @author      Kostya Nosov <k-nosov@yandex.ru>
 * @date        12.09.2000
 * @note        removing difference keys for same locations of jump
**/
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "bmfile.h"
#include "biewutil.h"
#include "plugins/disasm.h"
#include "bconsole.h"
#include "codeguid.h"
#include "reg_form.h"
#include "biewlib/biewlib.h"
#include "biewlib/twin.h"
#include "biewlib/pmalloc.h"
#include "biewlib/kbd_code.h"

#define BACK_ADDR_SIZE 256
#define GO_ADDR_SIZE   37

static unsigned long *BackAddr;
static int BackAddrPtr = -1;
static unsigned long *GoAddr;
static unsigned int  *GoLineNums;
static int GoAddrPtr = -1;
static unsigned char Alarm = 0;

extern int DisasmCurrLine;

char codeguid_image[] = "=>[X]";

tBool __FASTCALL__ initCodeGuider( void )
{
  tBool ret = False;
  BackAddr = PMalloc(sizeof(unsigned long)*BACK_ADDR_SIZE);
  if(BackAddr)
  {
    GoAddr = PMalloc(sizeof(unsigned long)*GO_ADDR_SIZE);
    if(!GoAddr) PFree(BackAddr);
    else
    {
      GoLineNums = PMalloc(sizeof(unsigned long)*GO_ADDR_SIZE);
      if(!GoLineNums) { PFree(BackAddr); PFree(GoAddr); }
      else ret = True;
    }
  }
  return ret;
}

void __FASTCALL__ termCodeGuider( void )
{
  PFree(GoLineNums);
  PFree(GoAddr);
  PFree(BackAddr);
}

   /*
      Added by "Kostya Nosov" <k-nosov@yandex.ru>:
      for removing difference keys for same locations of jump
   */

static char __FASTCALL__ gidGetAddressKey( unsigned index )
{
  int i,j;
  char key;
  tBool found;
  unsigned long addr1,addr2;
  key = 0;
  addr1 = GoAddr[index];
  for (i = 0;i <= GoAddrPtr;i++)
  {
    addr2 = GoAddr[i];
    if (addr2 == addr1) break;
    /* check for presence addr2 above */
    found = False;
    for (j = 0;j < i;j++)
    {
      if (GoAddr[j] == addr2)
      {
        found = True;
        break;
      }
    }
    if (!found) key++;
  }
  return key < 10 ? key + '0' : key - 10 + 'A';
}

static int __FASTCALL__ gidGetKeyIndex( char key )
{
  int res,i,j,index;
  tBool found;
  unsigned long addr;
  if (key > 'Z') key = key - 'z' + 'Z';
  key = key > '9' ? key - 'A' + 10 : key - '0';
  res = GoAddrPtr + 1;
  index = 0;
  for (i = 0;i <= GoAddrPtr;i++)
  {
    addr = GoAddr[i];
    /* check for presence addr above */
    found = False;
    for (j = 0;j < i;j++)
    {
      if (GoAddr[j] == addr)
      {
        found = True;
        break;
      }
    }
    if (i > 0 && !found) index++;
    if (index == key)
    {
      res = i;
      break;
    }
  }
  return res;
}

static char * __NEAR__ __FASTCALL__ gidBuildKeyStr( void )
{
  codeguid_image[3] = gidGetAddressKey(GoAddrPtr);
  return codeguid_image;
}

void __FASTCALL__ GidResetGoAddress( int keycode )
{
  Alarm = 0;
  if(keycode == KE_DOWNARROW)
  {
    int i;
    if(GoAddrPtr >= 0)
    {
      if(GoLineNums[0] == 0)
      {
        memmove(GoAddr,&GoAddr[1],GoAddrPtr*sizeof(unsigned long));
        memmove(GoLineNums,&GoLineNums[1],GoAddrPtr*sizeof(unsigned int));
        GoAddrPtr--;
      }
      for(i = 0;i <= GoAddrPtr;i++)
      {
        char dig;
        GoLineNums[i]--;
        dig = gidGetAddressKey(i);
        twDirectWrite(twGetClientWidth(MainWnd)-1,GoLineNums[i]+2,&dig,1);
      }
    }
  }
  else if(keycode == KE_UPARROW)
       {
          int i;
          Alarm = UCHAR_MAX;
          if(GoAddrPtr >= 0)
          {
            for(i = 0;i <= GoAddrPtr;i++) GoLineNums[i]++;
            if(GoLineNums[GoAddrPtr] >= twGetClientHeight(MainWnd)) GoAddrPtr--;
          }
       }
       else GoAddrPtr = -1;
}

extern tBool DisasmPrepareMode;

void __FASTCALL__ GidAddGoAddress(char *str,unsigned long addr)
{
  tAbsCoord width = twGetClientWidth(MainWnd);
  unsigned bytecodes=activeDisasm->max_insn_len()*2;
  int len,where;
  if(DisasmPrepareMode) return;
  len = strlen((char *)str);
  where = (disPanelMode == PANMOD_FULL ? width :
           disPanelMode == PANMOD_MEDIUM ? width-10 : width-11-bytecodes) - 5;
  if(Alarm)
  {
     int i;
      if(GoAddrPtr < GO_ADDR_SIZE - 2) GoAddrPtr++;
      memmove(&GoAddr[1],&GoAddr[0],GoAddrPtr*sizeof(long));
      memmove(&GoLineNums[1],&GoLineNums[0],GoAddrPtr*sizeof(int));
      GoAddr[0] = addr;
      GoLineNums[0] = DisasmCurrLine;
      if(len < where)
      {
        memset(&str[len],TWC_DEF_FILLER,where-len);
        str[where] = 0;
      }
      strcat(str,codeguid_image);
      str[where + 3] = '0';
      for(i = 1;i <= GoAddrPtr;i++)
      {
        char dig;
        dig = gidGetAddressKey(i);
        twDirectWrite(width-1,GoLineNums[i]+1,&dig,1);
      }
  }
  else
  if(GoAddrPtr < GO_ADDR_SIZE - 2)
  {
     GoAddrPtr++;
     GoAddr[GoAddrPtr] = addr;
     GoLineNums[GoAddrPtr] = DisasmCurrLine;
     if(len < where)
     {
        memset(&str[len],TWC_DEF_FILLER,where-len);
        str[where] = 0;
     }
     strcpy((char *)&str[where],(char *)gidBuildKeyStr());
  }
}

void __FASTCALL__ GidAddBackAddress( void )
{
  if(BackAddrPtr >= BACK_ADDR_SIZE - 2)
  {
      memmove(BackAddr,&BackAddr[1],BackAddrPtr);
      BackAddrPtr--;
  }
  BackAddrPtr++;
  BackAddr[BackAddrPtr] = BMGetCurrFilePos();
}

unsigned long __FASTCALL__ GidGetGoAddress(unsigned keycode)
{
  unsigned long ret;
  if(keycode == KE_BKSPACE)
       ret = BackAddrPtr >= 0 ? BackAddr[BackAddrPtr--] : BMGetCurrFilePos();
  else
  {
      int ptr;
      keycode &= 0x00FF;
      ptr = gidGetKeyIndex(keycode);
      if(ptr <= GoAddrPtr)
      {
        GidAddBackAddress();
        ret = GoAddr[ptr];
      }
      else
        ret = BMGetCurrFilePos();
  }
  return ret;
}

char * __FASTCALL__ GidEncodeAddress(unsigned long cfpos,tBool AddressDetail)
{
  static char addr[11];
  strcpy(addr,Get8Digit(cfpos));
  if(AddressDetail && detectedFormat->AddressResolving)
       detectedFormat->AddressResolving(addr,cfpos);
  strcat(addr,": ");
  return addr;
}
