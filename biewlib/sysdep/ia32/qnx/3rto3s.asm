; **
; * @namespace   biewlib
; * @file        biewlib/sysdep/ia32/qnx/3sto3r.asm
; * @brief       This file contains 3r->3s conversion function for photon3s.lib
; * @version     -
; * @remark      this source file is part of Binary vIEW project (BIEW).
; *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
; *              All rights reserved. This software is redistributable under the
; *              licence given in the file "Licence.en" ("Licence.ru" in russian
; *              translation) distributed in the BIEW archive.
; * @note        Requires POSIX compatible development system
; * @author      Andrew Golovnia
; * @since       2002
; * @note        Development, fixes and improvements
;**

.386p
.387
.model flat
.code

;	--------------------------------------------------------------------------
;	set the first part of an object to a given value
;	void *memset(void *dst,int c,size_t length);
;	--------------------------------------------------------------------------
extern memset :near
public memset_
memset_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	memset
	add		esp,0ch
	pop		ecx
	ret
memset_ endp

;	--------------------------------------------------------------------------
;	find the last occurrence of a character in a string
;	char *strrchr(const char *s,int c);
;	--------------------------------------------------------------------------
extern strrchr :near
public strrchr_
strrchr_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	strrchr
	add		esp,08h
	pop		ebx
	pop		ecx
	ret
strrchr_ endp

;	--------------------------------------------------------------------------
;	compare two strings
;	int strcmp(const char *s1,const char *s2);
;	--------------------------------------------------------------------------
extern strcmp :near
public strcmp_
strcmp_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	strcmp
	add		esp,08h
	pop		ebx
	pop		ecx
	ret
strcmp_ endp

;	--------------------------------------------------------------------------
;	compare two strings, up to a given length
;	int strncmp(const char *s1,const char *s2,size_t n);
;	--------------------------------------------------------------------------
extern strncmp :near
public strncmp_
strncmp_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	strncmp
	add		esp,0ch
	pop		ecx
	ret
strncmp_ endp

;	--------------------------------------------------------------------------
;	create a duplicate of a string
;	char *strdup(const char *src);
;	--------------------------------------------------------------------------
extern strdup :near
public strdup_
strdup_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	strdup
	add		esp,04h
	pop		edx
	pop		ebx
	pop		ecx
	ret
strdup_ endp

;	--------------------------------------------------------------------------
;	convert a character to lowercase
;	int tolower(int c);
;	--------------------------------------------------------------------------
extern tolower :near
public tolower_
tolower_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	tolower
	add		esp,04h
	pop		edx
	pop		ebx
	pop		ecx
	ret
tolower_ endp

;	--------------------------------------------------------------------------
;	convert a string to a long integer
;	long int strtol(const char *ptr,char **endptr,int base);
;	--------------------------------------------------------------------------
extern strtol :near
public strtol_
strtol_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	strtol
	add		esp,0ch
	pop		ecx
	ret
strtol_ endp

;	--------------------------------------------------------------------------
;	deallocate a block of memory
;	void free(void *ptr);
;	--------------------------------------------------------------------------
extern free :near
public free_
free_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	free
	add		esp,04h
	pop		edx
	pop		ebx
	pop		ecx
	ret
free_ endp

;	--------------------------------------------------------------------------
;	allocate space for an array
;	void *calloc(size_t n,size_t size);
;	--------------------------------------------------------------------------
extern calloc :near
public calloc_
calloc_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	calloc
	add		esp,08h
	pop		ebx
	pop		ecx
	ret
calloc_ endp

;	--------------------------------------------------------------------------
;	allocate memory
;	void *malloc(size_t size);
;	--------------------------------------------------------------------------
extern malloc :near
public malloc_
malloc_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	malloc
	add		esp,04h
	pop		edx
	pop		ebx
	pop		ecx
	ret
malloc_ endp

;	--------------------------------------------------------------------------
;	get an environment variable
;	char *getenv(const char *name);
;	--------------------------------------------------------------------------
extern getenv :near
public getenv_
getenv_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	getenv
	add		esp,04h
	pop		edx
	pop		ebx
	pop		ecx
	ret
getenv_ endp

;	--------------------------------------------------------------------------
;	get errno address
;	int *__get_errno_ptr(void);
;	--------------------------------------------------------------------------
extern __get_errno_ptr :near
public __get_errno_ptr_
__get_errno_ptr_ proc near
	call	__get_errno_ptr
__get_errno_ptr_ endp

;	--------------------------------------------------------------------------
;	set kernel errno
;	int __kererr(int err);
;	--------------------------------------------------------------------------
extern __kererr :near
public __kererr_
__kererr_ proc near
	call	__kererr
__kererr_ endp

;	--------------------------------------------------------------------------
;	send a message to a process associated with a file descriptor
;	int Sendfd(int fd,void *smsg,void *rmsg,unsigned snbytes,unsigned rnbytes);
;	--------------------------------------------------------------------------
extern __sendfd :near
public __sendfd_
__sendfd_ proc near
	push	08h[esp]
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	__sendfd
	add		esp,14h
	ret
__sendfd_ endp

;	--------------------------------------------------------------------------
;	determine the number of bytes comprising a multibyte character
;	int mblen(const char *s,size_t n);
;	--------------------------------------------------------------------------
extern mblen :near
public mblen_
mblen_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	mblen
	add		esp,08h
	pop		ebx
	pop		ecx
	ret
mblen_ endp

;	--------------------------------------------------------------------------
;	convert a multibyte character to a wide character
;	int mbtowc(wchar_t *pwc,const char *s,size_t n );
;	--------------------------------------------------------------------------
extern mbtowc :near
public mbtowc_
mbtowc_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	mbtowc
	add		esp,0ch
	pop		ecx
	ret
mbtowc_ endp

;	--------------------------------------------------------------------------
;	convert a wide character to a multibyte character
;	int wctomb(char *s,wchar_t wchar);
;	--------------------------------------------------------------------------
extern wctomb :near
public wctomb_
wctomb_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	wctomb
	add		esp,08h
	pop		ebx
	pop		ecx
	ret
wctomb_ endp

;	--------------------------------------------------------------------------
;	suspend a process for a given length of time
;	unsigned int delay(unsigned int milliseconds);
;	--------------------------------------------------------------------------
extern delay :near
public delay_
delay_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	delay
	add		esp,04h
	pop		edx
	pop		ebx
	pop		ecx
	ret
delay_ endp

;	--------------------------------------------------------------------------
;	map a region of a memory object
;	void *mmap(void *addr,size_t len,int prot,int flags,int fildes,off_t off);
;	--------------------------------------------------------------------------
extern mmap :near
public mmap_
mmap_ proc near
	push	0ch[esp]
	push	08h[esp]
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	mmap
	add		esp,18h
	ret
mmap_ endp

;	--------------------------------------------------------------------------
;	sort an array, using a modified Quicksort algorithm
;	void qsort(void *base,size_t num,size_t width,
;		int (*compar)(const void *,const void *));
;	--------------------------------------------------------------------------
extern qsort :near
public qsort_
qsort_ proc near
	push	ecx
	push	ebx
	push	edx
	push	eax
	call	qsort
	add		esp,10h
	ret
qsort_ endp

end
