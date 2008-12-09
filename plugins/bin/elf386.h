/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/elf386.h
 * @brief       This file defines standard ELF types, structures, and macros.
 * @version     -
 * @remark      Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation,
 *              Inc. This file is part of the GNU C Library.
 *              The GNU C Library is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU Library General Public License as
 *              published by the Free Software Foundation; either version 2 of the
 *              License, or (at your option) any later version.
 *              The GNU C Library is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *              Library General Public License for more details.
 *              You should have received a copy of the GNU Library General Public
 *              License along with the GNU C Library; see the file COPYING.LIB.  If not,
 *              write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *              Boston, MA 02111-1307, USA.
 * @remark	portions taken from FreeBSD (sys/elf*.h), and updates from
 *              current draft (http://www.caldera.com/developers/gabi/)
 * @note        Requires POSIX compatible development system
 *
 * @author      GNU FSF
 * @since       1995
**/
#ifndef __ELF_INC
#define __ELF_INC

#ifndef __SYS_DEP_H
#include "_sys_dep.h"
#endif

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

#define EI_NIDENT	16		/**< Size of e_ident[] */
/** Fields in e_ident[] */

#define EI_MAG0		0		/**< File identification byte 0 index */
#define ELFMAG0		0x7F		/**< Magic number byte 0 */

#define EI_MAG1		1		/**< File identification byte 1 index */
#define ELFMAG1		'E'		/**< Magic number byte 1 */

#define EI_MAG2		2		/**< File identification byte 2 index */
#define ELFMAG2		'L'		/**< Magic number byte 2 */

#define EI_MAG3		3		/**< File identification byte 3 index */
#define ELFMAG3		'F'		/**< Magic number byte 3 */

#define EI_CLASS	4		/**< File class */
#define ELFCLASSNONE	0		/**< Invalid class */
#define ELFCLASS32	1		/**< 32-bit objects */
#define ELFCLASS64	2		/**< 64-bit objects */

#define EI_DATA		5		/**< Data encoding */
#define ELFDATANONE	0		/**< Invalid data encoding */
#define ELFDATA2LSB	1		/**< 2's complement, little endian */
#define ELFDATA2MSB	2		/**< 2's complement, big endian */

#define EI_VERSION	6		/**< File version */

#define EI_OSABI	7		/**< Operating system / ABI identification */
#define ELFOSABI_SYSV		0	/**< UNIX System V ABI */
#define ELFOSABI_NONE		ELFOSABI_SYSV	/**< symbol used in old spec */
#define ELFOSABI_HPUX		1	/**< HP-UX operating system */
#define ELFOSABI_NETBSD		2	/**< NetBSD */
#define ELFOSABI_LINUX		3	/**< GNU/Linux */
#define ELFOSABI_HURD		4	/**< GNU/Hurd */
#define ELFOSABI_86OPEN		5	/**< 86Open common IA32 ABI */
#define ELFOSABI_SOLARIS	6	/**< Solaris */
#define ELFOSABI_MONTEREY	7	/**< Monterey */
#define ELFOSABI_IRIX		8	/**< IRIX */
#define ELFOSABI_FREEBSD	9	/**< FreeBSD */
#define ELFOSABI_TRU64		10	/**< TRU64 UNIX */
#define ELFOSABI_MODESTO	11	/**< Novell Modesto */
#define ELFOSABI_OPENBSD	12	/**< OpenBSD */
#define ELFOSABI_ARM		97	/**< ARM */
#define ELFOSABI_STANDALONE	255	/**< Standalone (embedded) application */

#define EI_ABIVERSION	8	/**< ABI version */
#define OLD_EI_BRAND	8	/**< Start of architecture identification. */
#define EI_PAD		9	/**< Start of padding (per SVR4 ABI). */
#define EI_NIDENT	16	/**< Size of e_ident array. */

/* e_ident */
#define IS_ELF(e_ident)	(e_ident[EI_MAG0] == ELFMAG0 && \
			 e_ident[EI_MAG1] == ELFMAG1 && \
			 e_ident[EI_MAG2] == ELFMAG2 && \
			 e_ident[EI_MAG3] == ELFMAG3)

/** Values for e_type, which identifies the object file type */

#define ET_NONE		0		/**< No file type */
#define ET_REL		1		/**< Relocatable file */
#define ET_EXEC		2		/**< Executable file */
#define ET_DYN		3		/**< Shared object file */
#define ET_CORE		4		/**< Core file */
#define ET_LOOS		0xFE00U		/**< OS-specific */
#define ET_HIOS		0xFEFFU		/**< OS-specific */
#define ET_LOPROC	0xFF00U		/**< Processor-specific */
#define ET_HIPROC	0xFFFFU		/**< Processor-specific */

/** Values for e_machine, which identifies the architecture */

#define EM_NONE		0	/**< No machine */
#define EM_M32		1	/**< AT&T WE 32100 */
#define EM_SPARC	2	/**< Sun SPARC */
#define EM_386		3	/**< Intel 80386 */
#define EM_68K		4	/**< Motorola m68k */
#define EM_88K		5	/**< Motorola m88k */
				/* 6 reserved, was EM_486 */
#define EM_860		7	/**< Intel 80860 */
#define EM_MIPS		8	/**< MIPS I */
#define EM_S370		9	/**< IBM System/370 */
#define EM_MIPS_RS3_LE	10	/**< MIPS R3000 little-endian */
				/**< 11-14 reserved */
#define EM_PARISC	15	/**< HP PA-RISC */
				/**< 16 reserved */
#define	EM_VPP500	17	/**< Fujitsu VPP500 */
#define EM_SPARC32PLUS	18	/**< Sun SPARC v8plus */
#define EM_960		19	/**< Intel 80960 */
#define EM_PPC		20	/**< PowerPC */
#define EM_PPC64	21	/**< PowerPC 64-bit */
#define	EM_S390		22	/**< IBM System/390 */
				/**< 23-35 reserved */
#define	EM_V800		36	/**< NEC V800 */
#define	EM_FR20		37	/**< Fujitsu FR20 */
#define	EM_RH32		38	/**< TRW RH-32 */
#define	EM_RCE		39	/**< Motorola RCE */
#define	EM_ARM		40	/**< Advanced RISC Machines ARM */
#define	EM_ALPHA	41	/**< Digital Alpha */
#define	EM_SH		42	/**< Hitachi SH */
#define	EM_SPARCV9	43	/**< SPARC Version 9 64-bit */
#define	EM_TRICORE	44	/**< Siemens TriCore embedded processor */
#define	EM_ARC		45	/**< Argonaut RISC Core, Argonaut Technologies Inc. */
#define	EM_H8_300	46	/**< Hitachi H8/300 */
#define	EM_H8_300H	47	/**< Hitachi H8/300H */
#define	EM_H8S		48	/**< Hitachi H8S */
#define	EM_H8_500	49	/**< Hitachi H8/500 */
#define	EM_IA_64	50	/**< Intel IA-64 processor architecture */
#define	EM_MIPS_X	51	/**< Stanford MIPS-X */
#define	EM_COLDFIRE	52	/**< Motorola ColdFire */
#define	EM_68HC12	53	/**< Motorola M68HC12 */
#define	EM_MMA		54	/**< Fujitsu MMA Multimedia Accelerator */
#define	EM_PCP		55	/**< Siemens PCP */
#define	EM_NCPU		56	/**< Sony nCPU embedded RISC processor */
#define	EM_NDR1		57	/**< Denso NDR1 microprocessor */
#define	EM_STARCORE	58	/**< Motorola Star*Core processor */
#define	EM_ME16		59	/**< Toyota ME16 processor */
#define	EM_ST100	60	/**< STMicroelectronics ST100 processor */
#define	EM_TINYJ	61	/**< Advanced Logic Corp. TinyJ embedded processor family */
#define	EM_X86_64	62	/**< AMD x86-64 architecture */
#define	EM_PDSP		63	/**< Sony DSP Processor */
#define EM_PDP10	64	/**< Digital Equipment Corp. PDP-10 */
#define EM_PDP11	65	/**< Digital Equipment Corp. PDP-11 */
#define	EM_FX66		66	/**< Siemens FX66 microcontroller */
#define	EM_ST9PLUS	67	/**< STMicroelectronics ST9+ 8/16 bit microcontroller */
#define	EM_ST7		68	/**< STMicroelectronics ST7 8-bit microcontroller */
#define	EM_68HC16	69	/**< Motorola MC68HC16 Microcontroller */
#define	EM_68HC11	70	/**< Motorola MC68HC11 Microcontroller */
#define	EM_68HC08	71	/**< Motorola MC68HC08 Microcontroller */
#define	EM_68HC05	72	/**< Motorola MC68HC05 Microcontroller */
#define	EM_SVX		73	/**< Silicon Graphics SVx */
#define	EM_ST19		74	/**< STMicroelectronics ST19 8-bit microcontroller */
#define	EM_VAX		75	/**< Digital VAX */
#define	EM_CRIS		76	/**< Axis Communications 32-bit embedded processor */
#define	EM_JAVELIN	77	/**< Infineon Technologies 32-bit embedded processor */
#define	EM_FIREPATH	78	/**< Element 14 64-bit DSP Processor */
#define	EM_ZSP		79	/**< LSI Logic 16-bit DSP Processor */
#define	EM_MMIX		80	/**< Donald Knuth's educational 64-bit processor */
#define	EM_HUANY	81	/**< Harvard University machine-independent object files */
#define	EM_PRISM	82	/**< SiTera Prism */
#define	EM_AVR		83	/**< Atmel AVR 8-bit microcontroller */
#define	EM_FR30		84	/**< Fujitsu FR30 */
#define	EM_D10V		85	/**< Mitsubishi D10V */
#define	EM_D30V		86	/**< Mitsubishi D30V */
#define	EM_V850		87	/**< NEC v850 */
#define	EM_M32R		88	/**< Mitsubishi M32R */
#define	EM_MN10300	89	/**< Matsushita MN10300 */
#define	EM_MN10200	90	/**< Matsushita MN10200 */
#define	EM_PJ		91	/**< picoJava */
#define	EM_OPENRISC	92	/**< OpenRISC 32-bit embedded processor */
#define	EM_ARC_A5	93	/**< ARC Cores Tangent-A5 */
#define	EM_XTENSA	94	/**< Tensilica Xtensa Architecture */
#define EM_VIDEOCORE	95	/**< Alphamosaic VideoCore processor */
#define EM_TMM_GPP	96	/**< Thompson Multimedia General Purpose Processor */
#define EM_NS32K	97	/**< National Semiconductor 32000 series */
#define EM_TPC		98	/**< Tenor Network TPC processor */
#define EM_SNP1K	99	/**< Trebia SNP 1000 processor */
#define EM_IP2K		101	/**< Ubicom IP2022 micro controller */
#define EM_CR		103	/**< National Semiconductor CompactRISC */
#define EM_MSP430	105	/**< TI msp430 micro controller */
#define EM_BLACKFIN	106	/**< ADI Blackfin */
#define EM_ALTERA_NIOS2	113	/**< Altera Nios II soft-core processor */
#define EM_CRX		114	/**< National Semiconductor CRX */

