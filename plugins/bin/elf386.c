/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/elf386.c
 * @brief       This file contains implementation of ELF-32 file format decoder.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1999
 * @note        Development, fixes and improvements
**/

/*
    A short ELF hacker's guide from author of biew.
    ===============================================
    IMHO ELF is too complex format and contains surplusing information
    like SECTION HEADERS and PROGRAM HEADERS.
    As a rule, in normal ELF files both types are present.
    But there is minor number of non-standard ELF files where
    some type of headers are missing (or section or program ones).
    In case when SECTION HEADERS are lost, executable files and shared
    objects are workable, but BFD utilities can not handle such file
    properly, and so it will be no longer possible to link something
    against such file (but biew works with such files properly).
    In case when PROGRAM HEADERS are lost, executable files and shared
    objects are not workable, but RELOCATABLE OBJECTS will be normally
    linked to target executable or shared object.
    See also:

    http://www.caldera.com/developers/gabi/
    http://segfault.net/~scut/cpu/generic/
    http://www.linuxassembly.org   
    
    ELFkickers This distribution is a collection of programs that are generally
               unrelated, except in that they all deal with the ELF file format.

    teensy     tools that are provided as exhibitions of just how compressed a
               Linux ELF executable can be and still function.

    These can be found at:
    http://www.muppetlabs.com/~breadbox/software/
*/

#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "colorset.h"
#include "plugins/bin/elf386.h"
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
#include "biewlib/pmalloc.h"
#include "biewlib/kbd_code.h"

/* #define	ELF64 */

static char is_big;

static Elf386_External_Ehdr elf;
static unsigned long active_shtbl = 0;
static unsigned long elf_min_va = ULONG_MAX;
static unsigned long __elfNumSymTab = 0;
static unsigned long __elfSymShTbl = 0;
static unsigned      __elfSymEntSize = 0;
static unsigned long __elfSymPtr = 0L;

struct tag_elfVAMap
{
  unsigned long va;
  unsigned long size;
  unsigned long foff;
  unsigned long nameoff;
  unsigned long flags;
};

#ifdef	ELF64

/*
    (VERY) preliminary experimental ELF-64 support.
    Perhaps it will work on 64-bit platforms someday.
    Perhaps (though unlikely) it works to some extent on 32-bit platforms.
    Note that you need to set elf_struct_base to correspondent address
    each time (if needed) BEFORE calling ELF_HALF and ELF_WORD macros.
*/

static char is_64bit;

static unsigned long elf_struct_base = (unsigned long)&elf + EI_NIDENT;

static inline unsigned long elf64_value(unsigned long offset)
{
    unsigned long *p = (unsigned long *)
		    (elf_struct_base + ((offset - elf_struct_base) << 1));
    return *p;
}

static inline unsigned short __elf_half(unsigned long cval, unsigned long addr)
{
    return !is_64bit ?	FMT_WORD(cval,is_big) :
			FMT_DWORD(elf64_value(addr), is_big);
}

static inline unsigned long __elf_word(unsigned long cval, unsigned long addr)
{
    return !is_64bit ?	FMT_DWORD(cval,is_big) :
			FMT_QWORD(elf64_value(addr), is_big);
}

#define ELF_HALF(cval) __elf_half((unsigned long)cval,(unsigned long)&cval)
#define ELF_WORD(cval) __elf_word((unsigned long)cval,(unsigned long)&cval)

#else

#define ELF_HALF(cval) FMT_WORD(cval,is_big)
#define ELF_WORD(cval) FMT_DWORD(cval,is_big)

#endif

static tBool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,unsigned long pa);
static void __FASTCALL__ elf_ReadPubNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *));
static unsigned long __FASTCALL__ elfVA2PA(unsigned long va);
static unsigned long __FASTCALL__ elfPA2VA(unsigned long pa);
static tBool IsSectionsPresent;

static unsigned long __NEAR__ __FASTCALL__ findPHEntry(unsigned long type,unsigned *nitems)
{
  unsigned long fpos,dynptr;
  Elf386_External_Phdr phdr;
  size_t i,limit;
  fpos = bmGetCurrFilePos();
  dynptr = 0;
  *nitems = 0;
  limit = ELF_HALF(elf.e_phnum);
  for(i = 0;i < limit;i++)
  {
   bmReadBufferEx(&phdr,sizeof(Elf386_External_Phdr),ELF_WORD(elf.e_phoff) + i*ELF_HALF(elf.e_phentsize),SEEKF_START);
   if(bmEOF()) break;
   if(ELF_WORD(phdr.p_type) == type)
   {
     dynptr = ELF_WORD(phdr.p_offset);
     *nitems = ELF_WORD(phdr.p_filesz)/sizeof(Elf386_External_Dyn);
     break;
   }
  }
  bmSeek(fpos,SEEKF_START);
  return dynptr;
}

static unsigned long __NEAR__ __FASTCALL__ findPHDynEntry(unsigned long type,unsigned long dynptr,unsigned nitems)
{
  unsigned i;
  unsigned long fpos;
  tBool is_found = False;
  Elf386_External_Dyn dyntab;
  fpos = bmGetCurrFilePos();
  bmSeek(dynptr,SEEKF_START);
  for(i = 0;i < nitems;i++)
  {
    bmReadBufferEx(&dyntab,sizeof(dyntab),dynptr,SEEKF_START);
    if(bmEOF()) break;
    dynptr += sizeof(dyntab);
    if(ELF_WORD(dyntab.d_tag) == type) { is_found = True; break; }
  }    
  bmSeek(fpos,SEEKF_START);
  return is_found ? ELF_WORD(dyntab.d_un.d_ptr) : 0L;
}

static unsigned long __NEAR__ __FASTCALL__ findPHPubSyms(unsigned *number, unsigned *ent_size, unsigned long *act_shtbl)
{
  unsigned long fpos, dynptr, dyn_ptr;
  unsigned i, nitems;
  fpos = bmGetCurrFilePos();
  *ent_size = UINT_MAX;
    /* Here we did an attempt to detect symbol table even if sections header
       are lost */
    dyn_ptr = dynptr = findPHEntry(PT_DYNAMIC,&nitems);
    if(dynptr)
    {
      dynptr = elfVA2PA(findPHDynEntry(DT_SYMTAB,dyn_ptr,nitems));
      if(dynptr)
      {
        *act_shtbl = elfVA2PA(findPHDynEntry(DT_STRTAB,dyn_ptr,nitems));
        *ent_size = findPHDynEntry(DT_SYMENT,dyn_ptr,nitems);
       /* Only way to determine size of symbol table entries it's find
           nearest section that follows DT_SYMTAB.
	   FixME: It dangerous technique. May be there exists most safety way ?
	*/   
        {
          unsigned long _fpos,dptr,max_val,cur_ptr;
          Elf386_External_Dyn dyntab;
          _fpos = bmGetCurrFilePos();
	  dptr = dyn_ptr;
          bmSeek(dptr,SEEKF_START);
	  max_val = bmGetFLength(); /* if section is last */
          for(i = 0;i < nitems;i++)
          {
            bmReadBufferEx(&dyntab,sizeof(dyntab),dptr,SEEKF_START);
            if(bmEOF()) break;
            dptr += sizeof(dyntab);
	    cur_ptr = elfVA2PA(ELF_WORD(dyntab.d_un.d_ptr));
            if(cur_ptr > dynptr && cur_ptr < max_val) max_val = cur_ptr;
          }    
          bmSeek(_fpos,SEEKF_START);
  	  *number = (max_val - dynptr) / *ent_size;
        }
      }	
    }
  bmSeek(fpos, SEEKF_START);
  return dynptr;
}

static unsigned long __NEAR__ __FASTCALL__
                     findSHEntry(BGLOBAL b_cache, unsigned long type,
                                 unsigned long *nitems,unsigned long *link,
                                 unsigned *ent_size)
{
  Elf386_External_Shdr shdr;
  unsigned long fpos, tableptr;
  size_t i, limit;
  fpos = bioTell(b_cache);
  tableptr = 0L;
  limit = ELF_HALF(elf.e_shnum);
  for(i = 0;i < limit;i++)
  {
   bioSeek(b_cache,ELF_WORD(elf.e_shoff) + i*ELF_HALF(elf.e_shentsize),SEEKF_START);
   bioReadBuffer(b_cache,&shdr,sizeof(Elf386_External_Shdr));
   if(bioEOF(b_cache)) break;
   if(ELF_WORD(shdr.sh_type) == type)
   {
     tableptr = ELF_WORD(shdr.sh_offset);
     *nitems = ELF_WORD(shdr.sh_size)/ELF_WORD(shdr.sh_entsize);
     *link = ELF_HALF(shdr.sh_link);
     *ent_size = ELF_WORD(shdr.sh_entsize);
     break;
   }
  }
  bioSeek(b_cache, fpos, SEEKF_START);
  return tableptr;
}

