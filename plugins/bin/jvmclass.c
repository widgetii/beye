/**
 * @namespace   biew_plugins_auto
 * @file        plugins/bin/jvmclass.c
 * @brief       This file contains implementation of decoder for Java's ClassFile
 *              file format.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       2004
 * @note        Development, fixes and improvements
**/
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "bconsole.h"
#include "biewhelp.h"
#include "biewutil.h"
#include "bin_util.h"
#include "bmfile.h"
#include "colorset.h"
#include "codeguid.h"
#include "reg_form.h"
#include "tstrings.h"
#include "plugins/disasm.h"
#include "biewlib/kbd_code.h"
#include "biewlib/pmalloc.h"

#define CONSTANT_UTF8		1
#define CONSTANT_INTEGER	3
#define CONSTANT_FLOAT		4
#define CONSTANT_LONG		5
#define CONSTANT_DOUBLE		6
#define CONSTANT_CLASS		7
#define CONSTANT_STRING		8
#define CONSTANT_FIELDREF	9
#define CONSTANT_METHODREF	10
#define CONSTANT_INTERFACEMETHODREF 11
#define CONSTANT_NAME_AND_TYPE	12

typedef struct ClassFile_s
{
    tUInt32	magic;
    tUInt16	minor;
    tUInt16	major;
    tUInt16	constant_pool_count;
    /* constant_pool[] */
    tUInt16	access_flags;
    tUInt16	this_class;
    tUInt16	super_class;
    tUInt16	interfaces_count;
/*    u2 interfaces[interfaces_count]; */
    tUInt16	fields_count;
/*    field_info fields[fields_count]; */
    tUInt16	methods_count;
/*    method_info methods[methods_count]; */
    tUInt16	attributes_count;
/*    attribute_info attributes[attributes_count]; */
/* private data: */
    __filesize_t header_length;
    __filesize_t constants_offset;
    __filesize_t interfaces_offset;
    __filesize_t fields_offset;
    __filesize_t methods_offset;
    __filesize_t attributes_offset;
/* meta data: */
    __filesize_t data_offset;
    __filesize_t code_offset;
    __filesize_t attrcode_offset;
}ClassFile_t;

ClassFile_t jvm_header;

static BGLOBAL jvm_cache;
static BGLOBAL pool_cache;

static tBool  __FASTCALL__ jvm_check_fmt( void )
{
  unsigned char id[4];
  bmReadBufferEx(id,sizeof(id),0,BM_SEEK_SET);
  /* Cafe babe !!! */
  return id[0]==0xCA && id[1]==0xFE && id[2]==0xBA && id[3]==0xBE;
}

static unsigned __NEAR__ __FASTCALL__ skip_constant(BGLOBAL handle,unsigned char id)
{
    unsigned add;
    unsigned short sval;
    add=0;
    switch(id)
    {
	default:
	case CONSTANT_STRING:
	case CONSTANT_CLASS: bioSeek(handle,2,BM_SEEK_CUR); break;
	case CONSTANT_INTEGER:
	case CONSTANT_FLOAT:
	case CONSTANT_FIELDREF:
	case CONSTANT_METHODREF:
	case CONSTANT_NAME_AND_TYPE:
	case CONSTANT_INTERFACEMETHODREF: bioSeek(handle,4,BM_SEEK_CUR); break;
	case CONSTANT_LONG:
	case CONSTANT_DOUBLE: bioSeek(handle,8,BM_SEEK_CUR); add=1; break;
	case CONSTANT_UTF8: 
			sval=bioReadWord(handle);
			sval=FMT_WORD(&sval,1);
			bioSeek(handle,sval,BM_SEEK_CUR);
			break;
    }
    return add;
}

static void __NEAR__ __FASTCALL__ skip_constant_pool(BGLOBAL handle,unsigned nitems)
{
    unsigned i;
    for(i=0;i<nitems;i++)
    {
	unsigned char id;
	id=bioReadByte(handle);
	i+=skip_constant(handle,id);
    }
}

static void __NEAR__ __FASTCALL__ get_utf8(BGLOBAL handle,unsigned nidx,char *str,unsigned slen)
{
    unsigned char id;
    bioSeek(handle,jvm_header.constants_offset,BM_SEEK_SET);
    skip_constant_pool(handle,nidx-1);
    id=bioReadByte(handle);
    if(id==CONSTANT_UTF8)
    {
	nidx=bioReadWord(handle);
	nidx=FMT_WORD(&nidx,1);
	nidx=min(nidx,slen-1);
	bioReadBuffer(handle,str,nidx);
	str[nidx]=0;
    }
}

