/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/sisx.c
 * @brief       This file contains implementation of decoder for SisX (EPOC)
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
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "reg_form.h"
#include "bmfile.h"
#include "bconsole.h"
#include "beyehelp.h"
#include "colorset.h"
#include "beyeutil.h"
#include "libbeye/kbd_code.h"
#include "plugins/bin/mmio.h"
#include "plugins/disasm.h"


struct E32ImageHeader
{
    unsigned long iUid1;
    unsigned long iUid2;
    unsigned long iUid3;
    unsigned long iUidCheckSum;
    unsigned char iSignature[4];
    unsigned long iHeaderCrc;
    unsigned long iModuleVersion;
    unsigned long iCompressionType;
    unsigned long iToolsVersion;
    unsigned long iTimeLo;
    unsigned long iTimeHi;
    unsigned long iFlags;
    unsigned long iCodeSize;
    unsigned long iDataSize;
    unsigned long iHeapSizeMin;
    unsigned long iHeapSizeMax;
    unsigned long iStackSize;
    unsigned long iBssSize;
    unsigned long iEntryPoint;
    unsigned long iCodeBase;
    unsigned long iDataBase;
    unsigned long iDllRefTableCount;
    unsigned long iExportDirOffset;
    unsigned long iExportDirCount;
    unsigned long iTextSize;
    unsigned long iCodeOffset;
    unsigned long iDataOffset;
    unsigned long iImportOffset;
    unsigned long iCodeRelocOffset;
    unsigned long iDataRelocOffset;
    unsigned short iProcessPriority;
    unsigned short iCpuIdentifier;
};

static tBool  __FASTCALL__ sisx_check_fmt( void )
{
    unsigned char sign[4];
    unsigned long id;
    bmSeek(0,BM_SEEK_SET);
    id=bmReadDWord();
    bmReadBufferEx(sign,sizeof(sign),16L,BM_SEEK_SET);
    if((id&0x10000000UL)==0x10000000UL && memcmp(sign,"EPOC",4)==0) return True;
    return False;
}
static void __FASTCALL__ sisx_init_fmt( void ) {}
static void __FASTCALL__ sisx_destroy_fmt(void) {}
static int  __FASTCALL__ sisx_platform(void) {
 unsigned id;
 struct E32ImageHeader img;
 bmReadBufferEx(&img,sizeof(img),0,BM_SEEK_SET);
 id=DISASM_DATA;
 if((img.iCpuIdentifier&0xF000)==0x1000) id=DISASM_CPU_IX86;
 else if((img.iCpuIdentifier&0xF000)==0x2000) id=DISASM_CPU_ARM;
 return id;
}

static __filesize_t __FASTCALL__ Show_SisX_Header( void )
{
 unsigned keycode;
 TWindow * hwnd;
 char *cpuname,*exetype,head[80];
 struct E32ImageHeader img;
 __filesize_t fpos,fpos2;
 fpos2=fpos = BMGetCurrFilePos();
 bmReadBufferEx(&img,sizeof(img),0,BM_SEEK_SET);
 switch(img.iUid1)
 {
    case 0x10000079: exetype="DLL"; break;
    case 0x1000007A: exetype="EXE"; break;
    default: exetype="UNK"; break;
 }
 switch(img.iCpuIdentifier)
 {
    case 0x1000: cpuname="x86"; break;
    case 0x2000: cpuname="ARMv4"; break;
    case 0x2001: cpuname="ARMv5"; break;
    case 0x2002: cpuname="ARMv6"; break;
    case 0x2003: cpuname="ARMv7"; break;
    case 0x2004: cpuname="ARMv8"; break;
    case 0x2005: cpuname="ARMv9"; break;
    case 0x4000: cpuname="MCore"; break;
    default:     cpuname="unknown"; break;
 }
 sprintf(head," E32Image Header (%s) ",exetype);
 hwnd = CrtDlgWndnls(head,68,15);
 twUseWin(hwnd);
 twGotoXY(1,1);
 twPrintF("Module/Tool Version  = 0x%08X/0x%08X\n"
          "Compression Type     = 0x%08X\n"
          "Flags                = 0x%08X (%s %s %s %s %s)\n"
          "Code/Data size       = 0x%08X/0x%08X\n"
          "Min/Max heap size    = 0x%08X/0x%08X\n"
          "Stack/BSS size       = 0x%08X/0x%08X\n"
          "Entry point          = 0x%08X\n"
          "Code/Data base       = 0x%08X/0x%08X\n"
          "Code/Data offset     = 0x%08X/0x%08X\n"
          "DLL ref offset/count = 0x%08X/%u\n"
          "ExportDir offset/cnt = 0x%08X/%u\n"
          "Text size            = %u\n"
          "Code/Data Reloc off  = 0x%08X/0x%08X\n"
          "Process priority     = 0x%04X\n"
          "CPU identifier       = 0x%04X(%s)\n"
	  ,img.iModuleVersion,img.iToolsVersion
	  ,img.iCompressionType
	  ,img.iFlags
	  ,img.iFlags&0x10000000?"PE_Fmt":""
	  ,img.iFlags&0x02000000?"Fmt_Ver":""
	  ,img.iFlags&0x00000020?"EKA2":""
	  ,img.iFlags&0x00000008?"EABI":""
	  ,img.iFlags&0x10000002?"No_call":""
	  ,img.iCodeSize,img.iDataSize
	  ,img.iHeapSizeMin,img.iHeapSizeMax
	  ,img.iStackSize,img.iBssSize
	  ,img.iEntryPoint
	  ,img.iCodeBase,img.iDataBase
	  ,img.iCodeOffset,img.iDataOffset
	  ,img.iImportOffset,img.iDllRefTableCount
	  ,img.iExportDirOffset,img.iExportDirCount
	  ,img.iTextSize
	  ,img.iCodeRelocOffset,img.iDataRelocOffset
	  ,img.iProcessPriority
	  ,img.iCpuIdentifier,cpuname
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

REGISTRY_BIN sisxTable =
{
  "SisX(EPOC) Symbian OS executable file",
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  sisx_check_fmt,
  sisx_init_fmt,
  sisx_destroy_fmt,
  Show_SisX_Header,
  NULL,
  NULL,
  sisx_platform,
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
