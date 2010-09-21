/*
 * Primitive binary blob generator
 *
 * Copyright 2010 konzeptpark Gmbh
 */
 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <errno.h>

#include "libbinblob.h"

#define VMSG( format, args...)  { if (verbose_flag) printf( "#  " format, args); }
#ifdef DEBUG
#define TRACE( format, args...)  { if (debug_flag) printf( "## " format, args); }
#else
#define TRACE( format, args...)
#endif

/*
 * Global binblob handle 
 */
BinaryBlob_s blobhandle;

/*
 * Global variables to track options
 */
static int verbose_flag;  	/* Flag set by ‘--verbose’. */
static int debug_flag;

static char * opt_progname;
static char * opt_script;
static char * opt_blobfile;
static int opt_initblobsize;
static int opt_queryonly;

static void dumphex(BinaryBlob_p bptr)
{
	int i;
	for (i=0; i < bptr->blob_size; i++)
		printf("%2.2x",  bptr->blob_ptr[i]);
}

static void dumpcb64(BinaryBlob_p bptr)
{
	blob_ConvBase64(bptr, NULL, 0);
	printf("%s",  bptr->blob_b64);
}

/*
 * Test stuff - to be removed
 */
#define TESTSIZE 3

static void dump_testvector(BinaryBlob_p bptr)
{
	dumphex (bptr);
	printf( " - " );
	dumpcb64 (bptr);
}