static BGLOBAL namecache = &bNull;
static BGLOBAL namecache2 = &bNull;
static BGLOBAL elfcache = &bNull;

static linearArray *va_map_phys,* va_map_virt;

static tCompare __FASTCALL__ vamap_comp_virt(const void __HUGE__ *v1,const void __HUGE__ *v2)
{
  const struct tag_elfVAMap __HUGE__ *pnam1,__HUGE__ *pnam2;
  pnam1 = (const struct tag_elfVAMap __HUGE__ *)v1;
  pnam2 = (const struct tag_elfVAMap __HUGE__ *)v2;
  return __CmpLong__(pnam1->va,pnam2->va);
}

static tCompare __FASTCALL__ vamap_comp_phys(const void __HUGE__ *v1,const void __HUGE__ *v2)
{
  const struct tag_elfVAMap __HUGE__ *pnam1,__HUGE__ *pnam2;
  pnam1 = (const struct tag_elfVAMap __HUGE__ *)v1;
  pnam2 = (const struct tag_elfVAMap __HUGE__ *)v2;
  return __CmpLong__(pnam1->foff,pnam2->foff);
}

static unsigned long __FASTCALL__ elfVA2PA(unsigned long va)
{
  unsigned long i;
  for(i = 0; i < va_map_virt->nItems;i++)
  {
    struct tag_elfVAMap __HUGE__ *evm;
    evm = &((struct tag_elfVAMap __HUGE__ *)va_map_virt->data)[i];
    if(va >= evm->va && va < evm->va + evm->size) return evm->foff + (va - evm->va);
  }
  return 0L;
}

static unsigned long __FASTCALL__ elfPA2VA(unsigned long pa)
{
  unsigned long i,ret;
  ret = 0L;
  for(i = 0; i < va_map_phys->nItems;i++)
  {
    struct tag_elfVAMap __HUGE__ *evm;
    evm = &((struct tag_elfVAMap __HUGE__ *)va_map_phys->data)[i];
    if(pa >= evm->foff && pa < evm->foff + evm->size)
    {
      ret = evm->va + (pa - evm->foff);
      break;
    }
  }
  return ret;
}

static tBool __FASTCALL__ elfLowMemFunc( unsigned long need_mem )
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

static void __NEAR__ __FASTCALL__ elf386_readnametable(unsigned long off,char *buf,unsigned blen)
{
  unsigned long foff;
  Elf386_External_Shdr sh;
  unsigned char ch;
  unsigned freq;
  BGLOBAL b_cache,b_cache2;
  b_cache = namecache;
  b_cache2 = namecache2;
  foff = ELF_WORD(elf.e_shoff)+(ELF_HALF(elf.e_shstrndx)*sizeof(Elf386_External_Shdr));
  bioSeek(b_cache2,foff,SEEKF_START);
  bioReadBuffer(b_cache2,&sh,sizeof(Elf386_External_Shdr));
  foff = ELF_WORD(sh.sh_offset) + off;
  freq = 0;
  while(1)
  {
     bioSeek(b_cache,foff++,SEEKF_START);
     ch = bioReadByte(b_cache);
     buf[freq++] = ch;
     if(!ch || freq >= blen || bioEOF(b_cache)) break;
  }
}

static void __NEAR__ __FASTCALL__ elf386_readnametableex(unsigned long off,char *buf,unsigned blen)
{
  unsigned long foff;
  Elf386_External_Shdr sh;
  unsigned char ch;
  unsigned freq;
  BGLOBAL b_cache,b_cache2;
  b_cache = namecache;
  b_cache2 = namecache2;
  if(ELF_WORD(elf.e_shoff))
  {
    foff = ELF_WORD(elf.e_shoff)+active_shtbl*sizeof(Elf386_External_Shdr);
    bioSeek(b_cache2,foff,SEEKF_START);
    bioReadBuffer(b_cache2,&sh,sizeof(Elf386_External_Shdr));
    foff = ELF_WORD(sh.sh_offset) + off;
  }
  /* if section headers are lost then active_shtbl should directly point to
     required string table */
  else  foff = active_shtbl + off;
  freq = 0;
  while(1)
  {
     bioSeek(b_cache,foff++,SEEKF_START);
     ch = bioReadByte(b_cache);
     buf[freq++] = ch;
     if(!ch || freq >= blen || bioEOF(b_cache)) break;
  }
}

static const char * __NEAR__ __FASTCALL__ elf_class(unsigned char id)
{
  switch(id)
  {
    case ELFCLASSNONE:	return "Invalid";
    case ELFCLASS32:	return "32-bit";
    case ELFCLASS64:	return "64-bit";
    default:		return "Unknown";
  }
}

static const char * __NEAR__ __FASTCALL__ elf_data(unsigned char id)
{
  switch(id)
  {
    case ELFDATANONE:	return "Invalid";
    case ELFDATA2LSB:	return "LSB - little endian";
    case ELFDATA2MSB:	return "MSB - big endian";
    default:		return "Unknown";
  }
}

static const char *__NEAR__ __FASTCALL__ elf_otype(unsigned id)
{
  switch(id)
  {
    case ET_NONE:	return "none";
    case ET_REL:	return "relocatable";
    case ET_EXEC:	return "executable";
    case ET_DYN:	return "shared object";
    case ET_CORE:	return "core";
    case ET_LOOS:	return "OS-specific low";
    case ET_HIOS:	return "OS-specific high";
    case ET_LOPROC:	return "processor-specific low";
    case ET_HIPROC:	return "processor-specific high";
    default:		return "Unknown";
  }
}

/*
    only common machine types are used, add remaining if needed
*/

static const char *__NEAR__ __FASTCALL__ elf_machine(unsigned id)
{
  switch(id)
  {
    case EM_NONE:	return "None";
    case EM_M32:	return "AT&T WE32100";
    case EM_SPARC:	return "Sun SPARC";
    case EM_386:	return "Intel 386";
    case EM_68K:	return "Motorola m68k";
    case EM_88K:	return "Motorola m88k";
    case EM_860:	return "Intel 80860";
    case EM_MIPS:	return "MIPS I";
    case EM_S370:	return "IBM System/370";
    case EM_MIPS_RS3_LE:return "MIPS R3000";
    case EM_PARISC:	return "HP PA-RISC";
    case EM_SPARC32PLUS:return "SPARC v8plus";
    case EM_960:	return "Intel 80960";
    case EM_PPC:	return "Power PC 32-bit";
    case EM_PPC64:	return "Power PC 64-bit";
    case EM_S390:	return "IBM System/390";

    case EM_ARM:	return "ARM";
    case EM_ALPHA:	return "DEC Alpha";
    case EM_SPARCV9:	return "SPARC v9 64-bit";

    case EM_IA_64:	return "Intel IA-64";
    case EM_X86_64:	return "AMD x86-64";
    case EM_VAX:	return "DEC VAX";
    default:		return "Unknown";
  }
}

static const char *__NEAR__ __FASTCALL__ elf_version(unsigned long id)
{
  switch(id)
  {
    case EV_NONE:    return "Invalid";
    case EV_CURRENT: return "Current";
    default:         return "Unknown";
  }
}

static const char *__NEAR__ __FASTCALL__ elf_osabi(unsigned char id)
{
  switch(id)
  {
    case ELFOSABI_SYSV:		return "UNIX System V";
    case ELFOSABI_HPUX:		return "HP-UX";
    case ELFOSABI_NETBSD:	return "NetBSD";
    case ELFOSABI_LINUX:	return "GNU/Linux";
    case ELFOSABI_HURD:		return "GNU/Hurd";
    case ELFOSABI_86OPEN:	return "86Open";
    case ELFOSABI_SOLARIS:	return "Solaris";
    case ELFOSABI_MONTEREY:	return "Monterey";
    case ELFOSABI_IRIX:		return "IRIX";
    case ELFOSABI_FREEBSD:	return "FreeBSD";
    case ELFOSABI_TRU64:	return "TRU64 UNIX";
    case ELFOSABI_MODESTO:	return "Novell Modesto";
    case ELFOSABI_OPENBSD:	return "OpenBSD";
    case ELFOSABI_ARM:		return "ARM";
    case ELFOSABI_STANDALONE:	return "Standalone (embedded) application";
    default:			return "Unknown";
  }
}