#define EM_XGATE	115	/**< Motorola XGATE embedded processor */
#define EM_C166		116	/**< Infineon C16x/XC16x processor */
#define EM_M16C		117	/**< Renesas M16C series microprocessors */
#define EM_DSPIC30F	118	/**< Microchip Technology dsPIC30F Digital Signal Controller */
#define EM_CE		119	/**< Freescale Communication Engine RISC core */
#define EM_M32C		120	/**< Renesas M32C series microprocessors */

#define EM_TSK3000	131	/**< Altium TSK3000 core */
#define EM_RS08		132	/**< Freescale RS08 embedded processor */

#define EM_ECOG2	134	/**< Cyan Technology eCOG2 microprocessor */
#define EM_SCORE	135	/**< Sunplus Score */
#define EM_DSP24	136	/**< New Japan Radio (NJR) 24-bit DSP Processor */
#define EM_VIDEOCORE3	137	/**< Broadcom VideoCore III processor */
#define EM_LATTICEMICO32 138	/**< RISC processor for Lattice FPGA architecture */
#define EM_SE_C17	139	/**< Seiko Epson C17 family */

#define EM_MMDSP_PLUS	160	/**< STMicroelectronics 64bit VLIW Data Signal Processor */
#define EM_CYPRESS_M8C	161	/**< Cypress M8C microprocessor */
#define EM_R32C		162	/**< Renesas R32C series microprocessors */
#define EM_TRIMEDIA	163	/**< NXP Semiconductors TriMedia architecture family */
#define EM_QDSP6	164	/**< QUALCOMM DSP6 Processor */
#define EM_8051		165	/**< Intel 8051 and variants */
#define EM_STXP7X	166	/**< STMicroelectronics STxP7x family */
#define EM_NDS32	167	/**< Andes Technology compact code size embedded RISC processor family */
#define EM_ECOG1X	168	/**< Cyan Technology eCOG1X family */
#define EM_MAXQ30	169	/**< Dallas Semiconductor MAXQ30 Core Micro-controllers */
#define EM_XIMO16	170	/**< New Japan Radio (NJR) 16-bit DSP Processor */
#define EM_MANIK	171	/**< M2000 Reconfigurable RISC Microprocessor */
#define EM_CRAYNV2	172	/**< Cray Inc. NV2 vector architecture */
#define EM_RX		173	/**< Renesas RX family */
#define EM_METAG	174	/**< Imagination Technologies META processor architecture */
#define EM_MCST_ELBRUS	175	/**< MCST Elbrus general purpose hardware architecture */
#define EM_ECOG16	176	/**< Cyan Technology eCOG16 family */
#define EM_CR16		177	/**< National Semiconductor CompactRISC 16-bit processor */

/** If it is necessary to assign new unofficial EM_* values, please pick large
   random numbers (0x8523, 0xa7f2, etc.) to minimize the chances of collision
   with official or non-GNU unofficial values.

   NOTE: Do not just increment the most recent number by one.
   Somebody else somewhere will do exactly the same thing, and you
   will have a collision.  Instead, pick a random number.  */

/** Cygnus PowerPC ELF backend.  Written in the absence of an ABI.  */
#define EM_CYGNUS_POWERPC 0x9025U

/** Cygnus M32R ELF backend.  Written in the absence of an ABI.  */
#define EM_CYGNUS_M32R 0x9041U

/** D10V backend magic number.  Written in the absence of an ABI.  */
#define EM_CYGNUS_D10V	0x7650U

/** mn10200 and mn10300 backend magic numbers.
   Written in the absense of an ABI.  */
#define EM_CYGNUS_MN10200	0xdeadL
#define EM_CYGNUS_MN10300	0xbeefL

/** See the above comment before you add a new EM_* value here.  */

/** Values for e_version */

#define EV_NONE		0		/**< Invalid ELF version */
#define EV_CURRENT	1		/**< Current version */

/** Values for program header, p_type field */

#define	PT_NULL		0		/**< Program header table entry unused */
#define PT_LOAD		1		/**< Loadable program segment */
#define PT_DYNAMIC	2		/**< Dynamic linking information */
#define PT_INTERP	3		/**< Program interpreter */
#define PT_NOTE		4		/**< Auxiliary information */
#define PT_SHLIB	5		/**< Reserved, unspecified semantics */
#define PT_PHDR		6		/**< Entry for header table itself */
#define	PT_NUM		7		/**< Number of defined types.  */
#define PT_LOPROC	0x70000000UL	/**< Processor-specific */
#define PT_HIPROC	0x7FFFFFFFUL	/**< Processor-specific */

/** Program segment permissions, in program header p_flags field */

#define PF_X		(1 << 0)	/**< Segment is executable */
#define PF_W		(1 << 1)	/**< Segment is writable */
#define PF_R		(1 << 2)	/**< Segment is readable */
#define PF_MASKPROC	0xF0000000UL	/**< Processor-specific reserved bits */

/** Values for section header, sh_type field */

#define SHT_NULL	0		/**< Section header table entry unused */
#define SHT_PROGBITS	1		/**< Program specific (private) data */
#define SHT_SYMTAB	2		/**< Link editing symbol table */
#define SHT_STRTAB	3		/**< A string table */
#define SHT_RELA	4		/**< Relocation entries with addends */
#define SHT_HASH	5		/**< A symbol hash table */
#define SHT_DYNAMIC	6		/**< Information for dynamic linking */
#define SHT_NOTE	7		/**< Information that marks file */
#define SHT_NOBITS	8		/**< Section occupies no space in file */
#define SHT_REL		9		/**< Relocation entries, no addends */
#define SHT_SHLIB	10		/**< Reserved, unspecified semantics */
#define SHT_DYNSYM	11		/**< Dynamic linking symbol table */
#define	SHT_NUM		12		/**< Number of defined types.  */

#define SHT_LOSUNW	0x6ffffffdUL	/**< Sun-specific low bound.  */

/** The next three section types are defined by Solaris, and are named
   SHT_SUNW*.  We use them in GNU code, so we also define SHT_GNU*
   versions.  */
