/**
 * @namespace   biewlib
 * @file        biewlib/bbio.h
 * @brief       This file contains prototypes of BBIO technology functions.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
**/
#ifndef __BBIO_H
#define __BBIO_H 1

#ifndef __BIEWLIB_H
#include "biewlib/biewlib.h"
#endif
/******************************************************************\
*  Buffered binary file streams input/output section               *
*  Helpful for read/write small size objects from/to file          *
\******************************************************************/

                         /** FORWARD: default (forward scan)
                             reposition of cache as 100% forward
                             from current file position */
#define BIO_OPT_FORWARD  0x0000
#define BIO_OPT_DB       BIO_OPT_FORWARD
                         /** RANDOM: middle scan
                             reposition of cache as 50% forward & 50%
                             backward from current file position */
#define BIO_OPT_RANDOM   0x0001
                         /** BACKWARD: backward scan
                             reposition of cache as 100% backward
                             from current file position */
#define BIO_OPT_BACKSCAN 0x0002
                         /** RANDOM FORWARD: reposition of cache as 90% forward
                             & 10% backward from current file position */
#define BIO_OPT_RFORWARD 0x0003
                         /** RANDOM BACKWARD: reposition of cache as 10% forward
                             & 90% backward from current file position */
#define BIO_OPT_RBACKSCAN 0x0004
#define BIO_OPT_DIRMASK  0x000F /**< direction mask */
#define BIO_OPT_USEMMF   0xFFFF /**< Use mmf instead buffering i/o. This covers all optimizations */
#define BIO_OPT_NOCACHE  0x8000 /**< disable cache */

#define BIO_SEEK_SET     SEEKF_START /**< specifies reference location from begin of file */
#define BIO_SEEK_CUR     SEEKF_CUR   /**< specifies reference location from current position of file */
#define BIO_SEEK_END     SEEKF_END   /**< specifies reference location from end of file */

typedef void * BGLOBAL;       /**< This is the data type used to represent buffered stream objects */

/*
   This struct is ordered as it documented in Athlon manual
   Publication # 22007 Rev: D
*/
/** Virtual file buffer structure */
typedef struct tagvfb
{
 int             handle;    /**< file handle */
 __filesize_t    FBufStart; /**< logical position of mirror the buffer onto file */
 char *          MBuffer;   /**< NULL - not buffered i/o */
 unsigned        MBufLen;   /**< length data, actually contains in buffer */
 unsigned        MBufSize;  /**< real size of buffer */
 tBool           updated;   /**< True if buffer contains data, that not pesent in file */
}vfb;

/** Memory mapped buffer structure */
typedef struct tagmmb
{
 mmfHandle       mmf;       /**< If OS support MMF contains handle of memory-mapped file */
 void *          mmf_addr;  /**< If OS support MMF contains base address of memory where file is mapped */
}mmb;

typedef struct tagBFILE
{
 __filesize_t    FilePos;   /**< current logical position in file */
 __filesize_t    FLength;   /**< real length of the file */
 char *          FileName;  /**< Real file name of opened file */
 unsigned        openmode;  /**< mode,that OsOpen this file */
 int             optimize;  /**< seek optimization */
 tBool           is_mmf;    /**< indicates that 'mmb' is used */
 tBool           primary_mmf; /**< If this is set then we have not duplicated handle */
 union                      /**< cache subsystem */
 {
    vfb          vfb;       /**< buffered file */
    mmb *        mmb;       /**< Pointer to memory mapped file. We must have pointer, but not object!!! It for bioDupEx */
 }b;
 tBool           is_eof;    /**< Indicates EOF for buffering streams */
}BFILE;

extern struct tagBFILE bNull; /**< Stream associated with STDERR */

                   /** Opens existed file and buffered it
                     * @return                handle of stream
                     * @param fname           indicates name of file to be open
                     * @param openmode        indicates opening mode flags - BIO_*
                     * @param buffSize        indicates size of buffer. Value UINT_MAX indicates - buffering entire file.
                     * @note                  Returns bNull if opening is fail.
                     * @warning               Carefully use parameter - buffSize.
                     *                        If you created new file with 0 bytes
                     *                        of length, and value of buffSize =
                     *                        UINT_MAX, then i/o will be unbuffered.
                     *                        It better to use values < UINT_MAX.
                     *                        Value UINT_MAX better to use for readonly
                     *                        operations for small files to load those into
                     *                        memory entire.
                     * @see                   bioClose
                    **/
BGLOBAL               __FASTCALL__ bioOpen(const char * fname,unsigned openmode,unsigned buffSize,unsigned optimization);

                   /** Changes size of opened file.
                     * @return                True if operation was succesfully performed
                     * @param bioFile         handle of opened stream
                     * @param newsize         new size of file in bytes
                     * @warning               If file is truncated, the data from
                     *                        the new end of file to the original
                     *                        end of the file are lost.
                    **/
