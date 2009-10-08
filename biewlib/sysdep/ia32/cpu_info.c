/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/cpu_info.c
 * @brief       This file contains function for retrieving CPU information for
 *              32-bit Intel x86 compatible platform
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 * @remark      I used such form of this file because of build-in assembler
 *              allow write calling convention independed code. In addition,
 *              GNU C compiler is ported under multiple OS's. If somebody will
 *              port it under ABC-xyz platform, then more easy find compiler
 *              with build-in assembler, instead rewriting of makefile with .s
 *              (or .asm) extensions for choosen development system.
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <stdio.h>
#include <string.h>

#include "biewlib/biewlib.h"

#define CPU_CLONE     0x000F
#define __HAVE_FPU    0x8000
#define __HAVE_CPUID  0x4000
#define __HAVE_MMX    0x2000
#define __HAVE_SSE    0x1000

#if !defined(__DISABLE_ASM) && (defined(__GNUC__) && defined(NDEBUG))

static unsigned __NEAR__ __FASTCALL__ __cpu_type( void )
{
  register unsigned retval;
  __asm __volatile(
      "	pushl	%%esp\n"
      "	pushfl\n"
      "	movl	%%esp, %%edx\n"
      "	andl	$~3, %%esp\n"
      " pushfl\n"
      "	popl	%0\n"
      "	movl	%0, %%ecx\n"
      "	xorl	$0x40000, %0\n"
      "	pushl	%0\n"
      "	popfl\n"
      "	pushfl\n"
      "	popl	%0\n"
      "	xorl	%%ecx, %0\n"
      "	shrl	$0x12, %0\n"
      "	andl	$1, %0\n"
      "	pushl	%%ecx\n"
      "	popfl\n"
      "	movl	%%edx, %%esp\n"
      "	cmpl	$0, %0\n"
      "	jnz	4f\n"
      "	movl	$3, %0\n"
      "	jmp	1f\n"
"4:\n"
      "	pushfl\n"
      "	popl	%0\n"
      "	movl	%0, %%ecx\n"
      "	xorl	$0x200000, %0\n"
      "	pushl	%0\n"
      "	popfl\n"
      "	pushfl\n"
      "	popl	%0\n"
      "	xorl	%%ecx, %0\n"
      "	jnz	5f\n"
      "	movl	$4, %0\n"
      "	jmp	1f\n"
"5:\n"
      "	movl	$1, %0\n"
      ".short	0xA20F\n" /* cpuid */
      "	movb	%h0, %b0\n"
      "	andl	$0x0F, %0\n"
      "	orl	$0x4000, %0\n"
      "	testl   $0x800000, %%edx\n"
      "	jz	0f\n"
      "	orl	$0x2000, %0\n"
"0:\n"
      "	testl   $0x2000000, %%edx\n"
      "	jz	1f\n"
      "	orl	$0x1000, %0\n"
"1:\n"
      "	popfl\n"
      "	popl	%%esp"	: /* end assembler block */
      "=a"(retval)	: /* means: return through eax */
      "0"(3)		: /* means: initialize eax with 3 */
      "ecx","edx","ebx"); /* means: modified registers: ecx, edx, (after cpuid: ebx) */
   return retval;
}

static void __NEAR__ __FASTCALL__ __cpu_name(char *buff)
{
  __asm __volatile("xorl	%%eax, %%eax\n"
      "	.short	0xA20F\n" /* cpuid */
      "	movl	%%ebx, %%eax\n"
      "	stosl\n"
      "	movl	%%edx, %%eax\n"
      "	stosl\n"
      "	movl	%%ecx, %%eax\n"
      "	stosl\n"
      "	xorb	%%al, %%al\n"
      "	stosb\n"	:
                	:
      "D"(buff)		: /* assume es == ds */
      "eax","ebx","ecx","edx");
}

