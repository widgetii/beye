/**
 * @namespace   biew_tools
 * @file        tools/biewhlp/biewhlp.c
 * @brief       This file is maker of BIEW help file.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "biewlib/bbio.h"
#include "biewlib/biewlib.h"
#include "biewlib/file_ini.h"
#include "../../biewhelp.h"

unsigned long items_freq = 0;
char outfname[FILENAME_MAX];
char id_string[80] = BIEW_HELP_VER;
char tmp_buff[0x1000];
char o_buff[0x4000];
char i_cache[0x1000];
char o_cache[0x1000];
BGLOBAL bOutput;

#define BBIO_CACHE_SIZE 0x1000
#define ARCHIVER  "./lzss e"
#define COMPNAME  "temp_fil.tmp"
#define TEMPFNAME "temp_hlp.tmp"

int comp_string( void )
{
  unsigned i,j,slen;
  unsigned char ch;
  slen = strlen(tmp_buff);
  for(j = i = 0;i < slen;i++)
  {
     ch = tmp_buff[i];
     switch(ch)
     {
       case '%':
                {
                  i++;
                  switch(tmp_buff[i])
                  {
                     case '%': ch = '%'; break;
                     case 'B': ch = HLPC_BOLD_ON; break;
                     case 'b': ch = HLPC_BOLD_OFF; break;
                     case 'U': ch = HLPC_UNDERLINE_ON; break;
                     case 'u': ch = HLPC_UNDERLINE_OFF; break;
                     case 'I': ch = HLPC_ITALIC_ON; break;
                     case 'i': ch = HLPC_ITALIC_OFF; break;
                     case 'S': ch = HLPC_STRIKETHROUGH_ON; break;
                     case 's': ch = HLPC_STRIKETHROUGH_OFF; break;
                     case 'R': ch = HLPC_REVERSE_ON; break;
                     case 'r': ch = HLPC_REVERSE_OFF; break;
                     default:  ch = tmp_buff[i]; break;
                  }
                }
                break;
        default:break;
     }
     o_buff[j++] = ch;
  }
  o_buff[j] = 0;
  return 0;
}

void hlpCompile(const char *srcfile)
{
  FILE *in,*out;
  in = fopen(srcfile,"rt");
  if(!in)
  {
    fprintf(stderr,"Can not open: %s\n",srcfile);
    exit(EXIT_FAILURE);
  }
  out = fopen(COMPNAME,"wb");
  if(!out)
  {
    fclose(in);
    fprintf(stderr,"Can not open/create: %s\n",COMPNAME);
    exit(EXIT_FAILURE);
  }
  setvbuf(in,i_cache,_IOFBF,sizeof(i_cache));
  setvbuf(out,o_cache,_IOFBF,sizeof(o_cache));
  while(1)
  {
     if(!fgets(tmp_buff,sizeof(tmp_buff),in))
     {
       if(feof(in)) break;
       fprintf(stderr,"Can not read from: %s\n",srcfile);
       fclose(in);
       fclose(out);
       exit(EXIT_FAILURE);
     }
     comp_string();
     if(fputs(o_buff,out) == EOF)
     {
       fprintf(stderr,"Can not write to: %s\n",COMPNAME);
       fclose(in);
       fclose(out);
       exit(EXIT_FAILURE);
     }
  }
  fclose(in);
  fclose(out);
}

tBool __FASTCALL__ MyCallBack(IniInfo *ini)
{
  if(strcmp(ini->section,"ITEMS") == 0)
  {
     if(strcmp(ini->subsection,"") == 0)
     {
       items_freq++;
     }
  }
  if(strcmp(ini->section,"OPTION") == 0)
  {
     if(strcmp(ini->subsection,"") == 0)
     {
       if(strcmp(ini->item,"output") == 0)
       {
         strncpy(outfname,ini->value,sizeof(outfname));
         outfname[sizeof(outfname)-1] = 0;
       }
       if(strcmp(ini->item,"id") == 0)
       {
         strncpy(id_string,ini->value,sizeof(id_string));
         id_string[sizeof(id_string)-1] = 0;
       }
     }
  }
  return False;
}

tBool __FASTCALL__ MyCallOut(IniInfo *ini)
{
  if(strcmp(ini->section,"ITEMS") == 0)
  {
     if(strcmp(ini->subsection,"") == 0)
     {
        unsigned long litem,fpos;
        unsigned copysize;
        BIEW_HELP_ITEM bhi;
        BGLOBAL bIn;
        int handle;
        fpos = bioTell(bOutput);
        printf("Processing: %s\n",ini->value);
        litem = strtoul(ini->item,NULL,10);
        sprintf(bhi.item_id,"%08lX",litem);
        hlpCompile(ini->value);
        strcpy(tmp_buff,ARCHIVER);
        strcat(tmp_buff," ");
        strcat(tmp_buff,COMPNAME);
        strcat(tmp_buff," ");
        strcat(tmp_buff,TEMPFNAME);
        if(system(tmp_buff))
        {
          perror("Error ocurred while processing");
          exit(EXIT_FAILURE);
        }
        bIn = bioOpen(TEMPFNAME,FO_READONLY | SO_DENYNONE,BBIO_CACHE_SIZE,BIO_OPT_DB);
         if(bIn == &bNull)
         bIn = bioOpen(TEMPFNAME,FO_READONLY | SO_COMPAT,BBIO_CACHE_SIZE,BIO_OPT_DB);
          if(bIn == &bNull)
          {
              fprintf(stderr,"Can not open %s",TEMPFNAME);
              exit(EXIT_FAILURE);
          }
        litem = __FileLength(bioHandle(bIn));
        sprintf(bhi.item_length,"%08lX",litem);
        handle = __OsOpen(COMPNAME,FO_READONLY | SO_DENYNONE);
        if(handle == -1)
        {
              fprintf(stderr,"Can not open %s",ini->value);
              exit(EXIT_FAILURE);
        }
        litem = __FileLength(handle);
        __OsClose(handle);
        sprintf(bhi.item_decomp_size,"%08lX",litem);
        sprintf(bhi.item_off,"%08lX",bioFLength(bOutput));
        bioSeek(bOutput,items_freq*sizeof(BIEW_HELP_ITEM)+strlen(id_string)+1+HLP_SLONG_LEN,SEEK_SET);
        bioWriteBuffer(bOutput,&bhi,sizeof(BIEW_HELP_ITEM));
        bioSeek(bOutput,fpos,SEEK_SET);
        litem = __FileLength(bioHandle(bIn));
        do
        {
           copysize = min(0x1000,litem);
           bioReadBuffer(bIn,tmp_buff,copysize);
           bioWriteBuffer(bOutput,tmp_buff,copysize);
           litem -= copysize;
        } while(litem);
        bioClose(bIn);
        bioFlush(bOutput);
        items_freq++;
     }
  }
  return False;
}

static void my_atexit( void ) { __term_sys(); }
char **ArgVector;

int main( int argc, char *argv[] )
{
  BIEW_HELP_ITEM bhi;
  int handle;
  unsigned long i;
  char sout[HLP_SLONG_LEN];
  if(argc < 2)
  {
     printf("Wrong number of arguments\n\
             Usage: biewhlp project.file\n");
     return -1;
  }
  ArgVector=argv;
  atexit(my_atexit);
  __init_sys();
  printf("Using %s as project\n", argv[1]);
  FiProgress(argv[1],MyCallBack);
  memset(&bhi,0,sizeof(BIEW_HELP_ITEM));
  if(__IsFileExists(outfname)) if(__OsDelete(outfname)) { fprintf(stderr,"Can not delete %s\n",argv[1]); return -1; }
  handle = __OsCreate(outfname);
  __OsClose(handle);
  bOutput = bioOpen(outfname,FO_READWRITE | SO_DENYNONE,BBIO_CACHE_SIZE,BIO_OPT_DB);
  if(bOutput == &bNull)
    bOutput = bioOpen(outfname,FO_READWRITE | SO_COMPAT,BBIO_CACHE_SIZE,BIO_OPT_DB);
    if(bOutput == &bNull)
      bOutput = bioOpen(outfname,FO_READONLY | SO_DENYNONE,BBIO_CACHE_SIZE,BIO_OPT_DB);
      if(bOutput == &bNull)
        bOutput = bioOpen(outfname,FO_READONLY | SO_COMPAT,BBIO_CACHE_SIZE,BIO_OPT_DB);
        if(bOutput == &bNull)
        {
          fprintf(stderr,"Can not work with %s",outfname);
          return -1;
        }
  bioSetOptimization(bOutput,BIO_OPT_RANDOM);
  bioWriteBuffer(bOutput,id_string,strlen(id_string)+1);
  sprintf(sout,"%08lX",items_freq);
  bioWriteBuffer(bOutput,sout,HLP_SLONG_LEN);
  for(i = 0;i < items_freq;i++) bioWriteBuffer(bOutput,&bhi,sizeof(BIEW_HELP_ITEM));
  items_freq = 0;
  FiProgress(argv[1],MyCallOut);
  bioClose(bOutput);
  __OsDelete(TEMPFNAME);
  __OsDelete(COMPNAME);
  printf("Help file looks - ok\n");
  return 0;
}
