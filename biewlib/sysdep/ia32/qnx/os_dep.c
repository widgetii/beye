/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/qnx/os_dep.c
 * @brief       This file contains implementation of OS depended part for QNX4.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/sched.h>
#include <sys/qnxterm.h>
#include <sys/kernel.h>

#include "biewlib/biewlib.h"

#define LIBDIR "/usr/lib"

static char _ini_name[FILENAME_MAX+1];
static char _rc_dir_name[FILENAME_MAX+1];
static tBool break_status;
static pid_t proxy=0;
static timer_t t=0;
static struct sigevent evp;
static struct itimerspec it;

char* __FASTCALL__ __get_ini_name(const char *progname)
{
	char *p;

	if((p=getenv("HOME"))==NULL||strlen(p)>FILENAME_MAX-10)
	strcpy(_ini_name,"/tmp");
	else
	strcpy(_ini_name,p);

	strcat(_ini_name,"/.");
	strcat(_ini_name,progname);
	strcat(_ini_name,"rc");

	return _ini_name;
}

char* __FASTCALL__ __get_rc_dir(const char *progname)
{
	strcpy(_rc_dir_name,LIBDIR"/");
	strcat(_rc_dir_name,progname);
	strcat(_rc_dir_name,"/");
	return _rc_dir_name;
}

void __FASTCALL__ __OsYield(void)
{
	it.it_value.tv_sec=0;
	it.it_value.tv_nsec=100000;
	it.it_interval.tv_sec=0;
	it.it_interval.tv_nsec=0;
	timer_settime(t,TIMER_ADDREL,&it,NULL);
	term_flush();
	Receive(proxy,NULL,0);
}

tBool __FASTCALL__ __OsGetCBreak(void)
{
	return break_status;
}

void __FASTCALL__ __OsSetCBreak(tBool state)
{
	break_status=state;
}

static void cleanup(int sig)
{
	__term_keyboard();
	__term_vio();
	__term_sys();
	printm("Terminated by signal %d\n", sig);
	_exit(1);
}

/* static struct sigaction sa; */

void __FASTCALL__ __init_sys(void)
{
	int i=0;
	struct sched_param sp;
	if(term_load()<0)
	{
		perror("Init terminal: ");
		exit(-1);
	}
	umask(0077);
	signal(SIGTERM,cleanup);
	signal(SIGINT,cleanup);
	signal(SIGQUIT,cleanup);
	signal(SIGILL,cleanup);
	sp.sched_priority=PRIO_USER_DFLT;
	sched_setscheduler(0,SCHED_OTHER,&sp);
	if(proxy==0)
		proxy=qnx_proxy_attach(0,NULL,0,-1);
		if(proxy==-1)
		{
			perror("proxy attach");
			exit(1);
		}
	evp.sigev_signo=-(proxy);
	if(t==0)
		t=timer_create(CLOCK_REALTIME,&evp);
		if(t==-1)
		{
			perror("timer create");
			exit(1);
		}
}

void __FASTCALL__ __term_sys(void)
{
	if(t!=-1)
		timer_delete(t);
	qnx_proxy_detach(proxy);
}

