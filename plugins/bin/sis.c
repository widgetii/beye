/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/sis.c
 * @brief       This file contains implementation of decoder for Sis (EPOC)
 *              file format.
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
#include <stddef.h>

#include "reg_form.h"
#include "bmfile.h"
#include "bconsole.h"
#include "biewhelp.h"
#include "colorset.h"
#include "biewutil.h"
#include "biewlib/kbd_code.h"
#include "plugins/bin/mmio.h"
#include "plugins/disasm.h"

struct SisHeader
{
    unsigned long UID1;
    unsigned long UID2;
    unsigned long UID3;
    unsigned long UID4;
    unsigned short Checksum;
    unsigned short nLanguages;
    unsigned short nFiles;
    unsigned short nRequisites;
    unsigned short iLanguages;
    unsigned short iFiles;
    unsigned short iDrive;
    unsigned short nCapabilities;
    unsigned long  InstallVer;
    unsigned short Options;
    unsigned short Type;
    unsigned short MajorVer;
    unsigned short MinorVer;
    unsigned long  Variant;
    unsigned long  LanguagePointer;
    unsigned long  FilesPointer;
    unsigned long  RequisitiesPointer;
    unsigned long  SertificatePointer;
    unsigned long  ComponentNamePointer;
};

static tBool  __FASTCALL__ sis_check_fmt( void )
{
    unsigned long id1,id2,id3;
    bmSeek(0,BM_SEEK_SET);
    id1=bmReadDWordEx(0,BM_SEEK_SET);
    id2=bmReadDWordEx(4,BM_SEEK_SET);
    id3=bmReadDWordEx(8,BM_SEEK_SET);
    if((id2==0x10003A12 || id2==0x1000006D) && id3==0x10000419) return True;
    /* try s60 3rd */
    if(id1==0x10201A7A) return True;
    return False;
}
static void __FASTCALL__ sis_init_fmt( void ) {}
static void __FASTCALL__ sis_destroy_fmt(void) {}
static int  __FASTCALL__ sis_platform( void) { return DISASM_CPU_ARM; }

static __filesize_t __FASTCALL__ Show_Sis3_Header( void )
{
    ErrMessageBox("Not implemented yet!","Sis v3 header");
    return BMGetCurrFilePos();
}

static __filesize_t __FASTCALL__ Show_Sis_Header( void )
{
 unsigned keycode;
 TWindow * hwnd;
 char *TypeName;
 struct SisHeader sis;
 __filesize_t newcpos,fpos,fpos2;
 fpos2=fpos = BMGetCurrFilePos();
 bmReadBufferEx(&sis,sizeof(sis),0,BM_SEEK_SET);
 if(sis.UID1==0x10201A7A) return Show_Sis3_Header();
 switch(sis.Type)
 {
    case 0x0000: TypeName="APP"; break;
    case 0x0001: TypeName="SYSTEM"; break;
    case 0x0002: TypeName="OPTION"; break;
    case 0x0003: TypeName="CONFIG"; break;
    case 0x0004: TypeName="PATCH"; break;
    case 0x0005: TypeName="UPGRADE"; break;
    default:     TypeName="unknown"; break;
 }
 hwnd = CrtDlgWndnls(" Sis Header ",78,13);
 twUseWin(hwnd);
 twGotoXY(1,1);
 twPrintF("Number of Lang/Files/Req   = %u/%u/%u\n"
          "Installation Lang/Files/Drv= %u/%u/%u\n"
          "Number of capabilities     = %u\n"
          "Installer Version          = 0x%08X\n"
          "Options                    = 0x%04X(%s %s %s %s)\n"
          "Type                       = 0x%04X(%s)\n"
          "Version                    = 0x%04X.%04X\n"
          "Variant                    = 0x%08X\n"
          "Language Pointer           = 0x%08X\n"
          "Files Pointer              = 0x%08X\n"
          "Requsites Pointer          = 0x%08X\n"
          "Certificates Pointer       = 0x%08X\n"
          "Component Name Pointer     = 0x%08X\n"
	  ,sis.nLanguages,sis.nFiles,sis.nRequisites
	  ,sis.iLanguages,sis.iFiles,sis.iDrive
	  ,sis.nCapabilities
	  ,sis.InstallVer
	  ,sis.Options
	  ,sis.Options&0x0001?"Unicode":""
	  ,sis.Options&0x0002?"Distrib":""
	  ,sis.Options&0x0008?"NoCompr":""
	  ,sis.Options&0x0010?"ShutDwn":""
	  ,sis.Type,TypeName
	  ,sis.MajorVer,sis.MinorVer
	  ,sis.Variant
	  ,sis.LanguagePointer
	  ,sis.FilesPointer
	  ,sis.RequisitiesPointer
	  ,sis.SertificatePointer
	  ,sis.ComponentNamePointer
	  );
 while(1)
 {
   keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
   if(keycode == KE_F(5) || keycode == KE_ENTER) { fpos = fpos2; break; }
   else
     if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
 }
 CloseWnd(hwnd);
 return fpos;
}

REGISTRY_BIN sisTable =
{
  "Sis(EPOC) Symbian OS installable file",
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  sis_check_fmt,
  sis_init_fmt,
  sis_destroy_fmt,
  Show_Sis_Header,
  NULL,
  NULL,
  sis_platform,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