static void __NEAR__ __FASTCALL__ get_name(BGLOBAL handle,char *str,unsigned slen)
{
    unsigned short nidx;
    nidx=bioReadWord(handle);
    nidx=FMT_WORD(&nidx,1);
    get_utf8(handle,nidx,str,slen);
}

static char * __NEAR__ __FASTCALL__ get_class_name(BGLOBAL handle,unsigned idx,char *str,unsigned slen)
{
    *str='\0';
    if(idx && idx<jvm_header.constant_pool_count-1)
    {
	unsigned char id;
	bioSeek(handle,jvm_header.constants_offset,BM_SEEK_SET);
	skip_constant_pool(handle,idx-1);
	id=bioReadByte(handle);
	if(id==CONSTANT_CLASS) get_name(handle,str,slen);
    }
    return str;
}

static void __NEAR__ __FASTCALL__ skip_attributes(BGLOBAL handle,unsigned nitems)
{
    unsigned i;
    for(i=0;i<nitems;i++)
    {
	unsigned long lval;
	bioSeek(handle,2,BM_SEEK_CUR);
	lval=bioReadDWord(handle);
	lval=FMT_DWORD(&lval,1);
	bioSeek(handle,lval,BM_SEEK_CUR);
    }
}

static void __NEAR__ __FASTCALL__ skip_fields(unsigned nitems,int attr)
{
    unsigned i;
    __filesize_t fpos;
    for(i=0;i<nitems;i++)
    {
	unsigned short sval;
	bmSeek(6,BM_SEEK_CUR);
	sval=bmReadWord();
	sval=FMT_WORD(&sval,1);
	fpos=bmGetCurrFilePos();
	if(i==0)
	{
	    __filesize_t lval;
	    lval=sval?fpos+6:fpos;
	    if(attr) jvm_header.code_offset=lval;
	    else jvm_header.data_offset=lval;
	}
	skip_attributes(bmbioHandle(),sval);
    }
}

static tBool __FASTCALL__ jvm_read_interfaces(BGLOBAL handle,memArray * names,unsigned nnames)
{
    unsigned i;
    __filesize_t fpos;
    unsigned short id;
    char str[80];
    bioSeek(handle,jvm_header.interfaces_offset,BM_SEEK_SET);
    for(i=0;i<nnames;i++)
    {
	id=bioReadWord(handle);
	fpos=bioTell(handle);
	id=FMT_WORD(&id,1);
	get_class_name(handle,id,str,sizeof(str));
	if(!ma_AddString(names,str,True)) break;
	bioSeek(handle,fpos,BM_SEEK_SET);
    }
    return True;
}

static unsigned __FASTCALL__ jvm_get_num_interfaces(BGLOBAL handle)
{
    UNUSED(handle);
    return jvm_header.interfaces_count;
}


static __filesize_t __FASTCALL__ ShowInterfaces(void)
{
  __filesize_t fpos;
  fpos = BMGetCurrFilePos();
  fmtShowList(jvm_get_num_interfaces,jvm_read_interfaces,
                    " interfaces ",
                    LB_SORTABLE,NULL);
  return fpos;
}

static tBool __FASTCALL__ jvm_read_attributes(BGLOBAL handle,memArray * names,unsigned nnames)
{
    unsigned i;
    __filesize_t fpos;
    unsigned long len;
    char str[80],sout[100];
    bioSeek(handle,jvm_header.attributes_offset,BM_SEEK_SET);
    for(i=0;i<nnames;i++)
    {
	fpos=bioTell(handle);
	get_name(handle,str,sizeof(str));
	bioSeek(handle,fpos+2,BM_SEEK_SET);
	len=bioReadDWord(handle);
	len=FMT_DWORD(&len,1);
	sprintf(sout,"%08lXH %s",len,str);
	if(!ma_AddString(names,sout,True)) break;
	bioSeek(handle,len,BM_SEEK_CUR);
    }
    return True;
}

static unsigned __FASTCALL__ jvm_get_num_attributes(BGLOBAL handle)
{
    UNUSED(handle);
    return jvm_header.attributes_count;
}


static __filesize_t __NEAR__ __FASTCALL__ __ShowAttributes(const char *title)
{
  __filesize_t fpos;
  int ret;
  fpos = BMGetCurrFilePos();
  ret=fmtShowList(jvm_get_num_attributes,jvm_read_attributes,
                title,
                LB_SELECTIVE,NULL);
  if(ret!=-1)
  {
    unsigned i;
    bmSeek(jvm_header.attributes_offset,BM_SEEK_SET);
    for(i=0;i<ret+1;i++)
    {
	unsigned long len;
	fpos=bmGetCurrFilePos();
	bmSeek(fpos+2,BM_SEEK_SET);
	len=bmReadDWord();
	len=FMT_DWORD(&len,1);
	fpos+=6;
	bmSeek(len,BM_SEEK_CUR);
    }
  }
  return fpos;
}

