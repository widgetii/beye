/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/elf386.c
 * @brief       This file contains implementation of ELF-32 file format decoder.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
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

static char is_msbf; /* is most significand byte first */
static char is_64bit;

typedef union
{
  Elf386_External_Ehdr e32;
  Elf64_External_Ehdr  e64;
}ElfXX_External_Ehdr;

static ElfXX_External_Ehdr elf;
#define ELF_EHDR(e,FIELD) (is_64bit?(((Elf64_External_Ehdr *)&e.e64)->FIELD):(((Elf386_External_Ehdr *)&e.e32)->FIELD))
#define ELF_EHDR_SIZE() (is_64bit?sizeof(Elf64_External_Ehdr):sizeof(Elf386_External_Ehdr))

typedef union
{
  Elf386_External_Dyn e32;
  Elf64_External_Dyn  e64;
}ElfXX_External_Dyn;
#define ELF_EDYN(e,FIELD) (is_64bit?(((Elf64_External_Dyn *)&e.e64)->FIELD):(((Elf386_External_Dyn *)&e.e32)->FIELD))
#define ELF_EDYN_SIZE() (is_64bit?sizeof(Elf64_External_Dyn):sizeof(Elf386_External_Dyn))

typedef union
{
  Elf386_External_Phdr e32;
  Elf64_External_Phdr  e64;
}ElfXX_External_Phdr;
#define ELF_PHDR(e,FIELD) (is_64bit?(((Elf64_External_Phdr *)&e.e64)->FIELD):(((Elf386_External_Phdr *)&e.e32)->FIELD))
#define ELF_PHDR_SIZE() (is_64bit?sizeof(Elf64_External_Phdr):sizeof(Elf386_External_Phdr))

typedef union
{
  Elf386_External_Shdr e32;
  Elf64_External_Shdr  e64;
}ElfXX_External_Shdr;
#define ELF_SHDR(e,FIELD) (is_64bit?(((Elf64_External_Shdr *)&e.e64)->FIELD):(((Elf386_External_Shdr *)&e.e32)->FIELD))
#define ELF_SHDR_SIZE() (is_64bit?sizeof(Elf64_External_Shdr):sizeof(Elf386_External_Shdr))

typedef union
{
  Elf386_External_Sym e32;
  Elf64_External_Sym  e64;
}ElfXX_External_Sym;
#define ELF_SYM(e,FIELD) (is_64bit?(((Elf64_External_Sym *)&e.e64)->FIELD):(((Elf386_External_Sym *)&e.e32)->FIELD))
#define ELF_SYM_SIZE() (is_64bit?sizeof(Elf64_External_Sym):sizeof(Elf386_External_Sym))

typedef union
{
  Elf386_External_Rel e32;
  Elf64_External_Rel  e64;
}ElfXX_External_Rel;
#define ELF_REL(e,FIELD) (is_64bit?(((Elf64_External_Rel *)&e.e64)->FIELD):(((Elf386_External_Rel *)&e.e32)->FIELD))
#define ELF_REL_SIZE() (is_64bit?sizeof(Elf64_External_Rel):sizeof(Elf386_External_Rel))

typedef union
{
  Elf386_External_Rela e32;
  Elf64_External_Rela  e64;
}ElfXX_External_Rela;
#define ELF_RELA(e,FIELD) (is_64bit?(((Elf64_External_Rela *)&e.e64)->FIELD):(((Elf386_External_Rela *)&e.e32)->FIELD))
#define ELF_RELA_SIZE() (is_64bit?sizeof(Elf64_External_Rela):sizeof(Elf386_External_Rela))

static __filesize_t active_shtbl = 0;
static __filesize_t elf_min_va = FILESIZE_MAX;
static unsigned long __elfNumSymTab = 0;
static __filesize_t  __elfSymShTbl = 0;
static unsigned long __elfSymEntSize = 0;
static __filesize_t  __elfSymPtr = 0L;

struct tag_elfVAMap
{
  __filesize_t va;
  __filesize_t size;
  __filesize_t foff;
  __filesize_t nameoff;
  __filesize_t flags;
};

#define ELF_HALF(cval) FMT_WORD(cval,is_msbf)
#define ELF_WORD(cval) FMT_DWORD(cval,is_msbf)
#define ELF_XWORD(cval) (is_64bit?FMT_QWORD(cval,is_msbf):FMT_DWORD(cval,is_msbf))
#define ELF_ADDR(cval) ELF_XWORD(cval)
#define ELF_OFF(cval) ELF_XWORD(cval)

static tBool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa);
static void __FASTCALL__ elf_ReadPubNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *));
static __filesize_t __FASTCALL__ elfVA2PA(__filesize_t va);
static __filesize_t __FASTCALL__ elfPA2VA(__filesize_t pa);
static tBool IsSectionsPresent;

static __filesize_t __NEAR__ __FASTCALL__ findPHEntry(unsigned long type,unsigned *nitems)
{
  __filesize_t fpos,dynptr;
  ElfXX_External_Phdr phdr;
  size_t i,limit;
  fpos = bmGetCurrFilePos();
  dynptr = 0;
  *nitems = 0;
  limit = ELF_HALF(ELF_EHDR(elf,e_phnum));
  for(i = 0;i < limit;i++)
  {
   bmReadBufferEx(&phdr,sizeof(phdr),ELF_OFF(ELF_EHDR(elf,e_phoff)) + i*ELF_HALF(ELF_EHDR(elf,e_phentsize)),SEEKF_START);
   if(bmEOF()) break;
   if(ELF_WORD(ELF_PHDR(phdr,p_type)) == type)
   {
     dynptr = ELF_OFF(ELF_PHDR(phdr,p_offset));
     *nitems = ELF_OFF(ELF_PHDR(phdr,p_filesz))/ELF_EDYN_SIZE();
     break;
   }
  }
  bmSeek(fpos,SEEKF_START);
  return dynptr;
}

static __filesize_t __NEAR__ __FASTCALL__ findPHDynEntry(unsigned long type,
							__filesize_t dynptr,
							unsigned long nitems)
{
  unsigned i;
  __filesize_t fpos;
  tBool is_found = False;
  ElfXX_External_Dyn dyntab;
  fpos = bmGetCurrFilePos();
  bmSeek(dynptr,SEEKF_START);
  for(i = 0;i < nitems;i++)
  {
    bmReadBufferEx(&dyntab,ELF_EDYN_SIZE(),dynptr,SEEKF_START);
    if(bmEOF()) break;
    dynptr += ELF_EDYN_SIZE();
    if(ELF_XWORD(ELF_EDYN(dyntab,d_tag)) == type) { is_found = True; break; }
  }    
  bmSeek(fpos,SEEKF_START);
  return is_found ? ELF_XWORD(ELF_EDYN(dyntab,d_un.d_ptr)) : 0L;
}