static void __NEAR__ __FASTCALL__ __extended_name(char *buff)
{
   __asm __volatile("movl	$0x80000002, %%eax\n"
      ".short	0xA20F\n" /* cpuid */
      "	stosl\n"
      "	movl	%%ebx, %%eax\n"
      "	stosl\n"
      "	movl	%%ecx, %%eax\n"
      "	stosl\n"
      "	movl	%%edx, %%eax\n"
      "	stosl\n"
      "	movl	$0x80000003, %%eax\n"
      ".short	0xA20F\n" /* cpuid */
      "	stosl\n"
      "	movl	%%ebx, %%eax\n"
      "	stosl\n"
      "	movl	%%ecx, %%eax\n"
      "	stosl\n"
      "	movl	%%edx, %%eax\n"
      "	stosl\n"
      "	movl	$0x80000004, %%eax\n"
      ".short	0xA20F\n" /* cpuid */
      "	stosl\n"
      "	movl	%%ebx, %%eax\n"
      "	stosl\n"
      "	movl	%%ecx, %%eax\n"
      "	stosl\n"
      "	movl	%%edx, %%eax\n"
      "	stosl\n"
      "	xorb	%%al, %%al\n"
      "	stosb\n"	:
                   	:
      "D"(buff)		: /* assume es == ds */
      "eax","ebx","ecx","edx");
}

static unsigned long __NEAR__ __FASTCALL__ __cpuid_edx(unsigned long *__r_eax)
{
  register unsigned long r_eax,r_edx,r_ecx,r_ebx;
  r_eax=*__r_eax;
   __asm __volatile(
	".short	0xA20F": /* cpuid */
	"=a"(r_eax),"=d"(r_edx),"=b"(r_ebx),"=c"(r_ecx):
	"0"(r_eax));
  *__r_eax=r_eax;
  return r_edx;
}

static unsigned long __NEAR__ __FASTCALL__ __cpuid_ebxecx(unsigned long *__r_eax)
{
  register unsigned long r_eax,r_edx,r_ecx,r_ebx;
  r_eax=*__r_eax;
   __asm __volatile(
	".short	0xA20F": /* cpuid */
	"=a"(r_eax),"=d"(r_edx),"=b"(r_ebx),"=c"(r_ecx):
	"0"(r_eax));
  *__r_eax=r_ecx;
  return r_ebx;
}

static unsigned __NEAR__ __FASTCALL__ __fpu_type( void )
{
  unsigned __cw;
  register unsigned retval;
   __asm __volatile("fninit\n"                     /* initialize 80387 (nowait) */
      "	movl	$0x20, %%ecx\n"
"1:\n"
      "	loop	1b\n"                 /* wait for it to complete */
      "	fnstcw	%1\n"                 /* store control word */
      "	movl	$0x10, %%ecx\n"
"2:\n"
      "	loop	2b\n"                 /* wait for it to complete */
/* Determine if we have an FPU */
      "	movl	%1, %%eax\n"
      "	andb	$0x0F, %%ah\n"
      "	cmpb	$0x03, %%ah\n"
      "	jnz	3f\n"                 /* no 80387 FPU found */
      "	movl	$3, %0\n"
      "	jmp	4f\n"
"3:\n"
      "	xorl	%0, %0\n"
"4:\n"				:
      "=r"(retval)		:
      "m"(__cw)			:
      "eax","ecx");
   return retval;
}

static unsigned long __NEAR__ __FASTCALL__ __OPS_nop(volatile unsigned *time_val)
{
  register unsigned long retval;
   __asm __volatile(
"1:\n"
      "	cmpl	$0, (%1)\n"
      "	jz	1b\n"
"2:\n"
      "	cmpl	$0, (%1)\n"
      "	jz	3f\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
".byte  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90\n"
      "	inc	%0\n"
      "	jmp	2b\n"
"3:"			:
      "=a"(retval)	:
      "r"(time_val),
      "0"(0));
  return retval;
}