static __filesize_t __FASTCALL__ ShowAttributes(void)
{
    return __ShowAttributes(" length   attributes ");
}

static tBool __FASTCALL__ jvm_read_methods(BGLOBAL handle,memArray * names,unsigned nnames)
{
    unsigned i;
    __filesize_t fpos;
    unsigned short flg,sval,acount;
    char str[80],str2[80],sout[256];
    bioSeek(handle,jvm_header.methods_offset,BM_SEEK_SET);
    for(i=0;i<nnames;i++)
    {
	fpos=bioTell(handle);
	flg=bioReadWord(handle);
	flg=FMT_WORD(&flg,1);
	get_name(handle,str,sizeof(str));
	bioSeek(handle,fpos+4,BM_SEEK_SET);
	get_name(handle,str2,sizeof(str2));
	bioSeek(handle,fpos+6,BM_SEEK_SET);
	sval=bioReadWord(handle);
	acount=FMT_WORD(&sval,1);
	skip_attributes(handle,acount);
	sprintf(sout,"%04XH %04XH %s %s",acount,flg,str,str2);
	if(!ma_AddString(names,sout,True)) break;
    }
    return True;
}

static unsigned __FASTCALL__ jvm_get_num_methods(BGLOBAL handle)
{
    UNUSED(handle);
    return jvm_header.methods_count;
}

static __filesize_t __FASTCALL__ ShowMethods(void)
{
  __filesize_t fpos;
  int ret;
  fpos = BMGetCurrFilePos();
  ret=fmtShowList(jvm_get_num_methods,jvm_read_methods,
                    " length   attributes ",
                    LB_SELECTIVE,NULL);
  if(ret!=-1)
  {
    char str[80];
    unsigned i;
    unsigned short acount;
    bmSeek(jvm_header.methods_offset,BM_SEEK_SET);
    for(i=0;i<ret+1;i++)
    {
	fpos=bmGetCurrFilePos();
	bmSeek(2,BM_SEEK_CUR);
	get_name(bmbioHandle(),str,sizeof(str));
	bmSeek(fpos+6,BM_SEEK_SET);
	acount=bmReadWord();
	acount=FMT_WORD(&acount,1);
	skip_attributes(bmbioHandle(),acount);
    }
    fpos += 6;
    if(acount>1)
    {
	__filesize_t a_offset;
	unsigned short a_count;
	a_offset = jvm_header.attributes_offset;
	a_count = jvm_header.attributes_count;
	jvm_header.attributes_offset=fpos+2;
	jvm_header.attributes_count=acount;	
	fpos=__ShowAttributes(str);
	jvm_header.attributes_offset=a_offset;
	jvm_header.attributes_count=a_count;
    }
    else fpos += acount?6:0;
  }
  return fpos;
}


static tBool __FASTCALL__ jvm_read_fields(BGLOBAL handle,memArray * names,unsigned nnames)
{
    unsigned i;
    __filesize_t fpos;
    unsigned short flg,sval,acount;
    char str[80],str2[80],sout[256];
    bioSeek(handle,jvm_header.fields_offset,BM_SEEK_SET);
    for(i=0;i<nnames;i++)
    {
	fpos=bioTell(handle);
	flg=bioReadWord(handle);
	flg=FMT_WORD(&flg,1);
	get_name(handle,str,sizeof(str));
	bioSeek(handle,fpos+4,BM_SEEK_SET);
	get_name(handle,str2,sizeof(str2));
	bioSeek(handle,fpos+6,BM_SEEK_SET);
	sval=bioReadWord(handle);
	acount=FMT_WORD(&sval,1);
	skip_attributes(handle,acount);
	sprintf(sout,"%04XH %04XH %s %s",acount,flg,str,str2);
	if(!ma_AddString(names,sout,True)) break;
    }
    return True;
}

static unsigned __FASTCALL__ jvm_get_num_fields(BGLOBAL handle)
{
    UNUSED(handle);
    return jvm_header.fields_count;
}

