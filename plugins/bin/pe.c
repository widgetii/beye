/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/pe.c
 * @brief       This file contains implementation of PE (Portable Executable)
 *              file format decoder.
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
 * @date        12.09.2000
 * @note        Some useful patches
**/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#include "colorset.h"
#include "plugins/bin/pe.h"
#include "plugins/disasm.h"
#include "bin_util.h"
#include "codeguid.h"
#include "bmfile.h"
#include "biewhelp.h"
#include "tstrings.h"
#include "biewutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"
#include "biewlib/pmalloc.h"

static PE_ADDR * peVA = NULL;
PEHEADER pe;

static BGLOBAL pe_cache = &bNull;
static BGLOBAL pe_cache1 = &bNull;
static BGLOBAL pe_cache2 = &bNull;
static BGLOBAL pe_cache3 = &bNull;
static BGLOBAL pe_cache4 = &bNull;

static tBool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,unsigned long pa);
static void __FASTCALL__ pe_ReadPubNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *));
static unsigned long __FASTCALL__ peVA2PA(unsigned long va);
static unsigned long __FASTCALL__ pePA2VA(unsigned long pa);

static tBool __FASTCALL__ peLowMemFunc( unsigned long need_mem )
{
  UNUSED(need_mem);
  if(!fmtActiveState)
  {
    if(PubNames)
    {
       la_Destroy(PubNames);
       PubNames = NULL;
       return True;
    }
  }
  return False;
}

static unsigned long __NEAR__ __FASTCALL__ CalcPEObjectEntry(long offset)
{
 unsigned long intp;
 intp = offset / pe.peFileAlign;
 if(offset % pe.peFileAlign) offset = ( offset / intp ) * intp;
 return offset;
}

static unsigned long __NEAR__ __FASTCALL__ RVA2Phys(unsigned long rva)
{
 int i;
 unsigned long ret,npages,size;
 unsigned long poff,obj_rva,pphys;
 for(i = pe.peObjects - 1;i >= 0;i--)
 {
   if(rva >= peVA[i].rva) break;
   if(IsKbdTerminate()) return 0;
 }
 if(i < 0) return 0;
 pphys = peVA[i].phys;
 obj_rva = peVA[i].rva;
 /* Added by Kostya Nosov <k-nosov@yandex.ru> */
 if (i < pe.peObjects - 1)
 {
   size = peVA[i+1].phys - pphys;
   if (rva - obj_rva > size) return 0;
 }
 /** each page is 4096 bytes
     each object can contain several pages
     it now contains previous object that contains this page */
 if(!pphys) return 0;
 npages = (rva - obj_rva) / 4096UL;
 poff = (rva - obj_rva) % 4096UL;
 ret = pphys + npages*4096UL + poff;
 return ret;
}

static unsigned long __NEAR__ __FASTCALL__ fioReadDWord(BGLOBAL handle,unsigned long offset,int origin)
{
 bioSeek(handle,offset,origin);
 return bioReadDWord(handle);
}

static unsigned __NEAR__ __FASTCALL__ fioReadWord(BGLOBAL handle,unsigned long offset,int origin)
{
 bioSeek(handle,offset,origin);
 return bioReadWord(handle);
}

static unsigned long __NEAR__ __FASTCALL__ fioReadDWord2Phys(BGLOBAL handle,unsigned long offset,int origin)
{
 unsigned long dword;
 dword = fioReadDWord(handle,offset,origin);
 return RVA2Phys(dword);
}

const char * __pecputype[] =
{
 "80386",
 "80486",
 "80586",
 "80686",
 "80787",
 "80887",
 "80987",
 "801087"
};

static const char * __NEAR__ __FASTCALL__ PECPUType(void)
{
  const char * cptr;
  if(pe.peCPUType >= 0x014C && pe.peCPUType < 0x0150) cptr = __pecputype[(pe.peCPUType - 0x014C) & 0x0007];
  else
    if(pe.peCPUType == 0x0162) cptr = "__MIPS Mark I (R2000, R3000)";
    else
      if(pe.peCPUType == 0x0163) cptr = "__MIPS Mark II (R6000)";
      else
        if(pe.peCPUType == 0x0166) cptr = "__MIPS Mark III (R4000)";
        else                            cptr = __pecputype[0];
  return cptr;
}

static unsigned long entryPE = 0L;

static void __NEAR__ PaintNewHeaderPE_1( void )
{
  twPrintF("Signature                      = '%c%c'\n"
           "Required CPU Type              = %s\n"
           "Number of object entries       = %hu\n"
           "Time/Data Stamp                = %s"
           "NT header size                 = %hu bytes\n"
           "Image flags :                    [%04hXH]\n"
           "    [%c] < Relocation info stripped >\n"
           "    [%c] Image is executable\n"
           "    [%c] < Line number stripped >\n"
           "    [%c] < Local symbols stripped >\n"
           "    [%c] < Minimal object >\n"
           "    [%c] < Update object >\n"
           "    [%c] < 16 bit word machine >\n"
           "    [%c] < 32 bit word machine >\n"
           "    [%c] Fixed\n"
           "    [%c] < System file >\n"
           "    [%c] Library image\n"
           "Linker version                 = %u.%02u\n"
           ,pe.peSignature[0],pe.peSignature[1]
           ,PECPUType()
           ,pe.peObjects
           ,ctime((time_t *)&pe.peTimeDataStamp)
           ,pe.peNTHdrSize
           ,pe.peFlags
           ,GetBool(pe.peFlags & 0x0001)
           ,GetBool(pe.peFlags & 0x0002)
           ,GetBool(pe.peFlags & 0x0004)
           ,GetBool(pe.peFlags & 0x0008)
           ,GetBool(pe.peFlags & 0x0010)
           ,GetBool(pe.peFlags & 0x0020)
           ,GetBool(pe.peFlags & 0x0040)
           ,GetBool(pe.peFlags & 0x0100)
           ,GetBool(pe.peFlags & 0x0200)
           ,GetBool(pe.peFlags & 0x1000)
           ,GetBool(pe.peFlags & 0x2000)
           ,(int)pe.peLMajor,(int)pe.peLMinor);
  twSetColorAttr(dialog_cset.entry);
  twPrintF("EntryPoint RVA    %s = %08lXH (Offset: %08lXH)",pe.peFlags & 0x2000 ? "[ LibEntry ]" : "[ EXEEntry ]",pe.peEntryPointRVA,entryPE); twClrEOL();
  twSetColorAttr(dialog_cset.main);
  twPrintF("\nImage base                     = %08lXH\n"
           "Object aligning                = %08lXH"
           ,pe.peImageBase
           ,pe.peObjectAlign);
}

