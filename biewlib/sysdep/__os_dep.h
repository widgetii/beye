/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/__os_dep.h
 * @brief       This file contains all operating system depended part of BIEW project.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1999
 * @note        Development, fixes and improvements
 * @warning     Under some OSes program may be destroyed if more than one timer per program is created
 * @deprecated  OS File manipulating functions: I had problem with standard i/o
 *              function like: open, read, write in different C libraries such
 *              as emx and possible some one.  It cause to born this section.
**/
#ifndef __OS_DEP_H
#define __OS_DEP_H 1

#ifndef __SYS_DEP_H
#include "biewlib/_sys_dep.h"
#endif

#ifndef __BIEWLIB_H
#define __NORECURSIVE
#include "biewlib/biewlib.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* KEYBOARD handling */

#define KBD_NONSTOP_ON_MOUSE_PRESS 0x00000001L /**< Defines that \e kbdGetKey must receive mouse events as frequently as it possible. Otherwise each press on mouse button will send only one event. */

                   /** Initializes keyboard handler.
                     * @return                none
                     * @note                  You must call this function before calling any other keyboard related functions
                     * @see                   __term_keyboard
                    **/
extern void      __FASTCALL__ __init_keyboard( void );

                   /** Terminates keyboard handler.
                     * @return                none
                     * @note                  You must call this function after last call any other keyboard related functions
                     * @see                   __init_keyboard
                    **/
extern void      __FASTCALL__ __term_keyboard( void );

                   /** Receives next keyboard event or note about mouse event.
                     * @return                event. For detail see KE_* flag definitions in keycode.h file.
                     * @param flg             Indicates how to react on mouse events. See KBD_* flag definitions.
                     * @note                  This function only sends note about
                     *                        occured mouse event. For detailizing
                     *                        event you must call one of__Ms*
                     *                        function family.
                     * @see                   __kbdTestKey __kbdGetShiftKey
                    **/
extern int       __FASTCALL__ __kbdGetKey( unsigned long flg );

                   /** Checks the keyboard and mouse to determine whether there is available event.
                     * @return                event if available, 0 - otherwise. For detail see KE_* flag definitions in keycode.h file.
                     * @param flg             Indicates how to react on mouse events. See KBD_* flag definitions.
                     * @note                  This function only sends note about
                     *                        occured mouse event. For detailizing
                     *                        event you must call one of__Ms*
                     *                        function family.
                     * @see                   __kbdGetKey __kbdGetShiftKey
                    **/
extern int       __FASTCALL__ __kbdTestKey( unsigned long flg );

                   /** Determines the current SHIFT key status.
                     * @return                Current SHIFT key state. For detail see KS_* flag definitions in keycode.h file.
                     * @see                   __kbdTestKey __kbdGetKey
                    **/
extern int       __FASTCALL__ __kbdGetShiftsKey( void );

#if __WORDSIZE == 16
typedef unsigned char tAbsCoord; /**< This is the data type used to represent screen-related coordinates */
#else
typedef unsigned tAbsCoord; /**< This is the data type used to represent screen-related coordinates */
#endif

/* MOUSE handling */

#define MS_LEFTPRESS    1  /**< Defines that left button of mouse have been pressed */
#define MS_RIGHTPRESS   2  /**< Defines that middle button of mouse have been pressed */
#define MS_MIDDLEPRESS  4  /**< Defines that right button of mouse have been pressed */

                   /** Initializes mouse handler.
                     * @return                none
                     * @note                  You must call this function before calling any other mouse related functions
                     * @see                   __term_mouse
                    **/
extern int       __FASTCALL__ __init_mouse( void );

                   /** Terminates mouse handler.
                     * @return                none
                     * @note                  You must call this function after last call any other mouse related functions
                     * @see                   __init_mouse
                    **/
extern void      __FASTCALL__ __term_mouse( void );

                   /** Returns mouse cursor visibility.
                     * @return                True if mouse cursor is in visible state
                     * @see                   __MsGetPos __MsGetBtns __MsSetState
                    **/
extern tBool     __FASTCALL__ __MsGetState( void );

                   /** Sets mouse cursor visibility.
                     * @return                none
                     * @param is_visible      Indicates visibility of mouse cursor
                     * @see                   __MsGetState
                    **/