#define SHT_SUNW_verdef	0x6ffffffdUL	/**< Versions defined by file */
#define SHT_SUNW_verneed 0x6ffffffeUL	/**< Versions needed by file */
#define SHT_SUNW_versym	0x6fffffffUL	/**< Symbol versions */

#define SHT_GNU_verdef	SHT_SUNW_verdef
#define SHT_GNU_verneed	SHT_SUNW_verneed
#define SHT_GNU_versym	SHT_SUNW_versym

#define SHT_LOPROC	0x70000000UL	/**< Processor-specific semantics, lo */
#define SHT_HIPROC	0x7FFFFFFFUL	/**< Processor-specific semantics, hi */
#define SHT_LOUSER	0x80000000UL	/**< Application-specific semantics */
#define SHT_HIUSER	0x8FFFFFFFUL	/**< Application-specific semantics */

/** Values for section header, sh_flags field */

#define SHF_WRITE	(1 << 0)	/**< Writable data during execution */
#define SHF_ALLOC	(1 << 1)	/**< Occupies memory during execution */
#define SHF_EXECINSTR	(1 << 2)	/**< Executable machine instructions */
#define SHF_MASKPROC	0xF0000000UL	/**< Processor-specific semantics */

/** Values of note segment descriptor types for core files. */

#define NT_PRSTATUS	1		/**< Contains copy of prstatus struct */
#define NT_FPREGSET	2		/**< Contains copy of fpregset struct */
#define NT_PRPSINFO	3		/**< Contains copy of prpsinfo struct */

/** Values of note segment descriptor types for object files.
   (Only for hppa right now.  Should this be moved elsewhere?)  */

#define NT_VERSION	1		/**< Contains a version string.  */

/** These three macros disassemble and assemble a symbol table st_info field,
    which contains the symbol binding and symbol type.  The STB_ and STT_
    defines identify the binding and type. */

#define ELF_ST_BIND(val)		(((tUInt8)(val)) >> 4)
#define ELF_ST_TYPE(val)		(((tUInt8)(val)) & 0xF)
#define ELF_ST_INFO(bind,type)		((((tUInt8)(bind)) << 4) + ((((tUInt8)type)) & 0xF))

#define STN_UNDEF	0		/**< undefined symbol index */

#define STB_LOCAL	0		/**< Symbol not visible outside obj */
#define STB_GLOBAL	1		/**< Symbol visible outside obj */
#define STB_WEAK	2		/**< Like globals, lower precedence */
#define	STB_NUM		3		/**< Number of defined types.  */
#define STB_LOPROC	13		/**< Application-specific semantics */
#define STB_HIPROC	15		/**< Application-specific semantics */

#define STT_NOTYPE	0		/**< Symbol type is unspecified */
#define STT_OBJECT	1		/**< Symbol is a data object */
#define STT_FUNC	2		/**< Symbol is a code object */
#define STT_SECTION	3		/**< Symbol associated with a section */
#define STT_FILE	4		/**< Symbol gives a file name */
#define	STT_NUM		5		/**< Number of defined types.  */
#define STT_LOPROC	13		/**< Application-specific semantics */
#define STT_HIPROC	15		/**< Application-specific semantics */

/** Special section indices, which may show up in st_shndx fields, among
    other places. */

#define SHN_UNDEF	0		/**< Undefined section reference */
#define SHN_LORESERVE	0xFF00U		/**< Begin range of reserved indices */
#define SHN_LOPROC	0xFF00U		/**< Begin range of appl-specific */
#define SHN_HIPROC	0xFF1FU		/**< End range of appl-specific */
#define SHN_ABS		0xFFF1U		/**< Associated symbol is absolute */
#define SHN_COMMON	0xFFF2U		/**< Associated symbol is in common */
#define SHN_HIRESERVE	0xFFFFU		/**< End range of reserved indices */

/** relocation info handling macros */

#define ELF32_R_SYM(i)		((i) >> 8)
#define ELF32_R_TYPE(i)		((i) & 0xff)
#define ELF32_R_INFO(s,t)	(((s) << 8) + ((t) & 0xff))

#define ELF64_R_SYM(i)		((i) >> 32)
#define ELF64_R_TYPE(i)		((i) & 0xffffffff)
#define ELF64_R_INFO(s,t)	(((bfd_vma) (s) << 32) + (bfd_vma) (t))

/** Legal values for d_tag (dynamic entry type).  */

#define DT_NULL		0		/**< Marks end of dynamic section */
#define DT_NEEDED	1		/**< Name of needed library */
#define DT_PLTRELSZ	2		/**< Size in bytes of PLT relocs */
#define DT_PLTGOT	3		/**< Processor defined value */
#define DT_HASH		4		/**< Address of symbol hash table */
#define DT_STRTAB	5		/**< Address of string table */
#define DT_SYMTAB	6		/**< Address of symbol table */
#define DT_RELA		7		/**< Address of Rela relocs */
#define DT_RELASZ	8		/**< Total size of Rela relocs */
#define DT_RELAENT	9		/**< Size of one Rela reloc */
#define DT_STRSZ	10		/**< Size of string table */
#define DT_SYMENT	11		/**< Size of one symbol table entry */
#define DT_INIT		12		/**< Address of init function */
#define DT_FINI		13		/**< Address of termination function */
#define DT_SONAME	14		/**< Name of shared object */
#define DT_RPATH	15		/**< Library search path */
#define DT_SYMBOLIC	16		/**< Start symbol search here */
#define DT_REL		17		/**< Address of Rel relocs */
#define DT_RELSZ	18		/**< Total size of Rel relocs */
#define DT_RELENT	19		/**< Size of one Rel reloc */
#define DT_PLTREL	20		/**< Type of reloc in PLT */
#define DT_DEBUG	21		/**< For debugging; unspecified */
#define DT_TEXTREL	22		/**< Reloc might modify .text */
#define DT_JMPREL	23		/**< Address of PLT relocs */
#define	DT_NUM		24		/**< Number used */
#define DT_LOPROC	0x70000000UL	/**< Start of processor-specific */
#define DT_HIPROC	0x7fffffffUL	/**< End of processor-specific */
#define	DT_PROCNUM	DT_MIPS_NUM	/**< Most used by any processor */

/** The versioning entry types.  The next are defined as part of the
   GNU extension.  */
#define DT_VERSYM	0x6ffffff0UL

/** These were chosen by Sun.  */
#define	DT_VERDEF	0x6ffffffcUL	/**< Address of version definition
					   table */
#define	DT_VERDEFNUM	0x6ffffffdUL	/**< Number of version definitions */
#define	DT_VERNEED	0x6ffffffeUL	/**< Address of table with needed
					   versions */
#define	DT_VERNEEDNUM	0x6fffffffUL	/**< Number of needed versions */
#define DT_VERSIONTAGIDX(tag)	(DT_VERNEEDNUM - (tag))	/**< Reverse order! */
#define DT_VERSIONTAGNUM 16

/** Sun added these machine-independent extensions in the "processor-specific"
   range.  Be compatible.  */
#define DT_AUXILIARY    0x7ffffffdUL      /**< Shared object to load before self */
#define DT_FILTER       0x7fffffffUL      /**< Shared object to get values from */
#define DT_EXTRATAGIDX(tag)	((tByte)-((Elf386_Sword) (tag) <<1>>1)-1)
#define DT_EXTRANUM	3


/** These constants are used for the version number of a Elf386_Verdef
   structure.  */

#define VER_DEF_NONE		0
#define VER_DEF_CURRENT		1
#define VER_DEF_NUM	        2		/**< Given version number */

/** These constants appear in the vd_flags field of a Elf386_Verdef
   structure.  */

#define VER_FLG_BASE		0x1
#define VER_FLG_WEAK		0x2

/** These special constants can be found in an Elf386_Versym field.  */

#define VER_NDX_LOCAL		0
#define VER_NDX_GLOBAL		1

/** These constants are used for the version number of a Elf386_Verneed
   structure.  */

#define VER_NEED_NONE		0
#define VER_NEED_CURRENT	1
#define VER_NEED_NUM	        2		/**< Given version number */

/** This flag appears in a Versym structure.  It means that the symbol
   is hidden, and is only visible with an explicit version number.
   This is a GNU extension.  */

#define VERSYM_HIDDEN		0x8000U

/** This is the mask for the rest of the Versym information.  */

#define VERSYM_VERSION		0x7fffU

/** This is a special token which appears as part of a symbol name.  It
   indictes that the rest of the name is actually the name of a
   version node, and is not part of the actual name.  This is a GNU
   extension.  For example, the symbol name `stat@ver2' is taken to
   mean the symbol `stat' in version `ver2'.  */

