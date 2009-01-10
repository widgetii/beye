/**
 * @namespace   biewlib
 * @file        biewlib/twin.c
 * @brief       This file contains implementation of Text Window manager.
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
 * @todo        Accelerate windows interaction algorithm
**/

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#ifdef __GNUC__
#include <unistd.h>
#endif

#include "biewlib/twin.h"
#include "biewlib/pmalloc.h"

const char TW_SINGLE_FRAME[8] = { TWC_Sd_Sr, TWC_SH, TWC_Sl_Sd, TWC_SV, TWC_SV, TWC_Su_Sr, TWC_SH, TWC_Sl_Su };
const char TW_DOUBLE_FRAME[8] = { TWC_Dd_Dr, TWC_DH, TWC_Dl_Dd, TWC_DV, TWC_DV, TWC_Du_Dr, TWC_DH, TWC_Dl_Du };
const char TW_MEDIUM_FRAME[8] = { TWC_FL_BLK, TWC_UP_HBLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_LF_HBLK, TWC_FL_BLK };
const char TW_THICK_FRAME[8] = { TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK };
const char TW_UP3D_FRAME[8] = "\x00\x00\x00\x00\x00\x00\x00\x00";
const char TW_DN3D_FRAME[8] = "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";

#ifndef NDEBUG
#ifdef __TSC__
#pragma save , call(inline=>on) /** GPF in protmode */
static void winInternalError( void ) = { 0xFF, 0xFF };
#elif defined _MSC_VER
static inline void winInternalError( void ) {};
#else
static inline void winInternalError( void ) { (void)0xFFFFFFFF; };
#endif
static tBool __NEAR__ __FASTCALL__ test_win(TWindow *win)
{
 tBool ret;
 ret = *((void **)(win->body.chars + win->wsize)) == win->body.chars &&
       *((void **)(win->body.oem_pg + win->wsize)) == win->body.oem_pg &&
       *((void **)(win->body.attrs + win->wsize)) == win->body.attrs &&
       *((void **)(win->saved.chars + win->wsize)) == win->saved.chars &&
       *((void **)(win->saved.oem_pg + win->wsize)) == win->saved.oem_pg &&
       *((void **)(win->saved.attrs + win->wsize)) == win->saved.attrs ? True : False;
 return ret;
}
#define CHECK_WINS(x) { if(!test_win(x)) winInternalError(); }
#else
#define CHECK_WINS(x)
#endif

#define IFLG_VISIBLE       0x00000001UL
#define IFLG_ENABLED       0x00000002UL
#define IFLG_CURSORBEENOFF 0x80000000UL

TWindow *active;

static void __NEAR__ __FASTCALL__ __NORETURN__ winerr(const char *str) { printm("\n\nInternal twin library error: %s\n",str); _exit(EXIT_FAILURE); }
static void __NEAR__ __FASTCALL__ wputc_oem(char ch,char oempg,char color,tBool update);
static void __NEAR__ __FASTCALL__ paint_internal(void);

#define wputc(ch,color,update) wputc_oem((char)ch,(char)0,(char)color,(tBool)update)

static TWindow *head = NULL;
static TWindow *cursorwin = NULL;

static unsigned long twin_flags = 0L;

#define __FIND_OVER(ret,win,x,y)\
{\
  TWindow *iter;\
  tAbsCoord xx,yy;\
  iter = head;\
  ret = NULL;\
  xx = win->X1+x;\
  yy = win->Y1+y;\
  while( iter && iter != win )\
  {\
    if((iter->iflags & IFLG_VISIBLE) == IFLG_VISIBLE)\
    {\
      if((yy >= iter->Y1) && (yy < iter->Y2) &&\
         (xx >= iter->X1) && (xx < iter->X2))\
                       ret = iter;\
    }\
    iter = iter->next;\
  }\
}

