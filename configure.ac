dnl ***********************************************
dnl * Please run autoreconf to test your changes! *
dnl ***********************************************

# Set VERSION so we only need to edit in one place (i.e., here)
m4_define(BEYE_VERSION, [6.1.1.alpha])

AC_INIT([beye], BEYE_VERSION, [https://github.com/widgetii/beye])
AC_CONFIG_AUX_DIR([.auto])
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects dist-bzip2])
AC_CONFIG_HEADERS([bconfig.h])

# Don't use default -O2 -g flags
: ${CFLAGS=""}

# Check for programs
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL
AM_PROG_AR

dnl This is for stuff that absolutely must end up in pyconfig.h.
dnl Please use pyport.h instead, if possible.
AH_TOP([
#ifndef Beye_CONFIG_H
#define Beye_CONFIG_H
])
dnl  output driver features
AC_ARG_ENABLE(slang,
  [  --enable-slang          slang graphics support (autodetected)])
AC_ARG_ENABLE(ncurses,
  [  --enable-ncurses        ncurses graphics support (autodetected)])
AC_ARG_ENABLE(iconv,
  [  --enable-iconv          iconv library support (autodetected)])


dnl  conditional builds
AC_ARG_ENABLE(debug,
  [  --enable-debug          build debug version of the program (default no)])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h gpm.h limits.h malloc.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/time.h termios.h unistd.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T

AC_TYPE_INT32_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([atexit ftruncate memmove memset munmap select strcasecmp strchr strcspn strdup strerror stricmp strncasecmp strpbrk strlwr strrchr strspn strstr strtol strtoul strtoull strupr sysinfo utime])


# TODO: make host machine autodetection
# NOW - just hardcoded
AC_DEFINE(__MACHINE__, ia32, hardcoded)
AC_DEFINE(_VT100_, 1, hardcoded now, but in future we need to check is it unix system)
AC_DEFINE(__DEFAULT_DISASM, 0, hardcoded now)

# Checking the size of int type
AC_CHECK_SIZEOF(size_t)
if test x$ac_cv_sizeof_size_t != x; then
  AC_DEFINE_UNQUOTED([__WORDSIZE__], $ac_cv_sizeof_size_t * 8, description)
  AH_BOTTOM([#ifndef __WORDSIZE
  #define __WORDSIZE __WORDSIZE__
  #endif])
fi

# Broken?
#AC_CHECK_HEADERS([endian.h],
# AC_TRY_COMPILE([#include ], [switch (1) { case __LITTLE_ENDIAN: break; case __BYTE_ORDER: break; } ],[is_little_endian=0], [is_little_endian=1]),
# AC_MSG_ERROR([Can't Determine Endianess])
#)
#if test $is_little_endian = 1 ; then
 #all are happy
#fi


#
# Platform specific setup
#
#############################
AC_CANONICAL_HOST

# Detect the target system
AC_MSG_CHECKING([for supported operating system])
case "${host_os}" in
    linux*)
        opsys=linux
        AC_DEFINE([LINUX], 1, [Define for the Linux operating system.])
        ;;
    cygwin*|mingw*)
        opsys=windows
        AC_DEFINE([WINDOWS], 1, [Define for the Windows operating system.])
        ;;
    darwin*)
    #   CFLAGS="$CFLAGS something"
    #   PASSING A VARIABLE FROM CONFIGURE.AC TO MAKEFILE.AM
    #   GL_LIBS="-lGLU -lGL"
    #   AC_SUBST(GL_LIBS)
    #
    #   Then in Makefile.am put:
    #
    #   myapplication_bin_LDADD=@GL_LIBS@
        AC_DEFINE([__DISABLE_SIGIO], 1, [We have problems on BSD systems with console input and segfaults on OS X])
        AC_DEFINE([OSX], 1, [Define for the Mac OS X.])
        opsys=osx
        ;;
    freebsd*|openbsd*|netbsd*)
        AC_DEFINE([BSD], 1, [Define for BSD systems.])
        opsys=bsd
        ;;
    *)
        AC_MSG_ERROR(["OS $host_os is not supported"])
        ;;
esac

AC_MSG_CHECKING([for supported architecture])
case "$host_cpu" in
i[[3456]]86|pentium)
	arch=i386
	AC_DEFINE([I386], 1, [Define for the i386 architecture.])
	;;