#define ELF_VER_CHR	'@'


typedef struct {
  tUInt8	e_ident[16];		/**< ELF "magic number" */
  tUInt8	e_type[2];		/**< Identifies object file type */
  tUInt8	e_machine[2];		/**< Specifies required architecture */
  tUInt8	e_version[4];		/**< Identifies object file version */
  tUInt8	e_entry[4];		/**< Entry point virtual address */
  tUInt8	e_phoff[4];		/**< Program header table file offset */
  tUInt8	e_shoff[4];		/**< Section header table file offset */
  tUInt8	e_flags[4];		/**< Processor-specific flags */
  tUInt8	e_ehsize[2];		/**< ELF header size in bytes */
  tUInt8	e_phentsize[2];		/**< Program header table entry size */
  tUInt8	e_phnum[2];		/**< Program header table entry count */
  tUInt8	e_shentsize[2];		/**< Section header table entry size */
  tUInt8	e_shnum[2];		/**< Section header table entry count */
  tUInt8	e_shstrndx[2];		/**< Section header string table index */
} Elf386_External_Ehdr;

typedef struct {
  tUInt8	e_ident[16];		/**< ELF "magic number" */
  tUInt8	e_type[2];		/**< Identifies object file type */
  tUInt8	e_machine[2];		/**< Specifies required architecture */
  tUInt8	e_version[4];		/**< Identifies object file version */
  tUInt8	e_entry[8];		/**< Entry point virtual address */
  tUInt8	e_phoff[8];		/**< Program header table file offset */
  tUInt8	e_shoff[8];		/**< Section header table file offset */
  tUInt8	e_flags[4];		/**< Processor-specific flags */
  tUInt8	e_ehsize[2];		/**< ELF header size in bytes */
  tUInt8	e_phentsize[2];		/**< Program header table entry size */
  tUInt8	e_phnum[2];		/**< Program header table entry count */
  tUInt8	e_shentsize[2];		/**< Section header table entry size */
  tUInt8	e_shnum[2];		/**< Section header table entry count */
  tUInt8	e_shstrndx[2];		/**< Section header string table index */
} Elf64_External_Ehdr;

/** Program header */

typedef struct {
  tUInt8	p_type[4];		/**< Identifies program segment type */
  tUInt8	p_offset[4];		/**< Segment file offset */
  tUInt8	p_vaddr[4];		/**< Segment virtual address */
  tUInt8	p_paddr[4];		/**< Segment physical address (ignored on SystemV) */
  tUInt8	p_filesz[4];		/**< Segment size in file */
  tUInt8	p_memsz[4];		/**< Segment size in memory */
  tUInt8	p_flags[4];		/**< Segment flags */
  tUInt8	p_align[4];		/**< Segment alignment, file & memory */
} Elf386_External_Phdr;

typedef struct {
  tUInt8	p_type[4];		/**< Identifies program segment type */
  tUInt8	p_flags[4];		/**< Segment flags */
  tUInt8	p_offset[8];		/**< Segment file offset */
  tUInt8	p_vaddr[8];		/**< Segment virtual address */
  tUInt8	p_paddr[8];		/**< Segment physical address (ignored on SystemV)*/
  tUInt8	p_filesz[8];		/**< Segment size in file */
  tUInt8	p_memsz[8];		/**< Segment size in memory */
  tUInt8	p_align[8];		/**< Segment alignment, file & memory */
} Elf64_External_Phdr;

/** Section header */

typedef struct {
  tUInt8	sh_name[4];		/**< Section name, index in string tbl */
  tUInt8	sh_type[4];		/**< Type of section */
  tUInt8	sh_flags[4];		/**< Miscellaneous section attributes */
  tUInt8	sh_addr[4];		/**< Section virtual addr at execution */
  tUInt8	sh_offset[4];		/**< Section file offset */
  tUInt8	sh_size[4];		/**< Size of section in bytes */
  tUInt8	sh_link[4];		/**< Index of another section */
  tUInt8	sh_info[4];		/**< Additional section information */
  tUInt8	sh_addralign[4];	/**< Section alignment */
  tUInt8	sh_entsize[4];		/**< Entry size if section holds table */
} Elf386_External_Shdr;

typedef struct {
  tUInt8	sh_name[4];		/**< Section name, index in string tbl */
  tUInt8	sh_type[4];		/**< Type of section */
  tUInt8	sh_flags[8];		/**< Miscellaneous section attributes */
  tUInt8	sh_addr[8];		/**< Section virtual addr at execution */
  tUInt8	sh_offset[8];		/**< Section file offset */
  tUInt8	sh_size[8];		/**< Size of section in bytes */
  tUInt8	sh_link[4];		/**< Index of another section */
  tUInt8	sh_info[4];		/**< Additional section information */
  tUInt8	sh_addralign[8];	/**< Section alignment */
  tUInt8	sh_entsize[8];		/**< Entry size if section holds table */
} Elf64_External_Shdr;

/** Symbol table entry */

typedef struct {
  tUInt8	st_name[4];		/**< Symbol name, index in string tbl */
  tUInt8	st_value[4];		/**< Value of the symbol */
  tUInt8	st_size[4];		/**< Associated symbol size */
  tUInt8	st_info[1];		/**< Type and binding attributes */
  tUInt8	st_other[1];		/**< No defined meaning, 0 */
  tUInt8	st_shndx[2];		/**< Associated section index */
} Elf386_External_Sym;

typedef struct {
  tUInt8	st_name[4];		/**< Symbol name, index in string tbl */
  tUInt8	st_info[1];		/**< Type and binding attributes */
  tUInt8	st_other[1];		/**< No defined meaning, 0 */
  tUInt8	st_shndx[2];		/**< Associated section index */
  tUInt8	st_value[8];		/**< Value of the symbol */
  tUInt8	st_size[8];		/**< Associated symbol size */
} Elf64_External_Sym;

/** Note segments */

typedef struct {
  tUInt8	namesz[4];		/**< Size of entry's owner string */
  tUInt8	descsz[4];		/**< Size of the note descriptor */
  tUInt8	type[4];		/**< Interpretation of the descriptor */
  tInt8	name[1];		/**< Start of the name+desc data */
} Elf_External_Note;

/** Relocation Entries */
typedef struct {
  tUInt8	r_offset[4];	/**< Location at which to apply the action */
  tUInt8	r_info[4];	/**< index and type of relocation */
} Elf386_External_Rel;

typedef struct {
  tUInt8	r_offset[4];	/**< Location at which to apply the action */
  tUInt8	r_info[4];	/**< index and type of relocation */
  tUInt8	r_addend[4];	/**< Constant addend used to compute value */
} Elf386_External_Rela;

typedef struct {
  tUInt8	r_offset[8];	/**< Location at which to apply the action */
  tUInt8	r_info[8];	/**< index and type of relocation */
} Elf64_External_Rel;

typedef struct {
  tUInt8	r_offset[8];	/**< Location at which to apply the action */
  tUInt8	r_info[8];	/**< index and type of relocation */
  tUInt8	r_addend[8];	/**< Constant addend used to compute value */
} Elf64_External_Rela;

/** dynamic section structure */

typedef struct {
  tUInt8	d_tag[4];		/**< entry tag value */
  union {
    tUInt8	d_val[4];
    tUInt8	d_ptr[4];
  } d_un;
} Elf386_External_Dyn;

typedef struct {
  tUInt8	d_tag[8];		/**< entry tag value */
  union {
    tUInt8	d_val[8];
    tUInt8	d_ptr[8];
  } d_un;
} Elf64_External_Dyn;

/** The version structures are currently size independent.  They are
   named without a 32 or 64.  If that ever changes, these structures
   will need to be renamed.  */

/** This structure appears in a SHT_GNU_verdef section.  */

typedef struct {
  tUInt8		vd_version[2];
  tUInt8		vd_flags[2];
  tUInt8		vd_ndx[2];
  tUInt8		vd_cnt[2];
  tUInt8		vd_hash[4];
  tUInt8		vd_aux[4];
  tUInt8		vd_next[4];
} Elf_External_Verdef;

/** This structure appears in a SHT_GNU_verdef section.  */

typedef struct {
  tUInt8		vda_name[4];
  tUInt8		vda_next[4];
} Elf_External_Verdaux;

/** This structure appears in a SHT_GNU_verneed section.  */

typedef struct {
  tUInt8		vn_version[2];
  tUInt8		vn_cnt[2];
  tUInt8		vn_file[4];
  tUInt8		vn_aux[4];
  tUInt8		vn_next[4];
} Elf_External_Verneed;

