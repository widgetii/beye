/**
 * @namespace   biew
 * @file        fileutil.c
 * @brief       This file contains file utilities of BIEW project.
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
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include "editor.h"
#include "bmfile.h"
#include "tstrings.h"
#include "plugins/hexmode.h"
#include "plugins/disasm.h"
#include "biewutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "biewlib/pmalloc.h"
#include "biewlib/bbio.h"
#include "biewlib/twin.h"
#include "biewlib/kbd_code.h"

extern tBool fioUseMMF;

static tBool ChSize( void )
{
 long psize,tile = 0;
 if(Get8DigitDlg(" Change size of file ","Num. of bytes (+-dec):",3,(unsigned long *)&tile))
 {
  if(tile != 0)
  {
    psize = BMGetFLength();
    psize += tile;
    if(psize > 0)
    {
       tBool ret;
       int my_errno = 0;
       char *fname = BMName();
       BGLOBAL bHandle;
       bHandle = biewOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
       if(bHandle == &bNull)
       {
         err:
         errnoMessageBox(RESIZE_FAIL,NULL,my_errno);
         return False;
       }
       ret = bioChSize(bHandle,psize);
       my_errno = errno;
       bioClose(bHandle);
       if(ret == False) goto err;
       BMReRead();
       return ret;
    }
    else ErrMessageBox("Invalid new length",NULL);
  }
 }
 return False;
}

static tBool __NEAR__ __FASTCALL__ InsBlock(BGLOBAL bHandle,unsigned long start,long psize)
{
   char *buffer;
   unsigned long tile,oflen,flen,crpos,cwpos;
   unsigned numtowrite;
   oflen = bioFLength(bHandle);
   flen = oflen + psize;
   tile = oflen - start;
   buffer = PMalloc(51200U);
   if(!buffer) return 0;
   if(!bioChSize(bHandle,oflen+psize))
   {
     ErrMessageBox(EXPAND_FAIL,NULL);
     PFREE(buffer);
     return False;
   }
   crpos = oflen-min(tile,51200U);
   cwpos = flen-min(tile,51200U);
   numtowrite = (unsigned)min(tile,51200U);
   while(tile)
   {
     bioSeek(bHandle,crpos,BIO_SEEK_SET);
     bioReadBuffer(bHandle,buffer,numtowrite);
     bioSeek(bHandle,cwpos,BIO_SEEK_SET);
     bioWriteBuffer(bHandle,buffer,numtowrite);
     tile -= numtowrite;
     numtowrite = (unsigned)min(tile,51200U);
     crpos -= numtowrite;
     cwpos -= numtowrite;
   }
   tile = oflen - start;
   cwpos = start;
   memset(buffer,0,51200U);
   while(psize)
   {
     numtowrite = (unsigned)min((unsigned long)psize,51200U);
     bioSeek(bHandle,cwpos,BIO_SEEK_SET);
     bioWriteBuffer(bHandle,buffer,numtowrite);
     psize -= numtowrite;
     cwpos += numtowrite;
   }
   PFREE(buffer);
   return True;
}

static tBool __NEAR__ __FASTCALL__ DelBlock(BGLOBAL bHandle,unsigned long start,long psize)
{
   char *buffer;
   unsigned long tile,oflen,crpos,cwpos;
   unsigned numtowrite;
   oflen = bioFLength(bHandle);
   tile = oflen - start;
   buffer = PMalloc(51200U);
   if(!buffer) return False;
   crpos = start-psize; /** psize is negative value */
   cwpos = start;
   while(tile)
   {
     numtowrite = (unsigned)min(tile,51200U);
     bioSeek(bHandle,crpos,BIO_SEEK_SET);
     bioReadBuffer(bHandle,buffer,numtowrite);
     bioSeek(bHandle,cwpos,BIO_SEEK_SET);
     bioWriteBuffer(bHandle,buffer,numtowrite);
     tile -= numtowrite;
     crpos += numtowrite;
     cwpos += numtowrite;
   }
   PFREE(buffer);
   if(!bioChSize(bHandle,oflen+psize))
   {
     ErrMessageBox(TRUNC_FAIL,NULL);
     PFREE(buffer);
   }
   return True;
}

static tBool InsDelBlock( void )
{
 unsigned long start;
 static long psize;
 tBool ret = False;
 start = BMGetCurrFilePos();
 if(GetInsDelBlkDlg(" Insert or delete block to/from file ",&start,&psize))
 {
    unsigned long fpos;
    BGLOBAL bHandle;
    char *fname;
    fpos = BMGetCurrFilePos();
    if(start > BMGetFLength()) { ErrMessageBox("Start is outside of file",NULL); return 0; }
    if(!psize) return 0;
    if(psize < 0) if(start+labs(psize) > BMGetFLength()) { ErrMessageBox("Use change size operation instead of block deletion",NULL); return 0; }
    fname = BMName();
    bHandle = biewOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
    if(bHandle == &bNull)
    {
      errnoMessageBox(OPEN_FAIL,NULL,errno);
    }
    else
    {
      if(psize < 0) ret = DelBlock(bHandle,start,psize);
      else          ret = InsBlock(bHandle,start,psize);
      bioClose(bHandle);
      BMReRead();
    }
    BMSeek(fpos,BM_SEEK_SET);
 }
 return ret;
}

static char ff_fname[FILENAME_MAX+1] = "biew.$$$";
static char xlat_fname[FILENAME_MAX+1];
static unsigned long ff_startpos = 0L,ff_len = 0L;

