/**
 * @namespace   biew
 * @file        info_win.c
 * @brief       This file contains information interface of BIEW project.
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
 * @author      Mauro Giachero
 * @date        02.11.2007
 * @note        Added ASSEMBle option to casmtext
**/
#include <stdio.h>
#include <string.h>

#include "colorset.h"
#include "bmfile.h"
#include "tstrings.h"
#include "reg_form.h"
#include "bconsole.h"
#include "biewutil.h"
#include "biewhelp.h"
#include "biewlib/kbd_code.h"
#include "biewlib/biewlib.h"

static void __NEAR__ __FASTCALL__ ShowFunKey(const char * key,const char * text)
{
 twSetColorAttr(prompt_cset.digit);
 twPutS(key);
 twSetColorAttr(prompt_cset.button);
 twPutS(text);
}

static const char * ftext[] = { "1"," 2"," 3"," 4"," 5"," 6"," 7"," 8"," 9","10" };

static void __NEAR__ __FASTCALL__ drawControlKeys(int flg)
{
  char ckey;
  if(flg & KS_SHIFT) ckey = 'S';
  else
    if(flg & KS_ALT) ckey = 'A';
    else
      if(flg & KS_CTRL) ckey = 'C';
      else              ckey = ' ';
  twGotoXY(1,1);
  twSetColorAttr(prompt_cset.control);
  twPutChar(ckey);
}

void __FASTCALL__ __drawMultiPrompt(const char *norm[], const char *shift[], const char *alt[], const char *ctrl[])
{
  TWindow *using;
  int flg = __kbdGetShiftsKey();
  int i;
  const char * cptr;
  using = twUsedWin();
  twUseWin(HelpWnd);
  twFreezeWin(HelpWnd);
  twGotoXY(2,1);
  for(i = 0;i < 10;i++)
  {
    /* todo: it might be better to ensure that if
       text!=NULL then text[i]!=NULL, rather than
       checking it all the time
     */
    if (flg & KS_SHIFT)
        cptr = shift && shift[i] ? shift[i] : "      ";
    else if (flg & KS_ALT)
        cptr = alt && alt[i] ? alt[i] : "      ";
    else if (flg & KS_CTRL)
        cptr = ctrl && ctrl[i] ? ctrl[i] : "      ";
    else cptr = norm && norm[i] ? norm[i] : "      ";

    ShowFunKey(ftext[i],cptr);
  }
  drawControlKeys(flg);
  twRefreshWin(HelpWnd);
  twUseWin(using);
}

void __FASTCALL__ __drawSinglePrompt(const char *prmt[])
{
  __drawMultiPrompt(prmt, NULL, NULL, NULL);
}


static const char * ShiftFxText[] =
{
  "ModHlp",
  "      ",
  "      ",
  "      ",
  "Where ",
  "SysInf",
  "NextSr",
  "Tools ",
  "      ",
  "FilUtl"
};

static const char * FxText[] =
{
  "Intro ",
  "ViMode",
  "More  ",
  "      ",
  "Goto  ",
  "ReRead",
  "Search",
  "      ",
  "Setup ",
  "Quit  "
};

static void __NEAR__ fillFxText( void )
{
  FxText[3] = activeMode->misckey_name ? activeMode->misckey_name() : NULL;
  FxText[7] = detectedFormat->showHdr || IsNewExe() ? "Header" : NULL;
}

void drawPrompt( void )
{
  fillFxText();
  __drawMultiPrompt(FxText, ShiftFxText, detectedFormat->prompt, activeMode->prompt);
}

static const char * amenu_names[] =
{
   "~Base",
   "~Alternative",
   "~Format depended",
   "~Mode depended"
};

int MainActionFromMenu( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(amenu_names)/sizeof(char *);
  i = SelBoxA(amenu_names,nModes," Select action: ",0);
  if(i != -1)
  {
    switch(i)
    {
	default:
	case 0:
		fillFxText();
		i = SelBoxA(FxText,10," Select base action: ",0);
		if(i!=-1) return KE_F(i+1);
		break;
	case 1:
		i = SelBoxA(ShiftFxText,10," Select alternative action: ",0);
		if(i!=-1) return KE_SHIFT_F(i+1);
		break;
	case 2:
		i = SelBoxA(detectedFormat->prompt,10," Select format depended action: ",0);
		if(i!=-1) return KE_ALT_F(i+1);
		break;
	case 3:
		i = SelBoxA(activeMode->prompt,10," Select mode depended action: ",0);
		if(i!=-1) return KE_CTL_F(i+1);
		break;
    }
  }
  return 0;
}

