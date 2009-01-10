/**
 * @namespace   biewlib
 * @file        biewlib/twin.h
 * @brief       This file contains prototypes of Text Window manager functions.
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
 * @warning     Program is destroyed, from twPrintF misapplication
 * @bug         Limitation of twPrintF using
**/

#ifndef __TWIN_H
#define __TWIN_H 1

#ifndef __BIEWLIB_H
#include "biewlib/biewlib.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Enumerates all colors used in twin library */
typedef enum
{
  Black = 0,               /**< Equivalent of RGB: 0 0 0 */
  Blue,                    /**< Equivalent of RGB: 0 0 128 */
  Green,                   /**< Equivalent of RGB: 0 128 0 */
  Cyan,                    /**< Equivalent of RGB: 0 128 128 */
  Red,                     /**< Equivalent of RGB: 128 0 0 */
  Magenta,                 /**< Equivalent of RGB: 128 0 128 */
  Brown,                   /**< Equivalent of RGB: 128 128 0 */
  LightGray,               /**< Equivalent of RGB: 192 192 192 */
  Gray,                    /**< Equivalent of RGB: 128 128 128 */
  LightBlue,               /**< Equivalent of RGB: 0 0 255 */
  LightGreen,              /**< Equivalent of RGB: 0 255 0 */
  LightCyan,               /**< Equivalent of RGB: 0 255 255 */
  LightRed,                /**< Equivalent of RGB: 255 0 0 */
  LightMagenta,            /**< Equivalent of RGB: 255 0 255 */
  Yellow,                  /**< Equivalent of RGB: 255 255 0 */
  White                    /**< Equivalent of RGB: 255 255 255 */
}Color;

/* Pseudographics and special characters support
  Box drawing in short:
   S - single  D - double
   H - horizontal  V - vertical
   u - up  d - down  l - left  r - right
*/

#define TWC_DEF_FILLER        0x20  /**< default filler char */

#ifndef HAVE_TERMINAL_OUT
#define TWC_UP_ARROW          ''   /**< Up arrow character */
#define TWC_DN_ARROW          ''   /**< Down arrow character */
#define TWC_LT_ARROW          ''   /**< Left arrow character */
#define TWC_RT_ARROW          ''   /**< Right arrow character */
#define TWC_THUMB             '\xFE'/**< Thumb character */
#else
#define TWC_UP_ARROW          '^'   /**< Up arrow character */
#define TWC_DN_ARROW          'v'   /**< Down arrow character */
#define TWC_LT_ARROW          '<'   /**< Left arrow character */
#define TWC_RT_ARROW          '>'   /**< Right arrow character */
#define TWC_THUMB             '#'   /**< Thumb character */
#endif

#define TWC_RADIO_CHAR        '*'   /**< Character for radio buttons */
#define TWC_CHECK_CHAR        'x'   /**< Character for check buttons */

#define TWC_LT_SHADE          0xB0  /**< Light shade characters */
#define TWC_MED_SHADE         0xB1  /**< Medium shade characters */
#define TWC_DK_SHADE          0xB2  /**< Dark shade characters */

