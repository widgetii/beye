/**
 * @namespace   biewlib
 * @file        biewlib/file_ini.c
 * @brief       This file contains implementation of .ini files services.
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
 * @bug         Fault if more than one ini file is opened at one time
 * @todo        Reentrance ini library
**/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include "biewlib/bbio.h"
#include "biewlib/file_ini.h"
#include "biewlib/pmalloc.h"

#define rewind_ini(h) (bioSeek(h,0L,BIO_SEEK_SET))

static unsigned char CaseSens = 2; /**< 2 - case 1 - upper 0 - lower */
static FiUserFunc proc;
static pVar FirstVar = NULL;
FiHandler ActiveFile = 0;
tBool ifSmarting = True;
char *fi_Debug_Str = NULL;

/**************************************************************\
*                      Low level support                       *
\**************************************************************/


#define __C_EOF 0x1A
#define __FI_MAXFILES 200

unsigned int  *FinCurrString;
char ** FiFileNames;
unsigned int  FiFilePtr = 0;
char *  FiUserMessage = NULL;
const char iniLegalSet[] = " _0123456789"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz";

#define IS_VALID_NAME(name) (strspn(name,iniLegalSet) == strlen(name))
#define IS_SECT(str,ch) (str[0] == ch)
#define FiisSection( str ) (IS_SECT(str,'['))
#define FiisSubSection( str ) (IS_SECT(str,'<'))
#define FiisCommand( str ) (IS_SECT(str,'#'))
#define FiisItem(str) (!(FiisSection(str) || FiisSubSection(str) || FiisCommand(str)))

static char * __NEAR__ __FASTCALL__ GETS(char *str,unsigned num,BGLOBAL h)
{
  char *ret;
  unsigned i;
  char ch,ch1;
  ret = str;
  for(i = 0;i < num;i++)
  {
     ch = bioReadByte(h);
     if(ch == '\n' || ch == '\r')
     {
       *str = ch;  str++;
       ch1 = bioReadByte(h);
       if((ch1 == '\n' || ch1 == '\r') && ch != ch1)
       {
         *str = ch; str++;
       }
       else
       {
         if(bioEOF(h))
         {
           if((signed char)ch1 != -1 && ch1 != __C_EOF)
           {
             *str = ch1; str++;
           }
           break;
         }
         bioSeek(h,-1,SEEK_CUR);
       }
       break;
     }
     if(bioEOF(h))
     {
       if((signed char)ch != -1 && ch != __C_EOF)
       {
         *str = ch; str++;
       }
       break;
     }
     *str = ch; str++;
  }
  *str = 0;
  return ret;
}

void __FASTCALL__ FiAError(int nError,int row)
{
 int eret = 0;
 if(FiError) eret = (*FiError)(nError,row);
 if(eret == __FI_EXITPROC) exit(255);
}

void __FASTCALL__ FiAErrorCL(int nError) { FiAError(nError,FinCurrString[FiFilePtr-1]); }

static const char * list[] = {
 "No errors",
 "Can't open file (bad '#include' statement?).",
 "Too many open files.",
 "Memory exhausted.",
 "Open 'if' (missing '#endif').",
 "Missing 'if' for 'endif' statement.",
 "Missing 'if' for 'else' statement.",
 "Unknown '#' directive.",
 "Syntax error in 'if' statement.",
 "Expected open section or subsection, or invalid string.",
 "Bad character on line (possible lost comment).",
 "Bad variable in 'set' or 'delete' statement.",
 "Bad value of variable in 'set' statement.",
 "Unrecognized name of variable in 'delete' statement.",
 "Undefined variable detected (case sensivity?).",
 "Missing 'if' for 'elif' statement.",
 "Open variable on line (use even number of '%' characters).",
 "Lost or mismatch character '=' in assigned expression.",
 "",
 "User error."
};

const char * __FASTCALL__ FiDecodeError(int nError)
{
 const char *ret;
 nError = abs(nError);
 if(nError >= 0 && nError <= abs(__FI_FIUSER)) ret = list[nError];
 else ret = "Unknown Error";
 return ret;
}

static int __FASTCALL__ StdError(int ne,int row)
{
    FILE * herr;
    const char * what;
    if((herr = fopen("fi_syserr.$$$","wt")) == NULL) herr = stderr;
    fprintf(herr,"About : [.Ini] file run-time support library. Written by N.Kurshev\n"
                 "Detected ");
    if(ne != __FI_TOOMANY && FiFilePtr)
    {
      fprintf(herr,"%s error in : %s",row ? "fatal" : "",FiFileNames[FiFilePtr - 1]);
    }
    fprintf(herr,"\n");
    if(row)  fprintf(herr,"At line : %i\n",row);
    what = FiDecodeError(ne);
    fprintf(herr,"Message : %s\n",what);
    if(FiUserMessage) fprintf(herr,"User message : %s\n",FiUserMessage);
    if(fi_Debug_Str) if(*fi_Debug_Str) fprintf(herr,"Debug info: '%s'\n",fi_Debug_Str);
    fclose(herr);
    printm("\nError in .ini file.\nFile fi_syser.$$$ created.\n");
    return __FI_EXITPROC;
}

int (__FASTCALL__ *FiError)(int nError,int row) = StdError;

FiHandler __FASTCALL__ FiOpen( const char * filename)
{
  char * activeFile;
  FiHandler ret;
  /* Try to load .ini file entire into memory */
  ret = bioOpen(filename,FO_READONLY | SO_DENYWRITE,UINT_MAX,BIO_OPT_USEMMF);
  if(ret == &bNull )
  {
    FiAError(__FI_BADFILENAME,0);
  }
  activeFile = (char *)PMalloc((strlen(filename) + 1));
  if(activeFile == NULL) FiAError(__FI_NOTMEM,0);
  strcpy(activeFile,filename);
  if(!FiFilePtr)
  {
    FiFileNames = PMalloc(sizeof(char *));
    FinCurrString = PMalloc(sizeof(unsigned int));
  }
  else
  {
    FiFileNames = PRealloc(FiFileNames,sizeof(char *)*(FiFilePtr+1));
    FinCurrString = PRealloc(FinCurrString,sizeof(unsigned int)*(FiFilePtr+1));
  }
  if(!FiFileNames || !FinCurrString) FiAError(__FI_NOTMEM,0);
  FiFileNames[FiFilePtr] = activeFile;
  FinCurrString[FiFilePtr++] = 0;
  if(FiFilePtr > __FI_MAXFILES-1) FiAError(__FI_TOOMANY,0);
  return ret;
}

void __FASTCALL__ FiClose(FiHandler h)
{
  PFREE(FiFileNames[FiFilePtr-1]);
  if(FiFilePtr)
  {
    FiFileNames = PRealloc(FiFileNames,sizeof(char *)*(FiFilePtr));
    FinCurrString = PRealloc(FinCurrString,sizeof(unsigned int)*(FiFilePtr));
    if(!FiFileNames || !FinCurrString) FiAError(__FI_NOTMEM,0);
    FiFilePtr--;
  }
  else
  {
    PFREE(FiFileNames);
    PFREE(FinCurrString);
  }
  bioClose(h);
}

static unsigned int __NEAR__ __FASTCALL__ __GetLengthBrStr(const char * src,char obr,char cbr)
{
 char *ends;
 unsigned ret = 0;
 if(*src == obr)
 {
   src++;
   ends = strchr(src,cbr);
   if(!ends) goto err;
   if(*(ends+1)) FiAErrorCL(__FI_BADCHAR);
   ret = ends-src;
 }
 else
 {
   err:
   FiAErrorCL(__FI_OPENSECT);
 }
 return ret;
}