static const char * fetext[] =
{
  "Help  ",
  "Update",
  "InitXX",
  "Not   ",
  "Or  XX",
  "And XX",
  "Xor XX",
  "Put XX",
  "Undo  ",
  "Escape"
};

static const char * casmtext[] =
{
  "AsmRef",
  "SysInf",
  "Tools ",
  "Assemb",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      "
};

static const char * empttext[] =
{
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "Escape"
};
static const char * emptlsttext[] =
{
  "      ",
  "      ",
  "      ",
  "SaveAs",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      "
};

void drawEditPrompt( void )
{
  __drawSinglePrompt(fetext);
}

/*
int EditActionFromMenu( void )
{
  int i;
  i = SelBoxA(fetext,10," Select editor's action: ",0);
  if(i != -1) return KE_F(i+1);
  return 0;
}
*/

void drawEmptyPrompt( void )
{
  __drawSinglePrompt(empttext);
}

void drawEmptyListPrompt( void )
{
  __drawSinglePrompt(emptlsttext);
}

void drawAsmEdPrompt( void )
{
  __drawMultiPrompt(fetext, NULL, NULL, casmtext);
}

int EditAsmActionFromMenu( void )
{
  int i;
  i = SelBoxA(amenu_names,2," Select asm editor's action: ",0);
  if(i != -1)
  {
    switch(i)
    {
	default:
	case 0:
		fillFxText();
		i = SelBoxA(fetext,10," Select base action: ",0);
		if(i!=-1) return KE_F(i+1);
		break;
	case 1:
		i = SelBoxA(casmtext,10," Select alternative action: ",0);
		if(i!=-1) return KE_CTL_F(i+1);
		break;
    }
  }
  return 0;
}

static const char * ordlisttxt[] =
{
  "      ",
  "SrtNam",
  "SrtOrd",
  "SaveAs",
  "      ",
  "      ",
  "Search",
  "      ",
  "      ",
  "Escape"
};

static const char * listtxt[] =
{
  "      ",
  "Sort  ",
  "      ",
  "SaveAs",
  "      ",
  "      ",
  "Search",
  "      ",
  "      ",
  "Escape"
};

static const char * searchlisttxt[] =
{
  "      ",
  "      ",
  "      ",
  "SaveAs",
  "      ",
  "      ",
  "Search",
  "      ",
  "      ",
  "Escape"
};

static const char * shlisttxt[] =
{
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "NextSr",
  "      ",
  "      ",
  "      "
};

static const char * helptxt[] =
{
  "Licenc",
  "KeyHlp",
  "Credit",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "Escape"
};

static const char * helplisttxt[] =
{
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "Search",
  "      ",
  "      ",
  "Escape"
};

void drawListPrompt( void )
{
  __drawMultiPrompt(listtxt, shlisttxt, NULL, NULL);
}

void drawOrdListPrompt( void )
{
  __drawMultiPrompt(ordlisttxt, shlisttxt, NULL, NULL);
}

void drawSearchListPrompt( void )
{
  __drawMultiPrompt(searchlisttxt, shlisttxt, NULL, NULL);
}

void drawHelpPrompt( void )
{
  __drawSinglePrompt(helptxt);
}

int HelpActionFromMenu( void )
{
  int i;
  i = SelBoxA(helptxt,10," Select help action: ",0);
  if(i != -1) return KE_F(i+1);
  return 0;
}

void drawHelpListPrompt( void )
{
  __drawMultiPrompt(helplisttxt, shlisttxt, NULL, NULL);
}

typedef struct tagvbyte
{
  char x;
  char y;
  char image;
}vbyte;

typedef struct tagcvbyte
{
  char x;
  char y;
  char image;
  Color color;
}cvbyte;

const vbyte stars[] = {
{ 51,8,'.' },
{ 48,6,'*' },
{ 50,11,'*' },
{ 49,9,'.' },
{ 69,6,'*' },
{ 72,11,'*' },
{ 68,10,'.' },
{ 71,8,'.' },
{ 70,12,'*' },
{ 48,12,'.' }
};

const cvbyte buttons[] = {
{ 65,12,0x07, LightRed },
{ 66,12,0x07, LightGreen },
{ 67,12,0x07, LightCyan },
{ 55,12,TWC_SH, Black },
{ 56,12,TWC_SH, Black }
};

