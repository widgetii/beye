/**
 * @namespace   biew
 * @file        biew.c
 * @brief       This file contains entry point of BIEW project.
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
 * @author      Kostya Nosov <k-nosov@yandex.ru>
 * @date        27.11.2000
 * @note        Changing technology recognition of new-exe files
**/
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

#include "bconsole.h"
#include "colorset.h"
#include "bmfile.h"
#include "codeguid.h"
#include "editor.h"
#include "tstrings.h"
#include "reg_form.h"
#include "biewutil.h"
#include "search.h"
#include "setup.h"
#include "biewlib/file_ini.h"
#include "biewlib/kbd_code.h"
#include "biewlib/biewlib.h"
#include "biewlib/pmalloc.h"


unsigned ArgCount;
char **  ArgVector;
unsigned ListFileCount;
static char **ListFile;
static char *LastOpenFileName;
unsigned long LastOffset;
static tBool UseIniFile=True;
char biew_help_name[FILENAME_MAX+1] = "";
char biew_skin_name[FILENAME_MAX+1] = "";
extern char last_skin_error[];
char biew_scheme_name[256] = "Build-in";
static char biew_ini_ver[32];
unsigned long biew_vioIniFlags = 0L;
unsigned long biew_twinIniFlags = 0L;
unsigned long biew_kbdFlags = 0L;
tBool iniSettingsAnywhere = False;
tBool fioUseMMF = False;
tBool iniPreserveTime = False;
unsigned long headshift = 0L;
char *ini_name;

TWindow * MainWnd = 0,*HelpWnd = 0,*TitleWnd = 0,*ErrorWnd = 0;

#define SHORT_PATH_LEN __TVIO_MAXSCREENWIDTH-54

char shortname[SHORT_PATH_LEN + 1];

extern REGISTRY_BIN binTable;
extern REGISTRY_BIN neTable;
extern REGISTRY_BIN peTable;
extern REGISTRY_BIN leTable;
extern REGISTRY_BIN lxTable;
extern REGISTRY_BIN nlm386Table;
extern REGISTRY_BIN elf386Table;
extern REGISTRY_BIN coff386Table;
extern REGISTRY_BIN archTable;
extern REGISTRY_BIN aoutTable;
extern REGISTRY_BIN OldPharLapTable;
extern REGISTRY_BIN PharLapTable;
extern REGISTRY_BIN rdoffTable;
extern REGISTRY_BIN rdoff2Table;
extern REGISTRY_BIN lmfTable;
extern REGISTRY_BIN mzTable;
extern REGISTRY_BIN dossysTable;

static REGISTRY_BIN *mainBinTable[] =
{
  &neTable,
  &peTable,
  &leTable,
  &lxTable,
  &nlm386Table,
  &elf386Table,
  &coff386Table,
  &archTable,
  &aoutTable,
  &OldPharLapTable,
  &PharLapTable,
  &rdoffTable,
  &rdoff2Table,
  &lmfTable,
  &mzTable,
  &dossysTable,
  &binTable
};

REGISTRY_BIN *detectedFormat = 0;


extern REGISTRY_MODE binMode;
extern REGISTRY_MODE textMode;
extern REGISTRY_MODE hexMode;
extern REGISTRY_MODE disMode;

static REGISTRY_MODE *mainModeTable[] =
{
  &textMode,
  &binMode,
  &hexMode,
  &disMode
};

REGISTRY_MODE *activeMode;
static size_t LastMode = sizeof(mainModeTable)/sizeof(REGISTRY_BIN *)+10;

static unsigned defMainModeSel = 0;

tBool SelectMode( void )
{
  const char *modeName[sizeof(mainModeTable)/sizeof(REGISTRY_MODE *)];
  size_t i,nModes;
  int retval;

  nModes = sizeof(mainModeTable)/sizeof(REGISTRY_MODE *);
  for(i = 0;i < nModes;i++) modeName[i] = mainModeTable[i]->name;
  retval = SelBoxA(modeName,nModes," Select translation mode: ",defMainModeSel);
  if(retval != -1)
  {
    if(activeMode->term) activeMode->term();
    activeMode = mainModeTable[retval];
    if(activeMode->init) activeMode->init();
    defMainModeSel = retval;
    return True;
  }
  return False;
}

