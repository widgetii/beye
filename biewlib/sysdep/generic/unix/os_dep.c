/** 
 * @namespace   biewlib
 * @file        biewlib/sysdep/generic/unix/os_dep.c
 * @brief       This file contains implementation of unix compatible OS dependent part.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Konstantin Boldyshev
 * @since       1999
 * @note        Development, fixes and improvements
**/

/*
    Copyright (C) 1999-2001 Konstantin Boldyshev <konst@linuxassembly.org>

    $Id$
*/

#ifndef lint
static const char rcs_id[] = "$Id$";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "biewlib/biewlib.h"
#include "console.h"

#ifndef	PREFIX
#define	PREFIX	"/usr/local"
#endif

#ifndef	DATADIR
#define DATADIR	PREFIX"/share"
#endif

static char _ini_name[FILENAME_MAX + 1];
static char _rc_dir_name[FILENAME_MAX + 1];

int terminal = TERM_UNKNOWN;

tBool break_status = False;	/**< CTRL+BREAK flag */
extern void ReadNextEvent(void);

static struct {
    char *name;
    char type;
} termtab[] = {
{ "linux",	TERM_LINUX },
{ "console",	TERM_LINUX },
{ "xterm",	TERM_XTERM },
{ "xterm-color", TERM_XTERM},
{ "color-xterm", TERM_XTERM},
{ "beterm",	TERM_XTERM },
{ "vt100",	TERM_VT100 },
{ "ansi",	TERM_ANSI  },
{ NULL,		TERM_UNKNOWN}};

/*

*/

char * __FASTCALL__ __get_ini_name(const char *progname)
{
    char *p = getenv("HOME");

    if (p == NULL || strlen(p) < 2) {
	struct passwd *psw = getpwuid(getuid());
	if (psw != NULL) p = psw->pw_dir;
    }	

    if (p == NULL || strlen(p) > FILENAME_MAX - (strlen(progname) + 4))
	p = "/tmp";

    strcpy(_ini_name, p);
    strcat(_ini_name, "/.");
    strcat(_ini_name, progname);
    return strcat(_ini_name, "rc");
}

char * __FASTCALL__ __get_rc_dir(const char *progname)
{
    strcpy(_rc_dir_name, DATADIR"/");
    strcat(_rc_dir_name, progname);
    return strcat(_rc_dir_name, "/");
}


tBool __FASTCALL__ __OsGetCBreak(void)
{
#ifndef	__ENABLE_SIGIO
    ReadNextEvent();
#endif
    return break_status;
}

void  __FASTCALL__ __OsSetCBreak(tBool state)
{
    break_status = state;
}

void __FASTCALL__ __OsYield(void)
{
#ifdef	__BEOS__
    /* usleep(10000); */
#else
    struct timespec t = { 0, 100000 };
    nanosleep(&t, NULL);
#endif
}

static void cleanup(int sig)
{
    __term_keyboard();
    __term_vio();
    __term_sys();
    printm("Terminated by signal %d\n", sig);
    _exit(EXIT_FAILURE);
}

/* static struct sigaction sa; */

void __FASTCALL__ __init_sys(void)
{
    int i = 0;
    char *t = getenv("TERM");

    if (t != NULL)
	for (i = 0; termtab[i].name != NULL && strcasecmp(t, termtab[i].name); i++);
    terminal = termtab[i].type;
    if (terminal == TERM_XTERM) {
	t = getenv("COLORTERM");
	if (t != NULL && !strcasecmp(t, "Eterm")) transparent = 1;
    }
#ifdef _VT100_
    if (terminal == TERM_UNKNOWN) {
        printm("Sorry, I can't handle terminal type '%s'.\nIf you are sure it is vt100 compatible, try setting TERM to vt100:\n\n$ TERM=vt100 biew filename\n\nIf you will not succeed, use slang/curses version of BIEW.\n\n", t);
        exit(2);
    }
#endif
    umask(0077);
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);
    signal(SIGQUIT, cleanup);
    signal(SIGILL, cleanup);
/*
    sa.sa_handler = cleanup;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
*/
}

void __FASTCALL__ __term_sys(void)
{
}