ia64)
	arch=ia64
	AC_DEFINE([IA64], 1, [Define for the IA64 architecture.])
	;;
m68k)
	arch=m68k
	AC_DEFINE([M68K], 1, [Define for the m68k architecture.])
	;;
sparc64*)
	arch=sparc64
	AC_DEFINE([SPARC64], 1, [Define for the SPARC64 architecture.])
	;;
sparc*)
	arch=sparc
	AC_DEFINE([SPARC], 1, [Define for the SPARC architecture.])
	;;
mips*)
	arch=mips
	AC_DEFINE([MIPS], 1, [Define for the MIPS architecture.])
	;;
alpha*)
	arch=alpha
	AC_DEFINE([ALPHA], 1, [Define for the Alpha architecture.])
	;;
powerpc*)
	arch=powerpc
	AC_DEFINE([POWERPC], 1, [Define for the PowerPC architecture.])
	;;
arm*)
	arch=arm
	AC_DEFINE([ARM], 1, [Define for the ARM architecture.])
	;;
s390)
	arch=s390
	AC_DEFINE([S390], 1, [Define for the S390 architecture.])
	;;
s390x)
	arch=s390x
	AC_DEFINE([S390X], 1, [Define for the S390x architecture.])
	;;
hppa*|parisc*)
	arch=hppa
	AC_DEFINE([HPPA], 1, [Define for the HPPA architecture.])
	;;
sh)
	arch=sh
	AC_DEFINE([SH], 1, [Define for the SH architecture.])
	;;
sh64)
	arch=sh64
	AC_DEFINE([SH64], 1, [Define for the SH64 architecture.])
	;;
x86?64*)
	arch=x86_64
	AC_DEFINE([X86_64], 1, [Define for the AMD x86-64 architecture.])
	;;
*)
	AC_MSG_RESULT([NO!])
	AC_MSG_ERROR([architecture $host_cpu is not supported])
	;;
esac
AC_MSG_RESULT($arch)

# Define $(opsys) in Makefile
AC_SUBST(opsys)
AC_SUBST(arch)

AC_DEFINE_UNQUOTED([__OS_NAME__],["$host_os"],[os name])
AC_DEFINE_UNQUOTED([__CPU_NAME__],["$arch"],[CPU name])

# Pass the conditionals to automake
AM_CONDITIONAL([LINUX], [test x$opsys = xlinux])
AM_CONDITIONAL([I386], [test x$arch = xi386])
AM_CONDITIONAL([X86_64], [test x$arch = xx86_64])
AM_CONDITIONAL([BSD], [test x$opsys = xbsd])
AM_CONDITIONAL([OSX], [test x$opsys = xosx])
AM_CONDITIONAL([WINDOWS], [test x$opsys = xwindows])





# check slang and ncurses only for unices
#if test x$opsys != xlinux; then
#AC_CHECK_LIB(nsl, main)
#fi

if test "${enable_slang}" != "no"; then
  ac_cv_my_have_slang="no"
  AC_CHECK_HEADERS(slang.h slang/slang.h,
   [ac_cv_my_have_slang="yes"
    AC_DEFINE(_SLANG_, 1, Define to 1 to activate the slang backend driver)
    AC_CHECK_LIB(slang, SLsmg_utf8_enable,
     [AC_DEFINE(HAVE_SLSMG_UTF8_ENABLE, 1, Define to 1 if you have the ‘SLsmg_utf8_enable’ function.)])
    CPPFLAGS="${CPPFLAGS} -DOPTIMISE_SLANG_PALETTE=1"
    BEYE_LIBS="${BEYE_LIBS} -lslang"
    BEYE_DRIVERS="${BEYE_DRIVERS} slang"
    break])
  if test "${ac_cv_my_have_slang}" = "no" -a "${enable_slang}" = "yes"; then
    AC_MSG_ERROR([cannot find slang development files])
  fi
fi

