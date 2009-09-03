/**
 * @namespace   biew
 * @file        biewhelp.h
 * @brief       This file contains prototypes of BIEW help system.
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
#ifndef __BIEWHELP__H
#define __BIEWHELP__H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __TWIN_H
#include "biewlib/twin.h"
#endif

#define BIEW_HELP_VER "BIEW_HLP v5.6.0"

#define HLP_SLONG_LEN 9
#define HLP_VER_LEN 16

/** Maximal size of help topic - is 0xFFFF bytes */

typedef struct tag_biew_help_item
{
  char     item_id[HLP_SLONG_LEN]; /**< character representation of short type. Null-terminated */
  char     item_off[HLP_SLONG_LEN];
  char     item_length[HLP_SLONG_LEN];
  char     item_decomp_size[HLP_SLONG_LEN];
}BIEW_HELP_ITEM;

typedef struct tag_biew_help
{
  char            help_version[HLP_VER_LEN]; /**< identifiaction signature */
  char            item_count[HLP_SLONG_LEN]; /**< total count of items */
  BIEW_HELP_ITEM  items[1];         /**< Array of items */
/**< Binary data of help */
}BIEW_HELP;

/** Color definition */
#define HLPC_BOLD_ON               0x01
#define HLPC_ITALIC_ON             0x02
#define HLPC_UNDERLINE_ON          0x03
#define HLPC_STRIKETHROUGH_ON      0x04
#define HLPC_REVERSE_ON            0x05
#define HLPC_LINK_ON               0x06
#define HLPC_BOLD_OFF              0x11
#define HLPC_ITALIC_OFF            0x12
#define HLPC_UNDERLINE_OFF         0x13
#define HLPC_STRIKETHROUGH_OFF     0x14
#define HLPC_REVERSE_OFF           0x15
#define HLPC_LINK_OFF              0x16

extern tBool           __FASTCALL__ hlpOpen( tBool interactive );
extern void            __FASTCALL__ hlpClose( void );
                       /** Return uncompressed size of help item
                          0 - if error occured */
extern unsigned long   __FASTCALL__ hlpGetItemSize(unsigned long item_id);
extern tBool           __FASTCALL__ hlpLoadItem(unsigned long item_id, void __HUGE__* buffer);
                       /** Fully-functionallity utility for displaying help */
extern void            __FASTCALL__ hlpDisplay(unsigned long id);

                       /** Returns array of char pointers.
                          Title always is data[0] */
extern char **         __FASTCALL__ hlpPointStrings(char __HUGE__ *data,unsigned long data_size,
                                       unsigned long *nstr);
                       /** Filles buffer as video memory from string */
extern unsigned        __FASTCALL__ hlpFillBuffer(tvioBuff * dest,unsigned int cw_dest,
                                     const char * str,unsigned int cb_str,
                                     unsigned int shift,unsigned *n_tabs,
                                     tBool is_hl);
                       /** Paints line of help */
extern void            __FASTCALL__ hlpPaintLine(TWindow *win,unsigned y,const char *str,
                                    tBool is_hl);

#ifdef __cplusplus
}
#endif

#endif