const char * __pesubsyst[] =
{
  "Unknown",
  "Native",
  "Windows GUI",
  "Windows Character",
  "OS/2 GUI",
  "OS/2 Character",
  "Posix GUI",
  "Posix Character"
};

static void __NEAR__ PaintNewHeaderPE_2( void )
{
  twPrintF("File align                     = %08lXH\n"
           "OS version                     = %hu.%hu\n"
           "User version                   = %hu.%hu\n"
           "Subsystem version              = %hu.%hu\n"
           "Image size                     = %lu bytes\n"
           "Header size                    = %lu bytes\n"
           "File checksum                  = %08lXH\n"
           "NT subsystem                   = %s\n"
           "DLL Flags :                      [%04hXH]\n"
           " [%c] Per-Process Library initialization\n"
           " [%c] Per-Process Library termination\n"
           " [%c] Per-Thread  Library initialization\n"
           " [%c] Per-Thread  Library termination\n"
           "Stack reserve size             = %lu bytes\n"
           "Stack commit size              = %lu bytes\n"
           "Heap reserve size              = %lu bytes\n"
           "Heap commit size               = %lu bytes\n"
           "Number interesting VA/Sizes    = %lu bytes\n"
           "Export Table RVA               = %08lXH\n"
           "Total export data size         = %lu bytes\n"
           "Import Table RVA               = %08lXH"
           ,pe.peFileAlign
           ,pe.peOSMajor,pe.peOSMinor
           ,pe.peUserMajor,pe.peUserMinor
           ,pe.peSubSystMajor,pe.peSubSystMinor
           ,pe.peImageSize
           ,pe.peHeaderSize
           ,pe.peFileChecksum
           ,pe.peSubSystem <= 0x0007 ? __pesubsyst[pe.peSubSystem] : "Unknown"
           ,pe.peDLLFlags
           ,GetBool(pe.peDLLFlags & 0x0001)
           ,GetBool(pe.peDLLFlags & 0x0002)
           ,GetBool(pe.peDLLFlags & 0x0004)
           ,GetBool(pe.peDLLFlags & 0x0008)
           ,pe.peStackReserveSize
           ,pe.peStackCommitSize
           ,pe.peHeapReserveSize
           ,pe.peHeapCommitSize
           ,pe.peInterestingVASize
           ,pe.peExportTableRVA
           ,pe.peTotalExportDataSize
           ,pe.peImportTableRVA);
}

static void __NEAR__ PaintNewHeaderPE_3( void )
{
  twPrintF("Total import data size            = %lu bytes\n"
           "Resource Table RVA                = %08lXH\n"
           "Total resource data size          = %lu bytes\n"
           "Exception Table RVA               = %08lXH\n"
           "Total exception data size         = %lu bytes\n"
           "Security Table RVA                = %08lXH\n"
           "Total security data size          = %lu bytes\n"
           "Fixup Table RVA                   = %08lXH\n"
           "Total fixup data size             = %lu bytes\n"
           "Debug Table RVA                   = %08lXH\n"
           "Total number of debug directories = %lu bytes\n"
           "Image Description string RVA      = %08lXH\n"
           "Total description data size       = %lu bytes\n"
           "Machine Specific value RVA        = %08lXH\n"
           "Machine specific value size       = %lu bytes\n"
           "Thread Local Storage RVA          = %08lXH\n"
           "Total Thread Local Storage size   = %lu bytes"
           ,pe.peTotalImportDataSize
           ,pe.peResourceTableRVA
           ,pe.peTotalResourceDataSize
           ,pe.peExceptionTableRVA
           ,pe.peTotalExceptionDataSize
           ,pe.peSecurityTableRVA
           ,pe.peTotalSecurityDataSize
           ,pe.peFixupTableRVA
           ,pe.peTotalFixupDataSize
           ,pe.peDebugTableRVA
           ,pe.peTotalDebugDirectories
           ,pe.peImageDescriptionRVA
           ,pe.peTotalDescriptionSize
           ,pe.peMachineSpecificRVA
           ,pe.peMachineSpecificSize
           ,pe.peThreadLocalStorageRVA
           ,pe.peTotalTLSSize);
}

static void (__NEAR__ * pephead[])( void ) =
{
 PaintNewHeaderPE_1,
 PaintNewHeaderPE_2,
 PaintNewHeaderPE_3
};

static void __FASTCALL__ PaintNewHeaderPE(TWindow * win,const void **ptr,unsigned npage,unsigned tpage)
{
  char text[80];
  UNUSED(ptr);
  twUseWin(win);
  twFreezeWin(win);
  twClearWin();
  sprintf(text," Portable Executable Header [%d/%d] ",npage + 1,tpage);
  twSetTitleAttr(win,text,TW_TMODE_CENTER,dialog_cset.title);
  twSetFooterAttr(win,PAGEBOX_SUB,TW_TMODE_RIGHT,dialog_cset.selfooter);
  if(npage < 3)
  {
    twGotoXY(1,1);
    (*(pephead[npage]))();
  }
  twRefreshFullWin(win);
}

