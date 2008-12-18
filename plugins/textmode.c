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
#include "bin_util.h"
#include "biewutil.h"
#include "biewhelp.h"
#include "bconsole.h"
#include "search.h"
#include "reg_form.h"
#include "biewlib/kbd_code.h"
#include "biewlib/pmalloc.h"

typedef struct tag_shash_s
{
    unsigned start;
    unsigned total;
}shash_t;

typedef struct tag_acontext_hl_s
{
    Color color;
    long  start_off;
    long  end_off;
}acontext_hl_t;

typedef struct tag_context_hl_s
{
    Color color;
    char  *start_seq;
    char  *end_seq;
}context_hl_t;

typedef struct tag_keyword_hl_s
{
    Color color;
    char  *keyword;
    unsigned len; /* for accelerating */
}keyword_hl_t;

static struct tag_syntax_hl_s
{
   char *name;
   context_hl_t *context;
   keyword_hl_t *keyword;
   shash_t       kwd_hash[256];
   ColorAttr     op_hash[256];
   unsigned context_num,keyword_num;
   unsigned maxkwd_len;
}syntax_hl;

acontext_hl_t __HUGE__ *acontext; /* means active context*/
unsigned long  acontext_num;

extern char biew_syntax_name[];
extern char **  ArgVector;

static int HiLight = 1;
static char detected_syntax_name[FILENAME_MAX+1] = "";
static unsigned char word_set[UCHAR_MAX+1],wset[UCHAR_MAX+1];

#define MAX_STRLEN 1000 /**< defines maximal length of string */
#define is_legal_word_char(ch) ((int)word_set[(unsigned char)ch])

static ColorAttr __NEAR__ __FASTCALL__ hlFindKwd(const char *str,Color col,unsigned *st_len)
{
  int found,res;
  unsigned i,len,st,end;
  ColorAttr defcol=LOGFB_TO_PHYS(col,BACK_COLOR(text_cset.normal));
  *st_len=0;
  if(syntax_hl.kwd_hash[(unsigned char)str[0]].start!=UINT_MAX)
  {
    st=syntax_hl.kwd_hash[(unsigned char)str[0]].start;
    end=st+syntax_hl.kwd_hash[(unsigned char)str[0]].total;
    for(i=st;i<end;i++)
    {
	len=syntax_hl.keyword[i].len;
	found=0;
	if(len>1)
	{
	    res=memcmp(&str[1],&syntax_hl.keyword[i].keyword[1],len-1);
	    if(res==0) found=1;
	}
	else found=1;
	if(found)
	{
		defcol=LOGFB_TO_PHYS(syntax_hl.keyword[i].color,BACK_COLOR(text_cset.normal));
		*st_len=len;
		break;
	}
    }
  }
  return defcol;
}

