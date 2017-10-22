/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/lx.c
 * @brief       This file contains implementation of LX (Linear eXecutable) file
 *              format decoder.
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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colorset.h"
#include "plugins/disasm.h"
#include "plugins/bin/lx_le.h"
#include "plugins/bin/ne.h"
#include "bin_util.h"
#include "bmfile.h"
#include "beyehelp.h"
#include "tstrings.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "libbeye/kbd_code.h"
#include "libbeye/beyelib.h"
#include "libbeye/pmalloc.h"

union LX_LE lxe;
int LXType;

BGLOBAL lx_cache = &bNull;

static __filesize_t __NEAR__ __FASTCALL__ CalcEntryPointLX(unsigned long objnum,__filesize_t _offset);

static const char * LXordering[] =
{
 "little endian",
 "big endian"
};

const char * LXcputype[] =
{
 "Unknown",
 "80286",
 "80386",
 "80486",
 "80586",
 "80686",
 "80787",
 "80887"
};

const char * LXostype[] =
{
  "Unknown",
  "OS/2",
  "Windows",
  "DOS 4.x",
  "Windows/386",
  "Unknown",
  "Unknown",
  "Unknown"
};

const char * __osModType[] =
{
  "PROGRAM",
  "LIBRARY",
  "PROT. MODE PROGRAM",
  "PROT. MODE LIBRARY",
  "PHYSICAL DEVICE DRIVER",
  "VIRTUAL DEVICE DRIVER",
  "PROT. MODE PHYSICAL DEVICE DRIVER",
  "PROT. MODE VIRTUAL DEVICE DRIVER"
};

static __filesize_t LXEntryPoint = 0;

static const char * __NEAR__ __FASTCALL__ GetOrderingLX(unsigned char type)
{
 if(type < 2) return LXordering[type];
 else         return "";
}

static const char * __NEAR__ __FASTCALL__ GetCPUTypeLX(int num)
{
 if(num > 5) num = 0;
 return LXcputype[num];
}

static const char * __NEAR__ __FASTCALL__ GetOSTypeLX(int num)
{
  if(num > 4) num = 0;
  return LXostype[num];
}

static const char * __NEAR__ __FASTCALL__ __getOSModType(char type)
{
  return __osModType[type & 0x07];
}

static void __NEAR__ PaintNewHeaderLX_1(void)
{
  twPrintF("Signature                        = '%c%c'\n"
           "Byte order                       = %02XH (%s)\n"
           "Word order                       = %02XH (%s)\n"
           "Format level                     = %08lXH\n"
           "OS Type                          = %s\n"
           "CPU Type                         = %s\n"
           "Module version                   = %hu.%04X\n"
           "Linear flags :                     [%08lXH]\n"
           "  Contest DATA in EXE: %s\n"
           "  [%c] Per-process library initializtion\n"
           "  [%c] Internal fixups have been applied\n"
           "  [%c] External fixups have been applied\n"
           "  %s\n"
           "  [%c] Module is not loadable (contains errors)\n"
           "  Module type is : %s\n"
           "  [%c] Symmetric Multi Processor mode disabled\n"
           "  [%c] Per-process library termination\n"
           "Number of pages                  = %08lXH\n"
           "EIP objects number               = %08lXH\n"
           "EIP                              = %08lXH\n"
           "ESP objects number               = %08lXH\n"
           "ESP                              = %08lXH"
           ,lxe.lx.lxSignature[0],lxe.lx.lxSignature[1]
           ,(int)lxe.lx.lxByteOrdering,GetOrderingLX(lxe.lx.lxByteOrdering)
           ,(int)lxe.lx.lxWordOrdering,GetOrderingLX(lxe.lx.lxWordOrdering)
           ,lxe.lx.lxFormatLevel
           ,GetOSTypeLX(lxe.lx.lxOSType)
           ,GetCPUTypeLX(lxe.lx.lxCPUType)
           ,(unsigned short)(lxe.lx.lxModuleVersion >> 16),(unsigned)(unsigned short)(lxe.lx.lxModuleVersion)
           ,lxe.lx.lxModuleFlags
           ,__nedata[lxe.lx.lxModuleFlags & 0x0000003]
           ,GetBool((lxe.lx.lxModuleFlags & 0x00000004L) == 0x00000004L)
           ,GetBool((lxe.lx.lxModuleFlags & 0x00000010L) == 0x00000010L)
           ,GetBool((lxe.lx.lxModuleFlags & 0x00000020L) == 0x00000020L)
           ,GetPMWinAPI((unsigned)(lxe.lx.lxModuleFlags))
           ,GetBool((lxe.lx.lxModuleFlags & 0x00002000L) == 0x00002000L)
           ,__getOSModType(((lxe.lx.lxModuleFlags & 0x00038000L) >> 15) & 0x07)
           ,GetBool((lxe.lx.lxModuleFlags & 0x00080000L) == 0x00080000L)
           ,GetBool((lxe.lx.lxModuleFlags & 0x40000000L) == 0x40000000L)
           ,lxe.lx.lxPageCount
           ,lxe.lx.lxEIPObjectNumbers
           ,lxe.lx.lxEIP
           ,lxe.lx.lxESPObjectNumbers
           ,lxe.lx.lxESP);
}