static void __NEAR__ __FASTCALL__ printObject(FILE *fout,unsigned obj_num,char *oname,int oclass,int obitness,unsigned long size)
{
  const char *name,*btn;
  char onumname[30];
  switch(obitness)
  {
    case DAB_USE16: btn = "USE16"; break;
    case DAB_USE32: btn = "USE32"; break;
    case DAB_USE64: btn = "USE64"; break;
    case DAB_USE128:btn = "USE128"; break;
    case DAB_USE256:btn = "USE256"; break;
    default: btn = "";
  }

  name = oname[0] ? oname : oclass == OC_DATA ? "DUMP_DATA" :
                            oclass == OC_CODE ? "DUMP_TEXT" :
                            "Unknown";
  if(!oname[0]) { sprintf(onumname,"%s%u",name,obj_num); name = onumname; }
  fprintf(fout,"\nSEGMENT %s BYTE PUBLIC %s '%s'\n; size: %lu bytes\n\n"
              ,name
              ,btn
              ,oclass == OC_DATA ? "DATA" : oclass == OC_CODE ? "CODE" : "NoObject"
              ,size);
}

static void __NEAR__ __FASTCALL__ printHdr(FILE * fout,REGISTRY_BIN *fmt)
{
  const char *cptr,*cptr1,*cptr2;
  time_t tim;
  cptr = cptr1 = ";"; cptr2 = "";
  time(&tim);
  fprintf(fout,"%s\n%sDissasembler dump of \'%s\'\n"
               "%sRange : %08lXH-%08lXH\n"
               "%sWritten by "BIEW_VER_MSG"\n"
               "%sDumped : %s\n"
               "%sFormat : %s\n"
               "%s\n\n"
              ,cptr1,cptr,BMName()
              ,cptr,ff_startpos,ff_startpos+ff_len
              ,cptr
              ,cptr,ctime(&tim)
              ,cptr,fmt->name
              ,cptr2);
}

static unsigned __NEAR__ __FASTCALL__ printHelpComment(char *buff,MBuffer codebuff,DisasmRet *dret)
{
  unsigned len,j;
   if(dis_severity > DISCOMSEV_NONE)
   {
     len = 3+strlen(dis_comments);
     strcat(buff,dis_comments);
     strcat(buff," ; ");
   }
   else len = 0;
   for(j = 0;j < dret->codelen;j++)
   {
      memcpy((char *)&buff[len],(char *)Get2Digit(codebuff[j]),2);
      len += 2;
   }
   buff[len] = 0;
   return len;
}

extern REGISTRY_MODE disMode;

#define GET_FUNC_CLASS(x) x == SC_LOCAL ? "private" : "public"

static void __NEAR__ __FASTCALL__ make_addr_column(char *buff,unsigned long offset)
{
   if(hexAddressResolv && detectedFormat->AddressResolving)
   {
     buff[0] = 0;
     detectedFormat->AddressResolving(buff,offset);
   }
   else sprintf(buff,"L%s",Get8Digit(offset));
   strcat(buff,":");
}

