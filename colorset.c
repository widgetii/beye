/**
 * @namespace   biew
 * @file        colorset.c
 * @brief       This file contains color part of BIEW project.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       2000
 * @note        Development, fixes and improvements
**/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "colorset.h"
#include "bconsole.h"
#include "biewlib/file_ini.h"
#include "biewlib/biewlib.h"

extern char biew_scheme_name[];
char last_skin_error[50];

namedColorDef named_color_def[16] =
{
   { "Black",        Black        },
   { "Blue",         Blue         },
   { "Green",        Green        },
   { "Cyan",         Cyan         },
   { "Red",          Red          },
   { "Magenta",      Magenta      },
   { "Brown",        Brown        },
   { "LightGray",    LightGray    },
   { "Gray",         Gray         },
   { "LightBlue",    LightBlue    },
   { "LightGreen",   LightGreen   },
   { "LightCyan",    LightCyan    },
   { "LightRed",     LightRed     },
   { "LightMagenta", LightMagenta },
   { "Yellow",       Yellow       },
   { "White",        White        }
};

static Color __NEAR__ __FASTCALL__ getColorByName(const char *name,Color defval,tBool *has_err)
{
  unsigned i;
  for(i = 0;i < sizeof(named_color_def)/sizeof(namedColorDef);i++)
  {
    if(strcmp(name,named_color_def[i].name) == 0)
    {
      *has_err = False;
      return named_color_def[i].color;
    }
  }
  strncpy(last_skin_error,name,sizeof(last_skin_error));
  last_skin_error[sizeof(last_skin_error)-1] = 0;
  *has_err = True;
  return defval;
}

static ColorAttr __NEAR__ __FASTCALL__ getColorPairByName(const char *name, ColorAttr defval,tBool* has_err)
{
  char wstr[80];
  char *p;
  tBool he;
  unsigned i,j,len;
  ColorAttr retval;
  Color fore,back;
  len = strlen(name);
  for(j = i = 0;i < len;i++) if(!isspace((unsigned char)name[i])) wstr[j++] = name[i];
  wstr[j] = 0;
  p = strchr(wstr,':');
  if(!p)
  {
    *has_err = True;
    strcpy(last_skin_error,"':' is lost");
    return defval;
  }
  *p = 0;
  p++;
  back = getColorByName(wstr,BACK_COLOR(defval),has_err);
  fore = getColorByName(p,FORE_COLOR(defval),&he);
  retval = LOGFB_TO_PHYS(fore,back);
  if(he) *has_err = he;
  return retval;
}

static tBool __NEAR__ __FASTCALL__ readColorPair(hIniProfile *ini,const char *section,
                                   const char *subsection,
                                   const char *item,
                                   ColorAttr *value)
{
  char cstr[80],cval[80];
  tBool has_err;
  sprintf(cval,"%s:%s"
          ,named_color_def[BACK_COLOR(*value) & 0x0F].name
          ,named_color_def[FORE_COLOR(*value) & 0x0F].name);
  iniReadProfileString(ini,section,subsection,item,cval,cstr,sizeof(cstr));
  *value = getColorPairByName(cstr,*value,&has_err);
  return has_err;
}

static tBool __NEAR__ __FASTCALL__ readButton(hIniProfile *ini,const char *section,
                                              const char *subsection,
                                              ButtonCSet *to)
{
  tBool has_err;
  has_err = readColorPair(ini,section,subsection,"Active",&to->active);
  has_err |= readColorPair(ini,section,subsection,"Focused",&to->focused);
  has_err |= readColorPair(ini,section,subsection,"Disabled",&to->disabled);
  return has_err;
}