static void __NEAR__ PaintNewHeaderLX_2( void )
{
  twPrintF("Page size                        = %08lXH\n"
           "Page offset shift                = %08lXH\n"
           "Fixup section size               = %08lXH\n"
           ,lxe.lx.lxPageSize
           ,lxe.lx.lxPageOffsetShift
           ,lxe.lx.lxFixupSectionSize);
  if(LXType == FILE_LX) twPrintF("Fixup section checksum           = %08lXH\n",lxe.lx.lxFixupSectionChecksum);
  else                  twPrintF("Page checksum                    = %08lXH\n",lxe.lx.lxFixupSectionChecksum);
  twPrintF("Loader section size              = %08lXH\n"
           "Loader section checksum          = %08lXH\n"
           "Object table offset              = %08lXH\n"
           "Number of objects in module      = %08lXH\n"
           "Object page table offset         = %08lXH\n"
           "Object iter page  offset         = %08lXH\n"
           "Resource table offset            = %08lXH\n"
           "Number of resource table entries = %08lXH\n"
           "Resident name table offset       = %08lXH\n"
           "Entry table table offset         = %08lXH\n"
           "Module directives offset         = %08lXH\n"
           "Number module directives         = %08lXH\n"
           "Fixup page table offset          = %08lXH\n"
           "Fixup record table offset        = %08lXH\n"
           "Import module table offset       = %08lXH\n"
           "Import module table entries      = %08lXH\n"
           "Import procedure table offset    = %08lXH"
           ,lxe.lx.lxLoaderSectionSize
           ,lxe.lx.lxLoaderSectionChecksum
           ,lxe.lx.lxObjectTableOffset
           ,lxe.lx.lxObjectCount
           ,lxe.lx.lxObjectPageTableOffset
           ,lxe.lx.lxObjectIterPageOffset
           ,lxe.lx.lxResourceTableOffset
           ,lxe.lx.lxNumberResourceTableEntries
           ,lxe.lx.lxResidentNameTableOffset
           ,lxe.lx.lxEntryTableOffset
           ,lxe.lx.lxModuleDirectivesOffset
           ,lxe.lx.lxNumberModuleDirectives
           ,lxe.lx.lxFixupPageTableOffset
           ,lxe.lx.lxFixupRecordTableOffset
           ,lxe.lx.lxImportModuleTableOffset
           ,lxe.lx.lxImportModuleTableEntries
           ,lxe.lx.lxImportProcedureTableOffset);
}

static void __NEAR__ PaintNewHeaderLX_3( void )
{
  twPrintF("Per - page checksum  offset      = %08lXH\n"
           "Data pages offset                = %08lXH\n"
           "Number of preload pages          = %08lXH\n"
           "Non resident name table offset   = %08lXH\n"
           "Non resident name table length   = %08lXH\n"
           "Non resident name table checksum = %08lXH\n"
           ,lxe.lx.lxPerPageChecksumOffset
           ,lxe.lx.lxDataPagesOffset
           ,lxe.lx.lxNumberPreloadPages
           ,lxe.lx.lxNonResidentNameTableOffset
           ,lxe.lx.lxNonResidentNameTableLength
           ,lxe.lx.lxNonResidentNameTableChecksum);
  if(LXType == FILE_LX)
  {
    twPrintF("Auto DS objects number           = %08lXH\n"
             "Debug info offset                = %08lXH\n"
             "Debug info length                = %08lXH\n"
             "Number instance preload          = %08lXH\n"
             "Number instance demand           = %08lXH\n"
             "Heap size                        = %08lXH\n"
             "Stack size                       = %08lXH\n"
             ,lxe.lx.lxAutoDSObjectNumber
             ,lxe.lx.lxDebugInfoOffset
             ,lxe.lx.lxDebugInfoLength
             ,lxe.lx.lxNumberInstancePreload
             ,lxe.lx.lxNumberInstanceDemand
             ,lxe.lx.lxHeapSize
             ,lxe.lx.lxStackSize);
  }
  else
  {
    twPrintF("Debug info offset                = %08lXH\n"
             "Debug info length                = %08lXH\n"
             ,lxe.lx.lxAutoDSObjectNumber
             ,lxe.lx.lxDebugInfoOffset);
  }
  twSetColorAttr(dialog_cset.entry);
  twPrintF(">Entry Point                     = %08lXH",LXEntryPoint);
  twClrEOL();
  twSetColorAttr(dialog_cset.main);
}

static void (__NEAR__ * lxphead[])( void ) =
{
  PaintNewHeaderLX_1,
  PaintNewHeaderLX_2,
  PaintNewHeaderLX_3
};

static void __FASTCALL__ PaintNewHeaderLX(TWindow * win,const void **ptr,unsigned npage,unsigned tpage)
{
  char text[80];
  UNUSED(ptr);
  twUseWin(win);
  twFreezeWin(win);
  twClearWin();
  sprintf(text," Linear eXecutable Header [%d/%d] ",npage + 1,tpage);
  twSetTitleAttr(win,text,TW_TMODE_CENTER,dialog_cset.title);
  twSetFooterAttr(win,PAGEBOX_SUB,TW_TMODE_RIGHT,dialog_cset.selfooter);
  if(npage < 3)
  {
    twGotoXY(1,1);
    (*(lxphead[npage]))();
  }
  twRefreshFullWin(win);
}

__filesize_t __FASTCALL__ ShowNewHeaderLX( void )
{
  __filesize_t fpos;
  LXEntryPoint = LXType == FILE_LX ? CalcEntryPointLX(lxe.lx.lxEIPObjectNumbers,lxe.lx.lxEIP) : CalcEntryPointLE(lxe.lx.lxEIPObjectNumbers,lxe.lx.lxEIP);
  if(LXEntryPoint == FILESIZE_MAX) LXEntryPoint = 0;
  fpos = BMGetCurrFilePos();
  if(PageBox(70,21,NULL,3,PaintNewHeaderLX) != -1)
  {
    if(LXEntryPoint) fpos = LXEntryPoint;
  }
  return fpos;
}

static unsigned __FASTCALL__ LXModRefNumItems(BGLOBAL handle)
{
  UNUSED(handle);
  return (unsigned)lxe.lx.lxImportModuleTableEntries;
}

unsigned __FASTCALL__ LXRNamesNumItems(BGLOBAL handle)
{
  return GetNamCountNE(handle,headshift + lxe.lx.lxResidentNameTableOffset);
}

unsigned __FASTCALL__ LXNRNamesNumItems(BGLOBAL handle)
{
  return GetNamCountNE(handle,lxe.lx.lxNonResidentNameTableOffset);
}