static tBool FStore( void )
{
 unsigned long flags;
 char *tmp_buff;
 unsigned long endpos,cpos;
 tmp_buff = PMalloc(0x1000);
 if(!tmp_buff)
 {
       MemOutBox("temporary buffer initialization");
       return False;
 }
 flags = FSDLG_USEMODES | FSDLG_BINMODE | FSDLG_COMMENT;
 DumpMode = True;
 ff_startpos = BMGetCurrFilePos();
 if(!ff_len) ff_len = BMGetFLength() - ff_startpos;
 if(GetFStoreDlg(" Store information to file ",ff_fname,&flags,&ff_startpos,&ff_len,FILE_PRMT))
 {
  endpos = ff_startpos + ff_len;
  endpos = endpos > BMGetFLength() ? BMGetFLength() : endpos;
  if(endpos > ff_startpos)
  {
   TWindow *progress_wnd;
   unsigned prcnt_counter,oprcnt_counter;
   cpos = BMGetCurrFilePos();
   progress_wnd = PercentWnd("Saving ..."," Save block to file ");
   if(!(flags & FSDLG_ASMMODE)) /** Write in binary mode */
   {
     BGLOBAL _bioHandle;
     int handle;
     unsigned long wsize,crpos,pwsize,awsize;
     unsigned rem;
     wsize = endpos - ff_startpos;
     if(__IsFileExists(ff_fname) == False) handle = __OsCreate(ff_fname);
     else
     {
       handle = __OsOpen(ff_fname,FO_READWRITE | SO_DENYNONE);
       if(handle == -1)  handle = __OsOpen(ff_fname,FO_READWRITE | SO_COMPAT);
       if(handle == -1)
       {
          use_err:
          errnoMessageBox("Can't use file",NULL,errno);
          goto Exit;
       }
       __OsTruncFile(handle,0L);
     }
     __OsClose(handle);
     _bioHandle = bioOpen(ff_fname,FO_READWRITE | SO_DENYNONE,BBIO_CACHE_SIZE,fioUseMMF ? BIO_OPT_USEMMF : BIO_OPT_DB);
     if(_bioHandle == &bNull)  _bioHandle = bioOpen(ff_fname,FO_READWRITE | SO_COMPAT,BBIO_CACHE_SIZE,fioUseMMF ? BIO_OPT_USEMMF : BIO_OPT_DB);
     if(_bioHandle == &bNull)  goto use_err;
     crpos = ff_startpos;
     bioSeek(_bioHandle,0L,SEEKF_START);
     prcnt_counter = oprcnt_counter = 0;
     pwsize = 0;
     awsize = wsize;
     while(wsize)
     {
        unsigned real_size;
        rem = (unsigned)min(wsize,4096);
        if(!BMReadBufferEx(tmp_buff,rem,crpos,BM_SEEK_SET))
        {
          errnoMessageBox(READ_FAIL,NULL,errno);
          bioClose(_bioHandle);
          goto Exit;
        }
        real_size = activeMode->convert_cp ? activeMode->convert_cp((char *)tmp_buff,rem,True) : rem;
        if(!bioWriteBuffer(_bioHandle,tmp_buff,real_size))
        {
          errnoMessageBox(WRITE_FAIL,NULL,errno);
          bioClose(_bioHandle);
          goto Exit;
        }
        wsize -= rem;
        crpos += rem;
        pwsize += rem;
        prcnt_counter = (unsigned)((pwsize*100)/awsize);
        if(prcnt_counter != oprcnt_counter)
        {
          oprcnt_counter = prcnt_counter;
          if(!ShowPercentInWnd(progress_wnd,prcnt_counter)) break;
        }
     }
     bioClose(_bioHandle);
   }
   else /** Write in disassembler mode */
   {
     FILE * fout = NULL;
     unsigned char *codebuff;
     char *file_cache = NULL,*tmp_buff2 = NULL;
     unsigned MaxInsnLen;
     char func_name[300],obj_name[300],data_dis[300];
     unsigned long func_pa,stop;
     unsigned func_class;
     unsigned long awsize,pwsize;
     tBool has_string;

     unsigned long obj_start,obj_end;
     int obj_class,obj_bitness;
     unsigned obj_num;

     if(activeMode != &disMode) disMode.init();
     if(flags & FSDLG_STRUCTS)
     {
       if(detectedFormat->set_state) detectedFormat->set_state(PS_ACTIVE);
       if(detectedFormat->prepare_structs)
                    detectedFormat->prepare_structs(ff_startpos,ff_startpos+ff_len);
     }
     MaxInsnLen = activeDisasm->max_insn_len();
     codebuff = PMalloc(MaxInsnLen);
     if(!codebuff)
     {
       MemOutBox("Disasm initialization");
       goto dis_exit;
     }
     tmp_buff2 = PMalloc(0x1000);
     file_cache = PMalloc(BBIO_SMALL_CACHE_SIZE);
     fout = fopen(ff_fname,"wt");
     if(fout == NULL)
     {
        errnoMessageBox(WRITE_FAIL,NULL,errno);
        PFREE(codebuff);
        goto Exit;
     }
     if(file_cache) setvbuf(fout,file_cache,_IOFBF,BBIO_SMALL_CACHE_SIZE);
     if(flags & FSDLG_COMMENT)
     {
       printHdr(fout,detectedFormat);
     }
     if(flags & FSDLG_STRUCTS)
     {
       if(detectedFormat->GetObjAttr)
       {
         obj_num = detectedFormat->GetObjAttr(ff_startpos,obj_name,
                                              sizeof(obj_name),&obj_start,
                                              &obj_end,&obj_class,&obj_bitness);
         obj_name[sizeof(obj_name)-1] = 0;
       }
       else goto defobj;
     }
     else
     {
       defobj:
       obj_num = 0;
       obj_start = 0;
       obj_end = BMGetFLength();
       obj_name[0] = 0;
       obj_class = OC_CODE;
       obj_bitness = detectedFormat->query_bitness ? detectedFormat->query_bitness(ff_startpos) : DAB_USE16;
     }
     if(flags & FSDLG_STRUCTS) printObject(fout,obj_num,obj_name,obj_class,obj_bitness,obj_end - obj_start);
     func_pa = 0;
     if(flags & FSDLG_STRUCTS)
     {
       if(detectedFormat->GetPubSym)
       {
         func_pa = detectedFormat->GetPubSym(func_name,sizeof(func_name),
                                             &func_class,ff_startpos,True);
         func_name[sizeof(func_name)-1] = 0;
         if(func_pa)
         {
            fprintf(fout,"%s %s:\n"
                        ,GET_FUNC_CLASS(func_class)
                        ,func_name);
            if(func_pa < ff_startpos && flags & FSDLG_COMMENT)
            {
              fprintf(fout,"; ...\n");
            }
         }
         func_pa = detectedFormat->GetPubSym(func_name,sizeof(func_name),
                                             &func_class,ff_startpos,False);
         func_name[sizeof(func_name)-1] = 0;
       }
     }
     prcnt_counter = oprcnt_counter = 0;
     awsize = endpos - ff_startpos;
     pwsize = 0;
     has_string = False;
     while(1)
     {
      DisasmRet dret;
      int len;
       if(flags & FSDLG_STRUCTS)
       {
          if(detectedFormat->GetObjAttr)
          {
            if(ff_startpos >= obj_end)
            {
               obj_num = detectedFormat->GetObjAttr(ff_startpos,obj_name,
                                                    sizeof(obj_name),&obj_start,
                                                    &obj_end,&obj_class,
                                                    &obj_bitness);
               obj_name[sizeof(obj_name)-1] = 0;
               printObject(fout,obj_num,obj_name,obj_class,obj_bitness,obj_end - obj_start);

            }
          }
          if(obj_class == OC_NOOBJECT)
          {
            unsigned long diff;
            fprintf(fout,"; L%08lXH-L%08lXH - no object\n",obj_start,obj_end);
            dret.codelen = min(UCHAR_MAX,obj_end - ff_startpos);
            /** some functions can placed in virtual area of objects
                mean at end of true data, but before next object */
             while(func_pa && func_pa >= obj_start && func_pa < obj_end && func_pa > ff_startpos)
             {
                  diff = func_pa - ff_startpos;
                  if(diff) fprintf(fout,"resb %08lXH\n",diff);
                  fprintf(fout,"%s %s: ;at offset - %08lXH\n"
                              ,GET_FUNC_CLASS(func_class)
                              ,func_name
                              ,func_pa);
                  ff_startpos = func_pa;
                  func_pa = detectedFormat->GetPubSym(func_name,sizeof(func_name),
                                                    &func_class,ff_startpos,False);
                  func_name[sizeof(func_name)-1] = 0;
                  if(func_pa == ff_startpos)
                  {
                    fprintf(fout,"...Probably internal error of biew...\n");
                    break;
                  }
              }
              diff = obj_end - ff_startpos;
              if(diff) fprintf(fout,"resb %08lXH\n",diff);
              ff_startpos = obj_end;
              goto next_obj;
          }
          if(detectedFormat->GetPubSym && func_pa)
          {
	    int not_silly;
	    not_silly = 0;
            while(ff_startpos == func_pa)
            {
              /* print out here all public labels */
              fprintf(fout,"%s %s:\n"
                          ,GET_FUNC_CLASS(func_class)
                          ,func_name);
              func_pa = detectedFormat->GetPubSym(func_name,sizeof(func_name),
                                                  &func_class,ff_startpos,False);
              func_name[sizeof(func_name)-1] = 0;
	      not_silly++;
              if(not_silly > 100)
              {
                fprintf(fout,"; [snipped out] ...\n");
                break;
              }
            }
          }
       }
       memset(codebuff,0,sizeof(codebuff));
       BMReadBufferEx((void *)codebuff,MaxInsnLen,ff_startpos,BM_SEEK_SET);
       if(obj_class == OC_CODE)
         dret = Disassembler(ff_startpos,codebuff,__DISF_NORMAL);
       else /** Data object */
       {
         unsigned dis_data_len,ifreq,data_len;
         char coll_str[__TVIO_MAXSCREENWIDTH];
         size_t cstr_idx = 0;
         dis_data_len = min(sizeof(coll_str)-1,MaxInsnLen);
         for(cstr_idx = 0;cstr_idx < dis_data_len;cstr_idx++)
         {
           if(isprint(codebuff[cstr_idx]))
           {
             coll_str[cstr_idx] = codebuff[cstr_idx];
           }
           else break;
         }
         coll_str[cstr_idx] = 0;
         switch(obj_bitness)
         {
           case DAB_USE16: dis_data_len = 2; break;
           case DAB_USE32: dis_data_len = 4; break;
           case DAB_USE64: dis_data_len = 8; break;
           case DAB_USE128: dis_data_len = 16; break;
           case DAB_USE256: dis_data_len = 32; break;
           default:         dis_data_len = 1; break;
         }
         data_len = 0;
         sprintf(data_dis,"db  ");
         if(cstr_idx > 1)
         {
           sprintf(&data_dis[strlen(data_dis)],"'%s'",coll_str);
           dret.codelen = cstr_idx;
           has_string = True;
         }
         else
         {
           for(ifreq = 0;ifreq < dis_data_len;ifreq++)
           {
              if(isprint(codebuff[ifreq]) && isprint(codebuff[ifreq+1]))
                                              break;
              if(isprint(codebuff[ifreq]) && has_string)
                  sprintf(&data_dis[strlen(data_dis)],"'%c',",codebuff[ifreq]);
              else
                sprintf(&data_dis[strlen(data_dis)],"%02Xh,",codebuff[ifreq]);
              data_len++;
              has_string = False;
           }
           dret.codelen = data_len;
         }
         dret.str = data_dis;
         dret.pro_clone = 0;
         dis_severity = DISCOMSEV_NONE;
       }
       stop = func_pa ? min(func_pa,obj_end) : obj_end;
       if(flags & FSDLG_STRUCTS)
       {
         if(detectedFormat->GetPubSym && stop && stop > ff_startpos &&
            ff_startpos + dret.codelen > stop)
         {
           unsigned lim,ii;
           make_addr_column(tmp_buff,ff_startpos);
           strcat(tmp_buff," db ");
           lim = (unsigned)(stop-ff_startpos);
           if(lim > MaxInsnLen) lim = MaxInsnLen;
           for(ii = 0;ii < lim;ii++)
             sprintf(&tmp_buff[strlen(tmp_buff)],"%s ",Get2Digit(codebuff[ii]));
           dret.codelen = lim;
         }
         else goto normline;
       }
       else 
       {
          normline:
          make_addr_column(tmp_buff,ff_startpos);
          sprintf(&tmp_buff[strlen(tmp_buff)]," %s",dret.str); 
       }
       len = strlen(tmp_buff);
       if(flags & FSDLG_COMMENT)
       {
          if(len < 48)
          {
            memset(&tmp_buff[len],' ',48-len);
            len = 48;
            tmp_buff[len] = 0;
          }
          strcat(tmp_buff,"; ");
          len += 2;
          len += printHelpComment(&((char *)tmp_buff)[len],codebuff,&dret);
       }
       if(tmp_buff2)
       {
         szSpace2Tab(tmp_buff2,tmp_buff);
         szTrimTrailingSpace(tmp_buff2);
       }
       strcat(tmp_buff2 ? tmp_buff2 : tmp_buff,"\n");
       if(fputs(tmp_buff2 ? tmp_buff2 : tmp_buff,fout) == EOF)
       {
         errnoMessageBox(WRITE_FAIL,NULL,errno);
         goto dis_exit;
       }
       if(flags & FSDLG_STRUCTS)
       {
         if(detectedFormat->GetPubSym && stop && ff_startpos != stop &&
            ff_startpos + dret.codelen > stop)
                      dret.codelen = stop - ff_startpos;
       }
       if(!dret.codelen)
       {
         ErrMessageBox("Internal fatal error"," Put structures ");
         goto dis_exit;
       }
       ff_startpos += dret.codelen;
       next_obj:
       if(ff_startpos >= endpos) break;
       pwsize += dret.codelen;
       prcnt_counter = (unsigned)((pwsize*100)/awsize);
       if(prcnt_counter != oprcnt_counter)
       {
         oprcnt_counter = prcnt_counter;
         if(!ShowPercentInWnd(progress_wnd,prcnt_counter)) break;
       }
     }
     dis_exit:
     PFREE(codebuff);
     fclose(fout);
     if(file_cache) PFREE(file_cache);
     if(tmp_buff2) PFREE(tmp_buff2);
     if(flags & FSDLG_STRUCTS)
     {
       if(detectedFormat->drop_structs) detectedFormat->drop_structs();
       if(detectedFormat->set_state) detectedFormat->set_state(PS_INACTIVE);
     }
     if(activeMode != &disMode) disMode.term();
   }
   Exit:
   CloseWnd(progress_wnd);
   BMSeek(cpos,BM_SEEK_SET);
  }
  else  ErrMessageBox("Start position > end position!",NULL);
 }
 PFREE(tmp_buff);
 DumpMode = False;
 return False;
}