void TestAPI(void)
{
	BinaryBlob_s myblob;
	unsigned int i,j,rc;
	#define CR(x) if (!(rc=(x))) printf("FAIL@%d rc=%d\n", __LINE__,rc)

	printf("Check init functions\n");
	for (j=1; j<=TESTSIZE; j++)
	{
		unsigned char mydata[ TESTSIZE ];
		unsigned char mystring[ blob_CalcB64Size(sizeof(TESTSIZE)) ];
		for (i = 0; i<4; i++)
		{
			CR( blob_InitBlob(&myblob, mydata, j, mystring, blob_CalcB64Size(j)) );
			memset(mydata, 0UL | (i<<6) | (i<<4) | (i<<2) | i, j);
			dump_testvector(&myblob);
		}
	}

	printf("Check alloc, free and resize functions\n");
	for (j=8; j<=8+8*4; j+=4)
	{
		printf("size=%d cbsize=%d :", j, blob_CalcB64Size(j) );
		CR( blob_AllocBlob(&myblob, j) );
		CR( blob_SetUnsigned(&myblob, 0, 17, 0xdead<<1) );
		CR( blob_SetUnsigned(&myblob, 17, 15, 0xc0de) );
		CR( blob_SetUnsigned(&myblob, myblob.blob_bits-32+0, 17, 0xdead<<1) );
		CR( blob_SetUnsigned(&myblob, myblob.blob_bits-32+17, 15, 0xc0de) );
		dump_testvector(&myblob);
		CR( blob_ResizeBlob(&myblob, 4) );
		CR( blob_SetUnsigned(&myblob, myblob.blob_bits-32+0, 17, 0xdead<<1) );
		CR( blob_SetUnsigned(&myblob, myblob.blob_bits-32+17, 15, 0xc0de) );
		dump_testvector(&myblob);

		blob_FreeBlob(&myblob);
	}

	{
		unsigned long long ll;
		unsigned long l;
		unsigned short w;
		unsigned char c;
		signed long long sll;
		signed long sl;
		signed short sw;
		signed char sc;
		char strbuff[12];
		int i;

		printf("Check resize functions\n");
		CR( blob_AllocBlob(&myblob, 4) );

		CR( blob_SetU16(&myblob, 8, 0xabcd) );
		dump_testvector(&myblob);
		CR( blob_ResizeBlob(&myblob, 8) );
		dump_testvector(&myblob);
		CR( blob_ResizeBlob(&myblob, 12) );
		dump_testvector(&myblob);
		CR( blob_ResizeBlob(&myblob, 20) );
		CR( blob_SetU32(&myblob, 10*8, 0xabcdbeef) );
		dump_testvector(&myblob);
		CR( blob_ResizeBlob(&myblob, 12) );
		dump_testvector(&myblob);
		CR( blob_ResizeBlob(&myblob, 8) );
		dump_testvector(&myblob);
		blob_FreeBlob(&myblob);


		printf("Check string functions\n");
		CR( blob_AllocBlob(&myblob, 12) );
		dump_testvector(&myblob);
		for (i=0;i<7;i++) {
			CR( blob_str2blob(&myblob, 0, 12*8, "") );
			CR( blob_str2blob(&myblob, 24+i,64, "01234") );
			dump_testvector(&myblob);
			CR( blob_blob2str(&myblob, 24+i,64, strbuff, sizeof(strbuff)) );
			printf("strcpy @%d:64 -> '%s' (%d)\n", 24+i, strbuff,rc);
		}
		blob_FreeBlob(&myblob);


		printf("Check memcpy functions\n");
		CR( blob_AllocBlob(&myblob, 12) );
		dump_testvector(&myblob);
		for (i=0;i<7;i++) {
			unsigned char binbuff[] = { 1,2,3,4 };
			unsigned char rbbuff[8];
			CR( blob_str2blob(&myblob, 0, 12*8, "") );
			CR( blob_mem2blob(&myblob, 24+i,64, binbuff, sizeof(binbuff)) );
			dump_testvector(&myblob);
			CR( blob_blob2mem(&myblob, 24+i,64, rbbuff, sizeof(rbbuff)) );
			printf("memcpy @%d:64 -> '%016llx' (%d)\n", 24+i, *(unsigned long long*)rbbuff,rc);
		}
		blob_FreeBlob(&myblob);


		printf("Check set functions (32,0,0,0)\n");
		CR( blob_AllocBlob(&myblob, 12) );
		dump_testvector(&myblob);
		CR( blob_SetU64(&myblob, 32, 0x8123456789abcdefULL));
		dump_testvector(&myblob);
		CR( blob_SetU32(&myblob, 0, 0x01234567));
		dump_testvector(&myblob);
		CR( blob_SetU16(&myblob, 0, 0x89ab));
		dump_testvector(&myblob);
		CR( blob_SetU8(&myblob, 0, 0xcd));
		dump_testvector(&myblob);

		printf("Check unsigned get functions (32,0,16,24)\n");
		CR( blob_GetU64(&myblob,32, &ll));
		printf("gU64: %llx %llu\n", ll,ll);
		CR( blob_GetU32(&myblob,0, &l));
		printf("gU32: %lx %lu\n", l,l);
		CR( blob_GetU16(&myblob,16, &w));
		printf("gU16: %x %u\n", w,w);
		CR( blob_GetU8(&myblob,24, &c));
		printf("gU8: %x %u\n", c,c);

		printf("Check signed get functions (32,0,16,24)\n");
		CR( blob_GetS64(&myblob,32, &sll));
		printf("gS64: %llx %lld\n", sll,sll);
		CR( blob_GetS32(&myblob,0, &sl));
		printf("gS32: %lx %ld\n", sl,sl);
		CR( blob_GetS16(&myblob,16, &sw));
		printf("gS16: %x %d\n", sw,sw);
		CR( blob_GetS8(&myblob,24, &sc));
		printf("gS8: %x %d\n", sc,sc);

		printf("Check signed/unsigned handling @8:8\n");
		CR( blob_GetU8(&myblob,8, &c));
		printf("gU8: %x %u\n", c,c);
		CR( blob_GetS8(&myblob,8, &sc));
		printf("gS8: %x %d\n", sc,sc);

		printf("Check base functions @12:6\n");
		CR( blob_GetUnsigned(&myblob,12, 6, &ll));
		printf("gU: %llx %llu\n", ll,ll);
		CR( blob_GetSigned(&myblob,12, 6,&sll));
		printf("gS: %llx %lld\n", sll,sll);

		blob_FreeBlob(&myblob);
	}
	#undef CR
}

