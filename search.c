/**
 * @namespace   biew
 * @file        search.c
 * @brief       This file contains implementation of file search interface.
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
#include <limits.h>
#include <stdlib.h>
#define  _CT_FTM
#include <ctype.h>

#include "colorset.h"
#include "bmfile.h"
#include "tstrings.h"
#include "search.h"
#include "biewhelp.h"
#include "biewutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "biewlib/kbd_code.h"
#include "biewlib/pmalloc.h"
#include "biewlib/bbio.h"

static TWindow *prcntswnd;
extern void ReReadFile( int );

unsigned char search_buff[MAX_SEARCH_SIZE] = "";
unsigned char search_len = 0;
unsigned biewSearchFlg = SF_NONE;

__filesize_t FoundTextSt = 0,FoundTextEnd = 0;

tBool __found;

void __FASTCALL__ fillBoyerMooreCache(int *cache, const char *pattern,
                                      unsigned pattern_len, tBool case_sens)
{
  size_t iptr;
  memset(cache,0,(UCHAR_MAX+1)*sizeof(int));
  for(iptr = 0;iptr < pattern_len;iptr++)
  {
    cache[ case_sens ? ((const unsigned char *)pattern)[iptr] :
                        toupper(((const unsigned char *)pattern)[iptr]) ] = iptr+1;
  }
}

#define __LF_NORMAL    0x0000 /**< Indicates normal search engine */
#define __LF_NOSEEK    0x0001 /**< Indicates that search must be performed at given offset only */
#define __LF_NOLEFT    0x0002 /**< Indicates that search engine should ignore left whitespace */
#define __LF_NORIGHT   0x0004 /**< Indicates that search engine should ignore right whitespace */
#define __LF_HIDEPRCNT 0x8000 /**< Indicates that search engine must not display percents >*/

                   /** Performs single search (without templates) within file.
                     * @return                address of found sequence
                     *                        if global variable __found is True
                     *                        otherwise:
                     *                        0L - if sequence is not found
                     *                        FILESIZE_MAX - if sequence can not be
                     *                        found (EOF is reached)
                     * @param sfrom           indicates string where search must
                     *                        be performed. If NULL then search
                     *                        will be performed in file stream.
                     * @param slen            indicates length of sfrom
                     * @param flags           indicates __LF_* flags family
                     * @param start           indicates offset from which search
                     *                        must be performed
                     * @param scache          indicates Boyer-Moore cache.
                     *                        If NULL then will be used internal
                     *                        cache.
                     * @param pattern         indicates search pattern
                     * @param pattern_size    indicates size of search pattern
                     * @param biewFlg         indicates global flags of Biew
                     *                        search engine.
                    **/