static __filesize_t __FASTCALL__ ShowFields(void)
{
  __filesize_t fpos;
  int ret;
  fpos = BMGetCurrFilePos();
  ret=fmtShowList(jvm_get_num_fields,jvm_read_fields,
                    " length   attributes ",
                    LB_SELECTIVE,NULL);
  if(ret!=-1)
  {
    char str[80];
    unsigned i;
    unsigned short acount;
    bmSeek(jvm_header.fields_offset,BM_SEEK_SET);
    for(i=0;i<ret+1;i++)
    {
	fpos=bmGetCurrFilePos();
	bmSeek(2,BM_SEEK_CUR);
	get_name(bmbioHandle(),str,sizeof(str));
	bmSeek(fpos+6,BM_SEEK_SET);
	acount=bmReadWord();
	acount=FMT_WORD(&acount,1);
	skip_attributes(bmbioHandle(),acount);
    }
    fpos += 6;
    if(acount>1)
    {
	__filesize_t a_offset;
	unsigned short a_count;
	a_offset = jvm_header.attributes_offset;
	a_count = jvm_header.attributes_count;
	jvm_header.attributes_offset=fpos+2;
	jvm_header.attributes_count=acount;
	fpos=__ShowAttributes(str);
	jvm_header.attributes_offset=a_offset;
	jvm_header.attributes_count=a_count;
    }
    else fpos += acount?6:0;
  }
  return fpos;
}

static tBool __FASTCALL__ jvm_read_pool(BGLOBAL handle,memArray * names,unsigned nnames)
{
    __filesize_t fpos;
    unsigned long lval,lval2;
    unsigned i;
    unsigned short flg,sval,slen;
    unsigned char utag;
    char str[80],str2[80],sout[256];
    bioSeek(handle,jvm_header.constants_offset,BM_SEEK_SET);
    for(i=0;i<nnames;i++)
    {
	fpos=bioTell(handle);
	utag=bioReadByte(handle);
	switch(utag)
	{
	    case CONSTANT_STRING:
	    case CONSTANT_CLASS:
			fpos=bioTell(handle);
			get_name(handle,str,sizeof(str));
			bioSeek(handle,fpos+2,BM_SEEK_SET);
			sprintf(sout,"%s: %s",utag==CONSTANT_CLASS?"Class":"String",str);
			break;
	    case CONSTANT_FIELDREF:
	    case CONSTANT_METHODREF:
	    case CONSTANT_INTERFACEMETHODREF:
			flg=bioReadWord(handle);
			flg=FMT_WORD(&flg,1);
			sval=bioReadWord(handle);
			sval=FMT_WORD(&sval,1);
			sprintf(sout,"%s: class=#%04XH name_type_idx=#%04XH"
			,utag==CONSTANT_FIELDREF?"FieldRef":utag==CONSTANT_METHODREF?"MethodRef":"InterfaceMethodRef"
			,flg,sval);
			break;
	    case CONSTANT_INTEGER:
	    case CONSTANT_FLOAT:
			lval=bioReadDWord(handle);
			lval=FMT_DWORD(&lval,1);
			sprintf(sout,"%s: %08lXH",utag==CONSTANT_INTEGER?"Integer":"Float"
			,lval);
			break;
	    case CONSTANT_LONG:
	    case CONSTANT_DOUBLE:
			lval=bioReadDWord(handle);
			lval=FMT_DWORD(&lval,1);
			lval2=bioReadDWord(handle);
			lval2=FMT_DWORD(&lval2,1);
			sprintf(sout,"%s: hi=%08lXH lo=%08lXH",utag==CONSTANT_LONG?"Long":"Double"
			,lval,lval2);
			i++;
			break;
	    case CONSTANT_NAME_AND_TYPE:
			fpos=bioTell(handle);
			get_name(handle,str,sizeof(str));
			bioSeek(handle,fpos+2,BM_SEEK_SET);
			get_name(handle,str2,sizeof(str2));
			bioSeek(handle,fpos+4,BM_SEEK_SET);
			sprintf(sout,"Name&Type: %s %s",str,str2);
			break;
	    case CONSTANT_UTF8: 
			sval=bioReadWord(handle);
			sval=FMT_WORD(&sval,1);
			slen=min(sizeof(str)-1,sval);
			fpos=bioTell(handle);
			bioReadBuffer(handle,str,slen);
			bioSeek(handle,fpos+sval,BM_SEEK_SET);
			str[slen]='\0';
			sprintf(sout,"UTF8: %s",str);
			break;
	    default:
			sprintf(sout,"Unknown: %u",utag);
			i=nnames;
			break;
	}
	if(!ma_AddString(names,sout,True)) break;
    }
    return True;
}

static unsigned __FASTCALL__ jvm_get_num_pools(BGLOBAL handle)
{
    UNUSED(handle);
    return jvm_header.constant_pool_count;
}

static __filesize_t __FASTCALL__ ShowPool(void)
{
  __filesize_t fpos;
  fpos = BMGetCurrFilePos();
  fmtShowList(jvm_get_num_pools,jvm_read_pool,
                    " Constant pool ",
                    LB_SORTABLE,NULL);
  return fpos;
}

