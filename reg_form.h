/**
 * @namespace   biew
 * @file        reg_form.h
 * @brief       This file contains structure prototypes for embededding new
 *              file formats, disassemblers, translation modes e.t.c. in BIEW.
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
**/
#ifndef __REG_FORM__H
#define __REG_FORM__H

#ifndef __FILE_INI_RUNTIME_SUPPORT_SYSTEM__
#include "biewlib/file_ini.h"
#endif

#ifndef __TWIN_H
#include "biewlib/twin.h"
#endif

#ifdef __cplusplus
#define extern "C" {
#endif

typedef unsigned long __FASTCALL__ (*BinFunc)( void );
typedef tBool         __FASTCALL__ (*ModFunc)( void );

#define APREF_NORMAL      0x0000 /**< Append references in short form if it really present in binary */
#define APREF_USE_TYPE    0x0001 /**< Append references in detail form if it really present in binary */
#define APREF_TRY_LABEL   0x0002 /**< Append references in short form even if it not present in binary (smart method) */
#define APREF_SAVE_VIRT   0x0004 /**< Notifies plugin about preserving of virtual address, if binding is local */
#define APREF_TRY_PIC     0x0008 /**< Append references in short form assuming that shift is offset in .GOT table where references are binded */

/**
   Appends dissasembler reference to string.
   * @param str          string buffer for append to
   * @param shift        physical address of field, that required of binding
   * @param flags        see above
   * @param codelen      length of field, that required binding
   * @param r_shift      used only if APPREF_TRY_LABEL mode is set, contains real value of field, that required binding
   * @return             one of RAPREF_* constants (see biewutil.h file for detail)
*/
typedef unsigned long __FASTCALL__ (*AppRefs)(char *str,unsigned long shift,int flags,int codelen,unsigned long r_shift);

/***************************************************************\
*  This form registry binary file formats                       *
\***************************************************************/

/** List of DisAssembler Bitness */

#define DAB_USE16   0
#define DAB_USE32   1
#define DAB_USE64   2
#define DAB_USE128  3
#define DAB_USE256  4
#define DAB_AUTO    0xFFFFU /**< never must return! Only for selection */

/** Public symbols classes */

#define SC_LOCAL    0 /**< means: present as entry but not exported */
#define SC_GLOBAL   1 /**< means: exported entry point */

/** object classes */

#define OC_CODE      0 /**< for code objects */
#define OC_DATA      1 /**< for any data objects */
#define OC_NOOBJECT -1 /**< for non objects (means: relocs, resources, tables ...) */

/*
   Binary plugins state constants:
     First called 'check_format' - plugin in UNPLUGGED state.
     Second called 'init' - plugin turn into PLUGGED IN state.
     Last called 'destroy' - plugin turn into UNPLUGGED state.
     These foundamental state is not defined through PS_XXX constants,
     because it meaningless.
     General purpose for PS_XXX state is activating of preemptive memory
     allocation mechanism. Means: when plugin in PS_INACTIVE state (when
     references resolving is 'None') then on low memory it can freed some
     buffers. (This need for prevent of infinity loop of memory
     allocation/freing, that possible in some case).
*/

#define PS_INACTIVE    0
#define PS_ACTIVE      1

