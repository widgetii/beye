/**
 * @namespace   biew
 * @file        events.c
 * @brief       This file contains console event handler of BIEW project.
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
#include <limits.h>
#include <string.h>

#include "bconsole.h"
#include "biewlib/kbd_code.h"
#include "biewlib/biewlib.h"

static int KB_Buff[64];
static size_t KB_freq = 0;

tBool  __FASTCALL__ IsKbdTerminate( void )
{
  return __OsGetCBreak( );
}

void __FASTCALL__ CleanKbdTermSig( void )
{
  __OsSetCBreak(False);
}

static tAbsCoord mx,my;

static int __NEAR__ __FASTCALL__ getPromptKey(int flag)
{
 int ret;
 if(mx <= 7) ret = KE_F(1);
 else
   if(mx <= 15) ret = KE_F(2);
   else
     if(mx <= 23) ret = KE_F(3);
     else
       if(mx <= 31) ret = KE_F(4);
       else
         if(mx <= 39) ret = KE_F(5);
         else
           if(mx <= 47) ret = KE_F(6);
           else
             if(mx <= 55) ret = KE_F(7);
             else
               if(mx <= 63) ret = KE_F(8);
               else
                 if(mx <= 71) ret = KE_F(9);
                 else         ret = KE_F(10);
 if(flag & KS_SHIFT) ret += KE_SHIFT_F(1) - KE_F(1);
 else
   if(flag & KS_ALT) ret += KE_ALT_F(1) - KE_F(1);
   else
     if(flag & KS_CTRL) ret += KE_CTL_F(1) - KE_F(1);
 return ret;
}

extern unsigned long biew_kbdFlags;

static int __NEAR__ __FASTCALL__ __GetEvent( void (*prompt)(void) ,TWindow *win)
{
 static unsigned char oFlag = UCHAR_MAX;
 static void (* oprompt)(void) = 0;
 unsigned char Flag;
 int key;
 /** Unconditional repaint prompt */
 Flag = __kbdGetShiftsKey();
 if(Flag != oFlag || oprompt !=prompt)
 {
    oFlag = Flag; oprompt = prompt;
    if(prompt) (*prompt)();
 }
 while(1)
 {
     key = __kbdGetKey(biew_kbdFlags);
     switch( key )
     {
       case KE_SHIFTKEYS:
             Flag = __kbdGetShiftsKey();
             if(Flag != oFlag || oprompt !=prompt)
             {
               oFlag = Flag; oprompt = prompt;
               if(prompt) (*prompt)();
             }
             break;
       case KE_MOUSE:
             __MsGetPos(&mx,&my);
             if((__MsGetBtns() & MS_LEFTPRESS) == MS_LEFTPRESS)
             {
                  if(my == (unsigned)(tvioHeight - 1))
                  {
                    while(1)
                    {
                      if((__MsGetBtns() & MS_LEFTPRESS) != MS_LEFTPRESS) break;
                      __MsGetPos(&mx,&my);
                    }
                    if(my != (unsigned)(tvioHeight - 1)) continue;
                    return getPromptKey(__kbdGetShiftsKey());
                  }
                  else
                  {
                    tAbsCoord X1,Y1,X2,Y2;
                    unsigned wdh,hght;
                    if(win)
                    {
                      twGetWinPos(win,&X1,&Y1,&X2,&Y2);
                      X1--; Y1--; X2--; Y2--;
                    }
                    else
                    {
                      X1 = 0; X2 = twGetClientWidth(MainWnd); Y1 = 1; Y2 = twGetClientHeight(MainWnd) - 1;
                    }
                    wdh = X2 - X1;
                    hght = Y2 - Y1;
                    if(win && (mx < X1 || mx > X2 || my < Y1 || my > Y2))
                    {
                       while(1)
                       {
                          if((__MsGetBtns() & MS_LEFTPRESS) != MS_LEFTPRESS) break;
                       }
                       return KE_ESCAPE;
                    }
                    if(my <= Y2 && my >= Y1 + 3*hght/4) return KE_DOWNARROW;
                    else
                      if(my >= Y1 && my <= Y1 + hght/4) return KE_UPARROW;
                      else
                        if(mx <= X2 && mx >= X1 + 3*wdh/4) return KE_RIGHTARROW;
                        else
                          if(mx >= X1 && mx <= X1 + wdh/4) return KE_LEFTARROW;
                  }
              }
        default: return key;
     }
 }
 return key;
}

static void __NEAR__ __FASTCALL__ __GetEventQue(void (*prompt)(void), TWindow *win)
{
  int key;
  key = __GetEvent(prompt,win);
  if(KB_freq < sizeof(KB_Buff)/sizeof(int)) KB_Buff[KB_freq++] = key;
}

int __FASTCALL__ GetEvent( void (*prompt)(void),int (*alt_action)(void),TWindow * win)
{
  int key;
  while(!KB_freq) __GetEventQue(prompt,win);
  key = KB_Buff[0];
  --KB_freq;
  if(KB_freq) memmove(KB_Buff,&KB_Buff[1],KB_freq-1);
  if(key==KE_TAB && alt_action) key=(*alt_action)();
  return key;
}

void __FASTCALL__ PostEvent(int code)
{
  if(KB_freq < sizeof(KB_Buff)/sizeof(int)) KB_Buff[KB_freq++] = code;
}
