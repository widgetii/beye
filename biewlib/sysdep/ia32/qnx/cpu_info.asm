; **
; * @namespace   biewlib
; * @file        biewlib/sysdep/ia32/qnx/cpu_info.asm
; * @brief       This file contains function for retrieving CPU information for
; *              32-bit Intel x86 compatible platform
; * @version     -
; * @remark      this source file is part of Binary vIEW project (BIEW).
; *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
; *              All rights reserved. This software is redistributable under the
; *              licence given in the file "Licence.en" ("Licence.ru" in russian
; *              translation) distributed in the BIEW archive.
; * @note        Requires POSIX compatible development system
; * @author      Andrew Golovnia
; * @since       2001
; * @note        Development, fixes and improvements
; * @note        Code for QNX 4.xx + Watcom C 10.6 (depricated)
;**

.386p
.387
.model flat

	public __OPS_nop_
	public __OPS_std_
	public __FOPS_nowait_
	public __MOPS_std_
	public __SSEOPS_std_

.code

;unsigned long __OPS_nop(volatile unsigned*)
__OPS_nop_ proc near
	push	edx
	push	ecx
	mov 	edx,eax
	xor 	eax,eax
on1:
	cmp 	dword ptr [edx],0
	jz  	on1
on2:
	cmp 	dword ptr [edx],0
	jz		on3
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	db		90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	inc 	eax
	jmp 	on2
on3:
	pop 	edx
	pop 	ecx
	ret
__OPS_nop_ endp

;unsigned long __OPS_std(volatile unsigned*,char*)
__OPS_std_ proc near
	push	ebx
	push	ecx
	push	esi
	push	edi
	mov 	esi,eax
	mov 	edi,edx
	xor 	eax,eax
os1:
	cmp 	dword ptr [esi],0
	jz		os1
os2:
	cmp 	dword ptr [esi],0
	jnz 	os3
	jmp 	os4
os3:
	push	eax
	mov 	eax,14h
	mov 	ecx,7
	mul 	ecx
	imul	ecx
	div 	ecx
	idiv	ecx
	add 	eax,ecx
	adc 	eax,1
	sub 	eax,ecx
	sbb 	eax,1
	push	esi
	push	edi
	mov 	esi,edi
	movsd
	call	os5
	cmpsd
	pop 	edi
	pop 	esi
	push	eax
	push	edx
	pop 	edx
	pop 	eax
	mov 	eax,14h
	mov 	ecx,7
	mul 	ecx
	imul	ecx
	div 	ecx
	idiv	ecx
	add 	eax,ecx
	adc 	eax,1
	sub 	eax,ecx
	sbb 	eax,1
	push	esi
	push	edi
	mov 	esi,edi
	movsd
	call	os5
	cmpsd
	pop 	edi
	pop 	esi
	push	eax
	push	edx
	pop 	edx
	pop 	eax
	mov 	eax,14h
	mov 	ecx,7
	mul 	ecx
	imul	ecx
	div 	ecx
	idiv	ecx
	add 	eax,ecx
	adc 	eax,1
	sub 	eax,ecx
	sbb 	eax,1
	push	esi
	push	edi
	mov 	esi,edi
	movsd
	call	os5
	cmpsd
	pop 	edi
	pop 	esi
	push	eax
	push	edx
	pop 	edx
	pop 	eax
	mov 	eax,14h
	mov 	ecx,7
	mul 	ecx
	imul	ecx
	div 	ecx
	idiv	ecx
	adc 	eax,1
	sub 	eax,ecx
	sbb 	eax,1
	push	edx
	pop 	edx
	pop 	eax
	inc 	eax
	jmp 	os2
os5:
	ret
os4:
	pop 	edi
	pop 	esi
	pop 	ecx
	pop 	ebx
	ret
__OPS_std_ endp

;unsigned long __FOPS_nowait(volatile unsigned*,char*)
__FOPS_nowait_ proc near
	push	ebx
	push	ecx
	push	esi
	push	edi
	mov 	esi,eax
	mov 	edi,edx
	xor 	eax,eax
fn1:
	cmp 	dword ptr [esi],0
	jz		fn1