static unsigned long __NEAR__ __FASTCALL__ __OPS_std(volatile unsigned *counter,char *arr8byte)
{
  register unsigned long retval;
   __asm __volatile("xorl	%0, %0\n"
"1:\n"
      "	cmpl	$0, (%1)\n"
      "	jz	1b\n"
"2:\n"
      "	cmpl	$0, (%1)\n"
      "	jnz	3f\n"
      "	jmp	4f\n"
"3:\n"
      "	pushl	%0\n"

      "	movl	$0x14, %%eax\n"
      "	movl	$0x07, %%ecx\n"
      "	mull	%%ecx\n"
      "	imull	%%ecx\n"
      "	divl	%%ecx\n"
      "	idivl	%%ecx\n"
      "	addl	%%ecx, %%eax\n"
      "	adcl	$0x01, %%eax\n"
      "	subl	%%ecx, %%eax\n"
      "	sbbl	$0x01, %%eax\n"
      "	pushl	%%esi\n"
      "	pushl	%%edi\n"
      "	movl	%2, %%esi\n"
      "	movsl\n"
      "	call	5f\n"
      "	cmpsl\n"
      "	popl	%%edi\n"
      "	popl	%%esi\n"
      "	pushl	%%eax\n"
      "	pushl	%%edx\n"
      "	popl	%%edx\n"
      "	popl	%%eax\n"
      "	movl	$0x14, %%eax\n"
      "	movl	$0x07, %%ecx\n"
      "	mull	%%ecx\n"
      "	imull	%%ecx\n"
      "	divl	%%ecx\n"
      "	idivl	%%ecx\n"
      "	addl	%%ecx, %%eax\n"
      "	adcl	$0x01, %%eax\n"
      "	subl	%%ecx, %%eax\n"
      "	sbbl	$0x01, %%eax\n"
      "	pushl	%%esi\n"
      "	pushl	%%edi\n"
      "	movl	%2, %%esi\n"
      "	movsl\n"
      "	call	5f\n"
      "	cmpsl\n"
      "	popl	%%edi\n"
      "	popl	%%esi\n"
      "	pushl	%%eax\n"
      "	pushl	%%edx\n"
      "	popl	%%edx\n"
      "	popl	%%eax\n"
      "	movl	$0x14, %%eax\n"
      "	movl	$0x07, %%ecx\n"
      "	mull	%%ecx\n"
      "	imull	%%ecx\n"
      "	divl	%%ecx\n"
      "	idivl	%%ecx\n"
      "	addl	%%ecx, %%eax\n"
      "	adcl	$0x01, %%eax\n"
      "	subl	%%ecx, %%eax\n"
      "	sbbl	$0x01, %%eax\n"
      "	pushl	%%esi\n"
      "	pushl	%%edi\n"
      "	movl	%2, %%esi\n"
      "	movsl\n"
      "	call	5f\n"
      "	cmpsl\n"
      "	popl	%%edi\n"
      "	popl	%%esi\n"
      "	pushl	%%eax\n"
      "	pushl	%%edx\n"
      "	popl	%%edx\n"
      "	popl	%%eax\n"
      "	movl	$0x14, %%eax\n"
      "	movl	$0x07, %%ecx\n"
      "	mull	%%ecx\n"
      "	imull	%%ecx\n"
      "	divl	%%ecx\n"
      "	idivl	%%ecx\n"
      "	adcl	$0x01, %%eax\n"
      "	subl	%%ecx, %%eax\n"
      "	sbbl	$0x01, %%eax\n"
      "	pushl	%%edx\n"
      "	popl	%%edx\n"

      "	popl	%0\n"
      "	incl	%0\n"
      "	jmp	2b\n"
"5:	ret\n"
"4:\n"			:
       "=a"(retval)	:
       "S"(counter),
       "D"(arr8byte)    :
       "edx","ecx");
  return retval;
}

