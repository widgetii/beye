/**
 * @namespace   biew
 * @file        biewutil.h
 * @brief       This file contains prototypes of BIEW utilities.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
**/
#ifndef __BIEWUTIL__H
#define __BIEWUTIL__H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __TWIN_H
#include "biewlib/twin.h"
#endif

#ifndef __BBIO_H
#include "biewlib/bbio.h"
#endif

#ifndef __FILE_INI_RUNTIME_SUPPORT_SYSTEM__
#include "biewlib/file_ini.h"
#endif

extern char legalchars[];
extern __filesize_t headshift;
extern tBool DumpMode;
extern tBool EditMode;

extern __filesize_t lastbyte;
extern char * ini_name;

extern int   __FASTCALL__ GetBool(tBool _bool);
extern void **            FAllocPtrPtr(unsigned num);
extern void *             FAllocPtr(unsigned size);
extern void               FFreeArr(void **arr,unsigned n);
extern void               CriticalExit(int code);

extern void               init_addons(void);
extern void               term_addons(void);
extern void               SelectTool( void );
extern void               init_sysinfo( void );
extern void               term_sysinfo( void );
extern void               SelectSysInfo( void );
                          /** return True if LastOpenFile == Current open file */
extern tBool              isValidIniArgs( void );

extern tBool              NewSource( void );
extern tBool              FileUtils( void );
extern __filesize_t       IsNewExe(void);

extern char * __FASTCALL__ Get2Digit(tUInt8);
extern char * __FASTCALL__ Get2SignDig(tInt8);
extern char * __FASTCALL__ Get4Digit(tUInt16);
extern char * __FASTCALL__ Get4SignDig(tInt16);
extern char * __FASTCALL__ Get8Digit(tUInt32);
extern char * __FASTCALL__ Get8SignDig(tInt32);
#ifdef INT64_C
extern char * __FASTCALL__ Get16Digit(tUInt64);
extern char * __FASTCALL__ Get16SignDig(tInt64);
#else
extern char * __FASTCALL__ Get16Digit(tUInt32 low,tUInt32 high);
extern char * __FASTCALL__ Get16SignDig(tInt32 low,tInt32 high);
#endif
extern char * __FASTCALL__ GetBinary(char val);

extern int      __FASTCALL__ ExpandHex(char * dest,const unsigned char * src,int size,char hard);
extern void     __FASTCALL__ CompressHex(unsigned char * dest,const char * src,unsigned sizedest,tBool usespace);
extern unsigned __FASTCALL__ Summ(unsigned char *array,unsigned size);

extern void   ExtHelp(void);
extern void   drawEditPrompt( void );
extern void   drawEmptyPrompt( void );
extern void   drawEmptyListPrompt( void );
extern void   drawAsmEdPrompt( void );
extern int    EditAsmActionFromMenu( void );
extern void   drawListPrompt( void );
extern void   drawOrdListPrompt( void );
extern void   drawSearchListPrompt( void );
extern void   drawHelpPrompt( void );
extern int    HelpActionFromMenu( void );
extern void   drawHelpListPrompt( void );
extern void   drawPrompt( void );
extern int    MainActionFromMenu(void);
extern void   About( void );

extern __filesize_t __FASTCALL__ WhereAMI(__filesize_t ctrl_pos);

#define RAPREF_NONE            0  /**< means reference is not appended */
#define RAPREF_DONE    UINT32_MAX /**< means reference is appended */

                   /** Appends disassembler reference to string.
                     * @param str          string buffer for append to
                     * @param ulShift      physical address of field, that required of binding
                     * @param mode         see reg_form.h for detail
                     * @param codelen      length of field, that required binding
                     * @param r_shift      used only if APPREF_TRY_LABEL mode is set, contains real value of field, that required binding
                     * @return             one of RAPREF_* constants or physical
                                           offset of target which is applied to
                                           fixing field.
                    **/
extern unsigned long __FASTCALL__ AppendAsmRef(char *str,__filesize_t ulShift,
                                               int mode,char codelen,
                                               __filesize_t r_shift);


extern void  ShowSysInfo( void );
extern void  PaintTitle( void );
extern void  MainLoop( void );

extern int  __FASTCALL__ isHOnLine(__filesize_t cp,int width);

#define HLS_NORMAL               0x0000
#define HLS_USE_DOUBLE_WIDTH     0x0001
#define HLS_USE_BUFFER_AS_VIDEO  0x0002

typedef union tag_HLInfo
{
  const char     *text;
  tvioBuff        buff;
}HLInfo;

extern void __FASTCALL__ HiLightSearch(TWindow *out,__filesize_t cfp,tRelCoord minx,
                          tRelCoord maxx,tRelCoord y,HLInfo *buff,unsigned flags);

/** Class memory array */

typedef struct tag_memArray
{
  void **  data;
  unsigned nItems;
  unsigned nSize;
}memArray;

extern memArray *__FASTCALL__ ma_Build( int maxitems, tBool interact );
extern tBool     __FASTCALL__ ma_AddString(memArray *obj,const char *data,tBool interact);
extern tBool     __FASTCALL__ ma_AddData(memArray *obj,const void *data,unsigned size,tBool interact);
extern void      __FASTCALL__ ma_Destroy(memArray *obj);
extern int       __FASTCALL__ ma_Display(memArray *obj,const char *title,int flg,unsigned defsel);

extern unsigned __FASTCALL__ biewReadProfileString(hIniProfile *ini,
                                      const char *section,
                                      const char *subsection,
                                      const char *_item,
                                      const char *def_value,
                                      char *buffer,
                                      unsigned cbBuffer);

extern tBool __FASTCALL__ biewWriteProfileString(hIniProfile *ini,
                                                 const char *section,
                                                 const char *subsection,
                                                 const char *item,
                                                 const char *value);

#ifdef __cplusplus
}
#endif

#endif