tBool __FASTCALL__ LXRNamesReadItems(BGLOBAL handle,memArray * obj,unsigned nnames)
{
   return RNamesReadItems(handle,obj,nnames,lxe.lx.lxResidentNameTableOffset + headshift);
}

static unsigned __FASTCALL__ LXImpNamesNumItems(BGLOBAL handle)
{
  __filesize_t fpos;
  unsigned char len;
  unsigned count;
  bioSeek(handle,lxe.lx.lxImportProcedureTableOffset + headshift,SEEKF_START);
  fpos = bioTell(handle);
  count = 0;
  while(fpos < lxe.lx.lxFixupSectionSize + lxe.lx.lxFixupPageTableOffset + headshift)
  {
    len = bioReadByte(handle);
    bioSeek(handle,len,SEEKF_CUR);
    fpos = bioTell(handle);
    if(bioEOF(handle)) break;
    count++;
  }
  return count;
}

static tBool __FASTCALL__ LXImpNamesReadItems(BGLOBAL handle,memArray * obj,unsigned nnames)
{
 unsigned i;
 unsigned char byte;
 bioSeek(handle,lxe.lx.lxImportProcedureTableOffset + headshift,SEEKF_START);
 for(i = 0;i < nnames;i++)
 {
   char nam[256];
   byte = bioReadByte(handle);
   if(IsKbdTerminate() || bioEOF(handle)) break;
   bioReadBuffer(handle,nam,byte);
   nam[byte] = 0;
   if(!ma_AddString(obj,nam,True)) break;
 }
 return True;
}


tBool __FASTCALL__ LXNRNamesReadItems(BGLOBAL handle,memArray * obj,unsigned nnames)
{
   return RNamesReadItems(handle,obj,nnames,lxe.lx.lxNonResidentNameTableOffset);
}

#if 0
extern unsigned __FASTCALL__ RNameReadFull(BGLOBAL handle,char * names,unsigned nindex,__filesize_t _offset);
static unsigned __FASTCALL__ LXRNamesReadFullName(BGLOBAL handle,char * names,unsigned index)
{
   return RNameReadFull(handle,names,index,lxe.lx.lxResidentNameTableOffset + headshift);
}

static unsigned __FASTCALL__ LXNRNamesReadFullName(BGLOBAL handle,char * names,unsigned index)
{
   return RNameReadFull(handle,names,index,lxe.lx.lxNonResidentNameTableOffset);
}
#endif
static tBool __FASTCALL__  __ReadModRefNamesLX(BGLOBAL handle,memArray * obj,unsigned nnames)
{
 unsigned i;
 unsigned char byte;
 bioSeek(handle,lxe.lx.lxImportModuleTableOffset + headshift,SEEKF_START);
 for(i = 0;i < nnames;i++)
 {
   char nam[256];
   byte = bioReadByte(handle);
   if(IsKbdTerminate() || bioEOF(handle)) break;
   bioReadBuffer(handle,nam,byte);
   nam[byte] = 0;
   if(!ma_AddString(obj,nam,True)) break;
 }
 return True;
}

static void __NEAR__ __FASTCALL__ objpaintLX(const LX_OBJECT *nam)
{
 twGotoXY(1,1);
 twPrintF("Virtual Size                         = %lX bytes\n"
          "BVA (base virtual address)           = %08lX\n"
          "FLAGS: %lX\n"
          "   [%c] Readable object\n"
          "   [%c] Writable object\n"
          "   [%c] Executable object\n"
          "   [%c] Resource object\n"
          "   [%c] Object is discardable\n"
          "   [%c] Object is shared\n"
          "   [%c] Object has preload pages\n"
          "   [%c] Object has invalid pages\n"
          "   [%c] Object is permanent and swappable\n"
          "   [%c] Object is permanent and resident\n"
          "   [%c] Object is permanent and long lockable\n"
          "   [%c] 16:16 alias required (80x86 specific)\n"
          "   [%c] Big/Default bit setting (80x86 specific)\n"
          "   [%c] Object is conforming for code (80x86 specific)\n"
          "   [%c] Object I/O privilege level (80x86 specific)\n"
          "   [%c] Object is loadable to High Memory (>512MiB)\n"
          "Object page map index                = %lu\n"
          "Number of entries in object page map = %lu"
          ,nam->o32_size
          ,nam->o32_base
          ,nam->o32_flags
          ,GetBool((nam->o32_flags & 0x00000001L) == 0x00000001L)
          ,GetBool((nam->o32_flags & 0x00000002L) == 0x00000002L)
          ,GetBool((nam->o32_flags & 0x00000004L) == 0x00000004L)
          ,GetBool((nam->o32_flags & 0x00000008L) == 0x00000008L)
          ,GetBool((nam->o32_flags & 0x00000010L) == 0x00000010L)
          ,GetBool((nam->o32_flags & 0x00000020L) == 0x00000020L)
          ,GetBool((nam->o32_flags & 0x00000040L) == 0x00000040L)
          ,GetBool((nam->o32_flags & 0x00000080L) == 0x00000080L)
          ,GetBool((nam->o32_flags & 0x00000100L) == 0x00000100L)
          ,GetBool((nam->o32_flags & 0x00000200L) == 0x00000200L)
          ,GetBool((nam->o32_flags & 0x00000400L) == 0x00000400L)
          ,GetBool((nam->o32_flags & 0x00001000L) == 0x00001000L)
          ,GetBool((nam->o32_flags & 0x00002000L) == 0x00002000L)
          ,GetBool((nam->o32_flags & 0x00004000L) == 0x00004000L)
          ,GetBool((nam->o32_flags & 0x00008000L) == 0x00008000L)
          ,GetBool((nam->o32_flags & 0x00010000L) == 0x00010000L)
          ,nam->o32_pagemap
          ,nam->o32_mapsize);
}