/*
 * Main Loop
 */

static void print_usage(void)
{
	printf(
	"Usage: %s <options> [commands]\n"
	"\n"
	"options:\n"
	"	--help		-?	show this help\n"
	"	--script	-s	scriptfile\n"
	"	--file		-f	blobfile\n"
	"	--init	<n>	-i	blobfile\n"
	"	--verbose	-v	verbose\n"
	"	--query		-q	query only, don't change values\n"
	"	--test		-t	internal tests\n"
	"	--debug		-d	debug outputs\n"
	"\n"
	"commands:\n"
	"	[name]@offset:size[SU$%%]\n"
	"	[name]@offset:size[SU$%%]=value\n"
	"	hex\n"
	"	base64\n"
	"	xor8\n"
	"\n"
	"type specifiers:\n"
	"	S,U	-	signed or unsigned (default) value\n"
	"	$	-	cstring data with controlchars\n"
	"	%%	-	binary data (val[,val,...]) (not yet implemented)\n"
	"\n"
	"notes:\n"
	"	- values can be specified in any format supported by strtoll()\n"
	"	- values are assumed to unsigned by default.\n"
	"\n",
		opt_progname
	);
}


char * Substitute(char *valueoutbuff, int valueoutbuffsize, char *value)
{
	int slen,rc;
	char cmdstr[1024], tmpfile[sizeof("/tmp/bb-XXXXXX")], *tp,*tfp;
	memset(cmdstr, 0,sizeof(cmdstr));
	
	if (strstr(value,"$(")==NULL)
		return value;
	valueoutbuff[0]=0;
	while ( (tp = strstr(value,"$("))!=NULL )
	{
		*tp = 0;
		strcat(valueoutbuff, value);
		value = tp+2;
		
		if ( NULL==(tp=strstr(value,")") )) {
			fprintf(stderr, "Underminated shell argument.\n");
			exit (EXIT_FAILURE);
		}
		else *tp = 0;
		
		memcpy(tmpfile,"/tmp/bb-XXXXXX", sizeof(tmpfile));
		tfp = mktemp(tmpfile);

		slen = snprintf(cmdstr,sizeof(cmdstr),"%s > %s", value, tmpfile);
		rc = system(cmdstr);
		if (WIFSIGNALED(rc) &&
		   (WTERMSIG(rc) == SIGINT || WTERMSIG(rc) == SIGQUIT)) {
			perror("Unexpected child error.");
			exit (EXIT_FAILURE);
		}
		if (rc == -1) {
			perror("System returned error .");
			exit (EXIT_FAILURE);
		}
		if ( WEXITSTATUS(rc)== 0 ) {
			FILE *fd;
			fd = fopen(tmpfile,"r");
			if (!fd) {
				perror("Can't open system command output.");
				exit (EXIT_FAILURE);
			}
			slen = strlen(valueoutbuff);
			if (NULL==fgets(valueoutbuff+slen, valueoutbuffsize-slen,fd)) {
				perror("Can't' read system command output.");
				exit (EXIT_FAILURE);
			}
			if ( (tfp=strstr(valueoutbuff+slen,"\n"))!=NULL)
				*tfp=0;
			fclose(fd);
			remove(tmpfile);
		}
		else {
			fprintf(stderr, "External command returned error %d\n", WEXITSTATUS(rc));
			exit(EXIT_FAILURE);
		}
		value = tp+1;
	}
	if ( (tfp=strstr(value,"\n"))!=NULL)
		*tfp=0;
	strcat(valueoutbuff, value);
	TRACE("Substituted '%s'\n", valueoutbuff);
	return valueoutbuff;
}