tBool csetReadIniFile(const char *ini_name)
{
  hIniProfile *cset;
  char cstr[80],cval[80],csec[80];
  unsigned value,i,j;
  tBool has_err,cur_err;
  has_err = False;
  cset = iniOpenFile(ini_name,&has_err);
  last_skin_error[0] = 0;
  if(has_err) return False; /** return no error, because ini_name was not found or unavailable */
  iniReadProfileString(cset,"Skin info","","Name","Unnamed",biew_scheme_name,256);
  for(i = 0;i < 16;i++)
  {
    sprintf(cval,"%i",named_color_def[i].color);
    iniReadProfileString(cset,"Color map","",named_color_def[i].name,cval,cstr,sizeof(cstr));
    value = atoi(cstr);
    twRemapColor(named_color_def[i].color,value);
  }
  for(i = 0;i < 8;i++)
  {
    sprintf(cval,"Trans%i",i+1);
    iniReadProfileString(cset,"Terminal","",cval,"",cstr,sizeof(cstr));
    if(cstr[0])
    {
      Color col;
      col = getColorByName(cstr,Black,&cur_err);
      if(cur_err) has_err = cur_err;
      else __vioSetTransparentColor(twGetMappedColor(col));
    }
  }
  has_err |= readColorPair(cset,"Browser","","Main",&browser_cset.main);
  has_err |= readColorPair(cset,"Browser","","Bound",&browser_cset.bound);
  has_err |= readColorPair(cset,"Browser","","HighLight",&browser_cset.hlight);
  has_err |= readColorPair(cset,"Browser","","HighLightLine",&browser_cset.highline);
  has_err |= readColorPair(cset,"Browser","","Title",&browser_cset.title);
  has_err |= readColorPair(cset,"Browser","","Footer",&browser_cset.footer);
  has_err |= readColorPair(cset,"Browser","","Scroller",&browser_cset.scroller);
  has_err |= readColorPair(cset,"Browser","Edit","Main",&browser_cset.edit.main);
  has_err |= readColorPair(cset,"Browser","Edit","Change",&browser_cset.edit.change);
  has_err |= readColorPair(cset,"Browser","Edit","Selected",&browser_cset.edit.selected);
  has_err |= readColorPair(cset,"Prompt","","Button",&prompt_cset.button);
  has_err |= readColorPair(cset,"Prompt","","Digit",&prompt_cset.digit);
  has_err |= readColorPair(cset,"Prompt","","Control",&prompt_cset.control);
  has_err |= readColorPair(cset,"Title","","Main",&title_cset.main);
  has_err |= readColorPair(cset,"Title","","Change",&title_cset.change);
  has_err |= readColorPair(cset,"Text","","Normal",&text_cset.normal);
  has_err |= readColorPair(cset,"Text","","Bold",&text_cset.bold);
  has_err |= readColorPair(cset,"Text","","Italic",&text_cset.italic);
  has_err |= readColorPair(cset,"Text","","Underline",&text_cset.underline);
  has_err |= readColorPair(cset,"Text","","StrikeThrough",&text_cset.strikethrough);
  has_err |= readColorPair(cset,"Disasm","","FamilyId",&disasm_cset.family_id);
  has_err |= readColorPair(cset,"Disasm","","Opcodes",&disasm_cset.opcodes);
  has_err |= readColorPair(cset,"Disasm","","Comments",&disasm_cset.comments);
  for(j = 0;j < sizeof(disasm_cset.cpu_cset)/sizeof(CPUCSet);j++)
  {
    sprintf(csec,"Processor%i",j);
    for(i = 0;i < sizeof(disasm_cset.cpu_cset[0])/sizeof(ColorAttr);i++)
    {
      sprintf(cval,"Clone%i",i+1);
      has_err |= readColorPair(cset,"Disasm",csec,cval,&disasm_cset.cpu_cset[j].clone[i]);
    }
  }
  has_err |= readColorPair(cset,"Programming","","Regular",&prog_cset.regular);
  has_err |= readColorPair(cset,"Programming","","Keyword",&prog_cset.keyword);
  has_err |= readColorPair(cset,"Programming","","Const",&prog_cset.consts);
  has_err |= readColorPair(cset,"Programming","","Preprocessor",&prog_cset.preproc);
  has_err |= readColorPair(cset,"Programming","","BadExpr",&prog_cset.bads);
  has_err |= readColorPair(cset,"Programming","","Punctuation",&prog_cset.puncts);
  has_err |= readColorPair(cset,"Programming","","Comments",&prog_cset.comments);
  has_err |= readColorPair(cset,"Error","","Main",&error_cset.main);
  has_err |= readColorPair(cset,"Error","","Border",&error_cset.border);
  has_err |= readButton(cset,"Error","Button",&error_cset.button);
  has_err |= readColorPair(cset,"Warning","","Main",&warn_cset.main);
  has_err |= readColorPair(cset,"Warning","","Border",&warn_cset.border);
  has_err |= readButton(cset,"Warning","Button",&warn_cset.button);
  has_err |= readColorPair(cset,"Notify","","Main",&notify_cset.main);
  has_err |= readColorPair(cset,"Notify","","Border",&notify_cset.border);
  has_err |= readButton(cset,"Notify","Button",&notify_cset.button);
  has_err |= readColorPair(cset,"Dialog","","Main",&dialog_cset.main);
  has_err |= readColorPair(cset,"Dialog","","Border",&dialog_cset.border);
  has_err |= readColorPair(cset,"Dialog","","Title",&dialog_cset.title);
  has_err |= readColorPair(cset,"Dialog","","Footer",&dialog_cset.footer);
  has_err |= readColorPair(cset,"Dialog","","SelFooter",&dialog_cset.selfooter);
  has_err |= readColorPair(cset,"Dialog","","Entry",&dialog_cset.entry);
  has_err |= readColorPair(cset,"Dialog","","AltEntry",&dialog_cset.altentry);
  has_err |= readColorPair(cset,"Dialog","","AddInfo",&dialog_cset.addinfo);
  has_err |= readColorPair(cset,"Dialog","","AltInfo",&dialog_cset.altinfo);
  has_err |= readColorPair(cset,"Dialog","","ExtraInfo",&dialog_cset.extrainfo);
  has_err |= readButton(cset,"Dialog","Group",&dialog_cset.group);
  has_err |= readButton(cset,"Dialog","Button",&dialog_cset.button);
  has_err |= readButton(cset,"Dialog","Any",&dialog_cset.any);
  has_err |= readColorPair(cset,"Dialog","Editor","Active",&dialog_cset.editor.active);
  has_err |= readColorPair(cset,"Dialog","Editor","Focused",&dialog_cset.editor.focused);
  has_err |= readColorPair(cset,"Dialog","Editor","Disabled",&dialog_cset.editor.disabled);
  has_err |= readColorPair(cset,"Dialog","Editor","Select",&dialog_cset.editor.select);
  has_err |= readColorPair(cset,"Menu","","Main",&menu_cset.main);
  has_err |= readColorPair(cset,"Menu","","Border",&menu_cset.border);
  has_err |= readColorPair(cset,"Menu","","Title",&menu_cset.title);
  has_err |= readColorPair(cset,"Menu","","HighLight",&menu_cset.highlight);
  has_err |= readButton(cset,"Menu","HotKey",&menu_cset.hotkey);
  has_err |= readButton(cset,"Menu","Item",&menu_cset.item);

  has_err |= readColorPair(cset,"Help","","Main",&help_cset.main);
  has_err |= readColorPair(cset,"Help","","Border",&help_cset.border);
  has_err |= readColorPair(cset,"Help","","Title",&help_cset.title);
  has_err |= readColorPair(cset,"Help","","Bold",&help_cset.bold);
  has_err |= readColorPair(cset,"Help","","Italic",&help_cset.italic);
  has_err |= readColorPair(cset,"Help","","Reverse",&help_cset.reverse);
  has_err |= readColorPair(cset,"Help","","Underline",&help_cset.underline);
  has_err |= readColorPair(cset,"Help","","StrikeThrough",&help_cset.strikethrough);
  has_err |= readColorPair(cset,"Help","","Link",&help_cset.link);
  has_err |= readColorPair(cset,"Help","","SelLink",&help_cset.sellink);
  iniCloseFile(cset);
  return has_err;
}