static __filesize_t __NEAR__ __FASTCALL__  ___lfind(const char *sfrom, 
                                                    unsigned slen, 
                                                    unsigned flags,
                                                    __filesize_t start, 
                                                    const int *scache,
                                                    const char *pattern,
                                                    unsigned pattern_size,
                                                    unsigned biewFlg)
{
  __filesize_t flen, endscan, orig_start;
  __filesize_t tsize,cpos,findptr = FILESIZE_MAX,retval;
  char fbuff[MAX_SEARCH_SIZE*__MAX_SYMBOL_SIZE], nbuff[__MAX_SYMBOL_SIZE];
  unsigned proc,pproc,pmult,bio_opt=0,symb_size;
  int direct,icache[UCHAR_MAX+1];
  const int *cache;
  tBool cond;
  unsigned char __search_len;
  unsigned char ch,ch1;
  char cbuff[MAX_SEARCH_SIZE];
  symb_size = activeMode->get_symbol_size();
  /*
   * Cache initialization for adapted Boyer-Moore search algorithm
  */
  __found = False;
  orig_start = start;
  retval = 0;
  if(!scache)
  {
    fillBoyerMooreCache(icache, pattern, pattern_size, biewFlg & SF_CASESENS);
    cache = icache;
  }
  else cache = scache;
  flen = sfrom ? slen : BMGetFLength();
  endscan = biewFlg & SF_REVERSE ? 0 : flen;
  direct  = biewFlg & SF_REVERSE ? -1 : 1;
  tsize = flen;
  pmult = 100;
  if(tsize > FILESIZE_MAX/100) { tsize /= 100; pmult = 1; }
  cond = False;
  pproc = proc = 0;
  /* seek to the last character of pattern by direction */
  start += biewFlg & SF_REVERSE ? 0 : (pattern_size-1)*symb_size;
  if(!sfrom)
  {
    bio_opt = bioGetOptimization(BMbioHandle());
    bioSetOptimization(BMbioHandle(),
                      (bio_opt & (~BIO_OPT_DIRMASK)) |
                      (biewFlg & SF_REVERSE ? BIO_OPT_RBACKSCAN : BIO_OPT_RFORWARD));
  }
  start = (start/symb_size)*symb_size; /** align on symbol boundary */
  memcpy(cbuff,pattern,pattern_size);
  if(!(biewFlg & SF_CASESENS)) memupr((void *)cbuff,pattern_size);
  for(cpos = start;start != endscan;start = direct == 1 ? start + pattern_size*symb_size : start - (pattern_size*symb_size),cpos=start)
  {
    /* If search direction is forward then start point at the end of pattern */
    if(direct == 1 && start*symb_size > flen)
    {
      retval = FILESIZE_MAX;
      break;
    }
    /* If search direction is backward then start point at the begin of pattern */
    if(direct == -1 && start < (pattern_size*symb_size))
    {
      retval = FILESIZE_MAX;
      break;
    }
    proc = (unsigned)((cpos*pmult)/tsize);
    if(proc != pproc && !(flags & __LF_HIDEPRCNT))
    {
      if(!ShowPercentInWnd(prcntswnd,pproc=proc))  break;
    }
    if(sfrom)
      memcpy(nbuff,&sfrom[start],symb_size);
    else
      BMReadBufferEx(nbuff,symb_size,start,BM_SEEK_SET);
    if((activeMode->flags & __MF_TEXT) == __MF_TEXT) activeMode->convert_cp(nbuff,symb_size,False);
    ch = nbuff[0];
    if(!(biewFlg & SF_CASESENS)) ch = toupper(ch);
    if(cache[ch])
    {
      if(pattern_size > 1)
      {
        findptr = start-(cache[ch]-1)*symb_size;
        if((flags & __LF_NOSEEK) && findptr != orig_start) break;
        if(sfrom)
          memcpy(fbuff,&sfrom[findptr],pattern_size*symb_size);
        else
          BMReadBufferEx((void *)fbuff,pattern_size*symb_size,findptr,BM_SEEK_SET);
        if((activeMode->flags & __MF_TEXT) == __MF_TEXT)
             __search_len = activeMode->convert_cp((char *)fbuff,pattern_size*symb_size,False);
        else __search_len = pattern_size;
        if(!(biewFlg & SF_CASESENS)) memupr((void *)fbuff,__search_len);
        if(memcmp(fbuff,cbuff,__search_len) == 0) cond = True;
        else
        {
          if(flags & __LF_NOSEEK) break;
          else
          {
            start = direct == 1 ? start-(pattern_size-1)*symb_size :
                                  start+(pattern_size-1)*symb_size;
            continue;
          }
        }
      }
      else { findptr = start; cond = True; }
      if((biewFlg & SF_WORDONLY) && cond)
      {
        if(start && !(flags & __LF_NOLEFT))
        {
          if(sfrom)
            memcpy(nbuff,&sfrom[findptr-symb_size],symb_size);
          else
            BMReadBufferEx(nbuff,symb_size,findptr - symb_size,BM_SEEK_SET);
          if((activeMode->flags & __MF_TEXT) == __MF_TEXT) activeMode->convert_cp(nbuff,symb_size,False);
          ch = nbuff[0];
        }
        else      ch = ' ';
        if(start + pattern_size < flen && !(flags & __LF_NORIGHT))
        {
          if(sfrom)
            memcpy(nbuff,&sfrom[findptr + (pattern_size*symb_size)],symb_size);
          else
            BMReadBufferEx(nbuff,symb_size,findptr + (pattern_size*symb_size),BM_SEEK_SET);
          if((activeMode->flags & __MF_TEXT) == __MF_TEXT) activeMode->convert_cp(nbuff,symb_size,False);
          ch1 = nbuff[0];
        }
        else      ch1 = ' ';
        if(!(isseparate(ch) && isseparate(ch1))) cond = False;
      }
    }
    if(cond) { __found = True; retval = findptr; break; }
    if(flags & __LF_NOSEEK) break;
  }
  if(!sfrom) bioSetOptimization(BMbioHandle(),bio_opt);
  return retval;
}