typedef struct tag_REGISTRY_BIN
{
  const char * name;                            /**< name of binary format */
  const char * prompt[10];                      /**< on ALT-Fx selection */
  BinFunc   action[10];                         /**< action on ALT-Fx selection */
  tBool   __FASTCALL__ (*check_format)( void ); /**< Checks format */
  void    __FASTCALL__ (*init)( void );         /**< Inits plugin (if check o'k) (constructor) */
  void    __FASTCALL__ (*destroy)( void );      /**< Destroys plugin (destructor) */
  BinFunc   showHdr;                            /**< if not an MZ style format */
  AppRefs   bind;                               /**< for show references */

                         /** This function is called, when activity of plugin is changed.
                           * @param state      See PS_XXX constants.
                           * @note             Plugin must support counter of
                           *                   states. (For multiple call purpose)
                          **/
  void    __FASTCALL__ (*set_state)(int state);

                         /** Returns CPU platform, that required by format.
                           * @note           Full list of platform please see in
                           *                 plugins/disasm.h file. If this
                           *                 function return -1 then platform is
                           *                 undefined.
                          **/
  int     __FASTCALL__ (*query_platform)( void );

                         /** Returns DAB_XXX. Quick version for disassembler */
  int     __FASTCALL__ (*query_bitness)(unsigned long);

                         /** For displaying offset within struct in left address column.
                           * @return         False if string is not modified.
                          **/
  tBool   __FASTCALL__ (*AddressResolving)(char *,unsigned long);

                         /** Converts virtual address to physical (means file offset).
                           * @param va       indicates virtual address to be converted
                           * @return         0 if operation meaningless
                          **/
  unsigned long __FASTCALL__ (*va2pa)(unsigned long va);

                         /** Converts physical address to virtual.
                           * @param pa       indicates physical address to be converted
                           * @note           seg pointer can be NULL
                          **/
  unsigned long __FASTCALL__ (*pa2va)(unsigned long pa);


/*-- Below placed functions for 'put structures' method of save as dialog --*/

                         /** Fills the string with public symbol
                           * @param str       pointer to the string to be filled
                           * @param cb_str    indicates maximal length of string
                           * @param _class    pointer to the memory where can be stored class of symbol (See SC_* conatnts)
                           * @param pa        indicates physical offset within file
                           * @param as_prev   indicates direction of symbol searching from given physical offset
                           * @return          0 - if no symbol name available
                           *                  in given direction (as_prev)
                           *                  physical address of public symbol
                           *                  which is found in given direction
                          **/
  unsigned long __FASTCALL__ (*GetPubSym)(char *str,unsigned cb_str,unsigned *_class,
                             unsigned long pa,tBool as_prev);

                         /** Determines attributes of object at given physical file address.
                           * @param pa        indicates physical file offset of object
                           * @param name      pointer to the string which is to be filled with object name
                           * @param cb_name   indicates maximal length of string
                           * @param start     pointer to the memory where must be stored start of given object, as file offset.
                           * @param end       pointer to the memory where must be stored end of given object, as file offset.
                           * @param _class    pointer to the memory where must be stored _class of object (See OC_* constants).
                           * @param bitness   pointer to the memory where must be stored bitness of object (See DAB_* constants).
                           * @return          logical number of object or 0 if at given offset is no object.
                           * @note            all arguments exclude name of object
                           *                  must be filled.
                           * @remark          For example: if exe-format - new
                           *                  exe i.e. contains MZ and NEW
                           *                  header and given file offset
                           *                  points to old exe stub then start
                           *                  = 0, end = begin of first data or
                           *                  code object).
                          **/
  unsigned    __FASTCALL__ (*GetObjAttr)(unsigned long pa,char *name,unsigned cb_name,
                              unsigned long *start,unsigned long *end,int *_class,int *bitness);

                         /** Prepares internal buffers for work file structures.
                           * @param start     indicates start position in the file, that is required for dissasembler
                           * @param end       indicates end position in the file, that is required for dissasembler
                           * return           False if success, True if an error
                           *                  is occured (sample: out of memory)
                           * @note            It is called before GetPubSym and GetObjAttr
                          **/
  tBool         __FASTCALL__ (*prepare_structs)(unsigned long start,unsigned long end);
                         /** Cleans internal buffers after stopping of structural disassembler */
  void          __FASTCALL__ (*drop_structs)( void );
}REGISTRY_BIN;

extern REGISTRY_BIN *detectedFormat;

/***************************************************************\
* This form registry modes of translation file                  *
\***************************************************************/

#define __MF_NONE          0x0000 /**< Indicates that no flags were defined */
#define __MF_TEXT          0x0001 /**< Indicates that plugin is text browser */
#define __MF_DISASM        0x0002 /**< Indicates that plugin is dissasembler */
#define __MF_USECODEGUIDE  0x0004 /**< Indicates that plugin uses code guider */

