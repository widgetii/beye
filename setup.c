/**
 * @namespace   biew
 * @file        setup.c
 * @brief       This file contains BIEW setup.
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
**/
#include <string.h>
#include <stdio.h>

#include "biewhelp.h"
#include "colorset.h"
#include "setup.h"
#include "bconsole.h"
#include "biewutil.h"
#include "biewlib/twin.h"
#include "biewlib/kbd_code.h"

extern char biew_help_name[];
extern char biew_skin_name[];
extern char biew_syntax_name[];
extern char biew_codepage[];

extern unsigned long biew_vioIniFlags;
extern unsigned long biew_twinIniFlags;
extern unsigned long biew_kbdFlags;
extern tBool iniSettingsAnywhere;
extern tBool fioUseMMF;
extern tBool iniPreserveTime;
extern tBool iniUseExtProgs;

#ifdef __QNX4__
extern int photon,bit7;
#endif
char * biewGetHelpName( void )
{
  if(!biew_help_name[0])
  {
    strcpy(biew_help_name,__get_rc_dir("biew"));
    strcat(biew_help_name,"biew.hlp");
  }
  return biew_help_name;
}

static char * __NEAR__ __FASTCALL__ biewGetColorSetName( void )
{
  if(!biew_skin_name[0])
  {
    strcpy(biew_skin_name,__get_rc_dir("biew"));
    strcat(biew_skin_name,"skn/" "standard.skn"); /* [dBorca] in skn/ subdir */
  }
  return biew_skin_name;
}

static char * __NEAR__ __FASTCALL__ biewGetSyntaxName( void )
{
  if(!biew_syntax_name[0])
  {
    strcpy(biew_syntax_name,__get_rc_dir("biew"));
    strcat(biew_syntax_name,"syntax/" "syntax.stx"); /* [dBorca] in syntax/ subdir */
  }
  return biew_syntax_name;
}


static const char * setuptxt[] =
{
  "Help  ",
  "Consol",
  "Color ",
  "Mouse ",
  "Bit   ",
  "Plugin",
  "MMF   ",
  "Time  ",
  "ExtPrg",
  "Escape"
};

static unsigned default_cp = 12;
static const char * cp_list[] =
{
  "CP437 - original set of the IBM PC",
  "CP737 - Greek language",
  "CP775 - Estonian, Lithuanian and Latvian languages",
  "CP850 - English DOS systems",
  "CP852 - Central European languages",
  "CP857 - Turkish language",
  "CP858 - Western European languages",
  "CP860 - Portuguese language",
  "CP861 - Icelandic language",
  "CP862 - Hebrew language",
  "CP863 - French language",
  "CP865 - Nordic languages",
  "CP866 - Cyrillic languages",
};

static tBool __FASTCALL__ select_codepage( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(cp_list)/sizeof(char *);
  i = SelBoxA(cp_list,nModes," Select single-byte codepage: ",default_cp);
  if(i != -1)
  {
    unsigned len;
    char *p;
    default_cp = i;
    p = strchr(cp_list[i],' ');
    len = p-cp_list[i];
    memcpy(biew_codepage,cp_list[i],len);
    biew_codepage[len] = '\0';
    return True;
  }
  return False;
}

static void drawSetupPrompt( void )
{
   __drawSinglePrompt(setuptxt);
}

static void __NEAR__ __FASTCALL__ setup_paint( TWindow *twin )
{
  TWindow *usd;
  usd = twUsedWin();
  twUseWin(twin);
  twSetColorAttr(dialog_cset.group.active);
  twGotoXY(2,9);
  twPrintF(" [%c] - Direct console access "
           ,GetBool((biew_vioIniFlags & __TVIO_FLG_DIRECT_CONSOLE_ACCESS) == __TVIO_FLG_DIRECT_CONSOLE_ACCESS));
  twGotoXY(2,10);
  twPrintF(" [%c] - Mouse sensitivity     "
           ,GetBool((biew_kbdFlags & KBD_NONSTOP_ON_MOUSE_PRESS) == KBD_NONSTOP_ON_MOUSE_PRESS));
  twGotoXY(2,11);
  twPrintF(" [%c] - Force mono            "
           ,GetBool((biew_twinIniFlags & TWIF_FORCEMONO) == TWIF_FORCEMONO));
  twGotoXY(2,12);
#ifdef __QNX4__
  if(photon)
  {
    twSetColorAttr(dialog_cset.group.disabled);
    twPrintF(" [%c] - Force 7-bit output    "
            ,GetBool(bit7));
    twSetColorAttr(dialog_cset.group.active);
  }
  else 
#endif
  twPrintF(" [%c] - Force 7-bit output    "
           ,GetBool((biew_vioIniFlags & __TVIO_FLG_USE_7BIT) == __TVIO_FLG_USE_7BIT));
  twGotoXY(32,9);
  twPrintF(" [%c] - Apply plugin settings to all files     "
           ,GetBool(iniSettingsAnywhere));
  twGotoXY(32,10);
  if(!__mmfIsWorkable()) twSetColorAttr(dialog_cset.group.disabled);
  twPrintF(" [%c] - Use MMF                                "
           ,GetBool(fioUseMMF));
  twSetColorAttr(dialog_cset.group.active);
  twGotoXY(32,11);
  twPrintF(" [%c] - Preserve timestamp                     "
           ,GetBool(iniPreserveTime));
  twGotoXY(32,12);
  twPrintF(" [%c] - Enable usage of external programs      "
           ,GetBool(iniUseExtProgs));
  twSetColorAttr(dialog_cset.main);
  twGotoXY(50,7); twPutS(biew_codepage);
  twUseWin(usd);
}