static __filesize_t __NEAR__ __FASTCALL__  ___adv_find(const char *sfrom, 
                                                       unsigned sfromlen,
                                                       __filesize_t start,
                                                       __filesize_t *slen,
                                                       const int *scache,
                                                       const char *pattern,
                                                       unsigned pattern_size,
                                                       unsigned biewFlg)
{
  __filesize_t _found,found_st=FILESIZE_MAX,prev_found;
  __filesize_t stable_found;
  unsigned i, orig_i, last_search_len;
  unsigned orig_slen, t_count, flags;
  tBool is_tmpl, has_question;
  tBool always_prcnt;
  unsigned orig_direct;
  char cbuff[MAX_SEARCH_SIZE];
  char firstc;
  is_tmpl = (biewFlg & SF_WILDCARDS) == SF_WILDCARDS;
  *slen = pattern_size;
  if(!is_tmpl) return ___lfind(sfrom, sfromlen, sfrom ? __LF_HIDEPRCNT : __LF_NORMAL,start, scache, pattern, pattern_size, biewFlg);
  memcpy(cbuff, pattern, pattern_size);
  orig_slen = pattern_size;
  orig_direct = biewFlg & SF_REVERSE;
  _found = 0L;
  restart:
  always_prcnt = False;
  i = 0;
  prev_found = start;
  flags = __LF_NORMAL;
  has_question = False;
  stable_found = FILESIZE_MAX;
  while(1)
  {
    orig_i = i;
    for(;i < pattern_size;i++)
    {
      if(cbuff[i] == '*' || cbuff[i] == '?')
      {
        flags |= __LF_NORIGHT;
        break;
      }
    }
    pattern_size = i-orig_i;
    last_search_len = pattern_size;
    if(pattern_size)
    {
      memcpy(cbuff,&pattern[orig_i],pattern_size);
      if((flags & __LF_NOSEEK && !always_prcnt) || sfrom) flags |= __LF_HIDEPRCNT;      
      _found = ___lfind(sfrom, sfromlen, flags,start, NULL, cbuff, pattern_size, biewFlg);
    }
    else
    {
      if(!orig_i && !(biewFlg & SF_WORDONLY))
      {
        /* it means: first character is wildcard and it much better
           to restart search engine */
        firstc = cbuff[0];
        pattern_size = orig_slen;
        if(orig_direct & SF_REVERSE) biewFlg &= ~SF_REVERSE;
        else                         biewFlg |= SF_REVERSE;
        memmove(cbuff, &pattern[1], --pattern_size);
        found_st = ___adv_find(sfrom, sfromlen, start,slen, scache, cbuff, pattern_size, biewFlg);
        (*slen)++;
        if(__found)
        {
          switch(firstc)
          {
            case '?': if(found_st) found_st--; break;
            default:  (*slen)+= found_st; found_st = 0L; break;
          }
        }
        goto exit;
      }
      else
      {
        __found = True;
        _found = prev_found;
        always_prcnt = True;
      }
    }
    flags = __LF_NORMAL; /* reseting flags immediately after search */
    if(__found)
    {
       if(!orig_i) stable_found = _found;
       t_count = 0;
       if(found_st == FILESIZE_MAX) found_st = _found;
       if(orig_i)
        if(pattern[orig_i-1] == '?' &&
          prev_found+last_search_len+t_count != _found) /* special case for '?' */
             found_st = _found-orig_i;
       still:
       switch(cbuff[i])
       {
         case '?': while(cbuff[i] == '?') { t_count++; i++; }
                   flags = __LF_NOSEEK | __LF_NOLEFT;
                   has_question = True;
                   goto still;
         case '*': while(cbuff[i] == '*') i++;
                   pattern_size = orig_slen-i;
                   memmove(cbuff, &pattern[i], pattern_size);
                   biewFlg &= ~SF_REVERSE;
          	   found_st = ___adv_find(sfrom, sfromlen,_found+last_search_len,slen, scache, cbuff, pattern_size, biewFlg);
                   (*slen)++;
                   goto exit;
         default: break;
       }
       start=_found+last_search_len+t_count;
       biewFlg &= ~SF_REVERSE; /* Anyway: we have found a first subsequence.
                                 For searching with using template need
                                 only forward search technology. */
       pattern_size = orig_slen;
    }
    else
    {
      if(!has_question || _found == FILESIZE_MAX)
      {
        found_st = FILESIZE_MAX;
        break;
      }
      else /* restarting search engine */
      {
        if(found_st == FILESIZE_MAX) break;
        pattern_size = orig_slen;
        if(orig_direct & SF_REVERSE) biewFlg |= SF_REVERSE;
        else                         biewFlg &= SF_REVERSE;
        start = biewFlg & SF_REVERSE ? stable_found-1 : stable_found+1;
        memcpy(cbuff,pattern,pattern_size);
        found_st = FILESIZE_MAX;
        goto restart;
      }
    }
    if(i >= orig_slen) break;
  }
  if(found_st == FILESIZE_MAX) found_st = 0;
  /* Special case if last character is wildcard */
  if(cbuff[orig_slen-1] == '?') last_search_len++;
  *slen = _found+last_search_len-found_st;
  if(cbuff[orig_slen-1] == '*') (*slen) = FILESIZE_MAX - _found;
exit:
  pattern_size = orig_slen;
  if(orig_direct & SF_REVERSE) biewFlg &= ~SF_REVERSE;
  else                         biewFlg |= SF_REVERSE;
  return found_st;
}