if test "${enable_ncurses}" != "no"; then
  ac_cv_my_have_ncurses="no"
  AC_CHECK_HEADERS(ncursesw/ncurses.h ncurses/ncurses.h ncurses.h curses.h,
   [ncurses="no"
    AC_CHECK_LIB(ncursesw, initscr,
     [ncurses="-lncursesw"],
     [AC_CHECK_LIB(ncurses, initscr,
       [ncurses="-lncurses"])])
    if test "${ncurses}" = "no"; then
      continue
    fi
    ac_cv_my_have_ncurses="yes"
    AC_DEFINE(_CURSES_, 1, Define to 1 to activate the ncurses backend driver)
    BEYE_LIBS="${BEYE_LIBS} ${ncurses}"
    BEYE_DRIVERS="${BEYE_DRIVERS} ncurses"
    dnl  Check for resizeterm or resize_term
    SAVED_LIBS="${LIBS}"
    LIBS="${LIBS} ${ncurses}"
    AC_CHECK_FUNCS(resizeterm resize_term)
    LIBS="${SAVED_LIBS}"
    break])
  if test "${ac_cv_my_have_ncurses}" = "no" -a "${enable_ncurses}" = "yes"; then
    AC_MSG_ERROR([cannot find ncurses development files])
  fi
fi

if test "${enable_iconv}" != "no"; then
  am_cv_func_iconv="no"

  # For OS X, Solaris, HP-UX, etc.: figure out if -liconv is needed.  We'll
  # accept either iconv_open or libiconv_open, since some include files map
  # the former to the latter.
  AC_SEARCH_LIBS(iconv_open, iconv)
  AC_SEARCH_LIBS(libiconv_open, iconv)

  AC_MSG_CHECKING([for iconv declaration])
  AC_CACHE_VAL(am_cv_proto_iconv, [
      AC_TRY_COMPILE([
  #include <stdlib.h>
  #include <iconv.h>
  extern
  #ifdef __cplusplus
  "C"
  #endif
  #if defined(__STDC__) || defined(__cplusplus)
  size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
  #else
  size_t iconv();
  #endif
  ], [], am_cv_proto_iconv_arg1="", am_cv_proto_iconv_arg1="const")
        am_cv_proto_iconv="extern size_t iconv (iconv_t cd, $am_cv_proto_iconv_arg1 char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);"
        am_cv_func_iconv="yes"])
      am_cv_proto_iconv=`echo "[$]am_cv_proto_iconv" | tr -s ' ' | sed -e 's/( /(/'`
  AC_MSG_RESULT([$]{ac_t:-
           }[$]am_cv_proto_iconv)
  AC_DEFINE_UNQUOTED(ICONV_CONST, $am_cv_proto_iconv_arg1,
  		   [Define as const if the declaration of iconv() needs const.])





  if test "${am_cv_func_iconv}" = "yes"; then
    AC_DEFINE(HAVE_ICONV_H, 1, Iconv present)
  fi

  if test "${am_cv_func_iconv}" = "no" -a "${enable_iconv}" = "yes"; then
    AC_MSG_ERROR([cannot find iconv development files])
  fi
fi

if test "${enable_debug}" = "yes"; then
  AC_DEFINE(DEBUG, 1, Define to 1 to activate debug)
  CFLAGS="$CFLAGS -g -O0 -ggdb"
else
  AC_DEFINE(NDEBUG, 1, Release build)
  CFLAGS="$CFLAGS -O2"
fi

AC_SUBST(BEYE_LIBS)

AC_CONFIG_FILES(
  [Makefile
   libbeye/Makefile
   tools/beyehelp/Makefile
   tools/lzss/Makefile
   testlab/file_ini/Makefile
   testlab/twin/Makefile

])

AH_BOTTOM([
/* Define the macros needed if on a UnixWare 7.x system. */
#if defined(__USLC__) && defined(__SCO_VERSION__)
#define STRICT_SYSV_CURSES /* Don't use ncurses extensions */
#endif

#endif /*Beye_CONFIG_H*/
])

AC_OUTPUT

# __CPU__=386
# -D__OS__=\"dos\"
# __MSDOS__
# __OS2__
# __DEFAULT_DISASM=0
# -DCAN_COMPILE_X86_GAS=0
# 
# __WIN32__?
# __GNUC__?
# __QNX4__
# __GLIBC__
# DJGPP
# __WATCOMC__
# __BYTE_ORDER = __LITTLE_ENDIAN, 
#__ENABLE_FASTCALL=1
#__HAVE_PRAGMA_PACK__=1
#sys/resource.h - -DHAVE_SYS_RESOURCE=1
#-DHAVE_POPEN=1
#-DHAVE_PCLOSE=