static unsigned long __NEAR__ __FASTCALL__ __FOPS_nowait(volatile unsigned *counter,char *arr18bytes)
{
  register unsigned long retval;
   __asm __volatile("xorl	%0, %0\n"
"1:\n"
      "	cmpl	$0, (%1)\n"
      "	jz	1b\n"
"2:\n"
      "	cmpl	$0, (%1)\n"
      "	jz	3f\n"

      "	fninit\n"
      "	fldt	8(%2)\n"
      "	fstpt	8(%2)\n"
      "	fstp	%%st(1)\n"
      "	fldz\n"
      "	fld1\n"
      "	fcompp\n"
      "	fnstsw	4(%2)\n"
      "	fnstcw	(%2)\n"
      "	fldcw	(%2)\n"
      "	fldpi\n"
      "	fstp	%%st(1)\n"
      "	fst	%%st(2)\n"
      "	fst	%%st(3)\n"
      "	f2xm1\n"
      "	fabs\n"
      "	fchs\n"
      "	fprem\n"
      "	fptan\n"
      "	fsqrt\n"
      "	frndint\n"
      "	faddp	%%st,%%st(1)\n"
      "	fstp	%%st(1)\n"
      "	fmulp	%%st,%%st(1)\n"
      "	fstp	%%st(1)\n"
      "	fld1\n"
      "	fstp	%%st(1)\n"
      "	fpatan\n"
      "	fstp	%%st(1)\n"
      "	fscale\n"
      "	fstp	%%st(1)\n"
      "	fdivrp	%%st, %%st(1)\n"
      "	fstp	%%st(1)\n"
      "	fsubp	%%st, %%st(1)\n"
      "	fstp	%%st(1)\n"
      "	fyl2x\n"
      "	fstp	%%st(1)\n"
      "	fyl2xp1\n"
      "	fstp	%%st(1)\n"
      "	fbld	8(%2)\n"
      "	fbstp	8(%2)\n"
      "	fild	(%2)\n"
      "	fistp	(%2)\n"
      "	fldt	4(%2)\n"
      "	fstpt	4(%2)\n"
      "	fstp	%%st(1)\n"
      "	fldz\n"
      "	fld1\n"
      "	fcompp\n"
      "	fnstsw	4(%2)\n"
      "	fnstcw	(%2)\n"
      "	fldcw	(%2)\n"
      "	fldpi\n"
      "	fstp	%%st(1)\n"
      "	fst	%%st(2)\n"
      "	fst	%%st(3)\n"
      "	f2xm1\n"
      "	fabs\n"
      "	fchs\n"
      "	fprem\n"
      "	fptan\n"
      "	fsqrt\n"
      "	frndint\n"
      "	faddp	%%st,%%st(1)\n"
      "	fstp	%%st(1)\n"
      "	fmulp	%%st,%%st(1)\n"
      "	fstp	%%st(1)\n"
      "	fld1\n"
      "	fstp	%%st(1)\n"
      "	fpatan\n"
      "	fstp	%%st(1)\n"
      "	fscale\n"
      "	fstp	%%st(1)\n"
      "	fdivrp	%%st, %%st(1)\n"
      "	fstp	%%st(1)\n"
      "	fsubp	%%st, %%st(1)\n"
      "	fstp	%%st(1)\n"
      "	fyl2x\n"
      "	fstp	%%st(1)\n"
      "	fyl2xp1\n"
      "	fstp	%%st(1)\n"
      "	fild	(%2)\n"
      "	fistp	(%2)\n"
      "	incl	%0\n"
      "	jmp	2b\n"
"3:"			:
     "=a"(retval)	:
     "S"(counter),
     "D"(arr18bytes)	:
     "st","st(1)","st(2)","st(3)");
  return retval;
}

static unsigned long __NEAR__ __FASTCALL__ __FOPS_w_wait(volatile unsigned *counter,char *arr14bytes)
{
  return __FOPS_nowait(counter,arr14bytes);
}