static tBool FRestore( void )
{
 unsigned long endpos,cpos,flags;
 tBool ret;
 ret = False;
 flags = FSDLG_NOMODES;
 if(GetFStoreDlg(" Restore information from file ",ff_fname,&flags,&ff_startpos,&ff_len,FILE_PRMT))
 {
   unsigned long flen,lval;
   int handle;
   BGLOBAL bHandle;
   char *fname;
   endpos = ff_startpos + ff_len;
   handle = __OsOpen(ff_fname,FO_READONLY | SO_DENYNONE);
   if(handle == -1) handle = __OsOpen(ff_fname,FO_READONLY | SO_COMPAT);
   if(handle == -1) goto err;
   flen = __FileLength(handle);
   __OsClose(handle);
   lval = endpos - ff_startpos;
   endpos = lval > flen ? flen + ff_startpos : endpos;
   endpos = endpos > BMGetFLength() ? BMGetFLength() : endpos;
   if(endpos > ff_startpos)
   {
     unsigned long wsize,cwpos;
     unsigned remaind;
     void *tmp_buff;
     handle = __OsOpen(ff_fname,FO_READONLY | SO_DENYNONE);
     if(handle == -1) handle = __OsOpen(ff_fname,FO_READONLY | SO_COMPAT);
     if(handle == -1)
     {
        err:
        errnoMessageBox(OPEN_FAIL,NULL,errno);
        return False;
     }
     cpos = BMGetCurrFilePos();
     wsize = endpos - ff_startpos;
     cwpos = ff_startpos;
     __OsSeek(handle,0L,SEEKF_START);
     tmp_buff = PMalloc(4096);
     if(!tmp_buff)
     {
       MemOutBox("temporary buffer initialization");
       return False;
     }
     fname = BMName();
     bHandle = biewOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
     if(bHandle != &bNull)
     {
       while(wsize)
       {
         remaind = (unsigned)min(wsize,4096);
         if(__OsRead(handle,tmp_buff,remaind) != remaind)
         {
           errnoMessageBox(READ_FAIL,NULL,errno);
           __OsClose(handle);
           ret = False;
           goto bye;
         }
         bioSeek(bHandle,cwpos,BIO_SEEK_SET);
         if(!bioWriteBuffer(bHandle,tmp_buff,remaind))
         {
           errnoMessageBox(WRITE_FAIL,NULL,errno);
           ret = False;
           goto bye;
         }
         wsize -= remaind;
         cwpos += remaind;
       }
       bye:
       bioClose(bHandle);
       BMReRead();
     }
     else errnoMessageBox(OPEN_FAIL,NULL,errno);
     PFREE(tmp_buff);
     __OsClose(handle);
     BMSeek(cpos,BM_SEEK_SET);
     ret = True;
   }
   else ErrMessageBox("Start position > end position!",NULL);
 }
 return ret;
}