extern void      __FASTCALL__ __MsSetState( tBool is_visible );

                   /** Returns mouse position in screen coordinates.
                     * @return                none
                     * @param x,y             Pointers to memory where will be stored current mouse coordinates.
                     * @note                  Coordinates is measured in text cells of screen
                     * @see                   __MsGetBtns __MsGetState
                    **/
extern void      __FASTCALL__ __MsGetPos( tAbsCoord *x, tAbsCoord *y );

                   /** Returns mouse buttons state.
                     * @return                State of mouse button. For detail see MS_ flag definitions.
                     * @see                   __MsGetState __MsGetPos
                    **/
extern int       __FASTCALL__ __MsGetBtns( void );

/* VIDEO subsystem handling */

typedef tUInt8 ColorAttr; /**< This is the data type used to represent attributes of color */
typedef tUInt8 t_vchar;   /**< This is the data type used to represent video character */

/** Internal structure of video buffer */
typedef struct tag_tvioBuff
{
  t_vchar *   chars;       /**< Pointer to video character array */
  t_vchar *   oem_pg;      /**< Pointer to OEM pseudographics. It needed for *nix terminals */
  ColorAttr * attrs;       /**< Pointer to color attributes array */
}tvioBuff;

/*
#define __TVIO_MAXSCREENHEIGHT  100 - No dependencies from screen height.
                                      Today project support any height of
                                      screen.
*/
#if __WORDSIZE == 16
#define __TVIO_MAXSCREENWIDTH   132 /**< Defines maximal width of screen */
#else
#define __TVIO_MAXSCREENWIDTH   255 /**< Defines maximal width of screen */
#endif

#define __TVIO_FLG_DIRECT_CONSOLE_ACCESS  0x00000001L /**< Defines that video subsystem must access to console directly, if it possible */
#define __TVIO_FLG_USE_7BIT               0x00000002L /**< Defines that video subsystem must strip high bit of video characters */


                   /** Initializes video handler.
                     * @return                none
                     * @note                  You must call this function before calling any other video related functions
                     * @see                   __term_vio
                    **/
extern void      __FASTCALL__ __init_vio( unsigned long flags );

                   /** Terminates video handler.
                     * @return                none
                     * @note                  You must call this function after last call any other video related functions
                     * @see                   __init_vio
                    **/
extern void      __FASTCALL__ __term_vio( void );

#define __TVIO_CUR_OFF   0 /** Defines that cursor in invisible state */
#define __TVIO_CUR_NORM  1 /**< Defines that cursor in normal state (filles 20% of the character cell) */
#define __TVIO_CUR_SOLID 2 /**< Defines that cursor in solid state (filles 100% of the character cell) */

                   /** Returns type of cursor.
                     * @return                cursor type. For detail see __TVIO_CUR* flag definitions.
                     * @see                   __vioSetCursorType
                    **/
extern int       __FASTCALL__ __vioGetCursorType( void );

                   /** Sets type of cursor.
                     * @return                none
                     * @param c_type          Type of cursor. For detail see __TVIO_CUR* flag definitions
                     * @see                   __vioGetCursorType
                    **/
extern void      __FASTCALL__ __vioSetCursorType( int c_type );

                   /** Returns cursor position.
                     * @return                none
                     * @param x,y             pointers to memory where will be stored current cursor coordinates.
                     * @note                  coordinates of cursor is measured in text cells
                     * @see                   __vioSetCursorPos
                    **/
extern void      __FASTCALL__ __vioGetCursorPos(tAbsCoord* x,tAbsCoord* y);

                   /** Sets cursor position.
                     * @return                none
                     * @param x,y             indicate cursor coordinates.
                     * @note                  coordinates of cursor is measured in text cells
                     * @see                   __vioGetCursorPos
                    **/
extern void      __FASTCALL__ __vioSetCursorPos(tAbsCoord x,tAbsCoord y);

                   /** Rereads information about state of video subsystem.
                     * @return                none
                     * @note                  This function updates information
                     *                        about width and height of video
                     *                        subsystem, number of colors e.t.c.
                    **/
