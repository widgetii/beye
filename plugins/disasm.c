/**
 * @namespace   biew_plugins_I
 * @file        plugins/disasm.c
 * @brief       This file contains universal interface for any disassembler.
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
 * @author      Kostya Nosov <k-nosov@yandex.ru>
 * @date        12.09.2000
 * @note        Adding virtual address as argument of jump and call insns
 * @author      Mauro Giachero
 * @date        02.11.2007
 * @note        Reworked inline assemblers support
**/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include "colorset.h"
#include "bmfile.h"
#include "bin_util.h"
#include "plugins/hexmode.h"
#include "plugins/disasm.h"
#include "biewutil.h"
#include "reg_form.h"
#include "bconsole.h"
#include "editor.h"
#include "codeguid.h"
#include "search.h"
#include "tstrings.h"
#include "biewlib/file_ini.h"
#include "biewlib/biewlib.h"
#include "biewlib/pmalloc.h"
#include "biewlib/kbd_code.h"

extern REGISTRY_DISASM ix86_Disasm;
extern REGISTRY_DISASM Null_Disasm;
extern REGISTRY_DISASM AVR_Disasm;
extern REGISTRY_DISASM ARM_Disasm;
extern REGISTRY_DISASM PPC_Disasm;
extern REGISTRY_DISASM Java_Disasm;

static REGISTRY_DISASM *mainDisasmTable[] =
{
  &Null_Disasm,
  &ix86_Disasm,
  &Java_Disasm,
  &AVR_Disasm,
  &ARM_Disasm,
  &PPC_Disasm
};

static unsigned DefDisasmSel = __DEFAULT_DISASM;
REGISTRY_DISASM *activeDisasm = NULL;

static unsigned long PrevPageSize,CurrPageSize,PrevStrLen,CurrStrLen;
unsigned disNeedRef = 0;
int DisasmCurrLine;
unsigned disPanelMode = 0;
static int           HiLight = 1;
static unsigned char *CurrStrLenBuff = NULL;
static unsigned long *PrevStrLenAddr = NULL;
static char          LastPrevLen;
static char          PrevStrCount = 0;
static char *        disCodeBuffer = NULL,*disCodeBuf2 = NULL;
static char *        disCodeBufPredict = NULL;
static int           disMaxCodeLen;

tBool DisasmPrepareMode = False;

char *   dis_comments;
unsigned dis_severity;

static void __NEAR__ __FASTCALL__ disAcceptActions( void );

static tBool __FASTCALL__ disSelect_Disasm( void )
{
  const char *modeName[sizeof(mainDisasmTable)/sizeof(REGISTRY_DISASM *)];
  size_t i,nModes;
  int retval;

  nModes = sizeof(mainDisasmTable)/sizeof(REGISTRY_DISASM *);
  for(i = 0;i < nModes;i++) modeName[i] = mainDisasmTable[i]->name;
  retval = SelBoxA(modeName,nModes," Select disassembler: ",DefDisasmSel);
  if(retval != -1)
  {
    if(activeDisasm->term) activeDisasm->term();
    activeDisasm = mainDisasmTable[retval];
    DefDisasmSel = retval;
    disAcceptActions();
    return True;
  }
  return False;
}

static void __FASTCALL__ FillPrevAsmPage(__filesize_t bound,unsigned predist)
{
  __filesize_t distin,addr;
  unsigned j;
  unsigned totallen;
  tAbsCoord height = twGetClientHeight(MainWnd);
  char showref,addrdet;
  if(!predist) predist = height*disMaxCodeLen;
  predist = (predist/16)*16+16;   /** align on 16-byte boundary */
  distin = bound >= predist ? bound - predist : 0;
  memset(PrevStrLenAddr,TWC_DEF_FILLER,height*sizeof(long));
  PrevStrCount = 0;
  totallen = 0;
  showref = disNeedRef;
  addrdet = hexAddressResolv;
  hexAddressResolv = 0;
  disNeedRef = 0;
  DisasmPrepareMode = True;
  for(j = 0;;j++)
  {
    DisasmRet dret;
       addr = distin + totallen;
       BMReadBufferEx(disCodeBuffer,disMaxCodeLen,addr,BM_SEEK_SET);
       dret = Disassembler(distin,(void *)disCodeBuffer,__DISF_SIZEONLY);
       if(addr >= bound) break;
       totallen += dret.codelen;
       if(j < height) PrevStrLenAddr[j] = addr;
       else
       {
          memmove(PrevStrLenAddr,&PrevStrLenAddr[1],(height - 1)*sizeof(long));
          PrevStrLenAddr[height - 1] = addr;
       }
  }
  PrevStrCount = j < height ? j : height;
  LastPrevLen = PrevStrCount ? bound - PrevStrLenAddr[PrevStrCount - 1] : 0;
  disNeedRef = showref;
  DisasmPrepareMode = False;
  hexAddressResolv = addrdet;
}

static void __FASTCALL__ PrepareAsmLines(int keycode,__filesize_t cfpos)
{
  tAbsCoord height = twGetClientHeight(MainWnd);
  switch(keycode)
  {
    case KE_DOWNARROW:
                    PrevStrLen = CurrStrLenBuff[0];
                    CurrStrLen = CurrStrLenBuff[1];
                    if((unsigned)PrevStrCount < height) PrevStrCount++;
                    else                                memmove(PrevStrLenAddr,&PrevStrLenAddr[1],sizeof(long)*(height - 1));
                    PrevStrLenAddr[PrevStrCount - 1] = cfpos - CurrStrLenBuff[0];
                    PrevPageSize = PrevStrLenAddr[PrevStrCount - 1] - PrevStrLenAddr[0] + PrevStrLen;
                    break;
    case KE_PGDN:
                    {
                      size_t i;
                      unsigned size;
                      __filesize_t prevpos;
                      size = Summ(CurrStrLenBuff,height);
                      prevpos = cfpos - size;
                      size = 0;
                      for(i = 0;i < height;i++)
                      {
                          size += CurrStrLenBuff[i];
                          PrevStrLenAddr[i] = prevpos + size;
                      }
                      PrevStrLen = CurrStrLenBuff[height - 1];
                      PrevPageSize = size;
                      PrevStrCount = height;
                    }
                    break;
   case KE_UPARROW:
                    FillPrevAsmPage(cfpos,(unsigned)(PrevPageSize + 15));
                    goto Calc;
    default:
                    FillPrevAsmPage(cfpos,0);
    Calc:
                    if(PrevStrCount)
                    {
                      PrevStrLen = LastPrevLen;
                      PrevPageSize = PrevStrLenAddr[PrevStrCount - 1] + LastPrevLen - PrevStrLenAddr[0];
                    }
                    else
                    {
                      PrevStrLen = 0;
                      PrevPageSize = 0;
                    }
                    break;
  }
}