static void __NEAR__ __FASTCALL__ CryptFunc(char * buff,unsigned len,char *pass)
{
  char ch,cxor;
  unsigned i,j;
  unsigned bigkey_idx;
  unsigned passlen;
  char big_key[UCHAR_MAX];
  cxor = 0;
  passlen = strlen(pass);
  memset(big_key,0,sizeof(big_key));
  for(j = 0;j < passlen;j++) cxor += pass[j]+j;
  cxor ^= passlen + len;
  for(j = i = 0;i < UCHAR_MAX;i++,j++)
  {
    if(j > passlen) j = 0;
    bigkey_idx = (pass[j] + i) * cxor;
    if(bigkey_idx > UCHAR_MAX) bigkey_idx = (bigkey_idx & 0xFF) ^ ((bigkey_idx >> 8) & 0xFF);
    big_key[i] = bigkey_idx;
  }
  for(bigkey_idx = j = i = 0;i < len;i++,bigkey_idx++,j++)
  {
   unsigned short xor;
   if(bigkey_idx > UCHAR_MAX)
   {
     /** rotate of big key */
     ch = big_key[0];
     memmove(big_key,&big_key[1],UCHAR_MAX-1);
     big_key[UCHAR_MAX-1] = ch;
     bigkey_idx = 0;
   }
   if(j > passlen) j = 0;
   xor = (big_key[bigkey_idx] + i)*cxor;
   if(xor > UCHAR_MAX) xor = (xor & 0xFF) ^ ((xor >> 8) & 0xFF);
   buff[i] = buff[i] ^ xor;
  }
  /** rotate of pass */
  ch = pass[0];
  memmove(pass,&pass[1],passlen-1);
  pass[passlen-1] = ch;
}