static unsigned long __FASTCALL__ ShowNewHeaderPE( void )
{
 long fpos;
 fpos = BMGetCurrFilePos();
 entryPE = RVA2Phys(pe.peEntryPointRVA);
 if(PageBox(70,21,NULL,3,PaintNewHeaderPE) != -1 && entryPE && entryPE < bmGetFLength()) fpos = entryPE;
 return fpos;
}

static void __FASTCALL__ ObjPaintPE(TWindow * win,const void ** names,unsigned start,unsigned nlist)
{
 char buffer[81];
 const PE_OBJECT ** nam = (const PE_OBJECT **)names;
 const PE_OBJECT *  objs = nam[start];
 twUseWin(win);
 twFreezeWin(win);
 twClearWin();
 sprintf(buffer," Object Table [ %u / %u ] ",start + 1,nlist);
 twSetTitleAttr(win,buffer,TW_TMODE_CENTER,dialog_cset.title);
 twSetFooterAttr(win,PAGEBOX_SUB,TW_TMODE_RIGHT,dialog_cset.selfooter);
 twGotoXY(1,1);
 twPrintF("Object Name                    = %8s\n"
          "Virtual Size                   = %lX bytes\n"
          "RVA (relative virtual address) = %08lX\n"
          "Physical size                  = %08lX bytes\n"
          "Physical offset                = %08lX bytes\n"
          "<Pointer to relocations>       = %08lX\n"
          "<Pointer to line numbers>      = %08lX\n"
          "<Number of relocations>        = %hu\n"
          "<Number of line numbers>       = %hu\n"
          "FLAGS: %lX\n"
          "   [%c] Code object\n"
          "   [%c] Initialized data object\n"
          "   [%c] Uninitialized data object\n"
          "   [%c] Object must not be cashed\n"
          "   [%c] Object is not pageable\n"
          "   [%c] Object is shared\n"
          "   [%c] Executable object\n"
          "   [%c] Readable object\n"
          "   [%c] Writable object"
          ,objs->oName
          ,objs->oVirtualSize
          ,objs->oRVA
          ,objs->oPhysicalSize
          ,objs->oPhysicalOffset
          ,objs->oRelocPtr
          ,objs->oLineNumbPtr
          ,objs->oNReloc
          ,objs->oNLineNumb
          ,objs->oFlags
          ,GetBool((objs->oFlags & 0x00000020UL) == 0x00000020UL)
          ,GetBool((objs->oFlags & 0x00000040UL) == 0x00000040UL)
          ,GetBool((objs->oFlags & 0x00000080UL) == 0x00000080UL)
          ,GetBool((objs->oFlags & 0x04000000UL) == 0x04000000UL)
          ,GetBool((objs->oFlags & 0x08000000UL) == 0x08000000UL)
          ,GetBool((objs->oFlags & 0x10000000UL) == 0x10000000UL)
          ,GetBool((objs->oFlags & 0x20000000UL) == 0x20000000UL)
          ,GetBool((objs->oFlags & 0x40000000UL) == 0x40000000UL)
          ,GetBool((objs->oFlags & 0x80000000UL) == 0x80000000UL));
 twRefreshFullWin(win);
}

static tBool __NEAR__ __FASTCALL__ __ReadObjectsPE(BGLOBAL handle,memArray * obj,unsigned n)
{
 unsigned i;
  for(i = 0;i < n;i++)
  {
    PE_OBJECT po;
    if(IsKbdTerminate() || bioEOF(handle)) break;
    bioReadBuffer(handle,&po,sizeof(PE_OBJECT));
    if(!ma_AddData(obj,&po,sizeof(PE_OBJECT),True)) break;
  }
  return True;
}

static unsigned long __FASTCALL__ ShowObjectsPE( void )
{
 BGLOBAL handle;
 unsigned nnames;
 unsigned long fpos;
 memArray * obj;
 fpos = BMGetCurrFilePos();
 nnames = pe.peObjects;
 if(!nnames) { NotifyBox(NOT_ENTRY," Objects Table "); return fpos; }
 if(!(obj = ma_Build(nnames,True))) return fpos;
 handle = pe_cache;
 bioSeek(handle,0x18 + pe.peNTHdrSize + headshift,SEEK_SET);
 if(__ReadObjectsPE(handle,obj,nnames))
 {
  int ret;
    ret = PageBox(70,19,(const void **)obj->data,obj->nItems,ObjPaintPE);
    if(ret != -1)  fpos = CalcPEObjectEntry(((PE_OBJECT *)obj->data[ret])->oPhysicalOffset);
 }
 ma_Destroy(obj);
 return fpos;
}

static unsigned __NEAR__ __FASTCALL__ GetImportCountPE(BGLOBAL handle,unsigned long phys)
{
  unsigned count;
  unsigned long fpos = bioTell(handle);
  unsigned long ctrl;
  count = 0;
  bioSeek(handle,phys,SEEKF_START);
  while(1)
  {
    ctrl = fioReadDWord(handle,12L,SEEKF_CUR);
    bioSeek(handle,4L,SEEKF_CUR);
    if(ctrl == 0 || count > 0xFFFD || IsKbdTerminate() || bioEOF(handle)) break;
    count++;
  }
  bioSeek(handle,fpos,SEEKF_START);
  return count;
}

/* returns really readed number of characters */
unsigned __NEAR__ __FASTCALL__ __peReadASCIIZName(BGLOBAL handle,unsigned long offset,char *buff, unsigned cb_buff)
{
  int j;
  char ch;
  unsigned long fpos;
  fpos = bioTell(handle);
  j = 0;
  bioSeek(handle,offset,SEEKF_START);
  while(1)
  {
    ch = bioReadByte(handle);
    buff[j++] = ch;
    if(!ch || j >= cb_buff || bioEOF(handle)) break;
  }
  if(j) buff[j-1]=0;
  bioSeek(handle,fpos,SEEKF_START);
  return j;
}