static void __FASTCALL__ ObjPaintLX(TWindow * win,const void ** names,unsigned start,unsigned nlist)
{
 char buffer[81];
 const LX_OBJECT ** nam = (const LX_OBJECT **)names;
 twUseWin(win);
 twFreezeWin(win);
 twClearWin();
 sprintf(buffer," Object Table [ %u / %u ] ",start + 1,nlist);
 twSetTitleAttr(win,buffer,TW_TMODE_CENTER,dialog_cset.title);
 twSetFooterAttr(win,PAGEBOX_SUB,TW_TMODE_RIGHT,dialog_cset.selfooter);
 objpaintLX(nam[start]);
 twRefreshFullWin(win);
}

static tBool __NEAR__ __FASTCALL__ __ReadObjectsLX(BGLOBAL handle,memArray * obj,unsigned n)
{
 size_t i;
  for(i = 0;i < n;i++)
  {
    LX_OBJECT lxo;
    if(IsKbdTerminate() || bioEOF(handle)) break;
    bioReadBuffer(handle,&lxo,sizeof(LX_OBJECT));
    if(!ma_AddData(obj,&lxo,sizeof(LX_OBJECT),True)) break;
  }
  return True;
}

static tBool __NEAR__ __FASTCALL__ __ReadEntriesLX(BGLOBAL handle,memArray *obj)
{
 unsigned i;
 unsigned char j;
 unsigned char cnt,type;
 tUIntFast16 numobj = 0;
 LX_ENTRY _lxe;
 i = 0;
 while(1)
 {
   tBool is_eof;
   is_eof = False;
   cnt = bioReadByte(handle);
   type = bioReadByte(handle);
   if(!cnt) break;
   if(type) numobj = bioReadWord(handle);
   for(j = 0;j < cnt;j++,i++)
   {
     char size;
     switch(type)
     {
       case 0: size = 0; break;
       case 1: size = 2; break;
       case 2:
       case 0x80:
       case 3: size = 4; break;
       default:
       case 4: size = 6; break;
     }
     is_eof = bioEOF(handle);
     if(IsKbdTerminate() || is_eof) goto exit;
     _lxe.b32_type = type;
     if(size)
     {
       _lxe.b32_obj = numobj;
       _lxe.entry.e32_flags = bioReadByte(handle);
       bioReadBuffer(handle,&_lxe.entry.e32_variant,size);
     }
     if(!ma_AddData(obj,&_lxe,sizeof(LX_ENTRY),True)) goto exit;
   }
   if(is_eof) break;
 }
 exit:
 return True;
}

static void __FASTCALL__ lxReadPageDesc(BGLOBAL handle,LX_MAP_TABLE *mt,unsigned long pageidx)
{
  bioSeek(handle,headshift+lxe.lx.lxObjectPageTableOffset+
          sizeof(LX_MAP_TABLE)*(pageidx - 1),SEEK_SET);
  bioReadBuffer(handle,(void *)mt,sizeof(LX_MAP_TABLE));
}

static __filesize_t __NEAR__ __FASTCALL__ __calcPageEntry(LX_MAP_TABLE *mt)
{
  __filesize_t dataoff;
  __filesize_t ret;
  dataoff = mt->o32_pagedataoffset << lxe.lx.lxPageOffsetShift;
  switch(mt->o32_pageflags)
  {
    default:
    case PAGE_VALID: ret = lxe.lx.lxDataPagesOffset;
                     break;
    case PAGE_ITERDATA2: /* This is very undocumented type.
                            I do not know - how handle it !!!*/
    case PAGE_ITERDATA: ret = lxe.lx.lxObjectIterPageOffset;
                        break;
    case PAGE_RANGE:
    case PAGE_INVALID:
    case PAGE_ZEROED:  ret = 0; break;
  }
  return ret + dataoff;
}

static __filesize_t __NEAR__ __FASTCALL__ CalcPageEntry(unsigned long pageidx)
{
  BGLOBAL handle;
  LX_MAP_TABLE mt;
  if(!pageidx) return -1;
  handle = lx_cache;
  lxReadPageDesc(handle,&mt,pageidx);
  return __calcPageEntry((void *)&mt);
}

static __filesize_t __NEAR__ __FASTCALL__ CalcEntryPointLX(unsigned long objnum,__filesize_t _offset)
{
  BGLOBAL handle;
  unsigned long i,diff;
  LX_OBJECT lo;
  LX_MAP_TABLE mt;
  if(!objnum) return BMGetCurrFilePos();
  handle = lx_cache;
  bioSeek(handle,lxe.lx.lxObjectTableOffset + headshift,SEEK_SET);
  bioSeek(handle,sizeof(LX_OBJECT)*(objnum - 1),SEEKF_CUR);
  bioReadBuffer(handle,(void *)&lo,sizeof(LX_OBJECT));
  i = _offset / lxe.lx.lxPageSize;
  diff = _offset - i*lxe.lx.lxPageSize;
  lxReadPageDesc(handle,&mt,i+lo.o32_pagemap);
  return __calcPageEntry((void *)&mt) + diff;
}

static void __FASTCALL__ ReadLXLEImpMod(__filesize_t offtable,unsigned num,char *str)
{
  BGLOBAL handle;
  unsigned i;
  unsigned char len;
  char buff[256];
  handle = lx_cache;
  bioSeek(handle,offtable,SEEK_SET);
  for(i = 1;i < num;i++)
  {
    len = bioReadByte(handle);
    bioSeek(handle,len,SEEKF_CUR);
  }
  len = bioReadByte(handle);
  bioReadBuffer(handle,(void *)buff,len);
  buff[len] = 0;
  strcat(str,buff);
}

static void __FASTCALL__ ReadLXLEImpName(__filesize_t offtable,unsigned num,char *str)
{
  BGLOBAL handle;
  unsigned char len;
  char buff[256];
  handle = lx_cache;
  bioSeek(handle,offtable+num,SEEK_SET);
  len = bioReadByte(handle);
  bioReadBuffer(handle,(void *)buff,len);
  buff[len] = 0;
  strcat(str,buff);
}