#define TWC_SV                0xB3  /**< SINGLE VERTICAL */
#define TWC_SV_Sl             0xB4  /**< SINGLE VERTICAL and SINGLE left */
#define TWC_Sl_SV             TWC_SV_Sl
#define TWC_SV_Dl             0xB5  /**< SINGLE VERTICAL and DOUBLE left */
#define TWC_Dl_SV             TWC_SV_Dl
#define TWC_DV_Sl             0xB6  /**< DOUBLE VERTICAL and SINGLE left */
#define TWC_Sl_DV             TWC_DV_Sl
#define TWC_Dd_Sl             0xB7  /**< DOUBLE down and SINGLE left */
#define TWC_Sl_Dd             TWC_Dd_Sl
#define TWC_Sd_Dl             0xB8  /**< SINGLE down and DOUBLE left */
#define TWC_Dl_Sd             TWC_Sd_Dl
#define TWC_DV_Dl             0xB9  /**< DOUBLE VERTICAL and DOUBLE left */
#define TWC_Dl_DV             TWC_DV_Dl
#define TWC_DV                0xBA  /**< DOUBLE VERTICAL */
#define TWC_Dd_Dl             0xBB  /**< DOUBLE down and DOUBLE left */
#define TWC_Dl_Dd             TWC_Dd_Dl
#define TWC_Du_Dl             0xBC  /**< DOUBLE up and DOUBLE left */
#define TWC_Dl_Du             TWC_Du_Dl
#define TWC_Du_Sl             0xBD  /**< DOUBLE up and SINGLE left */
#define TWC_Sl_Du             TWC_Du_Sl
#define TWC_Su_Dl             0xBE  /**< SINGLE up and DOUBLE left */
#define TWC_Dl_Su             TWC_Su_Dl
#define TWC_Sd_Sl             0xBF  /**< SINGLE down and SINGLE left */
#define TWC_Sl_Sd             TWC_Sd_Sl
#define TWC_Su_Sr             0xC0  /**< SINGLE up and SINGLE right */
#define TWC_Sr_Su             TWC_Su_Sr
#define TWC_SH_Su             0xC1  /**< SINGLE HORIZONTAL and SINGLE up */
#define TWC_Su_SH             TWC_SH_Su
#define TWC_SH_Sd             0xC2  /**< SINGLE HORIZONTAL and SINGLE down */
#define TWC_Sd_SH             TWC_SH_Sd
#define TWC_SV_Sr             0xC3  /**< SINGLE VERTICAL and SINGLE right */
#define TWC_Sr_SV             TWC_SV_Sr
#define TWC_SH                0xC4  /**< SINGLE HORIZONTAL */
#define TWC_SH_SV             0xC5  /**< SINGLE HORIZONTAL and SINGLE VERTICAL */
#define TWC_SV_SH             TWC_SH_SV
#define TWC_SV_Dr             0xC6  /**< SINGLE VERTICAL and DOUBLE right */
#define TWC_Dr_SV             TWC_SV_Dr
#define TWC_DV_Sr             0xC7  /**< DOUBLE VERTICAL and SINGLE right */
#define TWC_Sr_DV             TWC_DV_Sr
#define TWC_Du_Dr             0xC8  /**< DOUBLE up and DOUBLE right */
#define TWC_Dr_Du             TWC_Du_Dr
#define TWC_Dd_Dr             0xC9  /**< DOUBLE down and DOUBLE right */
#define TWC_Dr_Dd             TWC_Dd_Dr
#define TWC_DH_Du             0xCA  /**< DOUBLE HORIZONTAL and DOUBLE up */
#define TWC_Du_DH             TWC_DH_Du
#define TWC_DH_Dd             0xCB  /**< DOUBLE HORIZONTAL and DOUBLE down */
#define TWC_Dd_DH             TWC_DH_Dd
#define TWC_DV_Dr             0xCC  /**< DOUBLE VERTICAL and DOUBLE right */
#define TWC_Dr_DV             TWC_DV_Dr
#define TWC_DH                0xCD  /**< DOUBLE HORIZONTAL */
#define TWC_DH_DV             0xCE  /**< DOUBLE HORIZONTAL and DOUBLE VERTICAL */
#define TWC_DV_DH             TWC_DH_DV
#define TWC_DH_Su             0xCF  /**< DOUBLE HORIZONTAL and SINGLE up */
#define TWC_Su_DH             TWC_DH_Su
#define TWC_SH_Du             0xD0  /**< SINGLE HORIZONTAL and DOUBLE up */
#define TWC_Du_SH             TWC_SH_Du
#define TWC_DH_Sd             0xD1  /**< DOUBLE HORIZONTAL and SINGLE down */
#define TWC_Sd_DH             TWC_DH_Sd
#define TWC_SH_Dd             0xD2  /**< SINGLE HORIZONTAL and DOUBLE down */
#define TWC_Dd_SH             TWC_SH_Dd
#define TWC_Du_Sr             0xD3  /**< DOUBLE up and SINGLE right */
#define TWC_Sr_Du             TWC_Du_Sr
#define TWC_Su_Dr             0xD4  /**< SINGLE up and DOUBLE right */
#define TWC_Dr_Su             TWC_Su_Dr
#define TWC_Sd_Dr             0xD5  /**< SINGLE down and DOUBLE right */
#define TWC_Dr_Sd             TWC_Sd_Dr
#define TWC_Dd_Sr             0xD6  /**< DOUBLE down and SINGLE right */
#define TWC_Sr_Dd             TWC_Sr_Dd
#define TWC_DV_SH             0xD7  /**< DOUBLE VERTICAL and SINGLE HORIZONTAL */
#define TWC_SH_DV             TWC_DV_SH
#define TWC_SV_DH             0xD8  /**< SINGLE VERTICAL and DOUBLE HORIZONTAL */
#define TWC_DH_SV             TWC_SV_DH
#define TWC_Su_Sl             0xD9  /**< SINGLE up and SINGLE left */
#define TWC_Sl_Su             TWC_Su_Sl
#define TWC_Sd_Sr             0xDA  /**< SINGLE down and SINGLE right */
#define TWC_Sr_Sd             TWC_Sd_Sr

#define TWC_FL_BLK            0xDB  /**< Full block character */
#define TWC_LW_HBLK           0xDC  /**< Lower half block character */
#define TWC_LF_HBLK           0xDD  /**< Left half block character */
#define TWC_RT_HBLK           0xDE  /**< Right half block character */
#define TWC_UP_HBLK           0xDF  /**< Up half block character */

#define TWC_BLACK_SQUARE      0xFE  /**< Black square */

/** Describes window-related coordinate type */
#if __WORDSIZE == 16
typedef unsigned char tRelCoord;
#else
typedef unsigned tRelCoord;
#endif

/** align modes for title and footer */
typedef enum
{
  TW_TMODE_LEFT = 0,      /**< left alignment */
  TW_TMODE_CENTER,        /**< center alignment */
  TW_TMODE_RIGHT          /**< right alignment */
}tTitleMode;

/** Flags of twCreateWin */
#define TWS_NONE           0x0000 /**< Indicates no flags @see twCreateWin */
#define TWS_FRAMEABLE      0x0001 /**< Indicates that window has frame border @see twCreateWin */
#define TWS_VISIBLE        0x0002 /**< Indicates that window creates in visible state @see twCreateWin */
#define TWS_CURSORABLE     0x0004 /**< Indicates that window has text cursor @see twCreateWin */
#define TWS_NLSOEM         0x0100 /**< Indicates that window works in OEM mode @see twCreateWin */

extern const char TW_SINGLE_FRAME[];    /**< Flat single-line frame @see twinDrawFrame */
extern const char TW_DOUBLE_FRAME[];    /**< Flat double-line frame @see twinDrawFrame */
#define TW_THIN_FRAME   TW_SINGLE_FRAME /**< Synonym for TW_SINGLE_LINE @see twinDrawFrame */
extern const char TW_MEDIUM_FRAME[];    /**< Flat frame of medium width (filles 50% of the character cells) @see twinDrawFrame */
extern const char TW_THICK_FRAME[];     /**< Flat frame of full width (filles 100% of the character cells) @see twinDrawFrame */
extern const char TW_UP3D_FRAME[];      /**< Emulates 3D-frame that similar unpressed button @see twinDrawFrame */
extern const char TW_DN3D_FRAME[];      /**< Emulates 3D-frame that similar pressed button @see twinDrawFrame */

/** Defines color pair that contains original user color and converted system color */
typedef struct tagDefColor
{
  ColorAttr user;        /**< Original user color value */
  ColorAttr system;      /**< Converted system color value */
}DefColor;

                      /** Converts logical foreground and background into physical color attributes */
