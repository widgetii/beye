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
#include <string.h>

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
    unsigned long header_length;
    unsigned long constants_offset;
    unsigned long interfaces_offset;
    unsigned long fields_offset;
    unsigned long methods_offset;
    unsigned long attributes_offset;
}ClassFile_t;

ClassFile_t jvm_header;

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

static void __NEAR__ __FASTCALL__ skip_fields(unsigned nitems)
{
    unsigned i;
    for(i=0;i<nitems;i++)
    {
	unsigned short sval;
	bmSeek(6,BM_SEEK_CUR);
	sval=bmReadWord();
	sval=FMT_WORD(&sval,1);
	skip_attributes(bmbioHandle(),sval);
    }
}

static tBool __FASTCALL__ jvm_read_interfaces(BGLOBAL handle,memArray * names,unsigned nnames)
{
    unsigned i;
    unsigned long fpos;
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


static unsigned long __FASTCALL__ ShowInterfaces(void)
{
  unsigned long fpos;
  fpos = BMGetCurrFilePos();
  fmtShowList(jvm_get_num_interfaces,jvm_read_interfaces,
                    " interfaces ",
                    LB_SORTABLE,NULL);
  return fpos;
}

static tBool __FASTCALL__ jvm_read_attributes(BGLOBAL handle,memArray * names,unsigned nnames)
{
    unsigned i;
    unsigned long fpos;
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


static unsigned long __NEAR__ __FASTCALL__ __ShowAttributes(const char *title)
{
  unsigned long fpos;
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

static unsigned long __FASTCALL__ ShowAttributes(void)
{
    return __ShowAttributes(" length   attributes ");
}

static tBool __FASTCALL__ jvm_read_methods(BGLOBAL handle,memArray * names,unsigned nnames)
{
    unsigned i;
    unsigned long fpos;
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

static unsigned long __FASTCALL__ ShowMethods(void)
{
  unsigned long fpos;
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
	unsigned long a_offset;
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
    unsigned long fpos;
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

static unsigned long __FASTCALL__ ShowFields(void)
{
  unsigned long fpos;
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
	unsigned long a_offset;
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
    unsigned i;
    unsigned long fpos,lval,lval2;
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

static unsigned long __FASTCALL__ ShowPool(void)
{
  unsigned long fpos;
  fpos = BMGetCurrFilePos();
  fmtShowList(jvm_get_num_pools,jvm_read_pool,
                    " Constant pool ",
                    LB_SORTABLE,NULL);
  return fpos;
}

static void __FASTCALL__ jvm_init_fmt( void )
{
    unsigned long fpos;
    unsigned short sval;
    jvm_header.magic=0xCAFEBABE;
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
    skip_fields(jvm_header.fields_count);
    sval=bmReadWord();
    jvm_header.methods_count=FMT_WORD(&sval,1);
    jvm_header.methods_offset=bmGetCurrFilePos();
    skip_fields(jvm_header.fields_count); /* methods have the same struct as fields */
    sval=bmReadWord();
    jvm_header.attributes_count=FMT_WORD(&sval,1);
    jvm_header.attributes_offset=bmGetCurrFilePos();
    skip_attributes(bmbioHandle(),sval);
    jvm_header.header_length=bmGetCurrFilePos();
    bmSeek(fpos,BM_SEEK_SET);
}
static void __FASTCALL__ jvm_destroy_fmt(void) {}
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

static unsigned long __FASTCALL__ ShowJvmHeader( void )
{
    unsigned long entry;
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
	keycode = GetEvent(drawEmptyPrompt,hwnd);
	if(keycode == KE_F(5) || keycode == KE_ENTER) { entry = entry; break; }
	else
	if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
    }
    CloseWnd(hwnd);
    return entry;
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
  NULL,
  NULL,
  jvm_platform,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
