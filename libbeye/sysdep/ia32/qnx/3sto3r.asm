; **
; * @namespace   biewlib
; * @file        biewlib/sysdep/ia32/qnx/3rto3s.asm
; * @brief       This file contains 3s->3r conversion function for photon3s.lib
; * @version     -
; * @remark      this source file is part of Binary vIEW project (BIEW).
; *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
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
;	Open a communications channel
;	struct _Ph_ctrl *PhAttach(char const *name,PhChannelParms_t const *parms);
;	--------------------------------------------------------------------------
extern PhAttach_ :near
public PhAttach
PhAttach proc near
	push	edx
	mov		eax,08h[esp]
	mov		edx,0ch[esp]
	call	PhAttach_
	pop		edx
	ret
PhAttach endp

;	--------------------------------------------------------------------------
;	Collect cursor information
;	int PhQueryCursor(unsigned short ig,PhCursorInfo_t *buf);
;	--------------------------------------------------------------------------
extern PhQueryCursor_ :near
public PhQueryCursor
PhQueryCursor proc near
	push	edx
	mov		eax,08h[esp]
	mov		edx,0ch[esp]
	call	PhQueryCursor_
	pop		edx
	ret
PhQueryCursor endp

end