static tBool CryptBlock( void )
{
 unsigned long endpos,cpos,flags;
 char pass[81];
 tBool ret;
 ret = False;
 ff_startpos = BMGetCurrFilePos();
 if(!ff_len) ff_len = BMGetFLength() - ff_startpos;
 pass[0] = 0;
 flags = FSDLG_NOMODES;
 if(GetFStoreDlg(" (De)Crypt block of file ",pass,&flags,&ff_startpos,&ff_len,"Input password (WARNING! password will displayed):"))
 {
   unsigned long flen,lval;
   endpos = ff_startpos + ff_len;
   flen = BMGetFLength();
   lval = endpos - ff_startpos;
   endpos = lval > flen ? flen + ff_startpos : endpos;
   endpos = endpos > BMGetFLength() ? BMGetFLength() : endpos;
   if(!pass[0]) { ErrMessageBox("Password can't be empty",NULL); return False; }
   if(endpos > ff_startpos)
   {
     unsigned long wsize,cwpos;
     unsigned remaind;
     char *fname;
     BGLOBAL bHandle;
     void *tmp_buff;
     cpos = BMGetCurrFilePos();
     wsize = endpos - ff_startpos;
     cwpos = ff_startpos;
     tmp_buff = PMalloc(4096);
     if(!tmp_buff)
     {
       MemOutBox("temporary buffer initialization");
       return False;
     }
     fname = BMName();
     bHandle = biewOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
     if(bHandle != &bNull)
     {
       bioSeek(bHandle,ff_startpos,SEEK_SET);
       while(wsize)
       {
         remaind = (unsigned)min(wsize,4096);
         if(!bioReadBuffer(bHandle,tmp_buff,remaind))
         {
           errnoMessageBox(READ_FAIL,NULL,errno);
           ret = False;
           goto bye;
         }
         CryptFunc(tmp_buff,remaind,pass);
         bioSeek(bHandle,cwpos,BIO_SEEK_SET);
         if(!(bioWriteBuffer(bHandle,tmp_buff,remaind)))
         {
           errnoMessageBox(WRITE_FAIL,NULL,errno);
           ret = False;
           goto bye;
         }
         wsize -= remaind;
         cwpos += remaind;
       }
       bye:
       bioClose(bHandle);
       BMReRead();
     }
     PFREE(tmp_buff);
     BMSeek(cpos,BM_SEEK_SET);
     ret = True;
   }
   else ErrMessageBox("Start position > end position!",NULL);
 }
 return ret;
}

static void __NEAR__ __FASTCALL__ EndianifyBlock(char * buff,unsigned len, int type)
{
  unsigned i, step;
  if(!type) return; /* for now */
  switch(type)
  {
    default:
#ifdef INT64_C
    case 3: step = 8;
            break;
#endif
    case 2: step = 4;
            break;
    case 1:
            step = 2;
            break;
  }
  len /= step;
  len *= step;
  for(i = 0;i < len;i+=step, buff+=step)
  {
    switch(type)
    {
      default:

#ifdef INT64_C
      case 3: *((tUInt64 *)buff) = ByteSwapLL(*((tUInt64 *)buff));
              break;
#endif

      case 2: *((tUInt32 *)buff) = ByteSwapL(*((tUInt32 *)buff));
              break;
      case 1:
              *((tUInt16 *)buff) = ByteSwapS(*((tUInt16 *)buff));
              break;
    }
  }
}