static char * __NEAR__ __FASTCALL__ __GetBrStrName(const char * src,char * store,char obr,char cbr)
{
 char *ends;
 if(*src == obr)
 {
   unsigned len;
   src++;
   ends = strchr(src,cbr);
   if(!ends) goto err;
   if(*(ends+1)) FiAErrorCL(__FI_BADCHAR);
   len = ends-src;
   memcpy(store,src,len);
   store[len] = 0;
 }
 else
 {
   err:
   FiAErrorCL(__FI_OPENSECT);
 }
 return store;
}

#define FiGetLengthBracketString( str ) (__GetLengthBrStr(str,'"','"'))
#define FiGetBracketString( str, store) (__GetBrStrName(str,store,'"','"'))
#define FiGetSectionName( src, store) (__GetBrStrName(src,store,'[',']'))
#define FiGetSubSectionName( src, store) (__GetBrStrName(src,store,'<','>'))
#define FiGetLengthSection( src ) (__GetLengthBrStr(src,'[',']'))
#define FiGetLengthSubSection( src ) (__GetLengthBrStr(src,'<','>'))

unsigned int  __FASTCALL__ FiGetLengthItem( const char * src )
{
  char *sret;
  sret = strchr(src,'=');
  if(!sret) FiAErrorCL(__FI_NOTEQU);
  return sret - src;
}

unsigned int  __FASTCALL__ FiGetLengthValue( const char * src )
{
  char *sptr;
  unsigned len;
  len = strlen(src);
  sptr = strchr(src,'=');
  if(!sptr) FiAErrorCL(__FI_NOTEQU);
  return len-(sptr-src+1);
}

char *    __FASTCALL__ FiGetValueOfItem(const char * src,char * store)
{
  char *from;
  from = strchr(src,'=');
  if(from) strcpy(store,++from);
  else     FiAErrorCL(__FI_NOTEQU);
  return store;
}

unsigned int  __FASTCALL__ FiGetLengthCommandString( const char * src )
{
   unsigned i;
   i = strspn(src," #");
   return strlen(src) - i;
}

char *    __FASTCALL__ FiGetItemName(const char * src,char * store)
{
  char *sptr;
  unsigned len;
  sptr = strchr(src,'=');
  if(sptr)
  {
    len = sptr-src;
    memcpy(store,src,len);
    store[len] = 0;
    if(!IS_VALID_NAME(store)) FiAErrorCL(__FI_BADCHAR);
  }
  return store;
}

char *   __FASTCALL__ FiGetCommandString(const char * src,char * store)
{
  unsigned i;
  i = strspn(src," #");
  strcpy(store,&src[i]);
  return store;
}

unsigned int  __FASTCALL__ FiGetLengthNextWord( STRING * str, const char * illegal_symbols)
{
 unsigned int j,i;
 i = strspn(&str->str[str->iptr],illegal_symbols);
 j = strcspn(&str->str[i+str->iptr],illegal_symbols);
 return j;
}

char * __FASTCALL__ FiGetNextWord( STRING * str,const char * illegal_symbols,char * store)
{
 unsigned int j;
 str->iptr += strspn(&str->str[str->iptr],illegal_symbols);
 j = strcspn(&str->str[str->iptr],illegal_symbols);
 memcpy(store,&str->str[str->iptr],j);
 store[j] = 0;
 str->iptr += j;
 return store;
}

static unsigned int  __FASTCALL__ FiGetLengthNextLegWord( STRING * str, const char * legal_symbols)
{
 unsigned int j,i;
 i = strcspn(&str->str[str->iptr],legal_symbols);
 j = strspn(&str->str[i+str->iptr],legal_symbols);
 return j;
}

static char * __FASTCALL__ FiGetNextLegWord( STRING * str,const char * legal_symbols,char * store)
{
 unsigned int j;
 str->iptr += strcspn(&str->str[str->iptr],legal_symbols);
 j = strspn(&str->str[str->iptr],legal_symbols);
 memcpy(store,&str->str[str->iptr],j);
 store[j] = 0;
 str->iptr += j;
 return store;
}
/************** BEGIN of Construct section ********************/

/*
   These functions protect library from stack overflow in 16-bit mode,
   because #include statement does recursion.
*/

static char * __NEAR__ __FASTCALL__ __FiCMaxStr( void )
{
  char * ret;
  ret = (char *)PMalloc(FI_MAXSTRLEN + 1);
  if(ret == NULL) FiAError(__FI_NOTMEM,0);
  return ret;
}

static char * __NEAR__ __FASTCALL__ __FiCNWord( STRING *str , const char * illegal_set)
{
  char * ret;
  unsigned int lword;
  lword = FiGetLengthNextWord(str,illegal_set);
  ret = (char *)PMalloc(lword + 1);
  if(ret == NULL) FiAError(__FI_NOTMEM,0);
  if(lword) FiGetNextWord(str,illegal_set,ret);
  else      ret[0] = 0;
  return ret;
}

static char * __NEAR__ __FASTCALL__ __FiCNLegWord( STRING *str , const char * legal_set)
{
  char * ret;
  unsigned int lword;
  lword = FiGetLengthNextLegWord(str,legal_set);
  ret = (char *)PMalloc(lword + 1);
  if(ret == NULL) FiAError(__FI_NOTMEM,0);
  if(lword) FiGetNextLegWord(str,legal_set,ret);
  else      ret[0] = 0;
  return ret;
}

static char * __NEAR__ __FASTCALL__ __FiCBString( const char * src )
{
  char * ret;
  unsigned int lbr;
  lbr = FiGetLengthBracketString(src);
  ret = (char *)PMalloc(lbr + 1);
  if(ret == NULL) FiAError(__FI_NOTMEM,0);
  if(lbr) FiGetBracketString(src,ret);
  else    ret[0] = 0;
  return ret;
}

static char * __NEAR__ __FASTCALL__ __FiCItem( const char * src )
{
  char * ret;
  unsigned int li;
  li = FiGetLengthItem(src);
  ret = (char *)PMalloc(li + 1);
  if(ret == NULL) FiAError(__FI_NOTMEM,0);
  if(li) FiGetItemName(src,ret);
  else   ret[0] = 0;
  return ret;
}

static char * __NEAR__ __FASTCALL__ __FiCValue( const char * src )
{
  char * ret;
  unsigned int lv;
  lv = FiGetLengthValue(src);
  ret = (char *)PMalloc(lv + 1);
  if(ret == NULL) FiAError(__FI_NOTMEM,0);
  if(lv) FiGetValueOfItem(src,ret);
  else   ret[0] = 0;
  return ret;
}

static char * __NEAR__ __FASTCALL__ __FiCSection( const char * src )
{
  char * ret;
  unsigned int ls;
  ls = FiGetLengthSection(src);
  ret = (char *)PMalloc(ls + 1);
  if(ret == NULL) FiAError(__FI_NOTMEM,0);
  if(ls) FiGetSectionName(src,ret);
  else   ret[0] = 0;
  return ret;
}

static char * __NEAR__ __FASTCALL__ __FiCSubSection( const char * src )
{
  char * ret;
  unsigned int lss;
  lss = FiGetLengthSubSection(src);
  ret = (char *)PMalloc(lss + 1);
  if(ret == NULL) FiAError(__FI_NOTMEM,0);
  if(lss) FiGetSubSectionName(src,ret);
  else    ret[0] = 0;
  return ret;
}