#define __MAX_SYMBOL_SIZE  4 /**< Insicates maximal size of multibyte symbol. (For optimization purposes only).*/

typedef struct tag_REGISTRY_MODE
{
  const char *  name;
  const char *  prompt[10];                   /**< on Ctrl-Fx selection */
  ModFunc       action[10];                   /**< action on Ctrl-Fx selection */
  tBool         __FASTCALL__ (*detect)(void); /**< detects possibility to assign this mode as default mode for openned file. */
  unsigned       flags;                       /**< see __MF_* constants */

                         /** Paints the file on the screen.
                           * @param keycode   indicates keyboard code which caused repainting
                           * @param textshift indicates shift of text. Useful only for text mode.
                           * return           new shift of text
                          **/
  unsigned      __FASTCALL__ (*paint)(unsigned keycode,unsigned textshift);

                         /** Converts buffer with using selected NLS as xlat table.
                           * @param str       string to be converted
                           * @param len       length of string
                           * @param use_fs_nls specifies usage of nls of file
                                              system but not screen.
                           * @return          new size of blocks after conversation
                          **/
  unsigned      __FASTCALL__ (*convert_cp)(char *str,unsigned len, tBool use_fs_nls);

  unsigned      __FASTCALL__ (*get_symbol_size)( void ); /**< Returns symbol size in bytes for selected NLS codepage */
  const char *  __FASTCALL__ (*misckey_name)( void );    /**< F4 key name */
  void          __FASTCALL__ (*misc_action)( void );     /**< F4 action */
  unsigned long __FASTCALL__ (*PrevPageSize)(void);      /**< Get previous page size */
  unsigned long __FASTCALL__ (*CurPageSize)(void);       /**< Get current page size */
  unsigned long __FASTCALL__ (*PrevLineWidth)( void );   /**< Get previous line width */
  unsigned long __FASTCALL__ (*CurLineWidth)( void );    /**< Get current line width */
  void          __FASTCALL__ (*help)( void );            /**< display help about mode */
  void          __FASTCALL__ (*read_ini)( hIniProfile * );  /**< reads biew.ini file if need */
  void          __FASTCALL__ (*save_ini)( hIniProfile * );  /**< writes to biew.ini if need */
  void          __FASTCALL__ (*init)( void );            /**< initialize mode (constructor) */
  void          __FASTCALL__ (*term)( void );            /**< destroy mode (destructor) */
                         /** Performs search in plugin's output
                           * @param pwnd      indicates handle of Percent window with progress indicator
                           * @param start     indicates start offset within file where search must be performed
                           * @param slen      on output contains length of found sequence
                           * @param flags     indicates flags (SF_* family) of search.
                           * @param is_continue indicates initialization of search
                           *                  If set then search should be continued
                           * @param is_found  on output must contain True if result is really found
                           * @return          offset of found sequence or ULONG_MAX if not found
                          **/
unsigned long __FASTCALL__ (*search_engine)(TWindow *pwnd, unsigned long start, unsigned long *slen, unsigned flags, tBool is_continue, tBool *is_found);
}REGISTRY_MODE;

extern REGISTRY_MODE *activeMode;
extern tBool SelectMode( void );
extern void QuickSelectMode( void );

typedef struct tag_REGISTRY_TOOL
{
  const char *  name;                /**< Tool name */
  void          (*tool)( void );     /**< Tool body */
  void          (*read_ini)( void ); /**< read biew.ini if need */
  void          (*save_ini)( void ); /**< write to biew.ini if need */
}REGISTRY_TOOL;

typedef struct tag_REGISTRY_SYSINFO
{
  const char *  name;                /**< System depended information name */
  void          (*sysinfo)( void );  /**< System depended information body */
  void          (*read_ini)( void ); /**< reads biew.ini if need */
  void          (*save_ini)( void ); /**< writes to biew.ini if need */
}REGISTRY_SYSINFO;

#ifdef __cplusplus
}
#endif

#endif