static void __FASTCALL__ jvm_init_fmt( void )
{
    __filesize_t fpos;
    unsigned short sval;
    jvm_header.magic=0xCAFEBABE;
    jvm_header.attrcode_offset=-1;
    jvm_header.code_offset=-1;
    jvm_header.data_offset=-1;
    fpos=bmGetCurrFilePos();
    bmSeek(4,BM_SEEK_SET);
    sval=bmReadWord();
    jvm_header.minor=FMT_WORD(&sval,1);
    sval=bmReadWord();
    jvm_header.major=FMT_WORD(&sval,1);
    sval=bmReadWord();
    jvm_header.constant_pool_count=FMT_WORD(&sval,1);
    jvm_header.constants_offset=bmGetCurrFilePos();
    skip_constant_pool(bmbioHandle(),jvm_header.constant_pool_count-1);
    sval=bmReadWord();
    jvm_header.access_flags=FMT_WORD(&sval,1);
    sval=bmReadWord();
    jvm_header.this_class=FMT_WORD(&sval,1);
    sval=bmReadWord();
    jvm_header.super_class=FMT_WORD(&sval,1);
    sval=bmReadWord();
    jvm_header.interfaces_count=FMT_WORD(&sval,1);
    jvm_header.interfaces_offset=bmGetCurrFilePos();
    bmSeek(jvm_header.interfaces_count*2,BM_SEEK_CUR);
    sval=bmReadWord();
    jvm_header.fields_count=FMT_WORD(&sval,1);
    jvm_header.fields_offset=bmGetCurrFilePos();
    skip_fields(jvm_header.fields_count,0);
    sval=bmReadWord();
    jvm_header.methods_count=FMT_WORD(&sval,1);
    jvm_header.methods_offset=bmGetCurrFilePos();
    skip_fields(jvm_header.fields_count,1); /* methods have the same struct as fields */
    sval=bmReadWord();
    jvm_header.attrcode_offset=0;
    jvm_header.attributes_count=FMT_WORD(&sval,1);
    jvm_header.attributes_offset=bmGetCurrFilePos();
    if(jvm_header.attributes_count) jvm_header.attrcode_offset=jvm_header.attributes_offset;
    skip_attributes(bmbioHandle(),sval);
    jvm_header.header_length=bmGetCurrFilePos();
    bmSeek(fpos,BM_SEEK_SET);
    if((jvm_cache = bioDupEx(bmbioHandle(),BBIO_SMALL_CACHE_SIZE)) == &bNull) jvm_cache = bmbioHandle();
    if((pool_cache = bioDupEx(bmbioHandle(),BBIO_SMALL_CACHE_SIZE)) == &bNull) pool_cache = bmbioHandle();
}

static void __FASTCALL__ jvm_destroy_fmt(void)
{
  if(jvm_cache != &bNull && jvm_cache != bmbioHandle()) bioClose(jvm_cache);
  if(pool_cache != &bNull && pool_cache != bmbioHandle()) bioClose(pool_cache);
}

static int  __FASTCALL__ jvm_platform( void) { return DISASM_JAVA; }

static void __NEAR__ __FASTCALL__ decode_acc_flags(unsigned flags, char *str)
{
    if(flags & 0x0001) strcpy(str," PUBLIC");
    if(flags & 0x0010) strcat(str," FINAL");
    if(flags & 0x0020) strcat(str," SUPER");
    if(flags & 0x0200) strcat(str," INTERFACE");
    if(flags & 0x0400) strcat(str," ABSTRACT");
    strcat(str," ");
}

static __filesize_t __FASTCALL__ ShowJvmHeader( void )
{
    __filesize_t entry;
    TWindow * hwnd;
    unsigned keycode;
    char sinfo[70],sinfo2[70],sinfo3[70];
    entry=BMGetCurrFilePos();
    hwnd = CrtDlgWndnls(" ClassFile Header ",78,11);
    twUseWin(hwnd);
    twGotoXY(1,1);
    decode_acc_flags(jvm_header.access_flags,sinfo);
    get_class_name(bmbioHandle(),jvm_header.this_class,sinfo2,sizeof(sinfo2));
    get_class_name(bmbioHandle(),jvm_header.super_class,sinfo3,sizeof(sinfo3));
    twPrintF("Signature     = 'CAFEBABE'\n"
	     "Version       = %u.%u\n"
	     "# Constants   = %u\n"
	     "Access flags  = %04X (%s)\n"
	     "This class    = #%u '%s'\n"
	     "Super class   = #%u '%s'\n"
	     "# interfaces  = %u (at %08lXH)\n"
	     "# fields      = %u (at %08lXH)\n"
	     "# methods     = %u (at %08lXH)\n"
	     "# attributes  = %u (at %08lXH)\n"
	     "Header length = %lu\n"
	     ,jvm_header.major,jvm_header.minor
	     ,jvm_header.constant_pool_count-1
	     ,jvm_header.access_flags, sinfo
	     ,jvm_header.this_class,sinfo2
	     ,jvm_header.super_class,sinfo3
	     ,jvm_header.interfaces_count,jvm_header.interfaces_offset
	     ,jvm_header.fields_count,jvm_header.fields_offset
	     ,jvm_header.methods_count,jvm_header.methods_offset
	     ,jvm_header.attributes_count,jvm_header.attributes_offset
	     ,jvm_header.header_length
	     );
    while(1)
    {
	keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
	if(keycode == KE_F(5) || keycode == KE_ENTER) { entry = entry; break; }
	else
	if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
    }
    CloseWnd(hwnd);
    return entry;
}