static void __NEAR__ __FASTCALL__ init_modes( hIniProfile *ini )
{
  if(activeMode->init) activeMode->init();
  if(activeMode->read_ini) activeMode->read_ini(ini);
}

static void __NEAR__ __FASTCALL__ term_modes( void )
{
  if(activeMode->term) activeMode->term();
}

static void __NEAR__ __FASTCALL__ __init_biew( void )
{
   LastOpenFileName = PMalloc(4096);
   ListFile = PMalloc((ArgCount-1)*sizeof(char *));
   if((!LastOpenFileName) || (!ListFile))
   {
     printm("BIEW initialization failed! Out of memory!");
     exit(EXIT_FAILURE);
   }
}

static void __NEAR__ __FASTCALL__ __term_biew( void )
{
   PFREE(LastOpenFileName);
   PFREE(ListFile);
}

void QuickSelectMode( void )
{
  unsigned nModes;
  nModes = sizeof(mainModeTable)/sizeof(REGISTRY_MODE *);
  if(defMainModeSel < nModes - 1) defMainModeSel++;
  else                            defMainModeSel = 0;
  if(activeMode->term) activeMode->term();
  activeMode = mainModeTable[defMainModeSel];
  if(activeMode->init) activeMode->init();
}

static void __NEAR__ __FASTCALL__ MakeShortName( void )
{
  unsigned l;
  unsigned slen = twGetClientWidth(TitleWnd)-54;
  l = strlen(ArgVector[1]);
  if(l <= slen) strcpy(shortname,ArgVector[1]);
  else
  {
    strncpy(shortname,ArgVector[1],slen/2 - 3);
    shortname[slen/2-4] = 0;
    strcat(shortname,"...");
    strcat(shortname,&ArgVector[1][l - slen/2]);
  }
  __nls_CmdlineToOem((unsigned char *)shortname,strlen(shortname));
}

unsigned long IsNewExe()
{
  unsigned long ret;
  char id[2];
  bmReadBufferEx(id,sizeof(id),0,BM_SEEK_SET);
#if 0
   /*
      It is well documented technology, but it correctly working
      only with normal stubs, i.e. when New EXE header is located at
      offset > 0x40. However, in PC world exists files with wrong
      stubs, which are normal for Host OS. Hence biew must recognize
      them as normal New EXE files, despite the fact that DOS can
      not execute ones.
      Fixed by Kostya Nosov <k-nosov@yandex.ru>.
   */
   if(!( id[0] == 'M' && id[1] == 'Z' &&
        bmReadWordEx(0x18,BM_SEEK_SET) >= 0x40 &&
        (ret=bmReadDWordEx(0x3C,BM_SEEK_SET)) > 0x40L)) ret = 0;
#endif
   if(!( id[0] == 'M' && id[1] == 'Z' &&
        (ret=bmReadDWordEx(0x3C,BM_SEEK_SET)) > 0x02L)) ret = 0;
   return ret;
}

static void __NEAR__ __FASTCALL__ AutoDetectMode( void )
{
  int i,n;
  n = sizeof(mainModeTable) / sizeof(REGISTRY_MODE *);
  for(i = 0;i < n;i++)
  {
    if(mainModeTable[i]->detect())
    {
      defMainModeSel = i;
      break;
    }
  }
  activeMode = mainModeTable[i];
  BMSeek(0,BM_SEEK_SET);
}

struct tagbiewArg
{
  const char key[4];
  const char *prompt;
}biewArg[] =
{
  { "-a", "autodetect mode (default)" },
  { "-b", "view file in binary mode" },
  { "-d", "view file in disassembler mode" },
  { "-h", "view file in hexadecimal mode" },
  { "-t", "view file in text mode" },
  { "-s", "change size of file to NNN bytes (create, if file does not exist)" },
  { "-i", "ignore .ini file (create new)" },
  { "-?", "display this screen" }
};

static int __NEAR__ __FASTCALL__ queryKey(char *arg)
{
  int ret = -1;
  size_t i;
  for(i = 0;i < sizeof(biewArg)/sizeof(struct tagbiewArg);i++)
  {
    if(strcmp(arg,biewArg[i].key) == 0) { ret = i; break; }
  }
  return ret;
}