static unsigned __NEAR__ __FASTCALL__ __ReadImportPE(BGLOBAL handle,unsigned long phys,memArray *obj,unsigned nnames)
{
  unsigned i;
  unsigned long fpos = bioTell(handle);
  unsigned long rva,addr;
  bioSeek(handle,phys,SEEKF_START);
  for(i = 0;i < nnames;i++)
  {
    char tmp[300];
    tBool is_eof;
    rva = fioReadDWord(handle,12L,SEEKF_CUR);
    bioSeek(handle,4L,SEEKF_CUR);
    addr = RVA2Phys(rva);
    __peReadASCIIZName(handle,addr,tmp,sizeof(tmp));
    if(IsKbdTerminate()) break;
    is_eof = bioEOF(handle);
    if(!ma_AddString(obj,is_eof ? CORRUPT_BIN_MSG : tmp,True)) break;
    if(is_eof) break;
  }
  bioSeek(handle,fpos,SEEKF_START);
  return obj->nItems;
}

static unsigned long addr_shift_pe = 0L;

static unsigned __FASTCALL__ GetImpCountPE(BGLOBAL handle)
{
 unsigned count;
 unsigned long Hint;
 count = 0;
 if(addr_shift_pe)
 {
   bioSeek(handle,addr_shift_pe,SEEKF_START);
   while(1)
   {
     Hint = bioReadDWord(handle);
     if(Hint == 0L || count > 0xFFFD || IsKbdTerminate() || bioEOF(handle)) break;
     count++;
   }
 }
 return count;
}

static tBool __FASTCALL__  __ReadImpContPE(BGLOBAL handle,memArray * obj,unsigned nnames)
{
  unsigned i,VA;
  unsigned long Hint,rphys;
  bioSeek(handle,addr_shift_pe,SEEKF_START);
  VA = pePA2VA(addr_shift_pe);
  for(i = 0;i < nnames;i++)
  {
    char stmp[300];
    tBool is_eof;
    sprintf(stmp,".%08X: ",VA);
    VA += 4; /* sizeof(unsigned) ?*/
    Hint = bioReadDWord(handle);
    if(!(Hint & 0x80000000UL))
    {
      rphys = RVA2Phys(Hint & 0x7FFFFFFFUL);
      if(rphys > bmGetFLength() || bioEOF(handle))
      {
        if(!ma_AddString(obj,CORRUPT_BIN_MSG,True)) break;
      }
      else
      {
        char tmp[256];
        __peReadASCIIZName(handle,rphys+2,tmp,sizeof(tmp));
        strcat(stmp,tmp);
      }
    }
    else
    {
      char tmp[80];
      sprintf(tmp,"< By ordinal >   @%lu",Hint & 0x7FFFFFFFUL);
      strcat(stmp,tmp);
    }
    is_eof = bioEOF(handle);
    if(!ma_AddString(obj,is_eof ? CORRUPT_BIN_MSG : stmp,True)) break;
    if(IsKbdTerminate() || is_eof) break;
  }
  return True;
}

static unsigned long __FASTCALL__ ShowModRefPE( void )
{
  BGLOBAL handle;
  char petitle[80];
  memArray * obj;
  unsigned nnames;
  unsigned long phys,fret;
  fret = BMGetCurrFilePos();
  if(!pe.peImportTableRVA) { not_found: NotifyBox(NOT_ENTRY," Module References "); return fret; }
  handle = pe_cache;
  bioSeek(handle,0L,SEEK_SET);
  phys = RVA2Phys(pe.peImportTableRVA);
  if(!(nnames = GetImportCountPE(handle,phys))) goto not_found;
  if(!(obj = ma_Build(nnames,True))) goto exit;
  if(__ReadImportPE(handle,phys,obj,nnames))
  {
    int i;
    i = 0;
    while(1)
    {
     ImportDirPE imp_pe;
     unsigned long magic;

     i = ma_Display(obj,MOD_REFER,LB_SELECTIVE,i);
     if(i == -1) break;
     sprintf(petitle,"%s%s ",IMPPROC_TABLE,(char *)obj->data[i]);
     bioSeek(handle,phys + i*sizeof(ImportDirPE),SEEKF_START);
     bioReadBuffer(handle,&imp_pe,sizeof(ImportDirPE));
     if(bioEOF(handle)) break;
     if(!(imp_pe.idMajVer == 0 && imp_pe.idMinVer == 0 && imp_pe.idDateTime != 0xFFFFFFFFUL))
                                   magic = imp_pe.idFlags;
     else                          magic = imp_pe.idLookupTableRVA;
     addr_shift_pe = magic ? RVA2Phys(magic) : magic;
     fmtShowList(GetImpCountPE,__ReadImpContPE,petitle,0,NULL);
    }
  }
  ma_Destroy(obj);
  exit:
  return fret;
}

static ExportTablePE et;