static tBool ReverseBlock( void )
{
 unsigned long endpos,cpos,flags;
 tBool ret;
 ret = False;
 ff_startpos = BMGetCurrFilePos();
 if(!ff_len) ff_len = BMGetFLength() - ff_startpos;
 flags = FSDLG_USEBITNS;
 if(GetFStoreDlg(" Endianify block of file ",NULL,&flags,&ff_startpos,&ff_len,NULL))
 {
   unsigned long flen,lval;
   endpos = ff_startpos + ff_len;
   flen = BMGetFLength();
   lval = endpos - ff_startpos;
   endpos = lval > flen ? flen + ff_startpos : endpos;
   endpos = endpos > BMGetFLength() ? BMGetFLength() : endpos;
   if(endpos > ff_startpos)
   {
     unsigned long wsize,cwpos;
     unsigned remaind;
     char *fname;
     BGLOBAL bHandle;
     void *tmp_buff;
     cpos = BMGetCurrFilePos();
     wsize = endpos - ff_startpos;
     cwpos = ff_startpos;
     tmp_buff = PMalloc(4096);
     if(!tmp_buff)
     {
       MemOutBox("temporary buffer initialization");
       return False;
     }
     fname = BMName();
     bHandle = biewOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
     if(bHandle != &bNull)
     {
       bioSeek(bHandle,ff_startpos,SEEK_SET);
       while(wsize)
       {
         remaind = (unsigned)min(wsize,4096);
         if(!bioReadBuffer(bHandle,tmp_buff,remaind))
         {
           errnoMessageBox(READ_FAIL,NULL,errno);
           ret = False;
           goto bye;
         }
         EndianifyBlock(tmp_buff,remaind, flags & FSDLG_BTNSMASK);
         bioSeek(bHandle,cwpos,BIO_SEEK_SET);
         if(!(bioWriteBuffer(bHandle,tmp_buff,remaind)))
         {
           errnoMessageBox(WRITE_FAIL,NULL,errno);
           ret = False;
           goto bye;
         }
         wsize -= remaind;
         cwpos += remaind;
       }
       bye:
       bioClose(bHandle);
       BMReRead();
     }
     PFREE(tmp_buff);
     BMSeek(cpos,BM_SEEK_SET);
     ret = True;
   }
   else ErrMessageBox("Start position > end position!",NULL);
 }
 return ret;
}

static void __NEAR__ __FASTCALL__ TranslateBlock(char * buff,unsigned len, const unsigned char *xlt)
{
  unsigned i;
  for(i = 0;i < len;i++)
  {
    buff[i] = __Xlat__(xlt, (int)buff[i]);
  }
}


static tBool XLatBlock( void )
{
 unsigned char xlt[256];
 unsigned long endpos,cpos,flags;
 tBool ret;
 ret = False;
 ff_startpos = BMGetCurrFilePos();
 if(!ff_len) ff_len = BMGetFLength() - ff_startpos;
 flags = FSDLG_NOMODES;
 if(!xlat_fname[0])
 {
   strcpy(xlat_fname,__get_rc_dir("biew"));
   strcat(xlat_fname,"xlt");
 }
 if(GetFStoreDlg(" Table Look-up Translation ",xlat_fname,&flags,&ff_startpos,&ff_len,XLAT_PRMT))
 {
   unsigned long flen,lval;
   endpos = ff_startpos + ff_len;
   flen = BMGetFLength();
   lval = endpos - ff_startpos;
   endpos = lval > flen ? flen + ff_startpos : endpos;
   endpos = endpos > BMGetFLength() ? BMGetFLength() : endpos;
   if(endpos > ff_startpos)
   {
     unsigned long wsize,cwpos;
     unsigned remaind;
     char *fname;
     BGLOBAL bHandle, xHandle;
     void *tmp_buff;
     cpos = BMGetCurrFilePos();
     wsize = endpos - ff_startpos;
     cwpos = ff_startpos;
     /* Parse xlat file */
     xHandle = biewOpenRO(xlat_fname,BBIO_SMALL_CACHE_SIZE);
     if(xHandle == &bNull)
     {
       ErrMessageBox("Can't open xlat file", NULL);
       return False;
     }
     if(bioFLength(xHandle) != 320)
     {
       ErrMessageBox("Size of xlat file is not 320 bytes", NULL);
       bioClose(xHandle);
       return False;
     }
     bioReadBuffer(xHandle,xlt, 16);
     if(memcmp(xlt, "Biew Xlat Table.", 16) != 0)
     {
       ErrMessageBox("It seems that xlat file is corrupt", NULL);
       bioClose(xHandle);
       return False;
     }
     bioSeek(xHandle, 0x40, SEEKF_START);
     bioReadBuffer(xHandle, xlt, 256);
     bioClose(xHandle);
     tmp_buff = PMalloc(4096);
     if(!tmp_buff)
     {
       MemOutBox("temporary buffer initialization");
       return False;
     }
     fname = BMName();
     bHandle = biewOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
     if(bHandle != &bNull)
     {
       bioSeek(bHandle,ff_startpos,SEEK_SET);
       while(wsize)
       {
         remaind = (unsigned)min(wsize,4096);
         if(!bioReadBuffer(bHandle,tmp_buff,remaind))
         {
           errnoMessageBox(READ_FAIL,NULL,errno);
           ret = False;
           goto bye;
         }
         TranslateBlock(tmp_buff,remaind, xlt);
         bioSeek(bHandle,cwpos,BIO_SEEK_SET);
         if(!(bioWriteBuffer(bHandle,tmp_buff,remaind)))
         {
           errnoMessageBox(WRITE_FAIL,NULL,errno);
           ret = False;
           goto bye;
         }
         wsize -= remaind;
         cwpos += remaind;
       }
       bye:
       bioClose(bHandle);
       BMReRead();
     }
     PFREE(tmp_buff);
     BMSeek(cpos,BM_SEEK_SET);
     ret = True;
   }
   else ErrMessageBox("Start position > end position!",NULL);
 }
 return ret;
}

extern char shortname[];

