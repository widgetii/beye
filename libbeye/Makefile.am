modules_unix_common = sysdep/generic/posix/fileio.c sysdep/generic/unix/con_vt100.c sysdep/generic/unix/os_dep.c sysdep/generic/unix/nls.c sysdep/generic/cpu_info.c

modules_unix = sysdep/generic/unix/keyboard.c sysdep/generic/unix/vio.c sysdep/ia16/dos/mmfio.c sysdep/generic/unix/misc.c

modules_linux = sysdep/generic/linux/keyboard.c sysdep/generic/linux/vio.c sysdep/generic/linux/mmfio.c sysdep/generic/linux/misc.c

modules_windows = sysdep/generic/windows/fileio.c sysdep/generic/windows/kbd_code.h sysdep/generic/windows/keyboard.c sysdep/generic/windows/misc.c sysdep/generic/windows/mmfio.c sysdep/generic/windows/mouse.c sysdep/generic/windows/nls.c sysdep/generic/windows/os_dep.c sysdep/generic/windows/timer.c sysdep/generic/windows/vio.c sysdep/generic/cpu_info.c

if LINUX
  modules = $(modules_unix_common) $(modules_linux)
endif

if BSD
  modules = $(modules_unix_common) $(modules_unix)
endif

if WINDOWS
  modules = $(modules_windows)
endif

noinst_LIBRARIES = libbeye.a
libbeye_a_SOURCES = bbio.c beyelib.c file_ini.c pmalloc.c tw_class.c twin.c bbio.h beyelib.h file_ini.h kbd_code.h pmalloc.h tw_class.h twin.h $(modules)