static tBool __FASTCALL__ PEExportReadItems(BGLOBAL handle,memArray * obj,unsigned nnames)
{
  unsigned long nameaddr;
  unsigned long expaddr,nameptr,*addr;
  unsigned i,ord;
  char buff[40];
  nameptr = RVA2Phys(et.etNamePtrTableRVA);
  expaddr  = RVA2Phys(et.etOrdinalTableRVA);
  if(!(addr = PMalloc(sizeof(unsigned long)*nnames))) return True;
  bioSeek(handle,RVA2Phys(et.etAddressTableRVA),SEEKF_START);
  bioReadBuffer(handle,addr,sizeof(unsigned long)*nnames);
  for(i = 0;i < et.etNumNamePtrs;i++)
  {
    char stmp[300];
    tBool is_eof;
    nameaddr = fioReadDWord2Phys(handle,nameptr + 4*i,SEEKF_START);
    __peReadASCIIZName(handle,nameaddr, stmp, sizeof(stmp)-sizeof(buff)-1);
    if(IsKbdTerminate()) break;
    ord = fioReadWord(handle,expaddr + i*2,SEEKF_START);
    is_eof = bioEOF(handle);
    sprintf(buff,"%c%-9lu .%08lX",LB_ORD_DELIMITER, ord+(unsigned long)et.etOrdinalBase,addr[ord]+pe.peImageBase);
    addr[ord] = 0;
    strcat(stmp,buff);
    if(!ma_AddString(obj,is_eof ? CORRUPT_BIN_MSG : stmp,True)) { PFree(stmp); break; }
    if(is_eof) break;
  }
  for(i = 0;i < nnames;i++)
  {
    if(addr[i])
    {
      ord = i+et.etOrdinalBase;
      sprintf(buff," < by ordinal > %c%-5lu .%08lX",LB_ORD_DELIMITER, (unsigned long)ord,addr[i]+pe.peImageBase);
      if(!ma_AddString(obj,buff,True)) break;
    }
  }
  PFree(addr);
  return True;
}

static unsigned __FASTCALL__ PEExportNumItems(BGLOBAL handle)
{
  unsigned long addr;
  if(!pe.peExportTableRVA) return 0;
  addr = RVA2Phys(pe.peExportTableRVA);
  bioSeek(handle,addr,SEEKF_START);
  bioReadBuffer(handle,(void *)&et,sizeof(et));
  return (unsigned)(et.etNumEATEntries);
}

static unsigned long __NEAR__ __FASTCALL__ CalcEntryPE(unsigned ordinal,tBool dispmsg)
{
 unsigned long fret,rva;
 unsigned ord;
 BGLOBAL handle;
 fret = BMGetCurrFilePos();
 handle = pe_cache1;
 {
   unsigned long eret;
   rva = RVA2Phys(et.etAddressTableRVA);
   ord = (unsigned)ordinal - (unsigned)et.etOrdinalBase;
   eret = fioReadDWord2Phys(handle,rva + 4*ord,SEEKF_START);
   if(eret && eret < bmGetFLength()) fret = eret;
   else if(dispmsg) ErrMessageBox(NO_ENTRY,BAD_ENTRY);
 }
 return fret;
}

static unsigned long __FASTCALL__ ShowExpNamPE( void )
{
  unsigned long fpos = BMGetCurrFilePos();
  int ret;
  unsigned ordinal;
  unsigned long addr;
  char exp_nam[256], exp_buf[300];
  fpos = BMGetCurrFilePos();
  strcpy(exp_nam,EXP_TABLE);
  if(pe.peExportTableRVA)
  {
    addr = RVA2Phys(pe.peExportTableRVA);
    bmSeek(addr,SEEKF_START);
    bmReadBuffer((void *)&et,sizeof(et));
    if(et.etNameRVA)
    {
      char sftime[80];
      struct tm * tm;
      __peReadASCIIZName(bmbioHandle(),RVA2Phys(et.etNameRVA),exp_buf, sizeof(exp_buf));
      if(strlen(exp_buf) > 50) strcpy(&exp_buf[50],"...");
      tm = localtime((time_t *)&et.etDateTime);
      strftime(sftime,sizeof(sftime),"%x",tm);
      sprintf(exp_nam," %s (ver=%hX.%hX %s) "
              ,exp_buf
              ,et.etMajVer, et.etMinVer
              ,sftime);
    }
  }
  ret = fmtShowList(PEExportNumItems,PEExportReadItems,
                    exp_nam,
                    LB_SELECTIVE | LB_SORTABLE,&ordinal);
  if(ret != -1)
  {
    fpos = CalcEntryPE(ordinal,True);
  }
  return fpos;
}

/***************************************************************************/
/************************  FOR PE  *****************************************/
/***************************************************************************/

typedef struct tagRELOC_PE
{
  unsigned long modidx;
  union
  {
    unsigned long funcidx; /** if modidx != -1 */
    unsigned long type;    /** if modidx == -1 */
  }import;
  unsigned long laddr; /** lookup addr */
  unsigned long reserved;
}RELOC_PE;

static linearArray *CurrPEChain = NULL;

static tCompare __FASTCALL__ compare_pe_reloc_s(const void __HUGE__ *e1,const void __HUGE__ *e2)
{
  const RELOC_PE __HUGE__ *p1,__HUGE__ *p2;
  p1 = (const RELOC_PE __HUGE__ *)e1;
  p2 = (const RELOC_PE __HUGE__ *)e2;
  return __CmpLong__(p1->laddr,p2->laddr);
}