static void __NEAR__ __FASTCALL__ txtMarkupCtx(void)
{
    long ii,fpos,flen;
    unsigned i,len;
    int found;
    char tmps[MAX_STRLEN],etmps[MAX_STRLEN],ktmps[MAX_STRLEN],ch,chn;
    TWindow *hwnd;
    hwnd=PleaseWaitWnd();
    flen=BMGetFLength();
    fpos=BMGetCurrFilePos();
    BMSeek(0,BM_SEEK_SET);
    acontext_num=0;
    for(ii=0;ii<flen;ii++)
    {
	ch=BMReadByte();
	if(ch=='\r') ch='\n';
	for(i=0;i<syntax_hl.context_num;i++)
	{
	    unsigned ss_idx;
	    ss_idx=0;
	    if(syntax_hl.context[i].start_seq[0]=='\n')
	    {
		if(ii==0) ss_idx=1;
		else
		{ 
		    long ccpos;
		    ccpos=BMGetCurrFilePos();
		    chn=BMReadByteEx(ii-1,BM_SEEK_SET);
		    BMSeek(ccpos, BM_SEEK_SET);
		    if(chn=='\n' || chn=='\r') ss_idx=1;
		}
	    }
	    if(ch==syntax_hl.context[i].start_seq[ss_idx])
	    {
		long cpos;
		len=strlen(syntax_hl.context[i].start_seq);
		cpos=BMGetCurrFilePos();
		found=0;
		if(len>(ss_idx+1))
		{
		    BMReadBuffer(tmps,len-(ss_idx+1));
		    if(memcmp(tmps,&syntax_hl.context[i].start_seq[ss_idx+1],len-(ss_idx+1))==0) found=1;
		}
		else found=1;
		if(found)
		{
		    /* avoid context markup if it's equal one of keywords */
		    unsigned st_len;
		    long ckpos;
		    ktmps[0]=ch;
		    memcpy(&ktmps[1],tmps,len-(ss_idx+1));
		    if(syntax_hl.maxkwd_len>(len-ss_idx))
		    {
			ckpos=BMGetCurrFilePos();
			BMReadBuffer(&ktmps[len],syntax_hl.maxkwd_len-(len-ss_idx));
			BMSeek(ckpos,BM_SEEK_SET);
		    }
		    hlFindKwd(ktmps,0,&st_len);
		    if(st_len) found=0;
		}
		if(found)
		{
		    if(!acontext) acontext=PHMalloc(sizeof(acontext_hl_t));
		    else	  acontext=PHRealloc(acontext,sizeof(acontext_hl_t)*(acontext_num+1));
		    acontext[acontext_num].color=LOGFB_TO_PHYS(syntax_hl.context[i].color,BACK_COLOR(text_cset.normal));
		    acontext[acontext_num].start_off=ii-ss_idx;
		    acontext[acontext_num].end_off=flen;
		    ii+=len;
		    BMSeek(ii,BM_SEEK_SET);
		    /* try find end */
		    strcpy(etmps,syntax_hl.context[i].end_seq);
		    len=strlen(etmps);
		    for(;ii<flen;ii++)
		    {
			ch=BMReadByte();
			if(ch==etmps[0])
			{
			    long ecpos;
			    ecpos=BMGetCurrFilePos();
			    found=0;
			    if(len>1)
			    {
				BMReadBuffer(tmps,len-1);
				if(memcmp(tmps,&etmps[1],len-1)==0) found=1;
			    }
			    else found=1;
			    if(found)
			    {
				ii+=len;
				BMSeek(ii,BM_SEEK_SET);
				acontext[acontext_num].end_off=ii;
				break;
			    }
			    else BMSeek(ecpos,BM_SEEK_SET);
			}
		    }
		    acontext_num++;
		    ii--;
		}
		else BMSeek(cpos,BM_SEEK_SET);
	    }
	}
    }    
    BMSeek(fpos,BM_SEEK_SET);
    CloseWnd(hwnd);
}

static void unfmt_str(unsigned char *str)
{
   long result;
   unsigned base,i;
   char *src,*dest,ch,temp[MAX_STRLEN];
   src=dest=str;
   while(*src)
   {
	ch=*src;
	src++;
	if(ch == '\\')
	{
	    if(*src== '\\');
	    else
	    if(*src=='n') { src++; ch='\n'; }
	    else
	    if(*src=='t') { src++; ch='\t'; }
	    else
	    {
		base=10;
		i=0;
		if(*src=='0') { src++; base=8; }
		if(*src=='x' || *src=='X') { base=16; src++; while(isxdigit(*src)) temp[i++]=*src++; }
		else { while(isdigit(*src)) temp[i++]=*src++; }
		temp[i]=0;
		result=strtol(temp,NULL,base);
		ch=result;
	    }
	}
	*dest++=ch;
   }
   *dest=0;
}

static tBool __FASTCALL__ txtFiUserFunc1(IniInfo * info)
{
  char *p;
  if(strcmp(info->section,"Extensions")==0)
  {
	p = strrchr(ArgVector[1],'.');
	if(p)
	{
	    p++;
	    if(strcmp(p,info->item)==0)
	    {
		strcpy(detected_syntax_name,info->value);
		return True;
	    }
	}
  }
  if(strcmp(info->section,"Names")==0)
  {
	char *pp;
	p = strrchr(ArgVector[1],'/');
	pp = strrchr(ArgVector[1],'\\');
	p=max(p,pp);
	if(p) p++;
	else  p=ArgVector[1];
	if(memcmp(p,info->item,strlen(info->item))==0)
	{
	    strcpy(detected_syntax_name,info->value);
	    return True;
	}
  }
  if(strcmp(info->section,"Context")==0)
  {
	long off,fpos;
	unsigned i,ilen;
	int found,softmode;
	off = atol(info->item);
	p = strstr(info->value,"-->");
	if(!p) { ErrMessageBox("Missed separator in main context definition",NULL); return True; }
	*p=0;
	softmode=0;
	if(strcmp(info->subsection,"Soft")==0) softmode=1;
	unfmt_str(info->value);
	ilen=strlen(info->value);
	fpos=BMGetCurrFilePos();
	BMSeek(off,BM_SEEK_SET);
	found=1;
	for(i=0;i<ilen;i++)
	{
	    unsigned char ch;
	    ch=BMReadByte();
	    if(softmode) while(isspace(ch)) { ch=BMReadByte(); if(BMEOF()) { found = 0; break; }}
	    if(ch != info->value[i])
	    {
		found=0;
		break;
	    }
	}
	BMSeek(fpos,BM_SEEK_SET);
	if(found)
	{
	    strcpy(detected_syntax_name,p+3);
	    return True;
	}
  }
  return False;
}