static char * __NEAR__ __FASTCALL__ __FiCCmd( const char * src )
{
  char * ret;
  unsigned int lc;
  lc = FiGetLengthCommandString(src);
  ret = (char *)PMalloc(lc + 1);
  if(ret == NULL) FiAError(__FI_NOTMEM,0);
  if(lc) FiGetCommandString(src,ret);
  else   ret[0] = 0;
  return ret;
}
/************* END of Construct section *********************/
#if 0 /* Uncomment it later */
void __FASTCALL__ FiSeekTo( FiHandler h, unsigned int nSection, unsigned int nSubSection, unsigned int nItem )
{
  unsigned int ns = 0,nss = 0,ni = 0;
  bioSeek(h,0L,SEEKF_START);
  FinCurrString[FiFilePtr - 1] = 0;
  if(nSection)
  {
    char * buff;
    buff = __FiCMaxStr();
    while(!bioEOF(h))
    {
      FiGetNextString(h,buff,FI_MAXSTRLEN,NULL);
      if(FiisSection(buff)) ns++;
      if(ns < nSection) continue;
    }
    PFREE(buff);
  }
  if(nSubSection)
  {
   char * buff;
   buff = __FiCMaxStr();
   while(!bioEOF(h))
   {
     FiGetNextString(h,buff,FI_MAXSTRLEN,NULL);
     if(FiisSubSection(buff)) nss++;
     if(nss < nSubSection) continue;
   }
   PFREE(buff);
  }
  if(nItem)
  {
   char * buff;
   buff = __FiCMaxStr();
   while(!bioEOF(h))
   {
     FiGetNextString(h,buff,FI_MAXSTRLEN,NULL);
     if(FiisItem(buff)) ni++;
     if(nss < nItem) continue;
   }
   PFREE(buff);
  }
}

int __FASTCALL__ FiSearch(FiHandler h, const char * str )
{
  char *tmp,*buff;
  tmp  = __FiCMaxStr();
  buff = __FiCMaxStr();
  bioSeek(h,0L,SEEKF_START);
  FinCurrString[FiFilePtr - 1] = 0;
  while(!bioEOF(h))
  {
   FiGetNextString(h,buff,FI_MAXSTRLEN,NULL);
   if(FiisSection(buff))
   {
     FiGetSectionName(buff,tmp);
     if(strcmp(tmp,str) == 0) { PFREE(tmp); PFREE(buff); return __FI_SECTION;}
   }
   if(FiisSubSection(buff))
   {
     FiGetSubSectionName(buff,tmp);
     if(strcmp(tmp,str) == 0) { PFREE(buff); PFREE(tmp); return __FI_SUBSECTION; }
   }
   if(FiisItem(buff))
   {
     FiGetItemName(buff,tmp);
     if(strcmp(tmp,str) == 0) { PFREE(tmp); PFREE(buff); return __FI_ITEM; }
   }
  }
  bioSeek(h,0L,SEEKF_START);
  FinCurrString[FiFilePtr - 1] = 0;
  PFREE(tmp);
  PFREE(buff);
  return __FI_NOTFOUND;
}

unsigned int  __FASTCALL__ FiGetNumberOfSections( FiHandler h)
{
 long curr;
 unsigned int eret;
 unsigned int save;
 char * buff;
 eret = 0;
 save = FinCurrString[FiFilePtr -1];
 curr = bioTell(h);
 buff = __FiCMaxStr();
 bioSeek(h,0L,SEEKF_START);
 while(!bioEOF(h))
 {
   FiGetNextString(h,buff,FI_MAXSTRLEN,NULL);
   if(FiisSection(buff)) eret++;
 }
 bioSeek(h,curr,SEEKF_START);
 FinCurrString[FiFilePtr - 1] = save;
 PFREE(buff);
 return eret;
}

unsigned int __FASTCALL__ FiGetTotalNumberOfSubSections( FiHandler h )
{
 long curr;
 unsigned int eret;
 unsigned int save;
 char * buff;
 eret = 0;
 curr = bioTell(h);
 buff = __FiCMaxStr();
 save = FinCurrString[FiFilePtr - 1];
 bioSeek(h,0L,SEEKF_START);
 while(!bioEOF(h))
 {
   FiGetNextString(h,buff,FI_MAXSTRLEN,NULL);
   if(FiisSubSection(buff)) eret++;
 }
 bioSeek(h,curr,SEEKF_START);
 FinCurrString[FiFilePtr - 1] = save;
 PFREE(buff);
 return eret;
}

unsigned int __FASTCALL__ FiGetLocalNumberOfSubSections( FiHandler h, unsigned unsigned int nSection )
{
  long curr;
  unsigned int eret;
  unsigned int save;
  char * buff;
  save = FinCurrString[FiFilePtr - 1];
  curr = bioTell(h);
  eret = 0;
  buff = __FiCMaxStr();
  FiSeekTo(h,nSection,0,0);
  while(!bioEOF(h))
  {
    FiGetNextString(h,buff,FI_MAXSTRLEN,NULL);
    if(FiisItem(buff)) eret++;
  }
  bioSeek(h,curr,SEEKF_START);
  FinCurrString[FiFilePtr - 1] = save;
  PFREE(buff);
  return eret;
}

unsigned int  __FASTCALL__ FiGetTotalNumberOfItems( FiHandler h)
{
 long curr;
 unsigned int eret;
 unsigned int save;
 char * buff;
 eret = 0;
 curr = bioTell(h);
 save = FinCurrString[FiFilePtr - 1];
 buff = __FiCMaxStr();
 bioSeek(h,0L,SEEKF_START);
 while(!bioEOF(h))
 {
   FiGetNextString(h,buff,FI_MAXSTRLEN,NULL);
   if(FiisItem(buff)) eret++;
 }
 bioSeek(h,curr,SEEKF_START);
 FinCurrString[FiFilePtr - 1] = save;
 PFREE(buff);
 return eret;
}

unsigned int  __FASTCALL__ FiGetLocalNumberOfItems( FiHandler h,int nSection, int nSubSection)
{
  long curr;
  unsigned int eret;
  unsigned int save;
  char * buff;
  save = FinCurrString[FiFilePtr - 1];
  curr = bioTell(h);
  eret = 0;
  buff = __FiCMaxStr();
  FiSeekTo(h,nSection,nSubSection,0);
  while(!bioEOF(h))
  {
    FiGetNextString(h,buff,FI_MAXSTRLEN,NULL);
    if(FiisItem(buff)) eret++;
  }
  bioSeek(h,curr,SEEKF_START);
  FinCurrString[FiFilePtr - 1] = save;
  PFREE(buff);
  return eret;
}
#endif

#define FiOpenComment ';'
tBool  FiAllWantInput = False;

char * __FASTCALL__ FiGetNextString(FiHandler h, char * str,unsigned int size,char *original)
{
  unsigned char *sret;
  unsigned len;
  unsigned char ch;
  str[0] = 0;
  while(!bioEOF(h))
  {
    str[0] = 0;
    sret = GETS(str,size,h);
    len = strlen(str);
    while(len)
    {
      ch = str[len-1];
      if(ch == '\n' || ch == '\r') str[--len] = 0;
      else break;
    }
    if(original) strcpy(original,str);
    FinCurrString[FiFilePtr - 1]++;
    if((sret == NULL && !bioEOF( h )))  FiAError(__FI_BADFILENAME,0);
    sret = strchr(str,FiOpenComment);
    if(sret) *sret = 0;
    if(!FiAllWantInput)
    {
      szTrimTrailingSpace(str);
      szTrimLeadingSpace(str);
      /* kill all spaces around punctuations */
      /*
         Correct me: spaces will be sqweezed even inside " " brackets.
         But it is language dependent.
         For .ini files it is not significant.
      */
      sret = str;
      while((ch=*sret) != 0)
      {
        if(ispunct(ch)) sret = szKillSpaceAround(str,sret);
        sret++;
      }
      len = strlen(str);
      sret = str;
      while((ch=*sret) != 0)
      {
        if(isspace(ch))
        {
          if(isspace(*(sret+1))) sret = szKillSpaceAround(str,sret);
        }
        sret++;
      }
      if(strlen(str))  break; /* loop while comment */
    }
  }
  return str;
}