static unsigned __FASTCALL__ drawAsm( unsigned keycode, unsigned textshift )
{
 int i,I,Limit,dir,orig_commpos,orig_commoff;
 size_t j,len,len_64;
 __filesize_t cfpos,flen,TopCFPos;
 static __filesize_t amocpos = 0L;
 char outstr[__TVIO_MAXSCREENWIDTH];
 char savstring[20];
 HLInfo hli;
 ColorAttr cattr;
 flen = BMGetFLength();
 cfpos = TopCFPos = BMGetCurrFilePos();
 if(keycode == KE_UPARROW)
 {
   char showref,addrdet;
   DisasmRet dret;
   showref = disNeedRef;
   addrdet = hexAddressResolv;
   disNeedRef = hexAddressResolv = 0;
   BMReadBufferEx(disCodeBuffer,disMaxCodeLen,cfpos,BM_SEEK_SET);
   DisasmPrepareMode = True;
   dret = Disassembler(cfpos,(void *)disCodeBuffer,__DISF_SIZEONLY);
   if(cfpos + dret.codelen != amocpos && cfpos && amocpos) keycode = KE_SUPERKEY;
   DisasmPrepareMode = False;
   disNeedRef = showref;
   hexAddressResolv = addrdet;
 }
 PrepareAsmLines(keycode,cfpos);
 if(amocpos != cfpos || keycode == KE_SUPERKEY || keycode == KE_JUSTFIND)
 {
   tAbsCoord height = twGetClientHeight(MainWnd);
   tAbsCoord width = twGetClientWidth(MainWnd);
   GidResetGoAddress(keycode);
   I = 0;
   twFreezeWin(MainWnd);
   if(keycode == KE_UPARROW)
   {
     dir = -1;
     Limit = -1;
     /* All checks we have done above */
     twScrollWinDn(MainWnd,1,1);
     memmove(&CurrStrLenBuff[1],CurrStrLenBuff,height-1);
   }
   else
   {
     dir = 1;
     Limit = height;
   }
   if(keycode == KE_DOWNARROW)
   {
     if(CurrStrLenBuff[1])
     {
       I = height-1;
       twScrollWinUp(MainWnd,I,1);
       memmove(CurrStrLenBuff,&CurrStrLenBuff[1],I);
       cfpos += Summ(CurrStrLenBuff,I);
     }
     else
     {
       twRefreshWin(MainWnd);
       goto bye;
     }
   }
   if(cfpos > flen) cfpos = flen;
   amocpos = cfpos;
   twUseWin(MainWnd);
   for(i = I;i != Limit;i+=1*dir)
   {
     DisasmRet dret;
     memset(outstr,TWC_DEF_FILLER,__TVIO_MAXSCREENWIDTH);
     DisasmCurrLine = i;
     if(cfpos < flen)
     {
       len = cfpos + disMaxCodeLen < flen ? disMaxCodeLen : (int)(flen - cfpos);
       memset(disCodeBuffer,0,disMaxCodeLen);
       BMReadBufferEx((void *)disCodeBuffer,len,cfpos,BM_SEEK_SET);
       dret = Disassembler(cfpos,(void *)disCodeBuffer,__DISF_NORMAL);
       if(i == 0) CurrStrLen = dret.codelen;
       CurrStrLenBuff[i] = dret.codelen;
       twSetColorAttr(browser_cset.main);
       len_64=HA_LEN;
       memcpy(outstr,GidEncodeAddress(cfpos,hexAddressResolv),len_64);
       len = 0;
       if(disPanelMode < PANMOD_FULL)
       {
         static char _clone;
         len = len_64;
         twDirectWrite(1,i + 1,outstr,len);
         if(!hexAddressResolv)
         {
           twSetColorAttr(disasm_cset.family_id);
           _clone = activeDisasm->CloneShortName(dret.pro_clone);
           twDirectWrite(len_64,i + 1,&_clone,1);
           twSetColorAttr(browser_cset.main);
         }
       }
       if(disPanelMode < PANMOD_MEDIUM)
       {
         unsigned full_off,med_off,tmp_off;
         ColorAttr opc;
         med_off = disMaxCodeLen*2+1;
         full_off = med_off+len_64;
         for(j = 0;j < dret.codelen;j++,len+=2)
            memcpy(&outstr[len],Get2Digit(disCodeBuffer[j]),2);
         tmp_off = disPanelMode < PANMOD_FULL ? full_off : med_off;
         if(len < tmp_off) len = tmp_off;
         if(activeDisasm->GetOpcodeColor) 
		opc =	HiLight == 2 ? activeDisasm->altGetOpcodeColor(dret.pro_clone) :
			HiLight == 1 ? activeDisasm->GetOpcodeColor(dret.pro_clone) : disasm_cset.opcodes;
	 else	opc = disasm_cset.opcodes;
         twSetColorAttr(opc);
         twDirectWrite(disPanelMode < PANMOD_FULL ? len_64+1 : 1,
                       i + 1,
                       &outstr[len_64],
                       disPanelMode < PANMOD_FULL ? len - (len_64+1) : len - 1);
         if(isHOnLine(cfpos,dret.codelen))
         {
            hli.text = &outstr[len_64];
            HiLightSearch(MainWnd,cfpos,len_64,dret.codelen,i,&hli,HLS_USE_DOUBLE_WIDTH);
         }
       }
       twSetColorAttr(browser_cset.main);
       twDirectWrite(len,i + 1," ",1);  len++;
       cattr =	HiLight == 2 ?  activeDisasm->altGetInsnColor(dret.pro_clone) :
		HiLight == 1 ?  activeDisasm->GetInsnColor(dret.pro_clone) :
                                browser_cset.main;
       twSetColorAttr(cattr);
       j = strlen(dret.str);
       /* Here adding commentaries */
       savstring[0] = 0;
       orig_commoff = orig_commpos = 0;
       if(j > 5)
       if(dret.str[j-5] == codeguid_image[0] &&
          dret.str[j-4] == codeguid_image[1] &&
          dret.str[j-3] == codeguid_image[2] &&
          dret.str[j-1] == codeguid_image[4] &&
          dis_severity > DISCOMSEV_NONE)
       {
         int new_idx;
         orig_commpos = new_idx = j-5;
         orig_commoff = len;
         strcpy(savstring,&dret.str[new_idx]);
         dret.str[new_idx--] = 0;
         while(dret.str[new_idx] == ' ' && new_idx) new_idx--;
         if(dret.str[new_idx] != ' ') new_idx++;
         dret.str[new_idx] = 0;
         j = strlen(dret.str);
       }
       twDirectWrite(len,i+1,dret.str,j); len += j;
       if(dis_severity > DISCOMSEV_NONE)
       {
         twSetColorAttr(disasm_cset.comments);
         twGotoXY(len,i+1);
         twPutS(" ; "); len+=3;
         j = orig_commpos-len;
         j = min(j,strlen(dis_comments));
         twDirectWrite(len,i+1,dis_comments,j);
         len += j;
         if(savstring[0])
         {
           twGotoXY(len,i+1);
           twClrEOL();
           twSetColorAttr(cattr);
           len = orig_commoff + orig_commpos;
           twDirectWrite(len,i+1,savstring,5);
           len += 5;
         }
       }
       twSetColorAttr(browser_cset.main);
       if(len < width)
       {
         twGotoXY(len,i + 1);
         twClrEOL();
       }
       cfpos += dret.codelen;
       BMSeek(cfpos,BM_SEEK_SET);
     }
     else
     {
       twDirectWrite(1,i + 1,outstr,width);
       CurrStrLenBuff[i] = 0;
     }
   }
   twRefreshWin(MainWnd);
   twSetColorAttr(browser_cset.main);
   lastbyte = TopCFPos + Summ(CurrStrLenBuff,height);
   CurrPageSize = lastbyte-TopCFPos;
 }
 bye:
 return textshift;
}