static Color __NEAR__ __FASTCALL__ getCtxColorByName(const char *subsection,const char *item,Color cdef,tBool *err)
{
    PrgWordCSet *cset;
    *err=0;
    cset=NULL;
    if(strcmp(subsection,"Comments")==0) cset=&prog_cset.comments;
    else if(strcmp(subsection,"Preproc")==0) cset=&prog_cset.preproc;
    else if(strcmp(subsection,"Constants")==0) cset=&prog_cset.constants;
    else if(strcmp(subsection,"Keywords")==0) cset=&prog_cset.keywords;
    else if(strcmp(subsection,"Operators")==0) cset=&prog_cset.operators;
    else ErrMessageBox("Unknown context subsection definition",subsection);
    if(cset)
    {
	if(strcmp(item,"base")==0) return cset->base;
	if(strcmp(item,"extended")==0) return cset->extended;
	if(strcmp(item,"reserved")==0) return cset->reserved;
	if(strcmp(item,"alt")==0) return cset->alt;
	ErrMessageBox("Unknown context class definition",item);
    }
    *err=1;
    return cdef;
}

static Color __NEAR__ __FASTCALL__ getKwdColorByName(const char *item,Color cdef,tBool *err)
{
    *err=0;
    if(strcmp(item,"base")==0) return prog_cset.keywords.base;
    if(strcmp(item,"extended")==0) return prog_cset.keywords.extended;
    if(strcmp(item,"reserved")==0) return prog_cset.keywords.reserved;
    if(strcmp(item,"alt")==0) return prog_cset.keywords.alt;
    ErrMessageBox("Unknown keyword class definition",item);
    *err=1;
    return cdef;
}

static Color __NEAR__ __FASTCALL__ getOpColorByName(const char *item,Color cdef,tBool *err)
{
    *err=0;
    if(strcmp(item,"base")==0) return prog_cset.operators.base;
    if(strcmp(item,"extended")==0) return prog_cset.operators.extended;
    if(strcmp(item,"reserved")==0) return prog_cset.operators.reserved;
    if(strcmp(item,"alt")==0) return prog_cset.operators.alt;
    ErrMessageBox("Unknown keyword class definition",item);
    *err=1;
    return cdef;
}