/*************************************************************\
*                  Middle level support                       *
\*************************************************************/


/************* BEGIN of List Var section ********************/

pVar __FASTCALL__ FiConstructVar(const char *v,const char *a)
{
 char * str;
 pVar vv;
 vv = (pVar)PMalloc(sizeof(Var));
 if(vv == NULL) FiAError(__FI_NOTMEM,0);
 vv->next = NULL;
 vv->prev = NULL;
 str = (char *)PMalloc(strlen(v) + 1);
 if(str == NULL) FiAError(__FI_NOTMEM,0);
 strcpy(str,v);
 vv->variables = str;
 str = (char *)PMalloc(strlen(a) + 1);
 if(str == NULL) FiAError(__FI_NOTMEM,0);
 strcpy(str,a);
 vv->associate = str;
 return vv;
}

void __FASTCALL__ FiDeleteVar(pVar pp)
{
  PFREE(pp->variables);
  PFREE(pp->associate);
  PFREE(pp);
}

void __FASTCALL__ FiDeleteAllVar( void )
{
  pVar iter;
  if(FirstVar)
  {
    iter = FirstVar;
    while( iter!= 0 )
    {
      pVar temp;
      temp = iter->next;
      FiDeleteVar(iter);
      iter = temp;
    }
    FirstVar = 0;
  }
}

const char * __FASTCALL__ FiExpandVariables(const char * var)
{
 char *ret = 0;
 pVar iter;
 iter = FirstVar;
 if(FirstVar != NULL)
 {
  for(;;)
  {
   if(strcmp(var,iter->variables) == 0)
   {
     ret = iter->associate;
     break;
   }
   if(iter->next == NULL) FiAErrorCL(__FI_NODEFVAR);
   iter = iter->next;
  }
 }
 return ret;
}

tBool __FASTCALL__ FiExpandAllVar(const char * value,char * store)
{
  tBool ret;
  if(ifSmarting && strchr(value,'%'))
  {
    char tmp[FI_MAXSTRLEN+1];
    char isVar,npercent;
    unsigned char buffer_ptr,tmp_ptr;
    unsigned int i,slen;
    npercent = 0;
    buffer_ptr = tmp_ptr = isVar = 0;
    slen = strlen(value);
    for(i = 0;i < slen;i++)
    {
        char c;
        c = value[i];
        if(c == '%')
        {
          npercent++;
          isVar = !isVar;
          if(!isVar)
          {
            tmp[tmp_ptr] = '\0';
            strcpy(&store[buffer_ptr],FiExpandVariables(tmp));
            buffer_ptr = strlen(store);
            tmp_ptr = 0;
          }
        }
        else
        {
          if(isVar)  tmp[tmp_ptr++] = c;
          else store[buffer_ptr++] = c;
        }
    }
    store[buffer_ptr] = '\0';
    if( npercent%2 ) FiAErrorCL(__FI_OPENVAR);
    ret = True;
  }
  else
  {
    strcpy(store,value);
    ret = False;
  }
  return ret;
}

void __FASTCALL__ FiAddVariables(const char * var,const char * associate)
{
 pVar iter,prev;
 if(!FirstVar)
 {
    FirstVar = FiConstructVar(var,associate);
    FirstVar->next = NULL;
    FirstVar->prev = NULL;
 }
 else
 {
  iter = FirstVar;
  for(;;)
  {
   if(strcmp(iter->variables,var) == 0)
   {
     PFREE(iter->associate);
     iter->associate = PMalloc(strlen(associate) + 1);
     if(iter->associate == NULL) FiAError(__FI_NOTMEM,0);
     strcpy(iter->associate,associate);
     return;
   }
   if(iter->next == NULL) break;
   iter = iter->next;
  }
  prev = FiConstructVar(var,associate);
  iter->next = prev;
  prev->prev = iter;
 }
}

void __FASTCALL__ FiRemoveVariables(const char * var)
{
  pVar iter,p;
  if(FirstVar == NULL) return;
  iter = FirstVar;
  for(;;)
  {
    if(strcmp(var,iter->variables) == 0)
    {
      if(iter != FirstVar)
      {
        p = iter->prev;
        p->next = iter->next;
        FiDeleteVar(iter);
        return;
      }
      else
      {
        p = iter->next;
        FiDeleteVar(iter);
        FirstVar = p;
        return;
      }
    }
    if(iter->next == NULL) break;
    iter = iter->next;
  }
  FiAErrorCL(__FI_NOVAR);
}

/*************** END of List Var Section ***************/

tBool __FASTCALL__ FiGetConditionStd( const char *condstr)
{
  char * var,*user_ass;
  char * real_ass;
  const char *rvar;
  STRING str;
  tBool ret;
  char cond[3];

  str.iptr = 0;
  str.str = condstr;
  user_ass = __FiCMaxStr();
  real_ass = __FiCMaxStr();

  var = __FiCNWord(&str," !=");
  FiExpandAllVar(var,real_ass);
  PFREE(var);
  rvar = FiExpandVariables(real_ass);
  cond[0] = str.str[str.iptr++];
  cond[1] = str.str[str.iptr++];
  cond[2] = '\0';

  var = __FiCNWord(&str," ");
  FiExpandAllVar(var,user_ass);
  PFREE(var);
  if(str.str[str.iptr]) FiAErrorCL(__FI_BADCHAR);
  ret = False;
  if(strcmp(cond,"==") == 0)  ret = (strcmp(user_ass,rvar) == 0);
  else
   if(strcmp(cond,"!=") == 0)  ret = (strcmp(user_ass,rvar) != 0);
   else
     FiAErrorCL(__FI_BADCOND);
  PFREE(user_ass);
  PFREE(real_ass);
  return ret;
}