#define LOGFB_TO_PHYS(fore,back) ((((back) << 4) & 0xF0) | ((fore) & 0x0F))
                      /** Gets background color from physical attributes */
#define BACK_COLOR(attr) (((attr) >> 4) & 0x0F)
                      /** Gets foreground color from physical attributes */
#define FORE_COLOR(attr) ((attr) & 0x0F)
                      /** Converts physical color attributes into logical foreground and background */
#define PHYS_TO_LOGFB(attr,fore,back) { fore = FORE_COLOR(attr); back = BACK_COLOR(attr); }

/*
   This struct is ordered as it documented in Athlon manual
   Publication # 22007 Rev: D
*/
/** Internal structure of text window */
typedef struct tagTWindow
{
  char           Frame[8];    /**< Buffer, contaning frame component */
                              /* Frame encoding:   1---2---3 */
                              /*                   |       | */
                              /*                   4       5 */
                              /*                   |       | */
                              /*                   6---7---8 */
  unsigned long  iflags;      /**< contains internal flags of window state */
  struct tagTWindow *next;    /**< pointer to next window in list */
  void *         usrData;     /**< user data pointer */
  tvioBuff       body;        /**< Buffer containing image of window frame */
  tvioBuff       saved;       /**< Buffer containing saved image under window */
  char *         Title;       /**< Caption of window */
  char *         Footer;      /**< Footer of window */
  void *         method;      /**< Class callback */
  unsigned       class_flags; /**< Class flags */
  unsigned       wsize;       /**< Size of buffers in bytes */
  unsigned       wwidth;      /**< width of window */
  unsigned       wheight;     /**< height of window */
  unsigned       flags;       /**< Window flags */
  tAbsCoord      X1,Y1,X2,Y2; /**< coordinates of window on the screen */
  tRelCoord      cur_x,cur_y; /**< coordinates of cursor position inside the window */
  tTitleMode     TitleMode;   /**< alignment mode of title */
  tTitleMode     FooterMode;  /**< alignment mode of footer */
  DefColor       text;        /**< default color of text */
  DefColor       frame;       /**< default color of frame */
  DefColor       title;       /**< default color of title */
  DefColor       footer;      /**< default color of footer text */
}TWindow;

/* Below located list of window messages. Prefix WM_ was imported from
   MSWindows SDK, but I hope it so understandable. */

#define WM_NULL          0x0000 /**< Never to send */
#define WM_CREATE        0x0001 /**< It sent when window is being created, has no parameters */
#define WM_DESTROY       0x0002 /**< It sent when window is being destroyed, has no parameters*/
#define WM_SHOW          0x0003 /**< It sent when window is being displayed, has no parameters*/
#define WM_TOPSHOW       0x0004 /**< It sent when window is being displayed on top of all windows, has no parameters*/
#define WM_SHOWBENEATH   0x0005 /**< It sent when window is being displayed beneath of other window, has handle of top window as event_data*/
#define WM_HIDE          0x0006 /**< It sent when window is being hidded, has no parameters */

                   /** Calls window function with given arguments
                     * @return                function answer
                     * @param event           one of WM_* commands
                     * @param event_param     command related parameter
                     * @param event_data      command related data
                     * @see                   twcRegisterClass
                    **/
long __FASTCALL__ twinSendMessage(TWindow *win,unsigned event,unsigned long event_param, void *event_data);

                   /** Draws frame of given type in active window
                     * @return                none
                     * @param x1_,y1_,x2_,y2_ indicate coordinates inside window
                     * @param frame           indicates frame type
                     * @param fore,back       indicate logical foreground and background attributes
                     * @see                   twinDrawFrameAttr
                    **/
void __FASTCALL__ twinDrawFrame(tRelCoord x1_, tRelCoord y1_, tRelCoord x2_, tRelCoord y2_,const char *frame,Color fore, Color back);

                   /** Draws frame of given type in active window
                     * @return                none
                     * @param x1_,y1_,x2_,y2_ indicate coordinates inside window
                     * @param frame           indicates frame type
                     * @param attr            indicates physical color attributes
                     * @see                   twinDrawFrame
                    **/
void __FASTCALL__ twinDrawFrameAttr(tRelCoord x1_, tRelCoord y1_, tRelCoord x2_, tRelCoord y2_,const char *frame,ColorAttr attr);

#define TWIF_FORCEMONO   0x00000001L /**< forces monochrome mode of video output @see twInit */

                   /** Initialization of twin library and video subsystem
                     * @param user_cp     indicates character's codepage or IBM866 if NULL
                     * @param vio_flags   flags for _init_vio
                     * @param twin_flgs   flags of twin library (see above)
                     * @return            none
                     * @note              Call this function before any other
                     * @see               twDestroy
                    **/
void              __FASTCALL__ twInit(const char *user_cp, unsigned long vio_flags, unsigned long twin_flgs );

                   /** Terminates twin library and video subsystem
                     * @return            none
                     * @note              Call this function after all other
                     * @see               twInit
                    **/
void              __FASTCALL__ twDestroy( void );

                   /** Creates window
                     * @param x1_,y1_      indicate upper-left cornen of window
                     * @param width,height indicate width and height of window
                     * @param flags        indicates TWS_* flags
                     * @return             handle of window
                     * @see                twDestroyWin twCreateWinEx
                    **/
TWindow *         __FASTCALL__ twCreateWin(tAbsCoord x1_, tAbsCoord y1_, tAbsCoord width, tAbsCoord height, unsigned flags);

                   /** Creates extended window
                     * @param x1_,y1_      indicate upper-left cornen of window
                     * @param width,height indicate width and height of window
                     * @param flags        indicates TWS_* flags
                     * @param parent       indicates parent window
                     * @param classname    indicates name of class
                     * @return             handle of window
                     * @note               name of class must be registered before
                                           calling of this function
                     * @see                twDestroyWin twCreateWin twRegisterClass
                    **/
