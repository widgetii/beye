/**
 * @namespace   biew_plugins_I
 * @file        plugins/textmode.c
 * @brief       This file contains implementation of text viewer with different submodes.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#include "plugins/textmode.h"
#include "colorset.h"
#include "bmfile.h"
#include "biewutil.h"
#include "biewhelp.h"
#include "bconsole.h"
#include "search.h"
#include "reg_form.h"
#include "biewlib/kbd_code.h"
#include "biewlib/pmalloc.h"

extern REGISTRY_NLS RussianNLS;

static REGISTRY_NLS *nls_set[] =
{
  &RussianNLS
};

static REGISTRY_NLS *activeNLS = &RussianNLS;
static unsigned      defNLSSet = 0;

#define TEXT_TAB 8

#define MOD_PLAIN    0
#define MOD_BINARY   1

#define MOD_MAXMODE  1

typedef struct tagTSTR
{
  unsigned long st;
  unsigned long end;
}TSTR;

static const char * mod_names[] =
{
   "~Plain text",
   "~All characters 8bit (as is)"
};

static unsigned bin_mode = MOD_PLAIN; /**< points to currently selected mode text mode */
static BGLOBAL txtHandle = &bNull; /**< Own handle of BBIO stream. (For speed). */

#define MAX_STRLEN 1000 /**< defines maximal length of string */
static TSTR *tlines,*ptlines;
static unsigned int maxstrlen = MAX_STRLEN; /**< contains maximal length of string which can be displayed without wrapping */
static tBool wmode; /**< Wrap mode flag */
static unsigned long PrevPageSize,CurrPageSize,PrevStrLen,CurrStrLen;

unsigned strmaxlen = 0; /**< contains size of largest string on currently displayed page. (It for KE_END key) */

void __FASTCALL__ txt_cvt_full(char * str,int size,const unsigned char *tmpl)
{
  int i;
  for(i = 0;i < size;i++) str[i] = __Xlat__(tmpl,(unsigned char)str[i]);
}

void __FASTCALL__ txt_cvt_hi80(char * str,unsigned size,const unsigned char *tmpl)
{
 size_t i;
 unsigned char cc;
 for(i = 0;i < size;i++)
 {
   cc = str[i];
   str[i] = cc >= 0x80 ? __Xlat__(tmpl,cc-0x80) : cc;
 }
}

void __FASTCALL__ txt_cvt_lo80(char * str,unsigned size,const unsigned char *tmpl)
{
 size_t i;
 unsigned char cc;
 for(i = 0;i < size;i++)
 {
   cc = str[i];
   str[i] = cc < 0x80 ? __Xlat__(tmpl,cc) : cc;
 }
}

static unsigned char __NEAR__ __FASTCALL__ nlsReadByte(unsigned long cp)
{
 char nls_buff[256];
 unsigned sym_size;
 sym_size = activeNLS->get_symbol_size();
 bioSeek(txtHandle,cp,SEEK_SET);
 bioReadBuffer(txtHandle,nls_buff,sym_size);
 activeNLS->convert_buffer(nls_buff,sym_size,True);
 return (unsigned char)nls_buff[0];
}

static unsigned long __NEAR__ __FASTCALL__ BackScanCR( unsigned long cp )
{
 unsigned long lval;
 unsigned int freq;
 unsigned cp_symb_len;
 char ch;
 cp_symb_len = activeNLS->get_symbol_size();
 ch = nlsReadByte(cp);
 if((ch == '\n' || ch == '\r') && cp)
 {
     char ch1;
     cp-=cp_symb_len;
     ch1 = nlsReadByte(cp);
     if((ch1 == '\n' || ch1 == '\r') && ch != ch1) cp-=cp_symb_len;
 }
 for(lval = freq = 0;cp;cp-=cp_symb_len)
 {
    ch = nlsReadByte(cp);
    if(ch == '\n' || ch == '\r')
    {
      lval = cp + cp_symb_len;
      break;
    }
    freq++;
    if(freq >= maxstrlen) { lval = cp; break; }
 }
 return lval;
}