tBool __FASTCALL__ FiCommandProcessorStd( const char * cmd )
{
 char *word,*a,*v;
 char *fdeb_save;
 STRING str;
 static tBool cond_ret = True;
 str.iptr = 0;
 str.str = cmd;
 word = __FiCNLegWord(&str,&iniLegalSet[1]);
 strlwr(word);
 if(strcmp(word,"include") == 0)
 {
   char * bracket;
   char _v[FI_MAXSTRLEN+1];
   char pfile[FILENAME_MAX+1],*pfp,*pfp2;
   bracket = __FiCBString(&str.str[str.iptr]);
   FiExpandAllVar(bracket,_v);
   fdeb_save = fi_Debug_Str;
   /* make path if no path specified */
   strcpy(pfile,FiFileNames[FiFilePtr-1]);
   pfp=strrchr(pfile,'\\');
   pfp2=strrchr(pfile,'/');
   pfp=max(pfp,pfp2);
   if(pfp && !(strchr(_v,'\\') || strchr(_v,'/'))) strcpy(pfp+1,_v);
   else    strcpy(pfile,_v);
   (*FiFileProcessor)(pfile);
   fi_Debug_Str = fdeb_save;
   PFREE(bracket);
   goto Exit_CP;
 }
 else
 if(strcmp(word,"set") == 0)
 {
   v = __FiCNLegWord(&str,&iniLegalSet[1]);
   if(str.str[str.iptr] != '=') FiAErrorCL(__FI_NOTEQU);
   str.iptr++;
   a = __FiCNWord(&str," ");
   if(v[0] == '\0' || v == NULL ) FiAErrorCL(__FI_BADVAR);
   if(a[0] == '\0' || a == NULL ) FiAErrorCL(__FI_BADVAL);
   FiAddVariables(v,a);
   PFREE(a);
   PFREE(v);
   if(str.str[str.iptr]) FiAErrorCL(__FI_BADCHAR);
   goto Exit_CP;
 }
 else
 if(strcmp(word,"delete") == 0)
 {
   v = __FiCNLegWord(&str,&iniLegalSet[1]);
   a = __FiCMaxStr();
   FiExpandAllVar(v,a);
   if(a[0] == '\0' || a == NULL) FiAErrorCL(__FI_BADVAR);
   FiRemoveVariables(a);
   PFREE(v);
   PFREE(a);
   if(str.str[str.iptr]) FiAErrorCL(__FI_BADCHAR);
   goto Exit_CP;
 }
 else
 if(strcmp(word,"reset") == 0)
 {
   if(str.str[str.iptr]) FiAErrorCL(__FI_BADCHAR);
   FiDeleteAllVar();
   goto Exit_CP;
 }
 else
 if(strcmp(word,"case") == 0)
 {
   if(str.str[str.iptr]) FiAErrorCL(__FI_BADCHAR);
   CaseSens = 2;
   goto Exit_CP;
 }
 else
 if(strcmp(word,"smart") == 0)
 {
   if(str.str[str.iptr]) FiAErrorCL(__FI_BADCHAR);
   ifSmarting = True;
   goto Exit_CP;
 }
 else
 if(strcmp(word,"nosmart") == 0)
 {
   if(str.str[str.iptr]) FiAErrorCL(__FI_BADCHAR);
   ifSmarting = False;
   goto Exit_CP;
 }
 else
 if(strcmp(word,"uppercase") == 0)
 {
   if(str.str[str.iptr]) FiAErrorCL(__FI_BADCHAR);
   CaseSens = 1;
   goto Exit_CP;
 }
 else
 if(strcmp(word,"lowercase") == 0)
 {
   if(str.str[str.iptr]) FiAErrorCL(__FI_BADCHAR);
   CaseSens = 0;
   goto Exit_CP;
 }
 else
 if(strcmp(word,"error") == 0)
 {
   a = __FiCMaxStr();
   FiExpandAllVar(&str.str[str.iptr+1],a);
   FiUserMessage = a;
   FiAErrorCL(__FI_FIUSER);
 }
 else
 if(strcmp(word,"else") == 0) FiAErrorCL(__FI_ELSESTAT);
 else
 if(strcmp(word,"endif") == 0) FiAErrorCL(__FI_IFNOTFOUND);
 else
 if(strcmp(word,"elif") == 0) FiAErrorCL(__FI_ELIFSTAT);
 else
 if(strcmp(word,"if") == 0)
 {
   char *sstore;
   unsigned int nsave;
   tBool Condition,BeenElse;
   int nLabel;
   sstore = __FiCMaxStr();
   nsave = FinCurrString[FiFilePtr - 1];
   cond_ret = Condition = (*FiGetCondition)(&cmd[str.iptr]);
   nLabel = 1;
   BeenElse = False;
   while(!bioEOF(ActiveFile))
   {
     FiGetNextString(ActiveFile,sstore,FI_MAXSTRLEN,NULL);
     if(sstore[0] == '\0') { PFREE(sstore); goto Exit_CP; }
     if(FiisCommand(sstore))
     {
       STRING cStr;
       a = __FiCCmd(sstore);
       if(CaseSens == 1) strupr(a);
       if(CaseSens == 0) strlwr(a);
       cStr.iptr = 0;
       cStr.str = a;
       v = __FiCNWord(&cStr," ");
       strlwr(v);
       if(strcmp(v,"if") == 0 && !Condition) nLabel++;
       if(strcmp(v,"endif") == 0 )
       {
         nLabel--;
         if(nLabel == 0)
         {
           PFREE(sstore);
           PFREE(a);
           PFREE(v);
           goto Exit_CP;
         }
         if(nLabel <  0) FiAErrorCL(__FI_IFNOTFOUND);
       }
       if(strcmp(v,"else") == 0 && nLabel == 1)
       {
           if( BeenElse ) FiAErrorCL(__FI_ELSESTAT);
           if( nLabel == 1 ) cond_ret = Condition = (Condition ? False : True);
           if( nLabel == 1 ) BeenElse = True;
           PFREE(v);
           PFREE(a);
           continue;
       }
       if(strcmp(v,"elif") == 0 && nLabel == 1)
       {
           if( BeenElse ) FiAErrorCL(__FI_ELIFSTAT);
           if( nLabel == 1 ) cond_ret = Condition = (*FiGetCondition)(&a[cStr.iptr]);
           if( nLabel == 1 ) BeenElse = True;
           PFREE(v);
           PFREE(a);
           continue;
       }
       if(Condition) (*FiCommandProcessor)(a);
       PFREE(v);
       PFREE(a);
     }
     else { if(Condition) (*FiStringProcessor)(sstore); }
   }
   PFREE(sstore)
   FiAError(__FI_OPENCOND,nsave);
 }
 FiAErrorCL(__FI_UNRECOGN);
 Exit_CP:
 PFREE(word);
 return cond_ret;
}

static  char * curr_sect = NULL;
static  char * curr_subsect = NULL;

tBool __FASTCALL__ FiStringProcessorStd(char * curr_str)
{
  char *item,*val;
    if(FiisCommand(curr_str))
    {
      item = __FiCCmd(curr_str);
      (*FiCommandProcessor)(item);
      PFREE(item);
      return False;
    }
    else
    if(FiisSection(curr_str))
    {
      if(curr_sect) PFREE(curr_sect);
      if(curr_subsect) PFREE(curr_subsect);
      curr_sect = __FiCSection(curr_str);
      return False;
    }
    else
    if(FiisSubSection(curr_str))
    {
      if(curr_subsect) PFREE(curr_subsect);
      curr_subsect = __FiCSubSection(curr_str);
      return False;
    }
    else
    if(FiisItem(curr_str))
    {
      char buffer[FI_MAXSTRLEN+1];
      tBool retval;
      IniInfo info;
      FiExpandAllVar(curr_str,buffer);
      item = __FiCItem(buffer);
      retval = False;
      if(item[0])
      {
       val = __FiCValue(buffer);
       if(curr_sect) info.section = curr_sect;
       else info.section = "";
       if(curr_subsect) info.subsection = curr_subsect;
       else info.subsection = "";
       info.item = item;
       info.value = val;
       retval = (*proc)(&info);
       PFREE(val);
      }
      else FiAErrorCL(__FI_BADCHAR);
      PFREE(item);
      return retval;
    }
  return False;
}

void __FASTCALL__ FiFileProcessorStd(const char * filename)
{
  char * work_str, * ondelete;
  FiHandler h;
  FiHandler oldh;
  tBool done;
  oldh = ActiveFile;
  h = FiOpen(filename);
  ActiveFile = h;
  work_str = __FiCMaxStr();
  fi_Debug_Str = ondelete = work_str;
  while(!bioEOF(h))
  {
      FiGetNextString(h,work_str,FI_MAXSTRLEN,NULL);
      if(!work_str || !work_str[0]) break;
      if(CaseSens == 1) strupr(work_str);
      if(CaseSens == 0) strlwr(work_str);
      done = (*FiStringProcessor)(work_str);
      if(done) break;
  }
  FiClose(h);
  if(work_str) PFREE(work_str)
  else PFREE(ondelete)
  ActiveFile = oldh;
}

void __FASTCALL__  FiProgress(const char * filename,FiUserFunc usrproc)
{
  if(!usrproc) return;
  proc = usrproc;

  (*FiFileProcessor)(filename);

  proc = NULL;
  if(curr_sect) PFREE(curr_sect);
  if(curr_subsect) PFREE(curr_subsect);
  FiDeleteAllVar();
}

