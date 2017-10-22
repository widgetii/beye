SUBDIRS = libbeye

beye_src = addendum.c bconsole.c beye.c beyehelp.c beyeutil.c bin_util.c bmfile.c codeguid.c colorset.c dialogs.c editors.c events.c fileutil.c info_win.c mainloop.c refs.c search.c setup.c sysinfo.c tstrings.c

beye_headers = bbconfig.h console.h beyehelp.h beyeutil.h bin_util.h bmfile.h bswap.h codeguid.h colorset.h editor.h reg_form.h search.h setup.h tstrings.h

beye_addons_sys = addons/sys/ascii.c addons/sys/consinfo.c addons/sys/cpu_perf.c addons/sys/inview.c
beye_addons_tools = addons/tools/dig_conv.c addons/tools/eval.c

beye_plugins = plugins/binmode.c plugins/disasm.c plugins/hexmode.c plugins/textmode.c 
beye_plugins_h = lugins/disasm.h plugins/hexmode.h plugins/textmode.h
beye_plugins_bin = plugins/bin/aout.c plugins/bin/arch.c plugins/bin/asf.c plugins/bin/avi.c plugins/bin/bin.c plugins/bin/bmp.c plugins/bin/coff386.c plugins/bin/dos_sys.c plugins/bin/elf386.c plugins/bin/jpeg.c plugins/bin/jvmclass.c plugins/bin/le.c plugins/bin/lmf.c plugins/bin/lx.c plugins/bin/mov.c plugins/bin/mp3.c plugins/bin/mpeg.c plugins/bin/mz.c plugins/bin/ne.c plugins/bin/nlm386.c plugins/bin/opharlap.c plugins/bin/pe.c plugins/bin/pharlap.c plugins/bin/rdoff.c plugins/bin/rdoff2.c plugins/bin/realmedia.c plugins/bin/sis.c plugins/bin/sisx.c plugins/bin/wav.c
beye_plugins_bin_h = plugins/bin/aout64.h plugins/bin/arch.h plugins/bin/coff386.h plugins/bin/dos_sys.h plugins/bin/elf386.h plugins/bin/lmf.h plugins/bin/lx_le.h plugins/bin/mmio.h plugins/bin/mz.h plugins/bin/ne.h plugins/bin/nlm386.h plugins/bin/pe.h plugins/bin/pharlap.h plugins/bin/rdoff.h plugins/bin/rdoff2.h
beye_plugins_nls = plugins/nls/russian.c
beye_plugins_disasm = plugins/disasm/null_da.c plugins/disasm/arm/arm.c plugins/disasm/arm/arm16.c plugins/disasm/arm/arm32.c plugins/disasm/arm/arm.h plugins/disasm/avr/avr.c plugins/disasm/avr/avr.h plugins/disasm/ix86/ix86.c plugins/disasm/ix86/ix86_fpu.c plugins/disasm/ix86/ix86_fun.c plugins/disasm/ix86/ix86.h plugins/disasm/java/java.c plugins/disasm/ppc/ppc.c plugins/disasm/ppc/ppc.h

LDADD = libbeye/libbeye.a $(BEYE_LIBS)
AM_CPPFLAGS = -I$(srcdir)/libbeye

bin_PROGRAMS = beye
beye_SOURCES = $(beye_src) $(beye_headers) $(beye_addons_sys) $(beye_addons_tools) $(beye_plugins) $(beye_plugins_h) $(beye_plugins_bin) $(beye_plugins_bin_h) $(beye_plugins_nls) $(beye_plugins_disasm) 

