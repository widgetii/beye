/**
 * @namespace   biew_tools
 * @file        tools/lzss/lzss.c
 * @brief       A Data Compression Program.
 * @version     -
 * @remark      Use, distribute, and modify this program freely.
 *              Please send me your improved versions.
 * @note        Requires POSIX compatible development system
 *
 * @author      Haruhiko Okumura
 * @since       1989
 * @author      Nick Kurshev
 * @date        22.10.1999
 * @note        Modified for using with BIEW
**/
/**************************************************************
	LZSS.C -- A Data Compression Program
	(tab = 4 spaces)
***************************************************************
	4/6/1989 Haruhiko Okumura
	Use, distribute, and modify this program freely.
	Please send me your improved versions.
		PC-VAN		SCIENCE
		NIFTY-Serve	PAF01022
		CompuServe	74050,1022
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>

#include "biewlib/bbio.h"
#include "biewlib/pmalloc.h"
#include "biewlib/biewlib.h"
#define INTERACTIVE
#include "lzssutil.c"

char **ArgVector;

int main(int argc, char *argv[])
{
	char  *s;
	int handle;
	int retcode;
	if (argc != 4)
	{
	    printf("'lzss e file1 file2' encodes file1 into file2.\n"
		   "'lzss d file2 file1' decodes file2 into file1.\n");
	    return EXIT_FAILURE;
	}
        ArgVector=argv;
	if ((s = argv[1], s[1] || strpbrk(s, "DEde") == NULL)
  	    || (s = argv[2], (infile  = bioOpen(s,O_RDONLY,0xFFFF,BIO_OPT_DB)) == NULL))
        {
		printf("??? %s\n", s);
		return EXIT_FAILURE;
	}
	__init_sys();
	s = argv[3];
	if(__IsFileExists(s)) if(__OsDelete(s)) { Err: printf("Problem with %s\n",s); return EXIT_FAILURE; }
        handle = __OsCreate(s);
  	__OsClose(handle);
	if((outfile = bioOpen(s,O_RDWR,0x1000,BIO_OPT_DB)) == NULL) goto Err;
	if (toupper(*argv[1]) == 'E') retcode = Encode();
	else                          retcode = Decode(infile,NULL,0L,
      	                                               bioFLength(infile));
      	if(!retcode) fprintf(stderr,"Error allocating memory during operation\n");
	bioClose(infile);
	bioClose(outfile);
        __term_sys();
	return EXIT_SUCCESS;
}