void About( void )
{
 TWindow * hwnd;
 int i;
 char str[2];
 const char core[8] = { TWC_LT_SHADE, TWC_LT_SHADE, TWC_LT_SHADE, TWC_LT_SHADE, TWC_LT_SHADE, TWC_LT_SHADE, TWC_LT_SHADE, 0x00 };
 hwnd = WindowOpen(0,0,73,13,TWS_FRAMEABLE | TWS_NLSOEM);
 twCentredWin(hwnd,NULL);
 twSetColor(LightCyan,Black);
 twClearWin();
 twSetFrame(hwnd,TW_DOUBLE_FRAME,White,Black);
 twSetTitle(hwnd,BIEW_VER_MSG,TW_TMODE_CENTER,White,Black);
 twShowWin(hwnd);

 twUseWin(hwnd);
 twFreezeWin(hwnd);
 twGotoXY(1,1); twPutS(msgAboutText);
 twTextColor(White);
 for(i = 0;i < 13;i++)  { twGotoXY(47,i + 1); twPutChar(TWC_SV); }
 for(i = 0;i < 47;i++) { twGotoXY(i + 1,6); twPutChar(TWC_SH);  }
 twGotoXY(47,6); twPutChar(TWC_SV_Sl);
 twTextColor(LightRed);
 for(i = 0;i < 5;i++)
 {
   Color bcol[5] = { White, LightCyan, Cyan, LightBlue, Blue };
   twTextColor(bcol[i]);
   twGotoXY(49,i + 1);
   twPutS(BiewPicture[i]);
 }
 twTextColor(LightGreen); twTextBkGnd(Green);
 for(i = 0;i < 7;i++)
 {
   twGotoXY(1,i+7); twPutS(MBoardPicture[i]);
 }
 twinDrawFrame(3,8,13,12,TW_UP3D_FRAME,White,LightGray);
 twinDrawFrame(4,9,12,11,TW_DN3D_FRAME,Black,LightGray);
 twGotoXY(5,10);
 twPutS(core);
 twTextColor(Brown); twTextBkGnd(Black);
 for(i = 0;i < 7;i++)
 {
   twGotoXY(17,i+7); twPutS(ConnectorPicture[i]);
 }
 twTextColor(Gray); twTextBkGnd(Black);
 for(i = 0;i < 7;i++)
 {
   twGotoXY(22,i+7); twPutS(BitStreamPicture[i]);
 }
 twTextColor(LightGray); twTextBkGnd(Black);
 for(i = 0;i < 7;i++)
 {
   twGotoXY(31,i+7); twPutS(BiewerPicture[i]);
 }
 twTextColor(LightCyan); twTextBkGnd(Blue);
 for(i = 0;i < 5;i++)
 {
   twGotoXY(32,i+8); twPutS(BiewerScreenPicture[i]);
 }
 twTextBkGnd(Black);   twTextColor(LightCyan);
 for(i = 0;i < 10;i++) twDirectWrite(stars[i].x,stars[i].y,&stars[i].image,1);
 twTextColor(LightGray);
 for(i = 0;i < 7;i++)
 {
   twGotoXY(52,i + 6);
   twPutS(CompPicture[i]);
 }
 twTextBkGnd(LightGray);
 for(i = 0;i < 5;i++)
 {
   twTextColor(buttons[i].color);
   str[0] = buttons[i].image;
   str[1] = 0;
   twDirectWrite(buttons[i].x,buttons[i].y,str,1);
 }
 twSetColor(Yellow,Blue);
 for(i = 0;i < 4;i++)
 {
   twGotoXY(54,i + 7);
   twPutS(CompScreenPicture[i]);
 }
 twRefreshWin(hwnd);
 while(1)
 {
   int ch;
   ch = GetEvent(drawHelpPrompt,HelpActionFromMenu,hwnd);
   switch(ch)
   {
      case KE_ESCAPE:
      case KE_F(10):  goto bye_help;
      case KE_F(1):   hlpDisplay(1); break;
      case KE_F(2):   hlpDisplay(3); break;
      case KE_F(3):   hlpDisplay(4); break;
      default:        break;
   }
 }
 bye_help:
 CloseWnd(hwnd);
}