void __FASTCALL__ ShowFwdModOrdLX(const LX_ENTRY *lxent)
{
  char buff[513];
  buff[0] = 0;
  ReadLXLEImpMod(lxe.lx.lxImportModuleTableOffset + headshift,lxent->entry.e32_variant.e32_fwd.modord,buff);
  strcat(buff,".");
  if((lxent->entry.e32_flags & 0x01) == 0x01)
  {
    sprintf(&buff[strlen(buff)],"@%u",(unsigned)lxent->entry.e32_variant.e32_fwd.value);
  }
  else ReadLXLEImpName(lxe.lx.lxImportProcedureTableOffset + headshift,(unsigned)lxent->entry.e32_variant.e32_fwd.value,buff);
  TMessageBox(buff," Forwarder entry point ");
}

static __filesize_t __NEAR__ __FASTCALL__ CalcEntryLX(const LX_ENTRY *lxent)
{
  __filesize_t ret;
  ret = BMGetCurrFilePos();
      switch(lxent->b32_type)
      {
        case 1: ret = CalcEntryPointLX(lxent->b32_obj,lxent->entry.e32_variant.e32_offset.offset16);
                      break;
        case 2: ret = CalcEntryPointLX(lxent->b32_obj,lxent->entry.e32_variant.e32_callgate.offset);
                      break;
        case 3: ret = CalcEntryPointLX(lxent->b32_obj,lxent->entry.e32_variant.e32_offset.offset32);
                      break;
        case 4: ShowFwdModOrdLX(lxent);
        case 5:
        default: break;
      }
  return ret;
}

static __filesize_t __NEAR__ __FASTCALL__ CalcEntryBungleLX(unsigned ordinal,tBool dispmsg)
{
  BGLOBAL handle;
  tBool found;
  unsigned i;
  unsigned char j;
  unsigned char cnt,type;
  tUIntFast16 numobj = 0;
  LX_ENTRY lxent;
  __filesize_t ret;
  ret = BMGetCurrFilePos();
  handle = lx_cache;
  bioSeek(handle,lxe.lx.lxEntryTableOffset + headshift,SEEK_SET);
  i = 0;
  found = False;
  while(1)
  {
   tBool is_eof;
   is_eof = False;
   cnt = bioReadByte(handle);
   type = bioReadByte(handle);
   if(!cnt) break;
   if(type) numobj = bioReadWord(handle);
   if(bioEOF(handle)) break;
   for(j = 0;j < cnt;j++,i++)
   {
     char size;
     switch(type)
     {
       case 0: size = 0; break;
       case 1: size = 2; break;
       case 2:
       case 0x80:
       case 3: size = 4; break;
       default:
       case 4: size = 6; break;
     }
     if(i == ordinal - 1)
     {
       lxent.b32_type = type;
       found = True;
       if(size)
       {
         lxent.b32_obj = numobj;
         lxent.entry.e32_flags = bioReadByte(handle);
         bioReadBuffer(handle,(void *)&lxent.entry.e32_variant,size);
         is_eof = bioEOF(handle);
       }
       break;
     }
     else
       if(size) bioSeek(handle,size + sizeof(char),SEEKF_CUR);
     if(is_eof) break;
   }
   if(found || is_eof) break;
 }
 if(found) ret = CalcEntryLX((LX_ENTRY *)&lxent);
 else      if(dispmsg) ErrMessageBox(NOT_ENTRY,NULL);
 return ret;
}

__filesize_t __FASTCALL__ ShowObjectsLX( void )
{
 BGLOBAL handle;
 __filesize_t fpos;
 unsigned nnames;
 memArray * obj;
 fpos = BMGetCurrFilePos();
 nnames = (unsigned)lxe.lx.lxObjectCount;
 if(!nnames) { NotifyBox(NOT_ENTRY," Objects Table "); return fpos; }
 if(!(obj = ma_Build(nnames,True))) return fpos;
 handle = lx_cache;
 bioSeek(handle,lxe.lx.lxObjectTableOffset + headshift,SEEK_SET);
 if(__ReadObjectsLX(handle,obj,nnames))
 {
  int ret;
    ret = PageBox(70,20,(const void **)obj->data,obj->nItems,ObjPaintLX);
    if(ret != -1)  fpos = LXType == FILE_LX ? CalcPageEntry(((LX_OBJECT *)obj->data[ret])->o32_pagemap) : CalcPageEntryLE(((const LX_OBJECT *)obj->data[ret])->o32_pagemap);
 }
 ma_Destroy(obj);
 return fpos;
}

const char * mapattr[] =
{
"Valid Physical Page In .EXE",
"Iterated Data Page",
"Invalid Page",
"Zero Filled Page",
"Range Of Pages",
"Iterated Data Page Type II"
};

const char * __FASTCALL__ lxeGetMapAttr(unsigned long attr)
{
  if (attr > 5) return "Unknown";
  else  return mapattr[attr];
}
#if 0
static void __NEAR__ __FASTCALL__ iterpaintLX(const LX_ITER *nam)
{
 twGotoXY(1,1);
 twPrintF("Number of iteration                  = %hu\n"
          "Number of bytes                      = %hu\n"
          "Iterated data bytes                  = %hu"
          ,nam->LX_nIter
          ,nam->LX_nBytes
          ,(int)nam->LX_Iterdata);
}
#endif
const char *__e32type[] =
{
  "Empty",
  "Entry16",
  "Gate16",
  "Entry32",
  "EntryForwarder",
  "TypeInfo"
};

static const char * __NEAR__ __FASTCALL__ entryTypeLX(unsigned char type)
{
   if(type < 6) return __e32type[type];
   else         return "Unknown";
}