static void __FASTCALL__ hlFiFileProcessorStd(hIniProfile *ini)
{
  char * work_str, * ondelete;
  FiHandler oldh;
  tBool done;
  oldh = ActiveFile;
  ActiveFile = ini->handler;
  work_str = __FiCMaxStr();
  fi_Debug_Str = ondelete = work_str;
  rewind_ini(ini->handler);
  while(!bioEOF(ini->handler))
  {
      FiGetNextString(ini->handler,work_str,FI_MAXSTRLEN,NULL);
      if(!work_str || !work_str[0]) break;
      if(CaseSens == 1) strupr(work_str);
      if(CaseSens == 0) strlwr(work_str);
      done = (*FiStringProcessor)(work_str);
      if(done) break;
  }
  if(work_str) PFREE(work_str)
  else PFREE(ondelete)
  ActiveFile = oldh;
}

void __FASTCALL__ hlFiProgress(hIniProfile *ini,FiUserFunc usrproc)
{

  if(!usrproc) return;
  proc = usrproc;

  hlFiFileProcessorStd(ini);

  proc = NULL;
  if(curr_sect) PFREE(curr_sect);
  if(curr_subsect) PFREE(curr_subsect);
  FiDeleteAllVar();
}

void  (__FASTCALL__ *FiFileProcessor)(const char *fname) = FiFileProcessorStd;
tBool (__FASTCALL__ *FiStringProcessor)(char * curr_str) = FiStringProcessorStd;
tBool (__FASTCALL__ *FiCommandProcessor)(const char * cmd) = FiCommandProcessorStd;
tBool (__FASTCALL__ *FiGetCondition)( const char *condstr) = FiGetConditionStd;

/*****************************************************************\
*                      High level support                          *
\******************************************************************/

static hIniProfile *opening;

#define IC_STRING 0x0001

typedef struct tag_ini_Cache
{
  unsigned flags;
  char *   item;
  union
  {
    linearArray *leaf;
    char        *value;
  }v;
}ini_cache;

static tCompare __FASTCALL__ __full_compare_cache(const void __HUGE__ *v1,const void __HUGE__ *v2)
{
  const ini_cache __HUGE__ *c1, __HUGE__ *c2;
  int iflg;
  tCompare i_ret;
  c1 = (const ini_cache __HUGE__ *)v1;
  c2 = (const ini_cache __HUGE__ *)v2;
  iflg = __CmpLong__(c1->flags,c2->flags);
  i_ret = strcmp(c1->item,c2->item);
  return i_ret ? i_ret : iflg;
}
#define __compare_cache __full_compare_cache

static tBool __NEAR__ __FASTCALL__ __addCache(const char *section,const char *subsection,
                               const char *item,const char *value)
{
  void __HUGE__ *found;
  ini_cache __HUGE__ *it;
  ini_cache ic;
  (const char *)ic.item = section;
  ic.flags = 0;
  if(!(found =la_Find((linearArray *)opening->cache,&ic,__full_compare_cache)))
  {
      ic.item = PMalloc(strlen(section)+1);
      if(!ic.item) { opening->flags &= ~HINI_FULLCACHED; return True; }
      strcpy(ic.item,section);
      if(!(ic.v.leaf = la_Build(0,sizeof(ini_cache),0)))
      {
         PFree(ic.item);
         opening->flags &= ~HINI_FULLCACHED;
         return True;
      }
      ic.flags = 0;
      if(!(la_AddData((linearArray *)opening->cache,&ic,NULL)))
      {
        PFree(ic.item);
        la_Destroy(ic.v.leaf);
        opening->flags &= ~HINI_FULLCACHED;
        return True;
      }
      opening->flags |= HINI_UPDATED;
      la_Sort((linearArray *)opening->cache,__compare_cache);
      found = la_Find((linearArray *)opening->cache,&ic,__full_compare_cache);
      goto do_subsect;
  }
  else
  {
    do_subsect:
      it = (ini_cache __HUGE__ *)found;
      (const char *)ic.item = subsection;
      if(!(found=la_Find(it->v.leaf,&ic,__full_compare_cache)))
      {
        ic.item = PMalloc(strlen(subsection)+1);
        if(!ic.item) { opening->flags &= ~HINI_FULLCACHED; return True; }
        strcpy(ic.item,subsection);
        if(!(ic.v.leaf = la_Build(0,sizeof(ini_cache),0)))
        {
           PFree(ic.item);
           opening->flags &= ~HINI_FULLCACHED;
           return True;
        }
        ic.flags = 0;
        if(!(la_AddData(it->v.leaf,&ic,NULL)))
        {
          PFree(ic.item);
          la_Destroy(ic.v.leaf);
          opening->flags &= ~HINI_FULLCACHED;
          return True;
        }
        opening->flags |= HINI_UPDATED;
        la_Sort(it->v.leaf,__compare_cache);
        found = la_Find(it->v.leaf,&ic,__full_compare_cache);
        goto do_item;
      }
      else
      {
        do_item:
        it = (ini_cache __HUGE__ *)found;
        (const char *)ic.item = item;
        ic.flags = IC_STRING;
        if(!(found=la_Find(it->v.leaf,&ic,__full_compare_cache)))
        {
          ic.item = PMalloc(strlen(item)+1);
          if(!ic.item) { opening->flags &= ~HINI_FULLCACHED; return True; }
          strcpy(ic.item,item);
          ic.v.value = PMalloc(strlen(value)+1);
          if(!ic.v.value) { PFree(ic.item); opening->flags &= ~HINI_FULLCACHED; return True; }
          strcpy(ic.v.value,value);
          if(!(la_AddData(it->v.leaf,&ic,NULL)))
          {
            PFree(ic.item);
            PFree(ic.v.value);
            opening->flags &= ~HINI_FULLCACHED;
            return True;
          }
          opening->flags |= HINI_UPDATED;
          la_Sort(it->v.leaf,__compare_cache);
        }
        else
        {
          /* item already exists. Try replace it */
          it = (ini_cache __HUGE__ *)found;
          if(strcmp(it->v.value,value) != 0)
          {
             char *newval,*oldval;
             newval = PMalloc(strlen(value)+1);
             if(!newval)
             {
               opening->flags &= ~HINI_FULLCACHED;
               return True;
             }
             strcpy(newval,value);
             oldval = it->v.value;
             it->v.value = newval;
             PFree(oldval);
             opening->flags |= HINI_UPDATED;
          }
        }
      }
  }
  return False;
}

static tBool __FASTCALL__ __buildCache(IniInfo *ini)
{
  return __addCache(ini->section,ini->subsection,
                    ini->item,ini->value);
}

static void __FASTCALL__ __iter_destroy(void __HUGE__ *it);

#define __destroyCache(it) (la_IterDestroy(it,__iter_destroy))

static void __FASTCALL__ __iter_destroy(void __HUGE__ *it)
{
  ini_cache __HUGE__ *ic;
  ic = (ini_cache __HUGE__ *)it;
  if(ic->flags & IC_STRING)
  {
    PFree(ic->item);
    PFree(ic->v.value);
  }
  else
  {
    PFree(ic->item);
    la_IterDestroy(ic->v.leaf,__iter_destroy);
  }
}

static unsigned ret;
static unsigned buf_len;
static char *buf_ptr;
static const char *sect,* subsect,* item;

static int __NEAR__ __FASTCALL__ make_temp(const char *path,char *name_ptr)
{
  char *fullname, *nptr;
  unsigned i,len;
  int handle;
  fullname = PMalloc((strlen(path)+1)*2);
  if(!fullname) return -1;
  strcpy(fullname,path);
  len = strlen(fullname);
  if(fullname[len-4] == '.') nptr = &fullname[len-4];
  else                       nptr = &fullname[len];
  handle = -1;
  for(i = 0;i < 100;i++)
  {
  /*
     in this loop we are change only extension:
     file.tmp file.t1 ... file.t99
     (it's only for compatibilities between different OS FS)
  */
    sprintf(nptr,".t%i",i);
    handle = __OsOpen(fullname,FO_READONLY | SO_DENYNONE);
    if(handle == -1) handle = __OsOpen(fullname,FO_READONLY | SO_COMPAT);
    if(handle == -1)
    {
      handle = __OsCreate(fullname);
      if(handle != -1)
      {
        strcpy(name_ptr,fullname);
        goto bye;
      }
    }
  }
  bye:
  return handle;
}

