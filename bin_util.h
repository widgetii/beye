/**
 * @namespace   biew
 * @file        bin_util.h
 * @brief       This file contains prototypes of common functions of
 *              plugins\bin of BIEW project.
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
#ifndef __BIN_UTIL__H
#define __BIN_UTIL__H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __BBIO_H
#include "biewlib/bbio.h"
#endif

#ifndef __BIEWUTIL__H
#include "biewutil.h"
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define  FMT_WORD(cval,is_big)\
 (tUInt16)((tBool)is_big ? ByteSwapS(*(tUInt16 *)(tUInt8 *)cval) : *(tUInt16 *)(tUInt8 *)cval)
#define  FMT_DWORD(cval,is_big)\
 (tUInt32)((tBool)is_big ? ByteSwapL(*(tUInt32 *)(tUInt8 *)cval) : *(tUInt32 *)(tUInt8 *)cval)
#else
#define  FMT_WORD(cval,is_big)\
 (tUInt16)(!(tBool)is_big ? ByteSwapS(*(tUInt16 *)(tUInt8 *)cval) : *(tUInt16 *)(tUInt8 *)cval)
#define  FMT_DWORD(cval,is_big)\
 (tUInt32)(!(tBool)is_big ? ByteSwapL(*(tUInt32 *)(tUInt8 *)cval) : *(tUInt32 *)(tUInt8 *)cval)
#endif

struct PubName
{
  unsigned long pa;
  unsigned long nameoff;
  unsigned long addinfo;
  unsigned long attr;
};

extern linearArray *PubNames;
extern unsigned fmtActiveState;

typedef void (__FASTCALL__ *ReadPubName)(BGLOBAL b_cache,const struct PubName *it,
                            char *buff,unsigned cb_buff);
typedef void (__FASTCALL__ *ReadPubNameList)(BGLOBAL fmt_chahe,void (__FASTCALL__ *mem_out)(const char *));

extern void  __FASTCALL__ fmtSetState(int state);
extern tCompare __FASTCALL__ fmtComparePubNames(const void __HUGE__ *v1,const void __HUGE__ *v2);
extern tBool __FASTCALL__ fmtFindPubName(BGLOBAL fmt_cache,char *buff,unsigned cb_buff,
                                         unsigned long pa,
                                         ReadPubNameList fmtReadPubNameList,
                                         ReadPubName fmtReadPubName);
extern unsigned long __FASTCALL__ fmtGetPubSym(BGLOBAL fmt_cache,char *str,unsigned cb_str,
                                      unsigned *func_class,unsigned long pa,
                                      tBool as_prev,
                                      ReadPubNameList fmtReadPubNameList,
                                      ReadPubName fmtReadPubName);

typedef unsigned      __FASTCALL__ (* GetNumItems)(BGLOBAL handle);
typedef tBool         __FASTCALL__ (* ReadItems)(BGLOBAL handle,memArray * names,unsigned nnames);
typedef unsigned long __FASTCALL__ (* CalcEntry)(unsigned,int dispmsg);
extern  int           __FASTCALL__ fmtShowList( GetNumItems gni,ReadItems ri,const char * title,int flags,unsigned * ordinal);


#ifdef __cplusplus
}
#endif

#endif
