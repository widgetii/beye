/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/linux/misc.c
 * @brief       Misc. functions for linux-i386 (optional!!!)
 * @version     -
 * @author      Nick Kurshev
 * @date        2003
**/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

extern char rawkb_buf[];
extern int  rawkb_escape;
extern unsigned rawkb_len;
extern unsigned rawkb_mode;
extern int on_console;

int __FASTCALL__ __inputRawInfo(char *head, char *text)
{
    unsigned i;
    char appends[10];
    strcpy(head,"Name  Value");
    if(on_console) strcpy(text,"Raw   ");
    else 	   strcpy(text,"VT100 ");
    rawkb_escape=0;
    rawkb_len=0;
    rawkb_mode=1;
    while(rawkb_mode) usleep(0);
    for(i=0;i<rawkb_len;i++)
    {
	if(isprint(rawkb_buf[i])) { appends[0]=rawkb_buf[i]; appends[1]=0; }
	else sprintf(appends,"\\0%o ",rawkb_buf[i]);
	strcat(text,appends);
    }
    return rawkb_escape?0:1;
}