/** This structure appears in a SHT_GNU_verneed section.  */

typedef struct {
  tUInt8		vna_hash[4];
  tUInt8		vna_flags[2];
  tUInt8		vna_other[2];
  tUInt8		vna_name[4];
  tUInt8		vna_next[4];
} Elf_External_Vernaux;

/** This structure appears in a SHT_GNU_versym section.  This is not a
   standard ELF structure; ELF just uses Elf386_Half.  */

typedef struct {
  tUInt8		vs_vers[2];
} Elf_External_Versym;

/** Auxiliary vector.  */

/** This vector is normally only used by the program interpreter.  The
   usual definition in an ABI supplement uses the name auxv_t.  The
   vector is not usually defined in a standard <elf.h> file, but it
   can't hurt.  We rename it to avoid conflicts.  The sizes of these
   types are an arrangement between the exec server and the program
   interpreter, so we don't fully specify them here.  */

typedef struct
{
  tInt32 a_type;		/**< Entry type */
  union
    {
      tInt32 a_val;		/**< Integer value */
      void *a_ptr;		/**< Pointer value */
      void (*a_fcn) (void);	/**< Function pointer value */
    } a_un;
} Elf386_auxv_t;

typedef struct
{
  tInt32 a_type;		/**< Entry type */
  union
    {
      tInt32 a_val;		/**< Integer value */
      void *a_ptr;		/**< Pointer value */
      void (*a_fcn) (void);	/**< Function pointer value */
    } a_un;
} Elf64_auxv_t;

/** Legal values for a_type (entry type).  */

#define AT_NULL		0		/**< End of vector */
#define AT_IGNORE	1		/**< Entry should be ignored */
#define AT_EXECFD	2		/**< File descriptor of program */
#define AT_PHDR		3		/**< Program headers for program */
#define AT_PHENT	4		/**< Size of program header entry */
#define AT_PHNUM	5		/**< Number of program headers */
#define AT_PAGESZ	6		/**< System page size */
#define AT_BASE		7		/**< Base address of interpreter */
#define AT_FLAGS	8		/**< Flags */
#define AT_ENTRY	9		/**< Entry point of program */
#define AT_NOTELF	10		/**< Program is not ELF */
#define AT_UID		11		/**< Real uid */
#define AT_EUID		12		/**< Effective uid */
#define AT_GID		13		/**< Real gid */
#define AT_EGID		14		/**< Effective gid */

/** Motorola 68k specific definitions.  */

/** m68k relocs.  */

#define R_68K_NONE	0		/**< No reloc */
#define R_68K_32	1		/**< Direct 32 bit  */
#define R_68K_16	2		/**< Direct 16 bit  */
#define R_68K_8		3		/**< Direct 8 bit  */
#define R_68K_PC32	4		/**< PC relative 32 bit */
#define R_68K_PC16	5		/**< PC relative 16 bit */
#define R_68K_PC8	6		/**< PC relative 8 bit */
#define R_68K_GOT32	7		/**< 32 bit PC relative GOT entry */
#define R_68K_GOT16	8		/**< 16 bit PC relative GOT entry */
#define R_68K_GOT8	9		/**< 8 bit PC relative GOT entry */
#define R_68K_GOT32O	10		/**< 32 bit GOT offset */
#define R_68K_GOT16O	11		/**< 16 bit GOT offset */
#define R_68K_GOT8O	12		/**< 8 bit GOT offset */
#define R_68K_PLT32	13		/**< 32 bit PC relative PLT address */
#define R_68K_PLT16	14		/**< 16 bit PC relative PLT address */
#define R_68K_PLT8	15		/**< 8 bit PC relative PLT address */
#define R_68K_PLT32O	16		/**< 32 bit PLT offset */
#define R_68K_PLT16O	17		/**< 16 bit PLT offset */
#define R_68K_PLT8O	18		/**< 8 bit PLT offset */
#define R_68K_COPY	19		/**< Copy symbol at runtime */
#define R_68K_GLOB_DAT	20		/**< Create GOT entry */
#define R_68K_JMP_SLOT	21		/**< Create PLT entry */
#define R_68K_RELATIVE	22		/**< Adjust by program base */
#define R_68K_NUM	23


/* PowerPC related declarations */
#define R_PPC_NONE		  0
#define R_PPC_ADDR32		  1
#define R_PPC_ADDR24		  2
#define R_PPC_ADDR16		  3
#define R_PPC_ADDR16_LO		  4
#define R_PPC_ADDR16_HI		  5
#define R_PPC_ADDR16_HA		  6
#define R_PPC_ADDR14		  7
#define R_PPC_ADDR14_BRTAKEN	  8
#define R_PPC_ADDR14_BRNTAKEN	  9
#define R_PPC_REL24		 10
#define R_PPC_REL14		 11
#define R_PPC_REL14_BRTAKEN	 12
#define R_PPC_REL14_BRNTAKEN	 13
#define R_PPC_GOT16		 14
#define R_PPC_GOT16_LO		 15
#define R_PPC_GOT16_HI		 16
#define R_PPC_GOT16_HA		 17
#define R_PPC_PLTREL24		 18
#define R_PPC_COPY		 19
#define R_PPC_GLOB_DAT		 20
#define R_PPC_JMP_SLOT		 21
#define R_PPC_RELATIVE		 22
#define R_PPC_LOCAL24PC		 23
#define R_PPC_UADDR32		 24
#define R_PPC_UADDR16		 25
#define R_PPC_REL32		 26
#define R_PPC_PLT32		 27
#define R_PPC_PLTREL32		 28
#define R_PPC_PLT16_LO		 29
#define R_PPC_PLT16_HI		 30
#define R_PPC_PLT16_HA		 31
#define R_PPC_SDAREL16		 32
#define R_PPC_SECTOFF		 33
#define R_PPC_SECTOFF_LO	 34
#define R_PPC_SECTOFF_HI	 35
#define R_PPC_SECTOFF_HA	 36
#define R_PPC_ADDR30		 37

#define R_PPC64_ADDR64		 38
#define R_PPC64_ADDR16_HIGHER	 39
#define R_PPC64_ADDR16_HIGHERA	 40
#define R_PPC64_ADDR16_HIGHEST	 41
#define R_PPC64_ADDR16_HIGHESTA  42
#define R_PPC64_UADDR64		 43
#define R_PPC64_REL64		 44
#define R_PPC64_PLT64		 45
#define R_PPC64_PLTREL64	 46
#define R_PPC64_TOC16		 47
#define R_PPC64_TOC16_LO	 48
#define R_PPC64_TOC16_HI	 49
#define R_PPC64_TOC16_HA	 50
#define R_PPC64_TOC		 51
#define R_PPC64_PLTGOT16	 52
#define R_PPC64_PLTGOT16_LO	 53
#define R_PPC64_PLTGOT16_HI	 54
#define R_PPC64_PLTGOT16_HA	 55

  /* The following relocs were added in the 64-bit PowerPC ELF ABI
     revision 1.2. */
#define R_PPC64_ADDR16_DS	 56
#define R_PPC64_ADDR16_LO_DS	 57
#define R_PPC64_GOT16_DS	 58
#define R_PPC64_GOT16_LO_DS	 59
#define R_PPC64_PLT16_LO_DS	 60
#define R_PPC64_SECTOFF_DS	 61
#define R_PPC64_SECTOFF_LO_DS	 62
#define R_PPC64_TOC16_DS	 63
#define R_PPC64_TOC16_LO_DS	 64
#define R_PPC64_PLTGOT16_DS	 65
#define R_PPC64_PLTGOT16_LO_DS	 66

  /* Relocs added to support TLS.  */
#define R_PPC_TLS		 67
#define R_PPC_DTPMOD32		 68
#define R_PPC_TPREL16		 69
#define R_PPC_TPREL16_LO	 70
#define R_PPC_TPREL16_HI	 71
#define R_PPC_TPREL16_HA	 72
#define R_PPC_TPREL32		 73
#define R_PPC_DTPREL16		 74
#define R_PPC_DTPREL16_LO	 75
#define R_PPC_DTPREL16_HI	 76
#define R_PPC_DTPREL16_HA	 77
#define R_PPC_DTPREL32		 78
#define R_PPC_GOT_TLSGD16	 79
#define R_PPC_GOT_TLSGD16_LO	 80
#define R_PPC_GOT_TLSGD16_HI	 81
#define R_PPC_GOT_TLSGD16_HA	 82
#define R_PPC_GOT_TLSLD16	 83
#define R_PPC_GOT_TLSLD16_LO	 84
#define R_PPC_GOT_TLSLD16_HI	 85
#define R_PPC_GOT_TLSLD16_HA	 86
#define R_PPC_GOT_TPREL16	 87
#define R_PPC_GOT_TPREL16_LO	 88
#define R_PPC_GOT_TPREL16_HI	 89
#define R_PPC_GOT_TPREL16_HA	 90
#define R_PPC_GOT_DTPREL16	 91
#define R_PPC_GOT_DTPREL16_LO	 92
#define R_PPC_GOT_DTPREL16_HI	 93
#define R_PPC_GOT_DTPREL16_HA	 94