static void __NEAR__ __FASTCALL__ BuildPERefChain( void )
{
  unsigned long phys,i,j,cpos;
  RELOC_PE rel;
  ImportDirPE ipe;
  unsigned nnames;
  BGLOBAL handle;
  TWindow *w,*usd;
  usd = twUsedWin();
  if(!(CurrPEChain = la_Build(0,sizeof(RELOC_PE),MemOutBox))) return;
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  if(!PubNames) pe_ReadPubNameList(bmbioHandle(),MemOutBox);
  twUseWin(w);
  twGotoXY(1,1);
  twPutS(BUILD_REFS);
  twUseWin(usd);
  handle = pe_cache;
  bioSeek(handle,0L,SEEK_SET);
  /**
     building references chain for external
  */
  if(pe.peImportTableRVA)
  {
    phys = RVA2Phys(pe.peImportTableRVA);
    nnames = GetImportCountPE(handle,phys);
  }
  else
  {
    phys = 0;
    nnames = 0;
  }
  for(i = 0;i < nnames;i++)
  {
    unsigned long magic,Hint;
    bioSeek(handle,phys + i*sizeof(ImportDirPE),SEEKF_START);
    bioReadBuffer(handle,&ipe,sizeof(ImportDirPE));
    if(!(ipe.idMajVer == 0 && ipe.idMinVer == 0 && ipe.idDateTime != 0xFFFFFFFFUL))
                                 magic = ipe.idLookupTableRVA;
    else                         magic = ipe.idFlags;
    if(magic == 0) magic = ipe.idLookupTableRVA; /* Added by "Kostya Nosov" <k-nosov@yandex.ru> */
    addr_shift_pe = magic ? RVA2Phys(magic) : magic;
    if(addr_shift_pe)
    {
      tBool is_eof;
      bioSeek(handle,addr_shift_pe,SEEKF_START);
      j = 0;
      is_eof = False;
      while(1)
      {
        Hint = bioReadDWord(handle);
        is_eof = bioEOF(handle);
        if(!Hint || IsKbdTerminate() || is_eof) break;
        rel.modidx = i;
        rel.laddr = Hint;
        rel.import.funcidx = j;
        if(!la_AddData(CurrPEChain,&rel,MemOutBox)) goto bye;
        j++;
      }
      if(is_eof) break;
    }
  }
  /**
     building references chain for internal
  */
  if(pe.peTotalFixupDataSize)
  {
    phys = RVA2Phys(pe.peFixupTableRVA);
    bioSeek(handle,phys,SEEKF_START);
    cpos = bioTell(handle);
    while(bioTell(handle) < cpos + pe.peTotalFixupDataSize)
    {
      tUInt16 typeoff;
      unsigned long page,physoff,size,ccpos;
      tBool is_eof;
      ccpos = bioTell(handle);
      page = bioReadDWord(handle);
      physoff = RVA2Phys(page);
      size = bioReadDWord(handle);
      is_eof = False;
      while(bioTell(handle) < ccpos + size)
      {
        typeoff = bioReadWord(handle);
        is_eof = bioEOF(handle);
        if(IsKbdTerminate() || is_eof) break;
        rel.modidx = ULONG_MAX;
        rel.import.type = typeoff >> 12;
        rel.laddr = physoff + (typeoff & 0x0FFF);
        if(!la_AddData(CurrPEChain,&rel,MemOutBox)) goto bye;
      }
      if(is_eof) break;
    }
  }
  bye:
  la_Sort(CurrPEChain,compare_pe_reloc_s);
  CloseWnd(w);
}

static RELOC_PE __HUGE__ * __NEAR__ __FASTCALL__ __found_RPE(unsigned long laddr)
{
  RELOC_PE key;
  if(!CurrPEChain) BuildPERefChain();
  key.laddr = laddr;
  return la_Find(CurrPEChain,&key,compare_pe_reloc_s);
}

static unsigned long __NEAR__ __FASTCALL__ BuildReferStrPE(char *str,RELOC_PE __HUGE__ *rpe,int flags)
{
   BGLOBAL handle,handle2,handle3;
   unsigned long phys,rva,magic,Hint;
   unsigned long retrf;
   ImportDirPE ipe;
   char buff[400];
   handle = pe_cache;
   handle2 = pe_cache4;
   handle3 = pe_cache3;
   phys = RVA2Phys(pe.peImportTableRVA);
   bioSeek(handle,phys + 20L*rpe->modidx,SEEKF_START);
   rva = fioReadDWord(handle,12L,SEEKF_CUR);
   retrf = RAPREF_DONE;
   if(rpe->modidx != 0xFFFFFFFFUL)
   {
     char *is_ext;
     if(flags & APREF_USE_TYPE) strcat(str," off32");
     bioSeek(handle2,RVA2Phys(rva),SEEKF_START);
     bioReadBuffer(handle2,buff,400);
     buff[399] = 0;
     /*
        Removing extension .dll from import name.
        Modified by "Kostya Nosov" <k-nosov@yandex.ru>
     */
     is_ext = strrchr(buff,'.');
     if(is_ext && !strcmp(is_ext,".dll"))
     {
          *is_ext = 0;
          sprintf(&str[strlen(str)]," %s.",buff);
     }
     else
       sprintf(&str[strlen(str)]," <%s>.",buff);
     bioSeek(handle3,phys + rpe->modidx*sizeof(ImportDirPE),SEEKF_START);
     bioReadBuffer(handle3,&ipe,sizeof(ImportDirPE));
     if(!(ipe.idMajVer == 0 && ipe.idMinVer == 0 && ipe.idDateTime != 0xFFFFFFFFUL))
                                  magic = ipe.idFlags;
     else                         magic = ipe.idLookupTableRVA;
     if(magic == 0) magic = ipe.idLookupTableRVA; /* Added by "Kostya Nosov" <k-nosov@yandex.ru> */
     magic = magic ? RVA2Phys(magic) : magic;
     if(magic)
     {
       bioSeek(handle,magic + rpe->import.funcidx*sizeof(long),SEEKF_START);
       Hint = bioReadDWord(handle);
       if(Hint & 0x80000000UL)
       {
         char dig[15];
         sprintf(dig,"@%lu",Hint & 0x7FFFFFFFUL);
         strcat(str,dig);
       }
       else
       {
         phys = RVA2Phys(Hint & 0x7FFFFFFFUL);
         if(phys > bmGetFLength()) strcat(str,"???");
         else
         {
           bioSeek(handle2,phys + 2,SEEKF_START);
           bioReadBuffer(handle2,buff,400);
           buff[399] = 0;
           strcat(str,buff);
         }
       }
     }
   }
   else /** internal refs */
   {
     unsigned long delta,value,point_to;
     const char *pe_how;
     bioSeek(handle3,rpe->laddr,SEEKF_START);
     value = bioReadDWord(handle3);
     delta = pe.peImageBase;
     point_to = 0;
     switch(rpe->import.type)
     {
          default:
          case 0: /** ABSOLUTE, fixup is skipped */
                  pe_how = "(";
                  point_to = value;
                  break;
          case 1: /** HIGH 16-bit */
                  pe_how = "((high16)";
                  break;
          case 2: /** LOW 16-bit */
                  pe_how = "((low16)";
                  break;
          case 3: /** HIGHLOW */
                  point_to = peVA2PA(value);
                  pe_how = "((off32)";
                  break;
          case 4: /** HIGHADJUST */
                  bioSeek(handle,value,SEEK_SET);
                  value = bioReadDWord(handle);
                  point_to = peVA2PA(value);
                  pe_how = "((full32)";
                  break;
          case 5: /** MIPS JUMP ADDR */
                  pe_how = "((mips)";
                  break;
     }
     retrf = point_to ? point_to : value-delta;
     if(!(flags & APREF_SAVE_VIRT))
     {
       strcat(str,"*this.");
       if(flags & APREF_USE_TYPE) strcat(str,pe_how);
       /** if out of physical image */
       strcat(str,Get8Digit(retrf));
       if(flags & APREF_USE_TYPE)  strcat(str,")");
     }
     else strcat(str,Get8Digit(value));
   }
   return retrf;
}

