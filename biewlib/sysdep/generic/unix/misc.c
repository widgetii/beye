/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/generic/unix/misc.c
 * @brief       Misc. functions for unix (optional!!!)
 * @version     -
 * @author      Nickols_K
 * @date        2003
**/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"

extern char rawkb_buf[];
extern int  rawkb_escape;
extern int  rawkb_method;
extern unsigned rawkb_len;
extern unsigned rawkb_size;
extern unsigned rawkb_mode;
extern char * rawkb_name;
extern void __FASTCALL__ ReadNextEvent(void);

int __FASTCALL__ __inputRawInfo(char *head, char *text)
{
    unsigned i;
    char appends[20];
    strcpy(head,"ModeName Value");
    rawkb_escape=0;
    rawkb_len=0;
    rawkb_mode=1;
    while(rawkb_mode) { usleep(0); if(!rawkb_method) ReadNextEvent(); }
    strcpy(text,rawkb_name);
    for(i=strlen(text);i<9;i++) strcat(text," ");
    for(i=0;i<rawkb_len;i++)
    {
	if(isprint(rawkb_buf[i])&&rawkb_size==1) { appends[0]=rawkb_buf[i]; appends[1]=0; }
	else sprintf(appends,"\\0%o ",(int)(rawkb_size==4?(int)rawkb_buf[i]:rawkb_size==2?(short)rawkb_buf[i]:(char)rawkb_buf[i]));
	strcat(text,appends);
    }
    return rawkb_escape?0:1;
}