__filesize_t __FASTCALL__ WhereAMI(__filesize_t ctrl_pos)
{
  TWindow *hwnd,*wait_wnd;
  char vaddr[64],prev_func[61],next_func[61],oname[25];
  const char *btn;
  int obj_class,obj_bitness;
  unsigned obj_num,func_class;
  __filesize_t obj_start,obj_end;
  __filesize_t cfpos,ret_addr,va,prev_func_pa,next_func_pa;
  hwnd = CrtDlgWndnls(" Current position information ",78,5);
  twSetFooterAttr(hwnd,"[Enter] - Prev. entry [Ctrl-Enter | F5] - Next entry]",TW_TMODE_RIGHT,dialog_cset.selfooter);
  twGotoXY(1,1);
  wait_wnd = PleaseWaitWnd();
  cfpos = BMGetCurrFilePos();
  if(detectedFormat->set_state) detectedFormat->set_state(PS_ACTIVE);
  if(detectedFormat->prepare_structs)
           detectedFormat->prepare_structs(ctrl_pos,ctrl_pos);
  va = detectedFormat->pa2va ? detectedFormat->pa2va(ctrl_pos) : ctrl_pos;
  vaddr[0] = '\0';
#if (__WORDSIZE >= 32) && !defined(__QNX4__)
  sprintf(&vaddr[strlen(vaddr)],"%016llXH",va);
#else
  sprintf(&vaddr[strlen(vaddr)],"%08lXH",va);
#endif
  prev_func_pa = next_func_pa = 0;
  prev_func[0] = next_func[0] = '\0';
  if(detectedFormat->GetPubSym)
  {
     prev_func_pa = detectedFormat->GetPubSym(prev_func,sizeof(prev_func),
                                              &func_class,ctrl_pos,True);
     next_func_pa = detectedFormat->GetPubSym(next_func,sizeof(next_func),
                                              &func_class,ctrl_pos,False);
  }
  prev_func[sizeof(prev_func)-1] = next_func[sizeof(next_func)-1] = '\0';
  if(detectedFormat->GetObjAttr)
  {
     obj_num = detectedFormat->GetObjAttr(ctrl_pos,oname,sizeof(oname),
                                          &obj_start,&obj_end,&obj_class,
                                          &obj_bitness);
     oname[sizeof(oname)-1] = 0;
  }
  else
  {
    obj_num = 0;
    oname[0] = 0;
    obj_start = 0;
    obj_end = BMGetFLength();
    obj_class = OC_CODE;
    obj_bitness = DAB_USE16;
  }
  CloseWnd(wait_wnd);
  switch(obj_bitness)
  {
    case DAB_USE16: btn = "USE16"; break;
    case DAB_USE32: btn = "USE32"; break;
    case DAB_USE64: btn = "USE64"; break;
    case DAB_USE128:btn = "USE128"; break;
    case DAB_USE256:btn = "USE256"; break;
    default: btn = "";
  }
  twPrintF(
#if (__WORDSIZE >= 32) && !defined(__QNX4__)
	   "File  offset : %016llXH\n"
#else
	   "File  offset : %08lXH\n"
#endif
           "Virt. address: %s\n"
           "%s entry  : %s\n"
           "Next  entry  : %s\n"
#if (__WORDSIZE >= 32) && !defined(__QNX4__)
           "Curr. object : #%u %s %s %016llXH=%016llXH %s"
#else
           "Curr. object : #%u %s %s %08lXH=%08lXH %s"
#endif
           ,ctrl_pos
           ,vaddr
           ,prev_func_pa == ctrl_pos ? "Curr." : "Prev."
           ,prev_func
           ,next_func
           ,obj_num
           ,obj_class == OC_CODE ? "CODE" : obj_class == OC_DATA ? "DATA" : "no obj."
           ,btn
           ,obj_start
           ,obj_end
           ,oname
           );
  ret_addr = ctrl_pos;
  while(1)
  {
    int ch;
    ch = GetEvent(drawEmptyPrompt,NULL,hwnd);
    switch(ch)
    {
      case KE_F(10):
      case KE_ESCAPE: goto exit;
      case KE_ENTER:
                    {
                      if(prev_func_pa)
                      {
                        ret_addr = prev_func_pa;
                      }
                      else ErrMessageBox(NOT_ENTRY,NULL);
                    }
                    goto exit;
      case KE_F(5):
      case KE_CTL_ENTER:
                    {
                      if(next_func_pa)
                      {
                        ret_addr = next_func_pa;
                      }
                      else ErrMessageBox(NOT_ENTRY,NULL);
                    }
                    goto exit;
      default: break;
    }
  }
  exit:
  if(detectedFormat->drop_structs) detectedFormat->drop_structs();
  if(detectedFormat->set_state) detectedFormat->set_state(PS_INACTIVE);
  BMSeek(cfpos,BM_SEEK_SET);
  CloseWnd(hwnd);
  return ret_addr;
}
