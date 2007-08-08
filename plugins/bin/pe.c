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

#define ARRAY_SIZE(x)       (sizeof(x)/sizeof(x[0]))

static PE_ADDR * peVA = NULL;
static PEHEADER pe;
static PERVA *peDir;

static BGLOBAL pe_cache = &bNull;
static BGLOBAL pe_cache1 = &bNull;
static BGLOBAL pe_cache2 = &bNull;
static BGLOBAL pe_cache3 = &bNull;
static BGLOBAL pe_cache4 = &bNull;

static tBool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa);
static void __FASTCALL__ pe_ReadPubNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *));
static __filesize_t __FASTCALL__ peVA2PA(__filesize_t va);
static __filesize_t __FASTCALL__ pePA2VA(__filesize_t pa);
static __fileoff_t __NEAR__ CalcOverlayOffset( void );

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

static __filesize_t __NEAR__ __FASTCALL__ CalcPEObjectEntry(__fileoff_t offset)
{
 __filesize_t intp;
 intp = offset / pe.peFileAlign;
 if(offset % pe.peFileAlign) offset = ( offset / intp ) * intp;
 return offset;
}

static __filesize_t __NEAR__ __FASTCALL__ RVA2Phys(__filesize_t rva)
{
 int i;
 __filesize_t npages,poff,obj_rva,pphys,ret,size;
 for(i = pe.peObjects - 1;i >= 0;i--)
 {
   if(rva >= peVA[i].rva) break;
   if(IsKbdTerminate()) return 0;
 }
 if (i < 0) return rva;         // low RVAs fix -XF
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
 npages = (rva - obj_rva) / (__filesize_t)4096;
 poff = (rva - obj_rva) % (__filesize_t)4096;
 ret = pphys + npages*4096UL + poff;
 return ret;
}

static __filesize_t __NEAR__ __FASTCALL__ fioReadDWord(BGLOBAL handle,__filesize_t offset,int origin)
{
 bioSeek(handle,offset,origin);
 return bioReadDWord(handle);
}

static unsigned __NEAR__ __FASTCALL__ fioReadWord(BGLOBAL handle,__filesize_t offset,int origin)
{
 bioSeek(handle,offset,origin);
 return bioReadWord(handle);
}

static __filesize_t __NEAR__ __FASTCALL__ fioReadDWord2Phys(BGLOBAL handle,__filesize_t offset,int origin)
{
 unsigned long dword;
 dword = fioReadDWord(handle,offset,origin);
 return RVA2Phys(dword);
}

static const char * __NEAR__ __FASTCALL__ PECPUType(void)
{
    static const struct {
       int code;
       char *name;
    } pe_cpu[] = {
       {0x014C, "Intel 80386"},
//       {0x014D, "Intel 80486"},
//       {0x014E, "Intel 80586"},
//       {0x014F, "Intel 80686"},
//       {0x0150, "Intel 80786"},
       {0x0162, "MIPS R3000"},
       {0x0166, "MIPS R4000"},
       {0x0168, "MIPS R10000"},
       {0x0169, "MIPS WCE v2"},
       {0x0184, "DEC Alpha"},
       {0x01A2, "SH3"},
       {0x01A3, "SH3DSP"},
       {0x01A4, "SH3E"},
       {0x01A6, "SH4"},
       {0x01A8, "SH5"},
       {0x01C0, "ARM"},
       {0x01C2, "ARM Thumb"},
       {0x01D3, "AM33"},
       {0x01F0, "IBM PowerPC"},
       {0x01F1, "IBM PowerPC FP"},
       {0x0200, "Intel IA-64"},
       {0x0266, "MIPS16"},
       {0x0284, "DEC Alpha 64"},
       {0x0366, "MIPSFPU"},
       {0x0466, "MIPSFPU16"},
       {0x0520, "Tricore"},
       {0x0CEF, "CEF"},
       {0x0EBC, "EFI Byte Code"},
       {0x8664, "AMD64"},
       {0x9041, "M32R"},
       {0xC0EE, "CEE"},
       {0x0000, "Unknown"},
    };
    int i;

    for(i=0; i<(sizeof(pe_cpu)/sizeof(pe_cpu[0])); i++) {
       if(pe.peCPUType == pe_cpu[i].code)
          return pe_cpu[i].name;
    }

    return "Unknown";
}