extern void      __FASTCALL__ __vioRereadState( void );

                   /** Reads buffer from console (or terminal) video memory at given offset.
                     * @return                none
                     * @param x,y             indicate x and y coordinates at which buffer must be readed
                     * @param buff            pointer to logical video buffer where will be stored readed information
                     * @param len             indicates length of buffer
                     * @note                  When program is run in terminal
                     *                        mode then information will readed from
                     *                        console emulator but not from terminal
                     *                        directly.
                     * @see                   __tvioWriteBuff
                    **/
extern void      __FASTCALL__ __vioReadBuff(tAbsCoord x,tAbsCoord y,tvioBuff *buff,unsigned len);

                   /** Writes buffer to console (or terminal) video memory at given offset.
                     * @return                none
                     * @param x,y             indicate x and y coordinates at which buffer must be readed
                     * @param buff            pointer to logical video buffer to be written
                     * @param len             indicates length of buffer
                     * @see                   __tvioReadBuff
                    **/
extern void      __FASTCALL__ __vioWriteBuff(tAbsCoord x,tAbsCoord y,const tvioBuff *buff,unsigned len);

extern tAbsCoord              tvioWidth;     /**< Contains actual width of console (or terminal) */
extern tAbsCoord              tvioHeight;    /**< Contains actual height of console (or terminal) */
extern unsigned               tvioNumColors; /**< Contains number of actual colors that used by console (or terminal) */

                   /** Sets color to be transparented
                     * @return                none
                     * @param value           indicates logical number of physical color to be transparented
                     * @note                  Function works only if current
                     *                        terminal has support of transparent
                     *                        colors. Returns immediately otherwise.
                    **/
extern void      __FASTCALL__ __vioSetTransparentColor(unsigned char value);

/* OS specific functions */
                   /** Initializes system depended part of biewlib.
                     * @return                none
                     * @note                  You must call this function before calling any other function from biewlib library
                     * @see                   __term_sys
                    **/
extern void      __FASTCALL__ __init_sys( void );

                   /** Terminates system depended part of biewlib.
                     * @return                none
                     * @note                  You must call this function after last call any other function from biewlib library
                     * @see                   __init_sys
                    **/
extern void      __FASTCALL__ __term_sys( void );

                   /** Realizes time slice between waiting of input events
                     * @return                none
                     * @note                  This function provides mechanism of
                     *                        system speedup, when application is
                     *                        waiting for input events. For realizing
                     *                        time slice inside of computing loops
                     *                        application must call SLEEP or it
                     *                        analogs.
                    **/
extern void      __FASTCALL__ __OsYield( void );

                   /** Gets ctrl-break signal
                     * @return                True if occured; False - otherwise
                     * @note                  Function does not differ soft and
                     *                        hard c-break.
                     * @warning               After getting signal by program
                                              semaphore is not being cleaned.
                    **/
extern tBool     __FASTCALL__ __OsGetCBreak( void );

                   /** Sets control-break signal
                     * @return                none
                     * @param state           indicates new state of
                                              control-break semaphore
                     * @note                  Function does not differ soft and
                     *                        hard c-break.
                    **/
extern void      __FASTCALL__ __OsSetCBreak( tBool state );

                   /** Builds OS specific name of initializing file
                     * @return                fully qualified name of .ini file
                     * @param progname        indicates name of program, that will be used as part of .ini file name
                     * @note                  Best way it always pass to function
                     *                        argv[0] program argument.
                     * @see                   __get_rc_dir
                    **/
extern char *    __FASTCALL__ __get_ini_name( const char *progname );

                   /** Builds OS specific name of program resource directory
                     * @return                Slash terminated path to program resource directory
                     * @param progname        indicates name of program.
                     * @note                  Best way it always pass to function
                     *                        argv[0] program argument.
                     * @see                   __get_ini_name
                    **/
extern char *    __FASTCALL__ __get_rc_dir( const char *progname );

typedef void timer_callback( void ); /**< This is the code type used to represent user supplied function of timer callback */

                   /** Sets user defined function as timer callback with given time interval
                     * @return                Real call back interval in milliseconds
                     * @param ms              indicates desired call back interval in milliseconds
                     * @param func            indicates user supplied function to be used as timer callback
                     * @see                   __OsRestoreTimer
                    **/