TWindow *         __FASTCALL__ twCreateWinEx(tAbsCoord x1_, tAbsCoord y1_,
                                             tAbsCoord width, tAbsCoord height,
                                             unsigned flags, TWindow *parent,
                                             const char *classname);

                   /** Destroys given window
                     * @param              handle of window
                     * @return             none
                     * @see                twCreateWin
                    **/
void              __FASTCALL__ twDestroyWin(TWindow *win);

                   /** Hides given window
                     * @param win          handle of window
                     * @return             none
                     * @note               All operations are available for
                     *                     hidden window. Does not change
                     *                     Z-order of window.
                     * @see                twShowWin twShowWinBeneath twShowWinOnTop
                    **/
void              __FASTCALL__ twHideWin(TWindow *win);

                   /** Shows given window.
                     * @param win          handle of window
                     * @return             none
                     * @note               Does not change Z-order of window.
                     * @see                twHideWin
                    **/
void              __FASTCALL__ twShowWin(TWindow *win);

                   /** Shows given window beneath other window.
                     * @param win          handle of window
                     * @return             none
                     * @note               Does change Z-order of window
                     * @see                twHideWin
                    **/
void              __FASTCALL__ twShowWinBeneath(TWindow *win,TWindow *prev);

                   /** Shows given window on the top of the window stack.
                     * @param win          handle of window
                     * @return             none
                     * @note               Does change Z-order of window
                     * @see                twHideWin
                    **/
void              __FASTCALL__ twShowWinOnTop(TWindow *win);

                   /** Updates the window buffer from the screen.
                     * @param win          handle of window
                     * @return             none
                    **/
void              __FASTCALL__ twSnapShot(TWindow *win);

                   /** Causes all subsequent output to appear in the given window.
                     * @param win          handle of window to be used
                     * @return             none
                     * @note               \e win need not be the top window; not
                     *                     does \e win even have to be visible on
                     *                     the screen.
                     * @see                twUsedWin
                    **/
TWindow *         __FASTCALL__ twUseWin(TWindow *);

                   /** Returns the window currently being used for output.
                     * @return             handle of window currently being used
                     * @note               If no window has been assigned by
                     *                     twUse, then handle of last created
                     *                     window is returned.
                     * @see                twUseWin
                    **/
TWindow *         __FASTCALL__ twUsedWin( void );

                   /** Alters position of given window.
                     * @param win          handle of window to be moved
                     * @param dx,dy        specify relative change of x and y coordinates.
                     * @return             none
                     * @note               The contents of the window will be moved with it.
                     * @see                twCentredWin twResizeWin twGetWinPos
                    **/
void              __FASTCALL__ twMoveWin(TWindow *win,tAbsCoord dx, tAbsCoord dy);

                   /** Alters size of given window.
                     * @param win          handle of window to be resized
                     * @param width,height specify new width and height of the window.
                     * @return             none
                     * @note               If the window is expanded, blanks are filled in;
                                           If the window is contracted, text is clipped
                     * @see                twCentredWin twMoveWin twGetWinPos
                    **/
void              __FASTCALL__ twResizeWin(TWindow *win,tAbsCoord width, tAbsCoord height);

                   /** Scrolls part of given window up.
                     * @param win          handle of window to be scrolled
                     * @param ypos         specifies vertical coordinate of line. All lines above it and line itself will be scrolled up
                     * @param npos         specifies number of lines to be inserted.
                     * @return             none
                     * @note               If 0 is specified as npos, no lines are scrolled.
                     * @see                twScrollWinDn twScrollWinLf twScrollWinRt
                    **/
void              __FASTCALL__ twScrollWinUp(TWindow *win,tRelCoord ypos, unsigned npos);

                   /** Scrolls part of given window down.
                     * @param win          handle of window to be scrolled
                     * @param ypos         specifies vertical coordinate of line. All lines below it and line itself will be scrolled down
                     * @param npos         specifies number of lines to be inserted.
                     * @return             none
                     * @note               If 0 is specified as npos, no lines are scrolled.
                     * @see                twScrollWinUp twScrollWinLf twScrollWinRt
                    **/
void              __FASTCALL__ twScrollWinDn(TWindow *win,tRelCoord ypos, unsigned npos);

                   /** Scrolls part of given window left.
                     * @param win          handle of window to be scrolled
                     * @param xpos         specifies horizontal coordinate of column. All columns most left it and column itself will be scrolled left
                     * @param npos         specifies number of columns to be inserted.
                     * @return             none
                     * @note               If 0 is specified as npos, no columns are scrolled.
                     * @see                twScrollWinUp twScrollWinDn twScrollWinRt
                    **/
void              __FASTCALL__ twScrollWinLf(TWindow *win,tRelCoord xpos, unsigned npos);

                   /** Scrolls part of given window right.
                     * @param win          handle of window to be scrolled
                     * @param xpos         specifies horizontal coordinate of column. All columns most right it and column itself will be scrolled right
                     * @param npos         specifies number of columns to be inserted.
                     * @return             none
                     * @note               If 0 is specified as npos, no columns are scrolled.
                     * @see                twScrollWinUp twScrollWinDn twScrollWinLf
                    **/
void              __FASTCALL__ twScrollWinRt(TWindow *win,tRelCoord xpos, unsigned npos);

                   /** Does centring of given window relatively other window or screen.
                     * @param it           handle of window to be centred
                     * @param parent       handle of centring window.
                     * @return             none
                     * @note               If \e parent is NULL, the window will
                     *                     be centred for screen
                     * @see                twMoveWin twResizeWin twGetWinPos
                    **/
void              __FASTCALL__ twCentredWin(TWindow *it,const TWindow *parent);

                   /** Freezes redrawing of window.
                     * @param parent       handle of centring window.
                     * @return             none
                     * @note               Freezed only redrawing of the window.
                     *                     All output will be produced, but
                     *                     screen will not be updated.
                     * @see                twRefreshLine twRefreshPiece twRefreshWin twRefreshFullWin
                    **/
