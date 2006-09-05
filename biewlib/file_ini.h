/**
 * @namespace   biewlib
 * @file        biewlib/file_ini.h
 * @brief       This file contains prototypes of .ini file services.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1995
 * @note        Development, fixes and improvements
 * @warning     All internal functions is undocumented
 * @todo        To document internal functions
**/
#ifndef __FILE_INI_RUNTIME_SUPPORT_SYSTEM__
#define __FILE_INI_RUNTIME_SUPPORT_SYSTEM__ 1

#ifndef __BBIO_H
#include "biewlib/bbio.h"
#endif

/**
    List of possible errors that are generic
*/

#define __FI_NOERRORS      0 /**< No errors */
#define __FI_BADFILENAME  -1 /**< Can not open file */
#define __FI_TOOMANY      -2 /**< Too many opened files */
#define __FI_NOTMEM       -3 /**< Memory exhausted */
#define __FI_OPENCOND     -4 /**< Opened 'if' (missing '#endif') */
#define __FI_IFNOTFOUND   -5 /**< Missing 'if' for 'endif' statement */
#define __FI_ELSESTAT     -6 /**< Missing 'if' for 'else' statement */
#define __FI_UNRECOGN     -7 /**< Unknown '#' directive */
#define __FI_BADCOND      -8 /**< Syntax error in 'if' statement */
#define __FI_OPENSECT     -9 /**< Expected opened section or subsection or invalid string */
#define __FI_BADCHAR      -10 /**< Bad character on line (possible lost comment) */
#define __FI_BADVAR       -11 /**< Bad variable in 'set' or 'delete' statement */
#define __FI_BADVAL       -12 /**< Bad value of variable in 'set' statement */
#define __FI_NOVAR        -13 /**< Unrecognized name of variable in 'delete' statement */
#define __FI_NODEFVAR     -14 /**< Detected undefined variable (case sensitivity?) */
#define __FI_ELIFSTAT     -15 /**< Missing 'if' for 'elif' statement */
#define __FI_OPENVAR      -16 /**< Opened variable on line (use even number of '%' characters) */
#define __FI_NOTEQU       -17 /**< Lost or mismatch character '=' in assigned expression */
#define __FI_USER         -18 /**< User defined message */
#define __FI_FIUSER       -19 /**< User error */

/**
    possible answers to the errors
*/

#define __FI_IGNORE   0 /**< Ignore error and continue */
#define __FI_EXITPROC 1 /**< Terminate the program execution */

/**
    return constants for FiSearch
*/

#define __FI_NOTFOUND   0 /**< Required string is not found */
#define __FI_SECTION    1 /**< Required string is section */
#define __FI_SUBSECTION 2 /**< required string is subsection */
#define __FI_ITEM       3 /**< required string is item */