#define IS_VALID_XY(win,x,y)\
  ((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE ?\
   (tAbsCoord)x &&\
   (tAbsCoord)x < win->wwidth-1 &&\
   (tAbsCoord)y &&\
   (tAbsCoord)y < win->wheight-1 :\
   (tAbsCoord)x < win->wwidth &&\
   (tAbsCoord)y < win->wheight)

#define IS_VALID_X(win,x)\
  ((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE ?\
   (tAbsCoord)x &&\
   (tAbsCoord)x < win->wwidth-1:\
   (tAbsCoord)x < win->wwidth)

#define IS_VALID_Y(win,y)\
  ((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE ?\
   (tAbsCoord)y &&\
   (tAbsCoord)y < win->wheight-1 :\
   (tAbsCoord)y < win->wheight)

static tBool __NEAR__ __FASTCALL__ isOverlapped(TWindow *win)
{
  TWindow *iter;
  tBool ret = False;
  iter = head;
  while( iter && iter != win )
  {
    if((iter->iflags & IFLG_VISIBLE) == IFLG_VISIBLE)
    {
      if(!((iter->Y2 <= win->Y1) || (iter->Y1 >= win->Y2) ||
           (iter->X2 <= win->X1) || (iter->X1 >= win->X2)))
      {
        ret = True;
        break;
      }
    }
    iter = iter->next;
  }
  return ret;
}

/*
   In most cases all output is being performed into non overlapped windows.
   We should turn on special optimization for such windows, though overlapped
   windows will seem relatively slower. There are additional possibilities to
   accelerate all output by computing non overlapped lines of window or parts
   of lines, but I don't want to do such complex task.
*/
#define __TOPMOST(win) (!isOverlapped(win))

#define __ATHEAD(win)\
{\
  if(head) ((TWindow *)win)->next = head;\
  head = (TWindow *)win;\
}

#define __ATWIN(win,prev)\
{\
  if(!prev) __ATHEAD(win)\
  else\
  {\
     ((TWindow *)win)->next = ((TWindow *)prev)->next;\
     ((TWindow *)prev)->next = (TWindow *)win;\
  }\
}

#define __AT_POINT(iter,x,y)\
{\
  iter = head;\
  while( iter )\
  {\
    if((iter->iflags & IFLG_VISIBLE) == IFLG_VISIBLE)\
    {\
      if(((tAbsCoord)y >= iter->Y1) && ((tAbsCoord)y < iter->Y2) &&\
         ((tAbsCoord)x >= iter->X1) && ((tAbsCoord)x < iter->X2))\
                       break;\
    }\
    iter = iter->next;\
  }\
}

static int c_type = -1;
void __FASTCALL__ twSetCursorType(int type)
{
  if(type != c_type)
  {
    __vioSetCursorType(type);
    c_type = type;
  }
}

int __FASTCALL__ twGetCursorType( void )
{
  if(c_type == -1) c_type = __vioGetCursorType();
  return c_type;
}

void __FASTCALL__ twInit(const char *user_cp, unsigned long vio_flags, unsigned long twin_flgs )
{
  twin_flags = twin_flgs;
  __init_vio(user_cp?user_cp:"IBM866",vio_flags);
  __init_keyboard();
  if(tvioWidth > __TVIO_MAXSCREENWIDTH)
  {
    char outs[256];
    twDestroy();
    sprintf(outs,"Size of video buffer is too large: %u (max = %u)",tvioWidth,__TVIO_MAXSCREENWIDTH);
    winerr(outs);
  }
  twSetCursorType(TW_CUR_OFF);
}

void __FASTCALL__ twDestroy( void )
{
  twSetCursorType(TW_CUR_NORM);
  twcDestroyClassSet();
  __term_keyboard();
  __term_vio();
}
/*
  Hypothesis:
    When 16-bits color is coded then weight of component is:
    Blue  - 4 bits
    Green - 6 bits
    Red   - 6 bits
  cause of human eye more sensitivity for blue color,
  therefore weight of colors must will:

  Blue      - 6 balls
  Green     - 4 balls
  Red       - 4 balls

  Brightness is 128 192 255 i.e.:
                2   3   4
  Cause of programs can used multiple color conversation better use same
  values for each color component
*/
#define BR_BLUE  1
#define BR_GREEN 1
#define BR_RED   1

static const unsigned char brightness[16] =
{
        /* RGB: Name */
    BR_RED*0+BR_GREEN*0+BR_BLUE*0, /**< 000: Black */
    BR_RED*0+BR_GREEN*0+BR_BLUE*2, /**< 002: Blue */
    BR_RED*0+BR_GREEN*2+BR_BLUE*0, /**< 020: Green */
    BR_RED*0+BR_GREEN*2+BR_BLUE*2, /**< 022: Cyan */
    BR_RED*2+BR_GREEN*0+BR_BLUE*0, /**< 200: Red */
    BR_RED*2+BR_GREEN*0+BR_BLUE*2, /**< 202: Magenta */
    BR_RED*2+BR_GREEN*2+BR_BLUE*0, /**< 220: Brown */
    BR_RED*3+BR_GREEN*3+BR_BLUE*3, /**< 333: LightGray */
    BR_RED*2+BR_GREEN*2+BR_BLUE*2, /**< 222: Gray */
    BR_RED*0+BR_GREEN*0+BR_BLUE*4, /**< 004: LightBlue */
    BR_RED*0+BR_GREEN*4+BR_BLUE*0, /**< 040: LightGreen */
    BR_RED*0+BR_GREEN*4+BR_BLUE*4, /**< 044: LightCyan */
    BR_RED*4+BR_GREEN*0+BR_BLUE*0, /**< 400: LightRed */
    BR_RED*4+BR_GREEN*0+BR_BLUE*4, /**< 404: LightMagenta */
    BR_RED*4+BR_GREEN*4+BR_BLUE*0, /**< 440: Yellow */
    BR_RED*4+BR_GREEN*4+BR_BLUE*4  /**< 444: White */
};

unsigned char color_map[16] =
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

static void __NEAR__ __FASTCALL__ adjustColor(Color *fore,Color *back)
{
  if(tvioNumColors < 16 || (twin_flags & TWIF_FORCEMONO))
  {
    unsigned char br_fore, br_back;
    br_fore = brightness[(*fore) & 0x0F];
    br_back = brightness[(*back) & 0x0F];
    if(br_fore >= br_back)
    {
      *fore = LightGray;
      *back = Black;
    }
    else
    {
      *fore = Black;
      *back = LightGray;
    }
  }
  (*fore) = color_map[(*fore) & 0x0F];
  (*back) = color_map[(*back) & 0x0F];
}

static void __NEAR__ __FASTCALL__ __set_color(DefColor *to,Color fore,Color back)
{
  to->user = LOGFB_TO_PHYS(fore,back);
  adjustColor(&fore,&back);
  to->system = LOGFB_TO_PHYS(fore,back);
}

ColorAttr __FASTCALL__ twSetColor(Color fore,Color back)
{
  ColorAttr ret;
  ret = active->text.user;
  __set_color(&active->text,fore,back);
  return ret;
}

ColorAttr __FASTCALL__ twSetColorAttr(ColorAttr ca)
{
  return twSetColor(FORE_COLOR(ca),BACK_COLOR(ca));
}

ColorAttr __FASTCALL__ twGetColorAttr( void ) { return active->text.user; }

void __FASTCALL__ twGetColor(Color *fore,Color *back)
{
  ColorAttr ca = twGetColorAttr();
  PHYS_TO_LOGFB(ca,*fore,*back);
}

Color __FASTCALL__ twTextColor( Color col )
{
  Color back,ret;
  ColorAttr attr;
  attr = twGetColorAttr();
  back = BACK_COLOR(attr);
  ret = FORE_COLOR(attr);
  twSetColor(col, back);
  return ret;
}

Color __FASTCALL__ twTextBkGnd( Color col )
{
   Color fore,ret;
   ColorAttr attr;
   attr = twGetColorAttr();
   fore = FORE_COLOR(attr);
   ret = BACK_COLOR(attr);
   twSetColor(fore, col);
   return ret;
}

void __FASTCALL__ twSetFrame(TWindow *win,const char *frame,Color fore,Color back)
{
  TWindow *tmp;
  win->flags |= TWS_FRAMEABLE;
  memcpy(win->Frame,frame,8);
  __set_color(&win->frame,fore,back);
  tmp = active;
  active = win;
  paint_internal();
  active = tmp;
}

void __FASTCALL__ twSetFrameAttr(TWindow *win,const char *frame,ColorAttr attr)
{
   twSetFrame(win,frame,FORE_COLOR(attr),BACK_COLOR(attr));
}

void __FASTCALL__ twGetFrameAttr(TWindow *win,char *frame,ColorAttr* attr)
{
  if((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE)
  {
    memcpy(frame,active->Frame,8);
    *attr = win->frame.user;
  }
}

void __FASTCALL__ twGetFrame(TWindow *win,char *frame,Color* fore,Color* back)
{
  ColorAttr attr;
  twGetFrameAttr(win,frame,&attr);
  *fore = FORE_COLOR(attr);
  *back = BACK_COLOR(attr);
}

void __FASTCALL__ twSetTitle(TWindow *win,const char *title,tTitleMode mode,Color fore,Color back)
{
  TWindow *tmp;
  unsigned slen;
  slen = strlen(title);
  if(win->Title) PFREE(win->Title);
  win->Title = PMalloc(slen+1);
  if(!win->Title) winerr("Out of memory!");
  strcpy(win->Title,title);
  if((win->flags & TWS_NLSOEM) == TWS_NLSOEM)
         __nls_OemToOsdep((unsigned char *)win->Title,slen);
  win->TitleMode = mode;
  __set_color(&win->title,fore,back);
  tmp = active;
  active = win;
  paint_internal();
  active = tmp;
}

void __FASTCALL__ twSetTitleAttr(TWindow *win,const char *title,tTitleMode mode,ColorAttr attr)
{
  twSetTitle(win,title,mode,FORE_COLOR(attr),BACK_COLOR(attr));
}

tTitleMode __FASTCALL__ twGetTitleAttr(TWindow *win,char *title,unsigned cb_title,ColorAttr* attr)
{
  tTitleMode ret = 0;
  if((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE && win->Title)
  {
    strncpy(title,win->Title,cb_title);
    title[cb_title-1] = '\0';
    *attr = win->title.user;
    ret = win->TitleMode;
  }
  return ret;
}

tTitleMode __FASTCALL__ twGetTitle(TWindow *win,char *title,unsigned cb_title,Color* fore,Color* back)
{
  ColorAttr attr;
  tTitleMode ret;
  ret = twGetTitleAttr(win,title,cb_title,&attr);
  *fore = FORE_COLOR(attr);
  *back = BACK_COLOR(attr);
  return ret;
}

void __FASTCALL__ twSetFooter(TWindow *win,const char *footer,tTitleMode mode,Color fore,Color back)
{
  TWindow *tmp;
  unsigned slen;
  slen = strlen(footer);
  if(win->Footer) PFREE(win->Footer);
  win->Footer = PMalloc(slen+1);
  if(!win->Footer) winerr("Out of memory!");
  strcpy(win->Footer,footer);
  if((win->flags & TWS_NLSOEM) == TWS_NLSOEM)
       __nls_OemToOsdep((unsigned char *)win->Footer,slen);
  win->FooterMode = mode;
  __set_color(&win->footer,fore,back);
  tmp = active;
  active = win;
  paint_internal();
  active = tmp;
}

void __FASTCALL__ twSetFooterAttr(TWindow *win,const char *footer,tTitleMode mode,ColorAttr attr)
{
  twSetFooter(win,footer,mode,FORE_COLOR(attr),BACK_COLOR(attr));
}

tTitleMode __FASTCALL__ twGetFooterAttr(TWindow *win,char *footer,unsigned cb_footer,ColorAttr* attr)
{
  tTitleMode ret = 0;
  if((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE && win->Footer)
  {
    strncpy(footer,win->Footer,cb_footer);
    footer[cb_footer-1] = '\0';
    *attr = win->footer.user;
    ret = win->FooterMode;
  }
  return ret;
}


tTitleMode __FASTCALL__ twGetFooter(TWindow *win,char *footer,unsigned cb_footer,Color* fore,Color* back)
{
  ColorAttr attr;
  tTitleMode ret;
  ret = twGetFooterAttr(win,footer,cb_footer,&attr);
  *fore = FORE_COLOR(attr);
  *back = BACK_COLOR(attr);
  return ret;
}

static void __NEAR__ __FASTCALL__ __unlistwin(TWindow *win)
{
  if(head == win) head = win->next;
  else
  {
    TWindow *iter;
    iter = head;
    while(iter)
    {
       if(iter->next) if(iter->next == win) iter->next = win->next;
       iter = iter->next;
    }
  }
}

static TWindow * __NEAR__ __FASTCALL__ __prevwin(TWindow *win)
{
  TWindow *ret = NULL;
  if(head != win)
  {
    TWindow *iter;
    iter = head;
    while(iter)
    {
       if(iter->next) if(iter->next == win) { ret = iter; break; }
       iter = iter->next;
    }
  }
  return ret;
}

static TWindow * __NEAR__ __FASTCALL__ __findcursorablewin(void)
{
  TWindow *iter,*ret;
  iter = head;
  ret = NULL;
  while(iter)
  {
     if((iter->flags & TWS_CURSORABLE) == TWS_CURSORABLE) { ret = iter; break; }
     iter = iter->next;
  }
  return ret;
}

TWindow * __FASTCALL__ twGetWinAtPos(tAbsCoord x,tAbsCoord y)
{
  TWindow *ret;
  tAbsCoord xx,yy;
  xx = x-1;
  yy = y-1;
  __AT_POINT(ret,xx,yy);
  return ret;
}

void __FASTCALL__ twCvtWinCoords(TWindow *win,tRelCoord x, tRelCoord y,tAbsCoord *xa,tAbsCoord *ya)
{
  if(win->flags & TWS_FRAMEABLE) { x++; y++; }
  *xa = win->X1+x;
  *ya = win->Y1+y;
}

tBool __FASTCALL__ twCvtScreenCoords(TWindow *win,tAbsCoord x, tAbsCoord y,tRelCoord *xr,tRelCoord *yr)
{
  *xr = x - win->X1;
  *yr = y - win->Y1;
  if(win->flags & TWS_FRAMEABLE) { (*xr)--; (*yr)--; }
  return IS_VALID_XY(win,*xr,*yr) ? True : False;
}

tBool __FASTCALL__ twIsPieceVisible(TWindow *win,tRelCoord x, tRelCoord y)
{
  TWindow *over;
  if(win->flags & TWS_FRAMEABLE) { x++; y++; }
  __FIND_OVER(over,win,win->X1+x,win->Y1+y);
  return over ? True : False;
}

static void __NEAR__ __FASTCALL__ iGotoXY(tRelCoord x,tRelCoord y)
{
  if(x && y)
  {
    active->cur_x = x-1;
    active->cur_y = y-1;
    if(active->cur_x >= active->wwidth) active->cur_x = active->wwidth-1;
    if(active->cur_y >= active->wheight) active->cur_y = active->wheight-1;
  }
}

static void __NEAR__ __FASTCALL__ set_xy(tRelCoord x,tRelCoord y)
{
  tRelCoord width,height;
  if(x && y)
  {
    x--; y--;
    active->cur_x = x;
    active->cur_y = y;
    width = active->wwidth;
    height = active->wheight;
    if((active->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE)
    {
      width --;
      height --;
      active->cur_x++;
      active->cur_y++;
    }
    if(active->cur_x > width) active->cur_x = width-1;
    if(active->cur_y > height) active->cur_y = height-1;
  }
}

static void __NEAR__ __FASTCALL__ paint_cursor( void )
{
  TWindow * top;
  static tAbsCoord c_x = UCHAR_MAX, c_y = UCHAR_MAX;
  int _c_type = -1;
  unsigned x,y;
  int type;
  if(cursorwin && (cursorwin->iflags & IFLG_ENABLED) == IFLG_ENABLED)
  {
    __FIND_OVER(top,cursorwin,cursorwin->X1+cursorwin->cur_x,cursorwin->Y1+cursorwin->cur_y-1);
    if(!top && (cursorwin->iflags & IFLG_VISIBLE) == IFLG_VISIBLE && cursorwin == active)
    {
        type = twGetCursorType();
        if(type == TW_CUR_OFF)
        {
         twSetCursorType(_c_type == -1 ? TW_CUR_NORM : _c_type);
         _c_type = _c_type == -1 ? TW_CUR_NORM : _c_type;
        }
        x = active->X1 + active->cur_x;
        y = active->Y1 + active->cur_y;
        if(!(x == c_x && y == c_y))
        {
          __vioSetCursorPos(x,y);
          c_x = x;
          c_y = y;
        }
    }
    else goto hide_cursor;
  }
  else
  {
    c_x = c_y = UCHAR_MAX;
    hide_cursor:
    type = twGetCursorType();
    if(type != TW_CUR_OFF)
    {
      _c_type = type;
      twSetCursorType(TW_CUR_OFF);
    }
  }
}

void __FASTCALL__ twGotoXY(tRelCoord x,tRelCoord y)
{
  set_xy(x,y);
  paint_cursor();
}

static TWindow * __NEAR__ __FASTCALL__ makewin(tAbsCoord x1, tAbsCoord y1, tAbsCoord width, tAbsCoord height)
{
  TWindow *win;
  unsigned size;
  win = PMalloc(sizeof(TWindow));
  if(win)
  {
    memset(win,0,sizeof(TWindow));
    size = width*height;
    win->wsize = size;
    win->wwidth = width;
    win->wheight = height;
    if(!(win->body.chars = PMalloc(size*sizeof(t_vchar)
#ifndef NDEBUG
                                   +sizeof(void *)
#endif
      )))
    {
     bye0:
      PFREE(win);
      return NULL;
    }
    if(!(win->body.oem_pg = PMalloc(size*sizeof(t_vchar)
#ifndef NDEBUG
                                   +sizeof(void *)
#endif
      )))
    {
     bye1:
      PFREE(win->body.chars);
      goto bye0;
    }
    if(!(win->body.attrs = PMalloc(size*sizeof(ColorAttr)
#ifndef NDEBUG
                                   +sizeof(void *)
#endif
      )))
    {
     bye2:
      PFREE(win->body.oem_pg);
      goto bye1;
    }
    if(!(win->saved.chars = PMalloc(size*sizeof(t_vchar)
#ifndef NDEBUG
                                   +sizeof(void *)
#endif
      )))
    {
     bye3:
      PFREE(win->body.attrs);
      goto bye2;
    }
    if(!(win->saved.oem_pg = PMalloc(size*sizeof(t_vchar)
#ifndef NDEBUG
                                   +sizeof(void *)
#endif
      )))
    {
     bye4:
      PFREE(win->saved.chars);
      goto bye3;
    }
    if(!(win->saved.attrs = PMalloc(size*sizeof(ColorAttr)
#ifndef NDEBUG
                                   +sizeof(void *)
#endif
      )))
    {
      PFREE(win->saved.oem_pg);
      goto bye4;
    }
#ifndef NDEBUG
    *((void **)(win->body.chars + size)) = win->body.chars;
    *((void **)(win->saved.chars + size)) = win->saved.chars;
    *((void **)(win->body.oem_pg + size)) = win->body.oem_pg;
    *((void **)(win->saved.oem_pg + size)) = win->saved.oem_pg;
    *((void **)(win->body.attrs + size)) = win->body.attrs;
    *((void **)(win->saved.attrs + size)) = win->saved.attrs;
#endif
    win->X1 = x1;
    win->Y1 = y1;
    win->X2 = x1+width;
    win->Y2 = y1+height;
    __ATHEAD(win);
  }
  return win;
}

/**
 *  Three basic functions for copying from buffer to screen:
 *  ========================================================
 *  updatescreencharfrombuff: low level implementation updatescreen family
 *  updatescreenchar:  correctly copyed user record from win to screen
 *  restorescreenchar: correctly copyed window memory from win to screen
 */

static void __NEAR__ __FASTCALL__ updatescreencharfrombuff(TWindow *win,
                                              tRelCoord x,
                                              tRelCoord y,
                                              tvioBuff *buff,
                                              tvioBuff *accel)
{
  tvioBuff it;
  unsigned idx,aidx;
  if((win->iflags & IFLG_VISIBLE) == IFLG_VISIBLE)
  {
    TWindow * top;
    idx = y*win->wwidth+x;
    aidx = x;
    __FIND_OVER(top,win,x,y);
    if(top)
    {
      unsigned tidx;
      tAbsCoord tx,ty;
      tx = win->X1 - top->X1 + x;
      ty = win->Y1 - top->Y1 + y;
      tidx = tx + ty*top->wwidth;
      top->saved.chars[tidx] = buff->chars[idx];
      top->saved.oem_pg[tidx] = buff->oem_pg[idx];
      top->saved.attrs[tidx] = buff->attrs[idx];
      if(accel)
      {
        TWindow *vis;
        tAbsCoord xx,yy;
        xx = x+win->X1;
        yy = y+win->Y1;
        __AT_POINT(vis,xx,yy);
        tx = xx - vis->X1;
        ty = yy - vis->Y1;
        tidx = tx + ty*vis->wwidth;
        accel->chars[aidx] = vis->body.chars[tidx];
        accel->oem_pg[aidx] = vis->body.oem_pg[tidx];
        accel->attrs[aidx] = vis->body.attrs[tidx];
      }
      CHECK_WINS(top);
    }
    else
    {
      tBool ms_vis;
      tBool is_hidden = False;
      if(accel)
      {
        accel->chars[aidx] = buff->chars[idx];
        accel->oem_pg[aidx] = buff->oem_pg[idx];
        accel->attrs[aidx] = buff->attrs[idx];
      }
      else
      {
        if((win->iflags & IFLG_ENABLED) == IFLG_ENABLED)
        {
          tAbsCoord outx,outy;
          ms_vis = __MsGetState();
          outx = (unsigned)win->X1+x;
          outy = (unsigned)win->Y1+y;
          if(outx+1 <= tvioWidth && outy <= tvioHeight)
          {
            if(ms_vis)
            {
               tAbsCoord mx,my;
               __MsGetPos(&mx,&my);
               if(mx == outx && my == outy)
               {
                 is_hidden = True;
                 __MsSetState(False);
               }
            }
            it.chars = &buff->chars[idx];
            it.oem_pg = &buff->oem_pg[idx];
            it.attrs = &buff->attrs[idx];
            __vioWriteBuff(outx,outy,&it,1);
            if(is_hidden) __MsSetState(True);
          }
          CHECK_WINS(win);
        }
      }
    }
  }
}

#define updatescreenchar(win,x,y,accel)\
   updatescreencharfrombuff((TWindow *)win,\
                            (tRelCoord)x-1,\
                            (tRelCoord)y-1,\
                            &((TWindow *)win)->body,\
                            (tvioBuff *)accel)

#define restorescreenchar(win,x,y,accel)\
   updatescreencharfrombuff((TWindow *)win,\
                            (tRelCoord)x-1,\
                            (tRelCoord)y-1,\
                            &((TWindow *)win)->saved,\
                            (tvioBuff *)accel)
/**
 *  Three basic functions for copying from buffer to screen:
 *  ========================================================
 *  updatewinmemcharfromscreen: correctly copied screen to window memory
 *  screen2win: quick implementation of copying screen to window memory
 *  snapshot:   snap shot of screen to win body
 */

static void __NEAR__ __FASTCALL__ updatewinmemcharfromscreen(TWindow *win,tRelCoord x,tRelCoord y,tvioBuff *accel)
{
  unsigned idx,aidx;
  tvioBuff it;
  if((win->iflags & IFLG_VISIBLE) == IFLG_VISIBLE)
  {
    TWindow * top;
    idx = y*win->wwidth+x;
    aidx = x;
    __FIND_OVER(top,win,x,y);
    if(top)
    {
      unsigned tidx;
      tAbsCoord tx,ty;
      tx = win->X1 - top->X1 + x;
      ty = win->Y1 - top->Y1 + y;
      tidx = tx + ty*top->wwidth;
      win->saved.chars[idx] = top->saved.chars[tidx];
      win->saved.oem_pg[idx] = top->saved.oem_pg[tidx];
      win->saved.attrs[idx] = top->saved.attrs[tidx];
      top->saved.chars[tidx] = win->body.chars[idx];
      top->saved.oem_pg[tidx] = win->body.oem_pg[idx];
      top->saved.attrs[tidx] = win->body.attrs[idx];
      CHECK_WINS(top);
    }
    else
    {
      tBool ms_vis;
      tBool is_hidden = False;
      if(accel)
      {
        win->saved.chars[idx] = accel->chars[aidx];
        win->saved.oem_pg[idx] = accel->oem_pg[aidx];
        win->saved.attrs[idx] = accel->attrs[aidx];
      }
      else
      {
        tAbsCoord inx,iny;
        inx = (unsigned)win->X1+x;
        iny = (unsigned)win->Y1+y;
        if(inx+2 <= tvioWidth && iny <= tvioHeight)
        {
          ms_vis = __MsGetState();
          if(ms_vis)
          {
             tAbsCoord mx,my;
             __MsGetPos(&mx,&my);
             if(mx == inx && my == iny)
             {
               is_hidden = True;
               __MsSetState(False);
             }
          }
          it.chars = &win->saved.chars[idx];
          it.oem_pg = &win->saved.oem_pg[idx];
          it.attrs = &win->saved.attrs[idx];
          __vioReadBuff(inx,iny,&it,2);
          if(is_hidden) __MsSetState(True);
        }
        CHECK_WINS(win);
      }
    }
  }
}

static void __NEAR__ __FASTCALL__ screen2win(TWindow *win)
{
  tvioBuff it;
  unsigned i,lwidth,idx;
  tAbsCoord inx;
  tBool ms_vis;
  tBool is_hidden = False;
  inx = win->X1;
  lwidth = win->wwidth;
  if(inx + lwidth > tvioWidth) lwidth = tvioWidth > inx ? tvioWidth - inx : 0;
  if(lwidth)
  {
    ms_vis = __MsGetState();
    if(ms_vis)
    {
      tAbsCoord mx,my;
      __MsGetPos(&mx,&my);
      if(mx >= inx && mx <= win->X2 && my >= win->Y1 && my <= win->Y2)
      {
        is_hidden = True;
        __MsSetState(False);
      }
    }
    if(win->wwidth == tvioWidth && !win->X1)
    {
      __vioReadBuff(0, win->Y1, &win->saved, win->wwidth*win->wheight);
    }
    else
    {
      for(i = 0;i < win->wheight;i++)
      {
        tAbsCoord iny;
        iny = win->Y1+i;
        if(iny <= tvioHeight)
        {
          idx = i*win->wwidth;
          it.chars = &win->saved.chars[idx];
          it.oem_pg = &win->saved.oem_pg[idx];
          it.attrs = &win->saved.attrs[idx];
          __vioReadBuff(inx,iny,&it,lwidth);
        }
        else break;
      }
    }
    if(is_hidden) __MsSetState(True);
  }
  CHECK_WINS(win);
}

void __FASTCALL__ twSnapShot(TWindow *win) /**< for snapshot */
{
  tvioBuff it;
  unsigned i,lwidth,idx;
  tAbsCoord inx;
  tBool ms_vis;
  tBool is_hidden = False;
  inx = win->X1;
  lwidth = win->wwidth;
  if(inx + lwidth > tvioWidth) lwidth = tvioWidth > inx ? tvioWidth - inx : 0;
  if(lwidth)
  {
    ms_vis = __MsGetState();
    if(ms_vis)
    {
      tAbsCoord mx,my;
      __MsGetPos(&mx,&my);
      if(mx >= inx && mx <= win->X2 && my >= win->Y1 && my <= win->Y2)
      {
        is_hidden = True;
        __MsSetState(False);
      }
    }
    for(i = 0;i < win->wheight;i++)
    {
      tAbsCoord iny;
      iny = win->Y1+i;
      if(iny <= tvioHeight)
      {
        idx = i*win->wwidth;
        it.chars = &win->body.chars[idx];
        it.oem_pg = &win->body.oem_pg[idx];
        it.attrs = &win->body.attrs[idx];
        __vioReadBuff(inx,iny,&it,lwidth);
      }
      else break;
    }
    if(is_hidden) __MsSetState(True);
  }
  CHECK_WINS(win);
}

/**
 *  Helpful functions:
 *  ==================
 *  savedwin2screen:  restore entire window memory to screen
 *  updatescreen:     restore entire user record of window to screen
 *  updatescreenpiece:restore one piece of line from user record of window to screen
 *  updatewinmem:     update entire window memory from screen
 */

static void __NEAR__ __FASTCALL__ savedwin2screen(TWindow *win)
{
  unsigned i,j, tidx;
  tvioBuff accel;
  t_vchar chars[__TVIO_MAXSCREENWIDTH];
  t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
  ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
  tBool ms_vis, is_hidden = False, is_top;
  accel.chars = chars;
  accel.oem_pg = oem_pg;
  accel.attrs = attrs;
  if((win->iflags & IFLG_VISIBLE) == IFLG_VISIBLE)
  {
    tAbsCoord mx,my;
    ms_vis = __MsGetState();
    if(ms_vis)
    {
      __MsGetPos(&mx,&my);
      if(mx >= win->X1 && mx <= win->X2 && my >= win->Y1 && my <= win->Y2)
      {
        is_hidden = True;
        __MsSetState(False);
      }
    }
    is_top = __TOPMOST(win);
    if(is_top && win->wwidth == tvioWidth && !win->X1)
    {
      /* Special case of redrawing window interior at one call */
      __vioWriteBuff(0, win->Y1, &win->saved, win->wwidth*win->wheight);
    }
    else
    {
      for(i = 0;i < win->wheight;i++)
      {
        tAbsCoord outx,outy;
        unsigned nwidth;
        if(!is_top)
          for(j = 0;j < win->wwidth;j++) restorescreenchar(win,j+1,i+1,&accel);
        else
        {
           tidx = i*win->wwidth;
           accel.chars = &win->saved.chars[tidx];
           accel.attrs = &win->saved.attrs[tidx];
           accel.oem_pg = &win->saved.oem_pg[tidx];
        }
        outx = win->X1;
        outy = win->Y1+i;
        nwidth = win->wwidth;
        if(outx + nwidth > tvioWidth) nwidth = tvioWidth > outx ? tvioWidth - outx : 0;
        if(outy <= tvioHeight && nwidth)
            __vioWriteBuff(outx,outy,&accel,nwidth);
      }
    }
    if(is_hidden) __MsSetState(True);
    CHECK_WINS(win);
  }
}

static void __NEAR__ __FASTCALL__ updatescreen(TWindow *win,tBool full_area)
{
  unsigned i,j, tidx;
  tAbsCoord xs,xe,ys,ye,cx,rw;
  unsigned aoff;
  tvioBuff accel,it;
  t_vchar chars[__TVIO_MAXSCREENWIDTH];
  t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
  ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
  tBool ms_vis, is_hidden = False, is_top;
  accel.chars = chars;
  accel.oem_pg = oem_pg;
  accel.attrs = attrs;
  if((win->iflags & IFLG_VISIBLE) == IFLG_VISIBLE)
  {
    tAbsCoord mx,my;
    ms_vis = __MsGetState();
    if(ms_vis)
    {
      xs = win->X1;
      xe = win->X2;
      ys = win->Y1;
      ye = win->Y2;
      if((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE && (!full_area))
      {
        xs++; xe--; ys++; ye--;
      }
      __MsGetPos(&mx,&my);
      if(mx >= xs && mx <= xe && my >= ys && my <= ye)
      {
        is_hidden = True;
        __MsSetState(False);
      }
    }
    is_top = __TOPMOST(win);
    if(is_top && full_area && win->wwidth == tvioWidth && !win->X1 && (win->iflags & IFLG_ENABLED) == IFLG_ENABLED)
    {
      /* Special case of redrawing window interior at one call */
      __vioWriteBuff(0, win->Y1, &win->body, win->wwidth*win->wheight);
    }
    else
    {
      xs = ys = 0;
      xe = win->wwidth;
      ye = win->wheight;
      cx = win->X1;
      rw = win->wwidth;
      aoff = 0;
      if((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE && (!full_area))
      {
          xs++; xe--; ys++; ye--;
          cx++; rw-=2;
          aoff = 1;
      }
      for(i = ys;i < ye;i++)
      {
        tAbsCoord outy;
        if(!is_top)
          for(j = xs;j < xe;j++) updatescreenchar(win,j+1,i+1,&accel);
        if((win->iflags & IFLG_ENABLED) == IFLG_ENABLED)
        {
          outy = win->Y1+i;
          if(cx + rw > tvioWidth) rw = tvioWidth > cx ? tvioWidth - cx : 0;
          if(outy <= tvioHeight && rw)
          {
            tidx = i*win->wwidth+aoff;
            it.chars = is_top ? &win->body.chars[tidx] : &accel.chars[aoff];
            it.oem_pg = is_top ? &win->body.oem_pg[tidx] : &accel.oem_pg[aoff];
            it.attrs = is_top ? &win->body.attrs[tidx] : &accel.attrs[aoff];
            __vioWriteBuff(cx,outy,&it,rw);
          }
        }
      }
    }
    if(is_hidden) __MsSetState(True);
    CHECK_WINS(win);
  }
}

static void __NEAR__ __FASTCALL__ updatescreenpiece(TWindow *win,tRelCoord stx,tRelCoord endx,tRelCoord y)
{
  unsigned i,line, tidx;
  tAbsCoord _stx,_endx;
  tvioBuff accel,it;
  t_vchar chars[__TVIO_MAXSCREENWIDTH];
  t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
  ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
  tBool ms_vis, is_hidden = False, is_top;
  accel.chars = chars;
  accel.oem_pg = oem_pg;
  accel.attrs = attrs;
  if((win->iflags & IFLG_VISIBLE) == IFLG_VISIBLE)
  {
    tAbsCoord mx,my;
    ms_vis = __MsGetState();
    __MsGetPos(&mx,&my);
    line = win->Y1+y-1;
    if(ms_vis && my == line)
    {
      is_hidden = True;
      __MsSetState(False);
    }
    _stx = min(stx,win->wwidth);
    _endx = min(endx,win->wwidth);
    is_top = __TOPMOST(win);
    if(!is_top)
    {
      if(_stx < _endx)
        for(i = stx;i < endx;i++)
          updatescreenchar(win,i+1,y,&accel);
    }
    if((win->iflags & IFLG_ENABLED) == IFLG_ENABLED)
    {
      tAbsCoord outx;
      unsigned rw;
      outx = win->X1+_stx;
      rw = _endx - _stx;
      if(outx + rw > tvioWidth) rw = tvioWidth > outx ? tvioWidth - outx : 0;
      if(line <= tvioHeight && rw)
      {
        tidx = (y-1)*win->wwidth+_stx;
        it.chars = is_top ? &win->body.chars[tidx] : &accel.chars[_stx];
        it.oem_pg = is_top ? &win->body.oem_pg[tidx] : &accel.oem_pg[_stx];
        it.attrs = is_top ? &win->body.attrs[tidx] : &accel.attrs[_stx];
        __vioWriteBuff(outx,line,&it,rw);
      }
    }
    if(is_hidden) __MsSetState(True);
    CHECK_WINS(win);
  }
}

static void __NEAR__ __FASTCALL__ updatewinmem(TWindow *win)
{
  unsigned i,j,tidx;
  tvioBuff accel;
  t_vchar chars[__TVIO_MAXSCREENWIDTH];
  t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
  ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
  tBool ms_vis, is_hidden = False, is_top;  
  accel.chars = chars;
  accel.oem_pg = oem_pg;
  accel.attrs = attrs;
  if((win->iflags & IFLG_VISIBLE) == IFLG_VISIBLE)
  {
    tAbsCoord mx,my;
    ms_vis = __MsGetState();
    if(ms_vis)
    {
      __MsGetPos(&mx,&my);
      if(mx >= win->X1 && mx <= win->X2 && my >= win->Y1 && my <= win->Y2)
      {
        is_hidden = True;
        __MsSetState(False);
      }
    }
    is_top = __TOPMOST(win);
    if(is_top && win->wwidth == tvioWidth && !win->X1)
    {
      /* Special case of redrawing window interior at one call */
      __vioReadBuff(0, win->Y1, &win->saved, win->wwidth*win->wheight);
    }
    else
    {
      for(i = 0;i < win->wheight;i++)
      {
        tAbsCoord inx,iny;
        unsigned lwidth;
        lwidth = win->wwidth;
        inx = win->X1;
        iny = win->Y1+i;
        if(inx + lwidth > tvioWidth) lwidth = tvioWidth > inx ? tvioWidth - inx : 0;
        if(iny <= tvioHeight && lwidth)
        {
          if(is_top)
          {
            tidx = i*win->wwidth;
            accel.chars = &win->saved.chars[tidx];
            accel.attrs = &win->saved.attrs[tidx];
            accel.oem_pg = &win->saved.oem_pg[tidx];
          }
          __vioReadBuff(inx,iny,&accel,lwidth);
        }
        if(!is_top)
          for(j = 0;j < win->wwidth;j++)
             updatewinmemcharfromscreen(win,j,i,&accel);
      }
    }
    if(is_hidden) __MsSetState(True);
  }
}

static tRelCoord __NEAR__ __FASTCALL__ calc_title_off(tTitleMode mode,unsigned w,unsigned slen)
{
 tRelCoord stx;
   switch(mode)
   {
     case TW_TMODE_LEFT:
                      stx = 2;
                      break;
     case TW_TMODE_CENTER:
                      stx = ((w - slen)>>1) + 1;
                      break;
     default:
     case TW_TMODE_RIGHT:
                      stx = w - slen;
                      break;
   }
   return stx;
}

#define DO_OEM_PG(ch) ((active->flags & TWS_NLSOEM) == TWS_NLSOEM ? NLS_IS_OEMPG(ch) ? ch : 0 : 0)

static void __NEAR__ __FASTCALL__ __draw_frame( tRelCoord xs, tRelCoord ys, tRelCoord xe, tRelCoord ye,
                                   const void *_frame, DefColor color)
{
 unsigned i;
 ColorAttr cfr,csel;
 tRelCoord sx,sy;
 char frm[8],frame[8];
 char up,oem_ch;
 ColorAttr lt = 0,gr = 0,bl = 0;
 sx = active->cur_x;
 sy = active->cur_y;
 cfr = color.system;
 up = 0;
 memcpy(frame,_frame,8);
 if(memcmp(frame,TW_UP3D_FRAME,8) == 0 ||
    memcmp(frame,TW_DN3D_FRAME,8) == 0)
 {
   memcpy(frame,TW_SINGLE_FRAME,8);
   if(tvioNumColors >= 16 && BACK_COLOR(cfr) != Black)
   {
     Color cfbk;
     if(memcmp(_frame,TW_UP3D_FRAME,8) == 0) up = 1;
     else                                    up = 2;
     cfbk = BACK_COLOR(cfr);
     lt = LOGFB_TO_PHYS(cfbk < 8 ? cfbk + 8 : White,cfbk);
     gr = LOGFB_TO_PHYS(cfbk == LightGray ? Gray : LightGray,cfbk);
     bl = LOGFB_TO_PHYS(cfbk == LightGray ? Black : Gray,cfbk);
   }
 }
 memcpy(frm,frame,8);
 if((active->flags & TWS_NLSOEM) == TWS_NLSOEM)
   __nls_OemToOsdep((unsigned char *)frm,sizeof(frm));
 iGotoXY(xs,ys);
 csel = up ? up == 1 ? lt : bl : cfr;
 oem_ch = frame[0];
 wputc_oem(frm[0],DO_OEM_PG(oem_ch),csel,False);
 for(i = xs+1;i < xe; i++)
 {
   iGotoXY(i,ys);
   oem_ch = frame[1];
   wputc_oem(frm[1],DO_OEM_PG(oem_ch),csel,False);
 }
 iGotoXY(xe,ys);
 oem_ch = frame[2];
 wputc_oem(frm[2],DO_OEM_PG(oem_ch),up ? gr : cfr,False);
 for(i = ys+1;i < ye;i++)
 {
   iGotoXY(xs,i);
   oem_ch = frame[3];
   wputc_oem(frm[3],DO_OEM_PG(oem_ch),csel,True);
 }
 csel = up ? up == 1 ? bl : lt : cfr;
 for(i = ys+1;i < ye;i++)
 {
   iGotoXY(xe,i);
   oem_ch = frame[4];
   wputc_oem(frm[4],DO_OEM_PG(oem_ch),csel,True);
 }
 iGotoXY(xs,ye);
 oem_ch = frame[5];
 wputc_oem(frm[5],DO_OEM_PG(oem_ch),up ? gr : cfr,False);
 for(i = xs+1;i < xe; i++)
 {
   iGotoXY(i,ye);
   oem_ch = frame[6];
   wputc_oem(frm[6],DO_OEM_PG(oem_ch),csel,False);
 }
 iGotoXY(xe,ye);
 oem_ch = frame[7];
 wputc_oem(frm[7],DO_OEM_PG(oem_ch),csel,False);
 active->cur_x = sx;
 active->cur_y = sy;

}

static void __NEAR__ __FASTCALL__ make_frame( void )
{
 unsigned i,w,h;
 tRelCoord sx,sy;
 w = active->wwidth;
 h = active->wheight;
 sx = active->cur_x;
 sy = active->cur_y;
 __draw_frame(1,1,w,h,active->Frame,active->frame);
 if(active->Title)
 {
   unsigned slen;
   tRelCoord stx;
   slen = strlen(active->Title);
   if(slen > w) slen = w;
   stx = calc_title_off(active->TitleMode,w,slen);
   for(i = 0;i < slen;i++)
   {
      iGotoXY(stx+i,1);
      wputc(active->Title[i],active->title.system,False);
   }
 }
 if(active->Footer)
 {
   unsigned slen;
   tRelCoord stx;
   slen = strlen(active->Footer);
   if(slen > w) slen = w;
   stx = calc_title_off(active->FooterMode,w,slen);
   for(i = 0;i < slen;i++)
   {
      iGotoXY(stx+i,h);
      wputc(active->Footer[i],active->footer.system,False);
   }
 }
 updatescreenpiece(active,0,active->wwidth,1);
 updatescreenpiece(active,0,active->wwidth,h);
 active->cur_x = sx;
 active->cur_y = sy;
}

static void __NEAR__ __FASTCALL__ paint_internal( void )
{
  if((active->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE) make_frame();
}

void __FASTCALL__ twinDrawFrame(tRelCoord x1, tRelCoord y1, tRelCoord x2, tRelCoord y2,const char *frame,Color fore, Color back)
{
 DefColor dcol;
 tRelCoord sx,sy;
 sx = active->cur_x;
 sy = active->cur_y;
 if(memcmp(frame,TW_UP3D_FRAME,8) == 0 ||
    memcmp(frame,TW_DN3D_FRAME,8) == 0) __set_color(&dcol,fore,back);
 else dcol = active->text;
 if((active->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE)
 {
   x1++; y1++; x2++; y2++;
 }
 __draw_frame(x1,y1,x2,y2,frame,dcol);
 updatescreenpiece(active,x1-1,x2,y1);
 updatescreenpiece(active,x1-1,x2,y2);
 active->cur_x = sx;
 active->cur_y = sy;
}

void __FASTCALL__ twinDrawFrameAttr(tRelCoord x1, tRelCoord y1, tRelCoord x2, tRelCoord y2,const char *frame,ColorAttr attr)
{
  twinDrawFrame(x1,y1,x2,y2,frame,FORE_COLOR(attr),BACK_COLOR(attr));
}

TWindow * __FASTCALL__ twCreateWin(tAbsCoord x1, tAbsCoord y1, tAbsCoord width, tAbsCoord height, unsigned flags)
{
  TWindow *win;
  if((flags & TWS_FRAMEABLE) == TWS_FRAMEABLE) { width ++; height ++; }
  if((win = makewin(x1-1,y1-1,width,height)) != NULL)
  {
    win->flags = flags;
    win->iflags = IFLG_ENABLED;
    memcpy(win->Frame,TW_SINGLE_FRAME,8);
    memset(win->body.chars,TWC_DEF_FILLER,win->wsize);
    memset(win->body.oem_pg,0,win->wsize);
    memset(win->body.attrs,win->text.system,win->wsize);
    CHECK_WINS(win);
    __set_color(&win->text,LightGray,Black);
    __set_color(&win->frame,LightGray,Black);
    __set_color(&win->title,LightGray,Black);
    __set_color(&win->footer,LightGray,Black);
    win->cur_x = win->cur_y = 0;
    twUseWin(win);
    paint_internal();
    if((flags & TWS_VISIBLE) == TWS_VISIBLE) twShowWin(win);
  }
  return win;
}

TWindow *  __FASTCALL__ twCreateWinEx(tAbsCoord x1_, tAbsCoord y1_,
                                      tAbsCoord width, tAbsCoord height,
                                      unsigned flags, TWindow *parent,
                                      const char *classname)
{
  TWindow *ret;
  UNUSED(parent);
  if((ret = twCreateWin(x1_, y1_, width, height, flags)) != NULL)
  {
    TwClass *cls;
    cls = twcFindClass(classname);
    if(cls)
    {
      ret->method = cls->method;
      ret->class_flags = cls->flags;
    }
    twinSendMessage(ret,WM_CREATE,0L,NULL);
  }
  return ret;
}


void __FASTCALL__ twShowWin(TWindow* w)
{
  twinSendMessage(w,WM_SHOW,0L,NULL);
  if(!(w->iflags & IFLG_VISIBLE) == IFLG_VISIBLE)
  {
    w->iflags |= IFLG_VISIBLE;
    __unlistwin(w);
    __ATHEAD(w);
    updatewinmem(w);
    updatescreen(w,True);
    if((w->flags & TWS_CURSORABLE) == TWS_CURSORABLE)
    {
      cursorwin = w;
    }
    paint_cursor();
  }
}

void __FASTCALL__ twShowWinOnTop(TWindow* w)
{
  twinSendMessage(w,WM_TOPSHOW,0L,NULL);
  if((w->iflags & IFLG_VISIBLE) == IFLG_VISIBLE) twHideWin(w);
  w->iflags |= IFLG_VISIBLE;
  __unlistwin(w);
  __ATHEAD(w);
  screen2win(w);
  updatescreen(w,True);
  if((w->flags & TWS_CURSORABLE) == TWS_CURSORABLE)
  {
    cursorwin = w;
  }
  paint_cursor();
}

void __FASTCALL__ twShowWinBeneath(TWindow* w,TWindow *prev)
{
  twinSendMessage(w,WM_SHOWBENEATH,0L,prev);
  if((w->iflags & IFLG_VISIBLE) == IFLG_VISIBLE) twHideWin(w);
  w->iflags |= IFLG_VISIBLE;
  __unlistwin(w);
  __ATWIN(w,prev);
  updatewinmem(w);
  updatescreen(w,True);
}

void __FASTCALL__ twHideWin(TWindow *w)
{
  twinSendMessage(w,WM_HIDE,0L,NULL);
  if(cursorwin == w) twSetCursorType(TW_CUR_OFF);
  savedwin2screen(w);
  w->iflags &= ~IFLG_VISIBLE;
}

void __FASTCALL__ twDestroyWin(TWindow *win)
{
  twinSendMessage(win,WM_DESTROY,0L,NULL);
  twHideWin(win);
  if(win->Title) PFREE(win->Title);
  if(win->Footer) PFREE(win->Footer);
  PFREE(win->body.chars);
  PFREE(win->body.oem_pg);
  PFREE(win->body.attrs);
  PFREE(win->saved.chars);
  PFREE(win->saved.oem_pg);
  PFREE(win->saved.attrs);
  if(active == win)
  {
     if(win == head)   active = win->next;
     else              active = __prevwin(win);
  }
  __unlistwin(win);
  if(cursorwin == win) cursorwin = __findcursorablewin();
  paint_cursor();
  PFREE(win);
}

void __FASTCALL__ twGetWinPos(TWindow *win,tAbsCoord *x1,tAbsCoord *y1,tAbsCoord *x2,tAbsCoord *y2)
{
  *x1 = win->X1+1;
  *y1 = win->Y1+1;
  *x2 = win->X2;
  *y2 = win->Y2;
  if((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE)
  {
   (*x2)--;
   (*y2)--;
  }
}

unsigned          __FASTCALL__ twGetWinWidth(TWindow *win)
{
  return win->wwidth;
}

unsigned          __FASTCALL__ twGetWinHeight(TWindow *win)
{
  return win->wheight;
}

unsigned          __FASTCALL__ twGetClientWidth(TWindow *win)
{
  return win->flags & TWS_FRAMEABLE ? win->wwidth-2 : win->wwidth;
}

unsigned          __FASTCALL__ twGetClientHeight(TWindow *win)
{
  return win->flags & TWS_FRAMEABLE ? win->wheight-2 : win->wheight;
}

void __FASTCALL__ twMoveWin(TWindow *win,tAbsCoord dx,tAbsCoord dy)
{
  TWindow *prev;
  tRelCoord x,y;
  int vis;
  vis = (win->iflags & IFLG_VISIBLE) == IFLG_VISIBLE;
  x = twWhereX();
  y = twWhereY();
  prev = __prevwin(win);
  if(vis) twHideWin(win);
  win->X1 += dx;
  win->Y1 += dy;
  win->X2 += dx;
  win->Y2 += dy;
  if(vis) twShowWinBeneath(win,prev);
  twGotoXY(x,y);
}

void __FASTCALL__ twResizeWin(TWindow *win,tAbsCoord width,tAbsCoord height)
{
  TWindow * tmp,*prev;
  tvioBuff newbody;
  size_t ncopy,delta,fillsize;
  size_t from,to,size,i,loop,start,idx;
  tAbsCoord oldw,oldh;
  tRelCoord x,y;
  tBool vis;
  x = twWhereX();
  y = twWhereY();
  vis = (win->iflags & IFLG_VISIBLE) == IFLG_VISIBLE;
  prev = __prevwin(win);
  if(vis) twHideWin(win);
  size = width*height;
  if(!(newbody.chars = PMalloc(size*sizeof(t_vchar)
#ifndef NDEBUG
                               +sizeof(void *)
#endif
    )))
  {
    bye0:
    winerr("Out of memory!");
  }
  if(!(newbody.oem_pg = PMalloc(size*sizeof(t_vchar)
#ifndef NDEBUG
                               +sizeof(void *)
#endif
    )))
  {
   bye1:
    PFREE(newbody.chars);
    goto bye0;
  }
  if(!(newbody.attrs = PMalloc(size*sizeof(ColorAttr)
#ifndef NDEBUG
                               +sizeof(void *)
#endif
    )))
  {
    PFREE(newbody.oem_pg);
    goto bye1;
  }
#ifndef NDEBUG
  *((void **)(newbody.chars + size)) = newbody.chars;
  *((void **)(newbody.oem_pg + size)) = newbody.oem_pg;
  *((void **)(newbody.attrs + size)) = newbody.attrs;
#endif
  oldw = win->wwidth;
  oldh = win->wheight;
  /* --- Compute copy parameters --- */
  to = (win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE ? 1 : 0;
  from = (win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE ? 1 : 0;
  delta = 0;
  if((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE) delta = 2;
  start = (win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE ? 1 : 0;
  loop = min(oldh,height);
  ncopy = min(width,oldw);
  fillsize = width-oldw;
  for(i = start;i < loop;i++)
  {
      memcpy(&newbody.chars[to],&win->body.chars[from],ncopy);
      memcpy(&newbody.oem_pg[to],&win->body.oem_pg[from],ncopy);
      memcpy(&newbody.attrs[to],&win->body.attrs[from],ncopy);
      if(oldw < width)
      {
        idx = to+ncopy-delta;
        memset(&newbody.chars[idx],TWC_DEF_FILLER,fillsize);
        memset(&newbody.oem_pg[idx],0,fillsize);
        memset(&newbody.attrs[idx],win->text.system,fillsize);
      }
      CHECK_WINS(win);
      to += width;
      from += oldw;
  }
  if(oldh < height)
  {
    for(;i < height;i++)
    {
       memset(&newbody.chars[to],TWC_DEF_FILLER,width);
       memset(&newbody.oem_pg[to],0,width);
       memset(&newbody.attrs[to],win->text.system,width);
       to += width;
    }
  }
  PFREE(win->saved.chars);
  PFREE(win->saved.oem_pg);
  PFREE(win->saved.attrs);
  win->wsize = size;
  win->wwidth = width;
  win->wheight = height;
  if(!(win->saved.chars = PMalloc(win->wsize*sizeof(t_vchar)
#ifndef NDEBUG
                               +sizeof(void *)
#endif
    )))  goto bye0;
  if(!(win->saved.oem_pg = PMalloc(win->wsize*sizeof(t_vchar)
#ifndef NDEBUG
                               +sizeof(void *)
#endif
    )))
  {
   bye2:
    PFREE(win->saved.chars);
    goto bye0;
  }
  if(!(win->saved.attrs = PMalloc(win->wsize*sizeof(ColorAttr)
#ifndef NDEBUG
                               +sizeof(void *)
#endif
    )))
  {
    PFREE(win->saved.oem_pg);
    goto bye2;
  }
  PFREE(win->body.chars);
  PFREE(win->body.oem_pg);
  PFREE(win->body.attrs);
#ifndef NDEBUG
  *((void **)(win->saved.chars + win->wsize)) = win->saved.chars;
  *((void **)(win->saved.oem_pg + win->wsize)) = win->saved.oem_pg;
  *((void **)(win->saved.attrs + win->wsize)) = win->saved.attrs;
#endif
  win->body = newbody;
  win->X2 = win->X1 + width;
  win->Y2 = win->Y1 + height;
  CHECK_WINS(win);
  tmp = active;
  active = win;
  paint_internal();
  active = tmp;
  if(vis) twShowWinBeneath(win,prev);
  twGotoXY(x,y);
}

void __FASTCALL__ twCentredWin(TWindow * win,const TWindow *parent)
{
  tAbsCoord ww,wh,pww,pwh;
  int vis = (win->iflags & IFLG_VISIBLE) == IFLG_VISIBLE;
  if(vis) twHideWin(win);
  ww = win->wwidth;
  wh = win->wheight;
  if(!parent)
  {
    pww = tvioWidth;
    pwh = tvioHeight;
  }
  else
  {
    pww = parent->wwidth;
    pwh = parent->wheight;
  }
  win->X1 = ( pww - ww )>>1;
  win->X2 = ( pww + ww )>>1;
  win->Y1 = ( pwh - wh )>>1;
  win->Y2 = ( pwh + wh )>>1;
  if(vis) twShowWin(win);
}

void __FASTCALL__ twClearWinEx( unsigned char filler )
{
  size_t to,size,i,loop,start,delta,idx,fillsize;
  tRelCoord cx,cy;
  char oempg = 0;
  cx = active->cur_x;
  cy = active->cur_y;
  to = (active->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE ? 1 : 0;
  size = active->wwidth;
  delta = 0;
  if((active->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE) delta = 2;
  start = (active->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE ? 1 : 0;
  loop = active->wheight;
  if((active->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE) loop--;
  fillsize = size-delta;
  if((active->flags & TWS_NLSOEM) == TWS_NLSOEM)
  {
    oempg = filler;
    __nls_OemToOsdep(&filler,1);
  }
  for(i = start;i < loop;i++)
  {
      idx = to+i*size;
      memset(&active->body.chars[idx],filler,fillsize);
      memset(&active->body.oem_pg[idx],((active->flags & TWS_NLSOEM) == TWS_NLSOEM ? NLS_IS_OEMPG(oempg) ? oempg : 0 : 0),fillsize);
      memset(&active->body.attrs[idx],active->text.system,fillsize);
      CHECK_WINS(active);
      updatescreenpiece(active,0,active->wwidth,i+1);
  }
  active->cur_x = cx;
  active->cur_y = cy;
}

void __FASTCALL__ twClearWin( void )
{
  twClearWinEx(TWC_DEF_FILLER);
}

void __FASTCALL__ twClrEOLEx(unsigned char filler )
{
  size_t size,idx;
  char oempg = 0;
  size = active->wwidth - active->cur_x;
  if((active->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE) size--;
  idx = active->cur_x + active->cur_y*active->wwidth;
  if((active->flags & TWS_NLSOEM) == TWS_NLSOEM)
  {
    oempg = filler;
    __nls_OemToOsdep(&filler,1);
  }
  memset(&active->body.chars[idx],filler,size);
  memset(&active->body.oem_pg[idx],((active->flags & TWS_NLSOEM) == TWS_NLSOEM ? NLS_IS_OEMPG(oempg) ? oempg : 0 : 0),size);
  memset(&active->body.attrs[idx],active->text.system,size);
  CHECK_WINS(active);
  updatescreenpiece(active,active->cur_x,active->wwidth,active->cur_y+1);
}

void __FASTCALL__ twClrEOL( void )
{
  twClrEOLEx(TWC_DEF_FILLER);
}

TWindow * __FASTCALL__ twUseWin( TWindow * win)
{
  TWindow *ret;
  ret = active;
  active = win;
  if((active->flags & TWS_CURSORABLE) == TWS_CURSORABLE)
  {
     cursorwin = win;
  }
  paint_cursor();
  return ret;
}

TWindow * __FASTCALL__ twUsedWin( void )
{
  return active;
}

tRelCoord __FASTCALL__ twWhereX( void )
{
  return (active->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE ? active->cur_x : active->cur_x+1;
}

tRelCoord __FASTCALL__ twWhereY( void )
{
  return (active->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE ? active->cur_y : active->cur_y+1;
}

static void __NEAR__ __FASTCALL__ wputc_oem(char ch,char oempg,char color,tBool update)
{
  unsigned idx;
  idx = active->cur_x + active->cur_y*active->wwidth;
  active->body.chars[idx] = ch;
  active->body.oem_pg[idx] = oempg;
  active->body.attrs[idx] = color;
  CHECK_WINS(active);
  if(update) updatescreenchar(active,active->cur_x+1,active->cur_y+1,NULL);
}

void __FASTCALL__ twPutChar(char ch)
{
  tRelCoord cx,cy;
  char as_oem = 0;
  cx = active->cur_x;
  cy = active->cur_y;
  if(IS_VALID_XY(active,cx,cy))
  {
    if((active->flags & TWS_NLSOEM) == TWS_NLSOEM)
    {
       as_oem = ch;
       __nls_OemToOsdep((unsigned char *)&ch,1);
       if(!NLS_IS_OEMPG(as_oem)) as_oem = 0;
       wputc_oem(ch,as_oem,active->text.system,True);
    }
    else wputc(ch,active->text.system,True);
    active->cur_x++;
  }
}

char __FASTCALL__ twGetChar( void )
{
  unsigned idx;
  tRelCoord cx,cy;
  cx = active->X1 + active->cur_x;
  cy = active->Y1 + active->cur_y;
  idx = cx + cy*active->wwidth;
  return active->body.chars[idx];
}

int __FASTCALL__ twPutS(const char *str)
{
  char *__nls = NULL,* __nls_ptr;
  const char *__oem_ptr;
  int freq = 0;
  unsigned vidx;
  tAbsCoord usx;
  tRelCoord cx,cy;
  char ch,as_oem;
  if((active->flags & TWS_NLSOEM) == TWS_NLSOEM)
  {
     unsigned len;
     len = strlen(str);
     __nls = PMalloc(len+1);
     if(__nls)
     {
       strcpy(__nls,str);
       __nls_OemToOsdep((unsigned char *)__nls,len);
     }
     else __nls = str;
  }
  else __nls = str;
  __nls_ptr = __nls;
  __oem_ptr = str;
  vidx = active->cur_x + active->cur_y*active->wwidth;
  usx = active->cur_x;
  while((ch=*__nls++)!=0)
  {
    as_oem=*__oem_ptr++;
    if(ch == '\n')
    {
        updatescreenpiece(active,usx,active->cur_x,active->cur_y+1);
        set_xy(1,active->cur_y+1);
        usx = active->cur_x;
        vidx = active->cur_x + active->cur_y*active->wwidth;
        continue;
    }
    if(ch == '\r')
    {
        set_xy(1,active->cur_y);
        usx = active->cur_x;
        vidx = active->cur_x + active->cur_y*active->wwidth;
        continue;
    }
    cx = active->cur_x;
    cy = active->cur_y;
    if(IS_VALID_XY(active,cx,cy))
    {
        active->body.chars[vidx] = ch;
        if((active->flags & TWS_NLSOEM) == TWS_NLSOEM && NLS_IS_OEMPG(as_oem))
             active->body.oem_pg[vidx] = as_oem;
	else active->body.oem_pg[vidx] = 0;
        active->body.attrs[vidx++] = active->text.system;
        active->cur_x++;
        freq++;
    }
  }
  updatescreenpiece(active,usx,active->cur_x,active->cur_y+1);
  paint_cursor();
  if(__nls_ptr != str) PFREE(__nls_ptr);
  return freq;
}

int __FASTCALL__ twPrintF(const char *fmt,...)
{
  char *buff;
  int ret;
  va_list args;
  buff = PMalloc(tvioWidth*tvioHeight); /* Danger - format can be great that
                                         allocated buffer, but it meaningless */
  ret = 0;
  if(buff)
  {
    va_start(args,fmt);
    vsprintf(buff,fmt,args);
    va_end(args);
    ret = twPutS(buff);
    PFREE(buff);
  }
  return ret;
}

int __FASTCALL__ twDirectWrite(tRelCoord x, tRelCoord y,const void *str,unsigned len)
{
  unsigned i,rlen,ioff;
  const char *__nls = NULL,*__oem = NULL;
  char nlsBuff[__TVIO_MAXSCREENWIDTH];
  char oemBuff[__TVIO_MAXSCREENWIDTH];
  if(!((active->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE))
  {
    x--;  y--;
  }
  if(!IS_VALID_XY(active,x,y)) return 0;
  rlen = active->wwidth;
  if((active->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE) rlen-=1;
  rlen -= x;
  rlen = min(rlen,len);
  if((active->flags & TWS_NLSOEM) == TWS_NLSOEM)
  {
     memcpy(nlsBuff,str,rlen);
     memcpy(oemBuff,str,rlen);
     for(i = 0;i < rlen;i++) if(!NLS_IS_OEMPG(oemBuff[i])) oemBuff[i] = 0;
     __nls_OemToOsdep((unsigned char *)nlsBuff,rlen);
     __nls = nlsBuff;
     __oem = oemBuff;
  }
  else __nls = str;
  ioff = x+y*active->wwidth;
  memcpy(&active->body.chars[ioff],__nls,rlen);
  if(__oem) memcpy(&active->body.oem_pg[ioff],__oem,rlen);
  else      memset(&active->body.oem_pg[ioff],0,rlen);
  memset(&active->body.attrs[ioff],active->text.system,rlen);
  CHECK_WINS(active);
  updatescreenpiece(active,x,x+rlen,y+1);
  return rlen;
}

void __FASTCALL__ twWriteBuffer(TWindow *win,tRelCoord x,tRelCoord y,const tvioBuff *buff,unsigned len)
{
  const void *pbuff;
  unsigned rlen,i,loop_len;
  char nbuff[__TVIO_MAXSCREENWIDTH];
  rlen = (unsigned)x+len > win->wwidth ? win->wwidth-(unsigned)x+1 : len;
  if((unsigned)y <= win->wheight)
  {
    pbuff = buff->attrs;
    if((twin_flags & TWIF_FORCEMONO) && tvioNumColors >= 16)
    {
      memcpy(nbuff,buff->attrs,rlen);
      loop_len = rlen;
      for(i = 1;i < loop_len;i++)
      {
        Color fore,back;
        fore = FORE_COLOR(nbuff[i]);
        back = BACK_COLOR(nbuff[i]);
        adjustColor(&fore,&back);
        nbuff[i] = LOGFB_TO_PHYS(fore,back);
      }
      pbuff = nbuff;
    }
    i = (x-1)+(y-1)*win->wwidth;
    memcpy(&win->body.chars[i],buff->chars,rlen);
    memcpy(&win->body.oem_pg[i],buff->oem_pg,rlen);
    memcpy(&win->body.attrs[i],pbuff,rlen);
    CHECK_WINS(win);
    updatescreenpiece(win,x-1,x-1+rlen,y);
    paint_cursor();
  }
}

void __FASTCALL__ twReadBuffer(TWindow *win,tRelCoord x,tRelCoord y,tvioBuff *buff,unsigned len)
{
  unsigned rlen,idx;
  rlen = (unsigned)x+len > win->wwidth ? win->wwidth-(unsigned)x+1 : len;
  if((unsigned)y <= win->wheight)
  {
    idx = (x-1)+(y-1)*win->wwidth;
    memcpy(buff->chars,&win->body.chars[idx],rlen);
    memcpy(buff->oem_pg,&win->body.oem_pg[idx],rlen);
    memcpy(buff->attrs,&win->body.attrs[idx],rlen);
    CHECK_WINS(win);
    paint_cursor();
  }
}

void __FASTCALL__ twFreezeWin(TWindow *win)
{
  win->iflags &= ~IFLG_ENABLED;
}

void __FASTCALL__ twRefreshLine(TWindow *win,tRelCoord y)
{
  win->iflags |= IFLG_ENABLED;
  if((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE) y++;
  updatescreenpiece(win,0,win->wwidth,y);
  paint_cursor();
}

void __FASTCALL__ twRefreshPiece(TWindow *win,tRelCoord stx,tRelCoord endx,tRelCoord y)
{
  win->iflags |= IFLG_ENABLED;
  if((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE) { stx++; endx++; y++; }
  updatescreenpiece(win,stx-1,endx-1,y);
  paint_cursor();
}

void __FASTCALL__ twRefreshWin(TWindow *win)
{
  win->iflags |= IFLG_ENABLED;
  updatescreen(win,(win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE ? False : True);
  paint_cursor();
}

void __FASTCALL__ twRefreshFullWin(TWindow *win)
{
  win->iflags |= IFLG_ENABLED;
  updatescreen(win,True);
  paint_cursor();
}

void * __FASTCALL__ twGetUsrData(TWindow *win) { return win->usrData; }

void * __FASTCALL__ twSetUsrData(TWindow *win,void *data)
{
  void *ret;
  ret = win->usrData;
  win->usrData = data;
  return ret;
}

void __FASTCALL__ twRemapColor(Color color,unsigned char value)
{
  color_map[color & 0x0F] = value & 0x0F;
}

unsigned char __FASTCALL__ twGetMappedColor(Color color)
{
  return color_map[color & 0x0F];
}

void __FASTCALL__ twScrollWinUp(TWindow *win,tRelCoord ypos, unsigned npos)
{
  size_t i;
  tRelCoord wwidth;
  tvioBuff accel;
  t_vchar chars[__TVIO_MAXSCREENWIDTH];
  t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
  ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
  if(!npos || !IS_VALID_Y(win,ypos)) return;
  accel.chars = chars;
  accel.oem_pg = oem_pg;
  accel.attrs = attrs;
  wwidth = win->wwidth;
  if((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE) wwidth-=2;
  for(i = npos-1;i < ypos;i++)
  {
    twReadBuffer(win,1,i+2,&accel,wwidth);
    twWriteBuffer(win,1,i+1,&accel,wwidth);
  }
}

void __FASTCALL__ twScrollWinDn(TWindow *win,tRelCoord ypos, unsigned npos)
{
  size_t i,lim;
  tRelCoord wwidth,wheight;
  tvioBuff accel;
  t_vchar chars[__TVIO_MAXSCREENWIDTH];
  t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
  ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
  if(!npos || !IS_VALID_Y(win,ypos)) return;
  accel.chars = chars;
  accel.oem_pg = oem_pg;
  accel.attrs = attrs;
  lim = ypos > npos ? ypos - npos + 1 : 1;
  wwidth = win->wwidth;
  wheight = win->wheight;
  if((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE)
  {
    wwidth-=2;
    wheight-=2;
  }
  for(i = wheight;i > lim;i--)
  {
    twReadBuffer(win,1,i-npos,&accel,wwidth);
    twWriteBuffer(win,1,i,&accel,wwidth);
  }
}

void __FASTCALL__ twScrollWinLf(TWindow *win,tRelCoord xpos, unsigned npos)
{
 /** @todo Not tested!!! */
  size_t i,j,lim;
  tRelCoord wheight;
  tvioBuff accel;
  t_vchar chars[__TVIO_MAXSCREENWIDTH];
  t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
  ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
  if(!npos || !IS_VALID_X(win,xpos)) return;
  accel.chars = chars;
  accel.oem_pg = oem_pg;
  accel.attrs = attrs;
  lim = xpos > npos ? xpos - npos : 0;
  wheight = win->wheight;
  if((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE)
  {
    wheight-=2;
  }
  for(i = lim;i < xpos;i++)
  {
    /** @todo Optimize this block */
    for(j = 0;j < wheight;j++)
    {
      twReadBuffer(win,i+2,j,&accel,1);
      twWriteBuffer(win,i+1,j,&accel,1);
    }
  }
}

void __FASTCALL__ twScrollWinRt(TWindow *win,tRelCoord xpos, unsigned npos)
{
 /** @todo Not tested!!! */
  size_t i,j,lim;
  tRelCoord wwidth,wheight;
  tvioBuff accel;
  t_vchar chars[__TVIO_MAXSCREENWIDTH];
  t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
  ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
  if(!npos || !IS_VALID_X(win,xpos)) return;
  accel.chars = chars;
  accel.oem_pg = oem_pg;
  accel.attrs = attrs;
  lim = xpos > npos ? xpos - npos + 1 : 1;
  wwidth = win->wwidth;
  wheight = win->wheight;
  if((win->flags & TWS_FRAMEABLE) == TWS_FRAMEABLE)
  {
    wwidth-=2;
    wheight-=2;
  }
  for(i = wwidth;i > lim;i--)
  {
    /** @todo Optimize this block */
    for(j = 0;j < wheight;j++)
    {
      twReadBuffer(win,i-npos,j,&accel,1);
      twWriteBuffer(win,i,j,&accel,1);
    }
  }
}

long __FASTCALL__ twinSendMessage(TWindow *win,unsigned event,unsigned long event_param, void *event_data)
{
    if(win->method) return ((twClassFunc)(win->method))(win,event,event_param,event_data);
    return 0L;
}