static tBool FileInfo( void )
{
  TWindow* wnd;
  struct stat statbuf;
  unsigned evt;
  char attr[14];
  char stimes[3][80];
  memset(&statbuf,0,sizeof(struct stat));
  stat(BMName(),&statbuf);
  memset(attr,'-',sizeof(attr));
  attr[sizeof(attr)-1] = 0;
#ifdef S_IXOTH /** Execute by other */
  if((statbuf.st_mode & S_IXOTH) == S_IXOTH) attr[12] = 'X';
#endif
#ifdef S_IWOTH /** Write by other */
  if((statbuf.st_mode & S_IWOTH) == S_IWOTH) attr[11] = 'W';
#endif
#ifdef S_IROTH /** Read by other */
  if((statbuf.st_mode & S_IROTH) == S_IROTH) attr[10] = 'R';
#endif
#ifdef S_IXGRP /** Execute by group */
  if((statbuf.st_mode & S_IXGRP) == S_IXGRP) attr[9] = 'X';
#endif
#ifdef S_IWGRP /** Write by group */
  if((statbuf.st_mode & S_IWGRP) == S_IWGRP) attr[8] = 'W';
#endif
#ifdef S_IRGRP /** Read by group */
  if((statbuf.st_mode & S_IRGRP) == S_IRGRP) attr[7] = 'R';
#endif
#ifdef S_IEXEC /** Execute by owner */
  if((statbuf.st_mode & S_IEXEC) == S_IEXEC) attr[6] = 'X';
#endif
#ifdef S_IWRITE /** Write by owner */
  if((statbuf.st_mode & S_IWRITE) == S_IWRITE) attr[5] = 'W';
#endif
#ifdef S_IREAD /** Read by owner */
  if((statbuf.st_mode & S_IREAD) == S_IREAD) attr[4] = 'R';
#endif
#ifdef S_ISVTX /** Save swapped text after use (obsolete) */
  if((statbuf.st_mode & S_ISVTX) == S_ISVTX) attr[3] = 'V';
#endif
#ifdef S_ISGID /** Set GID on execution */
  if((statbuf.st_mode & S_ISGID) == S_ISGID) attr[2] = 'G';
#endif
#ifdef S_ISUID /** Set UID on execution */
  if((statbuf.st_mode & S_ISUID) == S_ISUID) attr[1] = 'U';
#endif
#ifdef S_ISFIFO /** it FIFO */
  if(S_ISFIFO(statbuf.st_mode)) attr[0] = 'F';
#endif
#ifdef S_IFCHR /** it character device */
  if((statbuf.st_mode & S_IFCHR) == S_IFCHR) attr[0] = 'C';
#endif
#ifdef S_IFDIR /** it directory */
  if((statbuf.st_mode & S_IFDIR) == S_IFDIR) attr[0] = 'D';
#endif
#ifdef S_IFBLK /** it block device */
  if((statbuf.st_mode & S_IFBLK) == S_IFBLK) attr[0] = 'B';
#endif
#ifdef S_IFREG /** it regular file (not dir or node) */
  if((statbuf.st_mode & S_IFREG) == S_IFREG) attr[0] = 'f';
#endif
#ifdef S_IFLNK /** it symbolic link */
  if((statbuf.st_mode & S_IFLNK) == S_IFLNK) attr[0] = 'L';
#endif
#ifdef S_IFSOCK /** it socket */
  if((statbuf.st_mode & S_IFSOCK) == S_IFSOCK) attr[0] = 'S';
#endif
  wnd = CrtDlgWndnls(" File information: ",tvioWidth-5,13);
  twGotoXY(1,1);
  strcpy(stimes[0],ctime(&statbuf.st_ctime));
  strcpy(stimes[1],ctime(&statbuf.st_mtime));
  strcpy(stimes[2],ctime(&statbuf.st_atime));
  twPrintF("Name                          = %s\n"
           "Type                          = %s\n"
           "Length                        = %lu bytes\n"
           "Attributes                    = %s\n"
           "                                TUGVOwnGrpOth\n"
           "Creation time                 = %s"
           "Modification time             = %s"
           "Last access time              = %s"
           "Device containing file        = %u\n"
           "File serial (inode) number    = %u\n"
           "Number of hard links to file  = %u\n"
           "User ID of the file owner     = %u\n"
           "Group ID of the file owner    = %u"
           ,shortname
           ,detectedFormat->name ? detectedFormat->name : "Not detected"
           ,BMGetFLength()
           ,attr
           ,stimes[0]
           ,stimes[1]
           ,stimes[2]
           ,(unsigned)statbuf.st_dev
           ,(unsigned)statbuf.st_ino
           ,(unsigned)statbuf.st_nlink
           ,(unsigned)statbuf.st_uid
           ,(unsigned)statbuf.st_gid);
  do
  {
    evt = GetEvent(drawEmptyPrompt,wnd);
  }
  while(!(evt == KE_ESCAPE || evt == KE_F(10)));
  CloseWnd(wnd);
  return False;
}

static const char * fu_names[] =
{
  "~File information...",
  "C~hange size of file",
  "~Save block as...",
  "~Restore block from...",
  "~Insert/delete block...",
  "~Crypt/decrypt block...", 
  "~Endianify block...", 
  "~Xlat block..."
};

typedef tBool (*FileFunc)( void );

static FileFunc fu_funcs[] =
{
  FileInfo,
  ChSize,
  FStore,
  FRestore,
  InsDelBlock,
  CryptBlock, 
  ReverseBlock,
  XLatBlock
};

tBool FileUtils( void )
{
  size_t nUtils;
  int retval;
  tBool ret;
  static unsigned def_sel = 0;
  nUtils = sizeof(fu_names)/sizeof(char *);
  retval = SelBoxA(fu_names,nUtils," File utilities: ",def_sel);
  if(retval != -1)
  {
     TWindow * w;
     w = PleaseWaitWnd();
     ret = (*fu_funcs[retval])();
     CloseWnd(w);
     def_sel = retval;
     return ret;
  }
  return False;
}