/* The remaining relocs are from the Embedded ELF ABI and are not
   in the SVR4 ELF ABI.  */
#define R_PPC_EMB_NADDR32	101
#define R_PPC_EMB_NADDR16	102
#define R_PPC_EMB_NADDR16_LO	103
#define R_PPC_EMB_NADDR16_HI	104
#define R_PPC_EMB_NADDR16_HA	105
#define R_PPC_EMB_SDAI16	106
#define R_PPC_EMB_SDA2I16	107
#define R_PPC_EMB_SDA2REL	108
#define R_PPC_EMB_SDA21		109
#define R_PPC_EMB_MRKREF	110
#define R_PPC_EMB_RELSEC16	111
#define R_PPC_EMB_RELST_LO	112
#define R_PPC_EMB_RELST_HI	113
#define R_PPC_EMB_RELST_HA	114
#define R_PPC_EMB_BIT_FLD	115
#define R_PPC_EMB_RELSDA	116

/* Fake relocations for branch stubs only used internally by ld.  */
#define R_PPC_RELAX32		245
#define R_PPC_RELAX32PC		246
#define R_PPC_RELAX32_PLT	247
#define R_PPC_RELAX32PC_PLT	248

/* These are GNU extensions used in PIC code sequences.  */
#define R_PPC_REL16		249
#define R_PPC_REL16_LO		250
#define R_PPC_REL16_HI		251
#define R_PPC_REL16_HA		252

/* These are GNU extensions to enable C++ vtable garbage collection.  */
#define R_PPC_GNU_VTINHERIT	253
#define R_PPC_GNU_VTENTRY	254

/* This is a phony reloc to handle any old fashioned TOC16 references
   that may still be in object files.  */
#define R_PPC_TOC16		255



/** Intel 80386 specific definitions.  */
/** 386 relocs.  */

#define R_386_NONE	0		/**< No reloc */
#define R_386_32	1		/**< Direct 32 bit  */
#define R_386_PC32	2		/**< PC relative 32 bit */
#define R_386_GOT32	3		/**< 32 bit GOT entry */
#define R_386_PLT32	4		/**< 32 bit PLT address */
#define R_386_COPY	5		/**< Copy symbol at runtime */
#define R_386_GLOB_DAT	6		/**< Create GOT entry */
#define R_386_JMP_SLOT	7		/**< Create PLT entry */
#define R_386_RELATIVE	8		/**< Adjust by program base */
#define R_386_GOTOFF	9		/**< 32 bit offset to GOT */
#define R_386_GOTPC	10		/**< 32 bit PC relative offset to GOT */
#define R_386_NUM	11
#define R_386_GNU_16	20		/**< Direct 16 bit  */
#define R_386_GNU_PC16	21		/**< PC relative 16 bit */
#define R_386_GNU_8	22		/**< Direct 8 bit  */
#define R_386_GNU_PC8	23		/**< PC relative 8 bit */
#define R_386_GNU_max	24

/** AMD64 specific definitions */
#define R_X86_64_NONE	0	/* No reloc */
#define R_X86_64_64	1	/* Direct 64 bit  */
#define R_X86_64_PC32	2	/* PC relative 32 bit signed */
#define R_X86_64_GOT32	3	/* 32 bit GOT entry */
#define R_X86_64_PLT32	4	/* 32 bit PLT address */
#define R_X86_64_COPY	5	/* Copy symbol at runtime */
#define R_X86_64_GLOB_DAT 6	/* Create GOT entry */
#define R_X86_64_JUMP_SLOT 7	/* Create PLT entry */
#define R_X86_64_RELATIVE 8	/* Adjust by program base */
#define R_X86_64_GOTPCREL 9	/* 32 bit signed pc relative offset to GOT entry */
#define R_X86_64_32	10	/* Direct 32 bit zero extended */
#define R_X86_64_32S	11	/* Direct 32 bit sign extended */
#define R_X86_64_16	12	/* Direct 16 bit zero extended */
#define R_X86_64_PC16	13	/* 16 bit sign extended pc relative*/
#define R_X86_64_8	14	/* Direct 8 bit sign extended */
#define R_X86_64_PC8	15	/* 8 bit sign extended pc relative*/
#define R_X86_64_DTPMOD64 16	/* ID of module containing symbol */
#define R_X86_64_DTPOFF64 17	/* Offset in TLS block */
#define R_X86_64_TPOFF64  18	/* Offset in initial TLS block */
#define R_X86_64_TLSGD	19	/* PC relative offset to GD GOT block */
#define R_X86_64_TLSLD	20	/* PC relative offset to LD GOT block */
#define R_X86_64_DTPOFF32 21	/* Offset in TLS block */
#define R_X86_64_GOTTPOFF 22	/* PC relative offset to IE GOT entry */
#define R_X86_64_TPOFF32 23	/* Offset in initial TLS block */
#define R_X86_64_PC64	24	/* PC relative 64 bit */
#define R_X86_64_GOTOFF64 25	/* 64 bit offset to GOT */
#define R_X86_64_GOTPC32 26	/* 32 bit signed pc relative offset to GOT */
#define R_X86_64_GOT64	27	/* 64 bit GOT entry offset */
#define R_X86_64_GOTPCREL64 28	/* 64 bit signed pc relative offset to GOT entry */
#define R_X86_64_GOTPC64 29	/* 64 bit signed pc relative offset to GOT */
#define R_X86_64_GOTPLT64 30	/* like GOT64, but indicates that PLT entry is needed */
#define R_X86_64_PLTOFF64 31	/* 64 bit GOT relative offset to PLT entry */
     /* 32 .. 33 */
#define R_X86_64_GOTPC32_TLSDESC 34 /* 32 bit signed pc relative offset to TLS descriptor in the GOT.  */
#define R_X86_64_TLSDESC_CALL 35 /* Relaxable call through TLS descriptor.  */
#define R_X86_64_TLSDESC 36	/* 2x64-bit TLS descriptor.  */
#define R_X86_64_GNU_VTINHERIT 250 /* GNU C++ hack  */
#define R_X86_64_GNU_VTENTRY 251 /* GNU C++ hack  */


/** SUN SPARC specific definitions.  */

/** SPARC relocs.  */

#define R_SPARC_NONE	0		/**< No reloc */
#define R_SPARC_8	1		/**< Direct 8 bit */
#define R_SPARC_16	2		/**< Direct 16 bit */
#define R_SPARC_32	3		/**< Direct 32 bit */
#define R_SPARC_DISP8	4		/**< PC relative 8 bit */
#define R_SPARC_DISP16	5		/**< PC relative 16 bit */
#define R_SPARC_DISP32	6		/**< PC relative 32 bit */
#define R_SPARC_WDISP30	7		/**< PC relative 30 bit shifted */
#define R_SPARC_WDISP22	8		/**< PC relative 22 bit shifted */
#define R_SPARC_HI22	9		/**< High 22 bit */
#define R_SPARC_22	10		/**< Direct 22 bit */
#define R_SPARC_13	11		/**< Direct 13 bit */
#define R_SPARC_LO10	12		/**< Truncated 10 bit */
#define R_SPARC_GOT10	13		/**< Truncated 10 bit GOT entry */
#define R_SPARC_GOT13	14		/**< 13 bit GOT entry */
#define R_SPARC_GOT22	15		/**< 22 bit GOT entry shifted */
#define R_SPARC_PC10	16		/**< PC relative 10 bit truncated */
#define R_SPARC_PC22	17		/**< PC relative 22 bit shifted */
#define R_SPARC_WPLT30	18		/**< 30 bit PC relative PLT address */
#define R_SPARC_COPY	19		/**< Copy symbol at runtime */
#define R_SPARC_GLOB_DAT 20		/**< Create GOT entry */
#define R_SPARC_JMP_SLOT 21		/**< Create PLT entry */
#define R_SPARC_RELATIVE 22		/**< Adjust by program base */
#define R_SPARC_UA32	23		/**< Direct 32 bit unaligned */
#define R_SPARC_NUM	24

/** MIPS R3000 specific definitions.  */