static unsigned long __FASTCALL__ ShowELFHeader( void )
{
  unsigned long fpos;
  TWindow *w;
  char hdr[81];
  unsigned keycode;
  unsigned long entrya;
  fpos = BMGetCurrFilePos();
  entrya = elfVA2PA(ELF_WORD(elf.e_entry));
  sprintf(hdr," ELF (Executable and Linking Format) ");
  w = CrtDlgWndnls(hdr,74,19);
  twGotoXY(1,1);
  twPrintF("Signature                         = %02X %02X %02X %02XH (%c%c%c%c)\n"
           "File class                        = %02XH (%s)\n"
           "Data encoding                     = %02XH (%s)\n"
           "ELF header version                = %02XH (%s)\n"
           "Operating system / ABI            = %02XH (%s)\n"
           "ABI version                       = %02XH (%d)\n"
           "Object file type                  = %04XH (%s)\n"
           "Machine architecture              = %04XH (%s)\n"
           "Object file version               = %08XH (%s)\n"
	    ,elf.e_ident[EI_MAG0],	elf.e_ident[EI_MAG1]
	    ,elf.e_ident[EI_MAG2],	elf.e_ident[EI_MAG3]
	    ,elf.e_ident[EI_MAG0],	elf.e_ident[EI_MAG1]
	    ,elf.e_ident[EI_MAG2],	elf.e_ident[EI_MAG3]
	    ,elf.e_ident[EI_CLASS], 	elf_class(elf.e_ident[EI_CLASS])
	    ,elf.e_ident[EI_DATA],	elf_data(elf.e_ident[EI_DATA])
	    ,elf.e_ident[EI_VERSION],	elf_version(elf.e_ident[EI_VERSION])
	    ,elf.e_ident[EI_OSABI],	elf_osabi(elf.e_ident[EI_OSABI])
	    ,elf.e_ident[EI_ABIVERSION],elf.e_ident[EI_ABIVERSION]
	    ,ELF_HALF(elf.e_type),	elf_otype(ELF_HALF(elf.e_type))
	    ,ELF_HALF(elf.e_machine),	elf_machine(ELF_HALF(elf.e_machine))
	    ,ELF_WORD(elf.e_version),	elf_version(ELF_WORD(elf.e_version))
  );
  twSetColorAttr(dialog_cset.entry);
  twPrintF("Entry point VA                    = %08lXH (offset: %08lXH)"
           ,ELF_WORD(elf.e_entry),entrya);
  twClrEOL(); twPrintF("\n");
  twSetColorAttr(dialog_cset.main);
  twPrintF("Program header table offset       = %08lXH\n"
           "Section header table offset       = %08lXH\n"
           "Processor specific flag           = %08lXH\n"
           "ELF header size (bytes)           = %04XH\n"
           "Program header table entry size   = %04XH\n"
           "Program header table entry count  = %04XH\n"
           "Section header table entry size   = %04XH\n"
           "Section header table entry count  = %04XH\n"
           "Section header string table index = %04XH"
           ,ELF_WORD(elf.e_phoff)
           ,ELF_WORD(elf.e_shoff)
           ,ELF_WORD(elf.e_flags)
           ,ELF_HALF(elf.e_ehsize)
           ,ELF_HALF(elf.e_phentsize)
           ,ELF_HALF(elf.e_phnum)
           ,ELF_HALF(elf.e_shentsize)
           ,ELF_HALF(elf.e_shnum)
           ,ELF_HALF(elf.e_shstrndx));
  while(1)
  {
    keycode = GetEvent(drawEmptyPrompt,w);
    if(keycode == KE_ENTER) { fpos = entrya; break; }
    else
      if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
  }
  CloseWnd(w);
  return fpos;
}

static const char * __NEAR__ __FASTCALL__ elf_encode_p_type(long p_type)
{
   switch(p_type)
   {
      case PT_NULL: return "Unusable";
      case PT_LOAD: return "Loadable";
      case PT_DYNAMIC: return "Dynalinking";
      case PT_INTERP: return "Interpreter";
      case PT_NOTE:  return "Auxiliary";
      case PT_SHLIB: return "Unspecified";
      case PT_PHDR:  return "header itself";
      case PT_NUM: return "Number of types";
      case PT_LOPROC: return "Low processor";
      case PT_HIPROC: return "High processor";
      default:  return "Unknown";
   }
}

static tBool __FASTCALL__ __elfReadPrgHdr(BGLOBAL handle,memArray *obj,unsigned nnames)
{
 size_t i;
  bioSeek(handle,ELF_WORD(elf.e_phoff),SEEKF_START);
  for(i = 0;i < nnames;i++)
  {
   unsigned long fp;
   char stmp[80];
   Elf386_External_Phdr phdr;
   if(IsKbdTerminate() || bioEOF(handle)) break;
   fp = bioTell(handle);
   bioReadBuffer(handle,&phdr,sizeof(Elf386_External_Phdr));
   bioSeek(handle,fp+ELF_HALF(elf.e_phentsize),SEEKF_START);
   sprintf(stmp,"%-15s %08lX %08lX %08lX %08lX %08lX %c%c%c %08lX",
                elf_encode_p_type(ELF_WORD(phdr.p_type)),
                (unsigned long)ELF_WORD(phdr.p_offset),
                (unsigned long)ELF_WORD(phdr.p_vaddr),
                (unsigned long)ELF_WORD(phdr.p_paddr),
                (unsigned long)ELF_WORD(phdr.p_filesz),
                (unsigned long)ELF_WORD(phdr.p_memsz),
                (ELF_WORD(phdr.p_flags) & PF_X) == PF_X ? 'X' : ' ',
                (ELF_WORD(phdr.p_flags) & PF_W) == PF_W ? 'W' : ' ',
                (ELF_WORD(phdr.p_flags) & PF_R) == PF_R ? 'R' : ' ',
                (unsigned long)ELF_WORD(phdr.p_align)
          );
   if(!ma_AddString(obj,stmp,True)) break;
  }
  return True;
}

static const char * __NEAR__ __FASTCALL__ elf_encode_sh_type(long sh_type)
{
   switch(sh_type)
   {
      case SHT_NULL: return "NULL";
      case SHT_PROGBITS: return "PRGBTS";
      case SHT_SYMTAB: return "SYMTAB";
      case SHT_STRTAB: return "STRTAB";
      case SHT_RELA:  return "RELA";
      case SHT_HASH: return "HSHTAB";
      case SHT_DYNAMIC:  return "DYNLNK";
      case SHT_NOTE: return "NOTES";
      case SHT_NOBITS: return "NOBITS";
      case SHT_REL:  return "REL";
      case SHT_SHLIB: return "UNSPEC";
      case SHT_DYNSYM: return "DYNSYM";
      case SHT_NUM: return "NTYPES";
      case SHT_GNU_verdef: return "VERDEF";
      case SHT_GNU_verneed: return "VERNED";
      case SHT_GNU_versym: return "SYMVER";
      case SHT_LOPROC: return "LOPROC";
      case SHT_HIPROC: return "HIPROC";
      case SHT_LOUSER: return "LOUSER";
      case SHT_HIUSER: return "HIUSER";
      default:  return "UNK";
   }
}

static tBool __FASTCALL__ __elfReadSecHdr(BGLOBAL handle,memArray *obj,unsigned nnames)
{
 size_t i;
  bioSeek(handle,ELF_WORD(elf.e_shoff),SEEKF_START);
  for(i = 0;i < nnames;i++)
  {
   Elf386_External_Shdr shdr;
   char tmp[17];
   unsigned long fp;
   char stmp[80];
   if(IsKbdTerminate() || bioEOF(handle)) break;
   fp = bioTell(handle);
   bioReadBuffer(handle,&shdr,sizeof(Elf386_External_Shdr));
   elf386_readnametable(ELF_WORD(shdr.sh_name),tmp,sizeof(tmp));
   bioSeek(handle,fp+ELF_HALF(elf.e_shentsize),SEEKF_START);
   tmp[16] = 0;
   sprintf(stmp,"%-16s %-6s %c%c%c %08lX %08lX %08lX %04hX %04hX %04hX %04hX",
                tmp,
                elf_encode_sh_type(ELF_WORD(shdr.sh_type)),
                (ELF_WORD(shdr.sh_flags) & SHF_WRITE) == SHF_WRITE ? 'W' : ' ',
                (ELF_WORD(shdr.sh_flags) & SHF_ALLOC) == SHF_ALLOC ? 'A' : ' ',
                (ELF_WORD(shdr.sh_flags) & SHF_EXECINSTR) == SHF_EXECINSTR ? 'X' : ' ',
                (unsigned long)ELF_WORD(shdr.sh_addr),
                (unsigned long)ELF_WORD(shdr.sh_offset),
                (unsigned long)ELF_WORD(shdr.sh_size),
                (tUInt16)ELF_WORD(shdr.sh_link),
                (tUInt16)ELF_WORD(shdr.sh_info),
                (tUInt16)ELF_WORD(shdr.sh_addralign),
                (tUInt16)ELF_WORD(shdr.sh_entsize)
          );
    if(!ma_AddString(obj,stmp,True)) break;
  }
  return True;
}

static const char * __NEAR__ __FASTCALL__ elf_SymTabType(char type)
{
  switch(ELF_ST_TYPE(type))
  {
    case STT_NOTYPE:  return "NoType";
    case STT_OBJECT:  return "Object";
    case STT_FUNC:    return "Func. ";
    case STT_SECTION: return "Sect. ";
    case STT_FILE:    return "File  ";
    case STT_NUM:     return "Number";
    case STT_LOPROC:  return "LoProc";
    case STT_HIPROC:  return "HiProc";
    default: return "Unknwn";
  }
}