int ParseCommand(char *arg)
{
	int rc=0, expectassign;
	char *tmp, *tmp2, *locus, *value;

	if ( strncmp(arg,"hex", 3)==0 )
		dumphex (&blobhandle), puts(""),rc=1;
	else if ( strncmp(arg,"base64", 6)==0 )
		dumpcb64 (&blobhandle),rc=1;
	else if ( strncmp(arg,"xor8", 4)==0 ) {
		if ( strncmp(arg,"xor8add", 7)==0 ) {
			TRACE("increased blob from %d to %d bytes.\n", blobhandle.blob_size, blobhandle.blob_size+1 );
			rc = blob_ResizeBlob( &blobhandle, blobhandle.blob_size + 1);
		}
		rc = blob_ChecksumXOR8( &blobhandle);
	}
	else if ( strncmp(arg,"truncate", 8)==0 ) {
		if (!opt_queryonly) {
			int i;
			for (i=blobhandle.blob_size-1;i>=1;i--)
				if (blobhandle.blob_ptr[i]!=0)
					break;
			if (++i+2 < blobhandle.blob_size) {
				TRACE("truncated blob from %d to %d bytes.\n", blobhandle.blob_size, i +2 );
				rc = blob_ResizeBlob(&blobhandle, i + 2);
			}
			else
				TRACE("%s","truncate impossible\n");
		}
		else rc=1;
	}
	else {
		//-- Decompose string into locus and value parts ------
		expectassign = (strstr(arg,"=")!=NULL) ? 1 : 0;
		locus = strtok(arg,"=");
		value = strtok(NULL,"=");
		tmp = strstr(locus,"@" );
		tmp2 = strstr(locus,":" );
		TRACE("locator=%s, value=%s, %d,%d\n", locus, value, tmp?tmp-locus:0, tmp2?tmp2-locus:0);

		if ( locus != NULL && tmp!=NULL && tmp2!=NULL )
		{
			char *lname, *loffp, *lsizep, label[32+1];
			int   loff=0,lsize=0;
			typedef enum valuetype { uint_val, sint_val, string_val, hexarray_val } valuetype_e;
			char valuetypechar[4] = { 'U','S','$','%' };
			valuetype_e valuetype;

			//-- Calc locator parts -----------------------
			*tmp  = 0; lname = locus;
			*tmp2 = 0; loffp = tmp+1;
			lsizep = tmp2+1;
			TRACE("%s@%s:%s\n", lname, loffp, lsizep);

			//-- Calc size and signess --------------------
			errno = 0;
			lsize = strtol(lsizep, &tmp, 0);
			valuetype = uint_val;
			if (errno != 0 && lsize == 0) {
				perror("Invalid locator size");
				exit(-errno);
			}
			TRACE ("strtoll value=%s tmp=%s\n", lsizep, tmp);
			if (lsizep==tmp) {
				lsize = 8;
			}
			else {
				switch (*tmp) {
				default :
					TRACE ("%s","assume valuetype 'U'\n");
				case 'U': valuetype = uint_val; break;
				case 'S': valuetype = sint_val; break;
				case '$': valuetype = string_val; break;
				case '%': valuetype = hexarray_val; break;
				}
			}
			//-- Calc offset ------------------------------
			errno = 0;
			loff = strtol(loffp, &tmp, 0);
			if (errno != 0 && loff == 0) {
				fprintf(stderr, "Invalid locator offset\n");
				exit(EXIT_FAILURE);
			}
			if (loffp==tmp) {
				loff = blobhandle.currentOffset;
			} else {
				if (strstr(tmp,".")) loff = blobhandle.currentOffset;
			}
			//-- get/Create label -------------------------
			if (*lname=='\0') {
				snprintf(label, sizeof(label),"label_%d", loff);
				lname = label;
			}

			TRACE("locator=%s@%d:%d expectedtype=%c\n", lname, loff, lsize, valuetypechar[valuetype]);

			/* Set a value in the binblob */
			if (expectassign && !opt_queryonly)
			{
				if ( value == NULL ) {
					printf("Missing value for locator '%s'.\n", locus);
				}
				else if (strncmp(value,"__BLOBSIZE__",12)==0) {
					rc = blob_SetUnsigned(&blobhandle,loff,lsize, blobhandle.blob_size);
					TRACE("%s@%d:%d <= %d (BLOBSIZE)\n", lname, loff, lsize, blobhandle.blob_size);
				}
				else {
					char valueoutbuff[1024];
					value=Substitute(valueoutbuff, sizeof(valueoutbuff), value);
					if (NULL==value) {
						fprintf(stderr,"Substitute failed.\n");
						exit (EXIT_FAILURE);
					}

					switch (valuetype) {
					case string_val : 
						{
							int slen, i,j;
							char cmdstr[1024];
							slen = strlen(value);
							while (slen>2 && isspace(value[slen-1])) slen--;
							
							if ( value[0]!='\"' && value[slen-1]!='\"' ) {
								fprintf(stderr, "Underminated shell argument\n");
								exit (EXIT_FAILURE);
							}
							else value[slen-1] = 0;

							if ( slen+3 > (int)sizeof(cmdstr) ){
								fprintf(stderr, "Internal buffer overflow (==bug)\n");
								exit (EXIT_FAILURE);
							}
							
							for (i=1,j=0; i<slen; i++) {
								if (value[i]=='\r') 
									continue;
								if (value[i]=='\n')
									cmdstr[j] = '\r', j++;
								if (value[i]=='\\') {
									i++;
									if (value[i]=='n') {
										cmdstr[j++] = '\r';
										cmdstr[j++] = '\n';
	//								} else {
	//									cmdstr[j++] = value[i] - 'A';
									}
									continue;
								}
								cmdstr[j++] = value[i];
								if (value[i]==0)
									break;
							}
							rc = blob_str2blob(&blobhandle, loff, lsize, cmdstr);
						
							VMSG("%s@%d:%d = %s (rc=%d)\n", lname, loff, lsize, cmdstr, rc);
						}
						break;
					case sint_val: 
						{
							signed long long sval;
							sval = strtoll(value, NULL, 0);
							rc = blob_SetSigned(&blobhandle,loff,lsize, sval);
							VMSG("%s@%d:%d%c = %lld (%llx)\n", lname, loff, lsize, valuetypechar[valuetype], sval, sval);
						}
						break;
					case uint_val: 
						{
							unsigned long long val;
							val = strtoll(value, NULL, 0);
							rc = blob_SetUnsigned(&blobhandle,loff,lsize, val);
							VMSG("%s@%d:%d%c = %llu (%llx)\n", lname, loff, lsize, valuetypechar[valuetype], val, val);
						}
						break;
					default :
						fprintf(stderr, "Unhandled valuetype %d\n", valuetype);
						break;
					}
					
				}
				if (!rc) printf("Assignment failed.\n");
			}
			/*
			 * Retrieve value from binblob
			 */
			else {
				char strbuff[ lsize / 8 + 1 ];
				signed long long rvals;
				unsigned long long rval;
				switch (valuetype) {
				case string_val :
					rc = blob_blob2str(&blobhandle,loff,lsize, (char*)strbuff, sizeof(strbuff) );
					VMSG ("%s@%d:%d (%c) => %s (rc=%d)\n", lname, loff, lsize, valuetypechar[valuetype], (char*)strbuff, rc);
					break;
				case sint_val :
					rc = blob_GetSigned(&blobhandle,loff,lsize, &rvals);
					VMSG("%s@%d:%d (%c) => %lld (0x%llx)\n", lname, loff, lsize, valuetypechar[valuetype], rvals, rvals);
					break;
				case uint_val:
					rc = blob_GetUnsigned(&blobhandle,loff,lsize, &rval);
					VMSG("%s@%d:%d (%c) => %llu (0x%llx)\n", lname, loff, lsize, valuetypechar[valuetype], rval, rval);
					break;
				default :
					fprintf(stderr, "Unhandled valuetype %d\n", valuetype);
					break;
				}
				if (!rc) printf("get function for valuetype %d failed.\n", valuetype);
				else switch(valuetype) {
				case string_val :	printf("%s=%s\n", lname?lname:"", strbuff); break;
				case sint_val   :	printf("%s=%lld\n", lname?lname:"", rvals); break;
				case uint_val   :	printf("%s=%llu\n", lname?lname:"", rval); break;
				default :
					exit (EXIT_FAILURE);
					break;
				}
			}
		}
		else if (strncmp(locus,"init",4)==0 && value!=NULL) {
			if (!opt_queryonly) {
				int isize;
				errno = 0;
				isize = strtol(value, NULL, 0);
				if (errno != 0 && isize == 0) {
					fprintf(stderr, "Invalid init size\n");
					exit(EXIT_FAILURE);
				}
				rc = blob_ResizeBlob( &blobhandle, isize );
				if (!rc)
					fprintf(stderr, "Resize failed.\n"), exit(EXIT_FAILURE);
			}
			else rc=1;
		}
		else printf("No locator or command found\n");
	}
	return rc;
}