/** Legal values for e_flags field of Elf386_Ehdr.  */

#define EF_MIPS_NOREORDER 1		/**< A .noreorder directive was used */
#define EF_MIPS_PIC	  2		/**< Contains PIC code */
#define EF_MIPS_CPIC	  4		/**< Uses PIC calling sequence */
#define EF_MIPS_ARCH	  0xf0000000U	/**< MIPS architecture level */

/** Legal values for MIPS architecture level.  */

#define E_MIPS_ARCH_1	  0x00000000UL	/**< -mips1 code.  */
#define E_MIPS_ARCH_2	  0x10000000UL	/**< -mips2 code.  */
#define E_MIPS_ARCH_3	  0x20000000UL	/**< -mips3 code.  */

/** Special section indices.  */

#define SHN_MIPS_ACOMMON 0xff00		/**< Allocated common symbols */
#define SHN_MIPS_TEXT	 0xff01		/**< Allocated test symbols.  */
#define SHN_MIPS_DATA	 0xff02		/**< Allocated data symbols.  */
#define SHN_MIPS_SCOMMON 0xff03		/**< Small common symbols */
#define SHN_MIPS_SUNDEFINED 0xff04	/**< Small undefined symbols */

/** Legal values for sh_type field of Elf386_Shdr.  */

#define SHT_MIPS_LIBLIST  0x70000000UL	/**< Shared objects used in link */
#define SHT_MIPS_CONFLICT 0x70000002UL	/**< Conflicting symbols */
#define SHT_MIPS_GPTAB	  0x70000003UL	/**< Global data area sizes */
#define SHT_MIPS_UCODE	  0x70000004UL	/**< Reserved for SGI/MIPS compilers */
#define SHT_MIPS_DEBUG	  0x70000005UL	/**< MIPS ECOFF debugging information */
#define SHT_MIPS_REGINFO  0x70000006UL	/**< Register usage information */
#define SHT_MIPS_OPTIONS  0x7000000dUL	/**< Miscellaneous options.  */
#define SHT_MIPS_DWARF    0x7000001eUL	/**< DWARF debugging information.  */
#define SHT_MIPS_EVENTS	  0x70000021UL	/**< Event section.  */

/** Legal values for sh_flags field of Elf386_Shdr.  */

#define SHF_MIPS_GPREL	0x10000000UL	/**< Must be part of global data area */

/** Entries found in sections of type SHT_MIPS_GPTAB.  */

typedef union
{
  struct
    {
      tUInt8 gt_current_g_value[4];	/**< -G value used for compilation */
      tUInt8 gt_unused[4];		/**< Not used */
    } gt_header;        	   	/**< First entry in section */
  struct
    {
      tUInt8 gt_g_value[4];		/**< If this value were used for -G */
      tUInt8 gt_bytes[4];		/**< This many bytes would be used */
    } gt_entry;				/**< Subsequent entries in section */
} Elf386_gptab;

/** Entry found in sections of type SHT_MIPS_REGINFO.  */

typedef struct
{
  tUInt8	ri_gprmask[4];		/**< General registers used */
  tUInt8	ri_cprmask[4][4];	/**< Coprocessor registers used */
  tUInt8	ri_gp_value[4];		/**< $gp register value */
} Elf386_RegInfo;

/** MIPS relocs.  */

#define R_MIPS_NONE	0		/**< No reloc */
#define R_MIPS_16	1		/**< Direct 16 bit */
#define R_MIPS_32	2		/**< Direct 32 bit */
#define R_MIPS_REL32	3		/**< PC relative 32 bit */
#define R_MIPS_26	4		/**< Direct 26 bit shifted */
#define R_MIPS_HI16	5		/**< High 16 bit */
#define R_MIPS_LO16	6		/**< Low 16 bit */
#define R_MIPS_GPREL16	7		/**< GP relative 16 bit */
#define R_MIPS_LITERAL	8		/**< 16 bit literal entry */
#define R_MIPS_GOT16	9		/**< 16 bit GOT entry */
#define R_MIPS_PC16	10		/**< PC relative 16 bit */
#define R_MIPS_CALL16	11		/**< 16 bit GOT entry for function */
#define R_MIPS_GPREL32	12		/**< GP relative 32 bit */
#define R_MIPS_NUM	13

/** Legal values for p_type field of Elf386_Phdr.  */

#define PT_MIPS_REGINFO	0x70000000	/**< Register usage information */

/** Legal values for d_tag field of Elf386_Dyn.  */

#define DT_MIPS_RLD_VERSION  0x70000001UL /**< Runtime linker interface version */
#define DT_MIPS_TIME_STAMP   0x70000002UL /**< Timestamp */
#define DT_MIPS_ICHECKSUM    0x70000003UL /**< Checksum */
#define DT_MIPS_IVERSION     0x70000004UL /**< Version string (string tbl index) */
#define DT_MIPS_FLAGS	     0x70000005UL /**< Flags */
#define DT_MIPS_BASE_ADDRESS 0x70000006UL /**< Base address */
#define DT_MIPS_CONFLICT     0x70000008UL /**< Address of CONFLICT section */
#define DT_MIPS_LIBLIST	     0x70000009UL /**< Address of LIBLIST section */
#define DT_MIPS_LOCAL_GOTNO  0x7000000aUL /**< Number of local GOT entries */
#define DT_MIPS_CONFLICTNO   0x7000000bUL /**< Number of CONFLICT entries */
#define DT_MIPS_LIBLISTNO    0x70000010UL /**< Number of LIBLIST entries */
#define DT_MIPS_SYMTABNO     0x70000011UL /**< Number of DYNSYM entries */
#define DT_MIPS_UNREFEXTNO   0x70000012UL /**< First external DYNSYM */
#define DT_MIPS_GOTSYM	     0x70000013UL /**< First GOT entry in DYNSYM */
#define DT_MIPS_HIPAGENO     0x70000014UL /**< Number of GOT page table entries */
#define DT_MIPS_RLD_MAP	     0x70000016UL /**< Address of run time loader map.  */
#define DT_MIPS_NUM	     0x17

/** Legal values for DT_MIPS_FLAG Elf386_Dyn entry.  */

#define RHF_NONE		   0		/**< No flags */
#define RHF_QUICKSTART		   (1 << 0)	/**< Use quickstart */
#define RHF_NOTPOT		   (1 << 1)	/**< Hash size not power of 2 */
#define RHF_NO_LIBRARY_REPLACEMENT (1 << 2)	/**< Ignore LD_LIBRARY_PATH */

/** Entries found in sections of type SHT_MIPS_LIBLIST.  */

typedef struct
{
  tUInt8	l_name[4];		/**< Name (string table index) */
  tUInt8	l_time_stamp[4];	/**< Timestamp */
  tUInt8	l_checksum[4];		/**< Checksum */
  tUInt8	l_version[4];		/**< Interface version */
  tUInt8	l_flags[4];		/**< Flags */
} Elf386_Lib;

/** Legal values for l_flags.  */

#define LL_EXACT_MATCH	  (1 << 0)	/**< Require exact match */
#define LL_IGNORE_INT_VER (1 << 1)	/**< Ignore interface version */

/** HPPA specific definitions.  */

/** Legal values for sh_type field of Elf386_Shdr.  */

#define SHT_PARISC_GOT		0x70000000UL /**< GOT for external data.  */
#define SHT_PARISC_ARCH		0x70000001UL /**< Architecture extensions.  */
#define SHT_PARISC_GLOBAL	0x70000002UL /**< Definition of $global$.  */
#define SHT_PARISC_MILLI	0x70000003UL /**< Millicode routines.  */
#define SHT_PARISC_UNWIND	0x70000004UL /**< Unwind information.  */
#define SHT_PARISC_PLT		0x70000005UL /**< Procedure linkage table.  */
#define SHT_PARISC_SDATA	0x70000006UL /**< Short initialized data.  */
#define SHT_PARISC_SBSS		0x70000007UL /**< Short uninitialized data.  */
#define SHT_PARISC_SYMEXTN	0x70000008UL /**< Argument/relocation info.  */
#define SHT_PARISC_STUBS	0x70000009UL /**< Linker stubs.  */

/** Legal values for sh_flags field of Elf386_Shdr.  */

#define SHF_PARISC_SHORT	0x20000000UL /**< Section with short addressing. */

/** Legal values for ST_TYPE subfield of st_info (symbol type).  */

#define STT_PARISC_MILLICODE	13	/**< Millicode function entry point.  */


/** Alpha specific declarations.  */

/** Alpha relocs.  */