void              __FASTCALL__ twFreezeWin(TWindow *win);

                   /** Updates the line of screen by line of buffer of given window.
                     * @param win          handle of refreshing window.
                     * @param y            specify relative y coordinate of line to be refreshed.
                     * @return             none
                     * @note               If window is not freezed all output
                     *                     is automatically displayed on
                     *                     the screen.
                     * @see                twFreezeWin twRefreshPiece twRefreshWin twRefreshFullWin
                    **/
void              __FASTCALL__ twRefreshLine(TWindow *win,tRelCoord y);

                   /** Updates the piece of screen line by piece of buffer of given window.
                     * @param win          handle of refreshing window.
                     * @param stx,endx     specify range of x coordinate of line to be refreshed.
                     * @param y            specify relative y coordinate of line to be refreshed.
                     * @return             none
                     * @note               If window is not freezed all output
                     *                     is automatically displayed on
                     *                     the screen.
                     * @see                twFreezeWin twRefreshLine twRefreshWin twRefreshFullWin
                    **/
void              __FASTCALL__ twRefreshPiece(TWindow *win,tRelCoord stx, tRelCoord endx,tRelCoord y);

                   /** Updates the screen buffer by client area of given window.
                     * @param win          handle of refreshing window.
                     * @return             none
                     * @note               If window is not freezed all output
                     *                     is automatically displayed on
                     *                     the screen.
                     * @see                twFreezeWin twRefreshLine twRefreshPiece twRefreshFullWin
                    **/
void              __FASTCALL__ twRefreshWin(TWindow *win);

                   /** Updates the screen buffer by buffer of given window entire.
                     * @param win          handle of refreshing window.
                     * @return             none
                     * @note               If window is not freezed all output
                     *                     is automatically displayed on
                     *                     the screen.
                     * @see                twFreezeWin twRefreshLine twRefreshPiece twRefreshWin
                    **/
void              __FASTCALL__ twRefreshFullWin(TWindow *win);

                   /** Returns pointer to the user data that stored in window.
                     * @param win          handle of window.
                     * @return             pointer to user data
                     * @note               If no user data previously stored
                     *                     in window then NULL is returned.
                     * @see                twSetUsrData
                    **/
void *            __FASTCALL__ twGetUsrData(TWindow *win);

                   /** Saves pointer to the user data in window.
                     * @param win          handle of window.
                     * @param data         pointer to the user data to be stored.
                     * @return             pointer to user data that previously stored in window
                     * @note               Pointer to previously stored user
                     *                     data is overwrited.
                     * @see                twGetUsrData
                    **/
void *            __FASTCALL__ twSetUsrData(TWindow *win,void *data);

                   /** Returns screen position of window.
                     * @param win          handle of window.
                     * @param x1_,y1_,x2_,y2_ pointers to coordinates where will be saved window position.
                     * @return             none
                     * @see                twMoveWin twResizeWin twCentredWin twGetWinAtPos
                    **/
void              __FASTCALL__ twGetWinPos(TWindow *win,tAbsCoord *x1_,tAbsCoord *y1_,tAbsCoord *x2_,tAbsCoord *y2_);

                   /** Returns width of window.
                     * @param win          handle of window.
                     * @return             width of window
                     * @see                twGetWinHeight twGetClientHeight twGetClientWidth
                    **/
unsigned          __FASTCALL__ twGetWinWidth(TWindow *win);

                   /** Returns width of height.
                     * @param win          handle of window.
                     * @return             height of window
                     * @see                twGetWinWidth twGetClientHeight twGetClientWidth
                    **/
unsigned          __FASTCALL__ twGetWinHeight(TWindow *win);

                   /** Returns width of window which is available for output.
                     * @param win          handle of window.
                     * @return             width of window which is available for output
                     * @see                twGetWinWidth twGetWinHeight twGetClientHeight
                    **/
unsigned          __FASTCALL__ twGetClientWidth(TWindow *win);

                   /** Returns height of window which is available for output.
                     * @param win          handle of window.
                     * @return             height of window which is available for output
                     * @see                twGetWinWidth twGetWinHeight twGetClientWidth
                    **/
unsigned          __FASTCALL__ twGetClientHeight(TWindow *win);

                   /** Returns handle of the window currently displayed at the absolute position x and y.
                     * @param x,y          specify coordinates of the location.
                     * @return             Handle for the window displayed at the
                     *                     specified position if a window is
                     *                     displayed. If no window is displayed
                     *                     at the this position, NULL is returned.
                     * @see                twGetWinPos
                    **/
TWindow *         __FASTCALL__ twGetWinAtPos(tAbsCoord x, tAbsCoord y);

                   /** Checks visibility of window piece.
                     * @param x,y          specify coordinates of the location.
                     * @return             True if specified window is visible
                     *                     in specified location (Is not obscured).
                     * @see                twGetWinPos
                    **/
tBool             __FASTCALL__ twIsPieceVisible(TWindow *win,tRelCoord x, tRelCoord y);

                   /** Converts window-relative coordinates to absolute screen coordinates
                     * @param win          handle of window
                     * @param x,y          specify the relative coordinates.
                     * @param xa,ya        specify pointers to the absolute coordinates.
                     * @return             none
                     * @see                twCvtScreenCoord
                    **/
void              __FASTCALL__ twCvtWinCoords(TWindow *win,tRelCoord x, tRelCoord y,tAbsCoord *xa,tAbsCoord *ya);

                   /** Converts screen-relative coordinates to relative window coordinates
                     * @param win          handle of window
                     * @param x,y          specify screen coordinates.
                     * @param xr,yr        specify pointers to the relative coordinates.
                     * @return             True if successful, False otherwise.
                     * @see                twCvtWinCoord
                    **/
tBool            __FASTCALL__ twCvtScreenCoords(TWindow *win,tAbsCoord x, tAbsCoord y,tRelCoord *xr,tRelCoord *yr);

                   /** Clears the current window window with default filler.
                     * @return             none
                     * @see                twClearWinEx
                    **/
