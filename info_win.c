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

static const char * __NEAR__ __FASTCALL__ gettextfx( int i )
{
  if(i == 3) return activeMode->misckey_name ? activeMode->misckey_name() : "      ";
  else
    if(i == 7) return detectedFormat->showHdr || IsNewExe() ? "Header" : "      ";
    else
    return FxText[i];
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

void drawPrompt( void )
{
  TWindow * using;
  int flg = __kbdGetShiftsKey();
  int i;
  using = twUsedWin();
  twUseWin(HelpWnd);
  twFreezeWin(HelpWnd);
  twGotoXY(2,1);
  for(i = 0;i < 10;i++)
  {
    ShowFunKey(ftext[i],
               (flg & KS_ALT) ? (detectedFormat->prompt[i] ? detectedFormat->prompt[i] : "      ")
                           : (flg & KS_CTRL) ? (activeMode->prompt[i] ? activeMode->prompt[i] : "      ")
                           : (flg & KS_SHIFT) ? ShiftFxText[i]
                           : !(flg & KS_CTRL_MASK) ? gettextfx(i)
                           : "      ");
  }
  drawControlKeys(flg);
  twRefreshWin(HelpWnd);
  twUseWin(using);
}

void __FASTCALL__ __drawSinglePrompt(const char *prmt[])
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
    cptr = (!(flg & KS_CTRL_MASK)) ? prmt[i] : "      ";
    ShowFunKey(ftext[i],cptr);
  }
  drawControlKeys(flg);
  twRefreshWin(HelpWnd);
  twUseWin(using);
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
  "      ",
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
  TWindow * using;
  int flg = __kbdGetShiftsKey();
  int i;
  const char * cptr;
  using = twUsedWin();
  twUseWin(HelpWnd);
  twFreezeWin(HelpWnd);
  twGotoXY(2,1);
  for(i = 0;i < 10;i++)
  {
    if(!(flg & KS_CTRL_MASK)) cptr = fetext[i];
    else   if(flg & KS_CTRL)  cptr = casmtext[i];
           else            cptr = "      ";
    ShowFunKey(ftext[i],cptr);
  }
  drawControlKeys(flg);
  twRefreshWin(HelpWnd);
  twUseWin(using);
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
   __drawSinglePrompt(listtxt);
}

void drawOrdListPrompt( void )
{
   __drawSinglePrompt(ordlisttxt);
}

void drawSearchListPrompt( void )
{
   __drawSinglePrompt(searchlisttxt);
}

void drawHelpPrompt( void )
{
  __drawSinglePrompt(helptxt);
}

void drawHelpListPrompt( void )
{
  __drawSinglePrompt(helplisttxt);
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
   ch = GetEvent(drawHelpPrompt,hwnd);
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

unsigned long __FASTCALL__ WhereAMI(unsigned long ctrl_pos)
{
  TWindow *hwnd,*wait_wnd;
  char vaddr[64],prev_func[61],next_func[61],oname[25];
  const char *btn;
  int obj_class,obj_bitness;
  unsigned obj_num,func_class;
  unsigned long obj_start,obj_end;
  unsigned long cfpos;
  unsigned long va,prev_func_pa,next_func_pa,ret_addr;
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
  sprintf(&vaddr[strlen(vaddr)],"%08lXH",va);
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
  twPrintF("File  offset : %08lXH\n"
           "Virt. address: %s\n"
           "%s entry  : %s\n"
           "Next  entry  : %s\n"
           "Curr. object : #%u %s %s %08lXH=%08lXH %s"
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
    ch = GetEvent(drawEmptyPrompt,hwnd);
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
