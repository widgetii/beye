/**
 * @namespace   biew
 * @file        biewhelp.c
 * @brief       This file contains LZSS-based help system functions.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "colorset.h"
#include "bmfile.h"
#include "bconsole.h"
#include "biewhelp.h"
#include "search.h"
#include "setup.h"
#include "tstrings.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"
#include "biewlib/pmalloc.h"

#include "tools/lzss/lzssutil.c"

#define TEXT_TAB 8

extern void drawHelpListPrompt( void );

#define HPROP_BOLD                  0x01
#define HPROP_ITALIC                0x02
#define HPROP_UNDERLINE             0x04
#define HPROP_STRIKETHROUGH         0x08
#define HPROP_REVERSE               0x10
#define HPROP_LINK                  0x20

unsigned __FASTCALL__ hlpFillBuffer(tvioBuff * dest,unsigned int alen,const char * str,unsigned int len,unsigned int shift,unsigned *n_tabs,tBool is_hl)
{
  t_vchar ch;
  ColorAttr defcol;
  unsigned char text_prop;
  int size,j;
  size_t i,k,freq;
  if(n_tabs) *n_tabs = 0;
  text_prop = 0;
  defcol = is_hl ? menu_cset.highlight : help_cset.main;
  for(i = 0,freq = 0,k = 0;i < len;i++,freq++)
  {
    ch = str[i];
    if(iscntrl(ch))
    {
      switch(ch)
      {
         case HLPC_BOLD_ON: text_prop |= HPROP_BOLD; break;
         case HLPC_ITALIC_ON: text_prop |= HPROP_ITALIC; break;
         case HLPC_UNDERLINE_ON: text_prop |= HPROP_UNDERLINE; break;
         case HLPC_STRIKETHROUGH_ON: text_prop |= HPROP_STRIKETHROUGH; break;
         case HLPC_REVERSE_ON: text_prop |= HPROP_REVERSE; break;
         case HLPC_LINK_ON: text_prop |= HPROP_LINK; break;
         case HLPC_BOLD_OFF: text_prop &= ~HPROP_BOLD; break;
         case HLPC_ITALIC_OFF: text_prop &= ~HPROP_ITALIC; break;
         case HLPC_UNDERLINE_OFF: text_prop &= ~HPROP_UNDERLINE; break;
         case HLPC_STRIKETHROUGH_OFF: text_prop &= ~HPROP_STRIKETHROUGH; break;
         case HLPC_REVERSE_OFF: text_prop &= ~HPROP_REVERSE; break;
         case HLPC_LINK_OFF: text_prop &= ~HPROP_LINK; break;
         case '\t': /** HT   horizontal tab*/
               {
                  size = TEXT_TAB - (freq%TEXT_TAB);
                  for(j = 0;j < size;j++,freq++)
                  {
                    if(k < alen && freq >= shift)
                    {
                      if(dest)
                      {
                        dest->chars[k] = TWC_DEF_FILLER;
                        dest->attrs[k] = defcol;
                      }
                      k++;
                    }
                  }
                  if(n_tabs) (*n_tabs)++;
               }
               freq--;
               break;
         case '\n':
         case '\r':
                  goto End;
      }
      if(text_prop & HPROP_BOLD)  defcol = help_cset.bold;
      else
      if(text_prop & HPROP_ITALIC)  defcol = help_cset.italic;
      else
      if(text_prop & HPROP_UNDERLINE)  defcol = help_cset.underline;
      else
      if(text_prop & HPROP_STRIKETHROUGH)  defcol = help_cset.strikethrough;
      else
      if(text_prop & HPROP_REVERSE)  defcol = help_cset.reverse;
      else
      if(text_prop & HPROP_LINK)  defcol = help_cset.link;
      else                        defcol = help_cset.main;
    }
    else
    {
      if(k < alen && freq >= shift)
      {
        if(dest)
        {
          dest->chars[k] = ch;
          dest->attrs[k] = defcol;
        }
        k++;
      }
    }
  }
  End:
  if(dest) __nls_PrepareOEMForTVio(dest,k);
  return k;
}

void __FASTCALL__ hlpPaintLine(TWindow *win,unsigned i,const char *name,tBool is_hl)
{
  tvioBuff it;
  unsigned rlen;
  t_vchar chars[__TVIO_MAXSCREENWIDTH];
  t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
  ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
  it.chars = chars;
  it.oem_pg = oem_pg;
  it.attrs = attrs;
  rlen = strlen(name);
  rlen = hlpFillBuffer(&it,__TVIO_MAXSCREENWIDTH,name,rlen,0,NULL,is_hl);
  twWriteBuffer(win,4,i+2,&it,rlen);
  twGotoXY(3+rlen,i+1);
  twClrEOL();
}