static unsigned long __FASTCALL__ disPrevPageSize( void ) { return PrevPageSize; }
static unsigned long __FASTCALL__ disCurrPageSize( void ) { return CurrPageSize; }
static unsigned long __FASTCALL__ disPrevLineWidth( void ) { return PrevStrLen; }
static unsigned long __FASTCALL__ disCurrLineWidth( void ) { return CurrStrLen; }
static const char *  __FASTCALL__ disMiscKeyName( void ) { return "Modify"; }

static const char *refsdepth_names[] =
{
   "~None",
   "~Calls/jmps only (navigation)",
   "~All",
   "Reference ~prediction (mostly full)"
};

static tBool __FASTCALL__ disReferenceResolving( void )
{
  size_t nModes;
  int retval;
  tBool ret;
  nModes = sizeof(refsdepth_names)/sizeof(char *);
  retval = SelBoxA(refsdepth_names,nModes," Reference resolving depth: ",disNeedRef);
  if(retval != -1)
  {
    disNeedRef = retval;
    ret = True;
  }
  else ret = False;
  if(detectedFormat->set_state)
    detectedFormat->set_state(disNeedRef ? PS_ACTIVE : PS_INACTIVE);
  return ret;
}

static const char *panmod_names[] =
{
   "~Full",
   "~Medium",
   "~Wide"
};

static tBool __FASTCALL__ disSelectPanelMode( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(panmod_names)/sizeof(char *);
  i = SelBoxA(panmod_names,nModes," Select panel mode: ",disPanelMode);
  if(i != -1)
  {
    disPanelMode = i;
    return True;
  }
  return False;
}

static const char *hilight_names[] =
{
   "~Mono",
   "~Highlight",
   "~Alt Highlight"
};

static tBool __FASTCALL__ disSelectHiLight( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(hilight_names)/sizeof(char *);
  i = SelBoxA(hilight_names,nModes," Select highlight mode: ",HiLight);
  if(i != -1)
  {
    HiLight = i;
    return True;
  }
  return False;
}

static tBool __FASTCALL__ disDetect( void ) { return True; }

static tBool __FASTCALL__ DefAsmAction(int _lastbyte,int start)
{
 int _index;
 tBool redraw,dox;
 char xx;
  redraw = True;
  xx = edit_x / 2;
  dox = True;
  _index = start + xx;
   switch(_lastbyte)
   {
     case KE_F(4)   : EditorMem.buff[_index] = ~EditorMem.buff[_index]; break;
     case KE_F(5)   : EditorMem.buff[_index] |= edit_XX; break;
     case KE_F(6)   : EditorMem.buff[_index] &= edit_XX; break;
     case KE_F(7)   : EditorMem.buff[_index] ^= edit_XX; break;
     case KE_F(8)   : EditorMem.buff[_index]  = edit_XX; break;
     case KE_F(9)   : EditorMem.buff[_index] = EditorMem.save[_index]; break;
     default        : redraw = edit_defaction(_lastbyte); dox = False; break;
   }
  if(dox) { xx++; edit_x = xx*2; }
  return redraw;
}

static void __FASTCALL__ DisasmScreen(TWindow* ewnd,__filesize_t cp,__filesize_t flen,int st,int stop,int start)
{
 int i,j,len,lim,len_64;
 char outstr[__TVIO_MAXSCREENWIDTH+1],outstr1[__TVIO_MAXSCREENWIDTH+1];
 tAbsCoord width = twGetClientWidth(MainWnd);
 DisasmRet dret;
 twFreezeWin(ewnd);
 for(i = st;i < stop;i++)
 {
  if(start + cp < flen)
  {
   len_64=HA_LEN;
   memcpy(outstr,GidEncodeAddress(cp + start,hexAddressResolv),len_64);
   twUseWin(MainWnd);
   twSetColorAttr(browser_cset.main);
   twDirectWrite(1,i + 1,outstr,len_64-1);
   dret = Disassembler(cp + start,&EditorMem.buff[start],__DISF_NORMAL);
   EditorMem.alen[i] = dret.codelen;
   memset(outstr,TWC_DEF_FILLER,width);
   memset(outstr1,TWC_DEF_FILLER,width);
   len = 0; for(j = 0;j < EditorMem.alen[i];j++) { memcpy(&outstr1[len],Get2Digit(EditorMem.save[start + j]),2); len += 2; }
   len = 0; for(j = 0;j < EditorMem.alen[i];j++) { memcpy(&outstr[len],Get2Digit(EditorMem.buff[start + j]),2); len += 2; }
   twUseWin(ewnd);
   len = disMaxCodeLen*2;
   for(j = 0;j < len;j++)
   {
     twSetColorAttr(outstr[j] == outstr1[j] ? browser_cset.edit.main : browser_cset.edit.change);
     twDirectWrite(j + 1,i + 1,&outstr[j],1);
   }
   len = strlen(dret.str);
   memset(outstr,TWC_DEF_FILLER,width);
   memcpy(outstr,dret.str,len);
   twUseWin(MainWnd);
   twSetColorAttr(browser_cset.main);
   lim = disMaxCodeLen*2+len_64+1;
   twDirectWrite(lim+1,i + 1,outstr,width-lim);
   start += EditorMem.alen[i];
  }
  else
  {
    twUseWin(MainWnd);
    twGotoXY(1,i + 1);
    twClrEOL();
    twUseWin(ewnd);
    twGotoXY(1,i + 1);
    twClrEOL();
    EditorMem.alen[i] = 0;
  }
 }
 twRefreshWin(ewnd);
}