void              __FASTCALL__ twClearWin(void);

                   /** Clears the current window window with given filler.
                     * @param filler       character for filling the window
                     * @return             none
                     * @see                twClearWin
                    **/
void              __FASTCALL__ twClearWinEx(unsigned char filler);

                   /** Clears the current window from current position to the end of line with default filler.
                     * @return             none
                     * @see                twClrEOLEx
                    **/
void              __FASTCALL__ twClrEOL(void);

                   /** Clears the current window from current position to the end of line with given filler.
                     * @param filler       character for filling the window
                     * @return             none
                     * @see                twClrEOL
                    **/
void              __FASTCALL__ twClrEOLEx(unsigned char filler);

                   /** Sets logical foreground and background colors of the text.
                     * @param fore,back    specify colors to be set.
                     * @return             Previously set color attributes
                     * @note               The function affects only those
                     *                     characters output after the call to
                     *                     foreground and background; the function
                     *                     does not affect characters that are
                     *                     already displayed.
                     * @see                twSetColorAttr twGetColor twGetColorAttr twTextColor twTextBkGnd
                    **/
ColorAttr         __FASTCALL__ twSetColor(Color fore,Color back);

                   /** Sets physical color attributes of the text.
                     * @param attr         specifies color attributes.
                     * @return             Previously set color attributes
                     * @note               The function affects only those
                     *                     characters output after the call to
                     *                     foreground and background; the function
                     *                     does not affect characters that are
                     *                     already displayed.
                     * @see                twSetColor twGetColor twGetColorAttr twTextColor twTextBkGnd
                    **/
ColorAttr         __FASTCALL__ twSetColorAttr(ColorAttr attr);

                   /** Returns logical foreground and background colors of the text.
                     * @param fore,back    specify pointers to memory where will be saved logical background and foreground of the text.
                     * @return             none
                     * @see                twSetColorAttr twSetColor twGetColorAttr twTextColor twTextBkGnd
                    **/
void              __FASTCALL__ twGetColor(Color *fore, Color *back );

                   /** Returns physical color attributes of the text.
                     * @return             physical attributes of the text
                     * @see                twSetColorAttr twSetColor twGetColor twTextColor twTextBkGnd
                    **/
ColorAttr         __FASTCALL__ twGetColorAttr( void );

                   /** Sets logical foreground color of the text.
                     * @param col          specifies color to be set.
                     * @return             Previously set foreground color
                     * @note               The function affects only those
                     *                     characters output after the call to
                     *                     foreground; the function does not
                     *                     affect characters that are already
                     *                     displayed.
                     * @see                twSetColorAttr twSetColor twGetColor twTextBkGnd
                    **/
Color             __FASTCALL__ twTextColor( Color col );

                   /** Sets logical background color of the text.
                     * @param col          specifies color to be set.
                     * @return             Previously set background color
                     * @note               The function affects only those
                     *                     characters output after the call to
                     *                     background; the function does not
                     *                     affect characters that are already
                     *                     displayed.
                     * @see                twSetColorAttr twSetColor twGetColor twTextColor
                    **/
Color             __FASTCALL__ twTextBkGnd( Color col );

                   /** Returns frame and frame attributes of given window.
                     * @param win          handle of window.
                     * @param frame        pointer to memory area (8-byte length) where will be stored frame.
                     * @param fore,back    pointers to memory area where will be stored logical colors of frame.
                     * @return             none
                     * @see                twGetFrameAttr twSetFrame twSetFrameAttr
                     *                     twGetTitle twGetTitleAttr twSetTitle twSetTitleAttr
                     *                     twGetFooter twGetFooterAttr twSetFooter twSetFooterAttr
                    **/
void              __FASTCALL__ twGetFrame(TWindow *win,char *frame,Color* fore,Color* back);

                   /** Returns title and title attributes of given window.
                     * @param win          handle of window.
                     * @param title        pointer to memory area where will be stored title.
                     * @param cb_title     size of memory area where will be stored title.
                     * @param fore,back    pointers to memory area where will be stored logical colors of title.
                     * @return             alignment mode of title
                     * @see                twGetFrame twGetFrameAttr twSetFrame twSetFrameAttr twGetFrame
                     *                     twGetTitleAttr twSetTitle twSetTitleAttr
                     *                     twGetFooter twGetFooterAttr twSetFooter twSetFooterAttr
                    **/
tTitleMode        __FASTCALL__ twGetTitle(TWindow *win,char* title,unsigned cb_title,Color* fore,Color* back);

                   /** Returns footer and footer attributes of given window.
                     * @param win          handle of window.
                     * @param footer       pointer to memory area where will be stored footer.
                     * @param cb_footer    size of memory area where will be stored footer.
                     * @param fore,back    pointers to memory area where will be stored logical colors of footer.
                     * @return             alignment mode of footer
                     * @see                twGetFrame twGetFrameAttr twSetFrame twSetFrameAttr
                     *                     twGetTitle twGetTitleAttr twSetTitle twSetTitleAttr
                     *                     twGetFooterAttr twSetFooter twSetFooterAttr
                    **/
tTitleMode        __FASTCALL__ twGetFooter(TWindow *win,char* footer,unsigned cb_footer,Color* fore,Color* back);

                   /** Returns frame and frame attributes of given window.
                     * @param win          handle of window.
                     * @param frame        pointer to memory area (8-byte length) where will be stored frame.
                     * @param attr         pointer to memory area where will be stored physical color attributes of frame.
                     * @return             none
                     * @see                twGetFrame twSetFrame twSetFrameAttr
                     *                     twGetTitle twGetTitleAttr twSetTitle twSetTitleAttr
                     *                     twGetFooter twGetFooterAttr twSetFooter twSetFooterAttr
                    **/