static void __NEAR__ __FASTCALL__ Paint(TWindow *win,char * * names,unsigned nlist,unsigned start,unsigned height,unsigned width)
{
 unsigned i, pos = 0;
 twUseWin(win);
 twFreezeWin(win);
 width -= 3;
 if (height>2 && height<nlist)
     pos = 1 + start*(height-2)/nlist;
 twSetColorAttr(help_cset.main);
 for(i = 0;i < height;i++)
 {
   twGotoXY(1,i + 1);
   if (i == 0)
       twPutChar(start ? TWC_UP_ARROW : TWC_DEF_FILLER);
   else if(i == height-1)
       twPutChar(start + height < nlist ? TWC_DN_ARROW : TWC_DEF_FILLER);
   else if (i == pos)
       twPutChar(TWC_THUMB);
   else twPutChar(TWC_DEF_FILLER);
  twGotoXY(2,i + 1);
   twPutChar(TWC_SV);
   hlpPaintLine(win,i,names[i + start],0);
 }
 twRefreshWin(win);
}

typedef char *lpstr;

static int __NEAR__ __FASTCALL__ __hlpListBox(char * * names,unsigned nlist,const char * title)
{
 TWindow * wlist;
 unsigned i,j,height,mwidth = strlen(title);
 int ret,start,ostart,cursor,ocursor,scursor;
 scursor = -1;
 for(i = 0;i < nlist;i++)
 {
    j = hlpFillBuffer(NULL,UINT_MAX,names[i],strlen(names[i]),0,NULL,0);
    if(j > mwidth) mwidth = j;
 }
 mwidth += 4;
 if(mwidth > (unsigned)(tvioWidth-2)) mwidth = tvioWidth-2;
 height = nlist < (unsigned)(tvioHeight - 4) ? nlist : tvioHeight - 4;
 wlist = CrtHlpWndnls(title,mwidth-1,height);
 ostart = start = cursor = ocursor = 0;
 Paint(wlist,names,nlist,start,height,mwidth);
 for(;;)
 {
   unsigned ch;
   ch = GetEvent(drawHelpListPrompt,NULL,wlist);
   if(ch == KE_ESCAPE || ch == KE_F(10)) { ret = -1; break; }
   if(ch == KE_ENTER)                    { ret = start + cursor; break; }
   if(ch!=KE_F(7) && ch!=KE_SHIFT_F(7))  scursor = -1;
   switch(ch)
   {
     case KE_F(7): /** perform binary search in help */
     case KE_SHIFT_F(7):
             {
               static char searchtxt[21] = "";
               static unsigned char searchlen = 0;
               static unsigned sflg = SF_NONE;

               if (!(ch==KE_SHIFT_F(7) && searchlen) &&
                   !SearchDialog(SD_SIMPLE,searchtxt,&searchlen,&sflg))
                   break;

               {
                  int direct,ii;
                  tBool found;
                  int endsearch,startsearch,cache[UCHAR_MAX];
                  searchtxt[searchlen] = 0;
                  endsearch = sflg & SF_REVERSE ? -1 : nlist;
                  direct = sflg & SF_REVERSE ? -1 : 1;
                  startsearch =    scursor != -1 ?
                                   scursor :
                                   start;
                  if(startsearch > (int)(nlist-1)) startsearch = nlist-1;
                  if(startsearch < 0) startsearch = 0;
                  if(scursor != -1)
                  {
                    sflg & SF_REVERSE ? startsearch-- : startsearch++;
                  }
                  found = False;
                  fillBoyerMooreCache(cache,searchtxt,searchlen, sflg & SF_CASESENS);
                  for(ii = startsearch;ii != endsearch;ii+=direct)
                  {
                     if(_lb_searchtext(names[ii],searchtxt,searchlen,cache,sflg))
                     {
                        start = scursor = ii;
                        if((unsigned)start > nlist - height) start = nlist - height;
                        ostart = start - 1;
                        found = True;
                        break;
                     }
                  }
                  if(!found) scursor = -1;
                  if(scursor == -1) ErrMessageBox(STR_NOT_FOUND,SEARCH_MSG);
               }
             }
             break;
     case KE_DOWNARROW : start ++; break;
     case KE_UPARROW   : start--; break;
     case KE_PGDN   : start += height; break;
     case KE_PGUP   : start -= height; break;
     case KE_CTL_PGDN  : start = nlist - height; cursor = height; break;
     case KE_CTL_PGUP  : start = cursor = 0; break;
     default : break;
   }
   if(start < 0) start = 0;
   if((unsigned)start > nlist - height) start = nlist - height;
   if(start != ostart)
   {
     ostart = start;
     Paint(wlist,names,nlist,start,height,mwidth);
   }
   if(scursor >= 0)
   {
     twSetColorAttr(menu_cset.highlight);
     if(scursor >= start && (unsigned)scursor < start + height) hlpPaintLine(wlist,scursor - start,names[scursor],True);
   }
 }
 CloseWnd(wlist);
 return ret;
}