static unsigned int  biew_mode     = UINT_MAX;
static unsigned long new_file_size = ULONG_MAX;

static void __NEAR__ __FASTCALL__ ParseCmdLine( void )
{
  unsigned i;
  ListFileCount = 0;
  for(i = 1;i < ArgCount;i++)
  {
     int biew_key;
     biew_key = queryKey(ArgVector[i]);
     switch(biew_key)
     {
       case 0: biew_mode = UINT_MAX; break;
       case 1: biew_mode = 1; break;
       case 2: biew_mode = 3; break;
       case 3: biew_mode = 2; break;
       case 4: biew_mode = 0; break;
       case 5: new_file_size = strtoul(ArgVector[++i],NULL,10); break;
       case 6: UseIniFile = False; break;
       case 7: ListFileCount = 0; return;
       default: ListFile[ListFileCount++] = ArgVector[i];
     }
  }
  if(ListFileCount) ArgVector[1] = ListFile[0];
}

static tBool __NEAR__ __FASTCALL__ LoadInfo( void )
{
   MakeShortName();
   if(new_file_size != ULONG_MAX)
   {
       int handle;
       if(__IsFileExists(ArgVector[1]) == False) handle = __OsCreate(ArgVector[1]);
       else
       {
         handle = __OsOpen(ArgVector[1],FO_READWRITE | SO_DENYNONE);
         if(handle == -1) handle = __OsOpen(ArgVector[1],FO_READWRITE | SO_COMPAT);
       }
       if(handle != -1)
       {
           __OsChSize(handle,new_file_size);
           __OsClose(handle);
       }
       else
       {
          errnoMessageBox(OPEN_FAIL,NULL,errno);
          return False;
       }
   }
   if(BMOpen(ArgVector[1]) != 0) return False;
   if(biew_mode != UINT_MAX)
   {
     defMainModeSel = biew_mode;
     activeMode = mainModeTable[defMainModeSel];
   }
   else
   {
     if(LastMode >= sizeof(mainModeTable)/sizeof(REGISTRY_MODE *) || !isValidIniArgs()) AutoDetectMode();
     else
     {
       defMainModeSel = LastMode;
       activeMode = mainModeTable[defMainModeSel];
     }
   }
 return True;
}

static void __NEAR__ __FASTCALL__ __detectBinFmt( void )
{
 unsigned i;
 if(!bmGetFLength())
 {
   detectedFormat = &binTable;
   return;
 }
 for(i = 0;i < sizeof(mainBinTable)/sizeof(REGISTRY_BIN *);i++)
 {
   if(mainBinTable[i]->check_format())
   {
     detectedFormat = mainBinTable[i];
     if(detectedFormat->init) detectedFormat->init();
     break;
   }
 }
 /* Special case: mz initialization */
 mzTable.check_format();
}

void PaintTitle( void )
{
 twUseWin(TitleWnd);
 twFreezeWin(TitleWnd);
 twGotoXY(1,1);
 twClrEOL();
 twPrintF("File : %s",shortname);
 twGotoXY(twGetClientWidth(TitleWnd)-43,1);
 twPrintF("Size : %8lu bytes",BMGetFLength());
 twRefreshWin(TitleWnd);
}

static void MyAtExit( void )
{
  if(MainWnd) CloseWnd(MainWnd);
  if(HelpWnd) CloseWnd(HelpWnd);
  if(TitleWnd) CloseWnd(TitleWnd);
  if(ErrorWnd) CloseWnd(ErrorWnd);
  termBConsole();
  __term_biew();
  __term_sys();
}

tBool isValidIniArgs( void )
{
  return iniSettingsAnywhere ? True :
         ArgVector[1] ? strcmp(ArgVector[1],LastOpenFileName) == 0 : False;
}

