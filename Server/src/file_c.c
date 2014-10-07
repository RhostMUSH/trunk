/* file_c.c -- File cache management */

#include "copyright.h"
#include "autoconf.h"

#include "config.h"
#include "interface.h"
#include "mudconf.h"
#include "command.h"
#include "file_c.h"
#include "alloc.h"

typedef struct filecache_hdr FCACHE;
typedef struct filecache_block_hdr FBLKHDR;
typedef struct filecache_block FBLOCK;

struct filecache_hdr {
    char *filename;
    FBLOCK *fileblock;
    const char *desc;
};

struct filecache_block {
    struct filecache_block_hdr {
	struct filecache_block *nxt;
	int nchars;
    } hdr;
    char data[MBUF_SIZE - sizeof(FBLKHDR)];
};

#define FBLOCK_SIZE (MBUF_SIZE - sizeof(FBLKHDR))

FCACHE fcache[] =
{
    {mudconf.conn_file, NULL, "Conn"},
    {mudconf.site_file, NULL, "Conn/Badsite"},
    {mudconf.down_file, NULL, "Conn/Down"},
    {mudconf.full_file, NULL, "Conn/Full"},
    {mudconf.guest_file, NULL, "Conn/Guest"},
    {mudconf.creg_file, NULL, "Conn/Reg"},
    {mudconf.crea_file, NULL, "Crea/Newuser"},
    {mudconf.regf_file, NULL, "Crea/RegFaill"},
    {mudconf.motd_file, NULL, "Motd"},
    {mudconf.wizmotd_file, NULL, "Wizmotd"},
    {mudconf.quit_file, NULL, "Quit"},
    {mudconf.gconf_file, NULL, "Conn/GuestFail"},
    {mudconf.areg_file, NULL, "Conn/AutoReg"},
    {mudconf.aregh_file, NULL, "Conn/AutoRegHost"},
    {NULL, NULL, NULL}};

NAMETAB list_files[] =
{
    {(char *) "badsite_connect", 1, CA_WIZARD, 0, FC_CONN_SITE},
    {(char *) "connect", 2, CA_WIZARD, 0, FC_CONN},
    {(char *) "create_register", 2, CA_WIZARD, 0, FC_CREA_REG},
    {(char *) "down", 1, CA_WIZARD, 0, FC_CONN_DOWN},
    {(char *) "full", 1, CA_WIZARD, 0, FC_CONN_FULL},
    {(char *) "guest_motd", 1, CA_WIZARD, 0, FC_CONN_GUEST},
    {(char *) "motd", 1, CA_WIZARD, 0, FC_MOTD},
    {(char *) "newuser", 1, CA_WIZARD, 0, FC_CREA_NEW},
    {(char *) "quit", 1, CA_WIZARD, 0, FC_QUIT},
    {(char *) "register_connect", 1, CA_WIZARD, 0, FC_CONN_REG},
    {(char *) "wizard_motd", 1, CA_WIZARD, 0, FC_WIZMOTD},
    {(char *) "cguest_fail", 2, CA_WIZARD, 0, FC_GUEST_FAIL},
    {(char *) "autoreg_connect", 2, CA_WIZARD, 0, FC_CONN_AUTO},
    {(char *) "areghost_connect", 2, CA_WIZARD, 0, FC_CONN_AUTO_H},
    {NULL, 0, 0, 0, 0}};

void 
do_list_file(dbref player, dbref cause, int extra, char *arg)
{
    int flagvalue;

    flagvalue = search_nametab(player, list_files, arg);
    if (flagvalue < 0) {
	display_nametab(player, list_files,
			(char *) "Unknown file.  Use one of:", 1);
	return;
    }
    fcache_send(player, flagvalue);
}

static FBLOCK *
fcache_fill(FBLOCK * fp, char ch)
{
    FBLOCK *tfp;

    if (fp->hdr.nchars >= (MBUF_SIZE - sizeof(FBLKHDR))) {

	/* We filled the current buffer.  Go get a new one. */

	tfp = fp;
	fp = (FBLOCK *) alloc_mbuf("fcache_fill");
	fp->hdr.nxt = NULL;
	fp->hdr.nchars = 0;
	tfp->hdr.nxt = fp;
    }
    fp->data[fp->hdr.nchars++] = ch;
    return fp;
}