extern unsigned  __FASTCALL__ __OsSetTimerCallBack(unsigned ms,timer_callback *func);

                   /** Restores time callback function to original state
                     * @return                none
                     * @see                   __OsSetTimercallBack
                    **/
extern void      __FASTCALL__ __OsRestoreTimer(void);

                   /** Closes opened stream
                     * @return                none
                     * @param handle          handle of opened stream
                     * @see                   __OsOpen
                    **/
extern void      __FASTCALL__ __OsClose(int handle);

                   /** Changes size of opened file
                     * @return                0 if operation was succesfully performed
                     * @param handle          handle of opened file
                     * @param size            new size of file in bytes
                     * @warning               If file is truncated, the data from
                     *                        the new end of file to the original
                     *                        end of the file are lost.
                     * @see                   __OsTruncFile
                    **/
extern int       __FASTCALL__ __OsChSize(int handle, long size);

                   /** Creates new file and return handle of opened stream associated with file
                     * @return                handle of opened stream if successful, -1 otherwise
                     * @param name            name of new file to be created
                     * @note                  Function creates file with read/write
                     *                        access by user.
                     * @see                   __OsOpen __OsClose __OsDelete
                    **/
extern int       __FASTCALL__ __OsCreate(const char *name);

                   /** Duplicates an opened file handle by assigning the same file to a new handle.
                     * @return                handle of new opened stream if successful, -1 otherwise
                     * @param name            opened handle to be duplicated
                     * @note                  Function duplicates handle with
                     *                        same characteristics as opened handle.
                    **/
extern int       __FASTCALL__ __OsDupHandle(int handle);

                   /** Deletes the file.
                     * @return                0 if operation was succesfully performed
                     * @param name            indicates file to be deleted
                     * @see                   __OsCreate __OsRename
                    **/
extern int       __FASTCALL__ __OsDelete(const char *name);

#define FO_READONLY           0x0000 /**< Defines flag of file opening, that indicates opening in read-only mode */
#define FO_WRITEONLY          0x0001 /**< Defines flag of file opening, that indicates opening in write-only mode */
#define FO_READWRITE          0x0002 /**< Defines flag of file opening, that indicates opening in read-write mode */

#define SO_COMPAT             0x0000 /**< Defines flag of file sharing, that indicates opening in compatibility mode */
#define SO_DENYALL            0x0010 /**< Defines flag of file sharing, that indicates opening in mode, which denies any access to file by other processes */
#define SO_DENYWRITE          0x0020 /**< Defines flag of file sharing, that indicates opening in mode, which denies write access to file by other processes */
#define SO_DENYREAD           0x0030 /**< Defines flag of file sharing, that indicates opening in mode, which denies read access to file by other processes */
#define SO_DENYNONE           0x0040 /**< Defines flag of file sharing, that indicates opening in mode, which allows any access to file by other processes */
#define SO_PRIVATE            0x0080 /**< Defines flag of file sharing, that indicates opening in mode, which denies any access to file by child processes */

                   /** Opens existed file.
                     * @return                handle of opened stream if successful, -1 otherwise
                     * @param name            name of existed file to be opened
                     * @param mode            combination of FO_* and SO_* flags
                     * @see                   __OsCreate __OsClose
                    **/
extern int       __FASTCALL__ __OsOpen(const char *name,int mode);

                   /** Checks whether the specified file exists
                     * @return                True if specified file is exists
                     * @param name            file being checked
                    **/
extern tBool  __FASTCALL__    __IsFileExists(const char *name);

                   /** Returns length of file in bytes
                     * @return                length of file if successful; 0 - otherwise
                     * @param handle          handle of opened stream
                    **/
extern long   __FASTCALL__    __FileLength(int handle);

#define SEEKF_START           (int)0  /**< Defines references location of computing file offset from beginning of file */
#define SEEKF_CUR             (int)1  /**< Defines references location of computing file offset from current position */
#define SEEKF_END             (int)2  /**< Defines references location of computing file offset from end of file */

                   /** Positions file pointer.
                     * @return                offset from beginning of file to new pointer position in bytes
                     * @param handle          handle of opened stream
                     * @param newpos          indicates number of bytes the new position is displaced from origin
                     * @param origin          indicates reference location from which an offset will be computed
                     * @see                   __OsTell
                    **/