static unsigned long __NEAR__ __FASTCALL__ __MOPS_std(volatile unsigned *counter,char *arr)
{
  register unsigned long retval;
   retval=0;
   while(*counter==0);
   while(*counter!=0){
    __asm __volatile(
"movd	%0,%%mm0\n"
"packssdw %%mm0,%%mm5 \n"
"packsswb %%mm0,%%mm4 \n"
"packuswb %%mm0,%%mm7 \n"
"paddb    %%mm3,%%mm2 \n"
"paddd    %%mm5,%%mm1 \n"
"psubsb   %%mm4,%%mm6 \n"
"psubusb  %%mm3,%%mm4 \n"
"pand     %%mm1,%%mm3 \n"
"pcmpeqd  %%mm0,%%mm0 \n"
"pcmpgtb  %%mm2,%%mm2 \n"
"pmaddwd  %%mm7,%%mm7 \n"
"pmullw   %%mm6,%%mm6 \n"
"por      %%mm2,%%mm4 \n"
"psllq    %%mm0,%%mm6 \n"
"psrad    %%mm1,%%mm3 \n"
"psubb    %%mm1,%%mm1 \n"
"psubsw   %%mm2,%%mm7 \n"
"psubusw  %%mm3,%%mm1 \n"
"punpckhdq %%mm0,%%mm4 \n"
"punpcklwd %%mm2,%%mm0 \n"
"pxor     %%mm4,%%mm2 \n"
"packssdw %%mm0,%%mm5 \n"
"packsswb %%mm0,%%mm4 \n"
"packuswb %%mm0,%%mm7 \n"
"paddb    %%mm3,%%mm2 \n"
"paddd    %%mm5,%%mm1 \n"
"psubsb   %%mm4,%%mm6 \n"
"psubusb  %%mm3,%%mm4 \n"
"pand     %%mm1,%%mm3 \n"
"pcmpeqd  %%mm0,%%mm0 \n"
"pcmpgtb  %%mm2,%%mm2 \n"
"pmaddwd  %%mm7,%%mm7 \n"
"pmullw   %%mm6,%%mm6 \n"
"por      %%mm2,%%mm4 \n"
"psllq    %%mm0,%%mm6 \n"
"psrad    %%mm1,%%mm3 \n"
"psubb    %%mm1,%%mm1 \n"
"psubsw   %%mm2,%%mm7 \n"
"psubusw  %%mm3,%%mm1 \n"
"punpckhdq %%mm0,%%mm4 \n"
"punpcklwd %%mm2,%%mm0 \n"
"pxor     %%mm4,%%mm2 \n"
"movd     %0,%%mm0 \n"
"packssdw %%mm0,%%mm5 \n"
"packsswb %%mm0,%%mm4 \n"
"packuswb %%mm0,%%mm7 \n"
"paddb    %%mm3,%%mm2 \n"
"paddd    %%mm5,%%mm1 \n"
"psubsb   %%mm4,%%mm6 \n"
"psubusb  %%mm3,%%mm4 \n"
"pand     %%mm1,%%mm3 \n"
"pcmpeqd  %%mm0,%%mm0 \n"
"pcmpgtb  %%mm2,%%mm2 \n"
"pmaddwd  %%mm7,%%mm7 \n"
"pmullw   %%mm6,%%mm6 \n"
"por      %%mm2,%%mm4 \n"
"psllq    %%mm0,%%mm6 \n"
"psrad    %%mm1,%%mm3 \n"
"psubb    %%mm1,%%mm1 \n"
"psubsw   %%mm2,%%mm7 \n"
"psubusw  %%mm3,%%mm1 \n"
"punpckhdq %%mm0,%%mm4 \n"
"punpcklwd %%mm2,%%mm0 \n"
"pxor     %%mm4,%%mm2 \n"
"packssdw %%mm0,%%mm5 \n"
"packsswb %%mm0,%%mm4 \n"
"packuswb %%mm0,%%mm7 \n"
"paddb    %%mm3,%%mm2 \n"
"paddd    %%mm5,%%mm1 \n"
"psubsb   %%mm4,%%mm6 \n"
"psubusb  %%mm3,%%mm4 \n"
"pand     %%mm1,%%mm3 \n"
"pcmpeqd  %%mm0,%%mm0 \n"
"pcmpgtb  %%mm2,%%mm2 \n"
"pmaddwd  %%mm7,%%mm7 \n"
"pmullw   %%mm6,%%mm6 \n"
"por      %%mm2,%%mm4 \n"
"psllq    %%mm0,%%mm6 \n"
"psrad    %%mm1,%%mm3 \n"
"psubb    %%mm1,%%mm1 \n"
"psubsw   %%mm2,%%mm7 \n"
"psubusw  %%mm3,%%mm1 \n"
"punpckhdq %%mm0,%%mm4 \n"
"punpcklwd %%mm2,%%mm0 \n"
"pxor     %%mm4,%%mm2"
  ::"r"(retval)
  :"mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm7");
  retval++;
  }
  return retval;
}