static hIniProfile * __NEAR__ __FASTCALL__ load_ini_info( void )
{
  char tmp[10];
  hIniProfile *ini;
  ini_name = getenv("BIEW_INI");
  if(!ini_name) ini_name = __get_ini_name("biew");
  ini = UseIniFile ? iniOpenFile(ini_name,NULL) : NULL;
  biewReadProfileString(ini,"Biew","Setup","HelpName","",biew_help_name,sizeof(biew_help_name));
  biewReadProfileString(ini,"Biew","Setup","SkinName","",biew_skin_name,sizeof(biew_skin_name));
  biewReadProfileString(ini,"Biew","Search","String","",(char *)search_buff,sizeof(search_buff));
  search_len = strlen((char *)search_buff);
  biewReadProfileString(ini,"Biew","Search","Case","off",tmp,sizeof(tmp));
  biewSearchFlg = stricmp(tmp,"on") == 0 ? SF_CASESENS : SF_NONE;
  biewReadProfileString(ini,"Biew","Search","Word","off",tmp,sizeof(tmp));
  if(stricmp(tmp,"on") == 0) biewSearchFlg |= SF_WORDONLY;
  biewReadProfileString(ini,"Biew","Search","Backward","off",tmp,sizeof(tmp));
  if(stricmp(tmp,"on") == 0) biewSearchFlg |= SF_REVERSE;
  biewReadProfileString(ini,"Biew","Search","Template","off",tmp,sizeof(tmp));
  if(stricmp(tmp,"on") == 0) biewSearchFlg |= SF_WILDCARDS;
  biewReadProfileString(ini,"Biew","Search","UsePlugin","off",tmp,sizeof(tmp));
  if(stricmp(tmp,"on") == 0) biewSearchFlg |= SF_PLUGINS;
  biewReadProfileString(ini,"Biew","Search","AsHex","off",tmp,sizeof(tmp));
  if(stricmp(tmp,"on") == 0) biewSearchFlg |= SF_ASHEX;
  biewReadProfileString(ini,"Biew","Browser","LastOpen","",LastOpenFileName,4096);
  sprintf(tmp,"%u",LastMode);
  biewReadProfileString(ini,"Biew","Browser","LastMode",tmp,tmp,sizeof(tmp));
  LastMode = (size_t)strtoul(tmp,NULL,10);
  biewReadProfileString(ini,"Biew","Browser","Offset","0",tmp,sizeof(tmp));
  LastOffset = atol(tmp); /** by watcom */
  biewReadProfileString(ini,"Biew","Setup","Version","",biew_ini_ver,sizeof(biew_ini_ver));
  biewReadProfileString(ini,"Biew","Setup","DirectConsole","yes",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) biew_vioIniFlags = __TVIO_FLG_DIRECT_CONSOLE_ACCESS;
  biewReadProfileString(ini,"Biew","Setup","ForceMono","no",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) biew_twinIniFlags = TWIF_FORCEMONO;
  biewReadProfileString(ini,"Biew","Setup","Force7Bit","no",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) biew_vioIniFlags |= __TVIO_FLG_USE_7BIT;
  biewReadProfileString(ini,"Biew","Setup","MouseSens","yes",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) biew_kbdFlags = KBD_NONSTOP_ON_MOUSE_PRESS;
  biewReadProfileString(ini,"Biew","Setup","IniSettingsAnywhere","no",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) iniSettingsAnywhere = True;
  biewReadProfileString(ini,"Biew","Setup","FioUseMMF","no",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) fioUseMMF = True;
  if(!__mmfIsWorkable()) fioUseMMF = False;
  biewReadProfileString(ini,"Biew","Setup","PreserveTimeStamp","no",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) iniPreserveTime = True;
  return ini;
}