tBool                 __FASTCALL__ bioChSize(BGLOBAL bioFile,__filesize_t newsize);

                   /** Closes opened stream.
                     * @return                True if operation was succesfully performed
                     * @param bioFile         handle of opened stream
                     * @see                   bioOpen
                    **/
tBool                 __FASTCALL__ bioClose(BGLOBAL bioFile);

                   /** Determines whether a opened stream has reached the End of File.
                     * @return                True if EOF has reached
                     * @param bioFile         handle of opened stream
                    **/
tBool                 __FASTCALL__ bioEOF(BGLOBAL bioHandle);

                   /** Returns the length (in bytes) of file associated with opened stream.
                     * @return                file length
                     * @param bioFile         handle of opened stream
                    **/
__filesize_t         __FASTCALL__ bioFLength(BGLOBAL bioFile);

                   /** Flushes buffer onto disk.
                     * @return                True if operation was succesfully performed
                     * @param bioFile         handle of opened stream
                     * @note                  This operation performs automatically
                     *                        when bioClose is called and
                     *                        openmode == READWRITE || WRITE, or
                     *                        when bioSeek is called and logical
                     *                        file position is out of buffer.
                     * @see                   bioReRead
                    **/
tBool                 __FASTCALL__ bioFlush(BGLOBAL bioFile);

                   /** Reads one byte from stream.
                     * @return                Readed byte
                     * @param bioFile         handle of opened stream
                     * @note                  Logical file position is
                     *                        incremented after operation.
                     * @see                   bioWriteByte bioReadWord bioReadDWord bioReadBuffer
                    **/
tUInt8               __FASTCALL__ bioReadByte(BGLOBAL bioFile);

                   /** Reads two bytes from stream.
                     * @return                Readed word
                     * @param bioFile         handle of opened stream
                     * @note                  Logical file position is
                     *                        incremented after operation.
                     * @see                   bioWriteWord bioReadByte bioReadDWord bioReadBuffer
                    **/
tUInt16              __FASTCALL__ bioReadWord(BGLOBAL bioFile);

                   /** Reads four bytes from stream.
                     * @return                Readed double word
                     * @param bioFile         handle of opened stream
                     * @note                  Logical file position is
                     *                        incremented after operation.
                     * @see                   bioWriteDWord bioReadByte bioReadWord bioReadBuffer
                    **/
tUInt32              __FASTCALL__ bioReadDWord(BGLOBAL bioFile);

                   /** Reads 8 bytes from stream.
                     * @return                Readed double word
                     * @param bioFile         handle of opened stream
                     * @note                  Logical file position is
                     *                        incremented after operation.
                     * @see                   bioWriteDWord bioReadByte bioReadWord bioReadBuffer
                    **/
tUInt64              __FASTCALL__ bioReadQWord(BGLOBAL bioFile);

                   /** Reads specified number of bytes from stream.
                     * @return                True if operation was succesfully performed
                     * @param bioFile         handle of opened stream
                     * @param buffer          specifies buffer, where readed information will be stored
                     * @param cbBuffer        specifies size of buffer
                     * @note                  Function increments logical file
                     *                        position by the number of bytes read.
                     * @see                   bioWriteBuffer bioReadByte bioReadWord bioReadByte
                    **/
tBool                 __FASTCALL__ bioReadBuffer(BGLOBAL bioFile,void * buffer,unsigned cbBuffer);

                   /** Rereads opened file from disk.
                     * @return                True if operation was succesfully performed
                     * @param bioFile         handle of opened stream
                     * @see                   bioFlush
                    **/
tBool                 __FASTCALL__ bioReRead(BGLOBAL bioFile);

                   /** Positions logical file pointer at the specified position.
                     * @return                True if operation was succesfully performed
                     * @param bioFile         handle of opened stream
                     * @param offset          specifies new offset of file pointer
                     * @param origin          specifies reference location from which offset will be computed
                     * @see                   bioTell BIO_SEEK_SET BIO_SEEK_CUR BIO_SEEK_END
                    **/
tBool                 __FASTCALL__ bioSeek(BGLOBAL bioFile,__fileoff_t offset,int origin);

                   /** Returns current optimization of buffering.
                     * @return                optimization (BIO_OPT_*)
                     * @param bioFile         handle of opened stream
                     * @see                   bioSetOptimization
                    **/
unsigned              __FASTCALL__ bioGetOptimization(BGLOBAL bioFile);

                   /** Sets new optimization of buffering and returns previous.
                     * @return                optimization (BIO_OPT_*)
                     * @param bioFile         handle of opened stream
                     * @see                   bioGetOptimization
                    **/
unsigned              __FASTCALL__ bioSetOptimization(BGLOBAL bioFile,unsigned flags);

                   /** Returns logical file position of opened stream.
                     * @return                offset from begin of file
                     * @param bioFile         handle of opened stream
                     * @see                   bioSeek
                    **/
