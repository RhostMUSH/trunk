/* dbconvert.c - Convert databases to various MUSH formats */

#include "copyright.h"
#include "autoconf.h"

#include "config.h"
#include "db.h"
#include "externs.h"
#include "debug.h"

extern void cf_init(void);
extern void do_dbck(dbref, dbref, int);

/* ---------------------------------------------------------------------------
 * info: display info about the file being read or written.
 */

void 
info(int fmt, int flags, int ver)
{
    const char *cp;

    switch (fmt) {
    case F_MUSH:
	cp = "TinyMUSH";
	break;
    case F_MUSE:
	cp = "TinyMUSE";
	break;
    case F_MUD:
	cp = "TinyMUD";
	break;
    case F_MUCK:
	cp = "TinyMUCK";
	break;
    default:
	cp = "*unknown*";
	break;
    }
    fprintf(stderr, "%s version %d:", cp, ver);
    if (flags & V_ZONE)
	fprintf(stderr, " Zone");
    if (flags & V_LINK)
	fprintf(stderr, " Link");
    if (flags & V_GDBM)
	fprintf(stderr, " GDBM");
    if (flags & V_ATRNAME)
	fprintf(stderr, " AtrName");
    if (flags & V_ATRKEY) {
	if ((fmt == F_MUSH) && (ver == 2))
	    fprintf(stderr, " ExtLocks");
	else
	    fprintf(stderr, " AtrKey");
    }
    if (flags & V_PARENT)
	fprintf(stderr, " Parent");
    if (flags & V_COMM)
	fprintf(stderr, " Comm");
    if (flags & V_ATRMONEY)
	fprintf(stderr, " AtrMoney");
    if (flags & V_XFLAGS)
	fprintf(stderr, " ExtFlags");
    fprintf(stderr, "\n");
}

void 
usage(char *prog)
{
    fprintf(stderr, "Usage: %s gdbm-file [flags] [<in-file] [>out-file]\n", prog);
    fprintf(stderr, "   Available flags are:\n");
    fprintf(stderr, "      C - Perform consistency check\n");
    fprintf(stderr, "      G - Write in gdbm format        g - Write in flat file format\n");
    fprintf(stderr, "      K - Store key as an attribute   k - Store key in the header\n");
    fprintf(stderr, "      L - Include link information    l - Don't include link information\n");
    fprintf(stderr, "      M - Store attr map if GDBM      m - Don't store attr map if GDBM\n");
    fprintf(stderr, "      N - Store name as an attribute  n - Store name in the header\n");
    fprintf(stderr, "      P - Include parent information  p - Don't include parent information\n");
    fprintf(stderr, "      W - Write the output file  b    w - Don't write the output file.\n");
    fprintf(stderr, "      X - Create a default GDBM db    x - Create a default flat file db\n");
    fprintf(stderr, "      Z - Include zone information    z - Don't include zone information\n");
    fprintf(stderr, "      <number> - Set output version number\n");
}

int 
main(int argc, char *argv[])
{
    int setflags, clrflags, ver, nochk;
    int db_ver, db_format, db_flags, do_check, do_write;
    char *fp;
    FILE *f_ptr;
    char s_filename[120];

    f_ptr = NULL;
    nochk = 0;
    if (argc == 3) {
	for (fp = argv[2]; *fp; fp++) {
           if (*fp == 'x') {
              nochk = 1;
              break;
           }
        }
    }
    if (!nochk) {
       sprintf(s_filename, "%.115s.db", argv[1]);
       if ( (f_ptr = fopen(s_filename, "r")) != NULL ) {
          fprintf(stderr, "GDBM file(s) already exist for %s\n", s_filename);
          fclose(f_ptr);
          exit(1);
       }
       sprintf(s_filename, "%.115s.dir", argv[1]);
       if ( (f_ptr = fopen(s_filename, "r")) != NULL ) {
          fprintf(stderr, "GDBM file(s) already exist for %s\n", s_filename);
          fclose(f_ptr);
          exit(1);
       }
       sprintf(s_filename, "%.115s.pag", argv[1]);
       if ( (f_ptr = fopen(s_filename, "r")) != NULL ) {
          fprintf(stderr, "GDBM file(s) already exist for %s\n", s_filename);
          fclose(f_ptr);
          exit(1);
       }
    }
    debugmem = (Debugmem *)malloc(sizeof(Debugmem));

    if( !debugmem )
      abort();

    INITDEBUG(debugmem);

    if ((argc < 2) || (argc > 3)) {
	usage(argv[0]);
	exit(1);
    }
    dddb_var_init();
    cache_var_init();
    cf_init();

    /* Decide what conversions to do and how to format the output file */

    setflags = clrflags = ver = do_check = 0;
    do_write = 1;

    if (argc == 3) {
	for (fp = argv[2]; *fp; fp++) {
	    switch (*fp) {
	    case 'C':
		do_check = 1;
		break;
	    case 'G':
		setflags |= V_GDBM;
		break;
	    case 'g':
		clrflags |= V_GDBM;
		break;
	    case 'Z':
		setflags |= V_ZONE;
		break;
	    case 'z':
		clrflags |= V_ZONE;
		break;
	    case 'L':
		setflags |= V_LINK;
		break;
	    case 'l':
		clrflags |= V_LINK;
		break;
	    case 'N':
		setflags |= V_ATRNAME;
		break;
	    case 'n':
		clrflags |= V_ATRNAME;
		break;
	    case 'K':
		setflags |= V_ATRKEY;
		break;
	    case 'k':
		clrflags |= V_ATRKEY;
		break;
	    case 'P':
		setflags |= V_PARENT;
		break;
	    case 'p':
		clrflags |= V_PARENT;
		break;
	    case 'W':
		do_write = 1;
		break;
	    case 'w':
		do_write = 0;
		break;
	    case 'X':
		clrflags = 0xffffffff;
		setflags = OUTPUT_FLAGS;
		ver = OUTPUT_VERSION;
		break;
	    case 'x':
		clrflags = 0xffffffff;
		setflags = UNLOAD_OUTFLAGS;
		ver = UNLOAD_VERSION;
		break;
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		ver = ver * 10 + (*fp - '0');
		break;
	    default:
		fprintf(stderr, "Unknown flag: '%c'\n", *fp);
		usage(argv[0]);
		exit(1);
	    }
	}
    }
    /* Open the gdbm file */

    if (init_gdbm_db(argv[1]) < 0) {
	fprintf(stderr, "Can't open GDBM file\n");
	exit(1);
    }
    /* Go do it */

    db_read(stdin, &db_format, &db_ver, &db_flags);
    fprintf(stderr, "Input: ");
    info(db_format, db_flags, db_ver);
    val_count();

    if (do_check)
	do_dbck(NOTHING, NOTHING, DBCK_FULL);

    if (do_write) {
	db_flags = (db_flags & ~clrflags) | setflags;
	if (db_format != F_MUSH)
	    db_ver = 3;
	if (ver != 0)
	    db_ver = ver;
	fprintf(stderr, "Output: ");
	info(F_MUSH, db_flags, db_ver);
	db_write(stdout, F_MUSH, db_ver | db_flags);
    }
    CLOSE;
    return(0);
}