static void __NEAR__ __FASTCALL__ save_ini_info( void )
{
  char tmp[10];
  hIniProfile *ini;
  search_buff[search_len] = 0;
  ini = iniOpenFile(ini_name,NULL);
  biewWriteProfileString(ini,"Biew","Setup","HelpName",biew_help_name);
  biewWriteProfileString(ini,"Biew","Setup","SkinName",biew_skin_name);
  biewWriteProfileString(ini,"Biew","Setup","Version",BIEW_VERSION);
  biewWriteProfileString(ini,"Biew","Search","String",(char *)search_buff);
  biewWriteProfileString(ini,"Biew","Search","Case",biewSearchFlg & SF_CASESENS ? "on" : "off");
  biewWriteProfileString(ini,"Biew","Search","Word",biewSearchFlg & SF_WORDONLY ? "on" : "off");
  biewWriteProfileString(ini,"Biew","Search","Backward",biewSearchFlg & SF_REVERSE ? "on" : "off");
  biewWriteProfileString(ini,"Biew","Search","Template",biewSearchFlg & SF_WILDCARDS ? "on" : "off");
  biewWriteProfileString(ini,"Biew","Search","UsePlugin",biewSearchFlg & SF_PLUGINS ? "on" : "off");
  biewWriteProfileString(ini,"Biew","Search","AsHex",biewSearchFlg & SF_ASHEX ? "on" : "off");
  biewWriteProfileString(ini,"Biew","Browser","LastOpen",ArgVector[1]);
  sprintf(tmp,"%u",defMainModeSel);
  biewWriteProfileString(ini,"Biew","Browser","LastMode",tmp);
  sprintf(tmp,"%lu",LastOffset);
  biewWriteProfileString(ini,"Biew","Browser","Offset",tmp);
  strcpy(tmp,biew_vioIniFlags & __TVIO_FLG_DIRECT_CONSOLE_ACCESS ? "yes" : "no");
  biewWriteProfileString(ini,"Biew","Setup","DirectConsole",tmp);
  strcpy(tmp,biew_twinIniFlags & TWIF_FORCEMONO ? "yes" : "no");
  biewWriteProfileString(ini,"Biew","Setup","ForceMono",tmp);
  strcpy(tmp,(biew_vioIniFlags & __TVIO_FLG_USE_7BIT) == __TVIO_FLG_USE_7BIT ? "yes" : "no");
  biewWriteProfileString(ini,"Biew","Setup","Force7Bit",tmp);
  strcpy(tmp,biew_kbdFlags & KBD_NONSTOP_ON_MOUSE_PRESS ? "yes" : "no");
  biewWriteProfileString(ini,"Biew","Setup","MouseSens",tmp);
  strcpy(tmp,iniSettingsAnywhere ? "yes" : "no");
  biewWriteProfileString(ini,"Biew","Setup","IniSettingsAnywhere",tmp);
  strcpy(tmp,fioUseMMF ? "yes" : "no");
  biewWriteProfileString(ini,"Biew","Setup","FioUseMMF",tmp);
  strcpy(tmp,iniPreserveTime ? "yes" : "no");
  biewWriteProfileString(ini,"Biew","Setup","PreserveTimeStamp",tmp);
  if(activeMode->save_ini) activeMode->save_ini(ini);
  iniCloseFile(ini);
}

static FTime ftim;
static tBool ftim_ok = False;

int main( int argc, char *argv[] )
{
 hIniProfile *ini;
 tBool skin_err;
 int retval;
 ArgCount = argc;
 ArgVector = argv;
 if(ArgCount < 2) goto show_usage;
 __init_sys();
 __init_biew();
 ParseCmdLine();
 if(!ListFileCount)
 {
   /** print usage message */
    size_t i;
    show_usage:
    printm("\n"BIEW_VER_MSG"\n");
    printm(" Usage: biew [OPTIONS] file...\n\n");
    for(i = 0;i < sizeof(biewArg)/sizeof(struct tagbiewArg);i++)
    {
      printm("  %s\t%s\n",biewArg[i].key,biewArg[i].prompt);
    }
    printm("\n");
    return EXIT_FAILURE;
 }
 ini = load_ini_info();
 skin_err = csetReadIniFile(biew_skin_name);
 initBConsole(biew_vioIniFlags,biew_twinIniFlags);
 ErrorWnd = WindowOpen(1,1,50,16,TWS_NONE | TWS_NLSOEM);
 if(ErrorWnd) twSetTitleAttr(ErrorWnd," Error ",TW_TMODE_CENTER,error_cset.border);
 else { printm("fatal error: can not create window"); return EXIT_FAILURE; }
 twCentredWin(ErrorWnd,NULL);
 twSetColorAttr(error_cset.main);
 twSetFrameAttr(ErrorWnd,TW_DOUBLE_FRAME,error_cset.border);
 HelpWnd = WindowOpen(1,tvioHeight,tvioWidth,tvioHeight,TWS_NLSOEM);
 twSetColorAttr(prompt_cset.digit);
 twClearWin();
 twShowWin(HelpWnd);
 if(strcmp(biew_ini_ver,BIEW_VERSION) != 0) Setup();
 TitleWnd = WindowOpen(1,1,tvioWidth,1,TWS_NONE);
 twSetColorAttr(title_cset.main);
 twClearWin();
 twShowWin(TitleWnd);
 activeMode = mainModeTable[1];
 atexit(MyAtExit);
 retval = EXIT_SUCCESS;
 if(skin_err)
 {
   char sout[256];
   sprintf(sout,"Detected error in skin file: '%s'",last_skin_error);
   ErrMessageBox(sout,NULL);
 }
 /* We must do it before opening a file because of some RTL has bug
    when are trying to open already open file with no sharing access */
 ftim_ok = __OsGetFTime(ArgVector[1],&ftim);
 if(!LoadInfo())
 {
   if(ini) iniCloseFile(ini);
   retval = EXIT_FAILURE;
   goto Bye;
 }
 __detectBinFmt();
 init_modes(ini);
 init_addons();
 init_sysinfo();
 if(ini) iniCloseFile(ini);
 MainWnd = WindowOpen(1,2,tvioWidth,tvioHeight-1,TWS_NONE);
 twSetColorAttr(browser_cset.main);
 twClearWin();
 PaintTitle();
 if(!isValidIniArgs() || LastOffset > BMGetFLength()) LastOffset = 0;
 twShowWin(MainWnd);
 MainLoop();
 LastOffset = BMGetCurrFilePos();
 save_ini_info();
 term_sysinfo();
 term_addons();
 term_modes();
 if(detectedFormat->destroy) detectedFormat->destroy();
 BMClose();
 if(iniPreserveTime && ftim_ok) __OsSetFTime(ArgVector[1],&ftim);
 Bye:
 return retval;
}