__filesize_t         __FASTCALL__ bioTell(BGLOBAL bioFile);

                   /** Writes one byte to stream.
                     * @return                True if operation was succesfully performed
                     * @param bioFile         handle of opened stream
                     * @param bVal            Byte to be written
                     * @note                  Logical file position is
                     *                        incremented after operation.
                     * @see                   bioReadByte bioWriteWord bioWriteWord bioWriteBuffer
                    **/
tBool                 __FASTCALL__ bioWriteByte(BGLOBAL bioFile,tUInt8 bVal);

                   /** Writes two bytes to stream.
                     * @return                True if operation was succesfully performed
                     * @param bioFile         handle of opened stream
                     * @param wVal            Word to be written
                     * @note                  Logical file position is
                     *                        incremented after operation.
                     * @see                   bioReadWord bioWriteWord bioWriteWord bioWriteBuffer
                    **/
tBool                 __FASTCALL__ bioWriteWord(BGLOBAL bioFile,tUInt16 wVal);

                   /** Writes four bytes to stream.
                     * @return                True if operation was succesfully performed
                     * @param bioFile         handle of opened stream
                     * @param dwVal           Double word to be written
                     * @note                  Logical file position is
                     *                        incremented after operation.
                     * @see                   bioReadDWord bioWriteWord bioWriteWord bioWriteBuffer
                    **/
tBool                 __FASTCALL__ bioWriteDWord(BGLOBAL bioFile,tUInt32 dwVal);

                   /** Writes 8 bytes to stream.
                     * @return                True if operation was succesfully performed
                     * @param bioFile         handle of opened stream
                     * @param dwVal           Double word to be written
                     * @note                  Logical file position is
                     *                        incremented after operation.
                     * @see                   bioReadDWord bioWriteWord bioWriteWord bioWriteBuffer
                    **/
tBool                 __FASTCALL__ bioWriteQWord(BGLOBAL bioFile,tUInt64 dwVal);

                   /** Writes specified number of bytes opened to stream.
                     * @return                True if operation was succesfully performed
                     * @param bioFile         handle of opened stream
                     * @param buffer          specifies buffer to be written
                     * @param cbBuffer        specifies size of buffer
                     * @note                  Function increments logical file
                     *                        position by the number of bytes writed.
                     * @see                   bioReadBuffer bioWriteWord bioWriteWord bioByte
                    **/
tBool                 __FASTCALL__ bioWriteBuffer(BGLOBAL bioFile,const void * buffer,unsigned cbBuffer);

                   /** Returns name of file associated with opened stream.
                     * @return                name of file
                     * @param bioFile         handle of opened stream
                    **/
char *                __FASTCALL__ bioFileName(BGLOBAL bioFile);

                   /** Causes opened stream to be duplicated.
                     * @return                handle of duplicted stream
                     * @param bioFile         handle of opened stream
                     * @note                  function duplicates OS handle
                     *                        of stream and buffer with all
                     *                        characteristics.
                     * @see                   bioDupEx
                    **/
BGLOBAL               __FASTCALL__ bioDup(BGLOBAL bioFile);

                   /** Causes opened stream to be duplicated.
                     * @return                handle of duplicted stream
                     * @param bioFile         handle of opened stream
                     * @param buffSize        specifies new size of buffer to be used with duplicated stream
                     * @note                  function duplicates OS handle
                     *                        of stream and buffer with all
                     *                        possible characteristics.
                     * @see                   bioDup
                    **/
BGLOBAL               __FASTCALL__ bioDupEx(BGLOBAL bioFile,unsigned buffSize);

                   /** Returns low-level OS handle of opened stream.
                     * @return                OS handle of opened stream
                     * @param bioFile         handle of opened stream
                    **/
int                   __FASTCALL__ bioHandle(BGLOBAL bioFile);

                   /** Returns pointer to buffer of opened stream.
                     * @return                pointer to buffer
                     * @param bioFile         handle of opened stream
                     * @note                  This function allowes direct
                     *                        access to file cache.
                     * @see                   bioBuffLen bioBuffPos
                    **/
void *                __FASTCALL__ bioBuffer(BGLOBAL bioFile);

                   /** Returns length of opened stream buffer.
                     * @return                length of buffer
                     * @param bioFile         handle of opened stream
                     * @note                  This function allowes direct
                     *                        access to file cache.
                     * @see                   bioBuff bioBuffPos
                    **/
unsigned              __FASTCALL__ bioBuffLen(BGLOBAL  bioFile);

                   /** Returns logical buffer position.
                     * @return                length of buffer
                     * @param bioFile         handle of opened stream
                     * @note                  This function allowes direct
                     *                        access to file cache.
                     * @warning               Logical buffer position is not
                     *                        logical file position.
                     * @see                   bioBuff bioBuffLen
                    **/
unsigned              __FASTCALL__ bioBuffPos(BGLOBAL bioFile);

#endif