static void __NEAR__ __FASTCALL__ entrypaintLX(const LX_ENTRY *nam)
{
 if(!nam->b32_type)
 {
   twGotoXY(35,4);
   twPrintF("Unused");
 }
 else
 {
   twGotoXY(1,1);
   twPrintF("Entry type: %s\n"
            "Object number : %hd\n"
            "Flags: %02XH\n"
            ,entryTypeLX(nam->b32_type)
            ,nam->b32_obj
            ,(int)nam->entry.e32_flags);
   if(nam->b32_type != 4)
   {
     twPrintF("   [%c] Exported Entry\n"
              "   [%c] Used Shared Data\n"
              "   %02XH - parameter word count mask\n"
              ,GetBool((nam->entry.e32_flags & 0x01) == 0x01)
              ,GetBool((nam->entry.e32_flags & 0x02) == 0x02)
              ,(int)(nam->entry.e32_flags >> 2));
   }
   else
   {
     twPrintF("\n"
              "   [%c] Import by ordinal\n"
              "\n"
              ,GetBool((nam->entry.e32_flags & 0x01) == 0x01));
   }
   if(nam->b32_type == 1)
   {
     twPrintF("Entry offset : %04hXH\n"
              "\n"
              ,nam->entry.e32_variant.e32_offset.offset16);
   }
   else
      if(nam->b32_type == 3)
      {
        twPrintF("Entry offset : %08XH\n"
                 "\n"
                 ,nam->entry.e32_variant.e32_offset.offset32);
      }
      else
      if(nam->b32_type == 2)
      {
       twPrintF("Callgate offset : %04hXH\n"
                "Callgate selector : %04hXH\n"
                ,nam->entry.e32_variant.e32_callgate.offset
                ,nam->entry.e32_variant.e32_callgate.callgate);
      }
      else
       if(nam->b32_type == 4)
       {
         twPrintF("Module ordinal number : %04hXH\n"
                  "Proc name offset or ordinal : %04hXH\n"
                  ,nam->entry.e32_variant.e32_fwd.modord
                  ,nam->entry.e32_variant.e32_fwd.value);
       }
   }
}
#if 0
static void __FASTCALL__ IterPaintLX(TWindow * win,const void ** names,unsigned start,unsigned nlist)
{
 char buffer[81];
 const LX_ITER ** nam = (const LX_ITER **)names;
 twUseWin(win);
 twFreezeWin(win);
 twClearWin();
 sprintf(buffer," Iter Table [ %u / %u ] ",start + 1,nlist);
 twSetTitleAttr(win,buffer,TW_TMODE_CENTER,dialog_cset.title);
 iterpaintLX(nam[start]);
 twRefreshFullWin(win);
}
#endif
static void __FASTCALL__ PaintEntriesLX(TWindow * win,const void ** names,unsigned start,unsigned nlist)
{
 char buffer[81];
 const LX_ENTRY ** nam = (const LX_ENTRY **)names;
 twUseWin(win);
 twFreezeWin(win);
 twClearWin();
 sprintf(buffer," Entries Table [ %u / %u ] ",start + 1,nlist);
 twSetTitleAttr(win,buffer,TW_TMODE_CENTER,dialog_cset.title);
 twSetFooterAttr(win,PAGEBOX_SUB,TW_TMODE_RIGHT,dialog_cset.selfooter);
 entrypaintLX(nam[start]);
 twRefreshFullWin(win);
}

static tBool __FASTCALL__ __ReadMapTblLX(BGLOBAL handle,memArray * obj,unsigned n)
{
  size_t i;
  for(i = 0;i < n;i++)
  {
    LX_MAP_TABLE mt;
    char stmp[80];
    if(IsKbdTerminate() || bioEOF(handle)) break;
    lxReadPageDesc(handle,&mt,i+1);
    sprintf(stmp,"Off=%08lXH Siz=%04hXH Flg:%04hXH=%s",
                 (unsigned long)mt.o32_pagedataoffset,
                 mt.o32_pagesize,
                 mt.o32_pageflags,
                 (const char *)lxeGetMapAttr(mt.o32_pageflags));
    if(!ma_AddString(obj,(const char *)stmp,True)) break;
  }
  return True;
}
#if 0
static tBool __NEAR__ __FASTCALL__ __ReadIterTblLX(BGLOBAL handle,memArray * obj,unsigned n)
{
 int i;
  for(i = 0;i < n;i++)
  {
    LX_ITER lxi;
    if(IsKbdTerminate() || bioEOF(handle)) break;
    bioReadBuffer(handle,&lxi,sizeof(LX_ITER));
    if(!ma_AddData(obj,&lxi,sizeof(LX_ITER),True)) break;
  }
  return True;
}
#endif
static unsigned __FASTCALL__ lxGetPageCount(BGLOBAL handle)
{
  UNUSED(handle);
  return (unsigned)lxe.lx.lxPageCount;
}

static __filesize_t __FASTCALL__ ShowMapTableLX( void )
{
 __filesize_t fpos;
 int ret;
 fpos = BMGetCurrFilePos();
 ret = fmtShowList(lxGetPageCount,__ReadMapTblLX," Map of pages ",
                   LB_SELECTIVE,NULL);
 if(ret != -1)
 {
   fpos = CalcPageEntry(ret + 1);
 }
 return fpos;
}

__filesize_t __FASTCALL__ ShowEntriesLX( void )
{
 BGLOBAL handle;
 __filesize_t fpos;
 memArray * obj;
 fpos = BMGetCurrFilePos();
 if(!lxe.lx.lxEntryTableOffset) { NotifyBox(NOT_ENTRY," Entry Table "); return fpos; }
 handle = lx_cache;
 bioSeek(handle,lxe.lx.lxEntryTableOffset + headshift,SEEK_SET);
 if(!(obj = ma_Build(0,True))) goto exit;
 if(__ReadEntriesLX(handle,obj))
 {
    int ret;
    if(!obj->nItems) { NotifyBox(NOT_ENTRY," Entry Table "); goto bye; }
    ret = PageBox(70,8,(const void **)obj->data,obj->nItems,PaintEntriesLX);
    if(ret != -1)  fpos = LXType == FILE_LX ? CalcEntryLX(obj->data[ret]) : CalcEntryLE(obj->data[ret]);
 }
 bye:
 ma_Destroy(obj);
 exit:
 return fpos;
}

