/**
 * @namespace   biew
 * @file        editor.h
 * @brief       This file contains editing function prototypes.
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
#ifndef __EDITOR__H
#define __EDITOR__H

#ifndef __TWIN_H
#include "biewlib/twin.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct tag_emem
{
  unsigned char *buff;
  unsigned char *save;
  unsigned char *alen;
  unsigned       size;
  unsigned       width;
};

extern struct tag_emem EditorMem;

extern int edit_x,edit_y;
extern unsigned char edit_XX;
extern long edit_cp;


extern void   __FASTCALL__ PaintETitle( int shift,tBool use_shift );
extern void   __FASTCALL__ CheckBounds( void );
extern void   __FASTCALL__ CheckYBounds( void );
extern void   __FASTCALL__ CheckXYBounds( void );
extern tBool  __FASTCALL__ edit_defaction(int _lastbyte);
extern void   __FASTCALL__ editSaveContest( void );
extern tBool  __FASTCALL__ editDefAction(int _lastbyte);
extern int    __FASTCALL__ FullEdit(TWindow * txtwnd);
extern tBool  __FASTCALL__ editInitBuffs(unsigned width);
extern void   __FASTCALL__ editDestroyBuffs( void );


#ifdef __cplusplus
}
#endif

#endif