static unsigned long __NEAR__ __FASTCALL__ __SSEOPS_std(volatile unsigned *counter,char *arr)
{
  register unsigned long retval;
   retval=0;
   while(*counter==0);
   while(*counter!=0){
    __asm __volatile(
"movaps   (%0), %%xmm0 \n"
"movhps   (%0),%%xmm1 \n"
"movlps   (%0),%%xmm2 \n"
"movups   (%0),%%xmm3 \n"
"addps    %%xmm0,%%xmm1 \n"
"addss    %%xmm0,%%xmm1 \n"
"cvtps2pi %%xmm1,%%mm0 \n"
"cvttps2pi %%xmm2,%%mm1 \n"
"maxps    %%xmm0,%%xmm1 \n"
"maxss    %%xmm0,%%xmm1 \n"
"minps    %%xmm0,%%xmm1 \n"
"minss    %%xmm0,%%xmm1 \n"
"mulps    %%xmm0,%%xmm1 \n"
"mulss    %%xmm0,%%xmm1 \n"
"andps    %%xmm0,%%xmm1 \n"
"orps     %%xmm0,%%xmm1 \n"
"xorps    %%xmm1,%%xmm1 \n"
"divps    %%xmm0,%%xmm1 \n"
"divss    %%xmm0,%%xmm1 \n"
"rcpps    %%xmm0,%%xmm1 \n"
"rcpss    %%xmm0,%%xmm1 \n"
"rsqrtps  %%xmm0,%%xmm1 \n"
"rsqrtss  %%xmm0,%%xmm1 \n"
"sqrtps   %%xmm0,%%xmm1 \n"
"sqrtss   %%xmm0,%%xmm1 \n"
"subps    %%xmm0,%%xmm1 \n"
"subss    %%xmm0,%%xmm1 \n"
"ucomiss  %%xmm0,%%xmm1 \n"
"unpckhps %%xmm0,%%xmm1 \n"
"unpcklps %%xmm0,%%xmm1 \n"
"movaps   (%0),%%xmm0 \n"
"movhps   (%0),%%xmm1 \n"
"movlps   (%0),%%xmm2 \n"
"movups   (%0),%%xmm3 \n"
"addps    %%xmm0,%%xmm1 \n"
"addss    %%xmm0,%%xmm1 \n"
"cvtps2pi %%xmm1,%%mm0 \n"
"cvttps2pi %%xmm2,%%mm1 \n"
"maxps    %%xmm0,%%xmm1 \n"
"maxss    %%xmm0,%%xmm1 \n"
"minps    %%xmm0,%%xmm1 \n"
"minss    %%xmm0,%%xmm1 \n"
"mulps    %%xmm0,%%xmm1 \n"
"mulss    %%xmm0,%%xmm1 \n"
"andps    %%xmm0,%%xmm1 \n"
"orps     %%xmm0,%%xmm1 \n"
"xorps    %%xmm1,%%xmm1 \n"
"divps    %%xmm0,%%xmm1 \n"
"divss    %%xmm0,%%xmm1 \n"
"rcpps    %%xmm0,%%xmm1 \n"
"rcpss    %%xmm0,%%xmm1 \n"
"rsqrtps  %%xmm0,%%xmm1 \n"
"rsqrtss  %%xmm0,%%xmm1 \n"
"sqrtps   %%xmm0,%%xmm1 \n"
"sqrtss   %%xmm0,%%xmm1 \n"
"subps    %%xmm0,%%xmm1 \n"
"subss    %%xmm0,%%xmm1 \n"
"ucomiss  %%xmm0,%%xmm1 \n"
"unpckhps %%xmm0,%%xmm1 \n"
"unpcklps %%xmm0,%%xmm1 \n"
::"r"(arr)
:"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7");
  retval++;
  }
  return retval;
}

#define __ASMPART_DEFINED 1
#include "biewlib/sysdep/ia16/cmn_ix86.c"

#elif defined(__WATCOMC__) && __WATCOMC__ >= 1100

#include "biewlib/sysdep/ia32/cpu_info.wc"

#define __ASMPART_DEFINED 1
#include "biewlib/sysdep/ia16/cmn_ix86.c"

#elif defined(__WATCOMC__) && defined(__QNX4__)

#include "biewlib/sysdep/ia32/qnx/cpu_info.qnx"

#define __ASMPART_DEFINED 1
#include "biewlib/sysdep/ia16/cmn_ix86.c"

#else

#include "biewlib/sysdep/generic/cpu_info.c"

#endif
