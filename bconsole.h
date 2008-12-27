/**
 * @namespace   biew
 * @file        bconsole.h
 * @brief       This file included BIEW console functions description.
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
 * @author      Mauro Giachero
 * @date        02.11.2007
 * @note        Added "ungotstring" function to enable inline assemblers
**/
#ifndef __BCONSOLE__H
#define __BCONSOLE__H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __TWIN_H
#include "biewlib/twin.h"
#endif

extern TWindow *MainWnd,*HelpWnd,*TitleWnd,*CritErrWnd;

extern void         __FASTCALL__ initBConsole( unsigned long vio_flg,unsigned long twin_flg );
extern void         __FASTCALL__ termBConsole( void );
extern tBool        __FASTCALL__ IsKbdTerminate( void );
extern void         __FASTCALL__ CleanKbdTermSig( void );

typedef void (__FASTCALL__ * pagefunc)(TWindow *win,const void **__obj,unsigned i__obj,unsigned total_obj);

extern void         __FASTCALL__ CloseWnd(TWindow *w);
extern TWindow *    __FASTCALL__ CrtDlgWnd(const char *,tAbsCoord,tAbsCoord);
extern TWindow *    __FASTCALL__ CrtDlgWndnls(const char *,tAbsCoord,tAbsCoord);
extern TWindow *    __FASTCALL__ CrtMnuWnd(const char *,tAbsCoord,tAbsCoord,tAbsCoord,tAbsCoord);
extern TWindow *    __FASTCALL__ CrtMnuWndnls(const char *,tAbsCoord,tAbsCoord,tAbsCoord,tAbsCoord);
extern TWindow *    __FASTCALL__ CrtLstWnd(const char *,tAbsCoord,tAbsCoord);
extern TWindow *    __FASTCALL__ CrtLstWndnls(const char *,tAbsCoord,tAbsCoord);
extern TWindow *    __FASTCALL__ CrtHlpWnd(const char *,tAbsCoord,tAbsCoord);
extern TWindow *    __FASTCALL__ CrtHlpWndnls(const char *,tAbsCoord,tAbsCoord);
extern TWindow *    __FASTCALL__ CreateEditor(tAbsCoord X1,tAbsCoord Y1,tAbsCoord X2,tAbsCoord Y2,unsigned flags);
extern TWindow *    __FASTCALL__ WindowOpen(tAbsCoord X1,tAbsCoord Y1,tAbsCoord X2,tAbsCoord Y2,unsigned flags);
extern void         __FASTCALL__ DisplayBox(const char **names,unsigned nlist,const char *title);

/** Edit string styles */
#define __ESS_ENABLEINSERT 0x0001U /**< enable insert mode */
#define __ESS_HARDEDIT     0x0002U /**< inform, that editing within hard multiline
                                        editor without insert mode */
#define __ESS_WANTRETURN   0x0004U /**< return from routine after each pressed key
                                        need for contest depended painting. */
#define __ESS_ASHEX        0x0008U /**< worked, as hexadecimal editor, i.e. insert
                                        space on each third position */
#define __ESS_NOTUPDATELEN 0x0010U /**< if attr & __ESS_ASHEX procedure not will
                                        update field *maxlength on each return */
#define __ESS_FILLER_7BIT  0x0020U /**< Editor for Assemebler mode */
#define __ESS_NON_C_STR    0x0040U /**< Notify editor about non-C string */
#define __ESS_NOREDRAW     0x8000U /**< Force no redraw string */

/** Arguments for (x,e)editstring:
   s         - pointer to a buffer with editing strings
   legal     - pointer to a legal character set (all if NULL)
   maxlength - pointer to maximal possible length of string
               contains real length on return from routine
      y      - if __ESS_HARDEDIT y position within using window
               else if __ESS_NON_C - real length of
               string, i.e. enabled for input CHAR 0x00
      stx    - pointer to a x position within using window (0 - base)
               contains last x position on return
               if NULL position = 0
      attr   - attributes of the editor (See 'Edit string styles')
     undo    - if not NULL twUsedWin as UnDo buffer, i.e.
               restore contest on CtrlBkSpace
     func    - if not NULL then called to display prompt string
*/
extern int         __FASTCALL__ eeditstring(char *s,const char *legal,
					unsigned *maxlength, unsigned y,
					unsigned *stx,unsigned attr,char *undo,
					void (*func)(void));