static unsigned long __NEAR__ __FASTCALL__ ForwardScanCR( unsigned long cp ,unsigned long flen)
{
 unsigned long lval;
 unsigned int freq = 0;
 unsigned cp_symb_len;
 cp_symb_len = activeNLS->get_symbol_size();

 for(;cp < flen;cp+=cp_symb_len)
 {
    unsigned char ch;
    ch = nlsReadByte(cp);
    if(ch == '\n' || ch == '\r')
    {
      char ch1;
      lval = cp + cp_symb_len;
      ch1 = nlsReadByte(lval);
      if((ch1 == '\n' || ch1 == '\r') && ch != ch1)  lval+=cp_symb_len;
      return lval;
    }
    freq++;
    if(freq > maxstrlen) return cp;
 }
 return flen;
}

static void __NEAR__ __FASTCALL__ FillPrevPage(unsigned long lval)
{
 unsigned cp_symb_len;
 int i;
 cp_symb_len = activeNLS->get_symbol_size();
 for(i = twGetClientHeight(MainWnd) - 1;i >= 0;i--)
 {
    ptlines[i].end = lval;
    if(lval >= cp_symb_len) lval = BackScanCR(lval - cp_symb_len);
    ptlines[i].st = lval;
 }
}

static void __NEAR__ __FASTCALL__ FillCurrPage(unsigned long lval,unsigned long flen)
{
 size_t i;
 tAbsCoord height = twGetClientHeight(MainWnd);
 for(i = 0;i < height;i++)
 {
   tlines[i].st = lval;
   if(lval < flen) lval = ForwardScanCR(lval,flen);
   tlines[i].end = lval;
 }
}

unsigned long tmocpos = 0;

static void __NEAR__ __FASTCALL__ PrepareLines(int keycode)
{
 int size,size1,h,height = twGetClientHeight(MainWnd);
 unsigned cp_symb_len;
 unsigned long lval,flen,cp = BMGetCurrFilePos();
 cp_symb_len = activeNLS->get_symbol_size();
 flen = BMGetFLength();
 /** search begin of first string */
 h=height-1;
 size = sizeof(TSTR)*h;
 size1 = size + sizeof(TSTR);
 if(keycode == KE_UPARROW)
 {
   /** i.e. going down */
   if(tlines[0].st)
   {
     memmove(&tlines[1],tlines,size);
     tlines[0] = ptlines[h];
     memmove(&ptlines[1],ptlines,size);
     lval = ptlines[1].st;
     ptlines[0].end = lval;
     if(lval >= cp_symb_len) lval-=cp_symb_len;
     ptlines[0].st = BackScanCR(lval);
   }
 }
 else
   if(keycode == KE_DOWNARROW)
   {
     /** i.e. going up */
     if(tlines[0].end < flen)
     {
       memmove(ptlines,&ptlines[1],size);
       ptlines[h] = tlines[0];
       memmove(tlines,&tlines[1],size);
       lval = tlines[h - 1].end;
       tlines[h].st = lval;
       tlines[h].end = ForwardScanCR(lval,flen);
     }
   }
   else
     if(keycode == KE_PGUP)
     {
      if(ptlines[0].st)
      {
       memcpy(tlines,ptlines,size1);
       lval = tlines[0].st;
       FillPrevPage(lval);
      }
      else goto CommonPart;
     }
     else
       if(keycode == KE_PGDN)
       {
        if(tlines[h].end < flen)
        {
          memcpy(ptlines,tlines,size1);
          lval = ptlines[h].end;
          FillCurrPage(lval,flen);
        }
        else goto CommonPart;
       }
       else
         if(cp != tmocpos || keycode == KE_SUPERKEY)
         {
          CommonPart:
           lval = BackScanCR(cp);
           if(cp != lval) cp = lval;
           if(cp) FillPrevPage(lval);
           else { PrevStrLen = PrevPageSize = 2; memset(ptlines,0,sizeof(TSTR)*height); }
           /** scan forward */
           FillCurrPage(lval,flen);
         }
 tmocpos = cp;
 PrevStrLen = ptlines[h].end - ptlines[h].st + cp_symb_len;
 PrevPageSize = ptlines[h].end - ptlines[0].st + cp_symb_len;
}