int main(int argc, char **argv)
{
	int c;
	opt_progname = argv[0];

	while (1)
	{
		static const struct option long_options[] =
		{
			/* These options set a flag. */
			{"verbose", no_argument,	&verbose_flag, 1},
			{"noverbose",   no_argument,	&verbose_flag, 0},

			/* These options don't set a flag.
			 * We distinguish them by their indices. */
			{"help",    no_argument,	0, '?'},
			{"debug",    no_argument,	0, 'd'},
			{"test",    no_argument,	0, 't'},
			{"query",    no_argument,	0, 'q'},

			{"script",  required_argument,	0, 's'},
			{"file",    required_argument,	0, 'f'},
			{"init",  required_argument,	0, 'i'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "?dts:f:i:vq",
				   long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c)
		{
			/* Long option with flag+val found. */
		case 0:
			/* If this option set a flag, do nothing else now. */
//			if (long_options[option_index].flag != 0)
//				break;
			TRACE ("option %s", long_options[option_index].name);
			if (optarg)
				printf (" with arg %s", optarg);
			printf ("\n");
			break;

		case '?':
			print_usage ();
			exit (1);

		case 'q':
			opt_queryonly = 1;
			break;
		case 'd':
			debug_flag = 1;
			break;
		case 't':
			TRACE ("%s","# option -t\n");
			TestAPI();
			break;

		case 'f':
			TRACE ("option -f with value `%s'\n", optarg);
			opt_blobfile = optarg;
			break;
		case 'i':
			TRACE ("option -i with value `%s'\n", optarg);
			opt_initblobsize = strtol(optarg, NULL, 0);
			break;

		case 's':
			TRACE ("option -s with value `%s'\n", optarg);
			opt_script = optarg;
			break;

		case 'v':
			verbose_flag = 1;
			break;

		default:
			abort ();
		}
	}

       /* Instead of reporting ‘--verbose’ and ‘--brief’ as they are
        * encountered, we report the final status resulting from them. */
	if (verbose_flag)
		VMSG ("verbose flag is set to %d\n", verbose_flag);
	if (opt_script)
		VMSG ("scriptfile is %s\n", opt_script);
	if (opt_initblobsize)
		VMSG ("init blobfile to initial size %d\n", opt_initblobsize);
	if (opt_blobfile)
		VMSG ("blobfile is %s\n", opt_blobfile);

	/* Prepare binary blob */

	if (NULL==opt_blobfile) {
		int rc;
retry_alloc:
		if (!opt_initblobsize) {
			TRACE("%s\n", "dbg: Initial size set to default of 1");
			opt_initblobsize = 1;
		}
		rc = blob_AllocBlob (&blobhandle, opt_initblobsize);
		if (!rc) {
			fprintf(stderr, "Can't alloc blob\n");
			exit (-ENOMEM);
		}
	}
	else {
		struct stat filestat;
		FILE *fd;
		int rc;
		if ( (rc = stat (opt_blobfile, &filestat)) ) {
			VMSG("Can't stat blobfile '%s' (%m)\n", opt_blobfile);
			goto retry_alloc;
		}
		if (opt_initblobsize) {
			rc = blob_AllocBlob (&blobhandle, opt_initblobsize);
		}
		else {
			rc = blob_AllocBlob (&blobhandle, filestat.st_size);
		}
		if (!rc) {
			fprintf(stderr, "Can't alloc blob\n");
			exit (-ENOMEM);
		}
		fd = fopen (opt_blobfile, "r");
		if (NULL == fd) {
			fprintf(stderr, "Can't open blobfile '%s' for reading.\n", opt_blobfile);
			exit (EXIT_FAILURE);
		}
		rc = fread (blobhandle.blob_ptr, 1, blobhandle.blob_size, fd);
		if (rc != blobhandle.blob_size) {
			TRACE ("Short read from blobfile '%s' (expected %d, got %d). Zero padded.\n", opt_blobfile, blobhandle.blob_size, rc);
		}
		fclose(fd);
		TRACE("Read %d bytes from blobfile\n", rc);
		opt_initblobsize = blobhandle.blob_size;
	}

	/* Parse script file, if specified */
	if (NULL!=opt_script) {
		FILE *fd;
		int rc;
		fd = fopen (opt_script, "r");
		if (NULL==fd) {
			fprintf (stderr, "Can't open scriptfile (%m).\n");
			exit (EXIT_FAILURE);
		}
		while ( !feof(fd) ) {
			char buffer[260], *wptr, *toktmp, *tmp;
			wptr = fgets( buffer, sizeof(buffer), fd);
			if (wptr!=NULL)
			{
				if ( NULL != (wptr = strstr(buffer,"#")))
					*wptr = 0;

				wptr = strtok_r(buffer,";\n", &toktmp);
				while (wptr!=NULL)
				{
					while (isspace(*wptr))
						wptr++;
					TRACE("scriptline argument: '%s'\n", wptr);
					if (strlen(wptr)>0) {
						tmp = strdup(wptr);
						if (tmp==NULL) {
							fprintf(stderr, "Out of memory.\n");
							exit (-ENOMEM);
						}
						rc = ParseCommand(tmp);
						free(tmp);
						if (!rc) {
							fputs("Aborted command parsing.", stderr);
							exit (EXIT_FAILURE);
						}
					}
					wptr = strtok_r(NULL, ";\n", &toktmp);
				}
			}
		}
		fclose(fd);
	}

	/* Execute command line arguments (not options). */
	if (optind < argc)
        {
		char *aptr;
		int rc;
		TRACE ("processing %d commandline arguments:\n", argc-optind);
		while (optind < argc) {
			aptr = argv[optind];
			TRACE ("cmdline argument : %s\n", aptr);
			rc = ParseCommand(aptr);
			if (!rc) {
				puts("Aborted command parsing.");
				exit (EXIT_FAILURE);
			}
			optind++;
		}
	}


	/* Write out to blobfile, if specified */
	if (NULL!=opt_blobfile) {
		FILE *fd;
		int rc;
		fd = fopen (opt_blobfile, "w");
		if (NULL==fd) {
			perror ("Can't open blobfile for writing.");
			exit (errno);
		}
		rc = fwrite( blobhandle.blob_ptr, 1, blobhandle.blob_size, fd);
		fclose(fd);
		if (rc != blobhandle.blob_size) {
			perror ("Can't write to blobfile.");
			exit (errno);
		}
		TRACE("Wrote %d bytes to blobfile '%s'\n", rc, opt_blobfile);
	}
	blob_FreeBlob ( &blobhandle );
	return 0;
}