extern char last_skin_error[];
static char *last_syntax_err="";
static tBool __FASTCALL__ txtFiUserFunc2(IniInfo * info)
{
  char *p;
  tBool err;
  Color cdef=FORE_COLOR(text_cset.normal);
  err=False;
  if(strcmp(info->section,"General")==0)
  {
     if(strcmp(info->item,"Name")==0)
     {
       syntax_hl.name=malloc(strlen(info->value)+1);
       strcpy(syntax_hl.name,info->value);
     }
     if(strcmp(info->item,"WSet")==0)
     {
       unsigned i,len,rlen;
       len=strlen(info->value);
       for(i=0;i<len;i++) word_set[(unsigned char)info->value[i]]=1;
       rlen=min(UCHAR_MAX,len);
       memcpy(wset,info->value,rlen);
       wset[rlen]=0;
     }     
  }     
  if(strcmp(info->section,"Context")==0)
  {
     Color col;
     col = getCtxColorByName(info->subsection,info->item,cdef,&err);
     if(!err)
     {
       if(!syntax_hl.context) syntax_hl.context=malloc(sizeof(context_hl_t));
       else syntax_hl.context=realloc(syntax_hl.context,sizeof(context_hl_t)*(syntax_hl.context_num+1));
       syntax_hl.context[syntax_hl.context_num].color=col;
       p=strstr(info->value,"...");
       if(!p) { last_syntax_err="Missed separator in context definition"; return True; }
       *p=0;
       p+=3;
       unfmt_str(info->value);
       unfmt_str(p);
       syntax_hl.context[syntax_hl.context_num].start_seq=malloc(strlen(info->value)+1);
       strcpy(syntax_hl.context[syntax_hl.context_num].start_seq,info->value);
       syntax_hl.context[syntax_hl.context_num].end_seq=malloc(strlen(p)+1);
       strcpy(syntax_hl.context[syntax_hl.context_num].end_seq,p);
       syntax_hl.context_num++;
     }
     else last_syntax_err=last_skin_error;
  }
  if(strcmp(info->section,"Keywords")==0)
  {
     Color col;
     col = getKwdColorByName(info->item,cdef,&err);
     if(!err)
     {
       if(!syntax_hl.keyword) syntax_hl.keyword=malloc(sizeof(keyword_hl_t));
       else syntax_hl.keyword=realloc(syntax_hl.keyword,sizeof(keyword_hl_t)*(syntax_hl.keyword_num+1));
       syntax_hl.keyword[syntax_hl.keyword_num].color=col;
       unfmt_str(info->value);
       syntax_hl.keyword[syntax_hl.keyword_num].len=strlen(info->value);
       syntax_hl.keyword[syntax_hl.keyword_num].keyword=malloc(syntax_hl.keyword[syntax_hl.keyword_num].len+1);
       strcpy(syntax_hl.keyword[syntax_hl.keyword_num].keyword,info->value);
       if(syntax_hl.keyword[syntax_hl.keyword_num].len > syntax_hl.maxkwd_len)
		syntax_hl.maxkwd_len=syntax_hl.keyword[syntax_hl.keyword_num].len;
       syntax_hl.keyword_num++;
     }
     else last_syntax_err=last_skin_error;
  }
  if(strcmp(info->section,"Operators")==0)
  {
     Color col;
     col = getOpColorByName(info->item,cdef,&err);
     if(!err)
     {
       unfmt_str(info->value);
       if(strlen(info->value)>1) { last_syntax_err="Too long operator has been found"; return True; }
       syntax_hl.op_hash[(unsigned char)info->value[0]]=LOGFB_TO_PHYS(col,BACK_COLOR(text_cset.normal));
     }
     else last_syntax_err=last_skin_error;
  }
  return err?True:False;
}

static tCompare __FASTCALL__ cmp_ctx(const void __HUGE__ *e1,const void __HUGE__ *e2)
{
  const char *k1,*k2;
  int sl1,sl2,res;
  k1=(*(const context_hl_t __HUGE__ *)e1).start_seq;
  k2=(*(const context_hl_t __HUGE__ *)e2).start_seq;
  sl1=strlen((*(const context_hl_t __HUGE__ *)e1).start_seq);
  sl2=strlen((*(const context_hl_t __HUGE__ *)e2).start_seq);
  res=memcmp(k1,k2,min(sl1,sl2));
  return res==0?__CmpLong__(sl2,sl1):res;
}

static tCompare __FASTCALL__ cmp_kwd(const void __HUGE__ *e1,const void __HUGE__ *e2)
{
  const char *k1,*k2;
  int sl1,sl2,res;
  k1=(*(const keyword_hl_t __HUGE__ *)e1).keyword;
  k2=(*(const keyword_hl_t __HUGE__ *)e2).keyword;
  sl1=strlen((*(const keyword_hl_t __HUGE__ *)e1).keyword);
  sl2=strlen((*(const keyword_hl_t __HUGE__ *)e2).keyword);
  res=memcmp(k1,k2,min(sl1,sl2));
  return res==0?__CmpLong__(sl2,sl1):res;
}