extern long   __FASTCALL__    __OsSeek(int handle, long newpos, int origin);

                   /** Returns current file position
                     * @return                offset from beginning of file to file position in bytes
                     * @param handle          handle of opened stream
                     * @see                   __OsSeek
                    **/
extern long   __FASTCALL__    __OsTell(int handle);

                   /** Truncates of opened file
                     * @return                0 if operation was succesfully performed
                     * @param handle          handle of opened file
                     * @param newsize         new size of file in bytes
                     * @depricated            Different C libraries have different
                     *                        methods for changing size of file
                     *                        and in many case it undocumented
                     * @see                   __OsChSize
                    **/
extern int       __FASTCALL__ __OsTruncFile(int handle, unsigned long newsize);

                   /** Changes name of specified file or directory
                     * @return                0 if operation was succesfully performed
                     * @param oldname         indicates name of an existing file or directory
                     * @param newname         indicates new name of file or directory
                     * @see                   __OsDelete
                    **/
extern int       __FASTCALL__ __OsRename(const char *oldname,const char *newname);

                   /** Reads specified number of bytes from file
                     * @return                number actually readed bytes if successful; -1 on error
                     * @param handle          handle of opened stream
                     * @param buff            pointer to memory where will be stored readed information
                     * @param size            specifies number of bytes to be readed from file
                     * @note                  Function reads specified number of bytes
                     *                        from current file position. File
                     *                        position is changed after reading
                     *                        on specified number of bytes.
                     * @see                   __OsWrite
                    **/
extern int       __FASTCALL__ __OsRead(int handle,void *buff,unsigned size);

                   /** Writes specified number of bytes to file
                     * @return                number actually writed bytes if successful; -1 on error
                     * @param handle          handle of opened stream
                     * @param buff            pointer to memory to be written
                     * @param size            specifies number of bytes to be writed to file
                     * @note                  Function writes specified number of bytes
                     *                        at current file position. File
                     *                        position is changed after writing
                     *                        on specified number of bytes.
                     * @see                   __OsRead
                    **/
extern int       __FASTCALL__ __OsWrite(int handle,const void *buff,unsigned size);

/** Structure for storing and setting file time information */
typedef struct tagFTime
{
   unsigned long acctime; /**< contains last access time */
   unsigned long modtime; /**< constains modification time */
}FTime;
                   /** Gets last access and modification time for given file.
                     * @return                True if success False otherwise
                     * @param name            specified name of file
                     * @param data            pointer to memory where information will be stored
                     * @see                   __OsSetFTime
                    **/
extern tBool      __FASTCALL__ __OsGetFTime(const char *name,FTime *data);

                   /** Sets last access and modification time for given file.
                     * @return                True if success False otherwise
                     * @param name            specified name of file
                     * @param data            pointer to memory where information is stored
                     * @see                   __OsGetFTime
                    **/
extern tBool      __FASTCALL__ __OsSetFTime(const char *name,const FTime *data);

/*
   First edition of Memory Mapped File Support (For modern OS).
   Note: MMF does not support preffered memory addresses and many other features
         of standard mmap technology.
   In this release it would be better to use this technique only for read only
   operations.
*/

typedef void * mmfHandle;

                   /** Opens existed file and mapped it into memory.
                     * @return                handle of opened stream if successful, NULL otherwise
                     * @param name            name of existed file to be opened
                     * @param mode            combination of FO_* and SO_* flags
                     * @see                   __mmfClose
                    **/
mmfHandle          __FASTCALL__ __mmfOpen(const char *fname,int mode);

                   /** Flushes memory mapped file to the physical file.
                     * @return                True if successful, False otherwise
                     * @param mh              handle of opened stream
                     * @see                   __mmfOpen __mmfClose __mmfSync
                    **/