static const char * __NEAR__ __FASTCALL__ elf_SymTabBind(char type)
{
  switch(ELF_ST_BIND(type))
  {
    case STB_LOCAL:  return "Local ";
    case STB_GLOBAL: return "Global";
    case STB_WEAK:   return "Weak  ";
    case STB_NUM:    return "Number";
    case STB_LOPROC: return "LoProc";
    case STB_HIPROC: return "HiProc";
    default: return "Unknwn";
  }
}

static const char * __NEAR__ __FASTCALL__ elf_SymTabShNdx(unsigned idx)
{
  static char ret[10];
  switch(idx)
  {
    case SHN_UNDEF:  return "Undef ";
    case SHN_LOPROC: return "LoProc";
    case SHN_HIPROC: return "HiProc";
    case SHN_ABS:    return "Abs.  ";
    case SHN_COMMON: return "Common";
    case SHN_HIRESERVE: return "HiRes.";
    default: sprintf(ret,"%04XH ",idx); return ret;
  }
}

static tBool __NEAR__ __FASTCALL__ ELF_IS_SECTION_PHYSICAL(unsigned sec_num)
{
  return !(sec_num == SHN_UNDEF || sec_num == SHN_LOPROC ||
           sec_num == SHN_HIPROC || sec_num == SHN_ABS ||
           sec_num == SHN_COMMON || sec_num == SHN_HIRESERVE);
}

static unsigned __FASTCALL__ __elfGetNumSymTab( BGLOBAL handle )
{
  UNUSED(handle);
  return __elfNumSymTab;
}

static tBool __FASTCALL__ __elfReadSymTab(BGLOBAL handle,memArray *obj,unsigned nsym)
{
 size_t i;
 char text[31];
  bioSeek(handle,__elfSymPtr,SEEK_SET);
  for(i = 0;i < nsym;i++)
  {
   unsigned long fp;
   char stmp[80];
   Elf386_External_Sym sym;
   if(IsKbdTerminate() || bioEOF(handle)) break;
   fp = bioTell(handle);
   bioReadBuffer(handle,&sym,sizeof(Elf386_External_Sym));
   bioSeek(handle,fp+__elfSymEntSize,SEEKF_START);
   elf386_readnametableex(ELF_WORD(sym.st_name),text,sizeof(text));
   text[sizeof(text)-1] = 0;
   sprintf(stmp,"%-31s %08lX %08lX %04hX %s %s %s"
               ,text
               ,(unsigned long)ELF_WORD(sym.st_value)
               ,(unsigned long)ELF_WORD(sym.st_size)
               ,ELF_HALF(sym.st_other)
               ,elf_SymTabType(sym.st_info[0])
               ,elf_SymTabBind(sym.st_info[0])
               ,elf_SymTabShNdx(ELF_HALF(sym.st_shndx))
               );
   if(!ma_AddString(obj,stmp,True)) break;
  }
  return True;
}

static tBool __NEAR__ __FASTCALL__ __elfReadDynTab(BGLOBAL handle,memArray *obj, unsigned ntbl,unsigned long entsize)
{
 size_t i;
 char sout[80];
 unsigned len,rlen;
  for(i = 0;i < ntbl;i++)
  {
   unsigned long fp;
   char stmp[80];
   Elf386_External_Dyn pdyn;
   fp = bioTell(handle);
   bioReadBuffer(handle,&pdyn,sizeof(Elf386_External_Dyn));
   bioSeek(handle,fp+entsize,SEEKF_START);
   fp = bioTell(handle);
   elf386_readnametableex(ELF_WORD(pdyn.d_tag),sout,sizeof(sout));
   len = strlen(sout);
   if(len > 56) len = 53;
   if(IsKbdTerminate() || bioEOF(handle)) break;
   strncpy(stmp,sout,len);
   stmp[len] = 0;
   if(len > 56) strcat(stmp,"...");
   rlen = strlen(stmp);
   if(rlen < 60) { memset(&stmp[rlen],' ',60-rlen); stmp[60] = 0; }
   sprintf(&stmp[strlen(stmp)]," vma=%08lXH",(unsigned long)ELF_WORD(pdyn.d_un.d_val));
   if(!ma_AddString(obj,stmp,True)) break;
   bioSeek(handle,fp,SEEKF_START);
  }
  return True;
}

static unsigned __FASTCALL__ Elf386PrgHdrNumItems(BGLOBAL handle)
{
   UNUSED(handle);
   return ELF_HALF(elf.e_phnum);
}

static unsigned long __FASTCALL__ ShowPrgHdrElf(void)
{
  unsigned long fpos;
  int ret;
  fpos = BMGetCurrFilePos();
  ret = fmtShowList(Elf386PrgHdrNumItems,__elfReadPrgHdr,
                    " type            fileoffs virtaddr physaddr filesize memsize  flg align   ",
                    LB_SELECTIVE,NULL);
  if(ret != -1)
  {
    Elf386_External_Phdr it;
    bmSeek(ELF_WORD(elf.e_phoff)+sizeof(Elf386_External_Phdr)*ret,SEEKF_START);
    bmReadBuffer(&it,sizeof(Elf386_External_Phdr));
    fpos = ELF_WORD(it.p_offset);
  }
  return fpos;
}

static unsigned __FASTCALL__ Elf386SecHdrNumItems(BGLOBAL handle)
{
  UNUSED(handle);
  return IsSectionsPresent ? ELF_HALF(elf.e_shnum) : 0;
}

static unsigned long __FASTCALL__ ShowSecHdrElf(void)
{
  unsigned long fpos;
  int ret;
  fpos = BMGetCurrFilePos();
  ret = fmtShowList(Elf386SecHdrNumItems,__elfReadSecHdr,
                    " name             type   flg virtaddr fileoffs   size   link info algn esiz",
                   LB_SELECTIVE,NULL);
  if(ret != -1)
  {
    Elf386_External_Shdr it;
    bmSeek(ELF_WORD(elf.e_shoff)+sizeof(Elf386_External_Shdr)*ret,SEEKF_START);
    bmReadBuffer(&it,sizeof(Elf386_External_Shdr));
    fpos = ELF_WORD(it.sh_offset);
  }
  return fpos;
}

static unsigned long __calcSymEntry(BGLOBAL handle,unsigned long num,tBool display_msg)
{
   Elf386_External_Sym it;
   Elf386_External_Shdr sec;
   unsigned long ffpos,fpos = 0L;
   ffpos = bioTell(handle);
   bioSeek(handle,__elfSymPtr+__elfSymEntSize*num,SEEKF_START);
   bioReadBuffer(handle,&it,sizeof(Elf386_External_Sym));
   bioSeek(handle,ELF_WORD(elf.e_shoff)+sizeof(Elf386_External_Shdr)*ELF_HALF(it.st_shndx),SEEKF_START);
   bioReadBuffer(handle,&sec,sizeof(Elf386_External_Shdr));
   bioSeek(handle,ffpos,SEEKF_START);
   if(ELF_IS_SECTION_PHYSICAL(ELF_HALF(it.st_shndx)))
/*
   In relocatable files, st_value holds alignment constraints for a
   symbol whose section index is SHN_COMMON.

   In relocatable files, st_value holds a section offset for a defined
   symbol. That is, st_value is an offset from the beginning of the
   section that st_shndx identifies.

   In executable and shared object files, st_value holds a virtual
   address. To make these files' symbols more useful for the dynamic
   linker, the section offset (file interpretation) gives way to a
   virtual address (memory interpretation) for which the section number
   is irrelevant.
*/
     fpos = ELF_HALF(elf.e_type) == ET_REL ? 
            ELF_WORD(sec.sh_offset) + ELF_WORD(it.st_value):
            elfVA2PA(ELF_WORD(it.st_value));
   else
     if(display_msg) ErrMessageBox(NO_ENTRY,BAD_ENTRY);
   return fpos;
}

static unsigned long __NEAR__ __FASTCALL__ displayELFsymtab( void )
{
  unsigned long fpos;
  int ret;
  fpos = BMGetCurrFilePos();
  ret = fmtShowList(__elfGetNumSymTab,__elfReadSymTab,
                    " Name                            Value    Size     Oth. Type   Bind   Sec# ",
                    LB_SELECTIVE,NULL);
  if(ret != -1)
  {
    unsigned long ea;
    ea = __calcSymEntry(bmbioHandle(),ret,True);
    fpos = ea ? ea : fpos;
  }
  return fpos;
}