void              __FASTCALL__ twGetFrameAttr(TWindow *win,char *frame,ColorAttr* attr);

                   /** Returns title and title attributes of given window.
                     * @param win          handle of window.
                     * @param title        pointer to memory area where will be stored title.
                     * @param cb_title     size of memory area where will be stored title.
                     * @param attr         pointers to memory area where will be stored physical color attributes of title.
                     * @return             alignment mode of title
                     * @see                twGetFrame twGetFrameAttr twSetFrame twSetFrameAttr twGetFrame
                     *                     twGetTitle twSetTitle twSetTitleAttr
                     *                     twGetFooter twGetFooterAttr twSetFooter twSetFooterAttr
                    **/
tTitleMode        __FASTCALL__ twGetTitleAttr(TWindow *win,char* title,unsigned cb_title,ColorAttr* attr);

                   /** Returns footer and footer attributes of given window.
                     * @param win          handle of window.
                     * @param footer       pointer to memory area where will be stored footer.
                     * @param cb_footer    size of memory area where will be stored footer.
                     * @param fore,back    pointers to memory area where will be stored physical color attributes of footer.
                     * @return             alignment mode of footer
                     * @see                twGetFrame twGetFrameAttr twSetFrame twSetFrameAttr
                     *                     twGetTitle twGetTitleAttr twSetTitle twSetTitleAttr
                     *                     twGetFooter twSetFooter twSetFooterAttr
                    **/
tTitleMode        __FASTCALL__ twGetFooterAttr(TWindow *win,char* footer,unsigned cb_footer,ColorAttr* attr);

                   /** Changes frame around given window with specified logical color attributes.
                     * @param win          handle of window
                     * @param frame        specifies new frame (8-byte length)
                     * @param fore,back    specify new background and foreground colors
                     * @return             none
                     * @see                twGetFrame twGetFrameAttr twGetFooterAttr twSetFrameAttr
                     *                     twGetTitle twGetTitleAttr twSetTitle twSetTitleAttr
                     *                     twGetFooter twSetFooter twSetFooterAttr
                    **/
void              __FASTCALL__ twSetFrame(TWindow *win,const char *frame,Color fore,Color back);

                   /** Updates window title with specified logical color attributes.
                     * @param win          handle of window
                     * @param title        specifies new title
                     * @param titlemode    specifies new title mode
                     * @param fore,back    specify new background and foreground colors
                     * @return             none
                     * @see                twGetFrame twGetFrameAttr twGetFooterAttr twSetFrameAttr
                     *                     twGetTitle twGetTitleAttr twSetFrame twSetTitleAttr
                     *                     twGetFooter twSetFooter twSetFooterAttr
                    **/
void              __FASTCALL__ twSetTitle(TWindow *win,const char* title,tTitleMode titlemode,Color fore,Color back);

                   /** Updates window footer with specified logical color attributes.
                     * @param win          handle of window
                     * @param footer       specifies new footer
                     * @param footermode   specifies new footer mode
                     * @param fore,back    specify new background and foreground colors
                     * @return             none
                     * @see                twGetFrame twGetFrameAttr twGetFooterAttr twSetFrameAttr
                     *                     twGetTitle twGetTitleAttr twSetFrame twSetTitleAttr
                     *                     twGetFooter twSetTitle twSetFooterAttr
                    **/
void              __FASTCALL__ twSetFooter(TWindow *win,const char* footer,tTitleMode footermode,Color fore,Color back);

                   /** Changes frame around given window with specified physical color attributes.
                     * @param win          handle of window
                     * @param frame        specifies new frame (8-byte length)
                     * @param attr         specifies new color attributes
                     * @return             none
                     * @see                twGetFrame twGetFrameAttr twGetFooterAttr twSetFrame
                     *                     twGetTitle twGetTitleAttr twSetTitle twSetTitleAttr
                     *                     twGetFooter twSetFooter twSetFooterAttr
                    **/
void              __FASTCALL__ twSetFrameAttr(TWindow *win,const char *frame,ColorAttr attr);

                   /** Updates window title with specified physical color attributes.
                     * @param win          handle of window
                     * @param title        specifies new title
                     * @param titlemode    specifies new title mode
                     * @param attr         specifies new color attributes
                     * @return             none
                     * @see                twGetFrame twGetFrameAttr twGetFooterAttr twSetFrameAttr
                     *                     twGetTitle twGetTitleAttr twSetFrame twSetTitle
                     *                     twGetFooter twSetFooter twSetFooterAttr
                    **/
void              __FASTCALL__ twSetTitleAttr(TWindow *win,const char* title,tTitleMode titlemode,ColorAttr attr);

                   /** Updates window footer with specified physical color attributes.
                     * @param win          handle of window
                     * @param footer       specifies new footer
                     * @param footermode   specifies new footer mode
                     * @param attr         specifies new logical color attributes
                     * @return             none
                     * @see                twGetFrame twGetFrameAttr twGetFooterAttr twSetFrameAttr
                     *                     twGetTitle twGetTitleAttr twSetFrame twSetTitleAttr
                     *                     twGetFooter twSetTitle twSetFooter
                    **/
void              __FASTCALL__ twSetFooterAttr(TWindow *win,const char* footer,tTitleMode footermode,ColorAttr attr);

#define TW_CUR_OFF   0 /**< Defines that cursor in invisible state */
#define TW_CUR_NORM  1 /**< Defines that cursor in normal state (filles 20% of the character cell) */
#define TW_CUR_SOLID 2 /**< Defines that cursor in solid state (filles 100% of the character cell) */

                   /** Sets the size and visibility of the cursor
                     * @param  type       indicates type of cursor
                     * @return            none
                     * @note              The cursor in particular window is
                     *                    visible only when the type of cursor
                     *                    is normal or solid and window is
                     *                    visible and has focus. Main difference
                     *                    between twSetCursorType and __vioSetCursorType
                     *                    is rememberring of cursor type and caching
                     *                    every call to the OS.
                     * @see               twGetCursorType
                    **/