#define __adv_find(start, slen) ___adv_find(NULL, 0, start, slen, NULL, search_buff, search_len, biewSearchFlg)

char * __FASTCALL__ strFind(const char *str, unsigned str_len,
                            const void *sbuff, unsigned sbuflen,
                            const int *cache, unsigned flg)
{
 __filesize_t slen;
 unsigned long lretval;
 lretval = ___adv_find(str, str_len, 0, &slen, cache, sbuff, sbuflen, flg & (~SF_REVERSE));
 return (char *)(__found ? &str[lretval] : 0);
}


int __FASTCALL__ ExpandHex(char * dest,const unsigned char * src,int size,char hard)
{
  int i,k;
    dest[0] = '\0';
    k = 0;
    for(i = 0;i < size;i++)
    {
        char * cptr;
        cptr = Get2Digit(src[i]);
        strcat(dest,cptr); k += 2;
        if(hard == 1) { strcat(dest,!((i + 1)%4) ? ( !((i+1)%16) ? " " : "-" ) : " "); k++; }
        else
          if(hard == 2) { strcat(dest,!((i + 1)%4) ? "  " : " ");  k += !((i + 1)%4) ? 2 : 1; }
          else { strcat(dest," "); k++; }
    }
  return k;
}

static void __NEAR__ __FASTCALL__ SearchPaint(TWindow *wdlg,int flags,
                                              unsigned sf_flags)
{
 TWindow *using = twUsedWin();
 twUseWin(wdlg);
 twSetColorAttr(dialog_cset.group.active);
 if(sf_flags & SF_ASHEX) twSetColorAttr(dialog_cset.group.disabled);
 else twSetColorAttr(dialog_cset.group.active);
 twGotoXY(4,4); twPutChar(GetBool(sf_flags & SF_CASESENS));
 twSetColorAttr(dialog_cset.group.active);
 twGotoXY(4,5); twPutChar(GetBool(sf_flags & SF_WORDONLY));
 twGotoXY(4,6); twPutChar(GetBool(sf_flags & SF_REVERSE));
 twGotoXY(46,4); twPutChar(GetBool(sf_flags & SF_ASHEX));
 twSetColorAttr((!(flags & SD_ALLFEATURES) || sf_flags & SF_ASHEX)?dialog_cset.group.disabled:dialog_cset.group.active); 
 twGotoXY(46,5); twPutChar(GetBool(sf_flags & SF_WILDCARDS));
 twSetColorAttr(!((flags & SD_ALLFEATURES) && activeMode->search_engine)?dialog_cset.group.disabled:dialog_cset.group.active); 
 twGotoXY(46,6); twPutChar(GetBool(sf_flags & SF_PLUGINS));
 twSetColorAttr(dialog_cset.main);
 twUseWin(using);
}