extern int          __FASTCALL__ xeditstring(char *s,const char *legal,
					unsigned maxlength, void(*func)(void));
extern void         __FASTCALL__ ErrMessageBox(const char * text,const char * title);
extern void         __FASTCALL__ WarnMessageBox(const char * text,const char * title);
extern void         __FASTCALL__ errnoMessageBox(const char * text,const char * title,int __errno__);
extern void         __FASTCALL__ ListBox(const char ** names,unsigned nlist,const char * title);
extern void         __FASTCALL__ TMessageBox(const char * text,const char * title);
extern void         __FASTCALL__ NotifyBox(const char * text,const char * title);
extern int          __FASTCALL__ PageBox(unsigned width,unsigned height,const void ** __obj,
                                 unsigned nobj,pagefunc func);
extern void         __FASTCALL__ MemOutBox(const char *user_msg);
extern TWindow *    __FASTCALL__ PleaseWaitWnd( void );

extern tBool        __FASTCALL__ Get2DigitDlg(const char *title,const char *text,unsigned char *xx);
#if __WORDSIZE >= 32
extern tBool        __FASTCALL__ Get16DigitDlg(const char *title,const char *text,char attr,
						unsigned long long int *xx);
#else
extern tBool        __FASTCALL__ Get8DigitDlg(const char *title,const char *text,char attr,
						unsigned long *xx);
#endif
extern tBool        __FASTCALL__ GetStringDlg(char * buff,const char * title,const char *subtitle,
                                     const char *prompt);

#define GJDLG_ABSOLUTE  0x00000000UL
#define GJDLG_RELATIVE  0x00000001UL
#define GJDLG_VIRTUAL   0x00000002UL
#define GJDLG_PERCENTS  0x00000003UL

extern tBool        __FASTCALL__ GetJumpDlg( __filesize_t * addr,unsigned long *flags);

#define FSDLG_BINMODE   0x00000000UL
#define FSDLG_ASMMODE   0x00000001UL
#define FSDLG_NOCOMMENT 0x00000000UL
#define FSDLG_COMMENT   0x00000002UL
#define FSDLG_STRUCTS   0x00000004UL
#define FSDLG_NOMODES   0x00000000UL
#define FSDLG_USEMODES  0x80000000UL

#define FSDLG_BTNSMASK  0x00000003UL /**< 0=8bit 1=16bit 2=32bit 3=64bit */
#define FSDLG_USEBITNS  0x40000000UL

extern tBool        __FASTCALL__ GetFStoreDlg(const char *title,char *fname,
                                     unsigned long *flags,
                                     __filesize_t *start,
                                     __filesize_t *end,
                                     const char *prompt);
extern tBool        __FASTCALL__ GetInsDelBlkDlg(const char *title,__filesize_t *start,
                                        __fileoff_t *size);

#define LB_SELECTIVE 0x01U
#define LB_SORTABLE  0x02U
#define LB_USEACC    0x04U

#define LB_ORD_DELIMITER TWC_FL_BLK
extern int          __FASTCALL__ CommonListBox(const char ** names,unsigned nlist,const char *title,
                                      int acc,unsigned defsel);
extern int          __FASTCALL__ SelBox(const char ** names,unsigned nlist,const char * title,
                               unsigned defsel);
extern int          __FASTCALL__ SelBoxA(const char ** names,unsigned nlist,const char * title,
                                unsigned defsel);
extern int         __FASTCALL__ SelListBox(const char ** names,unsigned nlist,const char * title,
                                   unsigned defsel);

extern TWindow *    __FASTCALL__ PercentWnd(const char *text,const char *title);

                           /** return True - if can continue
                                     False -  if user want terminate process */
extern tBool        __FASTCALL__ ShowPercentInWnd(TWindow *prcntwnd,unsigned n);

extern int          __FASTCALL__ GetEvent(void (*)(void),int (*)(void),TWindow *);
extern void         __FASTCALL__ PostEvent(int kbdcode);

extern tBool __FASTCALL__ _lb_searchtext(const char *str,const char *tmpl,
                                         unsigned searchlen,const int *cache,
                                         unsigned flg);
extern void __FASTCALL__ __drawSinglePrompt(const char *prmt[]);

extern tBool __FASTCALL__ ungotstring(char *string);

#ifdef __cplusplus
}
#endif

#endif