tBool NewSource( void )
{
  int i;
  tBool ret;
  unsigned j,freq;
  static int prev_file;
  char ** nlsListFile;
  nlsListFile = (char **)PMalloc(ListFileCount*sizeof(char *));
  if(nlsListFile)
  {
    for(j = 0;j < ListFileCount;j++)
    {
      nlsListFile[j] = PMalloc(strlen(ListFile[j])+1);
      if(!nlsListFile[j]) break;
    }
  }
  else { MemOutBox("Initializing List of File\n"); return 0; }
  for(freq = 0;freq < j;freq++)
  {
    unsigned ls;
    ls = strlen(ListFile[freq]);
    memcpy(nlsListFile[freq],ListFile[freq],ls+1);
    __nls_CmdlineToOem((unsigned char *)nlsListFile[freq],ls);
  }
  i = SelBoxA((const char **)nlsListFile,j," Select new file: ",prev_file);
  ret = 0;
  for(freq = 0;freq < j;freq++) PFree(nlsListFile[freq]);
  PFree(nlsListFile);
  if(i != -1)
  {
    if(iniPreserveTime && ftim_ok) __OsSetFTime(ArgVector[1],&ftim);
    BMClose();
    ftim_ok = __OsGetFTime(ListFile[i],&ftim);
    if(BMOpen(ListFile[i]) == 0)
    {
      ArgVector[1] = ListFile[i];
      if(detectedFormat->destroy) detectedFormat->destroy();
      if(activeMode->term) activeMode->term();
      MakeShortName();
      __detectBinFmt();
      if(activeMode->init) activeMode->init();
      ret = True;
    }
    else
    {
       if(BMOpen(ArgVector[1]) != 0)
       {
         exit(EXIT_FAILURE);
       }
       if(detectedFormat->destroy) detectedFormat->destroy();
       if(activeMode->term) activeMode->term();
       MakeShortName();
       __detectBinFmt();
       if(activeMode->init) activeMode->init();
       ret = False;
    }
  }
  return ret;
}

unsigned __FASTCALL__ biewReadProfileString(hIniProfile *ini,
                               const char *section,
                               const char *subsection,
                               const char *_item,
                               const char *def_value,
                               char *buffer,
                               unsigned cbBuffer)
{
  return UseIniFile ? iniReadProfileString(ini,section,subsection,
                                           _item,def_value,buffer,cbBuffer)
                    : 1;
}

tBool __FASTCALL__ biewWriteProfileString(hIniProfile *ini,
                                          const char *section,
                                          const char *subsection,
                                          const char *item,
                                          const char *value)
{
  return iniWriteProfileString(ini,section,subsection,item,value);
}