static unsigned long __NEAR__ __FASTCALL__ displayELFdyntab(unsigned long dynptr,unsigned long nitem,long entsize)
{
  unsigned long fpos;
  memArray *obj;
  BGLOBAL handle;
  unsigned ndyn;
  fpos = BMGetCurrFilePos();
  ndyn = (unsigned)nitem;
  if(!(obj = ma_Build(ndyn,True))) return fpos;
  handle = elfcache;
  bioSeek(handle,dynptr,SEEK_SET);
  if(__elfReadDynTab(handle,obj,ndyn,entsize))
  {
    int ret;
    ret = ma_Display(obj," Dynamic section ",LB_SELECTIVE | LB_SORTABLE,-1);
    if(ret != -1)
    {
       char *addr;
       addr = strstr(obj->data[ret],"vma=");
       if(addr)
       {
         unsigned long addr_probe;
         addr_probe = strtoul(&addr[4],NULL,16);
         if(addr_probe && addr_probe >= elf_min_va)
         {
           addr_probe = elfVA2PA(addr_probe);
           if(addr_probe && addr_probe < bmGetFLength()) fpos = addr_probe;
           else goto not_entry;
         }
         else goto not_entry;
       }
       else
       {
         not_entry:
         ErrMessageBox(NOT_ENTRY,NULL);
       }
    }
  }
  ma_Destroy(obj);
  return fpos;
}

static unsigned long __FASTCALL__ ShowELFSymTab( void )
{
  unsigned long fpos;
  fpos = BMGetCurrFilePos();
  if(!__elfSymPtr) { NotifyBox(NOT_ENTRY," ELF symbol table "); return fpos; }
  active_shtbl = __elfSymShTbl;
  return displayELFsymtab();
}

static unsigned long __FASTCALL__ ShowELFDynSec( void )
{
  unsigned long fpos,dynptr, number;
  unsigned nitems,ent_size = UINT_MAX;
  fpos = BMGetCurrFilePos();
  dynptr = findSHEntry(bmbioHandle(), SHT_DYNSYM, &number, &active_shtbl, &ent_size);
  if(!dynptr)
  {
    dynptr = findPHPubSyms(&nitems, &ent_size, &active_shtbl);
    number = nitems;
  }
  if(!dynptr) { NotifyBox(NOT_ENTRY," ELF dynamic symbol table "); return fpos; }
  return displayELFdyntab(dynptr,number,ent_size);
}

/***************************************************************************/
/************************ RELOCATION FOR ELF386 ****************************/
/***************************************************************************/
typedef struct tagElfRefChain
{
  tUInt32  offset;
  tUInt32  info;
  tUInt32  addend;
  tUInt32  sh_idx;
}Elf_Reloc;
static linearArray *CurrElfChain = NULL;

static tCompare __FASTCALL__ compare_elf_reloc(const void __HUGE__ *e1,const void __HUGE__ *e2)
{
  const Elf_Reloc __HUGE__ *p1,__HUGE__ *p2;
  p1 = (const Elf_Reloc __HUGE__ *)e1;
  p2 = (const Elf_Reloc __HUGE__ *)e2;
  return __CmpLong__(p1->offset,p2->offset);
}

static unsigned long get_f_offset(unsigned long r_offset,tUInt32 sh_link)
{
  /*
    r_offset member gives the location at which to apply the relocation
    action. For a relocatable file, the value is the byte offset from
    the beginning of the section to the storage unit affected by the
    relocation. For an executable file or a shared object, the value is
    the virtual address of the storage unit affected by the relocation.
  */
  unsigned long f_offset;
  BGLOBAL handle = elfcache;
  switch(ELF_HALF(elf.e_type))
  {
     case ET_REL:
                {
                  Elf386_External_Shdr shdr;
                  unsigned long fp;
                  fp = bioTell(handle);
                  bioSeek(handle,ELF_WORD(elf.e_shoff),SEEKF_START);
		  bioSeek(handle,sh_link*sizeof(Elf386_External_Shdr),SEEKF_CUR);
                  bioReadBuffer(handle,&shdr,sizeof(Elf386_External_Shdr));
		  bioSeek(handle,fp,SEEKF_START);
		  f_offset = ELF_WORD(shdr.sh_offset) + r_offset;
                }
     default: f_offset = elfVA2PA(r_offset);
              break;
  }
  return f_offset;
}

static void __NEAR__ __FASTCALL__ __elfReadRelSection(tUInt32 offset,
            			                      tUInt32 size,
            			                      tUInt32 sh_link,
            			                      tUInt32 info,
            			                      tUInt32 entsize)
{
  BGLOBAL handle = elfcache,handle2 = namecache;
  size_t i,nitems;
  Elf386_External_Rel relent;
  unsigned long fp, sfp;
  fp = bioTell(handle);
  bioSeek(handle,offset,SEEKF_START);
  nitems = (size_t)(size / entsize);
  sfp = bioTell(handle2);
  for(i = 0;i < nitems;i++)
  {
    Elf_Reloc erc;
    bioReadBuffer(handle,&relent,sizeof(Elf386_External_Rel));
    if(entsize > sizeof(Elf386_External_Rel)) bioSeek(handle,entsize-sizeof(Elf386_External_Rel),SEEKF_CUR);
    erc.offset = get_f_offset(ELF_WORD(relent.r_offset),info);
    erc.info = ELF_WORD(relent.r_info);
    /* Entries of type Elf32_Rel store an implicit addend in the
       location to be modified */
    bioSeek(handle2, erc.offset, SEEKF_START);
    switch(ELF32_R_TYPE(erc.info))
    {
      case R_386_GNU_8:
      case R_386_GNU_PC8:
               erc.addend = bioReadByte(handle2);
               break;
      case R_386_GNU_16:
      case R_386_GNU_PC16:
               erc.addend = bioReadWord(handle2);
               break;
      default:
               erc.addend = bioReadDWord(handle2);
               break;
    }
    erc.sh_idx = sh_link;
    if(!la_AddData(CurrElfChain,&erc,NULL)) break;
  }
  bioSeek(handle2,sfp,SEEKF_START);
  bioSeek(handle,fp,SEEKF_START);
}						

static void __NEAR__ __FASTCALL__ __elfReadRelaSection(tUInt32 offset,
            			                       tUInt32 size,
            			                       tUInt32 sh_link,
            			                       tUInt32 info,
            			                       tUInt32 entsize)
{
  BGLOBAL handle = elfcache;
  size_t i,nitems;
  Elf386_External_Rela relent;
  unsigned long fp;
  fp = bioTell(handle);
  bioSeek(handle,offset,SEEKF_START);
  nitems = (size_t)(size / entsize);
  for(i = 0;i < nitems;i++)
  {
    Elf_Reloc erc;
    bioReadBuffer(handle,&relent,sizeof(Elf386_External_Rela));
    if(entsize > sizeof(Elf386_External_Rela)) bioSeek(handle,entsize-sizeof(Elf386_External_Rela),SEEKF_CUR);
    erc.offset = get_f_offset(ELF_WORD(relent.r_offset),info);
    erc.info = ELF_WORD(relent.r_info);
    erc.addend = ELF_WORD(relent.r_addend);
    erc.sh_idx = sh_link;
    if(!la_AddData(CurrElfChain,&erc,NULL)) break;
  }
  bioSeek(handle,fp,SEEKF_START);
}						

static void __NEAR__ __FASTCALL__ buildElf386RelChain( void )
{
  size_t i,_nitems;
  TWindow *w,*usd;
  BGLOBAL handle = elfcache;
  unsigned long fp;
  usd = twUsedWin();
  if(!(CurrElfChain = la_Build(0,sizeof(Elf_Reloc),MemOutBox))) return;
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  twUseWin(w);
  twGotoXY(1,1);
  twPutS(BUILD_REFS);
  twUseWin(usd);
  fp = bioTell(handle);
  if(IsSectionsPresent) /* Section headers are present */
  {     
    bioSeek(handle,ELF_WORD(elf.e_shoff),SEEKF_START);
    _nitems = ELF_HALF(elf.e_shnum);
    for(i = 0;i < _nitems;i++)
    {
      Elf386_External_Shdr shdr;
      if(IsKbdTerminate() || bioEOF(handle)) break;
      bioReadBuffer(handle,&shdr,sizeof(Elf386_External_Shdr));
      switch(ELF_WORD(shdr.sh_type))
      {
        case SHT_REL: __elfReadRelSection(ELF_WORD(shdr.sh_offset),
               			          ELF_WORD(shdr.sh_size),
            			          ELF_WORD(shdr.sh_link),
            			          ELF_WORD(shdr.sh_info),
            			          ELF_WORD(shdr.sh_entsize));
                      break;
        case SHT_RELA: __elfReadRelaSection(ELF_WORD(shdr.sh_offset),
            				    ELF_WORD(shdr.sh_size),
            				    ELF_WORD(shdr.sh_link),
            			            ELF_WORD(shdr.sh_info),
            			            ELF_WORD(shdr.sh_entsize));
                      break;
        default: break;
     }
    } 
  }
  else if(ELF_HALF(elf.e_type) != ET_REL)
  {
    /* If section headers are lost then information can be taken
       from program headers
    */
    unsigned long dyn_ptr,dynptr,link,type;
    unsigned tsize,nitems;
    dynptr = findPHEntry(PT_DYNAMIC,&nitems);
    link = elfVA2PA(findPHDynEntry(DT_SYMTAB,dynptr,nitems));
    if(dynptr)
    {
      dyn_ptr = elfVA2PA(findPHDynEntry(DT_RELA,dynptr,nitems));
      if(dyn_ptr)
      {
        tsize = findPHDynEntry(DT_RELASZ,dynptr,nitems);
	__elfReadRelaSection(dyn_ptr,
            		     tsize,
            		     link,
            		     0,/* only executable can lose sections */
            		     sizeof(Elf386_External_Rela));
      }
      dyn_ptr = elfVA2PA(findPHDynEntry(DT_REL,dynptr,nitems));
      if(dyn_ptr)
      {
        tsize = findPHDynEntry(DT_RELSZ,dynptr,nitems);
	__elfReadRelSection(dyn_ptr,
        		    tsize,
            		    link,
            		    0,/* only executable can lose sections */
            		    sizeof(Elf386_External_Rel));
      }
      dyn_ptr = elfVA2PA(findPHDynEntry(DT_JMPREL,dynptr,nitems));
      if(dyn_ptr)
      {
        tsize = findPHDynEntry(DT_PLTRELSZ,dynptr,nitems);
        type = findPHDynEntry(DT_PLTREL,dynptr,nitems);
        if(type == DT_REL)
	__elfReadRelSection(dyn_ptr,
        		    tsize,
            		    link,
            		    0,/* only executable can lose sections */
            		    sizeof(Elf386_External_Rel));
        else
	__elfReadRelaSection(dyn_ptr,
        	 	     tsize,
            		     link,
            		     0,/* only executable can lose sections */
            		     sizeof(Elf386_External_Rel));
      }
    }
  }
  la_Sort(CurrElfChain,compare_elf_reloc);
  bioSeek(handle,fp,SEEKF_START);
  CloseWnd(w);
  return;
}