tBool              __FASTCALL__ __mmfFlush(mmfHandle mh);

                   /** Synchronizes memory mapped file with the physical file.
                     * @param mh              handle of opened stream
                     * @return                mh if successful, NULL otherwise.
                     * @note                  This function performs both operations
                                              writting to the file of dirty
                                              areas and rereading of the file
                                              back into the memory. On failure
                                              it closed all handles and freed
                                              mmfHandle structure.
                     * @see                   __mmfFlush
                    **/
mmfHandle          __FASTCALL__ __mmfSync(mmfHandle mh);

                   /** Changes access to the memory mapping of file.
                     * @return                True if successful, False otherwise
                     * @param mh              handle of opened stream
                     * @param flags           combination of FO_* and SO_* flags
                     * @see                   __mmfOpen __mmfClose
                    **/
tBool              __FASTCALL__ __mmfProtect(mmfHandle mh,int flags);

                   /** Changes size of existed file mapping.
                     * @return                True if successful, False otherwise
                     * @param mh              handle of opened stream
                     * @param size            new size of mapping in bytes
                     * @note                  May be not supported by some OS.
                     * @warning               If mapping is truncated, the data from
                     *                        the new end of file to the original
                     *                        end of the file are lost.
                     * @see                   __mmfOpen __mmfClose __mmfProtect
                    **/
tBool              __FASTCALL__ __mmfResize(mmfHandle mh,long size);

                   /** Closes existed file mapping.
                     * @return                None
                     * @param mh              handle of stream to be closed
                     * @see                   __mmfOpen
                    **/
void               __FASTCALL__ __mmfClose(mmfHandle mh);

                   /** Return address in memory where file mapping is assigned.
                     * @return                Assigned address of file mapping or -1 on error
                     * @param mh              handle of stream to be closed
                     * @see                   __mmfOpen
                    **/
void *             __FASTCALL__ __mmfAddress(mmfHandle mh);

                   /** Returns size of opened MMF stream.
                     * @return                Length in bytes of opened MMF stream
                     * @param mh              handle of stream
                     * @see                   __mmfOpen __mmfAddress
                    **/
long              __FASTCALL__ __mmfSize(mmfHandle mh);

                   /** Returns workability of MMF subsystem.
                     * @return                True if mmf is workable on given OS
                    **/
tBool             __FASTCALL__ __mmfIsWorkable( void );

/* National Language Support */

           /** Checks whether the specified character is OEM pseudographical symbol */
#define NLS_IS_OEMPG(ch) (((unsigned char)ch) >= 0xB0 && ((unsigned char)ch) <= 0xDF)

                   /** Prepares tvioBuff buffer from OEM codepage to currently used by OS.
                     * @return                none
                     * @param it              buffer to be converted
                     * @param size            size of buffer elemets in bytes
                     * @see                   __nls_OemToOsdep __nls_CmdlineToOem NLS_IS_OEMPG
                    **/
extern void      __FASTCALL__ __nls_PrepareOEMForTVio(tvioBuff *it,unsigned size);

                   /** Converts buffer from OEM codepage to currently used by OS.
                     * @return                none
                     * @param str             buffer to be converted
                     * @param size            size of buffer in bytes
                     * @see                   __nls_CmdlineToOem __nls_CmdlineToFs
                    **/
extern void      __FASTCALL__ __nls_OemToOsdep(unsigned char *str,unsigned size);

                   /** Converts buffer from codepage of command line to currently used by OS.
                     * @return                none
                     * @param str             buffer to be converted
                     * @param size            size of buffer in bytes
                     * @depricated            This function only used in Win32
                     *                        where codepages of console and command
                     *                        line is differ.
                     * @see                   __nls_OemToOsdep __nls_CmdlineToFs
                    **/
extern void      __FASTCALL__ __nls_CmdlineToOem(unsigned char *str,unsigned size);

                   /** Converts buffer from OEM codepage to currently used by OS's file system.
                     * @return                none
                     * @param str             buffer to be converted
                     * @param size            size of buffer in bytes
                     * @see                   __nls_OemToOsdep __nls_CmdlineToOem
                    **/
extern void      __FASTCALL__ __nls_OemToFs(unsigned char *str,unsigned size);

#ifdef __cplusplus
}
#endif

#endif