static int __NEAR__ __FASTCALL__ FullAsmEdit(TWindow * ewnd)
{
 int j,_lastbyte,start;
 unsigned rlen,len,flags;
 __filesize_t flen;
 unsigned max_buff_size = disMaxCodeLen*tvioHeight;
 tAbsCoord height = twGetClientHeight(MainWnd);
 tBool redraw = False;
 char outstr[__TVIO_MAXSCREENWIDTH],owork[__TVIO_MAXSCREENWIDTH];

 flen = BMGetFLength();
 edit_cp = BMGetCurrFilePos();
 start = 0;

 rlen = (__filesize_t)edit_cp + max_buff_size < flen ? max_buff_size : (unsigned)(flen - edit_cp);
 BMReadBufferEx((void *)EditorMem.buff,rlen,edit_cp,BM_SEEK_SET);
 memcpy(EditorMem.save,EditorMem.buff,max_buff_size);
 memset(EditorMem.alen,TWC_DEF_FILLER,height);

 DisasmScreen(ewnd,edit_cp,flen,0,height,start);
 PaintETitle(0,True);
 start = 0;
 twShowWin(ewnd);
 twSetCursorType(TW_CUR_NORM);
 while(1)
 {
   twUseWin(ewnd);

   len = 0; for(j = 0;j < EditorMem.alen[edit_y];j++) { memcpy(&owork[len],Get2Digit(EditorMem.save[start + j]),2); len += 2; }
   len = 0; for(j = 0;j < EditorMem.alen[edit_y];j++) { memcpy(&outstr[len],Get2Digit(EditorMem.buff[start + j]),2); len += 2; }
   flags = __ESS_FILLER_7BIT | __ESS_WANTRETURN | __ESS_HARDEDIT;
   if(!redraw) flags |= __ESS_NOREDRAW;
   _lastbyte = eeditstring(outstr,&legalchars[2],&len,(unsigned)(edit_y + 1),(unsigned *)&edit_x,
                          flags,owork,NULL);
   CompressHex(&EditorMem.buff[start],outstr,EditorMem.alen[edit_y],False);
   switch(_lastbyte)
   {
     case KE_CTL_F(4):
                      {
                       AsmRet aret;
                       if(activeDisasm->asm_f)
                       {
                         char code[81];
                         code[0]='\0';
                         if(GetStringDlg(code,activeDisasm->name,
                                         NULL,"Enter assembler instruction:"))
                         {
                           aret = (*activeDisasm->asm_f)(code);
                           if(aret.err_code)
                           {
                              char *message="Syntax error";
                              if (aret.insn[0])
                              {
                                message=aret.insn;
                              }
                              ErrMessageBox(message,NULL);
                              continue;
                           }
                           else
                           {
                              int i;
                              char bytebuffer[3];

                              for (i=aret.insn_len-1; i>=0; i--)
                              {
                                sprintf(bytebuffer, "%0.2x", aret.insn[i]);
                                ungotstring(bytebuffer);
                              }
                           }
                         }
                         break;
                       }
                       else
                       {
                         ErrMessageBox("Sorry, no assembler available",NULL);
                         continue;
                       }
                      }
     case KE_F(1)    : ExtHelp(); continue;
     case KE_CTL_F(1): activeDisasm->action[0](); continue;
     case KE_CTL_F(2): SelectSysInfo(); continue;
     case KE_CTL_F(3): SelectTool(); continue;
     case KE_F(2)    :
                      {
                         BGLOBAL bHandle;
                         char *fname;
                         fname = BMName();
                         if((bHandle = biewOpenRW(fname,BBIO_SMALL_CACHE_SIZE)) != &bNull)
                         {
                           bioSeek(bHandle,edit_cp,BIO_SEEK_SET);
                           if(!bioWriteBuffer(bHandle,(void *)EditorMem.buff,rlen))
                              errnoMessageBox(WRITE_FAIL,NULL,errno);
                           bioClose(bHandle);
                           BMReRead();
                         }
                         else errnoMessageBox("Can't reopen",NULL,errno);
                      }
     case KE_F(10):
     case KE_ESCAPE: goto bye;
     default: redraw = DefAsmAction(_lastbyte,start); break;
   }
   CheckYBounds();
   start = edit_y ? Summ(EditorMem.alen,edit_y) : 0;
   if(redraw)
   {
     DisasmRet dret;
     dret = Disassembler(edit_cp + start,&EditorMem.buff[start],__DISF_NORMAL);
     EditorMem.alen[edit_y] = dret.codelen;
     DisasmScreen(ewnd,edit_cp,flen,0,height,0);
   }
   PaintETitle(start + edit_x/2,True);
   CheckXYBounds();
   start = edit_y ? Summ(EditorMem.alen,edit_y) : 0;
 }
 bye:
 twSetCursorType(TW_CUR_OFF);
 return _lastbyte;
}

static void __FASTCALL__ disEdit( void )
{
 unsigned len_64;
 TWindow * ewnd;
 len_64=HA_LEN;
 if(!BMGetFLength()) { ErrMessageBox(NOTHING_EDIT,NULL); return; }
 ewnd = WindowOpen(len_64+1,2,disMaxCodeLen*2+len_64+1,tvioHeight-1,TWS_CURSORABLE);
 twSetColorAttr(browser_cset.edit.main); twClearWin();
 edit_x = edit_y = 0;
 EditMode = EditMode ? False : True;
 if(editInitBuffs(disMaxCodeLen,NULL,0))
 {
   FullAsmEdit(ewnd);
   editDestroyBuffs();
 }
 EditMode = EditMode ? False : True;
 CloseWnd(ewnd);
 PaintTitle();
}

static void __FASTCALL__ HelpAsm( void )
{
   if( activeDisasm->ShowShortHelp ) activeDisasm->ShowShortHelp();
}

static void __FASTCALL__ disReadIni( hIniProfile *ini )
{
  char tmps[10];
  if(isValidIniArgs())
  {
    biewReadProfileString(ini,"Biew","Browser","LastSubMode","0",tmps,sizeof(tmps));
    DefDisasmSel = (int)strtoul(tmps,NULL,10);
    if(DefDisasmSel >= sizeof(mainDisasmTable)/sizeof(REGISTRY_DISASM *)) DefDisasmSel = 0;
    ReadIniAResolv(ini);
    biewReadProfileString(ini,"Biew","Browser","SubSubMode7","0",tmps,sizeof(tmps));
    disPanelMode = (int)strtoul(tmps,NULL,10);
    if(disPanelMode > PANMOD_FULL) disPanelMode = 0;
    biewReadProfileString(ini,"Biew","Browser","SubSubMode8","0",tmps,sizeof(tmps));
    disNeedRef = (int)strtoul(tmps,NULL,10);
    if(disNeedRef > NEEDREF_PREDICT) disNeedRef = 0;
    biewReadProfileString(ini,"Biew","Browser","SubSubMode9","0",tmps,sizeof(tmps));
    HiLight = (int)strtoul(tmps,NULL,10);
    if(HiLight > 2) HiLight = 2;
    activeDisasm = mainDisasmTable[DefDisasmSel];
    disAcceptActions();
    if(activeDisasm->read_ini) activeDisasm->read_ini(ini);
  }
}