static Elf_Reloc __HUGE__ * __NEAR__ __FASTCALL__ __found_ElfRel(unsigned long offset)
{
  Elf_Reloc key;
  if(!CurrElfChain) buildElf386RelChain();
  key.offset = offset;
  return la_Find(CurrElfChain,&key,compare_elf_reloc);
}

static tBool __NEAR__ __FASTCALL__ __readRelocName(Elf_Reloc __HUGE__ *erl, char *buff, size_t cbBuff)
{
  tUInt32 r_sym;
  Elf386_External_Shdr shdr;
  Elf386_External_Sym sym;
  BGLOBAL handle = elfcache;
  unsigned long fp;
  tBool ret = True;
  r_sym = ELF32_R_SYM(erl->info);
  fp = bioTell(handle);
  if(IsSectionsPresent) /* Section headers are present */
  {
     bioSeek(handle,ELF_WORD(elf.e_shoff)+erl->sh_idx*sizeof(Elf386_External_Shdr),SEEKF_START);
     bioReadBuffer(handle,&shdr,sizeof(Elf386_External_Shdr));
     bioSeek(handle,ELF_WORD(shdr.sh_offset),SEEKF_START);
     /* Minor integrity test */
     ret = ELF_WORD(shdr.sh_type) == SHT_SYMTAB || ELF_WORD(shdr.sh_type) == SHT_DYNSYM;
  }     
  else bioSeek(handle,erl->sh_idx,SEEKF_START);
  if(ret)
  {
    /* We assume that dynsym and symtab are equal */
    unsigned old_active;
    old_active = active_shtbl;
    if(IsSectionsPresent) active_shtbl = ELF_HALF(shdr.sh_link);
    else
    {
      unsigned long dynptr;
      unsigned nitems;
      dynptr = findPHEntry(PT_DYNAMIC,&nitems);
      active_shtbl = elfVA2PA(findPHDynEntry(DT_STRTAB,dynptr,nitems));
    }  
    bioSeek(handle,r_sym*sizeof(Elf386_External_Sym),SEEKF_CUR);
    bioReadBuffer(handle,&sym,sizeof(Elf386_External_Sym));
    elf386_readnametableex(ELF_WORD(sym.st_name),buff,cbBuff);
    buff[cbBuff-1] = '\0';
    active_shtbl = old_active;
    if(!buff[0])
    {
      /* reading name failed - try read at least section name */
      if(IsSectionsPresent)
      {
       if(ELF_ST_TYPE(sym.st_info[0]) == STT_SECTION && 
          ELF_HALF(sym.st_shndx) &&
          ELF_IS_SECTION_PHYSICAL(ELF_HALF(sym.st_shndx)))
       {
         bioSeek(handle,ELF_WORD(elf.e_shoff)+ELF_HALF(sym.st_shndx)*sizeof(Elf386_External_Shdr),SEEKF_START);
         bioReadBuffer(handle,&shdr,sizeof(Elf386_External_Shdr));
         if(!FindPubName(buff, cbBuff, ELF_WORD(shdr.sh_offset)+erl->addend))
                      elf386_readnametable(ELF_WORD(shdr.sh_name),buff,cbBuff);         
       }
      }
      if(!buff[0]) strcpy(buff,"?noname");
    }
  }
  bioSeek(handle,fp,SEEKF_START);
  return ret;
}

static unsigned long __NEAR__ __FASTCALL__ BuildReferStrElf(char *str,
                                                        Elf_Reloc __HUGE__ *erl,
                                                        int flags)
{
  unsigned long retval = RAPREF_DONE;
  tUInt32 r_type;
  tBool ret=False, use_addend = False;
  char buff[300];
  r_type = ELF32_R_TYPE(erl->info);
  buff[0] = 0;
  switch(r_type)
  {
    default:
    case R_386_NONE: /* nothing to do */
    case R_386_COPY: /* nothing to do */
                   retval = RAPREF_NONE;
                   break;
    case R_386_GNU_8:
    case R_386_GNU_16:
    case R_386_32:  /* symbol + addendum */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret)
                   {
		     strcat(str,buff);
		     use_addend = True;
		   }
                   else retval = RAPREF_NONE;
		   break;
    case R_386_GNU_PC8:
    case R_386_GNU_PC16:
    case R_386_PC32: /* symbol + addendum - this */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret)
                   {
		     strcat(str,buff);
		     /* strcat(str,"-.here"); <- it's commented for readability */
		     use_addend = True;
		   }
                   else retval = RAPREF_NONE;
		   break;
    case R_386_GOT32: /* GOT[offset] + addendum - this */
                   strcat(str,"GOT-");
                   strcat(str,Get8Digit(erl->offset));
		   use_addend = True;
		   break;
    case R_386_PLT32: /* PLT[offset] + addendum - this */
                   strcat(str,"PLT-");
                   strcat(str,Get8Digit(erl->offset));
		   use_addend = True;
		   break;
    case R_386_GLOB_DAT:  /* symbol */
    case R_386_JMP_SLOT:  /* symbol */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret) strcat(str,buff);
		   break;
    case R_386_RELATIVE: /* BVA + addendum */
                   strcat(str,"BVA");
		   use_addend = True;
		   break;
    case R_386_GOTOFF: /* symbol + addendum - GOT */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret)
                   {
  		     strcat(str,buff);
		     strcat(str,"-GOT");
		     use_addend = True;
		   }
                   else retval = RAPREF_NONE;
		   break;
    case R_386_GOTPC: /* GOT + addendum - this */
		   strcat(str,"GOT-.here");
		   use_addend = True;
		   break;
  }
  if(erl->addend && use_addend && ret &&
     !(flags & APREF_TRY_LABEL)) /* <- it for readability */
  {
    strcat(str,"+");
    strcat(str,Get8Digit(erl->addend));
  }
  return retval;
}

#define S_INTERPRETER "Interpreter : "

static void __NEAR__ __FASTCALL__ displayELFdyninfo(unsigned long f_off,unsigned nitems)
{
  Elf386_External_Dyn dyntab;
  unsigned long curroff,stroff;
  unsigned i;
  tBool is_add;
  memArray * obj;
  char stmp[80];
  stroff = 0;
  stroff = elfVA2PA(findPHDynEntry(DT_STRTAB,f_off,nitems));
  if(!stroff) { NotifyBox(" String information not found!",NULL); return; }
  bmSeek(f_off,SEEKF_START);
  if(!(obj = ma_Build(0,True))) return;
  strcpy(stmp,S_INTERPRETER);
  curroff = findPHEntry(PT_INTERP, &i);
  if(curroff) bmReadBufferEx(&stmp[sizeof(S_INTERPRETER) - 1],sizeof(stmp)-sizeof(S_INTERPRETER)-1,
                             curroff,SEEKF_START);
  if(!ma_AddString(obj,stmp,True)) goto dyn_end;
  bmSeek(f_off,SEEKF_START);
  for(i = 0;i < nitems;i++)
  {
    bmReadBufferEx(&dyntab,sizeof(dyntab),f_off,SEEKF_START);
    if(bmEOF()) break;
    f_off += sizeof(dyntab);
    is_add = True;
    switch(ELF_WORD(dyntab.d_tag))
    {
      case DT_NULL: goto dyn_end;
      case DT_NEEDED:
                    {
                      strcpy(stmp,"Needed : ");
                      bmReadBufferEx(&stmp[strlen(stmp)],70,ELF_WORD(dyntab.d_un.d_ptr) + stroff,SEEKF_START);
                    }
                    break;
      case DT_SONAME:
                    {
                      strcpy(stmp,"SO name: ");
                      bmReadBufferEx(&stmp[strlen(stmp)],70,ELF_WORD(dyntab.d_un.d_ptr) + stroff,SEEKF_START);
                    }
                    break;
      case DT_RPATH:
                    {
                      strcpy(stmp,"LibPath: ");
                      bmReadBufferEx(&stmp[strlen(stmp)],70,ELF_WORD(dyntab.d_un.d_ptr) + stroff,SEEKF_START);
                    }
                    break;
       default:     is_add = False; break;
    }
    if(is_add) if(!ma_AddString(obj,stmp,True)) break;
  }
  dyn_end:
  ma_Display(obj," Dynamic linking information ",LB_SORTABLE,-1);
  ma_Destroy(obj);
}