static int 
fcache_read(FBLOCK ** cp, char *filename)
{
    int n, nmax, nmax2, tchars, fd;
    char *buff, *buff2;
    char *lbuf1, *lbuf1ptr, *lbuf2, *lbuf2ptr;
    FBLOCK *fp, *tfp;
    int max_lines, tot_lines;

    /* Free a prior buffer chain */

    fp = *cp;
    while (fp != NULL) {
	tfp = fp->hdr.nxt;
	free_mbuf(fp);
	fp = tfp;
    }
    *cp = NULL;

    /* Read the text file into a new chain */

    if ((fd = tf_open(filename, O_RDONLY)) == -1) {

	/* Failure: log the event */

	STARTLOG(LOG_PROBLEMS, "FIL", "OPEN")
	    buff = alloc_mbuf("fcache_read.LOG");
	sprintf(buff, "Couldn't open file '%.100s'.", filename);
	log_text(buff);
	free_mbuf(buff);
	ENDLOG
	    tf_close(fd);
	return -1;
    }
    buff = alloc_lbuf("fcache_read.temp");

    /* Set up the initial cache buffer to make things easier */

    fp = (FBLOCK *) alloc_mbuf("fcache_read.first");
    fp->hdr.nxt = NULL;
    fp->hdr.nchars = 0;
    *cp = fp;
    tchars = 0;

    /* Process the file, one lbuf at a time */

    max_lines = mudconf.nonindxtxt_maxlines;
    tot_lines = 0;
    lbuf1 = alloc_lbuf("Testing");
    lbuf2 = alloc_lbuf("Testing2");
    if ( mudconf.ansi_txtfiles ) {
       memset(lbuf1, '\0', LBUF_SIZE);
       memset(lbuf2, '\0', LBUF_SIZE);
       lbuf1ptr = lbuf1;
       lbuf2ptr = lbuf2;
       nmax2 = read(fd, buff, LBUF_SIZE);
       if ( nmax2 > 0 ) {
#ifdef ZENTY_ANSI
          parse_ansi(buff, lbuf1, &lbuf1ptr, lbuf2, &lbuf2ptr);
#else
          strcpy(lbuf1, buff);
#endif
       }
       nmax = strlen(lbuf1);
    } else {
       nmax = nmax2 = read(fd, lbuf1, LBUF_SIZE);
    }
    while ( (nmax > 0) && (nmax2 > 0) ) {
	for (n = 0; n < nmax; n++) {
	    switch (lbuf1[n]) {
	    case '\n':
		fp = fcache_fill(fp, '\r');
		fp = fcache_fill(fp, '\n');
		tchars += 2;
	    case '\0':
	    case '\r':
		break;
	    default:
		fp = fcache_fill(fp, lbuf1[n]);
		tchars++;
	    }
	}
        if ( mudconf.ansi_txtfiles ) {
	   nmax2 = read(fd, buff, LBUF_SIZE);
           memset(lbuf1, '\0', LBUF_SIZE);
           memset(lbuf2, '\0', LBUF_SIZE);
           lbuf1ptr = lbuf1;
           lbuf2ptr = lbuf2;
           if ( nmax2 > 0 ) {
#ifdef ZENTY_ANSI
              parse_ansi(buff, lbuf1, &lbuf1ptr, lbuf2, &lbuf2ptr);
#else
              strcpy(lbuf1, buff);
#endif
           }
           nmax = strlen(lbuf1);
        } else {
	   nmax = nmax2 = read(fd, lbuf1, LBUF_SIZE);
        }
        tot_lines++;
        if ( tot_lines > max_lines ) {
	   STARTLOG(LOG_PROBLEMS, "FIL", "OPEN")
	      buff2 = alloc_mbuf("fcache_read.LOG");
	      sprintf(buff2, "Error with file '%.100s' (too large).", filename);
	      log_text(buff2);
	      free_mbuf(buff2);
	   ENDLOG
           break;
        }
    }
    free_lbuf(buff);
    free_lbuf(lbuf1);
    free_lbuf(lbuf2);
    tf_close(fd);

    /* If we didn't read anything in, toss the initial buffer */

    if (fp->hdr.nchars == 0) {
	*cp = NULL;
	free_mbuf(fp);
    }
    return tchars;
}

void 
fcache_rawdump(int fd, int num)
{
    int cnt, remaining;
    char *start;
    FBLOCK *fp;

    if ((num < 0) || (num > FC_LAST))
	return;
    fp = fcache[num].fileblock;

    while (fp != NULL) {
	start = fp->data;
	remaining = fp->hdr.nchars;
	while (remaining > 0) {

	    cnt = WRITE(fd, start, remaining);
	    if (cnt < 0)
		return;
	    remaining -= cnt;
	    start += cnt;
	}
	fp = fp->hdr.nxt;
    }
    return;
}

void 
fcache_dump(DESC * d, int num)
{
    FBLOCK *fp;

    if ((num < 0) || (num > FC_LAST))
	return;
    fp = fcache[num].fileblock;

    while (fp != NULL) {
  	queue_write(d, fp->data, fp->hdr.nchars);
	fp = fp->hdr.nxt;
    }
}

void 
fcache_send(dbref player, int num)
{
    DESC *d;

    DESC_ITER_PLAYER(player, d) {
	fcache_dump(d, num);
    }
}

void 
fcache_load(dbref player)
{
    FCACHE *fp;
    char *buff, *bufc, *sbuf, filename[129 + 32];
    int i;

    buff = bufc = alloc_lbuf("fcache_load.lbuf");
    sbuf = alloc_sbuf("fcache_load.sbuf");
    for (fp = fcache; fp->filename; fp++) {
        sprintf(filename, "%s/%s", mudconf.txt_dir, fp->filename);
	i = fcache_read(&fp->fileblock, filename);
	if ((player != NOTHING) && !Quiet(player)) {
	    sprintf(sbuf, "%d", i);
	    if (fp == fcache)
		safe_str((char *) "File sizes: ", buff, &bufc);
	    else
		safe_str((char *) "  ", buff, &bufc);
	    safe_str((char *) fp->desc, buff, &bufc);
	    safe_str((char *) "...", buff, &bufc);
	    safe_str(sbuf, buff, &bufc);
	}
    }
    *bufc = '\0';
    if ((player != NOTHING) && !Quiet(player)) {
	notify(player, buff);
    }
    free_lbuf(buff);
    free_sbuf(sbuf);
}

void 
NDECL(fcache_init)
{
    FCACHE *fp;

    for (fp = fcache; fp->filename; fp++) {
	fp->fileblock = NULL;
    }
    fcache_load(NOTHING);
}