static unsigned long __FASTCALL__ AppendPERef(char *str,unsigned long ulShift,int flags,int codelen,unsigned long r_sh)
{
  RELOC_PE __HUGE__ *rpe;
  unsigned long retrf;
  BGLOBAL b_cache;
  char buff[400];
  UNUSED(codelen);
  b_cache = pe_cache3;
  retrf = RAPREF_NONE;
  if(pe.peImportTableRVA || pe.peFixupTableRVA)
  {
    bioSeek(b_cache,
            RVA2Phys(bmReadDWordEx(ulShift,SEEK_SET) - pe.peImageBase),
            SEEK_SET);
    rpe = __found_RPE(bioReadDWord(b_cache));
    if(!rpe) rpe = __found_RPE(ulShift);
    if(rpe)  retrf = BuildReferStrPE(str,rpe,flags);
  }
  if(!retrf && (flags & APREF_TRY_LABEL))
  {
     if(!PubNames) pe_ReadPubNameList(bmbioHandle(),MemOutBox);
     if(FindPubName(buff,sizeof(buff),r_sh))
     {
       strcat(str,buff);
       if(!DumpMode && !EditMode) GidAddGoAddress(str,r_sh);
       retrf = RAPREF_DONE;
     }
  }
  return retrf;
}

static tBool __FASTCALL__ IsPE( void )
{
   char id[2];
   headshift = IsNewExe();
   if(headshift)
   {
     bmReadBufferEx(id,sizeof(id),headshift,SEEKF_START);
     if(id[0] == 'P' && id[1] == 'E') return True;
   }
   return False;
}

