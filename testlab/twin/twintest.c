/**
   Test of text window library.
   Written by Nickols_K 2000
   Use, distribute, and modify this program freely.
   This subset version is hereby donated to the public domain
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "biewlib/twin.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

#define N_WINS 10

TWindow *win[N_WINS];
char **ArgVector;
int    ArgCount;

static volatile unsigned time_trigger = 0;

static void TimerCallBack( void )
{
  time_trigger = ~time_trigger;
}

void do_delay( void )
{
  static int trigger = 0;
  while(trigger != time_trigger);
  trigger = time_trigger;
}

int main( int argc, char *argv[] )
{
  int i,j,r,need_delay;
  if(argc < 2)
  {
    printf("Usage: %s n\nWhere n - use delay",argv[0]);
    return -1;
  }
  need_delay = (int)strtol(argv[1],NULL,10);
  __init_sys();
  twInit(NULL, 1,0);
  if(need_delay) need_delay = __OsSetTimerCallBack(54,TimerCallBack);
  for(i = 0;i < tvioHeight;i++)
  {
    for(j = 0;j < tvioWidth;j++)
    {
      r = (char)(i+j)+0x20;
      if(r < 0x20) r = '.';
      if(r > 0x7F) r = ',';
      printf("%c",(char)r);
    }
  }
  for(i = 0;i < N_WINS;i++)
  {
    char name[80],footer[80];
    unsigned c,f;
    win[i] = twCreateWin(1+i,1+i,50,10,TWS_NLSOEM | TWS_FRAMEABLE | TWS_VISIBLE);
    sprintf(name,"Window #%i",i);
    sprintf(footer,"Footer #%i",i);
    c = i;
    f = i+7;
    if(c > 15) c -= 16;
    if(f > 7) f -= 8;
    twSetColor(c,f);
    twSetFrame(win[i],TW_UP3D_FRAME,c,f);
    c = i+1;
    f = i+8;
    while(c > 15) c -= 16;
    while(f > 7) f -= 8;
    twSetTitle(win[i],name,TW_TMODE_CENTER,c,f);
    c = i+2;
    f = i+9;
    while(c > 15) c -= 16;
    while(f > 7) f -= 8;
    twSetFooter(win[i],footer,TW_TMODE_RIGHT,c,f);
    twClearWin();
  }
  for(i = 0;i < N_WINS;i++)
  {
    twUseWin(win[i]);
    twGotoXY(1,1);
    twPrintF("It's window #%i has created for demo purposes only",i);
    twGotoXY(1,8);
    twPrintF("It's window #%i has created for demo purposes only",i);
    if(need_delay) do_delay();
  }
  j = 0;
  while(1)
  {
    int rnum;
    int ch;
    if(__kbdTestKey(0))
    {
       ch = __kbdGetKey(0);
       if(ch == KE_ESCAPE)
       {
         break;
       }
    }
    rnum = j;
    for(i = 0;i < 20;i++)
    {
      twMoveWin(win[rnum],1,0);
      if(need_delay) do_delay();
    }
    for(i = 0;i < 10;i++)
    {
      twMoveWin(win[rnum],0,1);
      if(need_delay) do_delay();
    }
    twShowWinOnTop(win[rnum]);
    for(i = 0;i < 10;i++)
    {
      twMoveWin(win[rnum],0,-1);
      if(need_delay) do_delay();
    }
    for(i = 0;i < 20;i++)
    {
      twMoveWin(win[rnum],-1,0);
      if(need_delay) do_delay();
    }
    j++;
    if(j > 9) j = 0;
  }
  for(i = 0;i < N_WINS;i++)
  {
    twDestroyWin(win[i]);
    if(need_delay) do_delay();
  }
  if(need_delay) __OsRestoreTimer();
  twDestroy();
  __term_sys();
  return 0;
}