void              __FASTCALL__ twSetCursorType(int type);

                   /** Retrieves information about the size and visibility of the cursor
                     * @param             none
                     * @return            Current type of the cursor
                     * @note              Main difference between twGetCursorType
                     *                    and __vioGetCursorType is rememberring
                     *                    of cursor type and caching every call
                     *                    to the OS.
                     * @see               twSetCursorType
                    **/
int               __FASTCALL__ twGetCursorType( void );

                   /** Sets the cursor coordiantes relative to the used window
                     * @param x,y         specify horizontal and vertical coordinates for the location
                     * @return            none
                     * @note              If X or Y are outside the window frame,
                     *                    they will be clipped.
                     * @see               twWhereX twWhereY
                    **/
void              __FASTCALL__ twGotoXY(tRelCoord x,tRelCoord y);

                   /** Returns the x coordinate of the current cursor position, within currently used window.
                     * @return            none
                     * @note              If X or Y are outside the window frame,
                     *                    they will be clipped.
                     * @see               twWhereX twWhereY
                    **/
tRelCoord         __FASTCALL__ twWhereX( void );

                   /** Returns the y coordinate of the current cursor position, within currently used window.
                     * @return            none
                     * @note              If X or Y are outside the window frame,
                     *                    they will be clipped.
                     * @see               twWhereX twWhereY
                    **/
tRelCoord         __FASTCALL__ twWhereY( void );

                   /** Outputs the character to the active window at current cursor position.
                     * @param ch          character to be written
                     * @return            none
                     * @see               twGetChar twPutS twPrintF twDirectWrite twWriteBuffer
                    **/
void              __FASTCALL__ twPutChar(char ch);

                   /** Reads the character from the active window at current cursor position.
                     * @return            character read
                     * @note              there is no error return value.
                     * @see               twPutChar twPutS twPrintF twDirectWrite twWriteBuffer twReadBuffer
                    **/
char              __FASTCALL__ twGetChar( void );

                   /** Writes the null-terminated string to the active window at current cursor position.
                     * @param str         null-terminated string to be written
                     * @return            number of really written characters
                     * @note              Function has special reaction on carriage
                     *                    return and linefeed characters. If such
                     *                    characters occured, then cursor position
                     *                    will be changed automatically.
                     *                    Character '\n' changes both x and y
                     *                    cursor coordinates (for unix compatibility)
                     *                    as x=1 and y++
                     *                    Character '\r' changes only x coordinate
                     *                    as x=1.
                     *                    If written text is large than window, then
                     *                    it will be clipped.
                     * @see               twGetChar twPutChar twPrintF twDirectWrite twWriteBuffer
                    **/
int               __FASTCALL__ twPutS(const char *str);

                   /** Writes buffer directly to the active window at specified location.
                     * @param x,y         specify location of output
                     * @param buff        specifies buffer to be written
                     * @param len         specifies length of buffer
                     * @return            number of really written characters
                     * @note              Function writes all characters as is,
                     *                    without checking for carriage return and
                     *                    linefeed.
                     *                    If written text is large than window, then
                     *                    it will be clipped.
                     * @see               twGetChar twPutChar twPrintF twPutS twWriteBuffer
                    **/
int               __FASTCALL__ twDirectWrite(tRelCoord x,tRelCoord y,const void *buff,unsigned len);

                   /** Provides formatted output directly to the active window at current cursor position.
                     * @param fmt         specifies formatted string
                     * @return            number of really written characters
                     * @note              Function has same input format as
                     *                    standard *printf function family.
                     *                    Functiom has same reaction on carriage
                     *                    return and linefeed as \e twPutS.
                     *                    If written text is large than window, then
                     *                    it will be clipped.
                     * @bug               Program is destroyed, when final output
                     *                    is greated than size of console or terminal
                     *                    screen. I.e. greated than tvioWidth*tvioHeight
                     * @see               twGetChar twPutChar twDirectWrite twPutS twWriteBuffer
                    **/
int               __FASTCALL__ twPrintF(const char *fmt,...);

                   /** Accesses to the active window directly, writing a single line.
                     * @param win         handle of window
                     * @param x,y         specify location of output
                     * @param buff        pointer to virtual video buffer to be written
                     * @param len         specifies length of virtual video buffer
                     * @return            none
                     * @note              Function writes specified number of
                     *                    characters and attributes to window,
                     *                    beginning at specified location.
                     *                    If written output is large than window,
                     *                    then it will be clipped.
                     * @see               twGetChar twPutChar twDirectWrite twPutS twReadBuffer
                    **/
void              __FASTCALL__ twWriteBuffer(TWindow *win,tRelCoord x,tRelCoord y,const tvioBuff *buff,unsigned len);

                   /** Accesses to the active window directly, reading a single line.
                     * @param win         handle of window
                     * @param x,y         specify location of input
                     * @param buff        pointer to location at which readed material will be stored.
                     * @param len         specifies length of location
                     * @return            none
                     * @note              Function reads specified number of
                     *                    characters and attributes from window,
                     *                    beginning at specified location.
                     * @see               twGetChar twPutChar twDirectWrite twPutS twWriteBuffer
                    **/
void              __FASTCALL__ twReadBuffer(TWindow *win,tRelCoord x,tRelCoord y,tvioBuff *buff,unsigned len);

                   /** Remaps logical color with new physical value.
                     * @param color       lopical color to be remapped
                     * @param value       specifies new physical value of logical color
                     * @return            none
                     * @see               twGetRemappedColor
                    **/
void              __FASTCALL__ twRemapColor(Color color,unsigned char value);

                   /** Returns physical value of logical color.
                     * @param color       lopical color value
                     * @param value       specifies new physical value of logical color
                     * @return            physical value of logical color
                     * @see               twRemapColor
                    **/
unsigned char     __FASTCALL__ twGetMappedColor(Color color);

#ifdef __cplusplus
}
#endif

#include "biewlib/tw_class.h"

#endif