static void txtReadSyntaxes(void)
{
  if(__IsFileExists(biew_syntax_name))
  {
    FiProgress(biew_syntax_name,txtFiUserFunc1);
    if(detected_syntax_name[0])
    {
	char tmp[FILENAME_MAX+1];
	char *p;
	char *pp;
	strcpy(tmp,biew_syntax_name);
	p=strrchr(tmp,'/');
	pp=strrchr(tmp,'\\');
	p=max(p,pp);
	if(p) { p++; strcpy(p,detected_syntax_name); }
	strcpy(detected_syntax_name,tmp);
	if(__IsFileExists(detected_syntax_name))
	{
	    unsigned i,total;
	    int phash;
	    memset(word_set,0,sizeof(word_set));
	    FiProgress(detected_syntax_name,txtFiUserFunc2);
	    if(last_syntax_err[0]) ErrMessageBox(last_syntax_err,NULL);
	    /* put longest strings on top */
	    HQSort(syntax_hl.context,syntax_hl.context_num,sizeof(context_hl_t),cmp_ctx);
	    HQSort(syntax_hl.keyword,syntax_hl.keyword_num,sizeof(keyword_hl_t),cmp_kwd);
	    /* Fill keyword's hash */
	    phash=-1;
	    total=0;
	    for(i=0;i<sizeof(syntax_hl.kwd_hash)/sizeof(syntax_hl.kwd_hash[0]);i++)
		    syntax_hl.kwd_hash[i].start=syntax_hl.kwd_hash[i].total=UINT_MAX;
	    for(i=0;i<syntax_hl.keyword_num;i++)
	    {
		if(syntax_hl.keyword[i].keyword[0]!=phash)
		{
		    if(phash!=-1) syntax_hl.kwd_hash[phash].total=total;
		    phash=(unsigned char)(syntax_hl.keyword[i].keyword[0]);
		    syntax_hl.kwd_hash[phash].start=i;
		    total=0;
		}
		total++;
	    }
	    if(phash!=-1) syntax_hl.kwd_hash[phash].total=total;
	    if(syntax_hl.context_num) txtMarkupCtx();
	}
    }
  }
}

static ColorAttr __NEAR__ __FASTCALL__ hlGetCtx(long off,int *is_valid, long *end_ctx)
{
    unsigned long ii;
    *is_valid=0;
    *end_ctx=BMGetFLength();
    for(ii=0;ii<acontext_num;ii++)
    {
	if(acontext[ii].start_off <= off && off < acontext[ii].end_off)
	{
	    *is_valid=1;
	    *end_ctx=acontext[ii].end_off;
	    return acontext[ii].color;
	}
	if(off < acontext[ii].start_off) { *end_ctx=acontext[ii].start_off; break; }
    }
    return 0;
}

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

static unsigned char __NEAR__ __FASTCALL__ nlsReadByte(__filesize_t cp)
{
 char nls_buff[256];
 unsigned sym_size;
 sym_size = activeNLS->get_symbol_size();
 bioSeek(txtHandle,cp,SEEK_SET);
 bioReadBuffer(txtHandle,nls_buff,sym_size);
 activeNLS->convert_buffer(nls_buff,sym_size,True);
 return (unsigned char)nls_buff[0];
}