static unsigned __NEAR__ __FASTCALL__ Tab2Space(tvioBuff * dest,unsigned int alen,char * str,unsigned int len,unsigned int shift,unsigned *n_tabs)
{
  unsigned char ch,defcol;
  size_t i,size,j,k;
  unsigned int freq;
  if(n_tabs) *n_tabs = 0;
  for(i = 0,freq = 0,k = 0;i < len;i++,freq++)
  {
    defcol = text_cset.normal;
    ch = str[i];
    if(ch < 32)
    {
      switch(ch)
      {
         default:
         case 0: /**NUL  end string*/
         case 1: /*SOH  start of heading*/
         case 2: /**SOT  start of text*/
         case 3: /**ETX  end of text*/
         case 4: /**EOT  end of transmission*/
         case 5: /**ENQ  enquiry*/
         case 6: /**ACK  acknowledge*/
         case 7: /**BEL  bell*/
         case 11: /**VT  virtical tab*/
         case 12: /**FF  form feed*/
         case 14: /**SO  shift out*/
         case 15: /**SI  shift in*/
         case 16: /**DLE data line escape*/
         case 17: /**DC1 dev ctrl 1 (X-ON)*/
         case 18: /**DC2 dev ctrl 2*/
         case 19: /**DC3 dev ctrl 3 (X-OFF)*/
         case 20: /**DC4 dev ctrl 4*/
         case 21: /**NAK negative acknowledge*/
         case 22: /**SYN synhronous idel*/
         case 23: /**ETB end transmission block*/
         case 24: /**CAN cancel*/
         case 25: /**EM  end of medium*/
         case 26: /**SUB substitude*/
         case 27: /**ESC escape*/
         case 28: /**FS  file separator*/
         case 29: /**GS  group separator*/
         case 30: /**RS  record separator*/
         case 31: /**US  unit separator*/
                  break;
         case 9: /**HT   horizontal tab*/
               {
                  size = TEXT_TAB - (freq%TEXT_TAB);
                  for(j = 0;j < size;j++,freq++)
                  {
                    if(k < alen && freq >= shift)
                    {
                      if(dest)
                      {
                         dest->chars[k] = ' ';
                         dest->oem_pg[k] = 0;
                         dest->attrs[k] = defcol;
                      }
                      k++;
                    }
                  }
                  if(n_tabs) (*n_tabs)++;
               }
               freq--;
               break;
         case 10: /**LF  line feed*/
         case 13: /**CR  cariage return*/
                  goto End;
         case 8:  /**BS  backspace*/
               {
                  char pch;
                  pch = i ? str[i-1] : str[0];
                  switch(pch)
                  {
                    case '_': defcol = text_cset.underline; break;
                    case '-': defcol = text_cset.strikethrough; break;
                    default:  defcol = text_cset.bold;
                  }
                  if(i < len) ch = str[++i];
                  if(k) k--;
                  freq--;
               }
               if(freq >= shift) goto DefChar;
               break;
      }
    }
    else
    {
      DefChar:
      if(k < alen && freq >= shift)
      {
        if(dest)
        {
          dest->chars[k] = ch;
          dest->oem_pg[k] = 0;
          dest->attrs[k] = defcol;
        }
        k++;
      }
    }
  }
  End:
  return k;
}

static char *buff;

static void __NEAR__ __FASTCALL__ txtPaintSearch(HLInfo * cptr,unsigned int shift,int i,int size,int _bin_mode)
{
    int sh,she;
    long save,savee;
    unsigned cp_symb_len,loc_st,loc_end;
    cp_symb_len = activeNLS->get_symbol_size();
    loc_st = FoundTextSt > tlines[i].st ? (unsigned)(FoundTextSt - tlines[i].st) : 0;
    loc_end = (unsigned)(FoundTextEnd - tlines[i].st);
    if(_bin_mode)
    {
      sh = loc_st - shift;
      she = loc_end - shift;
    }
    else
    {
      sh = Tab2Space(NULL,UINT_MAX,buff,loc_st/cp_symb_len,shift,NULL)*cp_symb_len;
      she= Tab2Space(NULL,UINT_MAX,buff,loc_end/cp_symb_len,shift,NULL)*cp_symb_len;
    }
    save = FoundTextSt;
    savee= FoundTextEnd;
    FoundTextSt = tlines[i].st + shift + sh/cp_symb_len;
    FoundTextEnd = tlines[i].st + shift + she/cp_symb_len;
    HiLightSearch(MainWnd,tlines[i].st + shift,0,size,i,cptr,_bin_mode == MOD_BINARY ? HLS_NORMAL : HLS_USE_BUFFER_AS_VIDEO);
    FoundTextSt = save;
    FoundTextEnd = savee;
}