BrowserCSet browser_cset =
{
  LOGFB_TO_PHYS(LightCyan, Blue),
  LOGFB_TO_PHYS(Yellow, Blue),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(White, Blue),
  LOGFB_TO_PHYS(Blue, Cyan),
  LOGFB_TO_PHYS(Blue, Green),
  LOGFB_TO_PHYS(Yellow, Blue),
  { LOGFB_TO_PHYS(White, Blue),
    LOGFB_TO_PHYS(LightRed, Blue),
    LOGFB_TO_PHYS(Blue, LightGray) }
};

PromptCSet prompt_cset =
{
  LOGFB_TO_PHYS(Black, Cyan),
  LOGFB_TO_PHYS(LightCyan, Black),
  LOGFB_TO_PHYS(LightGray, Black),
};

TitleCSet title_cset =
{
  LOGFB_TO_PHYS(Black, Cyan),
  LOGFB_TO_PHYS(Red, Cyan)
};

MessageCSet error_cset =
{
  LOGFB_TO_PHYS(Yellow, Red),
  LOGFB_TO_PHYS(LightRed, Red),
  { LOGFB_TO_PHYS(Yellow, Green),
    LOGFB_TO_PHYS(White, Cyan),
    LOGFB_TO_PHYS(Gray, LightGray) }
};

MessageCSet notify_cset =
{
  LOGFB_TO_PHYS(White, Green),
  LOGFB_TO_PHYS(Yellow, Green),
  { LOGFB_TO_PHYS(Yellow, Green),
    LOGFB_TO_PHYS(White, Cyan),
    LOGFB_TO_PHYS(Gray, LightGray) }
};