void Setup(void)
{
  tAbsCoord x1,y1,x2,y2;
  tRelCoord X1,Y1,X2,Y2;
  int ret;
  TWindow * wdlg,*ewnd[4];
  char estr[3][FILENAME_MAX+1];
  int active = 0;
  strcpy(estr[0],biewGetHelpName());
  strcpy(estr[1],biewGetColorSetName());
  strcpy(estr[2],biewGetSyntaxName());
  wdlg = CrtDlgWndnls(" Setup ",78,13);
  twGetWinPos(wdlg,&x1,&y1,&x2,&y2);
  X1 = x1;
  Y1 = y1;
  X2 = x2;
  Y2 = y2;

  X1 += 2;
  X2 -= 1;
  Y1 += 2;
  Y2 = Y1;
  ewnd[0] = CreateEditor(X1,Y1,X2,Y2,TWS_CURSORABLE | TWS_NLSOEM);
  twShowWin(ewnd[0]);
  twUseWin(ewnd[0]);
  PostEvent(KE_ENTER);
  xeditstring(estr[0],NULL,sizeof(estr[0]), NULL);
  Y1 += 2;
  Y2 = Y1;
  ewnd[1] = CreateEditor(X1,Y1,X2,Y2,TWS_CURSORABLE | TWS_NLSOEM);
  twShowWin(ewnd[1]);
  twUseWin(ewnd[1]);
  PostEvent(KE_ENTER);
  xeditstring(estr[1],NULL,sizeof(estr[1]), NULL);

  Y1 += 2;
  Y2 = Y1;
  ewnd[2] = CreateEditor(X1,Y1,X2,Y2,TWS_CURSORABLE | TWS_NLSOEM);
  twShowWin(ewnd[2]);
  twUseWin(ewnd[2]);
  PostEvent(KE_ENTER);
  xeditstring(estr[2],NULL,sizeof(estr[2]), NULL);

  Y1 += 2;
  Y2 = Y1;
  ewnd[3] = WindowOpen(60,Y1,61,Y2,TWS_NLSOEM);

  twUseWin(wdlg);
  twGotoXY(2,1); twPutS("Enter help file name (including full path):");
  twGotoXY(2,3); twPutS("Enter color skin name (including full path):");
  twGotoXY(2,5); twPutS("Enter syntax name (including full path):");
  twGotoXY(2,7); twPutS("Enter OEM codepage (for utf-based terminals):");
  twSetFooterAttr(wdlg," [Enter] - Accept changes ",TW_TMODE_CENTER,dialog_cset.footer);
  twinDrawFrameAttr(1,8,78,13,TW_UP3D_FRAME,dialog_cset.main);

  setup_paint(wdlg);
  active = 0;
  twUseWin(ewnd[active]);
  while(1)
  {
   if(active==3) {
	if(select_codepage() == True) setup_paint(wdlg);
	ret = KE_TAB;
   }
   else ret = xeditstring(estr[active],NULL,sizeof(estr[active]),drawSetupPrompt);
   switch(ret)
   {
     case KE_F(10):
     case KE_ESCAPE: ret = 0; goto exit;
     case KE_ENTER: ret = 1; goto exit;
     case KE_SHIFT_TAB:
     case KE_TAB:   active++;
		    if(active>3) active=0;
                    twUseWin(ewnd[active]);
                    continue;
     case KE_F(1):  hlpDisplay(5);
                    break;
     case KE_F(2):  biew_vioIniFlags ^= __TVIO_FLG_DIRECT_CONSOLE_ACCESS;
                    break;
     case KE_F(3):  biew_twinIniFlags ^= TWIF_FORCEMONO ;
                    break;
     case KE_F(4):  biew_kbdFlags ^= KBD_NONSTOP_ON_MOUSE_PRESS;
                    break;
     case KE_F(5):  biew_vioIniFlags ^= __TVIO_FLG_USE_7BIT;
                    break;
     case KE_F(6):  iniSettingsAnywhere = iniSettingsAnywhere ? False : True;
                    break;
     case KE_F(7):  if(__mmfIsWorkable()) fioUseMMF = fioUseMMF ? False : True;
                    break;
     case KE_F(8):  iniPreserveTime = iniPreserveTime ? False : True;
                    break;
     case KE_F(9):  iniUseExtProgs = iniUseExtProgs ? False : True;
                    break;
     default: continue;
   }
   setup_paint(wdlg);
  }
  exit:
  if(ret)
  {
    strcpy(biew_help_name,estr[0]);
    strcpy(biew_skin_name,estr[1]);
    strcpy(biew_syntax_name,estr[2]);
  }
  CloseWnd(ewnd[0]);
  CloseWnd(ewnd[1]);
  CloseWnd(ewnd[2]);
  CloseWnd(wdlg);
}