#define R_ALPHA_NONE		0	/**< No reloc */
#define R_ALPHA_REFLONG		1	/**< Direct 32 bit */
#define R_ALPHA_REFQUAD		2	/**< Direct 64 bit */
#define R_ALPHA_GPREL32		3	/**< GP relative 32 bit */
#define R_ALPHA_LITERAL		4	/**< GP relative 16 bit w/optimization */
#define R_ALPHA_LITUSE		5	/**< Optimization hint for LITERAL */
#define R_ALPHA_GPDISP		6	/**< Add displacement to GP */
#define R_ALPHA_BRADDR		7	/**< PC+4 relative 23 bit shifted */
#define R_ALPHA_HINT		8	/**< PC+4 relative 16 bit shifted */
#define R_ALPHA_SREL16		9	/**< PC relative 16 bit */
#define R_ALPHA_SREL32		10	/**< PC relative 32 bit */
#define R_ALPHA_SREL64		11	/**< PC relative 64 bit */
#define R_ALPHA_OP_PUSH		12	/**< OP stack push */
#define R_ALPHA_OP_STORE	13	/**< OP stack pop and store */
#define R_ALPHA_OP_PSUB		14	/**< OP stack subtract */
#define R_ALPHA_OP_PRSHIFT	15	/**< OP stack right shift */
#define R_ALPHA_GPVALUE		16
#define R_ALPHA_GPRELHIGH	17
#define R_ALPHA_GPRELLOW	18
#define R_ALPHA_IMMED_GP_16	19
#define R_ALPHA_IMMED_GP_HI32	20
#define R_ALPHA_IMMED_SCN_HI32	21
#define R_ALPHA_IMMED_BR_HI32	22
#define R_ALPHA_IMMED_LO32	23
#define R_ALPHA_COPY		24	/**< Copy symbol at runtime */
#define R_ALPHA_GLOB_DAT	25	/**< Create GOT entry */
#define R_ALPHA_JMP_SLOT	26	/**< Create PLT entry */
#define R_ALPHA_RELATIVE	27	/**< Adjust by program base */
#define R_ALPHA_NUM		28

/* ARM soecific declaratoins */
#define R_ARM_NONE		  0
#define R_ARM_PC24		  1   /* deprecated */
#define R_ARM_ABS32		  2
#define R_ARM_REL32		  3
#define R_ARM_LDR_PC_G0		  4
#define R_ARM_ABS16		  5
#define R_ARM_ABS12		  6
#define R_ARM_THM_ABS5		  7
#define R_ARM_ABS8		  8
#define R_ARM_SBREL32		  9
#define R_ARM_THM_CALL		 10
#define R_ARM_THM_PC8		 11
#define R_ARM_BREL_ADJ		 12
#define R_ARM_SWI24		 13   /* obsolete */
#define R_ARM_THM_SWI8		 14   /* obsolete */
#define R_ARM_XPC25		 15   /* obsolete */
#define R_ARM_THM_XPC22		 16   /* obsolete */
#define R_ARM_TLS_DTPMOD32	 17
#define R_ARM_TLS_DTPOFF32	 18
#define R_ARM_TLS_TPOFF32	 19
#define R_ARM_COPY		 20   /* Copy symbol at runtime.  */
#define R_ARM_GLOB_DAT		 21   /* Create GOT entry.  */
#define R_ARM_JUMP_SLOT		 22   /* Create PLT entry.  */
#define R_ARM_RELATIVE		 23   /* Adjust by program base.  */
#define R_ARM_GOTOFF32		 24   /* 32 bit offset to GOT.  */
#define R_ARM_BASE_PREL		 25   /* 32 bit PC relative offset to GOT.  */
#define R_ARM_GOT_BREL		 26   /* 32 bit GOT entry.  */
#define R_ARM_PLT32		 27   /* deprecated - 32 bit PLT address.  */
#define R_ARM_CALL		 28
#define R_ARM_JUMP24		 29
#define R_ARM_THM_JUMP24	 30
#define R_ARM_BASE_ABS		 31
#define R_ARM_ALU_PCREL7_0	 32   /* obsolete */
#define R_ARM_ALU_PCREL15_8	 33   /* obsolete */
#define R_ARM_ALU_PCREL23_15	 34   /* obsolete */
#define R_ARM_LDR_SBREL_11_0	 35   /* deprecated should have _NC suffix */
#define R_ARM_ALU_SBREL_19_12	 36   /* deprecated should have _NC suffix */
#define R_ARM_ALU_SBREL_27_20	 37   /* deprecated should have _CK suffix */
#define R_ARM_TARGET1		 38
#define R_ARM_SBREL31		 39   /* deprecated */
#define R_ARM_V4BX		 40
#define R_ARM_TARGET2		 41
#define R_ARM_PREL31		 42
#define R_ARM_MOVW_ABS_NC	 43
#define R_ARM_MOVT_ABS		 44
#define R_ARM_MOVW_PREL_NC	 45
#define R_ARM_MOVT_PREL		 46
#define R_ARM_THM_MOVW_ABS_NC	 47
#define R_ARM_THM_MOVT_ABS	 48
#define R_ARM_THM_MOVW_PREL_NC	 49
#define R_ARM_THM_MOVT_PREL	 50
#define R_ARM_THM_JUMP19	 51
#define R_ARM_THM_JUMP6		 52
#define R_ARM_THM_ALU_PREL_11_0 53
#define R_ARM_THM_PC12		 54
#define R_ARM_ABS32_NOI		 55
#define R_ARM_REL32_NOI		 56
#define R_ARM_ALU_PC_G0_NC	 57
#define R_ARM_ALU_PC_G0		 58
#define R_ARM_ALU_PC_G1_NC	 59
#define R_ARM_ALU_PC_G1		 60
#define R_ARM_ALU_PC_G2		 61
#define R_ARM_LDR_PC_G1		 62
#define R_ARM_LDR_PC_G2		 63
#define R_ARM_LDRS_PC_G0	 64
#define R_ARM_LDRS_PC_G1	 65
#define R_ARM_LDRS_PC_G2	 66
#define R_ARM_LDC_PC_G0		 67
#define R_ARM_LDC_PC_G1		 68
#define R_ARM_LDC_PC_G2		 69
#define R_ARM_ALU_SB_G0_NC	 70
#define R_ARM_ALU_SB_G0		 71
#define R_ARM_ALU_SB_G1_NC	 72
#define R_ARM_ALU_SB_G1		 73
#define R_ARM_ALU_SB_G2		 74
#define R_ARM_LDR_SB_G0		 75
#define R_ARM_LDR_SB_G1		 76
#define R_ARM_LDR_SB_G2		 77
#define R_ARM_LDRS_SB_G0	 78
#define R_ARM_LDRS_SB_G1	 79
#define R_ARM_LDRS_SB_G2	 80
#define R_ARM_LDC_SB_G0		 81
#define R_ARM_LDC_SB_G1		 82
#define R_ARM_LDC_SB_G2		 83
#define R_ARM_MOVW_BREL_NC	 84
#define R_ARM_MOVT_BREL		 85
#define R_ARM_MOVW_BREL		 86
#define R_ARM_THM_MOVW_BREL_NC	 87
#define R_ARM_THM_MOVT_BREL	 88
#define R_ARM_THM_MOVW_BREL	 89
  /* 90-93 unallocated */
#define R_ARM_PLT32_ABS		 94
#define R_ARM_GOT_ABS		 95
#define R_ARM_GOT_PREL		 96
#define R_ARM_GOT_BREL12	 97
#define R_ARM_GOTOFF12		 98
#define R_ARM_GOTRELAX		 99
#define R_ARM_GNU_VTENTRY	100   /* deprecated - old C++ abi */
#define R_ARM_GNU_VTINHERIT	101   /* deprecated - old C++ abi */
#define R_ARM_THM_JUMP11	102
#define R_ARM_THM_JUMP8		103
#define R_ARM_TLS_GD32		104
#define R_ARM_TLS_LDM32		105
#define R_ARM_TLS_LDO32		106
#define R_ARM_TLS_IE32		107
#define R_ARM_TLS_LE32		108
#define R_ARM_TLS_LDO12		109
#define R_ARM_TLS_LE12		110
#define R_ARM_TLS_IE12GP	111
  /* 112 - 127 private range */
#define R_ARM_ME_TOO		128   /* obsolete */

  /* Extensions?  R=read-only?  */
#define R_ARM_RXPC25		249
#define R_ARM_RSBREL32		250
#define R_ARM_THM_RPC22		251
#define R_ARM_RREL32		252
#define R_ARM_RABS32		253
#define R_ARM_RPC24		254
#define R_ARM_RBASE		255



#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif

#endif