static void __NEAR__ __FASTCALL__ SearchUpdate(TWindow *wdlg,int _flags,
                                              unsigned sf_flags)
{
 TWindow *using = twUsedWin();
 twUseWin(wdlg);
 twSetColorAttr((sf_flags & SF_ASHEX)?dialog_cset.group.disabled:dialog_cset.group.active);
 twGotoXY(2,4); twPutS(msgFindOpt[0]);
 twSetColorAttr(dialog_cset.group.active);
 twGotoXY(2,5); twPutS(msgFindOpt[1]);
 twGotoXY(2,6); twPutS(msgFindOpt[2]);
 twGotoXY(44,4); twPutS(msgFindOpt2[0]);
 twSetColorAttr((!(_flags & SD_ALLFEATURES) || sf_flags & SF_ASHEX)?dialog_cset.group.disabled:dialog_cset.group.active); 
 twGotoXY(44,5); twPutS(msgFindOpt2[1]);
 twSetColorAttr(!((_flags & SD_ALLFEATURES) && activeMode->search_engine)?dialog_cset.group.disabled:dialog_cset.group.active); 
 twGotoXY(44,6); twPutS(msgFindOpt2[2]);
 twSetColorAttr(dialog_cset.main);
 twUseWin(using);
}

static const char * searchtxt[] =
{
  "Help  ",
  "CasSen",
  "WrdOnl",
  "ScanDr",
  "Hex   ",
  "Templt",
  "Plugin",
  "      ",
  "      ",
  "Escape"
};

static void drawSearchPrompt( void )
{
   __drawSinglePrompt(searchtxt);
}

tBool __FASTCALL__ SearchDialog(int _flags, char * searchbuff,
                                unsigned char *searchlen,
                                unsigned * sf_flags)
{
  TWindow *hwnd,* ewnd;
  tAbsCoord x1,y1,x2,y2;
  tRelCoord X1,Y1,X2,Y2;
  unsigned x[2] = { 0, 0 };
  int rret, active;
  tBool ret;
  int update;
  char attr[2] = { __ESS_FILLER_7BIT | __ESS_WANTRETURN | __ESS_ENABLEINSERT | __ESS_NON_C_STR,
                   __ESS_WANTRETURN | __ESS_ASHEX | __ESS_NON_C_STR };
  char ebuff1[MAX_SEARCH_SIZE],ebuff2[MAX_SEARCH_SIZE*3];
  char *ebuff[2],*legal[2];
  unsigned mlen[2],flags;
  int ch,i;
  hwnd = CrtDlgWndnls(FIND_STR,78,7);
  twSetFooterAttr(hwnd," [Enter] - Start ",TW_TMODE_CENTER,dialog_cset.footer);
  twGetWinPos(hwnd,&x1,&y1,&x2,&y2);
  X1 = x1+2;
  Y1 = y1+2;
  X2 = X1+75;
  Y2 = Y1;
  ewnd = CreateEditor(X1,Y1,X2,Y2,TWS_CURSORABLE | TWS_VISIBLE);
  twUseWin(hwnd);
  twinDrawFrameAttr(1,3,78,7,TW_DN3D_FRAME,dialog_cset.main);
  twinDrawFrameAttr(37,4,42,6,TW_UP3D_FRAME,dialog_cset.main);
  twGotoXY(38,5); twPutS("BASE");
  twGotoXY(2,1);  twPutS(TYPE_STR);
  SearchUpdate(hwnd,_flags,*sf_flags);
  legal[0] = NULL;
  legal[1] = &legalchars[2];
  ebuff1[0] = ebuff2[0] = '\0';
  ebuff[0] = ebuff1;
  ebuff[1] = ebuff2;
  if(searchlen)
  {
    memcpy(ebuff[0],searchbuff,*searchlen);
    ExpandHex(ebuff[1],(unsigned char *)searchbuff,*searchlen,0);
  }
  rret = 2;
  ret = True;
  SearchPaint(hwnd,_flags,*sf_flags);
  update = 1;
  while(1)
  {
    mlen[0] = MAX_SEARCH_SIZE;
    mlen[1] = MAX_SEARCH_SIZE*3;
    active = *sf_flags & SF_ASHEX ? 1 : 0;
    flags = attr[active];
    if(!update) flags |= __ESS_NOREDRAW;
    twUseWin(ewnd);
    ch = eeditstring(ebuff[active],legal[active],&mlen[active],
                     active ? (*searchlen)*3 : *searchlen,
                     &x[active],flags,NULL, drawSearchPrompt);
    update = 1;
    switch(ch)
    {
       case KE_ENTER    : if(searchlen) { rret = 1; ret = True; } else { rret = 0; ret = False; } break;
       case KE_F(10)    :
       case KE_ESCAPE   : rret = 0; ret = False; break;
       case KE_F(2)     : if(!(*sf_flags&SF_ASHEX)) *sf_flags ^= SF_CASESENS;
                          update = 0;
                          break;
       case KE_F(3)     : *sf_flags ^= SF_WORDONLY;
                          update = 0;
                          break;
       case KE_F(4)     : *sf_flags ^= SF_REVERSE;
                          update = 0;
                          break;
       case KE_F(1)     : hlpDisplay(7);
                          update = 0;
                          break;
       case KE_F(5)     : *sf_flags ^= SF_ASHEX;
                          update = 2;
                          break;
       case KE_F(6)     : if(!(*sf_flags&SF_ASHEX) && (_flags & SD_ALLFEATURES)) *sf_flags ^= SF_WILDCARDS;
                          update = 0;
                          break;
       case KE_F(7)     : if(_flags & SD_ALLFEATURES && activeMode->search_engine)
                          *sf_flags ^= SF_PLUGINS;
                          update = 0;
                          break;
       case KE_LEFTARROW:
       case KE_RIGHTARROW:
                          update = 0;
                          break;
       default : break;
    }
    if(rret != 2) break;
    twUseWin(hwnd);
    if(!active) { *searchlen = mlen[0]; memcpy(searchbuff,ebuff[0],mlen[0]); }
    else  { *searchlen = mlen[1] / 3; CompressHex((unsigned char *)searchbuff,ebuff[1],*searchlen,True); }
    if(searchlen) memcpy(ebuff[0],searchbuff,*searchlen);
    else     ebuff[0][0] = '\0';
    mlen[0] = *searchlen;
    ExpandHex(ebuff[1],(unsigned char *)searchbuff,*searchlen,0);
    mlen[1] = (*searchlen)*3;
    for(i = 0;i < 2;i++) if(x[i] > mlen[i]) x[i] = mlen[i];
    if(update>1) SearchUpdate(hwnd,_flags,*sf_flags);
    SearchPaint(hwnd,_flags,*sf_flags);
  }
  if(*sf_flags & SF_ASHEX)
  {
         *sf_flags &= ~(SF_WILDCARDS);
         *sf_flags |= SF_CASESENS;
  }
  CloseWnd(ewnd);
  CloseWnd(hwnd);
  return ret;
}