fn2:
	cmp 	dword ptr [esi],0
	jz		fn3
	finit
	fld 	tbyte ptr [edi+8]
	fstp	tbyte ptr [edi+8]
	fstp	st(1)
	fldz
	fld1
	fcompp
	fstsw	[edi+4]
	fstcw	[edi]
	fldcw	[edi]
	fldpi
	fstp	st(1)
	fst 	st(2)
	fst 	st(3)
	f2xm1
	fabs
	fchs
	fprem
	fptan
	fsqrt
	frndint
	faddp	st(1),st
	fstp	st(1)
	fmulp	st(1),st
	fstp	st(1)
	fld1
	fstp	st(1)
	fpatan
	fstp	st(1)
	fscale
	fstp	st(1)
	fdivp	st(1),st
	fstp	st(1)
	fsubrp	st(1),st
	fstp	st(1)
	fyl2x
	fstp	st(1)
	fyl2xp1
	fstp	st(1)
	fbld	tbyte ptr [edi+8]
	fbstp	tbyte ptr [edi+8]
	fild	word ptr [edi]
	fistp	word ptr [edi]
	fld 	tbyte ptr [edi+4]
	fstp	tbyte ptr [edi+4]
	fstp	st(1)
	fldz
	fld1
	fcompp
	fstsw	[edi+4]
	fstcw	[edi]
	fldcw	[edi]
	fldpi
	fstp	st(1)
	fst 	st(2)
	fst 	st(3)
	f2xm1
	fabs
	fchs
	fprem
	fptan
	fsqrt
	frndint
	faddp	st(1),st
	fstp	st(1)
	fmulp	st(1),st
	fstp	st(1)
	fld1
	fstp	st(1)
	fpatan
	fstp	st(1)
	fscale
	fstp	st(1)
	fdivp	st(1),st
	fstp	st(1)
	fsubrp	st(1),st
	fstp	st(1)
	fyl2x
	fstp	st(1)
	fyl2xp1
	fstp	st(1)
	fild	word ptr [edi]
	fistp	word ptr [edi]
	inc 	eax
	jmp 	fn2
fn3:
	pop		edi
	pop 	esi
	pop 	ecx
	pop 	ebx
	ret
__FOPS_nowait_ endp

;unsigned long __MOPS_std(volatile unsigned*,char*)
__MOPS_std_ proc near
	push	ebx
	push	ecx
	push	esi
	push	edi
	mov 	esi,eax
	mov 	edi,edx
	xor 	eax,eax
ms1:
	cmp 	dword ptr [esi],0
	jz		ms1
ms2:
	cmp 	dword ptr [esi],0
	jz		ms3
	db		00Fh,077h
	db		00Fh,06Eh,0C0h
	db		00Fh,06Bh,0C5h
	db		00Fh,063h,0C4h
	db		00Fh,067h,0C7h
	db		00Fh,0FCh,0DAh
	db		00Fh,0FEh,0E9h
	db		00Fh,0E8h,0E6h
	db		00Fh,0D8h,0DCh
	db		00Fh,0DBh,0CBh
	db		00Fh,076h,0C0h
	db		00Fh,064h,0D2h
	db		00Fh,0F5h,0FFh
	db		00Fh,0D5h,0F6h
	db		00Fh,0EBh,0D4h
	db		00Fh,0F3h,0C6h
	db		00Fh,0E2h,0CBh
	db		00Fh,0F8h,0C9h
	db		00Fh,0E9h,0D7h
	db		00Fh,0D9h,0D9h
	db		00Fh,06Ah,0C4h
	db		00Fh,061h,0D0h
	db		00Fh,0EFh,0E2h
	db		00Fh,06Bh,0C5h
	db		00Fh,063h,0C4h
	db		00Fh,067h,0C7h
	db		00Fh,0FCh,0DAh
	db		00Fh,0FEh,0E9h
	db		00Fh,0E8h,0E6h
	db		00Fh,0D8h,0DCh
	db		00Fh,0DBh,0CBh
	db		00Fh,076h,0C0h
	db		00Fh,064h,0D2h
	db		00Fh,0F5h,0FFh
	db		00Fh,0D5h,0F6h
	db		00Fh,0EBh,0D4h
	db		00Fh,0F3h,0C6h
	db		00Fh,0E2h,0CBh
	db		00Fh,0F8h,0C9h
	db		00Fh,0E9h,0D7h
	db		00Fh,0D9h,0D9h
	db		00Fh,06Ah,0C4h
	db		00Fh,061h,0D0h
	db		00Fh,0EFh,0E2h
	db		00Fh,077h
	db		00Fh,06Eh,0C0h
	db		00Fh,06Bh,0C5h
	db		00Fh,063h,0C4h
	db		00Fh,067h,0C7h
	db		00Fh,0FCh,0DAh
	db		00Fh,0FEh,0E9h
	db		00Fh,0E8h,0E6h
	db		00Fh,0D8h,0DCh
	db		00Fh,0DBh,0CBh
	db		00Fh,076h,0C0h
	db		00Fh,064h,0D2h
	db		00Fh,0F5h,0FFh
	db		00Fh,0D5h,0F6h
	db		00Fh,0EBh,0D4h
	db		00Fh,0F3h,0C6h
	db		00Fh,0E2h,0CBh
	db		00Fh,0F8h,0C9h
	db		00Fh,0E9h,0D7h
	db		00Fh,0D9h,0D9h
	db		00Fh,06Ah,0C4h
	db		00Fh,061h,0D0h
	db		00Fh,0EFh,0E2h
	db		00Fh,06Bh,0C5h
	db		00Fh,063h,0C4h
	db		00Fh,067h,0C7h
	db		00Fh,0FCh,0DAh
	db		00Fh,0FEh,0E9h
	db		00Fh,0E8h,0E6h
	db		00Fh,0D8h,0DCh
	db		00Fh,0DBh,0CBh
	db		00Fh,076h,0C0h
	db		00Fh,064h,0D2h
	db		00Fh,0F5h,0FFh
	db		00Fh,0D5h,0F6h
	db		00Fh,0EBh,0D4h
	db		00Fh,0F3h,0C6h
	db		00Fh,0E2h,0CBh
	db		00Fh,0F8h,0C9h
	db		00Fh,0E9h,0D7h
	db		00Fh,0D9h,0D9h
	db		00Fh,06Ah,0C4h
	db		00Fh,061h,0D0h
	db		00Fh,0EFh,0E2h
	inc 	eax
	jmp 	ms2
