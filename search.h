/**
 * @namespace   biew
 * @file        search.h
 * @brief       This file contains search function prototypes.
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
#ifndef __SEARCH__H
#define __SEARCH__H

#ifdef __cplusplus
extern "C" {
#endif

                   /** Main search routine
                     * @param is_continue  indicates initialization of search
                                           If set then search should be continued
                                           search dialog will displayed otherwise
                     * @return             new offset on successful search and
                                           current offset otherwise
                    **/
extern __filesize_t __FASTCALL__ Search( tBool is_continue );

#define SD_SIMPLE       0x0000   /**< indicates simple search dialog to be displayed */
#define SD_ALLFEATURES  0x0001   /**< indicates fully featured search dialog to be displayed */

#define SF_NONE         0x0000   /**< indicates no flags */
#define SF_CASESENS     0x0001   /**< indicates case sensitivity search engine */
#define SF_WORDONLY     0x0002   /**< indicates word only to be found */
#define SF_REVERSE      0x0004   /**< indicates reverse search */
#define SF_WILDCARDS    0x0008   /**< indicates wildcard can be used */
#define SF_PLUGINS      0x0010   /**< indicates using plugin's output */
#define SF_ASHEX        0x0020   /**< indicates hex mode of input sequence */

#define MAX_SEARCH_SIZE 76
extern tBool        __FASTCALL__ SearchDialog(int dlg_flags,
                                              char *searchbuff,
                                              unsigned char *searchlen,
                                              unsigned *search_flags);

                   /** Performs seacrh of given sequence in the string
                     * @param str          indicates string where search must be performed
                     * @param str_len      indicates length string where search must be performed
                     * @param sbuff        indicates searching sequence
                     * @param sbuflen      indicates length of sequence to be found
                     * @param cache        indicates Boyer-Moore cache
                     * @param flags        indicates SF_* flags
                     * @return             address of first occurence of 
                                           of searching sequence in the
                                           string or NULL if not found.
                    **/
extern char *       __FASTCALL__ strFind(const char *str, unsigned str_len, 
                                         const void * sbuff, unsigned sbuflen,
                                         const int *cache, unsigned flags);

                   /** Fills cache for Boyer-Moore search
                     * @param cache        indicates cache to be filled
                                           Should have size of 256 elements.
                     * @param pattern      indicates search pattern
                     * @param pattern_len  indicates length of search pattern
                     * @param case_sens    indicates case sensitivity of search
                     * @return             none
                    **/
void                __FASTCALL__ fillBoyerMooreCache(int *cache,
                                         const char *pattern,
                                         unsigned pattern_len,
                                         tBool case_sens);

extern unsigned char search_buff[MAX_SEARCH_SIZE];
extern unsigned char search_len;
extern unsigned biewSearchFlg;

extern __filesize_t FoundTextSt; /**< Indicates start of found text */
extern __filesize_t FoundTextEnd;/**< Indicates end of found text */

#ifdef __cplusplus
}
#endif

#endif