static BGLOBAL bHelp = &bNull;
static BIEW_HELP_ITEM bhi;

tBool __FASTCALL__ hlpOpen( tBool interact )
{
  char *help_name;
  char hlp_id[sizeof(BIEW_HELP_VER)];
  if(bHelp != &bNull) return False; /*means: help file is already opened */
  help_name = biewGetHelpName();
  bHelp = biewOpenRO(help_name,BBIO_SMALL_CACHE_SIZE);
  if(bHelp == &bNull)
  {
    if(interact) errnoMessageBox("Can't open help file",NULL,errno);
    return False;
  }
  bioSetOptimization(bHelp,BIO_OPT_RANDOM);
  bioSeek(bHelp,0L,SEEK_SET);
  bioReadBuffer(bHelp,hlp_id,sizeof(hlp_id));
  if(memcmp(hlp_id,BIEW_HELP_VER,sizeof(BIEW_HELP_VER)) != 0)
  {
    if(interact)
    {
      ErrMessageBox("Incorrect help version",NULL);
    }
    bioClose(bHelp);
    bHelp = &bNull;
    return False;
  }
  return True;
}

void __FASTCALL__ hlpClose( void )
{
  if(bHelp != &bNull)
  {
    bioClose(bHelp);
    bHelp = &bNull;
  }
}

static tBool __FASTCALL__ find_item(unsigned long item_id)
{
  unsigned long i,nsize,lval;
  char sout[HLP_SLONG_LEN];
  bioSeek(bHelp,HLP_VER_LEN,SEEK_SET);
  bioReadBuffer(bHelp,sout,sizeof(sout));
  nsize = strtoul(sout,NULL,16);
  for(i = 0;i < nsize;i++)
  {
    bioReadBuffer(bHelp,&bhi,sizeof(BIEW_HELP_ITEM));
    lval = strtoul(bhi.item_id,NULL,16);
    if(lval == item_id) return True;
  }
  return False;
}

unsigned long   __FASTCALL__ hlpGetItemSize(unsigned long item_id)
{
  unsigned long ret = 0;
  if(find_item(item_id)) ret = strtoul(bhi.item_decomp_size,NULL,16);
  else                   ErrMessageBox("Find: Item not found",NULL);
  return ret;
}

tBool   __FASTCALL__ hlpLoadItem(unsigned long item_id, void __HUGE__* buffer)
{
  unsigned long hlp_off,hlp_size;
  tBool ret = False;
  if(bHelp != &bNull)
  {
    if(find_item(item_id))
    {
      hlp_off = strtoul(bhi.item_off,NULL,16);
      hlp_size = strtoul(bhi.item_length,NULL,16);
      if(!Decode(bHelp,buffer,hlp_off,hlp_size))
      {
        MemOutBox("Help uncompression");
      }
      else ret = True;
    }
    else ErrMessageBox("Load: Item not found",NULL);
  }
  return ret;
}

char **   __FASTCALL__ hlpPointStrings(char __HUGE__ *data,unsigned long data_size,unsigned long *nstr)
{
  char **str_ptr,**new_ptr;
  unsigned long i;
  char ch,ch1;
  *nstr = 0;
  if((str_ptr = PMalloc(sizeof(char *))) != NULL)
  {
     for(i = 0;i < data_size;i++)
     {
       ch = data[i];
       if(ch == '\n' || ch == '\r')
       {
         data[i] = 0;
         if(!(new_ptr = PRealloc(str_ptr,((unsigned)(*nstr)+1)*sizeof(char *)))) goto mem_off;
         str_ptr = new_ptr;
         ch1 = data[i+1];
         if((ch1 == '\n' || ch1 == '\r') && ch != ch1) ++i;
         str_ptr[(*nstr)++] = &data[i+1];
       }
       if(*nstr > UINT_MAX-2) { mem_off: PFree(str_ptr); return NULL; }
     }
  }
  return str_ptr;
}

void __FASTCALL__ hlpDisplay( unsigned long item_id )
{
  char **str_ptr = 0;
  char __HUGE__ *data = 0;
  char *title;
  unsigned long data_size,nstr;
  if(!hlpOpen(True)) return;
  data_size = hlpGetItemSize(item_id);
  if(!data_size) goto hlp_bye;
  data = PHMalloc(data_size+1);
  if(!data)
  {
    mem_off:
    MemOutBox("Loading help");
    if(data) PHFREE(data);
    goto hlp_bye;
  }
  data[data_size] = 0;
  if(hlpLoadItem(item_id,data))
  {
    if(!(str_ptr = hlpPointStrings(data,data_size,&nstr))) goto mem_off;
    title = data;
    __hlpListBox(str_ptr,(unsigned)nstr,title);
    PFREE(str_ptr);
  }
  PHFREE(data);
  hlp_bye:
  hlpClose();
}