static unsigned long __FASTCALL__ ShowELFDynInfo( void )
{
  unsigned long dynptr,fpos;
  unsigned number;
  fpos = BMGetCurrFilePos();
  dynptr = findPHEntry(PT_DYNAMIC,&number);
  if(!dynptr) { NotifyBox(NOT_ENTRY," ELF dynamic linking information "); return fpos; }
  displayELFdyninfo(dynptr,number);
  BMSeek(fpos, SEEKF_START);
  return fpos;
}

static unsigned long __FASTCALL__ AppendELFRef(char *str,unsigned long ulShift,int flags,int codelen,unsigned long r_sh)
{
  char buff[400];
  unsigned long ret = RAPREF_NONE;
  Elf_Reloc __HUGE__ *erl;
  UNUSED(codelen);
  if(flags & APREF_TRY_PIC)
  {
       unsigned long off_in_got = bmReadDWordEx(ulShift, SEEKF_START);
       unsigned long dynptr, dyn_ent, got_off;
       unsigned nitems;
/** @todo: If "program header" will be lost and will be present "section
    header" only then we should handle such situation propertly too */
       dynptr = findPHEntry(PT_DYNAMIC,&nitems);
       if(dynptr)
       {
         dyn_ent = findPHDynEntry(DT_PLTGOT,dynptr,nitems);
         if(dyn_ent)
         {
           got_off = elfVA2PA(dyn_ent);
           return AppendELFRef(str, got_off + off_in_got, flags & ~APREF_TRY_PIC, codelen, r_sh);
         }
       }
       return RAPREF_NONE;
  }
  if(!PubNames) elf_ReadPubNameList(bmbioHandle(),MemOutBox);
  if((erl = __found_ElfRel(ulShift)) != NULL)
  {
    ret = BuildReferStrElf(str,erl,flags);
  }
  if(!ret)
  {
    if((erl = __found_ElfRel(elfVA2PA(bmReadDWordEx(ulShift,SEEKF_START)))) != NULL)
    {
      ret = BuildReferStrElf(str,erl,flags);
    }
  }
  if(!ret)
  {
    memset(buff,-1,sizeof(buff));
    if(flags & APREF_TRY_LABEL)
    {
       if(FindPubName(buff,sizeof(buff),r_sh))
       {
         if(strlen(buff))
         {
           strcat(str,buff);
           if(!DumpMode && !EditMode) GidAddGoAddress(str,r_sh);
           ret = RAPREF_DONE;
         }
       }
    }
  }
  return flags & APREF_TRY_LABEL ? ret ? RAPREF_DONE : RAPREF_NONE : ret;
}

static tBool __FASTCALL__ IsELF32( void )
{
  char id[4];
  bmReadBufferEx(id,sizeof(id),0,SEEKF_START);
  return IS_ELF(id);
//  [0] == EI_MAG0 && id[1] == EI_MAG1 && id[2] == 'L' && id[3] == 'F';
}

static void __FASTCALL__ __elfReadSegments(linearArray **to, tBool is_virt )
{
 Elf386_External_Phdr phdr;
 Elf386_External_Shdr shdr;
 struct tag_elfVAMap vamap;
 unsigned long fp;
 unsigned va_map_count;
 tBool test;
 size_t i;
   /* We'll try to build section headers first
      since they is used in calculations of objects.
      For translation of virtual address to physical and vise versa
      more preferred to use program headers. But they often are not presented
      and often is unordered by file offsets. */
   if(IsSectionsPresent) /* Section headers are present */
   {
     va_map_count = ELF_HALF(elf.e_shnum);
     if(!(*to = la_Build(0,sizeof(struct tag_elfVAMap),MemOutBox)))
     {
       exit(EXIT_FAILURE);
     }
     bmSeek(ELF_WORD(elf.e_shoff),SEEKF_START);
     for(i = 0;i < va_map_count;i++)
     {
       unsigned long flg,x_flags;
       fp = bmGetCurrFilePos();
       bmReadBuffer(&shdr,sizeof(Elf386_External_Shdr));
       bmSeek(fp+ELF_HALF(elf.e_shentsize),SEEKF_START);
       vamap.va = ELF_WORD(shdr.sh_addr);
       vamap.size = ELF_WORD(shdr.sh_size);
       vamap.foff = ELF_WORD(shdr.sh_offset);
       vamap.nameoff = ELF_WORD(shdr.sh_name);
       flg = ELF_WORD(shdr.sh_flags);
       x_flags = 0;
       /* I think - it would be better to use for computation of virtual and
          physical addresses maps only that sections which occupy memory
          during execution. All other are rubbish for analyze */
       if((flg & SHF_ALLOC) == SHF_ALLOC)
       {
         x_flags |= PF_R; /* Ugle: means flags is not empty */
         if(flg & SHF_WRITE)     x_flags |= PF_W;
         if(flg & SHF_EXECINSTR) x_flags |= PF_X;
         vamap.flags = x_flags;
         test = is_virt ? elfVA2PA(vamap.va) != 0 : elfPA2VA(vamap.foff) != 0;
         if(!test)
         {
           if(!la_AddData(*to,&vamap,MemOutBox)) exit(EXIT_FAILURE);
           /** We must sort va_map after adding of each element because ELF section
               header has unsorted and nested elements */
           la_Sort(*to,is_virt ? vamap_comp_virt : vamap_comp_phys);
         }
       }
     }
   }
   else /* Try to build program headers map */
    if((va_map_count = ELF_HALF(elf.e_phnum)) != 0) /* Program headers are present */
    {
      if(!(*to = la_Build(va_map_count,sizeof(struct tag_elfVAMap),MemOutBox)))
      {
        exit(EXIT_FAILURE);
      }
      bmSeek(ELF_WORD(elf.e_phoff),SEEKF_START);
      for(i = 0;i < va_map_count;i++)
      {
        fp = bmGetCurrFilePos();
        bmReadBuffer(&phdr,sizeof(Elf386_External_Phdr));
        bmSeek(fp+ELF_HALF(elf.e_phentsize),SEEKF_START);
        vamap.va = ELF_WORD(phdr.p_vaddr);
        vamap.size = max(ELF_WORD(phdr.p_filesz), ELF_WORD(phdr.p_memsz));
        vamap.foff = ELF_WORD(phdr.p_offset);
        vamap.nameoff = ELF_WORD(phdr.p_type) & 0x000000FFUL ? ~ELF_WORD(phdr.p_type) : 0xFFFFFFFFUL;
        vamap.flags = ELF_WORD(phdr.p_flags);
        test = is_virt ? elfVA2PA(vamap.va) != 0 : elfPA2VA(vamap.foff) != 0;
        if(!test)
        {
          if(!la_AddData(*to,&vamap,MemOutBox))
          {
            exit(EXIT_FAILURE);
          }
          /** We must sort va_map after adding of each element because ELF program
              header has unsorted and has nested elements */
          la_Sort(*to,is_virt ? vamap_comp_virt : vamap_comp_phys);
        }
      }
    }
}