const char * ResourceGrNamesLX[] =
{
  "RESERVED 0",
  "Mouse pointer share",
  "BITMAP",
  "MENU",
  "DIALOG",
  "STRINGTABLE",
  "FONTDIR",
  "FONT",
  "ACCELTABLE",
  "RCDATA",
  "Err msg table",
  "DlgInclude",
  "VKey table",
  "Key to UGL table",
  "Glyph to char table",
  "Display info",
  "FKA short",
  "FKA long",
  "Help table",
  "Help subtable",
  "DBCS font drive dir",
  "DBCS font drive"
};

static tBool __NEAR__ __FASTCALL__ __ReadResourceGroupLX(BGLOBAL handle,memArray *obj,unsigned nitems,long * addr)
{
 unsigned i;
 LXResource lxr;
 for(i = 0;i < nitems;i++)
 {
    char stmp[81];
    bioReadBuffer(handle,&lxr,sizeof(LXResource));
    addr[i] = lxr.offset;
    if(IsKbdTerminate() || bioEOF(handle)) break;
    sprintf(stmp,"%6hu = ",lxr.nameID);
    if(lxr.typeID < sizeof(ResourceGrNamesLX)/sizeof(char *)) strcat(stmp,ResourceGrNamesLX[lxr.typeID]);
    else  sprintf(&stmp[strlen(stmp)],"Unknown < %04hXH >",lxr.typeID);
    sprintf(&stmp[strlen(stmp)]," obj=%04hXH.%08lXH"
                               ,lxr.object
                               ,(unsigned long)lxr.offset);
    if(!ma_AddString(obj,stmp,True)) break;
 }
 return True;
}

static __filesize_t __FASTCALL__ ShowResourceLX( void )
{
 __filesize_t fpos;
 BGLOBAL handle;
 memArray * obj;
 long * raddr;
 unsigned nrgroup;
 fpos = BMGetCurrFilePos();
 handle = lx_cache;
 bioSeek(handle,(__fileoff_t)headshift + lxe.lx.lxResourceTableOffset,SEEK_SET);
 nrgroup = (unsigned)lxe.lx.lxNumberResourceTableEntries;
 if(!nrgroup) { NotifyBox(NOT_ENTRY," Resources "); return fpos; }
 if(!(obj = ma_Build(nrgroup,True))) goto exit;
 if(!(raddr  = PMalloc(nrgroup*sizeof(long)))) return fpos;
 if(__ReadResourceGroupLX(handle,obj,nrgroup,raddr))
 {
   ma_Display(obj," Resource groups : ",LB_SORTABLE,-1);
 }
 ma_Destroy(obj);
 free(raddr);
 exit:
 return fpos;
}

__filesize_t __FASTCALL__ ShowModRefLX( void )
{
  fmtShowList(LXModRefNumItems,
              __ReadModRefNamesLX,
              MOD_REFER,
              LB_SORTABLE,
              NULL);
  return BMGetCurrFilePos();
}

static __filesize_t __FASTCALL__ ShowResNamLX( void )
{
  __filesize_t fpos = BMGetCurrFilePos();
  int ret;
  unsigned ordinal;
  ret = fmtShowList(LXRNamesNumItems,LXRNamesReadItems,
                    RES_NAMES,
                    LB_SELECTIVE | LB_SORTABLE,&ordinal);
  if(ret != -1)
  {
    fpos = CalcEntryBungleLX(ordinal,True);
  }
  return fpos;
}

static __filesize_t __FASTCALL__ ShowNResNmLX( void )
{
  __filesize_t fpos;
  fpos = BMGetCurrFilePos();
  {
    int ret;
    unsigned ordinal;
    ret = fmtShowList(LXNRNamesNumItems,LXNRNamesReadItems,
                      NORES_NAMES,
                      LB_SELECTIVE | LB_SORTABLE,&ordinal);
    if(ret != -1)
    {
      fpos = CalcEntryBungleLX(ordinal,True);
    }
  }
  return fpos;
}

__filesize_t __FASTCALL__ ShowImpProcLXLE( void )
{
  fmtShowList(LXImpNamesNumItems,LXImpNamesReadItems,
              IMPPROC_TABLE,0,NULL);
  return BMGetCurrFilePos();
}

static tBool __FASTCALL__ isLX( void )
{
   char id[4];
   headshift = IsNewExe();
   bmReadBufferEx(id,sizeof(id),headshift,SEEKF_START);
   if(id[0] == 'L' && id[1] == 'X' && id[2] == 0 && id[3] == 0) return True;
   return False;
}