static tBool __FASTCALL__ MyCallback(IniInfo * ini)
{
  int cond;
  cond = 0;
  if(sect) cond = strcmp(ini->section,sect) == 0;
  if(subsect) cond &= strcmp(ini->subsection,subsect) == 0;
  if(item) cond &= strcmp(ini->item,item) == 0;
  if(cond)
  {
    ret = min(strlen(ini->value),buf_len);
    strncpy(buf_ptr,ini->value,buf_len);
    return True;
  }
  return False;
}

static int __NEAR__ __FASTCALL__ out_sect(FILE * handle,const char *section)
{
   fprintf(handle,"[ %s ]\n",section);
   return 2;
}

static int __NEAR__ __FASTCALL__ out_subsect(FILE * handle,const char *subsection)
{
   fprintf(handle,"  < %s >\n",subsection);
   return 2;
}

static void __NEAR__ __FASTCALL__ out_item(FILE * handle,unsigned nled,const char *_item,const char *value)
{
  char *sm_char;
  unsigned i;
  sm_char = strchr(value,'%');
  if(sm_char && ifSmarting)
  {
    fprintf(handle,"#nosmart\n");
  }
  for(i = 0;i < nled;i++)
  {
    fprintf(handle," ");
  }
  fprintf(handle,"%s = ",_item);
  fwrite(value,strlen(value),1,handle);
  fprintf(handle,"\n");
  if(sm_char && ifSmarting)
  {
    fprintf(handle,"#smart\n");
  }
}

hIniProfile * __FASTCALL__ iniOpenFile(const char *fname,tBool *has_error)
{
  hIniProfile *_ret;
  unsigned fname_size;
  _ret = PMalloc(sizeof(hIniProfile));
  if(has_error) *has_error = True;
  if(_ret)
  {
    fname_size = strlen(fname)+1;
    _ret->fname = PMalloc(fname_size);
    if(_ret->fname) strcpy(_ret->fname,fname);
    else
    {
      PFree(_ret);
      return NULL;
    }
    if(__IsFileExists(_ret->fname)) _ret->handler = FiOpen(fname);
    else                            _ret->handler = &bNull;
  }
  opening = _ret;
  _ret->flags |= HINI_FULLCACHED;
  _ret->cache = (void *)la_Build(0,sizeof(ini_cache),NULL);
  if(_ret->cache)
  {
    hlFiProgress(_ret,__buildCache);
    _ret->flags &= ~HINI_UPDATED;
  }
  else          _ret->flags = 0;
  if(has_error) *has_error = _ret->handler == &bNull;
  return _ret;
}

static tBool __NEAR__ __FASTCALL__ __flushCache(hIniProfile *ini);

void __FASTCALL__ iniCloseFile(hIniProfile *ini)
{
  if(ini)
  {
    if(ini->cache)
    {
      __flushCache(ini);
      __destroyCache((linearArray *)ini->cache);
    }
    if(ini->handler != &bNull) FiClose(ini->handler);
    PFree(ini->fname);
    PFree(ini);
  }
}

unsigned __FASTCALL__ iniReadProfileString(hIniProfile *ini,const char *section,const char *subsection,
                           const char *_item,const char *def_value,char *buffer,
                           unsigned cbBuffer)
{
   if(!buffer) return 0;
   ret = 1;
   buf_len = cbBuffer;
   buf_ptr = buffer;
   sect = section;
   subsect = subsection;
   item = _item;
   if(cbBuffer) strncpy(buffer,def_value,cbBuffer);
   else         buffer[0] = 0;
   if(ini)
   {
     if(ini->handler != &bNull)
     {
       tBool v_found;
       v_found = False;
       if(ini->cache)
       {
          ini_cache ic;
          void __HUGE__ *found,__HUGE__ *foundi,__HUGE__ *foundv;
          ini_cache __HUGE__ *fi;
          (const char *)ic.item = section;
          ic.flags = 0;
          if((found=la_Find(ini->cache,&ic,__full_compare_cache))!=NULL)
          {
            (const char *)ic.item=subsection;
            fi = (ini_cache __HUGE__ *)found;
            if((foundi=la_Find(fi->v.leaf,&ic,__full_compare_cache))!=NULL)
            {
               (const char *)ic.item = _item;
               ic.flags = IC_STRING;
               fi = (ini_cache __HUGE__ *)foundi;
               if((foundv=la_Find(fi->v.leaf,&ic,__full_compare_cache))!=NULL)
               {
                  fi = (ini_cache __HUGE__ *)foundv;
                  strncpy(buffer,fi->v.value,cbBuffer);
                  ret = min(strlen(fi->v.value),cbBuffer);
                  v_found = True;
               }
            }
          }
       }
       if(!v_found && (ini->flags & HINI_FULLCACHED) != HINI_FULLCACHED) hlFiProgress(ini,MyCallback);
     }
   }
   buffer[cbBuffer-1] = 0;
   return ret;
}

#define HINI_HEADER "; This file generated automatically by BIEWLIB.\n; WARNING: Any changes made by hands may be lost at following program start.\n"

static FILE * __NEAR__ __FASTCALL__ __makeIni(hIniProfile *ini)
{
  FILE *hout;
  hout = fopen(ini->fname,"wt");
  if(hout != NULL)
  {
     fprintf(hout,HINI_HEADER);
  }
  return hout;
}

static tBool __NEAR__ __FASTCALL__ __createIni(hIniProfile *ini,
                                const char *_section,
                                const char *_subsection,
                                const char *_item,
                                const char *_value)
{
  FILE *hout;
  unsigned nled;
  tBool _ret;
  hout = __makeIni(ini);
  _ret = True;
  if(hout == NULL) _ret = False;
  else
  {
     nled = 0;
     if(_section)
     {
       nled += out_sect(hout,_section);
     }
     if(_subsection)
     {
       nled += out_subsect(hout,_subsection);
     }
     if(_item)
     {
       out_item(hout,nled,_item,_value);
     }
     fclose(hout);
  }
  return _ret;
}

