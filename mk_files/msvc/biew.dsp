# Microsoft Developer Studio Project File - Name="BIEW" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=BIEW - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE
!MESSAGE NMAKE /f "BIEW.mak".
!MESSAGE
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "BIEW.mak" CFG="BIEW - Win32 Debug"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "BIEW - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "BIEW - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "BIEW - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I ".\\" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__WIN32__" /D __DEFAULT_DISASM=0 /D __CPU_NAME__="i386" /D __OS_NAME__=Win32 /D __HAVE_PRAGMA_PACK__=1 /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "BIEW - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\\" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__WIN32__" /D __DEFAULT_DISASM=0 /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF

# Begin Target

# Name "BIEW - Win32 Release"
# Name "BIEW - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\addendum.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\aout.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\arch.c
# End Source File
# Begin Source File

SOURCE=.\addons\sys\ascii.c
# End Source File
# Begin Source File

SOURCE=.\addons\sys\consinfo.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\bbio.c
# End Source File
# Begin Source File

SOURCE=.\bconsole.c
# End Source File
# Begin Source File

SOURCE=.\biew.c
# End Source File
# Begin Source File

SOURCE=.\biewhelp.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\biewlib.c
# End Source File
# Begin Source File

SOURCE=.\biewutil.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\bin.c
# End Source File
# Begin Source File

SOURCE=.\bin_util.c
# End Source File
# Begin Source File

SOURCE=.\plugins\binmode.c
# End Source File
# Begin Source File

SOURCE=.\bmfile.c
# End Source File
# Begin Source File

SOURCE=.\codeguid.c
# End Source File
# Begin Source File

SOURCE=.\colorset.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\coff386.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\sysdep\ia32\cpu_info.c
# End Source File
# Begin Source File

SOURCE=.\addons\sys\cpu_perf.c
# End Source File
# Begin Source File

SOURCE=.\dialogs.c
# End Source File
# Begin Source File

SOURCE=.\addons\tools\dig_conv.c
# End Source File
# Begin Source File

SOURCE=.\plugins\disasm.c
# End Source File
# Begin Source File

SOURCE=.\editors.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\elf386.c
# End Source File
# Begin Source File

SOURCE=.\addons\tools\eval.c
# End Source File
# Begin Source File

SOURCE=.\events.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\file_ini.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\sysdep\ia32\win32\fileio.c
# End Source File
# Begin Source File

SOURCE=.\fileutil.c
# End Source File
# Begin Source File

SOURCE=.\plugins\hexmode.c
# End Source File
# Begin Source File

SOURCE=.\info_win.c
# End Source File
# Begin Source File

SOURCE=.\plugins\disasm\avr\avr.c
# End Source File
# Begin Source File

SOURCE=.\plugins\disasm\ix86\ix86.c
# End Source File
# Begin Source File

SOURCE=.\plugins\disasm\ix86\ix86_fpu.c
# End Source File
# Begin Source File

SOURCE=.\plugins\disasm\ix86\ix86_fun.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\sysdep\ia32\win32\keyboard.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\le.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\lx.c
# End Source File
# Begin Source File

SOURCE=.\mainloop.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\sysdep\ia32\win32\mmfio.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\sysdep\ia32\win32\mouse.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\dos_sys.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\lmf.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\mz.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\ne.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\nlm386.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\sysdep\ia32\win32\nls.c
# End Source File
# Begin Source File

SOURCE=.\plugins\disasm\null_da.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\opharlap.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\sysdep\ia32\win32\os_dep.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\pe.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\pharlap.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\pmalloc.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\rdoff.c
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\rdoff2.c
# End Source File
# Begin Source File

SOURCE=.\refs.c
# End Source File
# Begin Source File

SOURCE=.\plugins\nls\russian.c
# End Source File
# Begin Source File

SOURCE=.\search.c
# End Source File
# Begin Source File

SOURCE=.\setup.c
# End Source File
# Begin Source File

SOURCE=.\sysinfo.c
# End Source File
# Begin Source File

SOURCE=.\plugins\textmode.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\sysdep\ia32\win32\timer.c
# End Source File
# Begin Source File

SOURCE=.\tstrings.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\twin.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\tw_class.c
# End Source File
# Begin Source File

SOURCE=.\biewlib\sysdep\ia32\win32\vio.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\__os_dep.h
# End Source File
# Begin Source File

SOURCE=.\_hrd_inf.h
# End Source File
# Begin Source File

SOURCE=.\_sys_dep.h
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\aout64.h
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\arch.h
# End Source File
# Begin Source File

SOURCE=.\bbio.h
# End Source File
# Begin Source File

SOURCE=.\biewlib\bbio.h
# End Source File
# Begin Source File

SOURCE=.\bconsole.h
# End Source File
# Begin Source File

SOURCE=.\biewhelp.h
# End Source File
# Begin Source File

SOURCE=.\biewlib.h
# End Source File
# Begin Source File

SOURCE=.\biewlib\biewlib.h
# End Source File
# Begin Source File

SOURCE=.\biewutil.h
# End Source File
# Begin Source File

SOURCE=.\bin_util.h
# End Source File
# Begin Source File

SOURCE=.\bmfile.h
# End Source File
# Begin Source File

SOURCE=.\codeguid.h
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\coff386.h
# End Source File
# Begin Source File

SOURCE=.\disasm.h
# End Source File
# Begin Source File

SOURCE=.\plugins\disasm.h
# End Source File
# Begin Source File

SOURCE=.\editor.h
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\elf386.h
# End Source File
# Begin Source File

SOURCE=.\file_ini.h
# End Source File
# Begin Source File

SOURCE=.\biewlib\file_ini.h
# End Source File
# Begin Source File

SOURCE=.\plugins\disasm\ix86\ix86.h
# End Source File
# Begin Source File

SOURCE=.\kbd_code.h
# End Source File
# Begin Source File

SOURCE=.\biewlib\sysdep\ia32\win32\kbd_code.h
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\lmf.h
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\lx_le.h
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\ne.h
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\nlm386.h
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\pe.h
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\pharlap.h
# End Source File
# Begin Source File

SOURCE=.\biewlib\pmalloc.h
# End Source File
# Begin Source File

SOURCE=.\pmalloc.h
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\rdoff.h
# End Source File
# Begin Source File

SOURCE=.\plugins\bin\rdoff2.h
# End Source File
# Begin Source File

SOURCE=.\reg_form.h
# End Source File
# Begin Source File

SOURCE=.\setup.h
# End Source File
# Begin Source File

SOURCE=.\tstrings.h
# End Source File
# Begin Source File

SOURCE=.\biewlib\twin.h
# End Source File
# Begin Source File

SOURCE=.\twin.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
