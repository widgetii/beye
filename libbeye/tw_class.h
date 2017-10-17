/**
 * @namespace   biewlib
 * @file        biewlib/tw_class.h
 * @brief       This file contains prototypes of classes for Text Window manager.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       2000
 * @note        Development, fixes and improvements
**/

#ifndef __TW_CLASS_H
#define __TW_CLASS_H 1

#ifndef __TWIN_H
#error Never use this header file directly. Use twin.h instead
#endif

#ifdef __cplusplus
extern "C" {
#endif
                   /** Defines class callback routine type
                     * @return                0L if no action performed
                     *                        not 0 depends from event
                     * @param win             indicates handle of window for which
                     *                        event handler is called
                     * @param event           indicates event type
                     * @param event_param     indicates parameters of event
                     * @param event_data      indicates data of event
                    **/
typedef long (__FASTCALL__ *twClassFunc)(TWindow *win,unsigned event,
                                  unsigned long event_param, void *event_data);

#define __CS_ORDINAL 0x0000 /**< Indicates ordinal type of callback function */

/** Internal structure of text window class */
typedef struct tagTwClass
{
  char       *name;     /**< name of class */
  twClassFunc method;   /**< callback routine */
  unsigned    flags;    /**< flags */
}TwClass;

                   /** Registers new class
                     * @return                True if successful
                     * @param name            indicates symbolic name of class
                     * @param flags           indicates __CS_* flags family
                     * @param method          indicates callback routine
                    **/
tBool __FASTCALL__ twcRegisterClass(const char *name, unsigned flags, twClassFunc method);

                   /** Destroys set of classes
                     * @return                none
                     * @param note            Do not call it directly. It is
                                              automatically called by library.
                     * @param warning         Library don't contain
                                              DeregisterClass function from
                                              safety reasons.
                    **/
void __FASTCALL__ twcDestroyClassSet(void);

                   /** Finds class descriptor
                     * @return                pointer to class descriptor or
                     *                        NULL if error occured
                     * @param name            indicates symbolic name of class
                    **/
TwClass * __FASTCALL__ twcFindClass(const char *name);

#ifdef __cplusplus
}
#endif

#endif