static __filesize_t entryPE = 0L;
static __fileoff_t overlayPE = -1L;

static void __NEAR__ PaintNewHeaderPE_1( void )
{
  entryPE = RVA2Phys(pe.peEntryPointRVA);
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

static void __NEAR__ PaintNewHeaderPE_2( void )
{
  static const char * subSystem[] =
  {
    "Unknown",
    "Native",
    "Windows GUI",
    "Windows Character",
    "OS/2 GUI",
    "OS/2 Character",
    "Posix GUI",
    "Posix Character",
    "Win9x Driver",
    "Windows CE",
    "EFI application",
    "EFI boot service driver",
    "EFI runtime driver",
    "EFI ROM",
    "X-Box",
  };

  twPrintF("File align                     = %08lXH\n"
           "OS version                     = %hu.%hu\n"
           "User version                   = %hu.%hu\n"
           "Subsystem version              = %hu.%hu\n"
           "Image size                     = %lu bytes\n"
           "Header size                    = %lu bytes\n"
           "File checksum                  = %08lXH\n"
           "Subsystem                      = %s\n"
           "DLL Flags :                      [%04hXH]\n"
           " [%c] Per-Process Library initialization\n"
           " [%c] Per-Process Library termination\n"
           " [%c] Per-Thread  Library initialization\n"
           " [%c] Per-Thread  Library termination\n"
           "Stack reserve size             = %lu bytes\n"
           "Stack commit size              = %lu bytes\n"
           "Heap reserve size              = %lu bytes\n"
           "Heap commit size               = %lu bytes\n"
           "Number of directory entries    = %lu bytes"
           ,pe.peFileAlign
           ,pe.peOSMajor,pe.peOSMinor
           ,pe.peUserMajor,pe.peUserMinor
           ,pe.peSubSystMajor,pe.peSubSystMinor
           ,pe.peImageSize
           ,pe.peHeaderSize
           ,pe.peFileChecksum
           ,pe.peSubSystem < ARRAY_SIZE(subSystem) ? subSystem[pe.peSubSystem] : "Unknown"
           ,pe.peDLLFlags
           ,GetBool(pe.peDLLFlags & 0x0001)
           ,GetBool(pe.peDLLFlags & 0x0002)
           ,GetBool(pe.peDLLFlags & 0x0004)
           ,GetBool(pe.peDLLFlags & 0x0008)
           ,pe.peStackReserveSize
           ,pe.peStackCommitSize
           ,pe.peHeapReserveSize
           ,pe.peHeapCommitSize
           ,pe.peDirSize);
  if (CalcOverlayOffset() != -1) {
    entryPE = overlayPE;
    twSetColorAttr(dialog_cset.entry);
    twPrintF("\nOverlay                        = %08lXH", entryPE); twClrEOL();
    twSetColorAttr(dialog_cset.main);
  }
}

static void (__NEAR__ * const pephead[])(void) = /* [dBorca] the table is const, not the void */
{
    PaintNewHeaderPE_1,
    PaintNewHeaderPE_2
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
  if(npage < 2)
  {
    twGotoXY(1,1);
    (*(pephead[npage]))();
  }
  twRefreshFullWin(win);
}

static __filesize_t __FASTCALL__ ShowNewHeaderPE( void )
{
 __fileoff_t fpos;
 fpos = BMGetCurrFilePos();
 if(PageBox(70,21,NULL,2,PaintNewHeaderPE) != -1 && entryPE && entryPE < bmGetFLength()) fpos = entryPE;
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

 memcpy(buffer, objs->oName, 8);
 buffer[8] = 0;

 twPrintF("Object Name                    = %8s\n"
          "Virtual Size                   = %lX bytes\n"
          "RVA (relative virtual address) = %08lX\n"
          "Physical size                  = %08lX bytes\n"
          "Physical offset                = %08lX bytes\n"
          "<Relocations>                  = %hu at %08lX\n"
          "<Line numbers>                 = %hu at %08lX\n"
          "FLAGS: %lX\n"
          "   [%c] Executable code          "   "   [%c] Shared object\n"
          "   [%c] Initialized data         "   "   [%c] Executable object\n"
          "   [%c] Uninitialized data       "   "   [%c] Readable object\n"
          "   [%c] Contains COMDAT          "   "   [%c] Writable object\n"
          "   [%c] Contains comments or other info\n"
          "   [%c] Won't become part of the image\n"
          "   [%c] Contains extended relocations\n"
          "   [%c] Discardable as needed\n"
          "   [%c] Must not be cashed\n"
          "   [%c] Not pageable\n"
          "   Alignment                   = %u %s\n"

          ,buffer
          ,objs->oVirtualSize
          ,objs->oRVA
          ,objs->oPhysicalSize
          ,objs->oPhysicalOffset
          ,objs->oNReloc
          ,objs->oRelocPtr
          ,objs->oNLineNumb
          ,objs->oLineNumbPtr
          ,objs->oFlags

          ,GetBool(objs->oFlags & 0x00000020UL), GetBool(objs->oFlags & 0x10000000UL)
          ,GetBool(objs->oFlags & 0x00000040UL), GetBool(objs->oFlags & 0x20000000UL)
          ,GetBool(objs->oFlags & 0x00000080UL), GetBool(objs->oFlags & 0x40000000UL)
          ,GetBool(objs->oFlags & 0x00001000UL), GetBool(objs->oFlags & 0x80000000UL)
          ,GetBool(objs->oFlags & 0x00000200UL)
          ,GetBool(objs->oFlags & 0x00000800UL)
          ,GetBool(objs->oFlags & 0x01000000UL)
          ,GetBool(objs->oFlags & 0x02000000UL)
          ,GetBool(objs->oFlags & 0x04000000UL)
          ,GetBool(objs->oFlags & 0x08000000UL)

          ,objs->oFlags&0x00F00000 ? 1 << (((objs->oFlags&0x00F00000)>>20)-1) : 0
          ,objs->oFlags&0x00F00000 ? "byte(s)" : "(default)");

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

static __fileoff_t __NEAR__ CalcOverlayOffset( void )
{
  if (overlayPE == -1 && pe.peObjects) {
    memArray *obj;
    if ((obj = ma_Build(pe.peObjects, True))) {
      bioSeek(pe_cache, 0x18 + pe.peNTHdrSize + headshift, SEEK_SET);
      if (__ReadObjectsPE(pe_cache, obj, pe.peObjects)) {
        int i;
	for (i = 0; i < pe.peObjects; i++) {
	  PE_OBJECT *o = (PE_OBJECT *)obj->data[i];
	  __fileoff_t end = o->oPhysicalOffset + ((o->oPhysicalSize + (pe.peFileAlign - 1)) & ~(pe.peFileAlign - 1));
	  if (overlayPE < end) {
	    overlayPE = end;
	  }
	}
      }
      ma_Destroy(obj);
    }
  }
  return overlayPE;
}

static __filesize_t __FASTCALL__ ShowObjectsPE( void )
{
 __filesize_t fpos;
 BGLOBAL handle;
 unsigned nnames;
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

static unsigned __NEAR__ __FASTCALL__ GetImportCountPE(BGLOBAL handle,__filesize_t phys)
{
  unsigned count;
  __filesize_t fpos = bioTell(handle);
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
unsigned __NEAR__ __FASTCALL__ __peReadASCIIZName(BGLOBAL handle,__filesize_t offset,char *buff, unsigned cb_buff)
{
  unsigned j;
  char ch;
  __filesize_t fpos;
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

static unsigned __NEAR__ __FASTCALL__ __ReadImportPE(BGLOBAL handle,__filesize_t phys,memArray *obj,unsigned nnames)
{
  unsigned i;
  __filesize_t fpos = bioTell(handle);
  __filesize_t rva,addr;
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

static __filesize_t addr_shift_pe = 0L;

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
  unsigned long Hint;
  __filesize_t rphys;
  bioSeek(handle,addr_shift_pe,SEEKF_START);
  VA = pePA2VA(addr_shift_pe);
  for(i = 0;i < nnames;i++)
  {
    char stmp[300];
    tBool is_eof;
    sprintf(stmp,".%08X: ", VA);
    VA += 4;
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

static __filesize_t __FASTCALL__ ShowModRefPE( void )
{
  BGLOBAL handle;
  char petitle[80];
  memArray * obj;
  unsigned nnames;
  __filesize_t phys,fret;
  fret = BMGetCurrFilePos();
  if(!peDir[PE_IMPORT].rva) { not_found: NotifyBox(NOT_ENTRY," Module References "); return fret; }
  handle = pe_cache;
  bioSeek(handle,0L,SEEK_SET);
  phys = RVA2Phys(peDir[PE_IMPORT].rva);
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

static inline void writeExportVA(__filesize_t va, BGLOBAL handle, char *buf, unsigned long bufsize)
{
    // check for forwarded export
    if (va>=peDir[PE_EXPORT].rva && va<peDir[PE_EXPORT].rva+peDir[PE_EXPORT].size)
        __peReadASCIIZName(handle, RVA2Phys(va), buf, bufsize);
    // normal export
    else
    sprintf(buf, ".%08lX", (unsigned long)(va + pe.peImageBase));
}

static tBool __FASTCALL__ PEExportReadItems(BGLOBAL handle,memArray * obj,unsigned nnames)
{
  __filesize_t nameaddr,expaddr,nameptr;
  unsigned long *addr;
  unsigned i,ord;
  char buff[80];

  nameptr = RVA2Phys(et.etNamePtrTableRVA);
  expaddr = RVA2Phys(et.etOrdinalTableRVA);
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
    sprintf(buff,"%c%-9lu ", LB_ORD_DELIMITER, ord+(unsigned long)et.etOrdinalBase);
    writeExportVA(addr[ord], handle, buff+11, sizeof(buff)-11);
    addr[ord] = 0;
    strcat(stmp,buff);
    if(!ma_AddString(obj,is_eof ? CORRUPT_BIN_MSG : stmp,True)) break;  // -XF removed PFree(stmp)
    if(is_eof) break;
  }

  for(i = 0;i < nnames;i++)
  {
    if(addr[i])
    {
      ord = i+et.etOrdinalBase;
      sprintf(buff," < by ordinal > %c%-9lu ",LB_ORD_DELIMITER, (unsigned long)ord);
      writeExportVA(addr[i], handle, buff+27, sizeof(buff)-27);
      if(!ma_AddString(obj,buff,True)) break;
    }
  }

  PFree(addr);
  return True;
}

static unsigned __FASTCALL__ PEExportNumItems(BGLOBAL handle)
{
  __filesize_t addr;
  if(!peDir[PE_EXPORT].rva) return 0;
  addr = RVA2Phys(peDir[PE_EXPORT].rva);
  bioSeek(handle,addr,SEEKF_START);
  bioReadBuffer(handle,(void *)&et,sizeof(et));
  return (unsigned)(et.etNumEATEntries);
}

static __filesize_t __NEAR__ __FASTCALL__ CalcEntryPE(unsigned ordinal,tBool dispmsg)
{
 __filesize_t fret,rva;
 unsigned ord;
 BGLOBAL handle;
 fret = BMGetCurrFilePos();
 handle = pe_cache1;
 {
   __filesize_t eret;
   rva = RVA2Phys(et.etAddressTableRVA);
   ord = (unsigned)ordinal - (unsigned)et.etOrdinalBase;
   eret = fioReadDWord2Phys(handle,rva + 4*ord,SEEKF_START);
   if(eret && eret < bmGetFLength()) fret = eret;
   else if(dispmsg) ErrMessageBox(NO_ENTRY,BAD_ENTRY);
 }
 return fret;
}

static __filesize_t __FASTCALL__ ShowExpNamPE( void )
{
  __filesize_t fpos = BMGetCurrFilePos();
  int ret;
  unsigned ordinal;
  __filesize_t addr;
  char exp_nam[256], exp_buf[300];
  fpos = BMGetCurrFilePos();
  strcpy(exp_nam,EXP_TABLE);
  if(peDir[PE_EXPORT].rva)
  {
    addr = RVA2Phys(peDir[PE_EXPORT].rva);
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


static tBool __FASTCALL__ PEReadRVAs(BGLOBAL handle, memArray * obj, unsigned nnames)
{
  unsigned i;
  static const char *rvaNames[] =
  {
      "~Export Table        ",
      "~Import Table        ",
      "~Resource Table      ",
      "E~xception Table     ",
      "Sec~urity Table      ",
      "Re~location Table    ",
      "~Debug Information   ",
      "Image De~scription   ",
      "~Machine Specific    ",
      "~Thread Local Storage",
      "Load Confi~guration  ",
      "~Bound Import Table  ",
      "Import ~Adress Table ",
      "Dela~y Import Table  ",
      "~COM+                ",
      "Reser~ved            "
  };
  UNUSED(handle);
  UNUSED(nnames);
  for (i=0; i<pe.peDirSize; i++)
  {
    char foo[80];

    sprintf(foo, "%s  %08lX  %8lu",
        i<ARRAY_SIZE(rvaNames) ? rvaNames[i] : "Unknown             ",
        (unsigned long)peDir[i].rva,
        (unsigned long)peDir[i].size);
    if (!ma_AddString(obj, foo, True))
        return False;
  }

  return True;
}

static unsigned __FASTCALL__ PENumRVAs(BGLOBAL handle)
{
  UNUSED(handle);
  return pe.peDirSize;
}

static __filesize_t __FASTCALL__ ShowPERVAs( void )
{
  __filesize_t fpos = BMGetCurrFilePos();
  int ret;
  ret = fmtShowList(PENumRVAs, PEReadRVAs, " Directory Entry       RVA           size ", LB_SELECTIVE|LB_USEACC, NULL);
  if (ret!=-1 && peDir[ret].rva)
    fpos = RVA2Phys(peDir[ret].rva);

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
  __filesize_t  phys,cpos;
  unsigned long i,j;
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
  if(peDir[PE_IMPORT].rva)
  {
    phys = RVA2Phys(peDir[PE_IMPORT].rva);
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
  if(peDir[PE_FIXUP].size)
  {
    phys = RVA2Phys(peDir[PE_FIXUP].rva);
    bioSeek(handle,phys,SEEKF_START);
    cpos = bioTell(handle);
    while(bioTell(handle) < cpos + peDir[PE_FIXUP].size)
    {
      tUInt16 typeoff;
      __filesize_t page,physoff,size,ccpos;
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

static RELOC_PE __HUGE__ * __NEAR__ __FASTCALL__ __found_RPE(__filesize_t laddr)
{
  RELOC_PE key;
  if(!CurrPEChain) BuildPERefChain();
  key.laddr = laddr;
  return la_Find(CurrPEChain,&key,compare_pe_reloc_s);
}

static __filesize_t __NEAR__ __FASTCALL__ BuildReferStrPE(char *str,RELOC_PE __HUGE__ *rpe,int flags)
{
   BGLOBAL handle,handle2,handle3;
   __filesize_t phys,rva,retrf;
   unsigned long magic,Hint;
   ImportDirPE ipe;
   char buff[400];
   handle = pe_cache;
   handle2 = pe_cache4;
   handle3 = pe_cache3;
   phys = RVA2Phys(peDir[PE_IMPORT].rva);
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

static unsigned long __FASTCALL__ AppendPERef(char *str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  RELOC_PE __HUGE__ *rpe;
  __filesize_t retrf;
  BGLOBAL b_cache;
  char buff[400];
  UNUSED(codelen);
  b_cache = pe_cache3;
  retrf = RAPREF_NONE;
  if(flags & APREF_TRY_PIC) return RAPREF_NONE;
  if(peDir[PE_IMPORT].rva || peDir[PE_FIXUP].rva)
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

   if(!(peDir = PMalloc(sizeof(PERVA)*pe.peDirSize)))
   {
     MemOutBox("PE initialization");
     exit(EXIT_FAILURE);
   }
   bmReadBuffer(peDir, sizeof(PERVA)*pe.peDirSize);

   if(!(peVA = PMalloc(sizeof(PE_ADDR)*pe.peObjects)))
   {
     MemOutBox("PE initialization");
     exit(EXIT_FAILURE);
   }
   bmSeek(0x18 + pe.peNTHdrSize + headshift,SEEKF_START);
   for(i = 0;i < pe.peObjects;i++)
   {
     peVA[i].rva = bmReadDWordEx(12L,SEEKF_CUR);
     peVA[i].phys = bmReadDWordEx(4L,SEEKF_CUR);
     bmSeek(16L,SEEKF_CUR);
   }

   main_handle = bmbioHandle();
   if((pe_cache = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache = main_handle;
   if((pe_cache1 = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache1 = main_handle;
   if((pe_cache2 = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache2 = main_handle;
   if((pe_cache3 = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache3 = main_handle;
   if((pe_cache4 = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache4 = main_handle;
}

static void __FASTCALL__ destroyPE( void )
{
  BGLOBAL main_handle;
  if(peVA) PFREE(peVA);
  if(peDir) PFREE(peDir);
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

static int __FASTCALL__ bitnessPE(__filesize_t off)
{
   if(off >= headshift)
   {
     return (pe.peFlags & 0x0040) ? DAB_USE16 : DAB_USE32;
   }
   else return DAB_USE16;
}

static __filesize_t __FASTCALL__ PEHelp( void )
{
  hlpDisplay(10009);
  return BMGetCurrFilePos();
}

static tBool __FASTCALL__ peAddressResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
 tBool bret = True;
 tUInt32 res;
 if(cfpos >= headshift && cfpos < headshift + sizeof(PEHEADER) + pe.peDirSize*sizeof(PERVA))
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

static __filesize_t __FASTCALL__ peVA2PA(__filesize_t va)
{
  return va >= pe.peImageBase ? RVA2Phys(va-pe.peImageBase) : 0L;
}

static __filesize_t __FASTCALL__ pePA2VA(__filesize_t pa)
{
  int i;
  __filesize_t ret_addr;
  bmSeek(0x18 + pe.peNTHdrSize + headshift,SEEK_SET);
  ret_addr = 0;
  for(i = 0;i < pe.peObjects;i++)
  {
    PE_OBJECT po;
    __filesize_t obj_pa;
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

static tBool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa)
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

static __filesize_t __FASTCALL__ peGetPubSym(char *str,unsigned cb_str,unsigned *func_class,
                          __filesize_t pa,tBool as_prev)
{
  return fmtGetPubSym(pe_cache,str,cb_str,func_class,pa,as_prev,
                      pe_ReadPubNameList,
                      pe_ReadPubName);
}

static unsigned __FASTCALL__ peGetObjAttr(__filesize_t pa,char *name,unsigned cb_name,
                      __filesize_t *start,__filesize_t *end,int *_class,int *bitness)
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
  { "PEHelp", "Import", "Export", NULL, NULL, NULL, NULL, "PEHead", "Dir   ", "Object" },
  { PEHelp, ShowModRefPE, ShowExpNamPE, NULL, NULL, NULL, NULL, ShowNewHeaderPE, ShowPERVAs, ShowObjectsPE },
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
