AC_INIT([beye], [6.1.1.alpha], [dima@doty.ru])
AC_CONFIG_AUX_DIR([.auto])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_CHECK_LIB(slang, SLsmg_utf8_enable,
     [AC_DEFINE(HAVE_SLSMG_UTF8_ENABLE, 1, Define to 1 if you have the ‘SLsmg_utf8_enable’ function.)])

AC_OUTPUT