static __filesize_t __FASTCALL__ jvm_VA2PA(__filesize_t va)
{
  return  va + jvm_header.code_offset;
}

static __filesize_t __FASTCALL__ jvm_PA2VA(__filesize_t pa)
{
  return pa >= jvm_header.code_offset ? pa - jvm_header.code_offset : 0L;
}

static tBool __FASTCALL__ jvm_AddressResolv(char *addr,__filesize_t cfpos)
{
  tBool bret = True;
  if(cfpos >= jvm_header.methods_offset)
  {
    addr[0]='.';
    strcpy(&addr[1],Get8Digit(jvm_PA2VA(cfpos)));
  }
  else
/*
  if(cfpos >= jvm_header.attributes_offset) sprintf(addr,"Attr:%s",Get4Digit(cfpos-jvm_header.attributes_offset));
  else
    if(cfpos >= jvm_header.methods_offset) sprintf(addr,"Code:%s",Get4Digit(cfpos-jvm_header.methods_offset));
    else
*/
	if(cfpos >= jvm_header.fields_offset) sprintf(addr,"Data:%s",Get4Digit(cfpos-jvm_header.fields_offset));
	else
	    if(cfpos >= jvm_header.interfaces_offset) sprintf(addr,"Imp :%s",Get4Digit(cfpos-jvm_header.interfaces_offset));
	    else
		if(cfpos >= jvm_header.constants_offset) sprintf(addr,"Pool:%s",Get4Digit(cfpos-jvm_header.constants_offset));
		else sprintf(addr,"Hdr :%s",Get4Digit(cfpos));
  return bret;
}

static void __FASTCALL__ jvm_ReadPubName(BGLOBAL b_cache,const struct PubName *it,
                            char *buff,unsigned cb_buff)
{
    bioSeek(b_cache,it->nameoff,BM_SEEK_SET);
    get_name(b_cache,buff,cb_buff);
    if(it->addinfo)
    {
	char *s_end;
	strcat(buff,".");
	s_end=buff+strlen(buff);
	bioSeek(b_cache,it->addinfo,SEEK_SET);
	get_name(b_cache,s_end,cb_buff-(s_end-buff));
    }
}

