dnl ***********************************************
dnl * Please run autoreconf to test your changes! *
dnl ***********************************************

# Set VERSION so we only need to edit in one place (i.e., here)
m4_define(BEYE_VERSION, [6.1.1.alpha])

AC_INIT([beye], BEYE_VERSION, [https://github.com/widgetii/beye])
AC_CONFIG_AUX_DIR([.auto])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_HEADERS([bconfig.h])
AC_CONFIG_FILES([Makefile])

# Check for programs
AC_PROG_CC
AC_PROG_INSTALL

dnl This is for stuff that absolutely must end up in pyconfig.h.
dnl Please use pyport.h instead, if possible.
AH_TOP([
#ifndef Beye_CONFIG_H
#define Beye_CONFIG_H
])
AH_BOTTOM([
/* Define the macros needed if on a UnixWare 7.x system. */
#if defined(__USLC__) && defined(__SCO_VERSION__)
#define STRICT_SYSV_CURSES /* Don't use ncurses extensions */
#endif

#endif /*Beye_CONFIG_H*/
])

dnl  output driver features
AC_ARG_ENABLE(slang,
  [  --enable-slang          slang graphics support (autodetected)])
AC_ARG_ENABLE(ncurses,
  [  --enable-ncurses        ncurses graphics support (autodetected)])

dnl  conditional builds
AC_ARG_ENABLE(debug,
  [  --enable-debug          build debug versions of the library (default no)])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h malloc.h iconv.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/time.h termios.h unistd.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([atexit ftruncate memmove memset munmap select strcasecmp strchr strcspn strdup strerror strncasecmp strpbrk strrchr strspn strstr strtol strtoul strtoull sysinfo utime])




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

if test "${enable_debug}" = "yes"; then
  AC_DEFINE(DEBUG, 1, Define to 1 to activate debug)
fi


AC_OUTPUT

# include <biewlib/sysdep/__MACHINE__/__config.h
# __WORDSIZE = 16, 32, 64
# __WIN32__?
# __GNUC__?
# __QNX4__
# __GLIBC__
# __UNIX__
# DJGPP
# __WATCOMC__

# __BYTE_ORDER = __LITTLE_ENDIAN, 