#ifdef __cplusplus
extern "C" {
#endif

typedef BGLOBAL FiHandler; /**< This is the data type used to represent ini stream objects */

/** Contains information about current record in ini file */
typedef struct tagIniInfo
{
  const char * section;      /**< section name */
  const char * subsection;   /**< subsection name */
  const char * item;         /**< item name */
  const char * value;        /**< value of item */
}IniInfo;

                   /** Pointer to a user supplied function that receive readed record from ini file.
                     * @return                For continue of scaning - False
                                              For terminating scaning - True (means: all done)
                     * @param info            pointers to current record from inni file
                    **/
typedef tBool      (__FASTCALL__ *FiUserFunc)(IniInfo * info);

/******************************************************\
* You can exchange all this pointers to self routines  *
*    and release virtual access to the methods         *
*                                                      *
* Chematic disgramm :                                  *
*                                                      *
*   FiFileProcessor   -- call --> FiStringProcessor    *
*   FiStringProcessor -- call --> FiCommandProcessor   *
*   FiCommandProcessor - call --> FiFileProcessor &    *
*                                 FiStringProcessor    *
\******************************************************/

extern  int    (__FASTCALL__ *FiError)(int nError,int row); /**< Default error handler */
extern  void   (__FASTCALL__ *FiFileProcessor)(const char *fname); /**< Default file processor */
extern  tBool  (__FASTCALL__ *FiStringProcessor)(char * curr_str); /**< Default string processor */
extern  tBool  (__FASTCALL__ *FiCommandProcessor)(const char * cmd); /**< Default command processor */
extern  tBool  (__FASTCALL__ *FiGetCondition)(const char * cond);    /**< Default processor of conditions */

#define FI_MAXSTRLEN 255 /**< Specifies maximal length of string, that can be readed from ini file */

                   /** Decodes error of ini library and return it string equivalent.
                     * @return                String, that described error
                     * @param nError          Specifies error number
                    **/
extern const char *  __FASTCALL__ FiDecodeError(int nError);
extern tBool         FiAllWantInput ; /**< Flags indicating, that all input exclude commentaries, i.e. carriage return and line feed and space characters will be returned */
extern char *        FiUserMessage;   /**< Pointer to user defined message string */
extern char          FiOpenComment;   /**< Character to be used as opening comment. @note comment always start with FiOpenComment symbol and termonated at end of line */

                   /** Creates file with error description.
                     * @return                none
                     * @param nError          Specifies error number
                     * @param row             Specifies row at which error occured
                     * @see                   FiAErrorCL
                    **/
void         __FASTCALL__ FiAError(int nError,int row);

                   /** Creates file with error description and treated error as occured at current line.
                     * @return                none
                     * @param nError          Specifies error number
                     * @see                   FiAError
                    **/
void         __FASTCALL__ FiAErrorCL(int nError); /**< error in curent line */

/********************************************************************\
* High Level procedure                                               *
*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
* Use only this function.                                            *
* This function calls UserFunc as CALLBACK routine.                  *
* All internal variables will be expanded, and command processor     *
* will be done. Unknown instruction and hotkeys should be ignored.   *
\********************************************************************/

                   /** Performs ini-file scanning.
                     * @return                none
                     * @param filename        Specifies name of file to be processed
                     * @param fuser           Specifies user-defined callback function
                     * @note                  Before calling user-defined function
                     *                        all internal variables will be expanded,
                     *                        and command processor will be done.
                     *                        Unknown instruction and hotkeys should
                     *                        be ignored.
                     * @see                   FiUserFunc
                    **/
void          __FASTCALL__ FiProgress(const char * filename,FiUserFunc fuser);

/******************************************************************\
* Low level routines                                               *
\******************************************************************/

FiHandler     __FASTCALL__ FiOpen( const char * filename );
void          __FASTCALL__ FiClose( FiHandler h );
int           __FASTCALL__ FiSearch( FiHandler h, const char * Name );
void          __FASTCALL__ FiSeekTo( FiHandler h, int nSection, int nSubSection, int nItem );
char *        __FASTCALL__ FiGetNextString( FiHandler h, char * store, unsigned int len, char *original );
unsigned int  __FASTCALL__ FiGetNumberOfSections( FiHandler h);
unsigned int  __FASTCALL__ FiGetTotalNumberOfSubSections( FiHandler h );
unsigned int  __FASTCALL__ FiGetLocalNumberOfSubSections( FiHandler h, int nSection );
unsigned int  __FASTCALL__ FiGetTotalNumberOfItems( FiHandler h);
unsigned int  __FASTCALL__ FiGetLocalNumberOfItems( FiHandler h,int nSection , int nSubSection);

tBool         __FASTCALL__ FiisSection( const char * str );
tBool         __FASTCALL__ FiisSubSection( const char * str );
tBool         __FASTCALL__ FiisItem( const char * str);
tBool         __FASTCALL__ FiisCommand( const char * str);

unsigned int  __FASTCALL__ FiGetLengthSection( const char * src );
unsigned int  __FASTCALL__ FiGetLengthSubSection( const char * src );
unsigned int  __FASTCALL__ FiGetLengthItem( const char * src );
unsigned int  __FASTCALL__ FiGetLengthValue( const char * src );
unsigned int  __FASTCALL__ FiGetLengthCommandString( const char * src );

char *        __FASTCALL__ FiGetSectionName(const char * src,char * store);
char *        __FASTCALL__ FiGetSubSectionName(const char * src, char * store);
char *        __FASTCALL__ FiGetItemName(const char * src, char * store);
char *        __FASTCALL__ FiGetValueOfItem(const char * src, char * store);
char *        __FASTCALL__ FiGetCommandString(const char * src, char * store);

void          __FASTCALL__ FiFileProcessorStd( const char * filename );
tBool         __FASTCALL__ FiStringProcessorStd( char * string );
tBool         __FASTCALL__ FiCommandProcessorStd( const char * cmd );

/**
    WORD processor
*/

typedef struct tagSTRING
{
  const char * str;
  unsigned int iptr;
}STRING;

unsigned int  __FASTCALL__ FiGetLengthBracketString( const char * src );
char *        __FASTCALL__ FiGetBracketString(const char * str, char * store);
int           __FASTCALL__ FiisLegal(const char * illegal,char c);
unsigned int  __FASTCALL__ FiGetLengthNextWord( STRING * str, const char * illegal_symbols);
char *        __FASTCALL__ FiGetNextWord( STRING * str,const char * illegal_symbols,char * store );

/**
    variables set
*/

typedef struct tagVar
{
  char * variables;
  char * associate;
  struct tagVar * next;
  struct tagVar * prev;
}Var;

typedef Var * pVar;

pVar            __FASTCALL__ FiConstructVar(const char *v,const char *a);
void            __FASTCALL__ FiDeleteVar(pVar pp);
void            __FASTCALL__ FiDeleteAllVar( void );
const char    * __FASTCALL__ FiExpandVariables(const char * var);
tBool           __FASTCALL__ FiExpandAllVar(const char * value,char * store);
void            __FASTCALL__ FiAddVariables(const char * var,const char * associate);
void            __FASTCALL__ FiRemoveVariables(const char * var);

tBool           __FASTCALL__ FiGetConditionStd( const char *condstr);

/**
    High level routines (similar to MS WIN SDK)
*/

#define HINI_FULLCACHED 0x0001
#define HINI_UPDATED    0x0002

typedef struct tag_iniProfile
{
   FiHandler     handler;
   char *        fname;
   void *        cache;
   unsigned      flags;
}hIniProfile;

/** For internal purposes */
extern void __FASTCALL__ hlFiProgress(hIniProfile *ini,FiUserFunc usrproc);

/* For public use */
                   /** Opens ini file for using with iniReadProfileString and iniWriteProfileString functions.
                     * @return                handle of opened stream
                     * @param filename        Specifies name of file to be open
                     * @param has_error       Pointer to the memory where will be stored error if occured
                     * @warning               You must not call any other function
                     *                        If error occured and has_error assigned
                     *                        non NULL value.
                     * @see                   iniCloseFile
                    **/
extern hIniProfile*    __FASTCALL__ iniOpenFile(const char *fname,tBool *has_error);

                   /** Closes ini file stream.
                     * @return                none
                     * @param ini             handle of opened stream
                     * @see                   iniOpenFile
                    **/
extern void            __FASTCALL__ iniCloseFile(hIniProfile *ini);

                   /** Performs search of given item in ini file and reads it value if found.
                     * @return                length of readed value
                     * @param ini             handle of opened stream
                     * @param section         specifies section name
                     * @param subsection      specifies subsection name
                     * @param _item           specifies item name
                     * @param def_value       specifies default return value
                     * @param buffer          specifies buffer where will be stored readed value
                     * @param cbBuffer        specifies size of buffer.
                     * @note                  if given item is not present in
                     *                        ini file, then default value will
                     *                        returned.
                     * @see                   iniWriteProfileString
                    **/
extern unsigned __FASTCALL__ iniReadProfileString(hIniProfile *ini,
                                     const char *section,
                                     const char *subsection,
                                     const char *_item,
                                     const char *def_value,
                                     char *buffer,
                                     unsigned cbBuffer);

                   /** Writes given item to ini file.
                     * @return                True if operation performed successfully
                     * @param ini             handle of opened stream
                     * @param section         specifies section name
                     * @param subsection      specifies subsection name
                     * @param item            specifies item name
                     * @param value           specifies value of item
                     * @see                   iniReadProfileString
                    **/
extern tBool __FASTCALL__ iniWriteProfileString(hIniProfile *ini,
                                     const char *section,
                                     const char *subsection,
                                     const char *item,
                                     const char *value);

#ifdef __cplusplus
}
#endif

#endif