static void __FASTCALL__ ELFinit( void )
{
 BGLOBAL main_handle;
 unsigned long fs;
 size_t i;
   PMRegLowMemCallBack(elfLowMemFunc);
   bmReadBufferEx(&elf,sizeof(Elf386_External_Ehdr),0,SEEKF_START);
   is_big = elf.e_ident[EI_DATA] == ELFDATA2MSB;
#ifdef	ELF64
   is_64bit = elf.e_ident[EI_CLASS] == ELFCLASS64;
#endif
   fs = bmGetFLength();
   IsSectionsPresent = ELF_HALF(elf.e_shnum) != 0 &&
                       ELF_WORD(elf.e_shoff) &&
                       ELF_WORD(elf.e_shoff) < fs && 
                       ELF_WORD(elf.e_shoff) +
                       ELF_HALF(elf.e_shnum)*ELF_HALF(elf.e_shentsize) <= fs;
   __elfReadSegments(&va_map_virt,True);
   __elfReadSegments(&va_map_phys,False);
   /** Find min value of virtual address */
   for(i = 0; i < va_map_virt->nItems;i++)
   {
     struct tag_elfVAMap __HUGE__ *evm;
     evm = &((struct tag_elfVAMap __HUGE__ *)va_map_virt->data)[i];
     if(evm->va < elf_min_va) elf_min_va = evm->va;
   }
   main_handle = bmbioHandle();
   if((namecache = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) namecache = main_handle;
   if((namecache2 = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) namecache2 = main_handle;
   if((elfcache = bioDupEx(main_handle,BBIO_SMALL_CACHE_SIZE)) == &bNull) elfcache = main_handle;
   /** Computing symbol table entry */
   __elfSymPtr = findSHEntry(bmbioHandle(), SHT_SYMTAB, &__elfNumSymTab, &__elfSymShTbl, &__elfSymEntSize);
}

static void __FASTCALL__ ELFdestroy( void )
{
   BGLOBAL main_handle;
   main_handle = bmbioHandle();
   if(namecache != &bNull && namecache != main_handle) bioClose(namecache);
   if(namecache2 != &bNull && namecache2 != main_handle) bioClose(namecache2);
   if(elfcache != &bNull && elfcache != main_handle) bioClose(elfcache);
   if(PubNames) { la_Destroy(PubNames); PubNames = 0; }
   if(CurrElfChain) { la_Destroy(CurrElfChain); CurrElfChain = 0; }
   PMUnregLowMemCallBack(elfLowMemFunc);
   la_Destroy(va_map_virt);
   la_Destroy(va_map_phys);
}

static int __FASTCALL__ ELFbitness(unsigned long off)
{
  UNUSED(off);
  return DAB_USE32;
}

static unsigned long __FASTCALL__ ELFHelp( void )
{
  hlpDisplay(10003);
  return BMGetCurrFilePos();
}

static tBool __FASTCALL__ ELFAddrResolv(char *addr,unsigned long cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  tBool bret = True;
  tUInt32 res;
  if(cfpos < sizeof(Elf386_External_Ehdr))
  {
    strcpy(addr,"ELFhdr:");
    strcpy(&addr[7],Get2Digit(cfpos));
  }
  else
    if((res=elfPA2VA(cfpos))!=0)
    {
      addr[0] = '.';
      strcpy(&addr[1],Get8Digit(res));
    }
    else bret = False;
  return bret;
}

static tCompare __FASTCALL__ compare_pubnames(const void __HUGE__ *v1,const void __HUGE__ *v2)
{
  const struct PubName __HUGE__ *pnam1,__HUGE__ *pnam2;
  pnam1 = (const struct PubName __HUGE__ *)v1;
  pnam2 = (const struct PubName __HUGE__ *)v2;
  return __CmpLong__(pnam1->pa,pnam2->pa);
}

static tBool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,unsigned long pa)
{
  struct PubName *ret,key;
  key.pa = pa;
  ret = la_Find(PubNames,&key,compare_pubnames);
  if(ret)
  {
    active_shtbl = ret->addinfo;
    elf386_readnametableex(ret->nameoff,buff,cb_buff);
    buff[cb_buff-1] = 0;
    return True;
  }
  return False;
}

static void __FASTCALL__ elf_ReadPubNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *))
{
  unsigned long fpos,fp,tableptr, number, pubname_shtbl;
  unsigned ent_size;
  struct PubName epn;
  BGLOBAL b_cache;
  unsigned i, nitems;
  b_cache = handle;
  if(!(PubNames = la_Build(0,sizeof(struct PubName),mem_out))) return;
  fpos = bioTell(b_cache);
  tableptr = findSHEntry(b_cache, SHT_DYNSYM, &number, &pubname_shtbl, &ent_size);
  if(!tableptr)
  {
    tableptr = findPHPubSyms(&nitems, &ent_size, &pubname_shtbl);
    number = nitems;
  }
  if(!(PubNames = la_Build(0,sizeof(struct PubName),mem_out))) return;
  if(tableptr)
  {
    bioSeek(b_cache,tableptr,SEEK_SET);
    for(i = 0;i < number;i++)
    {
     Elf386_External_Dyn pdyn;
     fp = bioTell(b_cache);
     bioReadBuffer(b_cache,&pdyn,sizeof(Elf386_External_Dyn));
     if(bioEOF(b_cache)) break;
     bioSeek(b_cache,fp+ent_size,SEEKF_START);
     epn.nameoff = ELF_WORD(pdyn.d_tag);
     epn.pa = elfVA2PA(ELF_WORD(pdyn.d_un.d_val));
     epn.addinfo = pubname_shtbl;
     epn.attr = ELF_ST_INFO(STB_GLOBAL,STT_NOTYPE);
     if(!la_AddData(PubNames,&epn,mem_out)) break;
    }
  }
  /** If present symbolic information we must read it */

  if(__elfNumSymTab)
  {
    bioSeek(handle,__elfSymPtr,SEEK_SET);
    for(i = 0;i < __elfNumSymTab;i++)
    {
      Elf386_External_Sym sym;
      fp = bioTell(handle);
      bioReadBuffer(handle,&sym,sizeof(Elf386_External_Sym));
      if(bioEOF(handle) || IsKbdTerminate()) break;
      bioSeek(handle,fp+__elfSymEntSize,SEEKF_START);
      if(ELF_IS_SECTION_PHYSICAL(ELF_HALF(sym.st_shndx)) &&
         ELF_ST_TYPE(sym.st_info[0]) != STT_SECTION)
      {
        epn.pa = __calcSymEntry(handle,i,False);
        epn.nameoff = ELF_WORD(sym.st_name);
        epn.addinfo = __elfSymShTbl;
        epn.attr = sym.st_info[0];
        if(!la_AddData(PubNames,&epn,MemOutBox)) break;
      }
    }
  }
  la_Sort(PubNames,compare_pubnames);
  bioSeek(b_cache,fpos,SEEK_SET);
}

static void __FASTCALL__ elf_ReadPubName(BGLOBAL b_cache,const struct PubName *it,
                            char *buff,unsigned cb_buff)
{
   UNUSED(b_cache);
   active_shtbl = it->addinfo;
   elf386_readnametableex(it->nameoff,buff,cb_buff);
}

static unsigned long __FASTCALL__ elfGetPubSym(char *str,unsigned cb_str,unsigned *func_class,
                           unsigned long pa,tBool as_prev)
{
   return fmtGetPubSym(elfcache,str,cb_str,func_class,pa,as_prev,
                       elf_ReadPubNameList,
                       elf_ReadPubName);
}

static unsigned __FASTCALL__ elfGetObjAttr(unsigned long pa,char *name,unsigned cb_name,
                       unsigned long *start,unsigned long *end,int *_class,int *bitness)
{
  unsigned i,ret;
  struct tag_elfVAMap *evam;
  *start = 0;
  *end = bmGetFLength();
  *_class = OC_NOOBJECT;
  *bitness = ELFbitness(pa);
  name[0] = 0;
  ret = 0;
  evam = va_map_phys->data;
  for(i = 0;i < va_map_phys->nItems;i++)
  {
    if(!(evam[i].foff && evam[i].size)) continue;
    if(pa >= *start && pa < evam[i].foff)
    {
      /** means between two objects */
      *end = evam[i].foff;
      ret = 0;
      break;
    }
    if(pa >= evam[i].foff &&
       pa < evam[i].foff + evam[i].size)
    {
      *start = evam[i].foff;
      *end = *start + evam[i].size;
      if(evam[i].flags)
      {
        if(evam[i].flags & PF_X) *_class = OC_CODE;
        else                     *_class = OC_DATA;
      }
      else  *_class = OC_NOOBJECT;
      elf386_readnametable(evam[i].nameoff,name,cb_name);
      ret = i+1;
      break;
    }
    *start = evam[i].foff + evam[i].size;
  }
  return ret;
}

static int __FASTCALL__ ELFplatform( void ) { return DISASM_CPU_IX86; }

REGISTRY_BIN elf386Table =
{
  "ELF (Executable and Linking Format)",
  { "ELFhlp", "DynInf", "DynSec", NULL, NULL, NULL, "SymTab", NULL, "SecHdr", "PrgDef" },
  { ELFHelp, ShowELFDynInfo, ShowELFDynSec, NULL, NULL, NULL, ShowELFSymTab, NULL, ShowSecHdrElf, ShowPrgHdrElf },
  IsELF32, ELFinit, ELFdestroy,
  ShowELFHeader,
  AppendELFRef,
  fmtSetState,
  ELFplatform,
  ELFbitness,
  ELFAddrResolv,
  elfVA2PA,
  elfPA2VA,
  elfGetPubSym,
  elfGetObjAttr,
  NULL,
  NULL
};