MessageCSet warn_cset =
{
  LOGFB_TO_PHYS(White, Magenta),
  LOGFB_TO_PHYS(LightMagenta, Magenta),
  { LOGFB_TO_PHYS(Yellow, Green),
    LOGFB_TO_PHYS(White, Cyan),
    LOGFB_TO_PHYS(Gray, LightGray) }
};

TextCSet text_cset =
{
  LOGFB_TO_PHYS(LightCyan, Blue),
  LOGFB_TO_PHYS(White, Blue),
  LOGFB_TO_PHYS(LightGreen, Blue),
  LOGFB_TO_PHYS(Yellow, Blue),
  LOGFB_TO_PHYS(LightBlue, Blue)
};

DisasmCSet disasm_cset =
{
  LOGFB_TO_PHYS(LightBlue, Blue),
  LOGFB_TO_PHYS(LightCyan, Blue),
  LOGFB_TO_PHYS(Yellow, Blue),
  {
   {
    {
     LOGFB_TO_PHYS(LightCyan, Blue),
     LOGFB_TO_PHYS(LightGray, Blue),
     LOGFB_TO_PHYS(Yellow, Blue),
     LOGFB_TO_PHYS(LightGreen, Blue),
     LOGFB_TO_PHYS(Red, Blue),
     LOGFB_TO_PHYS(LightRed, Blue),
     LOGFB_TO_PHYS(LightMagenta, Blue),
     LOGFB_TO_PHYS(LightBlue, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Gray, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue)
    }
   },
   {
    {
     LOGFB_TO_PHYS(Cyan, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Brown, Blue),
     LOGFB_TO_PHYS(Green, Blue),
     LOGFB_TO_PHYS(Magenta, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(LightMagenta, Blue),
     LOGFB_TO_PHYS(LightBlue, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Gray, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue)
    }
   },
   {
    {
     LOGFB_TO_PHYS(LightBlue, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(White, Blue),
     LOGFB_TO_PHYS(LightMagenta, Blue),
     LOGFB_TO_PHYS(LightBlue, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Gray, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue)
    }
   },
   {
    {
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue)
    }
   }
  }
};

ProgCSet prog_cset =
{
  LOGFB_TO_PHYS(Yellow, Blue),
  LOGFB_TO_PHYS(White, Blue),
  LOGFB_TO_PHYS(LightCyan, Blue),
  LOGFB_TO_PHYS(LightGreen, Blue),
  LOGFB_TO_PHYS(Black, Red),
  LOGFB_TO_PHYS(LightBlue, Blue),
  LOGFB_TO_PHYS(LightGray, Blue)
};

DialogCSet dialog_cset =
{
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(White, Black),
  LOGFB_TO_PHYS(White, Black),
  LOGFB_TO_PHYS(Yellow, Blue),
  LOGFB_TO_PHYS(Yellow, LightGray),
  LOGFB_TO_PHYS(LightCyan, LightGray),
  LOGFB_TO_PHYS(Red, LightGray),
  { LOGFB_TO_PHYS(Black, Cyan),
    LOGFB_TO_PHYS(Black, Cyan),
    LOGFB_TO_PHYS(Gray, Cyan) },
  { LOGFB_TO_PHYS(Yellow, Blue),
    LOGFB_TO_PHYS(White, Green),
    LOGFB_TO_PHYS(Gray, LightGray) },
  { LOGFB_TO_PHYS(Black, LightGray),
    LOGFB_TO_PHYS(Black, LightGray),
    LOGFB_TO_PHYS(Gray, LightGray) },
  { LOGFB_TO_PHYS(White, Cyan),
    LOGFB_TO_PHYS(White, Cyan),
    LOGFB_TO_PHYS(Gray, Cyan),
    LOGFB_TO_PHYS(Cyan, LightGray) }
};

MenuCSet menu_cset =
{
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, Green),
  { LOGFB_TO_PHYS(Red, LightGray),
    LOGFB_TO_PHYS(Red, Black),
    LOGFB_TO_PHYS(Gray, LightGray) },
  { LOGFB_TO_PHYS(Black, LightGray),
    LOGFB_TO_PHYS(White, Black),
    LOGFB_TO_PHYS(Gray, LightGray) }
};

HelpCSet help_cset =
{
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Yellow, LightGray),
  LOGFB_TO_PHYS(Blue, LightGray),
  LOGFB_TO_PHYS(Magenta, LightGray),
  LOGFB_TO_PHYS(Red, LightGray),
  LOGFB_TO_PHYS(Gray, LightGray),
  LOGFB_TO_PHYS(Yellow, LightCyan),
  LOGFB_TO_PHYS(White, Black)
};