static void __FASTCALL__ initPE( void )
{
   int i;
   BGLOBAL main_handle;
   PMRegLowMemCallBack(peLowMemFunc);
   bmReadBufferEx(&pe,sizeof(PEHEADER),headshift,SEEKF_START);
   if(!(peVA = PMalloc(sizeof(PE_ADDR)*pe.peObjects)))
   {
     MemOutBox("PE initialization");
     exit(EXIT_FAILURE);
   }
   main_handle = bmbioHandle();
   bioSeek(main_handle,0x18 + pe.peNTHdrSize + headshift,SEEKF_START);
   for(i = 0;i < pe.peObjects;i++)
   {
     peVA[i].rva = fioReadDWord(main_handle,12L,SEEKF_CUR);
     peVA[i].phys = fioReadDWord(main_handle,4L,SEEKF_CUR);
     bioSeek(main_handle,16L,SEEKF_CUR);
   }
   if((pe_cache = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache = main_handle;
   if((pe_cache1 = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache1 = main_handle;
   if((pe_cache2 = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache2 = main_handle;
   if((pe_cache3 = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache3 = main_handle;
   if((pe_cache4 = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache4 = main_handle;
}

static void __FASTCALL__ destroyPE( void )
{
  BGLOBAL main_handle;
  if(peVA) PFree(peVA);
  if(PubNames) { la_Destroy(PubNames); PubNames = 0; }
  if(CurrPEChain) { la_Destroy(CurrPEChain); CurrPEChain = 0; } /* Fixed by "Kostya Nosov" <k-nosov@yandex.ru> */
  main_handle = bmbioHandle();
  if(pe_cache != &bNull && pe_cache != main_handle) bioClose(pe_cache);
  if(pe_cache1 != &bNull && pe_cache1 != main_handle) bioClose(pe_cache1);
  if(pe_cache2 != &bNull && pe_cache2 != main_handle) bioClose(pe_cache2);
  if(pe_cache3 != &bNull && pe_cache3 != main_handle) bioClose(pe_cache3);
  if(pe_cache4 != &bNull && pe_cache4 != main_handle) bioClose(pe_cache4);
  PMUnregLowMemCallBack(peLowMemFunc);
}

static int __FASTCALL__ bitnessPE(unsigned long off)
{
   if(off >= headshift)
   {
     return (pe.peFlags & 0x0040) ? DAB_USE16 : DAB_USE32;
   }
   else return DAB_USE16;
}

static unsigned long __FASTCALL__ PEHelp( void )
{
  hlpDisplay(10009);
  return BMGetCurrFilePos();
}

static tBool __FASTCALL__ peAddressResolv(char *addr,unsigned long cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
 tBool bret = True;
 tUInt32 res;
 if(cfpos >= headshift && cfpos < headshift + sizeof(PEHEADER))
 {
    strcpy(addr,"PEH :");
    strcpy(&addr[5],Get4Digit(cfpos - headshift));
 }
 else
 if(cfpos >= headshift + pe.peNTHdrSize + 0x18 &&
    cfpos <  headshift + pe.peNTHdrSize + 0x18 + pe.peObjects*sizeof(PE_OBJECT))
 {
    strcpy(addr,"PEOD:");
    strcpy(&addr[5],Get4Digit(cfpos - headshift - pe.peNTHdrSize - 0x18));
 }
 else  /* Added by "Kostya Nosov" <k-nosov@yandex.ru> */
   if((res=pePA2VA(cfpos))!=0)
   {
     addr[0] = '.';
     strcpy(&addr[1],Get8Digit(res));
   }
   else bret = False;
  return bret;
}

static unsigned long __FASTCALL__ peVA2PA(unsigned long va)
{
  return va >= pe.peImageBase ? RVA2Phys(va-pe.peImageBase) : 0L;
}

static unsigned long __FASTCALL__ pePA2VA(unsigned long pa)
{
 int i;
  unsigned long ret_addr;
  bmSeek(0x18 + pe.peNTHdrSize + headshift,SEEK_SET);
  ret_addr = 0;
  for(i = 0;i < pe.peObjects;i++)
  {
    PE_OBJECT po;
    unsigned long obj_pa;
    if(IsKbdTerminate() || bmEOF()) break;
    bmReadBuffer(&po,sizeof(PE_OBJECT));
    obj_pa = CalcPEObjectEntry(po.oPhysicalOffset);
    if(pa >= obj_pa && pa < obj_pa + po.oPhysicalSize)
    {
      ret_addr = po.oRVA + (pa - obj_pa) + pe.peImageBase;
      break;
    }
  }
  return ret_addr;
}

static void __FASTCALL__ pe_ReadPubName(BGLOBAL b_cache,const struct PubName *it,
                           char *buff,unsigned cb_buff)
{
    char stmp[300];
    __peReadASCIIZName(b_cache,it->nameoff,stmp, sizeof(stmp));
    strncpy(buff,stmp,cb_buff);
    buff[cb_buff-1] = 0;
}

static tBool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,unsigned long pa)
{
  return fmtFindPubName(pe_cache4,buff,cb_buff,pa,
                        pe_ReadPubNameList,
                        pe_ReadPubName);
}

static void __FASTCALL__ pe_ReadPubNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *))
{
  unsigned long i,nitems,expaddr,nameptr,nameaddr,entry_pa;
  unsigned ord;
  struct PubName pn;
  BGLOBAL b_cache;
  b_cache = pe_cache4 == &bNull ? handle : pe_cache4;
  nitems = PEExportNumItems(handle);
  if(!(PubNames = la_Build(0,sizeof(struct PubName),mem_out))) return;
  expaddr  = RVA2Phys(et.etOrdinalTableRVA);
  nameptr = RVA2Phys(et.etNamePtrTableRVA);
  for(i = 0;i < nitems;i++)
  {
    nameaddr = fioReadDWord2Phys(handle,nameptr + 4*i,SEEKF_START);
    ord = fioReadWord(b_cache,expaddr + i*2,SEEKF_START) + (unsigned)et.etOrdinalBase;
    entry_pa = CalcEntryPE((unsigned)ord,False);
    pn.pa = entry_pa;
    pn.nameoff = nameaddr;
    pn.attr = SC_GLOBAL;
    if(!la_AddData(PubNames,&pn,mem_out)) break;
  }
  la_Sort(PubNames,fmtComparePubNames);
}

static unsigned long __FASTCALL__ peGetPubSym(char *str,unsigned cb_str,unsigned *func_class,
                          unsigned long pa,tBool as_prev)
{
  return fmtGetPubSym(pe_cache,str,cb_str,func_class,pa,as_prev,
                      pe_ReadPubNameList,
                      pe_ReadPubName);
}

static unsigned __FASTCALL__ peGetObjAttr(unsigned long pa,char *name,unsigned cb_name,
                      unsigned long *start,unsigned long *end,int *_class,int *bitness)
{
  unsigned i,nitems,ret;
  *start = 0;
  *end = bmGetFLength();
  *_class = OC_NOOBJECT;
  *bitness = bitnessPE(pa);
  name[0] = 0;
  nitems = pe.peObjects;
  ret = 0;
  bmSeek(0x18 + pe.peNTHdrSize + headshift,SEEK_SET);
  for(i = 0;i < nitems;i++)
  {
    PE_OBJECT po;
    if(IsKbdTerminate() || bmEOF()) break;
    bmReadBuffer(&po,sizeof(PE_OBJECT));
    if(pa >= *start && pa < po.oPhysicalOffset)
    {
      /** means between two objects */
      *end = po.oPhysicalOffset;
      ret = 0;
      break;
    }
    if(pa >= po.oPhysicalOffset && pa < po.oPhysicalOffset + po.oPhysicalSize)
    {
      *start = po.oPhysicalOffset;
      *end = *start + po.oPhysicalSize;
      *_class = po.oFlags & 0x00000020L ? OC_CODE : OC_DATA;
      strncpy(name,(const char *)po.oName,cb_name);
      ret = i+1;
      break;
    }
    *start = po.oPhysicalOffset + po.oPhysicalSize;
  }
  return ret;
}

static int __FASTCALL__ pePlatform( void ) { return DISASM_CPU_IX86; }

REGISTRY_BIN peTable =
{
  "PE (Portable Executable)",
  { "PEHelp", "Import", "Export", NULL, NULL, NULL, NULL, "PEHead", NULL, "Object" },
  { PEHelp, ShowModRefPE, ShowExpNamPE, NULL, NULL, NULL, NULL, ShowNewHeaderPE, NULL, ShowObjectsPE },
  IsPE, initPE, destroyPE,
  NULL,
  AppendPERef,
  fmtSetState,
  pePlatform,
  bitnessPE,
  peAddressResolv,
  peVA2PA,
  pePA2VA,
  peGetPubSym,
  peGetObjAttr,
  NULL,
  NULL
};
