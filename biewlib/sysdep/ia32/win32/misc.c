/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/win32/misc.c
 * @brief       Misc. functions for Win32 (optional!!!)
 * @version     -
 * @author      Nick Kurshev
 * @date        2003
**/
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "biewlib/biewlib.h"

extern HANDLE hIn;

int __FASTCALL__ __inputRawInfo(char *head, char *text)
{
  int rval=1;
  DWORD nread;
  INPUT_RECORD ir;
  ReadConsoleInput(hIn,&ir,1,&nread);
    switch(ir.EventType)
    {
      case WINDOW_BUFFER_SIZE_EVENT:
      {
	strcpy(head,"Type dwSize");
	sprintf(text,"Size %08lX"
                ,ir.Event.WindowBufferSizeEvent.dwSize
                );
      }
      break;
      case FOCUS_EVENT:
      {
	strcpy(head,"Type bSetFocus");
	sprintf(text,"Focs %i"
                ,ir.Event.FocusEvent.bSetFocus
                );
      }
      break;
      case MENU_EVENT:
      {
	strcpy(head,"Type dwCommandId");
	sprintf(text,"Menu %08lX"
                ,ir.Event.MenuEvent.dwCommandId
                );
      }
      break;
      case MOUSE_EVENT:
      {
	strcpy(head,"Type Position ButtonState ControlKeyState Flags");
	sprintf(text,"Mous %08lX %-11lX %-15lX %-5lX"
                ,ir.Event.MouseEvent.dwMousePosition
                ,ir.Event.MouseEvent.dwButtonState
                ,ir.Event.MouseEvent.dwControlKeyState
                ,ir.Event.MouseEvent.dwEventFlags
                );
      }
      break;
      case KEY_EVENT:
      {
	strcpy(head,"Type KDown RepeatCnt KeyCode ScanCode Ascii ControlKeyState");
	sprintf(text,"KEY  %-5i %-9i %-7X %-7X  %-5c %-15lX"
                ,ir.Event.KeyEvent.bKeyDown
                ,ir.Event.KeyEvent.wRepeatCount
                ,ir.Event.KeyEvent.wVirtualKeyCode
                ,ir.Event.KeyEvent.wVirtualScanCode
                ,ir.Event.KeyEvent.uChar.AsciiChar?ir.Event.KeyEvent.uChar.AsciiChar:' '
                ,ir.Event.KeyEvent.dwControlKeyState
                );

        if( ir.Event.KeyEvent.wVirtualKeyCode == 27 && ir.Event.KeyEvent.wVirtualScanCode == 1 &&
            !ir.Event.KeyEvent.uChar.AsciiChar )
            ir.Event.KeyEvent.uChar.AsciiChar = ir.Event.KeyEvent.wVirtualKeyCode;
        rval=ir.Event.KeyEvent.wVirtualKeyCode==27&&!ir.Event.KeyEvent.bKeyDown?0:1;
      }
      break;
      default: break;
    }
  return rval;
}