ms3:
	pop		edi
	pop 	esi
	pop 	ecx
	pop 	ebx
	ret
__MOPS_std_ endp

;unsigned long __SSEOPS_std(volatile unsigned*,char*)
__SSEOPS_std_ proc near
	push	ebx
	push	ecx
	push	esi
	push	edi
	mov 	esi,edx
	mov 	edi,ebx
	xor 	eax,eax
ss1:
	cmp 	dword ptr [esi],0
	jz		ss1
ss2:
	cmp 	dword ptr [esi],0
	jz		ss3
	db		00Fh,077h
	db		00Fh,028h,007h
	db		00Fh,016h,00Fh
	db		00Fh,012h,017h
	db		00Fh,010h,01Fh
	db		00Fh,058h,0C1h
	db		0F3h,00Fh,058h,0C1h
	db		00Fh,02Dh,0C1h
	db		00Fh,02Ch,0CAh
	db		00Fh,05Fh,0C1h
	db		0F3h,00Fh,05Fh,0C1h
	db		00Fh,05Dh,0C1h
	db		0F3h,00Fh,05Dh,0C1h
	db		00Fh,059h,0C1h
	db		0F3h,00Fh,059h,0C1h
	db		00Fh,054h,0C1h
	db		00Fh,056h,0C1h
	db		00Fh,057h,0C9h
	db		00Fh,05Eh,0C1h
	db		0F3h,00Fh,05Eh,0C1h
	db		00Fh,053h,0C1h
	db		0F3h,00Fh,053h,0C1h
	db		00Fh,052h,0C1h
	db		0F3h,00Fh,052h,0C1h
	db		00Fh,051h,0C1h
	db		0F3h,00Fh,051h,0C1h
	db		00Fh,05Ch,0C1h
	db		0F3h,00Fh,05Ch,0C1h
	db		00Fh,02Eh,0C1h
	db		00Fh,015h,0C1h
	db		00Fh,014h,0C1h
	db		00Fh,077h
	db		00Fh,028h,007h
	db		00Fh,016h,00Fh
	db		00Fh,012h,017h
	db		00Fh,010h,01Fh
	db		00Fh,058h,0C1h
	db		0F3h,00Fh,058h,0C1h
	db		00Fh,02Dh,0C1h
	db		00Fh,02Ch,0CAh
	db		00Fh,05Fh,0C1h
	db		0F3h,00Fh,05Fh,0C1h
	db		00Fh,05Dh,0C1h
	db		0F3h,00Fh,05Dh,0C1h
	db		00Fh,059h,0C1h
	db		0F3h,00Fh,059h,0C1h
	db		00Fh,054h,0C1h
	db		00Fh,056h,0C1h
	db		00Fh,057h,0C9h
	db		00Fh,05Eh,0C1h
	db		0F3h,00Fh,05Eh,0C1h
	db		00Fh,053h,0C1h
	db		0F3h,00Fh,053h,0C1h
	db		00Fh,052h,0C1h
	db		0F3h,00Fh,052h,0C1h
	db		00Fh,051h,0C1h
	db		0F3h,00Fh,051h,0C1h
	db		00Fh,05Ch,0C1h
	db		0F3h,00Fh,05Ch,0C1h
	db		00Fh,02Eh,0C1h
	db		00Fh,015h,0C1h
	db		00Fh,014h,0C1h
	inc 	eax
	jmp 	ss2
ss3:
	pop		edi
	pop 	esi
	pop 	ecx
	pop 	ebx
	ret
__SSEOPS_std_ endp

	end