static void __FASTCALL__ disInit( void )
{
  unsigned i;
  int def_platform;
  CurrStrLenBuff = PMalloc(tvioHeight);
  PrevStrLenAddr = PMalloc(tvioHeight*sizeof(long));
  dis_comments   = PMalloc(DISCOM_SIZE*sizeof(char));
  if((!CurrStrLenBuff) || (!PrevStrLenAddr) || (!dis_comments))
  {
    err:
    MemOutBox("Disassembler initialization");
    exit(EXIT_FAILURE);
  }
  def_platform = DISASM_DATA;
  if(detectedFormat->query_platform) def_platform = detectedFormat->query_platform();
  activeDisasm = mainDisasmTable[0];
  DefDisasmSel = DISASM_DATA;
  for(i=0;i<sizeof(mainDisasmTable)/sizeof(REGISTRY_DISASM *);i++) {
    if(mainDisasmTable[i]->type == def_platform) {
	activeDisasm=mainDisasmTable[i];
	DefDisasmSel = def_platform;
	break;
    }
  }
  disAcceptActions();
  if(!initCodeGuider()) goto err;
}

static void __FASTCALL__ disTerm( void )
{
  termCodeGuider();
  if(activeDisasm->term) activeDisasm->term();
  PFREE(CurrStrLenBuff);
  PFREE(PrevStrLenAddr);
  PFREE(dis_comments);
  PFREE(disCodeBuffer);
  PFREE(disCodeBuf2);
  PFREE(disCodeBufPredict);
}

static void __FASTCALL__ disSaveIni( hIniProfile *ini )
{
  char tmps[10];
  sprintf(tmps,"%i",DefDisasmSel);
  biewWriteProfileString(ini,"Biew","Browser","LastSubMode",tmps);
  WriteIniAResolv(ini);
  sprintf(tmps,"%i",disPanelMode);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode7",tmps);
  sprintf(tmps,"%i",disNeedRef);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode8",tmps);
  sprintf(tmps,"%i",HiLight);
  biewWriteProfileString(ini,"Biew","Browser","SubSubMode9",tmps);
  if(activeDisasm->save_ini) activeDisasm->save_ini(ini);
}

DisasmRet Disassembler(__filesize_t ulShift,MBuffer buffer,unsigned flags)
{
  dis_comments[0] = 0;
  dis_severity = DISCOMSEV_NONE;
  return activeDisasm->disasm(ulShift,buffer,flags);
}

static unsigned __FASTCALL__ disCharSize( void ) { return 1; }

static __filesize_t __FASTCALL__ disSearch(TWindow *pwnd, __filesize_t start,
                                            __filesize_t *slen, unsigned flags,
                                            tBool is_continue, tBool *is_found)
{
  DisasmRet dret;
  __filesize_t tsize, retval, flen, dfpos, cfpos, sfpos; /* If search have no result */
  char *disSearchBuff;
  unsigned len, lw, proc, pproc, pmult;
  int cache[UCHAR_MAX+1];
  cfpos = sfpos = BMGetCurrFilePos();
  flen = BMGetFLength();
  retval = FILESIZE_MAX;
  disSearchBuff  = PMalloc(1002+DISCOM_SIZE);
  DumpMode = True;
  if(!disSearchBuff)
  {
     MemOutBox("Disassembler search initialization");
     goto bye;
  }
  cfpos = start;
  tsize = flen;
  pmult = 100;
  if(tsize > FILESIZE_MAX/100) { tsize /= 100; pmult = 1; }
  pproc = proc = 0;
  /* Attempt to balance disassembler output */
  PrepareAsmLines(KE_SUPERKEY, cfpos);
  lw = disPrevLineWidth();
  if(cfpos && cfpos >= lw) cfpos -= lw;
  if(!(is_continue && (flags & SF_REVERSE))) cfpos += disCurrLineWidth();
  /* end of attempt */
  fillBoyerMooreCache(cache, search_buff, search_len, flags & SF_CASESENS);
  while(1)
  {
    proc = (unsigned)((cfpos*pmult)/tsize);
    if(proc != pproc)
    {
      if(!ShowPercentInWnd(pwnd,pproc=proc))  break;
    }
    if(flags & SF_REVERSE)
    {
       PrepareAsmLines(KE_UPARROW, cfpos);
       lw = disPrevLineWidth();
       if(cfpos && lw && cfpos >= lw)
       {
         len = cfpos + disMaxCodeLen < flen ? disMaxCodeLen : (int)(flen - cfpos);
         memset(disCodeBuffer,0,disMaxCodeLen);
         dfpos = cfpos;
         BMReadBufferEx((void *)disCodeBuffer,len,cfpos,BM_SEEK_SET);
         dret = Disassembler(cfpos,(void *)disCodeBuffer,__DISF_NORMAL);
         cfpos -= lw;
       }
       else break;
    }
    else
    {
         len = cfpos + disMaxCodeLen < flen ? disMaxCodeLen : (int)(flen - cfpos);
         memset(disCodeBuffer,0,disMaxCodeLen);
         dfpos = cfpos;
         BMReadBufferEx((void *)disCodeBuffer,len,cfpos,BM_SEEK_SET);
         dret = Disassembler(cfpos,(void *)disCodeBuffer,__DISF_NORMAL);
         cfpos += dret.codelen;
         if(cfpos >= flen) break;
    }
    strcpy(disSearchBuff, dret.str);
    strcat(disSearchBuff, dis_comments);
    if(strFind(disSearchBuff, strlen(disSearchBuff), search_buff, search_len, cache, flags))
    {
      *is_found = True;
      retval = dfpos;
      *slen = dret.codelen;
      break;
    }
  }
  PFREE(disSearchBuff);
  bye:
  BMSeek(sfpos, SEEK_SET);
  DumpMode = False;
  return retval;
}