static void __FASTCALL__ jvm_ReadPubNameList(BGLOBAL handle,void (__FASTCALL__ *mem_out)(const char *))
{
 __filesize_t fpos,len;
 unsigned i;
 struct PubName jvm_pn;
 unsigned short acount,flg;
 if(!PubNames)
   if(!(PubNames = la_Build(0,sizeof(struct PubName),mem_out))) return;
/* Lookup fields */
 bioSeek(handle,jvm_header.fields_offset,BM_SEEK_SET);
 for(i = 0;i < jvm_header.fields_count;i++)
 {
    fpos=bioTell(handle);
    flg=bioReadWord(handle);
    flg=FMT_WORD(&flg,1);
    jvm_pn.nameoff = bioTell(handle);
    jvm_pn.addinfo=0;
    bioSeek(handle,fpos+6,BM_SEEK_SET);
    acount=bioReadWord(handle);
    acount=FMT_WORD(&acount,1);
    for(i=0;i<acount;i++)
    {
	fpos=bioTell(handle);
	jvm_pn.addinfo=fpos;
	bioSeek(handle,fpos+2,BM_SEEK_SET);
	len=bioReadDWord(handle);
	len=FMT_DWORD(&len,1);
	jvm_pn.pa = bioTell(handle);
	jvm_pn.attr    = flg & 0x0008 ? SC_LOCAL : SC_GLOBAL;
	if(!la_AddData(PubNames,&jvm_pn,mem_out)) break;
	bioSeek(handle,len,BM_SEEK_CUR);
	if(bioEOF(handle)) break;
    }
    if(!acount)
    {
	jvm_pn.pa      = bioTell(handle);
	jvm_pn.attr    = flg & 0x0008 ? SC_LOCAL : SC_GLOBAL;
	if(!la_AddData(PubNames,&jvm_pn,mem_out)) break;
    }
    if(bioEOF(handle)) break;
 }
/* Lookup methods */
 bioSeek(handle,jvm_header.methods_offset,BM_SEEK_SET);
 for(i=0;i<jvm_header.fields_count;i++)
 {
    fpos=bioTell(handle);
    flg=bioReadWord(handle);
    flg=FMT_WORD(&flg,1);
    jvm_pn.nameoff = bioTell(handle);
    bioSeek(handle,fpos+6,BM_SEEK_SET);
    acount=bioReadWord(handle);
    acount=FMT_WORD(&acount,1);
    for(i=0;i<acount;i++)
    {
	fpos=bioTell(handle);
	jvm_pn.addinfo=fpos;
	bioSeek(handle,fpos+2,BM_SEEK_SET);
	len=bioReadDWord(handle);
	len=FMT_DWORD(&len,1);
	jvm_pn.pa = bioTell(handle);
	jvm_pn.attr    = flg & 0x0008 ? SC_LOCAL : SC_GLOBAL;
	if(!la_AddData(PubNames,&jvm_pn,mem_out)) break;
	bioSeek(handle,len,BM_SEEK_CUR);
	if(bioEOF(handle)) break;
    }
    if(!acount)
    {
	jvm_pn.pa      = bioTell(handle);
	jvm_pn.attr    = flg & 0x0008 ? SC_LOCAL : SC_GLOBAL;
	if(!la_AddData(PubNames,&jvm_pn,mem_out)) break;
    }
    if(bioEOF(handle)) break;
 }
 if(PubNames->nItems) la_Sort(PubNames,fmtComparePubNames);
}

static __filesize_t __FASTCALL__ jvm_GetPubSym(char *str,unsigned cb_str,unsigned *func_class,
                           __filesize_t pa,tBool as_prev)
{
  return fmtGetPubSym(bmbioHandle(),str,cb_str,func_class,pa,as_prev,
                      jvm_ReadPubNameList,
                      jvm_ReadPubName);
}

static unsigned __FASTCALL__ jvm_GetObjAttr(__filesize_t pa,char *name,unsigned cb_name,
                      __filesize_t *start,__filesize_t *end,int *_class,int *bitness)
{
  unsigned ret;
  UNUSED(cb_name);
  *start = 0;
  *end = bmGetFLength();
  *_class = OC_NOOBJECT;
  *bitness = DAB_USE16;
  name[0] = 0;
  if(pa < jvm_header.data_offset)
  {
    *end =jvm_header.data_offset;
    ret = 0;
  }
  else
    if(pa >= jvm_header.data_offset && pa < jvm_header.methods_offset)
    {
      *_class = OC_DATA;
      *start = jvm_header.data_offset;
      *end = jvm_header.methods_offset;
      ret = 1;
    }
    else
    if(pa >= jvm_header.methods_offset && pa < jvm_header.code_offset)
    {
      *_class = OC_NOOBJECT;
      *start = jvm_header.methods_offset;
      *end = jvm_header.code_offset;
      ret = 2;
    }
    else
    if(pa >= jvm_header.code_offset && pa < jvm_header.attributes_offset)
    {
      *_class = OC_CODE;
      *start = jvm_header.code_offset;
      *end = jvm_header.attributes_offset;
      ret = 3;
    }
    else
    if(pa >= jvm_header.attributes_offset && pa < jvm_header.attrcode_offset)
    {
      *_class = OC_NOOBJECT;
      *start = jvm_header.attributes_offset;
      *end = jvm_header.attrcode_offset;
      ret = 4;
    }
    else
    {
      __filesize_t fpos;
      unsigned long len;
      fpos=bmGetCurrFilePos();
      bmSeek(jvm_header.attributes_offset,BM_SEEK_SET);
      get_name(bmbioHandle(),name,cb_name);
      bmSeek(fpos+2,BM_SEEK_SET);
      len=bmReadDWord();
      len=FMT_DWORD(&len,1);
      bmSeek(fpos,BM_SEEK_SET);
      *_class = OC_CODE;
      *start = jvm_header.attributes_offset+6;
      *end = *start+len;
      ret = 5;
    }
  return ret;
}

static int __FASTCALL__ jvm_bitness(__filesize_t off)
{
    UNUSED(off);
    return DAB_USE16;
}

