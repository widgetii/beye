/**
 * @namespace   biew
 * @file        biewutil.c
 * @brief       This file contains useful primitives of BIEW project.
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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>

#include "bmfile.h"
#include "biewutil.h"
#include "bconsole.h"
#include "tstrings.h"
#include "biewlib/biewlib.h"
#include "biewlib/pmalloc.h"

tBool DumpMode = False;
tBool EditMode = False;

int __FASTCALL__ GetBool(tBool _bool) { return _bool ? TWC_CHECK_CHAR : TWC_DEF_FILLER; }

void FFreeArr(void ** arr,unsigned n)
{
  unsigned i;
  for(i = 0;i < n;i++) PFREE(arr[i]);
}

unsigned __FASTCALL__ Summ(unsigned char *str,unsigned size)
{
  unsigned res,i;
  res = 0;
  for(i = 0;i < size;i++) res += str[i];
  return res;
}

char * __FASTCALL__ GetBinary(char val)
{
  static char bstr[9];
  int i;
  bstr[8] = 0;
  for(i = 0;i < 8;i++) bstr[7-i] = ((val >> i) & 1) + '0';
  return bstr;
}

#define GET2DIGIT(str,legs,val)\
{\
  char *s = (char *)str;\
  s[0] = legs[(((unsigned char)val) >> 4) & 0x0F];\
  s[1] = legs[((unsigned char)val) & 0x0F];\
}

char * __FASTCALL__ Get2Digit(tUInt8 val)
{
  static char str[3] = "  ";
  const char *legs = &legalchars[2];
  GET2DIGIT(str,legs,val);
  return str;
}

char * __FASTCALL__ Get2SignDig(tInt8 val)
{
  static char str[4] = "   ";
  const char *legs = &legalchars[2];
  str[0] = val >= 0 ? '+' : '-';
  if(val < 0) val = abs(val);
  GET2DIGIT(&str[1],legs,val);
  return str;
}

char * __FASTCALL__ Get4Digit(tUInt16 val)
{
  static char rstr[5];
  const char *legs = &legalchars[2];
  unsigned char v;
  v = val>>8;
  GET2DIGIT(rstr,legs,v);
  GET2DIGIT(&rstr[2],legs,val);
  return rstr;
}

char * __FASTCALL__ Get4SignDig(tInt16 val)
{
  static char rstr[6];
  const char *legs = &legalchars[2];
  unsigned char v;
  rstr[0] = val >= 0 ? '+' : '-';
  if(val < 0) val = abs(val);
  v = val>>8;
  GET2DIGIT(&rstr[1],legs,v);
  GET2DIGIT(&rstr[3],legs,val);
  return rstr;
}

char * __FASTCALL__ Get8Digit(tUInt32 val)
{
  static char rstr[9];
  const char *legs = &legalchars[2];
  unsigned char v;
  v = val>>24;
  GET2DIGIT(rstr,legs,v);
  v = val>>16;
  GET2DIGIT(&rstr[2],legs,v);
  v = val>>8;
  GET2DIGIT(&rstr[4],legs,v);
  GET2DIGIT(&rstr[6],legs,val);
  return rstr;
}

char * __FASTCALL__ Get8SignDig(tInt32 val)
{
  static char rstr[10];
  const char *legs = &legalchars[2];
  unsigned char v;
  rstr[0] = val >= 0 ? '+' : '-';
  if(val < 0) val = labs(val);
  v = val>>24;
  GET2DIGIT(&rstr[1],legs,v);
  v = val>>16;
  GET2DIGIT(&rstr[3],legs,v);
  v = val>>8;
  GET2DIGIT(&rstr[5],legs,v);
  GET2DIGIT(&rstr[7],legs,val);
  return rstr;
}

static char __NEAR__ __FASTCALL__ GetHexAnalog(char val)
{
  return val >= '0' && val <= '9' ? val-'0' : ((toupper(val)-'A'+10)) & 0x0F;
}

void __FASTCALL__ CompressHex(unsigned char * dest,const char * src,int sizedest,tBool usespace)
{
  int i,j;
  for(i = j = 0;j < sizedest;j++)
  {
      dest[j] = (GetHexAnalog(src[i++]) << 4) | GetHexAnalog(src[i++]);
      if(usespace) i++;
  }
}

memArray * __FASTCALL__ ma_Build( int nitems, tBool interact )
{
  memArray * ret;
  ret = PMalloc(sizeof(memArray));
  if(ret)
  {
    memset(ret,0,sizeof(memArray));
    if(nitems)
    {
      ret->data = PMalloc(nitems*sizeof(char *));
      if(ret->data)
      {
        ret->nSize = nitems;
      }
    }
  }
  else
  {
    if(interact) MemOutBox("List creation");
  }
  return ret;
}

void  __FASTCALL__ ma_Destroy(memArray *obj)
{
  unsigned i;
  for(i = 0;i < obj->nItems;i++)
  {
    PFREE(obj->data[i]);
  }
  PFREE(obj->data);
  PFREE(obj);
}

#define LST_STEP 16

tBool  __FASTCALL__ ma_AddData(memArray *obj,const void *udata,unsigned len,tBool interact)
{
  char *new_item;
  if(obj->nSize > UINT_MAX - (LST_STEP+1)) return 0;
  if(obj->nItems + 1 > obj->nSize)
  {
    void *ptr;
    if(!obj->data) ptr = PMalloc((obj->nSize+LST_STEP)*sizeof(char *));
    else           ptr = PRealloc(obj->data,sizeof(char *)*(obj->nSize+LST_STEP));
    if(ptr)
    {
      obj->nSize = obj->nSize+LST_STEP;
      obj->data = ptr;
    }
    else goto err;
  }
  new_item = PMalloc(len);
  if(new_item)
  {
    memcpy(new_item,udata,len);
    obj->data[obj->nItems++] = new_item;
    return True;
  }
  else
  {
    err:
    if(interact)
    {
      MemOutBox("Building list");
    }
  }
  return False;
}

tBool __FASTCALL__ ma_AddString(memArray *obj,const char *udata,tBool interact)
{
  return ma_AddData(obj,udata,strlen(udata)+1,interact);
}

int __FASTCALL__ ma_Display(memArray *obj,const char *title,int flg, unsigned defsel)
{
  return CommonListBox((const char **)obj->data,obj->nItems,title,flg,defsel);
}