REGISTRY_MODE disMode =
{
  "~Disassembler",
  { NULL, "Disasm", NULL, NULL, NULL, "AResol", "PanMod", "ResRef", "HiLght", "UsrNam" },
  { NULL, disSelect_Disasm, NULL, NULL, NULL, hexAddressResolution, disSelectPanelMode, disReferenceResolving, disSelectHiLight, udnUserNames },
  disDetect,
  __MF_USECODEGUIDE | __MF_DISASM,
  drawAsm,
  NULL,
  disCharSize,
  disMiscKeyName,
  disEdit,
  disPrevPageSize,
  disCurrPageSize,
  disPrevLineWidth,
  disCurrLineWidth,
  HelpAsm,
  disReadIni,
  disSaveIni,
  disInit,
  disTerm,
  disSearch
};

static void __NEAR__ __FASTCALL__ disAcceptActions( void )
{
  if(activeDisasm->init) activeDisasm->init();
  disMaxCodeLen = activeDisasm->max_insn_len();
  if(disCodeBuffer) PFREE(disCodeBuffer);
  disCodeBuffer = PMalloc(disMaxCodeLen);
  if(disCodeBuf2) PFREE(disCodeBuf2);
  disCodeBuf2 = PMalloc(disMaxCodeLen);
  if(disCodeBufPredict) PFREE(disCodeBufPredict);
  disCodeBufPredict = PMalloc(disMaxCodeLen*PREDICT_DEPTH);
  if(!(disCodeBuffer && disCodeBuf2 && disCodeBufPredict))
  {
    MemOutBox("Disassembler initialization");
    exit(EXIT_FAILURE);
  }
  disMode.prompt[0] = activeDisasm->prompt[0];
  disMode.action[0] = activeDisasm->action[0];
  disMode.prompt[2] = activeDisasm->prompt[1];
  disMode.action[2] = activeDisasm->action[1];
  disMode.prompt[3] = activeDisasm->prompt[2];
  disMode.action[3] = activeDisasm->action[2];
  disMode.prompt[4] = activeDisasm->prompt[3];
  disMode.action[4] = activeDisasm->action[3];
}

/** Common disassembler utility */

char * __FASTCALL__ TabSpace(char * str,unsigned nSpace)
{
  int i,mx;
  unsigned len;
  len = strlen(str);
  mx = max(len,nSpace);
  for(i = len;i < mx;i++) str[i] = TWC_DEF_FILLER;
  if(len >= nSpace) str[i++] = TWC_DEF_FILLER;
  str[i] = 0;
  return str;
}

void  __FASTCALL__ disSetModifier(char *str,const char *modf)
{
  unsigned i,len,mlen;
  i = 0;
  len = strlen(str);
  mlen = strlen(modf);
  while(str[i] != TWC_DEF_FILLER) i++;
  i++;
  memcpy(&str[i],modf,mlen);
  if(i+mlen > len) { str[i+mlen] = TWC_DEF_FILLER; str[i+mlen+1] = 0; }
}