static unsigned long __FASTCALL__ jvm_AppendRef(char *str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
 unsigned long  retrf = RAPREF_DONE;
 unsigned slen=1000; /* According on disasm/java/java.c */
    if((flags & APREF_TRY_LABEL)!=APREF_TRY_LABEL)
    {
	__filesize_t fpos;
	unsigned long lidx,lval,lval2;
	unsigned sl;
	unsigned short sval,sval2;
	unsigned char utag;
	bioSeek(jvm_cache,ulShift,BM_SEEK_SET);
	switch(codelen)
	{
	    case 4: lidx=bioReadDWord(jvm_cache); lidx=FMT_DWORD(&lidx,1); break;
	    case 2: sval=bioReadWord(jvm_cache); lidx=FMT_WORD(&sval,1); break;
	    default:
	    case 1: lidx=bioReadByte(jvm_cache); break;
	}
	bioSeek(pool_cache,jvm_header.constants_offset,BM_SEEK_SET);
	if(lidx<1 || lidx>jvm_header.constant_pool_count) { retrf = RAPREF_NONE; goto bye; }
	skip_constant_pool(pool_cache,lidx-1);
	utag=bioReadByte(pool_cache);
	str=&str[strlen(str)];
	switch(utag)
	{
	    case CONSTANT_STRING:
	    case CONSTANT_CLASS:
			get_name(pool_cache,str,slen);
			break;
	    case CONSTANT_FIELDREF:
	    case CONSTANT_METHODREF:
	    case CONSTANT_INTERFACEMETHODREF:
			fpos=bioTell(pool_cache);
			sval=bioReadWord(pool_cache);
			sval=FMT_WORD(&sval,1);
			sval2=bioReadWord(pool_cache);
			sval2=FMT_WORD(&sval2,1);
			bioSeek(pool_cache,jvm_header.constants_offset,BM_SEEK_SET);
			get_class_name(pool_cache,sval,str,slen);
			strcat(str,".");
			sl=strlen(str);
			slen-=sl;
			str+=sl;
			bioSeek(pool_cache,jvm_header.constants_offset,BM_SEEK_SET);
			skip_constant_pool(pool_cache,sval2-1);
			utag=bioReadByte(pool_cache);
			if(utag!=CONSTANT_NAME_AND_TYPE) break;
			goto name_type;
	    case CONSTANT_INTEGER:
	    case CONSTANT_FLOAT:
			lval=bioReadDWord(pool_cache);
			lval=FMT_DWORD(&lval,1);
			strcpy(str,utag==CONSTANT_INTEGER?"Integer":"Float");
			strcat(str,":");
			strcat(str,Get8Digit(lval));
			break;
	    case CONSTANT_LONG:
	    case CONSTANT_DOUBLE:
			lval=bioReadDWord(pool_cache);
			lval=FMT_DWORD(&lval,1);
			lval2=bioReadDWord(pool_cache);
			lval2=FMT_DWORD(&lval2,1);
			strcpy(str,utag==CONSTANT_INTEGER?"Long":"Double");
			strcat(str,":");
			strcat(str,Get8Digit(lval));
			strcat(str,Get8Digit(lval2));
			break;
	    case CONSTANT_NAME_AND_TYPE:
	    name_type:
			fpos=bioTell(pool_cache);
			get_name(pool_cache,str,slen);
			bioSeek(pool_cache,fpos+2,BM_SEEK_SET);
			strcat(str," ");
			sl=strlen(str);
			slen-=sl;
			str+=sl;
			get_name(pool_cache,str,slen);
			break;
	    case CONSTANT_UTF8: 
			sval=bioReadWord(pool_cache);
			sval=FMT_WORD(&sval,1);
			sl=min(slen,sval);
			fpos=bioTell(pool_cache);
			bioReadBuffer(pool_cache,str,sl);
			str[sl]='\0';
			break;
	    default:	retrf = RAPREF_NONE;
			break;
	}
    }
    else retrf = RAPREF_NONE;
    bye:
    return retrf;
}

REGISTRY_BIN jvmTable =
{
  "Java's ClassFile",
  { NULL, "Import", "Code  ", "Data  ", NULL, NULL, NULL, "Pool  ", NULL, "Attrib" },
  { NULL, ShowInterfaces, ShowMethods, ShowFields, NULL, NULL, NULL, ShowPool, NULL, ShowAttributes },
  jvm_check_fmt,
  jvm_init_fmt,
  jvm_destroy_fmt,
  ShowJvmHeader,
  jvm_AppendRef,
  fmtSetState,
  jvm_platform,
  jvm_bitness,
  jvm_AddressResolv,
  jvm_VA2PA,
  jvm_PA2VA,
  jvm_GetPubSym,
  jvm_GetObjAttr,
  NULL,
  NULL
};
