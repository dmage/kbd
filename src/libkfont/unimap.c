#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "findfileP.h"
#include "kfont.h"
#include "kfontP.h"
#include "xmalloc.h"

#define _(x) x

static void kfontP_unimap_append(struct kfont_unimap_node **head, struct kfont_unimap_node **tail,
	                             uint32_t font_pos, uint32_t uc)
{
	struct kfont_unimap_node *unimap = xmalloc(sizeof(struct kfont_unimap_node) + sizeof(uint32_t));

	unimap->font_pos = font_pos;
	unimap->len      = 1;
	unimap->seq[0]   = uc;

	if (*tail) {
		(*tail)->next = unimap;
		*tail = unimap;
	} else {
		*head = unimap;
		*tail = unimap;
	}
}

/*
 * Skip spaces and read U+1234 or return -1 for error.
 * Return first non-read position in *p0 (unchanged on error).
 */
static int getunicode(char **p0)
{
	char *p = *p0;

	while (*p == ' ' || *p == '\t')
		p++;
#if 0
	/* The code below also allows one to accept 'utf8' */
	if (*p == '\'') {
		int err;
		unsigned long u;
		char *p1 = p+1;

		/*
		 * Read a single complete utf-8 character, and
		 * expect it to be closed by a single quote.
		 */
		u = from_utf8(&p1, 0, &err);
		if (err || *p1 != '\'')
			return -1;
		*p0 = p1+1;
		return u;
	}
#endif
	if (*p != 'U' || p[1] != '+' || !isxdigit(p[2]) || !isxdigit(p[3]) ||
	    !isxdigit(p[4]) || !isxdigit(p[5]) || isxdigit(p[6]))
		return -1;
	*p0 = p + 6;
	return strtol(p + 2, 0, 16);
}

/*
 * Syntax accepted:
 *	<fontpos>	<unicode> <unicode> ...
 *	<range>		idem
 *	<range>		<unicode>
 *	<range>		<unicode range>
 *
 * where <range> ::= <fontpos>-<fontpos>
 * and <unicode> ::= U+<h><h><h><h>
 * and <h> ::= <hexadecimal digit>
 */

static void parseline(char *buffer, char *tblname, struct kfont_unimap_node **head, struct kfont_unimap_node **tail)
{
	int fontlen = 512;
	int i;
	int fp0, fp1, un0, un1;
	char *p, *p1;

	p = buffer;

	while (*p == ' ' || *p == '\t')
		p++;
	if (!*p || *p == '#')
		return; /* skip comment or blank line */

	fp0 = strtol(p, &p1, 0);
	if (p1 == p) {
		fprintf(stderr, _("Bad input line: %s\n"), buffer);
		abort(); // FIXME(dmage)
	}
	p = p1;

	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '-') {
		p++;
		fp1 = strtol(p, &p1, 0);
		if (p1 == p) {
			fprintf(stderr, _("Bad input line: %s\n"), buffer);
			abort(); // FIXME(dmage)
		}
		p = p1;
	} else
		fp1 = 0;

	if (fp0 < 0 || fp0 >= fontlen) {
		fprintf(stderr,
		        _("%s: Glyph number (0x%x) larger than font length\n"),
		        "tblname", fp0);
		abort(); // FIXME(dmage)
	}
	if (fp1 && (fp1 < fp0 || fp1 >= fontlen)) {
		fprintf(stderr,
		        _("%s: Bad end of range (0x%x)\n"),
		        "tblname", fp1);
		abort(); // FIXME(dmage)
	}

	if (fp1) {
		/* we have a range; expect the word "idem" or a Unicode range
		   of the same length or a single Unicode value */
		while (*p == ' ' || *p == '\t')
			p++;
		if (!strncmp(p, "idem", 4)) {
			p += 4;
			for (i = fp0; i <= fp1; i++)
				kfontP_unimap_append(head, tail, i, i);
			goto lookattail;
		}

		un0 = getunicode(&p);
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p != '-') {
			for (i = fp0; i <= fp1; i++)
				kfontP_unimap_append(head, tail, i, un0);
			goto lookattail;
		}

		p++;
		un1 = getunicode(&p);
		if (un0 < 0 || un1 < 0) {
			fprintf(stderr,
			        _("%s: Bad Unicode range corresponding to "
			          "font position range 0x%x-0x%x\n"),
			        "tblname", fp0, fp1);
			abort(); // FIXME(dmage)
		}
		if (un1 - un0 != fp1 - fp0) {
			fprintf(stderr,
			        _("%s: Unicode range U+%x-U+%x not of the same"
			          " length as font position range 0x%x-0x%x\n"),
			        "tblname", un0, un1, fp0, fp1);
			abort(); // FIXME(dmage)
		}
		for (i = fp0; i <= fp1; i++)
			kfontP_unimap_append(head, tail, i, un0 - fp0 + i);

	} else {
		/* no range; expect a list of unicode values
		   for a single font position */

		while ((un0 = getunicode(&p)) >= 0)
			kfontP_unimap_append(head, tail, fp0, un0);
	}
lookattail:
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p && *p != '#')
		fprintf(stderr, _("%s: trailing junk (%s) ignored\n"),
		        tblname, p);
}

// FIXME(dmage)
// static const char *const unidirpath[]  = { "", DATADIR "/" UNIMAPDIR "/", 0 };
static const char *const unidirpath[]  = { "", 0 };
static const char *const unisuffixes[] = { "", ".uni", ".sfm", 0 };

enum kfont_error kfont_load_unimap(const char *filename, struct kfont_unimap_node **unimap)
{
	char buffer[65536];
	char *p;
	fpfile_t fp;

	if (findfile(filename, unidirpath, unisuffixes, &fp) != 0) {
		return errno;
	}

	// TODO(dmage)
	// if (verbose)
	// 	printf(_("Loading unicode map from file %s\n"), fp.pathname);

	struct kfont_unimap_node *head, *tail;

	while (fgets(buffer, sizeof(buffer), fp.fd) != NULL) {
		if ((p = strchr(buffer, '\n')) != NULL)
			*p = '\0';
		else
			// FIXME(dmage)
			fprintf(stderr, _("%s: %s: Warning: line too long\n"),
			        "progname", filename);

		parseline(buffer, filename, &head, &tail);
	}

	fpclose(&fp);

	*unimap = head;

	return KFONT_ERROR_SUCCESS;
}

void kfont_free_unimap(struct kfont_unimap_node *unimap)
{
	while (unimap) {
		struct kfont_unimap_node *next = unimap->next;
		xfree(unimap);
		unimap = next;
	}
}
