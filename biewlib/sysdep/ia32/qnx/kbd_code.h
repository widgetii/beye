/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/qnx/kbd_code.h
 * @brief       This file contains definitions of QNX4 keyboard codes.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Andrew Golovnia
 * @since       2001
 * @note        Development, fixes and improvements
**/

#ifndef __KBD_CODES_H
#define __KBD_CODES_H

#include<sys/qnxterm.h>

/* This codes describe shift keys state */

#define KS_SHIFT           3
#define KS_CTRL            4
#define KS_ALT             8
#define KS_CTRL_MASK      15
#define KS_SCRLOCK        16
#define KS_NUMLOCK        32
#define KS_CAPSLOCK       64
#define KS_INSERT        128

/*
    not real keys, for internal use only
*/

#define KE_SUPERKEY	0xFFFF
#define KE_JUSTFIND	0xFFFE

/*
   This code describe mouse event
*/

#define KE_MOUSE        0xFFFB

/*
   This code describe shift keys event
*/

#define KE_SHIFTKEYS    0xFFFA

/*
   This section describe real key code
*/

#define KE_ESCAPE            K_ESC
#define KE_ENTER             K_ENTER
#define KE_CTL_ENTER         (K_CTL_ENTER<<8)
#define KE_SPACE             0x0020 /*0x3920*/ //?
#define KE_BKSPACE           K_RUBOUT
#define KE_CTL_BKSPACE       (K_CTL_BACKSP<<8)
#define KE_TAB               K_TAB
#define KE_SHIFT_TAB         (0x0161<<8) /*0x0F00*/ //?

#define KE_DOWNARROW         (K_DOWN<<8)
#define KE_UPARROW           (K_UP<<8)
#define KE_LEFTARROW         (K_LEFT<<8)
#define KE_RIGHTARROW        (K_RIGHT<<8)

#define KE_CTL_LEFTARROW     (K_CTL_LEFT<<8)
#define KE_CTL_RIGHTARROW    (K_CTL_RIGHT<<8)

#define KE_HOME              (K_HOME<<8)
#define KE_END               (K_END<<8)
#define KE_PGUP              (K_PGUP<<8)
#define KE_PGDN              (K_PGDN<<8)
#define KE_INS               (K_INSERT<<8)
#define KE_DEL               (K_DELETE<<8)

#define KE_CTL_PGDN          (K_CTL_PGDN<<8)
#define KE_CTL_PGUP          (K_CTL_PGUP<<8)
#define KE_CTL_HOME          (K_CTL_HOME<<8)
#define KE_CTL_END           (K_CTL_END<<8)
#define KE_CTL_INS           (K_CTL_INSERT<<8)
#define KE_CTL_DEL           (K_CTL_DELETE<<8)

#define KE_F(x)		((K_F1-1+(x))<<8)
#define KE_SHIFT_F(x)	((K_SHF_F1-1+(x))<<8)
#define KE_CTL_F(x)	((K_CTL_F1-1+(x))<<8)
#define KE_ALT_F(x)	((K_ALT_F1-1+(x))<<8)

#define SCAN_A  0x1e
#define SCAN_B  0x30
#define SCAN_C  0x2e
#define SCAN_D  0x20
#define SCAN_E  0x12
#define SCAN_F  0x21
#define SCAN_G  0x22
#define SCAN_H  0x23
#define SCAN_I  0x17
#define SCAN_J  0x24
#define SCAN_K  0x25
#define SCAN_L  0x26
#define SCAN_M  0x31
#define SCAN_N  0x32
#define SCAN_O  0x18
#define SCAN_P  0x19
#define SCAN_Q  0x10
#define SCAN_R  0x13
#define SCAN_S  0x1f
#define SCAN_T  0x14
#define SCAN_U  0x16
#define SCAN_V  0x2f
#define SCAN_W  0x11
#define SCAN_X  0x2d
#define SCAN_Y  0x15
#define SCAN_Z  0x2c

#define ENUM_A  0x01
#define ENUM_B  0x02
#define ENUM_C  0x03
#define ENUM_D  0x04
#define ENUM_E  0x05
#define ENUM_F  0x06
#define ENUM_G  0x07
#define ENUM_H  0x08
#define ENUM_I  0x09
#define ENUM_J  0x0a
#define ENUM_K  0x0b
#define ENUM_L  0x0c
#define ENUM_M  0x0d
#define ENUM_N  0x0e
#define ENUM_O  0x0f
#define ENUM_P  0x10
#define ENUM_Q  0x11
#define ENUM_R  0x12
#define ENUM_S  0x13
#define ENUM_T  0x14
#define ENUM_U  0x15
#define ENUM_V  0x16
#define ENUM_W  0x17
#define ENUM_X  0x18
#define ENUM_Y  0x19
#define ENUM_Z  0x1a

#define __SCAN_(x) (SCAN_##x)
#define __ENUM_(x) (ENUM_##x)

#define KE_CTL_(x) (((__SCAN_(x) << 8) & 0xFF00) | ((__ENUM_(x)-__ENUM_(A)+1) & 0x00FF))
#define KE_ALT_(x) (((__SCAN_(x) << 8) & 0xFF00)

#endif/*__KBD_CODES_H*/

