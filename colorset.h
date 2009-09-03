/**
 * @namespace   biew
 * @file        colorset.h
 * @brief       This file contains color function prototypes.
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
#ifndef __COLORSET__H
#define __COLORSET__H

#ifndef __TWIN_H
#include "biewlib/twin.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tag_namedColorDef
{
  const char  *name;
  Color  color;
}namedColorDef;

extern namedColorDef named_color_def[16];

typedef struct tag_MultiEditCSet
{
  ColorAttr  main;
  ColorAttr  change;
  ColorAttr  selected;
}MultiEditCSet;

/** Browser window */
typedef struct tag_BrowserCSet
{
  ColorAttr main;
  ColorAttr bound;
  ColorAttr hlight;
  ColorAttr highline;
  ColorAttr title;
  ColorAttr footer;
  ColorAttr scroller;
  MultiEditCSet edit;
}BrowserCSet;

extern BrowserCSet browser_cset;

/** Prompt window */
typedef struct tag_PromptCSet
{
  ColorAttr button;
  ColorAttr digit;
  ColorAttr control;
}PromptCSet;

extern PromptCSet prompt_cset;

/** Title window */
typedef struct tag_TitleCSet
{
  ColorAttr main;
  ColorAttr change;
}TitleCSet;

extern TitleCSet title_cset;

typedef struct tag_ButtonCSet
{
  ColorAttr active;
  ColorAttr focused;
  ColorAttr disabled;
}ButtonCSet;

/** message box */
typedef struct tag_MessageCSet
{
  ColorAttr main;
  ColorAttr border;
  ButtonCSet button;
}MessageCSet;

extern MessageCSet error_cset;
extern MessageCSet warn_cset;
extern MessageCSet notify_cset;

/** Text highlight */
typedef struct tag_TextCSet
{
  ColorAttr normal;
  ColorAttr bold;
  ColorAttr italic;
  ColorAttr underline;
  ColorAttr strikethrough;
}TextCSet;

extern TextCSet text_cset;

/** CPU highlight */
typedef struct tag_CPUCSet
{
  ColorAttr clone[16];
}CPUCSet;

/** Disassembler highlight */
typedef struct tag_DisasmCSet
{
  ColorAttr family_id;
  ColorAttr opcodes;
  ColorAttr opcodes0;
  ColorAttr opcodes1;
  ColorAttr opcodes2;
  ColorAttr opcodes3;
  ColorAttr opcodes4;
  ColorAttr opcodes5;
  ColorAttr comments;
  CPUCSet   cpu_cset[4];
}DisasmCSet;

extern DisasmCSet disasm_cset;

/** Programming highlight */

typedef struct tag_PrgWordCSet
{
    ColorAttr base;
    ColorAttr extended;
    ColorAttr reserved;
    ColorAttr alt;
}PrgWordCSet;

typedef struct tag_ProgCSet
{
  ColorAttr bads;
  PrgWordCSet comments;
  PrgWordCSet keywords;
  PrgWordCSet constants;
  PrgWordCSet operators;
  PrgWordCSet preproc;
}ProgCSet;

extern ProgCSet prog_cset;

typedef struct tag_EditorCSet
{
  ColorAttr  active;
  ColorAttr  focused;
  ColorAttr  disabled;
  ColorAttr  select;
}EditorCSet;


/** Dialog window */
typedef struct tag_DialogCSet
{
  ColorAttr main;
  ColorAttr border;
  ColorAttr title;
  ColorAttr footer;
  ColorAttr selfooter;
  ColorAttr entry;
  ColorAttr altentry;
  ColorAttr addinfo;
  ColorAttr altinfo;
  ColorAttr extrainfo;
  ButtonCSet group;
  ButtonCSet button;
  ButtonCSet any;
  EditorCSet editor;
}DialogCSet;

extern DialogCSet dialog_cset;

typedef struct tag_MenuCSet
{
  ColorAttr  main;
  ColorAttr  border;
  ColorAttr  title;
  ColorAttr  highlight;
  ButtonCSet hotkey;
  ButtonCSet item;
}MenuCSet;

extern MenuCSet menu_cset;

/** Help window */
typedef struct tag_HelpCSet
{
  ColorAttr main;
  ColorAttr border;
  ColorAttr title;
  ColorAttr bold;
  ColorAttr italic;
  ColorAttr reverse;
  ColorAttr underline;
  ColorAttr strikethrough;
  ColorAttr link;
  ColorAttr sellink;
}HelpCSet;

extern HelpCSet help_cset;

extern tBool   csetReadIniFile(const char *pal_name);

#ifdef __cplusplus
}
#endif

#endif