static void __NEAR__ __FASTCALL__ drawBound(int x,int y,char ch)
{
  twGotoXY(x,y);
  twSetColorAttr(browser_cset.bound);
  twPutChar(ch);
  twSetColorAttr(browser_cset.main);
}

static unsigned __FASTCALL__ txtConvertBuffer(char *_buff,unsigned size,tBool use_fs_nls)
{
  return activeNLS->convert_buffer(_buff,size,use_fs_nls);
}

static unsigned __FASTCALL__ drawText( unsigned keycode , unsigned shift )
{
  int hilightline;
  size_t i;
  unsigned size,rsize,rshift;
  unsigned long cpos;
  unsigned cp_symb_len,len,tmp,textmaxlen;
  tAbsCoord height = twGetClientHeight(MainWnd);
  HLInfo hli;
  tvioBuff it;
  t_vchar chars[__TVIO_MAXSCREENWIDTH];
  t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
  ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
  it.chars = chars;
  it.oem_pg = oem_pg;
  it.attrs = attrs;
  cp_symb_len = activeNLS->get_symbol_size();
  strmaxlen = 0;
  if(shift%cp_symb_len)
  {
    if(keycode == KE_RIGHTARROW || keycode == KE_PGDN)
                                     shift+=shift%cp_symb_len;
    else
                                     shift=(shift/cp_symb_len)*cp_symb_len;
  }
  maxstrlen = wmode ? tvioWidth : (MAX_STRLEN / cp_symb_len) - 3;
  cpos = BMGetCurrFilePos();
  if(cpos%cp_symb_len)
  {
    if(keycode == KE_RIGHTARROW || keycode == KE_PGDN)
                                    cpos+=cpos%cp_symb_len;
    else
                                    cpos=(cpos/cp_symb_len)*cp_symb_len;
  }
  BMSeek(cpos,BIO_SEEK_SET);
  if(!(keycode == KE_LEFTARROW || keycode == KE_RIGHTARROW))
                                     PrepareLines(keycode);
  hilightline = -1;
  for(i = 0;i < height;i++)
  {
    len = (unsigned)(tlines[i].end - tlines[i].st);
    if(len > strmaxlen) strmaxlen = len;
    if(isHOnLine(tlines[i].st,len))
    {
      hilightline = i;
      if(keycode == KE_JUSTFIND)
      {
        if(bin_mode != MOD_BINARY)
        {
          unsigned n_tabs,b_ptr,b_lim;
          len = min(MAX_STRLEN,FoundTextSt > tlines[i].st ? (int)(FoundTextSt-tlines[i].st):0);
          BMReadBufferEx((void *)buff,len,tlines[i].st,BM_SEEK_SET);
          len = txtConvertBuffer(buff,len,False);
          for(b_lim=len,b_ptr = 0;b_ptr < len;b_ptr+=2,b_lim-=2)
          {
            shift = Tab2Space(NULL,UINT_MAX,&buff[b_ptr],b_lim,0,&n_tabs);
            if(shift) shift-=cp_symb_len;
            if(shift < (unsigned)(tvioWidth/2)) break;
          }
          shift = Tab2Space(NULL,UINT_MAX,buff,b_ptr,0,NULL);
        }
        else
        {
          if(!isHOnLine((tlines[i].st+shift)*cp_symb_len,min(len,tvioWidth)))
          {
            shift = ((unsigned)(FoundTextSt - tlines[i].st)-tvioWidth/2)/cp_symb_len;
            if((int)shift < 0) shift = 0;
            if(shift%cp_symb_len) shift+=shift%cp_symb_len;
          }
        }
      }
      break;
    }
  }
  textmaxlen = maxstrlen - 2;
  twFreezeWin(MainWnd);
  for(i = 0;i < height;i++)
  {
    len = (int)(tlines[i].end - tlines[i].st);
    if(isHOnLine(tlines[i].st,len)) hilightline = i;
    rshift = bin_mode != MOD_BINARY ? 0 : shift;
    rsize = size = len - rshift;
    if(len > rshift)
    {
        BMReadBufferEx((void *)buff,size,tlines[i].st + rshift,BM_SEEK_SET);
        rsize = size = txtConvertBuffer(buff,size,False);
        if(bin_mode != MOD_BINARY)
        {
             rsize = size = Tab2Space(&it,__TVIO_MAXSCREENWIDTH,buff,size,shift,NULL);
             tmp = size + shift;
             if(strmaxlen < tmp) strmaxlen = tmp;
             if(textmaxlen < tmp) textmaxlen = tmp;
        }
        if(size > tvioWidth) size = tvioWidth;
        if(i == (unsigned)hilightline)
        {
          if(bin_mode == MOD_BINARY) hli.text = buff;
          else                       hli.buff = it;
          txtPaintSearch(&hli,shift,i,size,bin_mode == MOD_BINARY);
        }
        else
        {
          if(bin_mode == MOD_BINARY) twDirectWrite(1,i+1,buff,size);
          else                       twWriteBuffer(twUsedWin(),1,i + 1,&it,size);
        }
        if(rsize < tvioWidth)
        {
          twGotoXY(1 + rsize,i + 1);
          twClrEOL();
        }
        else
          if(rsize > tvioWidth)
             drawBound(tvioWidth,i + 1,TWC_RT_ARROW);
    }
    else
    {
       twGotoXY(1,i + 1);
       twClrEOL();
    }
    if(shift) drawBound(1,i + 1,TWC_LT_ARROW);
    lastbyte = tlines[i].st;
    lastbyte += bin_mode == MOD_BINARY ? shift + size : rshift + len;
  }
  twRefreshWin(MainWnd);
  tmp = textmaxlen - tvioWidth + 2;
  if(shift > tmp) shift = tmp;
  if(!tlines[1].st) tlines[1].st = tlines[0].end;
  CurrStrLen = tlines[0].end - tlines[0].st;
  CurrPageSize = lastbyte - tlines[0].st;
  BMSeek(cpos,BM_SEEK_SET);
  return shift;
}

