/* TODO */

/*
 * mapscrn.c
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "kbd.h"
#include "paths.h"
#include "findfile.h"
#include "kdmapop.h"
#include "utf8.h"
#include "nls.h"
#include "kbd_error.h"

/* the two exported functions */
void saveoldmap(int fd, char *omfil);
void loadnewmap(int fd, char *mfil);

static int ctoi(char *);

/* search for the map file in these directories (with trailing /) */
static const char *const mapdirpath[]  = { "", DATADIR "/" TRANSDIR "/", 0 };
static const char *const mapsuffixes[] = { "", ".trans", "_to_uni.trans", ".acm", 0 };

/*
 * Read two-column file (index, value) with index in 0-255
 * and value in 0-65535. Acceptable representations:
 * decimal (nnn), octal (0nnn), hexadecimal (0xnnn), Unicode (U+xxxx),
 * character ('x').
 * In the character encoding, x may be a single byte binary value
 * (but not space, tab, comma, #) or a single Unicode value coded
 * as a UTF-8 sequence.
 *
 * Store values in ubuf[], and small values also in buf[].
 * Set u to 1 in case a value above 255 or a U+ occurs.
 * Set lineno to line number of first error.
 */
static int
parsemap(FILE *fp, char *buf, unsigned short *ubuf, int *u, int *lineno)
{
	char buffer[256];
	int in, on, ln, ret = 0;
	char *p, *q;

	ln = 0;
	while (fgets(buffer, sizeof(buffer) - 1, fp)) {
		ln++;
		if (!*u && strstr(buffer, "U+"))
			*u = 1;
		p          = strtok(buffer, " \t\n");
		if (p && *p != '#') {
			q = strtok(NULL, " \t\n#");
			if (q) {
				in = ctoi(p);
				on = ctoi(q);
				if (in >= 0 && in < 256 &&
				    on >= 0 && on < 65536) {
					ubuf[in] = on;
					if (on < 256)
						buf[in] = on;
					else
						*u = 1;
				} else {
					if (!ret)
						*lineno = ln;
					ret             = -1;
				}
			}
		}
	}
	return ret;
}

static int
readnewmapfromfile(char *mfil, char *buf, unsigned short *ubuf)
{
	struct stat stbuf;
	int u      = 0;
	int lineno = 0;
	lkfile_t fp;

	if (lk_findfile(mfil, mapdirpath, mapsuffixes, &fp)) {
		fprintf(stderr, _("mapscrn: cannot open map file _%s_\n"),
		        mfil);
		exit(1);
	}
	if (stat(fp.pathname, &stbuf)) {
		perror(fp.pathname);
		fprintf(stderr, _("Cannot stat map file"));
		exit(1);
	}
	if (stbuf.st_size == E_TABSZ) {
		if (verbose)
			printf(_("Loading binary direct-to-font screen map "
			         "from file %s\n"),
			       fp.pathname);
		if (fread(buf, E_TABSZ, 1, fp.fd) != 1) {
			fprintf(stderr,
			        _("Error reading map from file `%s'\n"),
			        fp.pathname);
			exit(1);
		}
	} else if (stbuf.st_size == 2 * E_TABSZ) {
		if (verbose)
			printf(_("Loading binary unicode screen map "
			         "from file %s\n"),
			       fp.pathname);
		if (fread(ubuf, 2 * E_TABSZ, 1, fp.fd) != 1) {
			fprintf(stderr,
			        _("Error reading map from file `%s'\n"),
			        fp.pathname);
			exit(1);
		}
		u = 1;
	} else {
		if (verbose)
			printf(_("Loading symbolic screen map from file %s\n"),
			       fp.pathname);
		if (parsemap(fp.fd, buf, ubuf, &u, &lineno)) {
			fprintf(stderr,
			        _("Error parsing symbolic map "
			          "from `%s', line %d\n"),
			        fp.pathname, lineno);
			exit(1);
		}
	}
	lk_fpclose(&fp);
	return u;
}

void loadnewmap(char *mfil)
{
	unsigned short ubuf[E_TABSZ];
	char buf[E_TABSZ];
	int i, u;

	/* default: trivial straight-to-font */
	for (i = 0; i < E_TABSZ; i++) {
		buf[i]  = i;
		ubuf[i] = (0xf000 + i);
	}

	u = 0;
	if (mfil)
		u = readnewmapfromfile(mfil, buf, ubuf);
}

/*
 * Read decimal, octal, hexadecimal, Unicode (U+xxxx) or character
 * ('x', x a single byte or a utf8 sequence).
 */
int ctoi(char *s)
{
	int i;

	if ((strncmp(s, "0x", 2) == 0) &&
	    (strspn(s + 2, "0123456789abcdefABCDEF") == strlen(s + 2)))
		(void)sscanf(s + 2, "%x", &i);

	else if ((*s == '0') &&
	         (strspn(s, "01234567") == strlen(s)))
		(void)sscanf(s, "%o", &i);

	else if (strspn(s, "0123456789") == strlen(s))
		(void)sscanf(s, "%d", &i);

	else if ((strncmp(s, "U+", 2) == 0) && strlen(s) == 6 &&
	         (strspn(s + 2, "0123456789abcdefABCDEF") == 4))
		(void)sscanf(s + 2, "%x", &i);

	else if ((strlen(s) == 3) && (s[0] == '\'') && (s[2] == '\''))
		i = s[1];

	else if (s[0] == '\'') {
		int err;
		char *s1 = s + 1;

		i = from_utf8(&s1, 0, &err);
		if (err || s1[0] != '\'' || s1[1] != 0)
			return -1;
	}

	else
		return -1;

	return i;
}