extern TWindow * ErrorWnd;

__filesize_t __FASTCALL__ Search( tBool is_continue )
{
  __filesize_t found;
  __filesize_t fmem,lmem,slen, flen;
  tBool ret;
  fmem = BMGetCurrFilePos();
  flen = BMGetFLength();
  ret = is_continue ? True :
        SearchDialog(SD_ALLFEATURES,(char *)search_buff,&search_len,&biewSearchFlg);
  if(ret && search_len)
  {
    prcntswnd = PercentWnd(PLEASE_WAIT,SEARCHING);
    lmem = fmem;
    if(FoundTextSt != FoundTextEnd)
    {
      unsigned cp_symb_size;
      if(is_continue) lmem = FoundTextSt;
      cp_symb_size = activeMode->get_symbol_size();
      if((biewSearchFlg & SF_REVERSE) && lmem) lmem-=cp_symb_size;
      else if(lmem < flen) lmem+=cp_symb_size;
    }
    __found = False;
    found = (biewSearchFlg & SF_PLUGINS) && activeMode->search_engine ?
       activeMode->search_engine(prcntswnd,lmem,&slen,biewSearchFlg,
                                 is_continue,&__found):
       __adv_find(lmem,&slen);
    CloseWnd(prcntswnd);
    if(__found)
    {
       FoundTextSt = found;
       FoundTextEnd = found + slen*activeMode->get_symbol_size();
       /* it is not an error of search engine it is special case:
          adv_find function don't want to use a file stream directly */
       if(FoundTextEnd > flen) FoundTextEnd = flen;

       return found;
    }
    else  ErrMessageBox(STR_NOT_FOUND,SEARCH_MSG);
  }
  BMSeek(fmem,BM_SEEK_SET);
  return fmem;
}