static void __FASTCALL__ LXinit( void )
{
   BGLOBAL main_handle;
   LXType = FILE_LX;
   bmReadBufferEx(&lxe.lx,sizeof(LXHEADER),headshift,SEEKF_START);
   main_handle = bmbioHandle();
   if((lx_cache = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) lx_cache = main_handle;
}

static void __FASTCALL__ LXdestroy( void )
{
   BGLOBAL main_handle;
   main_handle = bmbioHandle();
   if(lx_cache != &bNull && lx_cache != main_handle) bioClose(lx_cache);
}

static __filesize_t __FASTCALL__ LXHelp( void )
{
  hlpDisplay(10005);
  return BMGetCurrFilePos();
}

static __filesize_t __FASTCALL__ lxVA2PA(__filesize_t va)
{
 /* First we must determine object number for given virtual address */
  BGLOBAL handle;
  unsigned long i,diff,oidx;
  __filesize_t rva,pa;
  LX_OBJECT lo;
  LX_MAP_TABLE mt;
  handle = lx_cache;
  bioSeek(handle,lxe.lx.lxObjectTableOffset + headshift,SEEK_SET);
  pa = oidx = 0; /* means: error */
  for(i = 0;i < lxe.lx.lxObjectCount;i++)
  {
    bioReadBuffer(handle,(void *)&lo,sizeof(LX_OBJECT));
    if(lo.o32_base <= va && va < lo.o32_base + lo.o32_size)
    {
      oidx = i+1;
      break;
    }
  }
  /* Secondly we must determine page within object */
  if(oidx)
  {
    rva = va - lo.o32_base;
    i = rva / lxe.lx.lxPageSize;
    diff = rva - i*lxe.lx.lxPageSize;
    lxReadPageDesc(handle,&mt,i+lo.o32_pagemap);
    pa = __calcPageEntry((void *)&mt) + diff;
  }
  return pa;
}

static __filesize_t __FASTCALL__ lxPA2VA(__filesize_t pa)
{
 /* First we must determine page for given physical address */
  BGLOBAL handle;
  unsigned long i,pidx,pagentry;
  __filesize_t rva,va;
  LX_OBJECT lo;
  LX_MAP_TABLE mt;
  handle = lx_cache;
  pagentry = va = pidx = 0;
  for(i = 0;i < lxe.lx.lxPageCount;i++)
  {
    lxReadPageDesc(handle,&mt,i+1);
    pagentry = __calcPageEntry(&mt);
    if(pagentry <= pa && pa < pagentry + mt.o32_pagesize)
    {
      pidx = i+1;
      break;
    }
  }
  /* Secondly we must determine object number for given physical address */
  if(pidx)
  {
    rva = pa - pagentry + (pidx-1)*lxe.lx.lxPageSize;
    bioSeek(handle,lxe.lx.lxObjectTableOffset + headshift,SEEK_SET);
    for(i = 0;i < lxe.lx.lxObjectCount;i++)
    {
      bioReadBuffer(handle,(void *)&lo,sizeof(LX_OBJECT));
      if(lo.o32_pagemap <= pidx && pidx < lo.o32_pagemap + lo.o32_mapsize)
      {
        va = lo.o32_base + rva;
        break;
      }
    }
  }
  return va;
}

static int __FASTCALL__ lxBitness(__filesize_t pa)
{
 /* First we must determine page for given physical address */
  BGLOBAL handle;
  unsigned long i,pidx,pagentry;
  int ret;
  LX_OBJECT lo;
  LX_MAP_TABLE mt;
  handle = lx_cache;
  pagentry = pidx = 0;
  ret = DAB_USE16;
  for(i = 0;i < lxe.lx.lxPageCount;i++)
  {
    lxReadPageDesc(handle,&mt,i+1);
    pagentry = __calcPageEntry(&mt);
    if(pagentry <= pa && pa < pagentry + mt.o32_pagesize)
    {
      pidx = i+1;
      break;
    }
  }
  /* Secondly we must determine object number for given physical address */
  if(pidx)
  {
    bioSeek(handle,lxe.lx.lxObjectTableOffset + headshift,SEEK_SET);
    for(i = 0;i < lxe.lx.lxObjectCount;i++)
    {
      bioReadBuffer(handle,(void *)&lo,sizeof(LX_OBJECT));
      if(lo.o32_pagemap <= pidx && pidx < lo.o32_pagemap + lo.o32_mapsize)
      {
        ret = (lo.o32_flags & 0x00002000L) == 0x00002000L ? DAB_USE32 : DAB_USE16;
        break;
      }
    }
  }
  return ret;
}

static tBool __FASTCALL__ lxAddressResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  tBool bret = True;
  tUInt32 res;
  if(cfpos >= headshift && cfpos < headshift + sizeof(LXHEADER))
  {
    strcpy(addr,"LXH :");
    strcpy(&addr[5],Get4Digit(cfpos - headshift));
  }
  else
  if(cfpos >= headshift + lxe.lx.lxObjectTableOffset &&
     cfpos <  headshift + lxe.lx.lxObjectTableOffset + sizeof(LX_OBJECT)*lxe.lx.lxObjectCount)
  {
    strcpy(addr,"LXOD:");
    strcpy(&addr[5],Get4Digit(cfpos - headshift - lxe.lx.lxObjectTableOffset));
  }
  else
  if(cfpos >= headshift + lxe.lx.lxObjectPageTableOffset &&
     cfpos <  headshift + lxe.lx.lxObjectPageTableOffset + sizeof(LX_MAP_TABLE)*lxe.lx.lxPageCount)
  {
    strcpy(addr,"LXPD:");
    strcpy(&addr[5],Get4Digit(cfpos - headshift - lxe.lx.lxObjectPageTableOffset));
  }
  else
   if((res=lxPA2VA(cfpos))!=0)
   {
     addr[0] = '.';
     strcpy(&addr[1],Get8Digit(res));
   }
   else bret = False;
  return bret;
}

static int __FASTCALL__ lxPlatform( void ) { return DISASM_CPU_IX86; }

REGISTRY_BIN lxTable =
{
  "LX (Linear eXecutable)",
  { "LXhelp", "Import", "ResNam", "NRsNam", "ImpNam", "Entry ", "ResTbl", "LXHead", "MapTbl", "Object" },
  { LXHelp, ShowModRefLX, ShowResNamLX, ShowNResNmLX, ShowImpProcLXLE, ShowEntriesLX, ShowResourceLX, ShowNewHeaderLX, ShowMapTableLX, ShowObjectsLX },
  isLX, LXinit, LXdestroy,
  NULL,
  NULL,
  NULL,
  lxPlatform,
  lxBitness,
  NULL,
  lxAddressResolv,
  lxVA2PA,
  lxPA2VA,
  NULL,
  NULL,
  NULL,
  NULL
};