static tBool __NEAR__ __FASTCALL__ __directWriteProfileString(hIniProfile *ini,
                                               const char *_section,
                                               const char *_subsection,
                                               const char *_item,
                                               const char *_value)
{
   FILE * tmphandle;
   char *tmpname, *prev_val;
   char * workstr, *wstr2;
   char *original;
   unsigned nled,prev_val_size;
   int hsrc;
   tBool _ret,need_write,s_ok,ss_ok,i_ok,done,sb_ok,ssb_ok,written,Cond,if_on;
   /* test for no change of value */
   prev_val_size = strlen(_value)+2;
   prev_val = PMalloc(prev_val_size);
   need_write = True;
   if(!ini) return False;
   if(prev_val && ini->handler != &bNull)
   {
     const char *def_val;
     if(strcmp(_value,"y") == 0) def_val = "n";
     else                       def_val = "y";
     iniReadProfileString(ini,_section,_subsection,_item,def_val,prev_val,prev_val_size);
     if(strcmp(prev_val,_value) == 0) need_write = False;
     PFree(prev_val);
   }
   if(!need_write) return True;
   tmpname = PMalloc((strlen(ini->fname)+1)*2);
   if(!tmpname) return False;
   if(ini->handler == &bNull) /* if file does not exist make it. */
   {
     _ret = __createIni(ini,_section,_subsection,_item,_value);
     if(_ret) ini->handler = FiOpen(ini->fname);
     goto Exit_WS;
   }
   bioSeek(ini->handler,0L,BIO_SEEK_SET);
   ActiveFile = ini->handler;
   hsrc = make_temp(ini->fname,tmpname);
   if(hsrc == -1) { _ret = False; goto Exit_WS; }
   __OsClose(hsrc);
   tmphandle = fopen(tmpname,"wt");
   if(tmphandle == NULL) { _ret = False; goto Exit_WS; }
   fprintf(tmphandle,HINI_HEADER);
   workstr = __FiCMaxStr();
   wstr2 = __FiCMaxStr();
   original = __FiCMaxStr();
   if(!workstr || !wstr2 || !original)
   {
     fclose(tmphandle);
     __OsDelete(tmpname);
     if(workstr) PFREE(workstr);
     if(wstr2) PFREE(wstr2);
     if(original) PFREE(original);
     _ret = False;
     goto Exit_WS;
   }
   sb_ok = ssb_ok = s_ok = ss_ok = i_ok = done = False;
   nled = 0;
   if(!_section) { s_ok = sb_ok = True; if(!_subsection) ss_ok = ssb_ok = True; }
   wstr2[0] = 1;
   Cond = True;
   if_on = False;
   while(1)
   {
     written = False;
     FiGetNextString(ini->handler,workstr,FI_MAXSTRLEN,original);
     if(workstr[0] == 0) break;
     if(FiisCommand(workstr))
     {
       char *cstr;
       cstr = __FiCCmd(workstr);
       if(strncmp(cstr,"if",2) == 0 ||
          strncmp(cstr,"elif",4) == 0 ||
          strncmp(cstr,"else",4) == 0 )
       if_on = True;
       if(strncmp(cstr,"endif",5) == 0) if_on = False;
       PFREE(cstr);
     }
     if(Cond)
     {
       if(FiisSection(workstr))
       {
          if(_section)
          {
            FiGetSectionName(workstr,wstr2);
            if(strcmp(_section,wstr2) == 0)
            {
              s_ok = sb_ok = True;
              nled = 2;
              ss_ok = ssb_ok = !_subsection;
            }
            else s_ok = False;
          }
          else s_ok = False;
       }
       else
       if(FiisSubSection(workstr))
       {
          if(_subsection)
          {
            FiGetSubSectionName(workstr,wstr2);
            if(strcmp(_subsection,wstr2) == 0)
            {
              ss_ok = ssb_ok = True;
              nled = 4;
            }
            else ss_ok = False;
          }
          else ss_ok = False;
       }
       else
       if(FiisItem(workstr))
       {
          FiGetItemName(workstr,wstr2);
          if(strcmp(_item,wstr2) == 0) i_ok = True;
          if(i_ok && s_ok && ss_ok)
          {
            if(!done || if_on)
            {
               out_item(tmphandle,nled,_item,_value);
               written = True;
               done = True;
            }
          }
          else i_ok = False;
      }
      /**********************************************/
      if(sb_ok && !s_ok)
      {
          if(!done)
          {
            if(!ssb_ok && !ss_ok && _subsection) { nled += 2; out_subsect(tmphandle,_subsection); }
            out_item(tmphandle,nled,_item,_value);
            written = False;
            done = True;
          }
      }
      if(s_ok && ssb_ok && !ss_ok)
      {
         if(!done)
         {
             out_item(tmphandle,nled,_item,_value);
             written = False;
             done = True;
         }
      }
    }
    if(!written) fprintf(tmphandle,"%s\n",original);
   }
   /************ this is end of loop *************/
   if(!done)
   {
     if(!sb_ok && _section)
     {
        nled = out_sect(tmphandle,_section);
        if(_subsection)
        {
          ssb_ok = True;
          nled+=out_subsect(tmphandle,_subsection);
        }
     }
     if(!ssb_ok && _subsection)
     {
        nled+=out_subsect(tmphandle,_subsection);
     }
     out_item(tmphandle,nled,_item,_value);
     done = True;
   }
   fclose(tmphandle);
   FiClose(ini->handler);
   __OsDelete(ini->fname);
   __OsRename(tmpname,ini->fname);
   ini->handler = FiOpen(ini->fname);
   PFREE(workstr);
   PFREE(wstr2);
   PFREE(original);
   _ret = True;
   Exit_WS:
   PFREE(tmpname);
   return _ret;
}

static char *__partSect,*__partSubSect;
static hIniProfile *part_ini_profile;

static void __FASTCALL__ part_flush_item(void __HUGE__ *data)
{
  ini_cache __HUGE__ *it;
  it = (ini_cache __HUGE__ *)data;
  __directWriteProfileString(part_ini_profile,
                             __partSect,
                             __partSubSect,
                             it->item,
                             it->v.value);
}

static void __FASTCALL__ part_flush_subsect(void __HUGE__ *data)
{
  ini_cache __HUGE__ *it;
  it = (ini_cache __HUGE__ *)data;
  __partSubSect = it->item;
  la_ForEach(it->v.leaf,part_flush_item);
}

static void __FASTCALL__ part_flush_sect(void __HUGE__ *data)
{
  ini_cache __HUGE__ *it;
  it = (ini_cache __HUGE__ *)data;
  __partSect = it->item;
  la_ForEach(it->v.leaf,part_flush_subsect);
}

static void __NEAR__ __FASTCALL__ __flushPartialCache(hIniProfile *ini)
{
  part_ini_profile = ini;
  la_ForEach((linearArray *)ini->cache,part_flush_sect);
}


tBool __FASTCALL__ iniWriteProfileString(hIniProfile *ini, const char *_section,
                                         const char *_subsection,
                                         const char *_item, const char *_value)
{
   tBool _ret;
   opening = ini;
   _ret = False;
   if(ini->cache)
   {
     if((ini->flags & HINI_FULLCACHED) != HINI_FULLCACHED) goto flush_it;
     if(__addCache(_section,_subsection,_item,_value) != 0)
     {
       flush_it:
       __flushPartialCache(ini);
       __destroyCache((linearArray *)ini->cache);
       ini->cache = 0;
       goto do_def;
     }
   }
   else
   {
     do_def:
     _ret = __directWriteProfileString(ini,_section,_subsection,_item,_value);
   }
   return _ret;
}

static int __nled;
static FILE * flush_handler = NULL;

static void __FASTCALL__ flush_item(void __HUGE__ *data)
{
  ini_cache __HUGE__ *it;
  it = (ini_cache __HUGE__ *)data;
  out_item(flush_handler,__nled,it->item,it->v.value);
}

static void __FASTCALL__ flush_subsect(void __HUGE__ *data)
{
  ini_cache __HUGE__ *it;
  int _has_led;
  it = (ini_cache __HUGE__ *)data;
  _has_led = __nled;
  if(strlen(it->item)) __nled += out_subsect(flush_handler,it->item);
  la_ForEach(it->v.leaf,flush_item);
  __nled = _has_led;
}

static void __FASTCALL__ flush_sect(void __HUGE__ *data)
{
  ini_cache __HUGE__ *it;
  it = (ini_cache __HUGE__ *)data;
  __nled = 0;
  if(strlen(it->item)) __nled += out_sect(flush_handler,it->item);
  la_ForEach(it->v.leaf,flush_subsect);
}

static tBool __NEAR__ __FASTCALL__ __flushCache(hIniProfile *ini)
{
  if((ini->flags & HINI_UPDATED) == HINI_UPDATED && ini->cache)
  {
    if(ini->handler != &bNull) FiClose(ini->handler);
    flush_handler = __makeIni(ini);
    if(!flush_handler) return True;
    la_ForEach((linearArray *)ini->cache,flush_sect);
    fclose(flush_handler);
    ini->handler = FiOpen(ini->fname);
  }
  return False;
}