static __filesize_t __NEAR__ __FASTCALL__ findPHPubSyms(unsigned long *number,
							unsigned long *ent_size,
							__filesize_t *act_shtbl)
{
  __filesize_t fpos, dynptr, dyn_ptr;
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
          __filesize_t _fpos,dptr,max_val,cur_ptr;
          ElfXX_External_Dyn dyntab;
          _fpos = bmGetCurrFilePos();
	  dptr = dyn_ptr;
          bmSeek(dptr,SEEKF_START);
	  max_val = bmGetFLength(); /* if section is last */
          for(i = 0;i < nitems;i++)
          {
            bmReadBufferEx(&dyntab,sizeof(dyntab),dptr,SEEKF_START);
            if(bmEOF()) break;
            dptr += ELF_EDYN_SIZE();
	    cur_ptr = elfVA2PA(ELF_XWORD(ELF_EDYN(dyntab,d_un.d_ptr)));
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

static __filesize_t __NEAR__ __FASTCALL__
                     findSHEntry(BGLOBAL b_cache, unsigned long type,
                                 unsigned long *nitems,__filesize_t *link,
                                 unsigned long *ent_size)
{
  ElfXX_External_Shdr shdr;
  __filesize_t fpos, tableptr;
  size_t i, limit;
  fpos = bioTell(b_cache);
  tableptr = 0L;
  limit = ELF_HALF(ELF_EHDR(elf,e_shnum));
  for(i = 0;i < limit;i++)
  {
   bioSeek(b_cache,ELF_OFF(ELF_EHDR(elf,e_shoff)) + i*ELF_HALF(ELF_EHDR(elf,e_shentsize)),SEEKF_START);
   bioReadBuffer(b_cache,&shdr,sizeof(shdr));
   if(bioEOF(b_cache)) break;
   if(ELF_WORD(ELF_SHDR(shdr,sh_type)) == type)
   {
     tableptr = ELF_OFF(ELF_SHDR(shdr,sh_offset));
     *nitems = ELF_XWORD(ELF_SHDR(shdr,sh_size))/ELF_XWORD(ELF_SHDR(shdr,sh_entsize));
     *link = ELF_WORD(ELF_SHDR(shdr,sh_link));
     *ent_size = ELF_XWORD(ELF_SHDR(shdr,sh_entsize));
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
  return pnam1->va<pnam2->va?-1:pnam1->va>pnam2->va?1:0;
}

static tCompare __FASTCALL__ vamap_comp_phys(const void __HUGE__ *v1,const void __HUGE__ *v2)
{
  const struct tag_elfVAMap __HUGE__ *pnam1,__HUGE__ *pnam2;
  pnam1 = (const struct tag_elfVAMap __HUGE__ *)v1;
  pnam2 = (const struct tag_elfVAMap __HUGE__ *)v2;
  return pnam1->foff<pnam2->foff?-1:pnam1->foff>pnam2->foff?1:0;
}

static __filesize_t __FASTCALL__ elfVA2PA(__filesize_t va)
{
  unsigned long i;
  if(va_map_virt)
  for(i = 0; i < va_map_virt->nItems;i++)
  {
    struct tag_elfVAMap __HUGE__ *evm;
    evm = &((struct tag_elfVAMap __HUGE__ *)va_map_virt->data)[i];
    if(va >= evm->va && va < evm->va + evm->size) return evm->foff + (va - evm->va);
  }
  return 0L;
}

static __filesize_t __FASTCALL__ elfPA2VA(__filesize_t pa)
{
  unsigned long i;
  __filesize_t ret;
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

static void __NEAR__ __FASTCALL__ elf386_readnametable(__filesize_t off,char *buf,unsigned blen)
{
  __filesize_t foff;
  ElfXX_External_Shdr sh;
  unsigned char ch;
  unsigned freq;

  BGLOBAL b_cache,b_cache2;
  b_cache = namecache;
  b_cache2 = namecache2;
  foff = ELF_OFF(ELF_EHDR(elf,e_shoff))+ELF_HALF(ELF_EHDR(elf,e_shstrndx))*ELF_HALF(ELF_EHDR(elf, e_shentsize));
  bioSeek(b_cache2,foff,SEEKF_START);
  bioReadBuffer(b_cache2,&sh,sizeof(sh));
  foff = ELF_OFF(ELF_SHDR(sh,sh_offset)) + off;
  freq = 0;
  while(1)
  {
     bioSeek(b_cache,foff++,SEEKF_START);
     ch = bioReadByte(b_cache);
     buf[freq++] = ch;
     if(!ch || freq >= blen || bioEOF(b_cache)) break;
  }
}

static void __NEAR__ __FASTCALL__ elf386_readnametableex(__filesize_t off,char *buf,unsigned blen)
{
  __filesize_t foff;
  ElfXX_External_Shdr sh;
  unsigned char ch;
  unsigned freq;
  BGLOBAL b_cache,b_cache2;
  b_cache = namecache;
  b_cache2 = namecache2;
  if(ELF_OFF(ELF_EHDR(elf,e_shoff)))
  {
    foff = ELF_OFF(ELF_EHDR(elf,e_shoff))+active_shtbl*ELF_HALF(ELF_EHDR(elf, e_shentsize));
    bioSeek(b_cache2,foff,SEEKF_START);
    bioReadBuffer(b_cache2,&sh,sizeof(sh));
    foff = ELF_OFF(ELF_SHDR(sh,sh_offset)) + off;
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

static const char *__NEAR__ __FASTCALL__ elf_machine(unsigned id,unsigned *disasm)
{
  *disasm=DISASM_DATA;
  switch(id)
  {
    case EM_NONE:	return "None";
    case EM_M32:	return "AT&T WE32100";
    case EM_SPARC:	*disasm = DISASM_CPU_SPARC; return "Sun SPARC";
    case EM_386:	*disasm = DISASM_CPU_IX86; return "Intel 386";
    case EM_68K:	*disasm = DISASM_CPU_PPC; return "Motorola m68k";
    case EM_88K:	*disasm = DISASM_CPU_PPC; return "Motorola m88k";
    case EM_860:	return "Intel 80860";
    case EM_MIPS:	*disasm = DISASM_CPU_MIPS; return "MIPS I";
    case EM_S370:	return "IBM System/370";
    case EM_MIPS_RS3_LE:*disasm = DISASM_CPU_MIPS; return "MIPS R3000";
    case EM_PARISC:	return "HP PA-RISC";
    case EM_SPARC32PLUS:*disasm = DISASM_CPU_SPARC; return "SPARC v8plus";
    case EM_960:	return "Intel 80960";
    case EM_PPC:	*disasm = DISASM_CPU_PPC; return "Power PC 32-bit";
    case EM_PPC64:	*disasm = DISASM_CPU_PPC; return "Power PC 64-bit";
    case EM_S390:	return "IBM System/390";
    case EM_ADSP:	return "Atmel ADSP";
    case EM_V800:	return "NEC V800";
    case EM_FR20:	return "Fujitsu FR20";
    case EM_RH32:	return "TRW RH-32";
    case EM_RCE:	return "Motorola RCE";
    case EM_ARM:	*disasm=DISASM_CPU_ARM; return "ARM";
    case EM_ALPHA:	*disasm = DISASM_CPU_ALPHA; return "DEC Alpha";
    case EM_SH:		*disasm = DISASM_CPU_SH; return "Hitachi SH";
    case EM_SPARCV9:	*disasm = DISASM_CPU_SPARC; return "SPARC v9 64-bit";
    case EM_TRICORE:	return "Siemens TriCore embedded processor";
    case EM_ARC:	return "Argonaut RISC Core";
    case EM_H8_300:	return "Hitachi H8/300";
    case EM_H8_300H:	return "Hitachi H8/300H";
    case EM_H8S:	return "Hitachi H8S";
    case EM_H8_500:	return "Hitachi H8/500";
    case EM_IA_64:	*disasm = DISASM_CPU_IA64; return "Intel IA-64";
    case EM_MIPS_X:	*disasm = DISASM_CPU_MIPS; return "Stanford MIPS-X";
    case EM_COLDFIRE:	return "Motorola ColdFire";
    case EM_68HC12:	return "Motorola M68HC12";
    case EM_MMA:	return "Fujitsu MMA Multimedia Accelerator";
    case EM_PCP:	return "Siemens PCP";
    case EM_NCPU:	return "Sony nCPU embedded RISC processor";
    case EM_NDR1:	return "Denso NDR1 microprocessor";
    case EM_STARCORE:	return "Motorola StarCore processor";
    case EM_ME16:	return "Toyota ME16 processor";
    case EM_ST100:	return "STMicroelectronics ST100 processor";
    case EM_TINYJ:	return "Advanced Logic Corp. TinyJ";
    case EM_X86_64:	*disasm = DISASM_CPU_IX86; return "AMD x86-64";
    case EM_PDSP:	return "Sony DSP Processor";
    case EM_PDP10:	return "DEC PDP-10";
    case EM_PDP11:	return "DEC PDP-11";
    case EM_FX66:	return "Siemens FX66 microcontroller";
    case EM_ST9PLUS:	return "STMicroelectronics ST9+ 8/16 bit microcontroller";
    case EM_ST7:	return "STMicroelectronics ST7 8-bit microcontroller";
    case EM_68HC16:	return "Motorola MC68HC16 Microcontroller";
    case EM_68HC11:	return "Motorola MC68HC11 Microcontroller";
    case EM_68HC08:	return "Motorola MC68HC08 Microcontroller";
    case EM_68HC05:	return "Motorola MC68HC05 Microcontroller";
    case EM_SVX:	return "Silicon Graphics SVx";
    case EM_ST19:	return "STMicroelectronics ST19 8-bit microcontroller";
    case EM_VAX:	return "DEC VAX";
    case EM_CRIS:	return "Axis Comm. 32-bit embedded processor";
    case EM_JAVELIN:	return "Infineon Tech. 32-bit embedded processor";
    case EM_FIREPATH:	return "Element 14 64-bit DSP Processor";
    case EM_ZSP:	return "LSI Logic 16-bit DSP Processor";
    case EM_MMIX:	return "Donald Knuth's educational 64-bit processor";
    case EM_HUANY:	return "Harvard University machine-independent object files";
    case EM_PRISM:	return "SiTera Prism";
    case EM_AVR:	*disasm=DISASM_CPU_AVR; return "Atmel AVR 8-bit";
    case EM_FR30:	return "Fujitsu FR30";
    case EM_D10V:	return "Mitsubishi D10V";
    case EM_D30V:	return "Mitsubishi D30V";
    case EM_V850:	return "NEC v850";
    case EM_M32R:	return "Mitsubishi M32R";
    case EM_MN10300:	return "Matsushita MN10300";
    case EM_MN10200:	return "Matsushita MN10200";
    case EM_PJ:		return "picoJava";
    case EM_OPENRISC:	return "OpenRISC 32-bit embedded processor";
    case EM_ARC_A5:	return "ARC Cores Tangent-A5";
    case EM_XTENSA:	return "Tensilica Xtensa Architecture";
    case EM_VIDEOCORE:	return "Alphamosaic VideoCore processor";
    case EM_TMM_GPP:	return "Thompson Multimedia General Purpose Processor";
    case EM_NS32K:	return "National Semiconductor 32000 series";
    case EM_TPC:	return "Tenor Network TPC processor";
    case EM_SNP1K:	return "Trebia SNP 1000 processor";
    case EM_IP2K:	return "Ubicom IP2022 micro controller";
    case EM_CR:		return "National Semiconductor CompactRISC";
    case EM_MSP430:	return "TI msp430 micro controller";
    case EM_BLACKFIN:	return "ADI Blackfin";
    case EM_ALTERA_NIOS2: return "Altera Nios II soft-core processor";
    case EM_CRX:	return "National Semiconductor CRX";
    case EM_XGATE:	return "Motorola XGATE embedded processor";
    case EM_C166:	return "Infineon C16x/XC16x processor";
    case EM_M16C:	return "Renesas M16C series microprocessors";
    case EM_DSPIC30F:	return "Microchip Technology dsPIC30F Digital Signal Controller";
    case EM_CE:		return "Freescale Communication Engine RISC core";
    case EM_M32C:	return "Renesas M32C series microprocessors";
    case EM_TSK3000:	return "Altium TSK3000 core";
    case EM_RS08:	return "Freescale RS08 embedded processor";
    case EM_ECOG2:	return "Cyan Technology eCOG2 microprocessor";
    case EM_SCORE:	return "Sunplus Score";
    case EM_DSP24:	return "New Japan Radio (NJR) 24-bit DSP Processor";
    case EM_VIDEOCORE3:	return "Broadcom VideoCore III processor";
    case EM_LATTICEMICO32: return "RISC processor for Lattice FPGA architecture";
    case EM_SE_C17:	return "Seiko Epson C17 family";
    case EM_MMDSP_PLUS:	return "STMicroelectronics 64bit VLIW Data Signal Processor";
    case EM_CYPRESS_M8C:return "Cypress M8C microprocessor";
    case EM_R32C:	return "Renesas R32C series microprocessors";
    case EM_TRIMEDIA:	return "NXP Semiconductors TriMedia architecture family";
    case EM_QDSP6:	return "QUALCOMM DSP6 Processor";
    case EM_8051:	return "Intel 8051 and variants";
    case EM_STXP7X:	return "STMicroelectronics STxP7x family";
    case EM_NDS32:	return "Andes Technology compact code size embedded RISC processor family";
    case EM_ECOG1X:	return "Cyan Technology eCOG1X family";
    case EM_MAXQ30:	return "Dallas Semiconductor MAXQ30 Core Micro-controllers";
    case EM_XIMO16:	return "New Japan Radio (NJR) 16-bit DSP Processor";
    case EM_MANIK:	return "M2000 Reconfigurable RISC Microprocessor";
    case EM_CRAYNV2:	*disasm = DISASM_CPU_CRAY; return "Cray Inc. NV2 vector architecture";
    case EM_RX:		return "Renesas RX family";
    case EM_METAG:	return "Imagination Technologies META processor architecture";
    case EM_MCST_ELBRUS:return "MCST Elbrus general purpose hardware architecture";
    case EM_ECOG16:	return "Cyan Technology eCOG16 family";
    case EM_CR16:	return "National Semiconductor CompactRISC 16-bit processor";
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


static __filesize_t __FASTCALL__ ShowELFHeader( void )
{
  __filesize_t fpos;
  TWindow *w;
  char hdr[81];
  unsigned keycode,dummy;
  __filesize_t entrya;
  fpos = BMGetCurrFilePos();
  entrya = elfVA2PA(ELF_ADDR(ELF_EHDR(elf,e_entry)));
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
           "Object file version               = %08lXH (%s)\n"
	    ,ELF_EHDR(elf,e_ident[EI_MAG0]),	ELF_EHDR(elf, e_ident[EI_MAG1])
	    ,ELF_EHDR(elf,e_ident[EI_MAG2]),	ELF_EHDR(elf,e_ident[EI_MAG3])
	    ,ELF_EHDR(elf,e_ident[EI_MAG0]),	ELF_EHDR(elf,e_ident[EI_MAG1])
	    ,ELF_EHDR(elf,e_ident[EI_MAG2]),	ELF_EHDR(elf,e_ident[EI_MAG3])
	    ,ELF_EHDR(elf,e_ident[EI_CLASS]), 	elf_class(ELF_EHDR(elf,e_ident[EI_CLASS]))
	    ,ELF_EHDR(elf,e_ident[EI_DATA]),	elf_data(ELF_EHDR(elf,e_ident[EI_DATA]))
	    ,ELF_EHDR(elf,e_ident[EI_VERSION]),	elf_version(ELF_EHDR(elf,e_ident[EI_VERSION]))
	    ,ELF_EHDR(elf,e_ident[EI_OSABI]),	elf_osabi(ELF_EHDR(elf,e_ident[EI_OSABI]))
	    ,ELF_EHDR(elf,e_ident[EI_ABIVERSION]),ELF_EHDR(elf,e_ident[EI_ABIVERSION])
	    ,ELF_HALF(ELF_EHDR(elf,e_type)),	elf_otype(ELF_HALF(ELF_EHDR(elf,e_type)))
	    ,ELF_HALF(ELF_EHDR(elf,e_machine)),	elf_machine(ELF_HALF(ELF_EHDR(elf,e_machine)),&dummy)
	    ,ELF_WORD(ELF_EHDR(elf,e_version)),	elf_version(ELF_WORD(ELF_EHDR(elf,e_version)))
  );
  twSetColorAttr(dialog_cset.entry);
  if(is_64bit)
    twPrintF("Entry point VA             = %016llXH (offset: %016llXH)"
           ,(unsigned long long)ELF_ADDR(ELF_EHDR(elf,e_entry)),(unsigned long long)entrya);
  else
    twPrintF("Entry point VA                    = %08lXH (offset: %08lXH)"
           ,(unsigned)ELF_ADDR(ELF_EHDR(elf,e_entry)),(unsigned)entrya);
  twClrEOL(); twPrintF("\n");
  twSetColorAttr(dialog_cset.main);
  if(is_64bit)
    twPrintF("Program header table offset       = %016llXH\n"
           "Section header table offset       = %016llXH\n"
           ,ELF_OFF(ELF_EHDR(elf,e_phoff))
           ,ELF_OFF(ELF_EHDR(elf,e_shoff)));
  else
    twPrintF("Program header table offset       = %08lXH\n"
           "Section header table offset       = %08lXH\n"
           ,ELF_OFF(ELF_EHDR(elf,e_phoff))
           ,ELF_OFF(ELF_EHDR(elf,e_shoff)));
  twPrintF(
           "Processor specific flag           = %08lXH\n"
           "ELF header size (bytes)           = %04XH\n"
           "Program header table entry size   = %04XH\n"
           "Program header table entry count  = %04XH\n"
           "Section header table entry size   = %04XH\n"
           "Section header table entry count  = %04XH\n"
           "Section header string table index = %04XH"
           ,ELF_WORD(ELF_EHDR(elf,e_flags))
           ,ELF_HALF(ELF_EHDR(elf,e_ehsize))
           ,ELF_HALF(ELF_EHDR(elf,e_phentsize))
           ,ELF_HALF(ELF_EHDR(elf,e_phnum))
           ,ELF_HALF(ELF_EHDR(elf,e_shentsize))
           ,ELF_HALF(ELF_EHDR(elf,e_shnum))
           ,ELF_HALF(ELF_EHDR(elf,e_shstrndx)));
  while(1)
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
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
  bioSeek(handle,ELF_OFF(ELF_EHDR(elf,e_phoff)),SEEKF_START);
  for(i = 0;i < nnames;i++)
  {
   __filesize_t fp;
   char stmp[80];
   ElfXX_External_Phdr phdr;
   if(IsKbdTerminate() || bioEOF(handle)) break;
   fp = bioTell(handle);
   bioReadBuffer(handle,&phdr,sizeof(phdr));
   bioSeek(handle,fp+ELF_HALF(ELF_EHDR(elf,e_phentsize)),SEEKF_START);
   sprintf(stmp,"%-15s %08lX %08lX %08lX %08lX %08lX %c%c%c %08lX",
                elf_encode_p_type(ELF_WORD(ELF_PHDR(phdr,p_type))),
                (unsigned long)ELF_OFF(ELF_PHDR(phdr,p_offset)),
                (unsigned long)ELF_ADDR(ELF_PHDR(phdr,p_vaddr)),
                (unsigned long)ELF_ADDR(ELF_PHDR(phdr,p_paddr)),
                (unsigned long)ELF_OFF(ELF_PHDR(phdr,p_filesz)),
                (unsigned long)ELF_OFF(ELF_PHDR(phdr,p_memsz)),
                (ELF_WORD(ELF_PHDR(phdr,p_flags)) & PF_X) == PF_X ? 'X' : ' ',
                (ELF_WORD(ELF_PHDR(phdr,p_flags)) & PF_W) == PF_W ? 'W' : ' ',
                (ELF_WORD(ELF_PHDR(phdr,p_flags)) & PF_R) == PF_R ? 'R' : ' ',
                (unsigned long)ELF_OFF(ELF_PHDR(phdr,p_align))
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
  bioSeek(handle,ELF_OFF(ELF_EHDR(elf,e_shoff)),SEEKF_START);
  for(i = 0;i < nnames;i++)
  {
   ElfXX_External_Shdr shdr;
   char tmp[17];
   __filesize_t fp;
   char stmp[80];
   if(IsKbdTerminate() || bioEOF(handle)) break;
   fp = bioTell(handle);
   bioReadBuffer(handle,&shdr,sizeof(shdr));
   elf386_readnametable(ELF_WORD(ELF_SHDR(shdr,sh_name)),tmp,sizeof(tmp));
   bioSeek(handle,fp+ELF_HALF(ELF_EHDR(elf,e_shentsize)),SEEKF_START);
   tmp[16] = 0;
   sprintf(stmp,"%-16s %-6s %c%c%c %08lX %08lX %08lX %04hX %04hX %04hX %04hX",
                tmp,
                elf_encode_sh_type(ELF_WORD(ELF_SHDR(shdr,sh_type))),
                (ELF_XWORD(ELF_SHDR(shdr,sh_flags)) & SHF_WRITE) == SHF_WRITE ? 'W' : ' ',
                (ELF_XWORD(ELF_SHDR(shdr,sh_flags)) & SHF_ALLOC) == SHF_ALLOC ? 'A' : ' ',
                (ELF_XWORD(ELF_SHDR(shdr,sh_flags)) & SHF_EXECINSTR) == SHF_EXECINSTR ? 'X' : ' ',
                (unsigned long)ELF_ADDR(ELF_SHDR(shdr,sh_addr)),
                (unsigned long)ELF_OFF(ELF_SHDR(shdr,sh_offset)),
                (unsigned long)ELF_XWORD(ELF_SHDR(shdr,sh_size)),
                (tUInt16)ELF_WORD(ELF_SHDR(shdr,sh_link)),
                (tUInt16)ELF_WORD(ELF_SHDR(shdr,sh_info)),
                (tUInt16)ELF_XWORD(ELF_SHDR(shdr,sh_addralign)),
                (tUInt16)ELF_XWORD(ELF_SHDR(shdr,sh_entsize))
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
 size_t i,tlen;
 char text[37];
  tlen=is_64bit?29:37;
  bioSeek(handle,__elfSymPtr,SEEK_SET);
  for(i = 0;i < nsym;i++)
  {
   __filesize_t fp;
   char stmp[80];
   ElfXX_External_Sym sym;
   if(IsKbdTerminate() || bioEOF(handle)) break;
   fp = bioTell(handle);
   bioReadBuffer(handle,&sym,sizeof(sym));
   bioSeek(handle,fp+__elfSymEntSize,SEEKF_START);
   elf386_readnametableex(ELF_WORD(ELF_SYM(sym,st_name)),text,tlen);
   text[tlen-1] = 0;
   if(is_64bit)
   sprintf(stmp,"%-29s %016llX %08lX %04hX %s %s %s"
               ,text
               ,(unsigned long long)ELF_XWORD(ELF_SYM(sym,st_value))
               ,(unsigned)ELF_XWORD(ELF_SYM(sym,st_size))
               ,ELF_HALF(ELF_SYM(sym,st_other))
               ,elf_SymTabType(ELF_SYM(sym,st_info[0]))
               ,elf_SymTabBind(ELF_SYM(sym,st_info[0]))
               ,elf_SymTabShNdx(ELF_HALF(ELF_SYM(sym,st_shndx)))
               );
   else
   sprintf(stmp,"%-37s %08lX %08lX %04hX %s %s %s"
               ,text
               ,(unsigned)ELF_XWORD(ELF_SYM(sym,st_value))
               ,(unsigned)ELF_XWORD(ELF_SYM(sym,st_size))
               ,ELF_HALF(ELF_SYM(sym,st_other))
               ,elf_SymTabType(ELF_SYM(sym,st_info[0]))
               ,elf_SymTabBind(ELF_SYM(sym,st_info[0]))
               ,elf_SymTabShNdx(ELF_HALF(ELF_SYM(sym,st_shndx)))
               );
   if(!ma_AddString(obj,stmp,True)) break;
  }
  return True;
}

static tBool __NEAR__ __FASTCALL__ __elfReadDynTab(BGLOBAL handle,memArray *obj, unsigned ntbl,__filesize_t entsize)
{
 size_t i;
 char sout[80];
 unsigned len,rlen,rborder;
  for(i = 0;i < ntbl;i++)
  {
   __filesize_t fp;
   char stmp[80];
   ElfXX_External_Dyn pdyn;
   fp = bioTell(handle);
   bioReadBuffer(handle,&pdyn,sizeof(pdyn));
   bioSeek(handle,fp+entsize,SEEKF_START);
   fp = bioTell(handle);
   /* Note: elf-64 specs requre ELF_XWORD here! But works ELF_WORD !!! */
   elf386_readnametableex(ELF_WORD(ELF_EDYN(pdyn,d_tag)),sout,sizeof(sout));
   len = strlen(sout);
   rborder=is_64bit?52:60;
   if(len > rborder-4) len = rborder-7;
   if(IsKbdTerminate() || bioEOF(handle)) break;
   strncpy(stmp,sout,len);
   stmp[len] = 0;
   if(len > rborder-4) strcat(stmp,"...");
   rlen = strlen(stmp);
   if(rlen < rborder) { memset(&stmp[rlen],' ',rborder-rlen); stmp[rborder] = 0; }
   if(is_64bit)
    sprintf(&stmp[strlen(stmp)]," vma=%016llXH",(unsigned long long)ELF_XWORD(ELF_EDYN(pdyn,d_un.d_val)));
   else
    sprintf(&stmp[strlen(stmp)]," vma=%08lXH",(unsigned long)ELF_XWORD(ELF_EDYN(pdyn,d_un.d_val)));
   if(!ma_AddString(obj,stmp,True)) break;
   bioSeek(handle,fp,SEEKF_START);
  }
  return True;
}

static unsigned __FASTCALL__ Elf386PrgHdrNumItems(BGLOBAL handle)
{
   UNUSED(handle);
   return ELF_HALF(ELF_EHDR(elf,e_phnum));
}

static __filesize_t __FASTCALL__ ShowPrgHdrElf(void)
{
  __filesize_t fpos;
  int ret;
  fpos = BMGetCurrFilePos();
  ret = fmtShowList(Elf386PrgHdrNumItems,__elfReadPrgHdr,
                    " type            fileoffs virtaddr physaddr filesize memsize  flg align   ",
                    LB_SELECTIVE,NULL);
  if(ret != -1)
  {
    ElfXX_External_Phdr it;
    bmSeek(ELF_OFF(ELF_EHDR(elf,e_phoff))+ELF_PHDR_SIZE()*ret,SEEKF_START);
    bmReadBuffer(&it,sizeof(it));
    fpos = ELF_OFF(ELF_PHDR(it,p_offset));
  }
  return fpos;
}

static unsigned __FASTCALL__ Elf386SecHdrNumItems(BGLOBAL handle)
{
  UNUSED(handle);
  return IsSectionsPresent ? ELF_HALF(ELF_EHDR(elf,e_shnum)) : 0;
}

static __filesize_t __FASTCALL__ ShowSecHdrElf(void)
{
  __filesize_t fpos;
  int ret;
  fpos = BMGetCurrFilePos();
  ret = fmtShowList(Elf386SecHdrNumItems,__elfReadSecHdr,
                    " name             type   flg virtaddr fileoffs   size   link info algn esiz",
                   LB_SELECTIVE,NULL);
  if(ret != -1)
  {
    ElfXX_External_Shdr it;
//    bmSeek(ELF_OFF(ELF_EHDR(elf,e_shoff))+ELF_SHDR_SIZE()*ret,SEEKF_START);
    bmSeek(ELF_OFF(ELF_EHDR(elf,e_shoff))+ELF_HALF(ELF_EHDR(elf, e_shentsize))*ret,SEEKF_START);
    bmReadBuffer(&it,sizeof(it));
    fpos = ELF_OFF(ELF_SHDR(it,sh_offset));
  }
  return fpos;
}

static __filesize_t __calcSymEntry(BGLOBAL handle,__filesize_t num,tBool display_msg)
{
   ElfXX_External_Sym it;
   ElfXX_External_Shdr sec;
   __filesize_t ffpos,fpos = 0L;
   ffpos = bioTell(handle);
   bioSeek(handle,__elfSymPtr+__elfSymEntSize*num,SEEKF_START);
   bioReadBuffer(handle,&it,sizeof(it));
//   bioSeek(handle,ELF_OFF(ELF_EHDR(elf,e_shoff))+ELF_SHDR_SIZE()*ELF_HALF(ELF_SYM(it,st_shndx)),SEEKF_START);
   bioSeek(handle,ELF_OFF(ELF_EHDR(elf,e_shoff))+ELF_HALF(ELF_EHDR(elf, e_shentsize))*ELF_HALF(ELF_SYM(it,st_shndx)),SEEKF_START);
   bioReadBuffer(handle,&sec,sizeof(sec));
   bioSeek(handle,ffpos,SEEKF_START);
   if(ELF_IS_SECTION_PHYSICAL(ELF_HALF(ELF_SYM(it,st_shndx))))
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
     fpos = ELF_HALF(ELF_EHDR(elf,e_type)) == ET_REL ?
            ELF_OFF(ELF_SHDR(sec,sh_offset)) + ELF_XWORD(ELF_SYM(it,st_value)):
            elfVA2PA(ELF_XWORD(ELF_SYM(it,st_value)));
   else
     if(display_msg) ErrMessageBox(NO_ENTRY,BAD_ENTRY);
   return fpos;
}

static __filesize_t __NEAR__ __FASTCALL__ displayELFsymtab( void )
{
  __filesize_t fpos;
  int ret;
  fpos = BMGetCurrFilePos();
  ret = fmtShowList(__elfGetNumSymTab,__elfReadSymTab,
                " Name                                  Value    Size     Oth. Type   Bind   Sec# ",
                LB_SELECTIVE,NULL);
  if(ret != -1)
  {
    __filesize_t ea;
    ea = __calcSymEntry(bmbioHandle(),ret,True);
    fpos = ea ? ea : fpos;
  }
  return fpos;
}

static __filesize_t __NEAR__ __FASTCALL__ displayELFdyntab(__filesize_t dynptr,
							unsigned long nitem,
							long entsize)
{
  __filesize_t fpos;
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
         __filesize_t addr_probe;
         addr_probe = is_64bit?strtoull(&addr[4],NULL,16):strtoul(&addr[4],NULL,16);
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

static __filesize_t __FASTCALL__ ShowELFSymTab( void )
{
  __filesize_t fpos;
  fpos = BMGetCurrFilePos();
  if(!__elfSymPtr) { NotifyBox(NOT_ENTRY," ELF symbol table "); return fpos; }
  active_shtbl = __elfSymShTbl;
  return displayELFsymtab();
}

static __filesize_t __FASTCALL__ ShowELFDynSec( void )
{
  __filesize_t fpos,dynptr;
  unsigned long number;
  unsigned long nitems,ent_size = UINT_MAX;
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
/************************ RELOCATION FOR ELF *******************************/
/***************************************************************************/
typedef struct tagElfRefChain
{
  __filesize_t  offset;
  __filesize_t  info;
  __filesize_t  addend;
  __filesize_t  sh_idx;
}Elf_Reloc;
static linearArray *CurrElfChain = NULL;

static tCompare __FASTCALL__ compare_elf_reloc(const void __HUGE__ *e1,const void __HUGE__ *e2)
{
  const Elf_Reloc __HUGE__ *p1,__HUGE__ *p2;
  p1 = (const Elf_Reloc __HUGE__ *)e1;
  p2 = (const Elf_Reloc __HUGE__ *)e2;
  return p1->offset<p2->offset?-1:p1->offset>p2->offset?1:0;
}

static __filesize_t get_f_offset(__filesize_t r_offset,__filesize_t sh_link)
{
  /*
    r_offset member gives the location at which to apply the relocation
    action. For a relocatable file, the value is the byte offset from
    the beginning of the section to the storage unit affected by the
    relocation. For an executable file or a shared object, the value is
    the virtual address of the storage unit affected by the relocation.
  */
  __filesize_t f_offset;
  BGLOBAL handle = elfcache;
  switch(ELF_HALF(ELF_EHDR(elf,e_type)))
  {
     case ET_REL:
                {
                  ElfXX_External_Shdr shdr;
                  __filesize_t fp;
                  fp = bioTell(handle);
//                  bioSeek(handle,ELF_OFF(ELF_EHDR(elf,e_shoff))+sh_link*ELF_SHDR_SIZE(),SEEKF_START);
                  bioSeek(handle,ELF_OFF(ELF_EHDR(elf,e_shoff))+sh_link*ELF_HALF(ELF_EHDR(elf, e_shentsize)),SEEKF_START);
                  bioReadBuffer(handle,&shdr,sizeof(shdr));
		  bioSeek(handle,fp,SEEKF_START);
		  f_offset = ELF_OFF(ELF_SHDR(shdr,sh_offset)) + r_offset;
                }
     default: f_offset = elfVA2PA(r_offset);
              break;
  }
  return f_offset;
}

#define bioRead30(handle2) (bioReadDWord(handle2)&0x3FFFFFFFUL)
#define bioRead25(handle2) (bioReadDWord(handle2)&0x01FFFFFFUL)
#define bioRead24(handle2) (bioReadDWord(handle2)&0x00FFFFFFUL)
#define bioRead22(handle2) (bioReadDWord(handle2)&0x003FFFFFUL)
#define bioRead19(handle2) (bioReadDWord(handle2)&0x0007FFFFUL)
#define bioRead12(handle2) (bioReadWord(handle2)&0x0FFFUL)

static void __elf_arm_read_erc(BGLOBAL handle2,Elf_Reloc *erc)
{
    switch(ELF32_R_TYPE(erc->info))
    {
      case R_ARM_THM_JUMP6:
      case R_ARM_THM_PC8:
      case R_ARM_ABS8:
      case R_ARM_THM_JUMP8:
               erc->addend = bioReadByte(handle2);
               break;
      case R_ARM_THM_JUMP11:
      case R_ARM_GOT_BREL12:
      case R_ARM_GOTOFF12:
      case R_ARM_TLS_LDO12:
      case R_ARM_TLS_LE12:
      case R_ARM_TLS_IE12GP:
      case R_ARM_ABS12:
      case R_ARM_THM_PC12:
               erc->addend = bioRead12(handle2);
               break;
      case R_ARM_ABS16:
               erc->addend = bioReadWord(handle2);
               break;
      case R_ARM_THM_JUMP19:
               erc->addend = bioRead19(handle2);
               break;
      case R_ARM_THM_XPC22:
      case R_ARM_THM_RPC22:
               erc->addend = bioRead22(handle2);
               break;
      case R_ARM_SWI24:
      case R_ARM_JUMP24:
      case R_ARM_THM_JUMP24:
      case R_ARM_RPC24:
               erc->addend = bioRead24(handle2);
               break;
      case R_ARM_XPC25:
      case R_ARM_RXPC25:
               erc->addend = bioRead25(handle2);
               break;
      default:
               erc->addend = bioReadDWord(handle2);
               break;
    }
}

static void __elf_i386_read_erc(BGLOBAL handle2,Elf_Reloc *erc)
{
    switch(ELF32_R_TYPE(erc->info))
    {
      case R_386_GNU_8:
      case R_386_GNU_PC8:
               erc->addend = bioReadByte(handle2);
               break;
      case R_386_GNU_16:
      case R_386_GNU_PC16:
               erc->addend = bioReadWord(handle2);
               break;
      default:
               erc->addend = bioReadDWord(handle2);
               break;
    }
}

static void __elf_x86_64_read_erc(BGLOBAL handle2,Elf_Reloc *erc) {
    switch(ELF32_R_TYPE(erc->info))
    {
      case R_X86_64_8:
      case R_X86_64_PC8:
               erc->addend = bioReadByte(handle2);
               break;
      case R_X86_64_16:
      case R_X86_64_PC16:
               erc->addend = bioReadWord(handle2);
               break;
      case R_X86_64_32:
      case R_X86_64_32S:
      case R_X86_64_PC32:
      case R_X86_64_GOT32:
      case R_X86_64_PLT32:
      case R_X86_64_DTPOFF32:
      case R_X86_64_GOTTPOFF:
      case R_X86_64_TPOFF32:
      case R_X86_64_GOTPC32_TLSDESC:
               erc->addend = bioReadDWord(handle2);
               break;
      default:
               erc->addend = bioReadQWord(handle2);
               break;
    }
}

static void __elf_ppc_read_erc(BGLOBAL handle2,Elf_Reloc *erc)
{
    switch(ELF32_R_TYPE(erc->info))
    {
      case R_ARM_THM_JUMP6:
      case R_ARM_THM_PC8:
      case R_ARM_ABS8:
      case R_ARM_THM_JUMP8:
               erc->addend = bioReadByte(handle2);
               break;
      case R_ARM_THM_JUMP11:
      case R_ARM_GOT_BREL12:
      case R_ARM_GOTOFF12:
      case R_ARM_TLS_LDO12:
      case R_ARM_TLS_LE12:
      case R_ARM_TLS_IE12GP:
      case R_ARM_ABS12:
      case R_ARM_THM_PC12:
               erc->addend = bioRead12(handle2);
               break;
      default:
               erc->addend = bioReadWord(handle2);
               break;
      case R_PPC_ADDR24:
      case R_PPC_REL24:
      case R_PPC_PLTREL24:
      case R_PPC_LOCAL24PC:
               erc->addend = bioRead24(handle2);
               break;
      case R_PPC_ADDR32:
      case R_PPC_GLOB_DAT:
      case R_PPC_JMP_SLOT:
      case R_PPC_RELATIVE:
      case R_PPC_UADDR32:
      case R_PPC_REL32:
      case R_PPC_PLT32:
      case R_PPC_PLTREL32:
      case R_PPC_ADDR30:
      case R_PPC_DTPMOD32:
      case R_PPC_TPREL32:
      case R_PPC_DTPREL32:
      case R_PPC_EMB_NADDR32:
      case R_PPC_RELAX32:
      case R_PPC_RELAX32PC:
      case R_PPC_RELAX32_PLT:
      case R_PPC_RELAX32PC_PLT:
      case R_PPC_GNU_VTINHERIT:
      case R_PPC_GNU_VTENTRY:
               erc->addend = bioReadDWord(handle2);
               break;
      case R_PPC64_ADDR64:
      case R_PPC64_UADDR64:
      case R_PPC64_REL64:
      case R_PPC64_PLT64:
      case R_PPC64_PLTREL64:
      case R_PPC64_TOC:
               erc->addend = bioReadQWord(handle2);
               break;
    }
}

static void __NEAR__ __FASTCALL__ __elfReadRelSection(__filesize_t offset,
							__filesize_t size,
							__filesize_t sh_link,
							__filesize_t info,
							__filesize_t entsize)
{
  BGLOBAL handle = elfcache,handle2 = namecache;
  size_t i,nitems;
  ElfXX_External_Rel relent;
  __filesize_t fp, sfp, lfp;
  if(!entsize) return;
  fp = bioTell(handle);
  bioSeek(handle,offset,SEEKF_START);
  nitems = (size_t)(size / entsize);
  sfp = bioTell(handle2);
  for(i = 0;i < nitems;i++)
  {
    Elf_Reloc erc;
    lfp=bioTell(handle);
    bioReadBuffer(handle,&relent,sizeof(relent));
    bioSeek(handle,lfp+ELF_REL_SIZE(),SEEKF_START);
    if(entsize > ELF_REL_SIZE()) bioSeek(handle,entsize-ELF_REL_SIZE(),SEEKF_CUR);
    erc.offset = get_f_offset(ELF_OFF(ELF_REL(relent,r_offset)),info);
    erc.info = ELF_XWORD(ELF_REL(relent,r_info));
    /* Entries of type Elf32_Rel store an implicit addend in the
       location to be modified */
    bioSeek(handle2, erc.offset, SEEKF_START);
    switch(ELF_HALF(ELF_EHDR(elf,e_machine)))
    {
      default: erc.addend = 0;
      case EM_ARM: __elf_arm_read_erc(handle2,&erc); break;
      case EM_386: __elf_i386_read_erc(handle2,&erc); break;
      case EM_X86_64: __elf_x86_64_read_erc(handle2,&erc); break;
      case EM_PPC:
      case EM_PPC64: __elf_ppc_read_erc(handle2,&erc); break;
    }
    erc.sh_idx = sh_link;
    if(!la_AddData(CurrElfChain,&erc,NULL)) break;
  }
  bioSeek(handle2,sfp,SEEKF_START);
  bioSeek(handle,fp,SEEKF_START);
}

static void __NEAR__ __FASTCALL__ __elfReadRelaSection(__filesize_t offset,
							__filesize_t size,
							__filesize_t sh_link,
							__filesize_t info,
							__filesize_t entsize)
{
  BGLOBAL handle = elfcache;
  size_t i,nitems;
  ElfXX_External_Rela relent;
  __filesize_t fp, lfp;
  if(!entsize) return;
  fp = bioTell(handle);
  bioSeek(handle,offset,SEEKF_START);
  nitems = (size_t)(size / entsize);
  for(i = 0;i < nitems;i++)
  {
    Elf_Reloc erc;
    lfp=bioTell(handle);
    bioReadBuffer(handle,&relent,sizeof(relent));
    bioSeek(handle,lfp+ELF_RELA_SIZE(), SEEKF_START);
    if(entsize > ELF_RELA_SIZE()) bioSeek(handle,entsize-ELF_RELA_SIZE(),SEEKF_CUR);
    erc.offset = get_f_offset(ELF_OFF(ELF_RELA(relent,r_offset)),info);
    erc.info = ELF_XWORD(ELF_RELA(relent,r_info));
    erc.addend = ELF_XWORD(ELF_RELA(relent,r_addend));
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
  __filesize_t fp;
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
    bioSeek(handle,ELF_OFF(ELF_EHDR(elf,e_shoff)),SEEKF_START);
    _nitems = ELF_HALF(ELF_EHDR(elf,e_shnum));
    for(i = 0;i < _nitems;i++)
    {
      ElfXX_External_Shdr shdr;
      __filesize_t fp;
      if(IsKbdTerminate() || bioEOF(handle)) break;
      fp=bioTell(handle);
      bioReadBuffer(handle,&shdr,sizeof(shdr));
//      bioSeek(handle,fp+ELF_SHDR_SIZE(),SEEKF_START);
      bioSeek(handle,fp+ELF_HALF(ELF_EHDR(elf, e_shentsize)),SEEKF_START);
      switch(ELF_WORD(ELF_SHDR(shdr,sh_type)))
      {
        case SHT_REL: __elfReadRelSection(ELF_OFF(ELF_SHDR(shdr,sh_offset)),
               			          ELF_XWORD(ELF_SHDR(shdr,sh_size)),
            			          ELF_WORD(ELF_SHDR(shdr,sh_link)),
            			          ELF_WORD(ELF_SHDR(shdr,sh_info)),
            			          ELF_XWORD(ELF_SHDR(shdr,sh_entsize)));
                      break;
        case SHT_RELA: __elfReadRelaSection(ELF_OFF(ELF_SHDR(shdr,sh_offset)),
            				    ELF_XWORD(ELF_SHDR(shdr,sh_size)),
            				    ELF_WORD(ELF_SHDR(shdr,sh_link)),
            			            ELF_WORD(ELF_SHDR(shdr,sh_info)),
            			            ELF_XWORD(ELF_SHDR(shdr,sh_entsize)));
                      break;
        default: break;
     }
    }
  }
  else if(ELF_HALF(ELF_EHDR(elf,e_type)) != ET_REL)
  {
    /* If section headers are lost then information can be taken
       from program headers
    */
    __filesize_t dyn_ptr,dynptr,link,type;
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

static Elf_Reloc __HUGE__ * __NEAR__ __FASTCALL__ __found_ElfRel(__filesize_t offset)
{
  Elf_Reloc key;
  if(!CurrElfChain) buildElf386RelChain();
  key.offset = offset;
  return la_Find(CurrElfChain,&key,compare_elf_reloc);
}

static tBool __NEAR__ __FASTCALL__ __readRelocName(Elf_Reloc __HUGE__ *erl, char *buff, size_t cbBuff)
{
  __filesize_t r_sym;
  ElfXX_External_Shdr shdr;
  ElfXX_External_Sym sym;
  BGLOBAL handle = elfcache;
  __filesize_t fp;
  tBool ret = True;
  r_sym = is_64bit?ELF64_R_SYM(erl->info):ELF32_R_SYM(erl->info);
  fp = bioTell(handle);
  if(IsSectionsPresent) /* Section headers are present */
  {
//     bioSeek(handle,ELF_OFF(ELF_EHDR(elf,e_shoff))+erl->sh_idx*ELF_SHDR_SIZE(),SEEKF_START);
     bioSeek(handle,ELF_OFF(ELF_EHDR(elf,e_shoff))+erl->sh_idx*ELF_HALF(ELF_EHDR(elf, e_shentsize)),SEEKF_START);

     bioReadBuffer(handle,&shdr,sizeof(shdr));
     bioSeek(handle,ELF_OFF(ELF_SHDR(shdr,sh_offset)),SEEKF_START);
     /* Minor integrity test */
     ret = ELF_WORD(ELF_SHDR(shdr,sh_type)) == SHT_SYMTAB || ELF_WORD(ELF_SHDR(shdr,sh_type)) == SHT_DYNSYM;
  }
  else bioSeek(handle,erl->sh_idx,SEEKF_START);
  if(ret)
  {
    /* We assume that dynsym and symtab are equal */
    unsigned old_active;
    old_active = active_shtbl;
    if(IsSectionsPresent) active_shtbl = ELF_WORD(ELF_SHDR(shdr,sh_link));
    else
    {
      __filesize_t dynptr;
      unsigned nitems;
      dynptr = findPHEntry(PT_DYNAMIC,&nitems);
      active_shtbl = elfVA2PA(findPHDynEntry(DT_STRTAB,dynptr,nitems));
    }
    bioSeek(handle,r_sym*ELF_SYM_SIZE(),SEEKF_CUR);
    bioReadBuffer(handle,&sym,sizeof(sym));
    elf386_readnametableex(ELF_WORD(ELF_SYM(sym,st_name)),buff,cbBuff);
    buff[cbBuff-1] = '\0';
    active_shtbl = old_active;
    if(!buff[0])
    {
      /* reading name failed - try read at least section name */
      if(IsSectionsPresent)
      {
       if(ELF_ST_TYPE(ELF_SYM(sym,st_info[0])) == STT_SECTION && 
          ELF_HALF(ELF_SYM(sym,st_shndx)) &&
          ELF_IS_SECTION_PHYSICAL(ELF_HALF(ELF_SYM(sym,st_shndx))))
       {
//         bioSeek(handle,ELF_OFF(ELF_EHDR(elf,e_shoff))+ELF_HALF(ELF_SYM(sym,st_shndx))*ELF_SHDR_SIZE(),SEEKF_START);
         bioSeek(handle,ELF_OFF(ELF_EHDR(elf,e_shoff))+ELF_HALF(ELF_SYM(sym,st_shndx))*ELF_HALF(ELF_EHDR(elf, e_shentsize)),SEEKF_START);
         bioReadBuffer(handle,&shdr,sizeof(shdr));
         if(!FindPubName(buff, cbBuff, ELF_OFF(ELF_SHDR(shdr,sh_offset))+erl->addend))
                      elf386_readnametable(ELF_WORD(ELF_SHDR(shdr,sh_name)),buff,cbBuff);
       }
      }
      if(!buff[0]) strcpy(buff,"?noname");
    }
  }
  bioSeek(handle,fp,SEEKF_START);
  return ret;
}

static unsigned long __NEAR__ __FASTCALL__ BuildReferStrElf_arm(char *str,
							Elf_Reloc __HUGE__ *erl,
							int flags,unsigned codelen,
							__filesize_t defval)
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
    case R_ARM_RELATIVE: /* BVA + addendum */
    case R_ARM_NONE: /* nothing to do */
    case R_ARM_COPY: /* nothing to do */
                   retval = RAPREF_NONE;
                   break;
    case R_ARM_THM_ABS5:
    case R_ARM_ABS8:
    case R_ARM_ABS12:
    case R_ARM_ABS16:
    case R_ARM_ABS32:  /* symbol + addendum */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret)
                   {
		     strcat(str,buff);
		     use_addend = True;
		   }
                   else retval = RAPREF_NONE;
		   break;
    case R_ARM_THM_PC8:
    case R_ARM_THM_PC12:
    case R_ARM_PC24: /* symbol + addendum - this */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret)
                   {
		     strcat(str,buff);
		     /* strcat(str,"-.here"); <- it's commented for readability */
		     use_addend = True;
		   }
                   else retval = RAPREF_NONE;
		   break;
    case R_ARM_PLT32: /* PLT[offset] + addendum - this */
                   strcat(str,"PLT-");
                   strcat(str,Get8Digit(erl->offset));
		   use_addend = True;
		   break;
    case R_ARM_GLOB_DAT:  /* symbol */
    case R_ARM_JUMP_SLOT:  /* symbol */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret) strcat(str,buff);
		   break;
    case R_ARM_GOTOFF32: /* symbol + addendum - GOT */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret)
                   {
  		     strcat(str,buff);
		     strcat(str,"-GOT");
		     use_addend = True;
		   }
                   else retval = RAPREF_NONE;
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

static unsigned long __NEAR__ __FASTCALL__ BuildReferStrElf_i386(char *str,
							Elf_Reloc __HUGE__ *erl,
							int flags,unsigned codelen,
							__filesize_t defval)
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
    case R_386_RELATIVE: /* BVA + addend. Ignoring it is best choice */
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

static unsigned long __NEAR__ __FASTCALL__ BuildReferStrElf_x86_64(char *str,
							Elf_Reloc __HUGE__ *erl,
							int flags,unsigned codelen,
							__filesize_t defval)
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
    case R_X86_64_RELATIVE: /* BVA + addendum */
    case R_X86_64_NONE: /* nothing to do */
    case R_X86_64_COPY: /* nothing to do */
                   retval = RAPREF_NONE;
                   break;
    case R_X86_64_8:
    case R_X86_64_16:
    case R_X86_64_32:
    case R_X86_64_64:  /* symbol + addendum */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret)
                   {
		     strcat(str,buff);
		     use_addend = True;
		   }
                   else retval = RAPREF_NONE;
		   break;
    case R_X86_64_PC8:
    case R_X86_64_PC16:
    case R_X86_64_PC32:
    case R_X86_64_PC64: /* symbol + addendum - this */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret)
                   {
		     strcat(str,buff);
		     /* strcat(str,"-.here"); <- it's commented for readability */
		     use_addend = True;
		   }
                   else retval = RAPREF_NONE;
		   break;
    case R_X86_64_GOT32:
                   strcat(str,"GOT-");
                   strcat(str,Get8Digit(erl->offset));
		   use_addend = True;
		   break;
    case R_X86_64_GOT64:
    case R_X86_64_GOTPC64:
    case R_X86_64_GOTPLT64: /* GOT[offset] + addendum - this */
                   strcat(str,"GOT-");
                   strcat(str,Get16Digit(erl->offset));
		   use_addend = True;
		   break;
    case R_X86_64_PLT32:
                   strcat(str,"PLT-");
                   strcat(str,Get8Digit(erl->offset));
		   use_addend = True;
		   break;
    case R_X86_64_PLTOFF64: /* PLT[offset] + addendum - this */
                   strcat(str,"PLT-");
                   strcat(str,Get16Digit(erl->offset));
		   use_addend = True;
		   break;
    case R_X86_64_GLOB_DAT:  /* symbol */
    case R_X86_64_JUMP_SLOT:  /* symbol */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret) strcat(str,buff);
		   break;
    case R_X86_64_GOTOFF64: /* symbol + addendum - GOT */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret)
                   {
  		     strcat(str,buff);
		     strcat(str,"-GOT");
		     use_addend = True;
		   }
                   else retval = RAPREF_NONE;
		   break;
    case R_X86_64_GOTPCREL64: /* GOT + addendum - this */
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

static unsigned long __NEAR__ __FASTCALL__ BuildReferStrElf_ppc(char *str,
							Elf_Reloc __HUGE__ *erl,
							int flags,unsigned codelen,
							__filesize_t defval)
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
    case R_PPC_RELATIVE: /* BVA + addendum */
    case R_PPC_NONE: /* nothing to do */
    case R_PPC_COPY: /* nothing to do */
                   retval = RAPREF_NONE;
                   break;
    case R_PPC_ADDR14:
    case R_PPC_ADDR14_BRTAKEN:
    case R_PPC_ADDR14_BRNTAKEN:
    case R_PPC_ADDR16:
    case R_PPC_ADDR16_LO:
    case R_PPC_ADDR16_HI:
    case R_PPC_ADDR16_HA:
    case R_PPC_ADDR24:
    case R_PPC_ADDR32:
    case R_PPC_UADDR32:
    case R_PPC64_ADDR64:
    case R_PPC64_UADDR64: /* symbol + addendum */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret)
                   {
		     strcat(str,buff);
		     use_addend = True;
		   }
                   else retval = RAPREF_NONE;
		   break;
    case R_PPC_REL14:
    case R_PPC_REL14_BRTAKEN:
    case R_PPC_REL14_BRNTAKEN:
    case R_PPC_REL16:
    case R_PPC_REL16_LO:
    case R_PPC_REL16_HI:
    case R_PPC_REL16_HA:
    case R_PPC_REL24:
    case R_PPC_REL32:
    case R_PPC64_REL64: /* symbol + addendum - this */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret)
                   {
		     strcat(str,buff);
		     /* strcat(str,"-.here"); <- it's commented for readability */
		     use_addend = True;
		   }
                   else retval = RAPREF_NONE;
		   break;
    case R_PPC_GOT16_LO:
    case R_PPC_GOT16_HI:
    case R_PPC_GOT16_HA:
		   strcat(str,"GOT-");
		strcat(str,Get8Digit(erl->offset));
		   use_addend = True;
		   break;
    case R_PPC_PLT16_LO:
    case R_PPC_PLT16_HI:
    case R_PPC_PLT16_HA:
    case R_PPC_PLT32:
                   strcat(str,"PLT-");
                   strcat(str,Get8Digit(erl->offset));
		   use_addend = True;
		   break;
    case R_PPC64_PLT64: /* PLT[offset] + addendum - this */
                   strcat(str,"PLT-");
                   strcat(str,Get16Digit(erl->offset));
		   use_addend = True;
		   break;
    case R_PPC_GLOB_DAT:  /* symbol */
    case R_PPC_JMP_SLOT:  /* symbol */
                   ret = __readRelocName(erl, buff, sizeof(buff));
                   if(buff[0] && ret) strcat(str,buff);
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

static unsigned long __NEAR__ __FASTCALL__ BuildReferStrElf(char *str,
							Elf_Reloc __HUGE__ *erl,
							int flags,unsigned codelen,
							__filesize_t defval)
{
    switch(ELF_HALF(ELF_EHDR(elf,e_machine)))
    {
      default: return RAPREF_NONE;
      case EM_ARM: return BuildReferStrElf_arm(str,erl,flags,codelen,defval);
      case EM_386: return BuildReferStrElf_i386(str,erl,flags,codelen,defval);
      case EM_X86_64: return BuildReferStrElf_x86_64(str,erl,flags,codelen,defval);
      case EM_PPC:
      case EM_PPC64: return BuildReferStrElf_ppc(str,erl,flags,codelen,defval);
    }
}

#define S_INTERPRETER "Interpreter : "

static void __NEAR__ __FASTCALL__ displayELFdyninfo(__filesize_t f_off,unsigned nitems)
{
  ElfXX_External_Dyn dyntab;
  __filesize_t curroff,stroff;
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
    f_off += ELF_EDYN_SIZE();
    is_add = True;
    switch(ELF_XWORD(ELF_EDYN(dyntab,d_tag)))
    {
      case DT_NULL: goto dyn_end;
      case DT_NEEDED:
                    {
                      strcpy(stmp,"Needed : ");
                      bmReadBufferEx(&stmp[strlen(stmp)],70,ELF_XWORD(ELF_EDYN(dyntab,d_un.d_ptr)) + stroff,SEEKF_START);
                    }
                    break;
      case DT_SONAME:
                    {
                      strcpy(stmp,"SO name: ");
                      bmReadBufferEx(&stmp[strlen(stmp)],70,ELF_XWORD(ELF_EDYN(dyntab,d_un.d_ptr)) + stroff,SEEKF_START);
                    }
                    break;
      case DT_RPATH:
                    {
                      strcpy(stmp,"LibPath: ");
                      bmReadBufferEx(&stmp[strlen(stmp)],70,ELF_XWORD(ELF_EDYN(dyntab,d_un.d_ptr)) + stroff,SEEKF_START);
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

static __filesize_t __FASTCALL__ ShowELFDynInfo( void )
{
  __filesize_t dynptr,fpos;
  unsigned number;
  fpos = BMGetCurrFilePos();
  dynptr = findPHEntry(PT_DYNAMIC,&number);
  if(!dynptr) { NotifyBox(NOT_ENTRY," ELF dynamic linking information "); return fpos; }
  displayELFdyninfo(dynptr,number);
  BMSeek(fpos, SEEKF_START);
  return fpos;
}

static unsigned long __FASTCALL__ AppendELFRef(char *str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  char buff[400];
  unsigned long ret = RAPREF_NONE;
  Elf_Reloc __HUGE__ *erl;
  __filesize_t defval;
  switch(codelen) {
    default:
    case 1: defval = bmReadByteEx(ulShift, SEEKF_START); break;
    case 2: defval = bmReadWordEx(ulShift, SEEKF_START); break;
    case 4: defval = bmReadDWordEx(ulShift, SEEKF_START); break;
    case 8: defval = bmReadQWordEx(ulShift, SEEKF_START); break;
  }
  if(flags & APREF_TRY_PIC)
  {
       __filesize_t off_in_got = defval;
       __filesize_t dynptr, dyn_ent, got_off;
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
    ret = BuildReferStrElf(str,erl,flags,codelen,defval);
  }
  if(!ret && ELF_HALF(ELF_EHDR(elf,e_type))>ET_REL && codelen>=4)
  {
    if((erl = __found_ElfRel(elfVA2PA(bmReadDWordEx(ulShift,SEEKF_START)))) != NULL)
    {
      ret = BuildReferStrElf(str,erl,flags,codelen,defval);
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
 ElfXX_External_Phdr phdr;
 ElfXX_External_Shdr shdr;
 struct tag_elfVAMap vamap;
 __filesize_t fp;
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
     va_map_count = ELF_HALF(ELF_EHDR(elf,e_shnum));
     if(!(*to = la_Build(0,sizeof(struct tag_elfVAMap),MemOutBox)))
     {
       exit(EXIT_FAILURE);
     }
     bmSeek(ELF_OFF(ELF_EHDR(elf,e_shoff)),SEEKF_START);
     for(i = 0;i < va_map_count;i++)
     {
       __filesize_t flg,x_flags;
       fp = bmGetCurrFilePos();
       bmReadBuffer(&shdr,sizeof(shdr));
       bmSeek(fp+ELF_HALF(ELF_EHDR(elf,e_shentsize)),SEEKF_START);
       vamap.va = ELF_OFF(ELF_SHDR(shdr,sh_addr));
       vamap.size = ELF_XWORD(ELF_SHDR(shdr,sh_size));
       vamap.foff = ELF_OFF(ELF_SHDR(shdr,sh_offset));
       vamap.nameoff = ELF_WORD(ELF_SHDR(shdr,sh_name));
       flg = ELF_XWORD(ELF_SHDR(shdr,sh_flags));
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
    if((va_map_count = ELF_HALF(ELF_EHDR(elf,e_phnum))) != 0) /* Program headers are present */
    {
      if(!(*to = la_Build(va_map_count,sizeof(struct tag_elfVAMap),MemOutBox)))
      {
        exit(EXIT_FAILURE);
      }
      bmSeek(ELF_OFF(ELF_EHDR(elf,e_phoff)),SEEKF_START);
      for(i = 0;i < va_map_count;i++)
      {
        fp = bmGetCurrFilePos();
        bmReadBuffer(&phdr,sizeof(phdr));
        bmSeek(fp+ELF_HALF(ELF_EHDR(elf,e_phentsize)),SEEKF_START);
        vamap.va = ELF_ADDR(ELF_PHDR(phdr,p_vaddr));
        vamap.size = max(ELF_OFF(ELF_PHDR(phdr,p_filesz)), ELF_OFF(ELF_PHDR(phdr,p_memsz)));
        vamap.foff = ELF_OFF(ELF_PHDR(phdr,p_offset));
        vamap.nameoff = ELF_WORD(ELF_PHDR(phdr,p_type)) & 0x000000FFUL ? ~ELF_WORD(ELF_PHDR(phdr,p_type)) : 0xFFFFFFFFUL;
        vamap.flags = ELF_WORD(ELF_PHDR(phdr,p_flags));
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
 __filesize_t fs;
 size_t i;
   PMRegLowMemCallBack(elfLowMemFunc);
   bmReadBufferEx(&elf,sizeof(ElfXX_External_Ehdr),0,SEEKF_START);
   is_msbf = ELF_EHDR(elf,e_ident[EI_DATA]) == ELFDATA2MSB;
   is_64bit = ELF_EHDR(elf,e_ident[EI_CLASS]) == ELFCLASS64;
   fs = bmGetFLength();
   IsSectionsPresent = ELF_HALF(ELF_EHDR(elf,e_shnum)) != 0 &&
                       ELF_OFF(ELF_EHDR(elf,e_shoff)) &&
                       ELF_OFF(ELF_EHDR(elf,e_shoff)) < fs &&
                       ELF_OFF(ELF_EHDR(elf,e_shoff)) +
                       ELF_HALF(ELF_EHDR(elf,e_shnum))*ELF_HALF(ELF_EHDR(elf,e_shentsize)) <= fs;
   __elfReadSegments(&va_map_virt,True);
   __elfReadSegments(&va_map_phys,False);
   /** Find min value of virtual address */
   if(va_map_virt)
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

static int __FASTCALL__ ELFbitness(__filesize_t off)
{
  UNUSED(off);
  return is_64bit?DAB_USE64:DAB_USE32;
}

static __filesize_t __FASTCALL__ ELFHelp( void )
{
  hlpDisplay(10003);
  return BMGetCurrFilePos();
}

static tBool __FASTCALL__ ELFAddrResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  tBool bret = True;
  __filesize_t res;
  if(cfpos < ELF_EHDR_SIZE())
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

static tBool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa)
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
  return udnFindName(pa,buff,cb_buff);
}

static void __FASTCALL__ elf_ReadPubNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *))
{
  __filesize_t fpos,fp,tableptr,pubname_shtbl;
  unsigned long i,number,ent_size,nitems;
  struct PubName epn;
  BGLOBAL b_cache;
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
     ElfXX_External_Dyn pdyn;
     fp = bioTell(b_cache);
     bioReadBuffer(b_cache,&pdyn,sizeof(pdyn));
     if(bioEOF(b_cache)) break;
     bioSeek(b_cache,fp+ent_size,SEEKF_START);
     epn.nameoff = ELF_XWORD(ELF_EDYN(pdyn,d_tag));
     epn.pa = elfVA2PA(ELF_XWORD(ELF_EDYN(pdyn,d_un.d_val)));
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
      ElfXX_External_Sym sym;
      fp = bioTell(handle);
      bioReadBuffer(handle,&sym,sizeof(sym));
      if(bioEOF(handle) || IsKbdTerminate()) break;
      bioSeek(handle,fp+__elfSymEntSize,SEEKF_START);
      if(ELF_IS_SECTION_PHYSICAL(ELF_HALF(ELF_SYM(sym,st_shndx))) &&
         ELF_ST_TYPE(ELF_SYM(sym,st_info[0])) != STT_SECTION)
      {
        epn.pa = __calcSymEntry(handle,i,False);
        epn.nameoff = ELF_WORD(ELF_SYM(sym,st_name));
        epn.addinfo = __elfSymShTbl;
        epn.attr = ELF_SYM(sym,st_info[0]);
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

static __filesize_t __FASTCALL__ elfGetPubSym(char *str,unsigned cb_str,unsigned *func_class,
                           __filesize_t pa,tBool as_prev)
{
   return fmtGetPubSym(elfcache,str,cb_str,func_class,pa,as_prev,
                       elf_ReadPubNameList,
                       elf_ReadPubName);
}

static unsigned __FASTCALL__ elfGetObjAttr(__filesize_t pa,char *name,unsigned cb_name,
                       __filesize_t *start,__filesize_t *end,int *_class,int *bitness)
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

static int __FASTCALL__ ELFplatform( void ) {
    unsigned id;
    elf_machine(ELF_HALF(ELF_EHDR(elf,e_machine)),&id);
    return id;
}

static int __FASTCALL__ ELFendian(__filesize_t off) {
 return is_msbf?DAE_BIG:DAE_LITTLE;
}

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
  ELFendian,
  ELFAddrResolv,
  elfVA2PA,
  elfPA2VA,
  elfGetPubSym,
  elfGetObjAttr,
  NULL,
  NULL
};