static void __FASTCALL__ HelpTxt( void )
{
   hlpDisplay(1001);
}

static unsigned long __FASTCALL__ txtPrevPageSize( void ) { return PrevPageSize; }
static unsigned long __FASTCALL__ txtCurrPageSize( void ) { return CurrPageSize; }
static unsigned long __FASTCALL__ txtPrevLineWidth( void ) { return PrevStrLen; }
static unsigned long __FASTCALL__ txtCurrLineWidth( void ) { return CurrStrLen; }
static const char *  __FASTCALL__ txtMiscKeyName( void ) { return wmode ? "Unwrap" : "Wrap  "; }
static void          __FASTCALL__ txtMiscKeyAction( void ) { wmode = wmode ? False : True; }

static tBool __FASTCALL__ txtSelectCP( void )
{
  return activeNLS->select_table();
}

static tBool __FASTCALL__ txtSelectMode( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(mod_names)/sizeof(char *);
  i = SelBoxA(mod_names,nModes," Select viewing page: ",bin_mode);
  if(i != -1)
  {
    bin_mode = i;
    return True;
  }
  return False;
}

static tBool __FASTCALL__ txtSelectNLS( void )
{
  const char *modeName[sizeof(nls_set)/sizeof(REGISTRY_NLS *)];
  size_t i,nModes;
  int retval;

  nModes = sizeof(nls_set)/sizeof(REGISTRY_NLS *);
  for(i = 0;i < nModes;i++) modeName[i] = nls_set[i]->set_name;
  retval = SelBoxA(modeName,nModes," Select NLS set: ",defNLSSet);
  if(retval != -1)
  {
    if(activeNLS->term) activeNLS->term();
    activeNLS = nls_set[retval];
    if(activeNLS->init) activeNLS->init();
    defNLSSet = retval;
    return True;
  }
  return False;
}