int __FASTCALL__ disAppendDigits(char *str,__filesize_t ulShift,int flags,
                     char codelen,void *defval,unsigned type)
{
 unsigned long app;
 char comments[DISCOM_SIZE];
 const char *appstr;
 unsigned dig_type;
 __filesize_t fpos;
 fpos = bmGetCurrFilePos();
#ifndef NDEBUG
  if(ulShift >= BMGetFLength()-codelen)
  {
     char sout[75];
     static tBool displayed = False;
     if(!displayed)
     {
       strncpy(sout,str,sizeof(sout)-1);
       sout[sizeof(sout)-1] = 0;
       ErrMessageBox(sout," Internal disassembler error detected ");
       displayed = True;
     }
  }
#endif
  if(hexAddressResolv && detectedFormat->AddressResolving) flags |= APREF_SAVE_VIRT;
  app = disNeedRef >= NEEDREF_ALL ? AppendAsmRef(str,ulShift,flags,codelen,0L) :
                                    RAPREF_NONE;
  if(app != RAPREF_DONE)
  {
    dig_type = type & 0x00FFU;
    comments[0] = 0;
    /* @todo Remove dependencies from 4-byte size of operand */
                                         /* Only if immediate operand */
    if(((type & DISARG_IMM) || (type & DISARG_DISP) ||
	(type & DISARG_IDXDISP) || (type & DISARG_RIP)) &&
	disNeedRef >= NEEDREF_PREDICT)    /* Only when reference prediction is on */
    {
      tUInt64 _defval;
      unsigned fld_len=1;
      switch(dig_type)
      {
        default:
        case DISARG_BYTE: _defval = *(tUInt8 *)defval;  fld_len=1; break;
        case DISARG_CHAR: _defval = *(tInt8 *)defval;   fld_len=1; break;
        case DISARG_WORD: _defval = *(tUInt16 *)defval; fld_len=2; break;
        case DISARG_SHORT:_defval = *(tInt16 *)defval;  fld_len=2; break;
        case DISARG_DWORD:_defval = *(tUInt32 *)defval; fld_len=4; break;
        case DISARG_LONG: _defval = *(tInt32 *)defval;  fld_len=4; break;
        case DISARG_QWORD:_defval = *(tUInt64 *)defval; fld_len=8; break;
        case DISARG_LLONG:_defval = *(tInt64 *)defval;  fld_len=8; break;
      }
      if(_defval)         /* Do not perform operation on NULL */
      {
      __filesize_t pa,psym;
      unsigned _class;
      if(type & DISARG_RIP) {
	_defval += (detectedFormat->pa2va ?
		    detectedFormat->pa2va(ulShift) :
		    ulShift)+fld_len;
     }
      if(!app) pa = detectedFormat->va2pa ? detectedFormat->va2pa(_defval) :
                                           _defval;
      else pa = app;
      if(pa)
      {
        /* 1. Try to determine immediate as offset to public symbol */
	if(type & DISARG_RIP) app = AppendAsmRef(str,pa,flags,codelen,0L);
	if(app == RAPREF_DONE) goto next_step;
        if(dis_severity < DISCOMSEV_FUNC)
        {
          strcpy(comments,".*");
          if(detectedFormat->GetPubSym)
          {
            psym = detectedFormat->GetPubSym(&comments[2],sizeof(comments)-2,
                                             &_class,pa,False);
            if(psym != pa) comments[0] = 0;
            else
            {
                dis_severity = DISCOMSEV_FUNC;
                strcpy(dis_comments,comments);
            }
          }
        }
        /* 2. Try to determine immediate as offset to string constant */
        comments[0] = 0;
        if(dis_severity < DISCOMSEV_STRPTR)
        {
          size_t index;
          unsigned char rch;
          index = 0;
          strcat(comments,"->\"");
          for(index = 3;index < sizeof(comments)-5;index++)
          {
            bmSeek(pa+index-3,BM_SEEK_SET);
            rch = bmReadByte();
            if(isprint(rch)) comments[index] = rch;
            else break;
          }
          if(!comments[3]) comments[0] = 0;
          else
          {
            comments[index++] = '"'; comments[index] = 0;
            dis_severity = DISCOMSEV_STRPTR;
            strcpy(dis_comments,comments);
          }
        }
      }
      }
    }
    next_step:
    comments[0] = 0;
    if(app == RAPREF_NONE)
    {
     switch(dig_type)
     {
      case DISARG_LLONG:
#ifdef INT64_C
			 appstr = Get16SignDig(*(tInt64 *)defval);
#else
			 appstr = Get16SignDig(((tInt32 *)defval)[0],((tInt32 *)defval)[1]);
#endif
                         if(type & DISARG_IMM &&
                            disNeedRef >= NEEDREF_PREDICT &&
                            dis_severity < DISCOMSEV_STRING &&
                            isprint(((unsigned char *)defval)[0]) &&
                            isprint(((unsigned char *)defval)[1]) &&
                            isprint(((unsigned char *)defval)[2]) &&
                            isprint(((unsigned char *)defval)[3]) &&
                            isprint(((unsigned char *)defval)[4]) &&
                            isprint(((unsigned char *)defval)[5]) &&
                            isprint(((unsigned char *)defval)[6]) &&
                            isprint(((unsigned char *)defval)[7]))
                            sprintf(comments,"\"%c%c%c%c%c%c%c%c\""
                                            ,((unsigned char *)defval)[0]
                                            ,((unsigned char *)defval)[1]
                                            ,((unsigned char *)defval)[2]
                                            ,((unsigned char *)defval)[3]
                                            ,((unsigned char *)defval)[4]
                                            ,((unsigned char *)defval)[5]
                                            ,((unsigned char *)defval)[6]
                                            ,((unsigned char *)defval)[7]);
                         break;
      case DISARG_LONG:  appstr = Get8SignDig(*(long *)defval);
                         if(type & DISARG_IMM &&
                            disNeedRef >= NEEDREF_PREDICT &&
                            dis_severity < DISCOMSEV_STRING &&
                            isprint(((unsigned char *)defval)[0]) &&
                            isprint(((unsigned char *)defval)[1]) &&
                            isprint(((unsigned char *)defval)[2]) &&
                            isprint(((unsigned char *)defval)[3]))
                            sprintf(comments,"\"%c%c%c%c\""
                                            ,((unsigned char *)defval)[0]
                                            ,((unsigned char *)defval)[1]
                                            ,((unsigned char *)defval)[2]
                                            ,((unsigned char *)defval)[3]);
                         break;
      case DISARG_SHORT: appstr = Get4SignDig(*(short *)defval);
                         if(type & DISARG_IMM &&
                            disNeedRef >= NEEDREF_PREDICT &&
                            dis_severity < DISCOMSEV_STRING &&
                            isprint(((unsigned char *)defval)[0]) &&
                            isprint(((unsigned char *)defval)[1]))
                            sprintf(comments,"\"%c%c\""
                                            ,((unsigned char *)defval)[0]
                                            ,((unsigned char *)defval)[1]);
                         break;
      case DISARG_CHAR:  appstr = Get2SignDig(*(char *)defval);
                         if(type & DISARG_IMM &&
                            disNeedRef >= NEEDREF_PREDICT &&
                            dis_severity < DISCOMSEV_STRING &&
                            isprint(*(unsigned char *)defval))
                            sprintf(comments,"'%c'",*(unsigned char *)defval);
                         break;
      case DISARG_BYTE:  appstr = Get2Digit(*(unsigned char *)defval);
                         if(type & DISARG_IMM &&
                            disNeedRef >= NEEDREF_PREDICT &&
                            dis_severity < DISCOMSEV_STRING &&
                            isprint(*(unsigned char *)defval))
                            sprintf(comments,"'%c'",*(unsigned char *)defval);
                         break;
      case DISARG_WORD:  appstr = Get4Digit(*(unsigned short *)defval);
                         if(type & DISARG_IMM &&
                            disNeedRef >= NEEDREF_PREDICT &&
                            dis_severity < DISCOMSEV_STRING &&
                            isprint(((unsigned char *)defval)[0]) &&
                            isprint(((unsigned char *)defval)[1]))
                            sprintf(comments,"\"%c%c\""
                                            ,((unsigned char *)defval)[0]
                                            ,((unsigned char *)defval)[1]);
                         break;
      default:
      case DISARG_DWORD: appstr = Get8Digit(*(unsigned long *)defval);
                         if(type & DISARG_IMM &&
                            disNeedRef >= NEEDREF_PREDICT &&
                            dis_severity < DISCOMSEV_STRING &&
                            isprint(((unsigned char *)defval)[0]) &&
                            isprint(((unsigned char *)defval)[1]) &&
                            isprint(((unsigned char *)defval)[2]) &&
                            isprint(((unsigned char *)defval)[3]))
                            sprintf(comments,"\"%c%c%c%c\""
                                            ,((unsigned char *)defval)[0]
                                            ,((unsigned char *)defval)[1]
                                            ,((unsigned char *)defval)[2]
                                            ,((unsigned char *)defval)[3]);
                         break;
      case DISARG_QWORD:
#ifdef INT64_C
			 appstr = Get16Digit(*(tUInt64 *)defval);
#else
			 appstr = Get16Digit(((tUInt32 *)defval)[0],((tUInt32 *)defval)[1]);
#endif
                         if(type & DISARG_IMM &&
                            disNeedRef >= NEEDREF_PREDICT &&
                            dis_severity < DISCOMSEV_STRING &&
                            isprint(((unsigned char *)defval)[0]) &&
                            isprint(((unsigned char *)defval)[1]) &&
                            isprint(((unsigned char *)defval)[2]) &&
                            isprint(((unsigned char *)defval)[3]) &&
                            isprint(((unsigned char *)defval)[4]) &&
                            isprint(((unsigned char *)defval)[5]) &&
                            isprint(((unsigned char *)defval)[6]) &&
                            isprint(((unsigned char *)defval)[7]))
                            sprintf(comments,"\"%c%c%c%c%c%c%c%c\""
                                            ,((unsigned char *)defval)[0]
                                            ,((unsigned char *)defval)[1]
                                            ,((unsigned char *)defval)[2]
                                            ,((unsigned char *)defval)[3]
                                            ,((unsigned char *)defval)[4]
                                            ,((unsigned char *)defval)[5]
                                            ,((unsigned char *)defval)[6]
                                            ,((unsigned char *)defval)[7]);
                         break;
    }
    strcat(str,appstr);
   }
   if(comments[0])
   {
     dis_severity = DISCOMSEV_STRING;
     strcpy(dis_comments,comments);
   }
  }
  bmSeek(fpos,BM_SEEK_SET);
  return app;
}