static __filesize_t __NEAR__ __FASTCALL__ BackScanCR( __filesize_t cp )
{
 __filesize_t lval;
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

static __filesize_t __NEAR__ __FASTCALL__ ForwardScanCR( __filesize_t cp ,__filesize_t flen)
{
 __filesize_t lval;
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

static void __NEAR__ __FASTCALL__ FillPrevPage(__filesize_t lval)
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

static void __NEAR__ __FASTCALL__ FillCurrPage(__filesize_t lval,__filesize_t flen)
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

static __filesize_t tmocpos = 0;

static void __NEAR__ __FASTCALL__ PrepareLines(int keycode)
{
 int size,size1,h,height = twGetClientHeight(MainWnd);
 unsigned cp_symb_len;
 __filesize_t lval,flen,cp = BMGetCurrFilePos();
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

static unsigned __NEAR__ __FASTCALL__ Tab2Space(tvioBuff * dest,unsigned int alen,char * str,unsigned int len,unsigned int shift,unsigned *n_tabs,long lstart)
{
  long end_ctx;
  unsigned char ch,defcol;
  size_t i,size,j,k;
  unsigned int freq, end_kwd, st_len;
  int in_ctx,in_kwd;
  ColorAttr ctx_color=text_cset.normal;
  if(n_tabs) *n_tabs = 0;
  in_ctx=0;
  in_kwd=0;
  end_kwd=0; end_ctx=0;
  st_len=0;
  for(i = 0,freq = 0,k = 0;i < len;i++,freq++)
  {
    defcol = text_cset.normal;
    ch = str[i];
    if(dest && HiLight)
    {
        if(end_ctx)
	{
		if((lstart+(long)i)>=end_ctx) goto rescan;
	}
	else
	{
		rescan:
		ctx_color=hlGetCtx(lstart+i,&in_ctx,&end_ctx);
	}
	if(!in_ctx && i>=end_kwd)
	{
	    ctx_color = hlFindKwd(&str[i],(Color)defcol,&st_len);
	    in_kwd=st_len?1:0;
	    if(is_legal_word_char(str[i+st_len])||(i && is_legal_word_char(str[i-1]))) in_kwd=0;
	    if(in_kwd) end_kwd=i+st_len;
	    else       end_kwd=strspn(&str[i],wset);
	}
	if(in_kwd || in_ctx) defcol=ctx_color;
    }
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
	  if(!(in_ctx||in_kwd) && HiLight) defcol=syntax_hl.op_hash[ch];
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
    __fileoff_t save,savee;
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
      sh = Tab2Space(NULL,UINT_MAX,buff,loc_st/cp_symb_len,shift,NULL,0L)*cp_symb_len;
      she= Tab2Space(NULL,UINT_MAX,buff,loc_end/cp_symb_len,shift,NULL,0L)*cp_symb_len;
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
  __filesize_t cpos;
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
            shift = Tab2Space(NULL,UINT_MAX,&buff[b_ptr],b_lim,0,&n_tabs,0L);
            if(shift) shift-=cp_symb_len;
            if(shift < (unsigned)(tvioWidth/2)) break;
          }
          shift = Tab2Space(NULL,UINT_MAX,buff,b_ptr,0,NULL,0L);
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
             rsize = size = Tab2Space(&it,__TVIO_MAXSCREENWIDTH,buff,size,shift,NULL,tlines[i].st);
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
  i = SelBoxA(mod_names,nModes," Select text mode: ",bin_mode);
  if(i != -1)
  {
    bin_mode = i;
    return True;
  }
  return False;
}

static const char *hilight_names[] =
{
   "~Mono",
   "~Highlight"
};
static tBool __FASTCALL__ txtSelectHiLight( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(hilight_names)/sizeof(char *);
  i = SelBoxA(hilight_names,nModes," Select highlight mode: ",HiLight);
  if(i != -1)
  {
    HiLight = i;
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
  __filesize_t flen,fpos;
  maxl = 1000;
  flen = BMGetFLength();
  fpos=BMGetCurrFilePos();
  if(maxl > flen) maxl = (size_t)flen;
  for(i = 0;i < maxl;i++)
  {
   char ch;
   ch = BMReadByteEx(i,BM_SEEK_SET);
   if((bin=isBinByte(ch))!=False) break;
  }
  if(bin==False) bin_mode = MOD_PLAIN;
  BMSeek(fpos,BM_SEEK_SET);
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
    biewReadProfileString(ini,"Biew","Browser","SubSubMode9","0",tmps,sizeof(tmps));
    HiLight = (int)strtoul(tmps,NULL,10);
    if(HiLight > 1) HiLight = 1;
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
  sprintf(tmps,"%i",HiLight);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode9",tmps);
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
   memset(&syntax_hl,0,sizeof(syntax_hl));
   /* Fill operator's hash */
   memset(syntax_hl.op_hash,text_cset.normal,sizeof(syntax_hl.op_hash));
   if(txtDetect()) txtReadSyntaxes();
}

static tBool __FASTCALL__ txtShowType( void )
{
    char *type;
    if(syntax_hl.name) type=syntax_hl.name;
    else type="Unknown";
    TMessageBox(type," Detected text type: ");
    return False;
}

static void __FASTCALL__ txtTerm( void )
{
  unsigned i;
  PFREE(buff);
  PFREE(tlines);
  PFREE(ptlines);
  if(txtHandle != BMbioHandle()) { bioClose(txtHandle); txtHandle = &bNull; }
  if(syntax_hl.name) free(syntax_hl.name);
  if(syntax_hl.context)
  {
     for(i=0;i<syntax_hl.context_num;i++) { free(syntax_hl.context[i].start_seq); free(syntax_hl.context[i].end_seq); }
     free(syntax_hl.context);
  }
  if(syntax_hl.keyword)
  {
     for(i=0;i<syntax_hl.keyword_num;i++) { free(syntax_hl.keyword[i].keyword); }
     free(syntax_hl.keyword);
  }
  PHFree(acontext);
  acontext_num=0;
  memset(&syntax_hl,0,sizeof(syntax_hl));
}

static unsigned __FASTCALL__ txtCharSize( void ) { return activeNLS->get_symbol_size(); }

REGISTRY_MODE textMode =
{
  "~Text mode",
  { NULL, "CodPag", "TxMode", "NLSSet", NULL, NULL, NULL, "TxType", "HiLght", "UsrNam" },
  { NULL, txtSelectCP, txtSelectMode, txtSelectNLS, NULL, NULL, NULL, txtShowType, txtSelectHiLight, udnUserNames },
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