static tBool __NEAR__ __FASTCALL__ isBinByte(unsigned char ch)
{
  return ch < 32 && !isspace(ch & 0xFF) && ch != 0x08 && ch != 0x1A;
}

static tBool __FASTCALL__ txtDetect( void )
{
  size_t maxl,i;
  tBool bin = False;
  unsigned long flen;
  maxl = 1000;
  flen = BMGetFLength();
  if(maxl > flen) maxl = (size_t)flen;
  for(i = 0;i < maxl;i++)
  {
   char ch;
   ch = BMReadByteEx(i,BM_SEEK_SET);
   if((bin=isBinByte(ch))!=False) break;
  }
  if(bin==False) bin_mode = MOD_PLAIN;
  return bin == False;
}

static void __FASTCALL__ txtReadIni( hIniProfile *ini )
{
  char tmps[10];
  if(isValidIniArgs())
  {
    int w_m;
    biewReadProfileString(ini,"Biew","Browser","SubSubMode4","0",tmps,sizeof(tmps));
    defNLSSet = (unsigned)strtoul(tmps,NULL,10);
    if(defNLSSet > sizeof(nls_set)/sizeof(REGISTRY_NLS *)) defNLSSet = 0;
    activeNLS = nls_set[defNLSSet];
    if(activeNLS->init) activeNLS->init();
    activeNLS->read_ini(ini);
    biewReadProfileString(ini,"Biew","Browser","SubSubMode3","0",tmps,sizeof(tmps));
    bin_mode = (unsigned)strtoul(tmps,NULL,10);
    if(bin_mode > MOD_MAXMODE) bin_mode = 0;
    biewReadProfileString(ini,"Biew","Browser","MiscMode","0",tmps,sizeof(tmps));
    w_m = (int)strtoul(tmps,NULL,10);
    wmode = w_m ? True : False;
 }
}

static void __FASTCALL__ txtSaveIni( hIniProfile *ini )
{
  char tmps[10];
  sprintf(tmps,"%i",bin_mode);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode3",tmps);
  biewWriteProfileString(ini,"Biew","Browser","MiscMode",wmode ? "1" : "0");
  sprintf(tmps,"%i",defNLSSet);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode4",tmps);
  activeNLS->save_ini(ini);
}

static void __FASTCALL__ txtInit( void )
{
   buff = PMalloc(MAX_STRLEN);
   tlines = PMalloc(sizeof(TSTR)*__TVIO_MAXSCREENWIDTH);
   ptlines = PMalloc(sizeof(TSTR)*__TVIO_MAXSCREENWIDTH);
   if((!buff) || (!tlines) || !(ptlines))
   {
     MemOutBox("Text mode initialization");
     exit(EXIT_FAILURE);
   }
   if((txtHandle = bioDup(BMbioHandle())) == &bNull) txtHandle = BMbioHandle();
}

static void __FASTCALL__ txtTerm( void )
{
   PFREE(buff);
   PFREE(tlines);
   PFREE(ptlines);
   if(txtHandle != BMbioHandle()) { bioClose(txtHandle); txtHandle = &bNull; }
}

static unsigned __FASTCALL__ txtCharSize( void ) { return activeNLS->get_symbol_size(); }

REGISTRY_MODE textMode =
{
  "~Text mode",
  { NULL, "CodPag", "TxMode", "NLSSet", NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, txtSelectCP, txtSelectMode, txtSelectNLS, NULL, NULL, NULL, NULL, NULL, NULL },
  txtDetect,
  __MF_TEXT,
  drawText,
  txtConvertBuffer,
  txtCharSize,
  txtMiscKeyName,
  txtMiscKeyAction,
  txtPrevPageSize,
  txtCurrPageSize,
  txtPrevLineWidth,
  txtCurrLineWidth,
  HelpTxt,
  txtReadIni,
  txtSaveIni,
  txtInit,
  txtTerm,
  NULL
};