int __FASTCALL__ disAppendFAddr(char * str,__fileoff_t ulShift,__fileoff_t distin,__filesize_t r_sh,char type,unsigned seg,char codelen)
{
 int needref;
 __filesize_t fpos;
 char *modif_to;
 DisasmRet dret;
 int appended = RAPREF_NONE;
 int flags;
 fpos = bmGetCurrFilePos();
 /* Prepare insn type */
 if(disNeedRef > NEEDREF_NONE)
 {
   /* Forward prediction: ulShift = offset of binded field but r_sh is
      pointer where this field is referenced. */
   memset(disCodeBufPredict,0,disMaxCodeLen*PREDICT_DEPTH);
   bmSeek(r_sh, SEEK_SET);
   bmReadBuffer(disCodeBufPredict,disMaxCodeLen*PREDICT_DEPTH);
   dret = Disassembler(r_sh,(MBuffer)disCodeBufPredict,__DISF_GETTYPE);
 }
#ifndef NDEBUG
  if(ulShift >= (__fileoff_t)BMGetFLength()-codelen)
  {
     char sout[75];
     static tBool displayed = False;
     if(!displayed)
     {
       strncpy(sout,str,sizeof(sout)-1);
       sout[sizeof(sout)-1] = 0;
       ErrMessageBox(sout," Internal disassembler error detected ");
       displayed = True;
     }
  }
#endif
 if(disNeedRef > NEEDREF_NONE)
 {
   if(dret.pro_clone == __INSNT_JMPPIC || dret.pro_clone == __INSNT_JMPRIP) goto try_pic; /* skip defaults for PIC */
   flags = APREF_TRY_LABEL;
   if(hexAddressResolv && detectedFormat->AddressResolving) flags |= APREF_SAVE_VIRT;
   if(AppendAsmRef(str,ulShift,flags,codelen,r_sh)) appended = RAPREF_DONE;
   else
   {
      /*
         Forwarding references.
         Dereferencing ret instruction.
         Idea and PE implementation by "Kostya Nosov" <k-nosov@yandex.ru>
      */
       if(dret.pro_clone == __INSNT_JMPVVT) /* jmp (mod r/m) */
       {
            if(AppendAsmRef(str,r_sh+dret.field,APREF_TRY_LABEL,dret.codelen,r_sh))
            {
              appended = RAPREF_DONE;
              modif_to = strchr(str,' ');
              if(modif_to)
              {
                while(*modif_to == ' ') modif_to++;
                *(modif_to-1) = '*';
              }
              if(!DumpMode && !EditMode) GidAddGoAddress(str,r_sh);
            }
       }
       else
       if(dret.pro_clone == __INSNT_JMPPIC) /* jmp [ebx+offset] */
       {
            try_pic:
            if(dret.pro_clone == __INSNT_JMPRIP) goto try_rip;
            if(AppendAsmRef(str,r_sh+dret.field,APREF_TRY_PIC,dret.codelen,r_sh))
            {
              appended = RAPREF_DONE; /* terminate appending any info anyway */
              if(!DumpMode && !EditMode) GidAddGoAddress(str,r_sh);
            }
       }
       else
       if(dret.pro_clone == __INSNT_JMPRIP) /* jmp [rip+offset] */
       {
	__filesize_t _defval,fpos,pa;
	unsigned long app;
		try_rip:
		fpos = BMGetCurrFilePos();
		_defval = dret.codelen==8 ? BMReadQWordEx(r_sh+dret.field,SEEKF_START):
					    BMReadDWordEx(r_sh+dret.field,SEEKF_START);
		BMSeek(fpos,SEEKF_START);
	_defval += (detectedFormat->pa2va ?
		    detectedFormat->pa2va(r_sh+dret.field) :
		    r_sh+dret.field)+dret.codelen;
	pa = detectedFormat->va2pa ? detectedFormat->va2pa(_defval) :
                                           _defval;
	    app=AppendAsmRef(str,pa,APREF_TRY_LABEL,dret.codelen,0L);
            if(app)
            {
              appended = RAPREF_DONE; /* terminate appending any info anyway */
              if(!DumpMode && !EditMode) GidAddGoAddress(str,r_sh);
            }
       }
   }
 }
   /*
      Idea and PE release of "Kostya Nosov" <k-nosov@yandex.ru>:
      make virtual address as argument of "jxxx" and "callx"
   */
 if(!appended)
 {
   if(hexAddressResolv && detectedFormat->AddressResolving)
   {
     r_sh = r_sh ? r_sh : (__filesize_t)ulShift;
     appended = detectedFormat->AddressResolving(&str[strlen(str)],r_sh) ? RAPREF_DONE : RAPREF_NONE;
   }
   if(!appended)
   {
     needref = disNeedRef;
     disNeedRef = NEEDREF_NONE;
     if(r_sh <= BMGetFLength())
     {
       const char * cptr;
       char lbuf[20];
       cptr = DumpMode ? "L" : "file:";
       strcat(str,cptr);
#if (__WORDSIZE >= 32) && !defined(__QNX4__)
	if((BMFileFlags&BMFF_USE64))
	    sprintf(lbuf,"%016llX",r_sh);
	else
#endif
       sprintf(lbuf,"%08lX",(unsigned long)r_sh);
       strcat(str,lbuf);
       appended = RAPREF_DONE;
     }
     else
     {
       const char * pstr = "";
       if(type & DISADR_USESEG)
       {
         strcat(str,Get4Digit(seg));
         strcat(str,":");
       }
       if(!type) pstr = Get2SignDig((char)distin);
       else
         if(type & DISADR_NEAR16)
              pstr = type & DISADR_USESEG ? Get4Digit((unsigned)distin) :
                                            Get4SignDig((unsigned)distin);
         else
          if(type & DISADR_NEAR32)   pstr = Get8SignDig(distin);
	  else
	   if(type & DISADR_NEAR64) pstr = Get16SignDig(distin);
       strcat(str,pstr);
     }
     disNeedRef = needref;
   }
   if(disNeedRef >= NEEDREF_PREDICT && dis_severity < DISCOMSEV_INSNREF)
   {
     const char * comms;
     comms = NULL;
     switch(dret.pro_clone)
     {
        case __INSNT_RET:   comms = "RETURN"; break;
        case __INSNT_LEAVE: comms = "LEAVE"; break;
        default:            break;
     }
     if(comms)
     {
        dis_severity = DISCOMSEV_INSNREF;
        strcpy(dis_comments,comms);
     }
   }
   if(appended && !DumpMode && !EditMode) GidAddGoAddress(str,r_sh);
 }
 bmSeek(fpos,BM_SEEK_SET);
 return appended;
}










