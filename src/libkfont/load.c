#include <stdio.h>

#include "findfileP.h"
#include "kfont.h"
#include "kfontP.h"
#include "xmalloc.h"

#define MAXFONTSIZE 65536

static bool kfontP_blob_read(FILE *f, unsigned char **buffer, size_t *size)
{
	size_t buflen      = MAXFONTSIZE / 4; /* actually an arbitrary value */
	size_t n           = 0;
	unsigned char *buf = xmalloc(buflen);

	while (1) {
		if (n == buflen) {
			if (buflen > SIZE_MAX / 2) {
				xfree(buf);
				return false;
			}
			buflen *= 2;
			buf = xrealloc(buf, buflen);
		}
		n += fread(buf + n, 1, buflen - n, f);
		if (ferror(f)) {
			xfree(buf);
			return false;
		}
		if (feof(f)) {
			break;
		}
	}

	*buffer = buf;
	*size   = n;

	return true;
}

enum kfont_error kfont_load(const char *filename, struct kfont_parse_options opts, kfont_handler_t *font)
{
	const char *const suffixes[] = { "", ".psfu", ".psf", ".cp", ".fnt", 0 };
	fpfile_t fp;
	if (findfile(filename, opts.fonts_dirs, suffixes, &fp) != 0) {
		return KFONT_ERROR_NOT_FOUND;
	}

	unsigned char *buf;
	size_t size;
	if (!kfontP_blob_read(fp.fd, &buf, &size)) {
		fpclose(&fp);
		return KFONT_ERROR_READ;
	}

	fpclose(&fp);

	enum kfont_error err = kfont_parse(buf, size, opts, font);
	if (err != KFONT_ERROR_SUCCESS) {
		xfree(buf);
		return err;
	}

	(*font)->blob = buf;

	return KFONT_ERROR_SUCCESS;
}

enum kfont_error kfont_parse(unsigned char *buf, size_t size, struct kfont_parse_options opts, kfont_handler_t *font)
{
	struct kfont_slice p;
	p.ptr = buf;
	p.end = buf + size;

	*font = xmalloc(sizeof(struct kfont_handler));

	(*font)->unimap_head = NULL;
	(*font)->unimap_tail = NULL;
	(*font)->blob        = NULL;

	enum kfont_error err = kfontP_parse_psf2(&p, *font);
	if (err != KFONT_ERROR_BAD_MAGIC) {
		goto ret;
	}

	err = kfontP_parse_psf1(&p, *font);
	if (err != KFONT_ERROR_BAD_MAGIC) {
		goto ret;
	}

	if (opts.partial_fonts_dirs) {
		err = kfontP_parse_combined(&p, *font, opts.partial_fonts_dirs);
		if (err != KFONT_ERROR_BAD_MAGIC) {
			goto ret;
		}
	}

	err = kfontP_parse_legacy(&p, *font);
	if (err != KFONT_ERROR_BAD_MAGIC) {
		goto ret;
	}

ret:
	if (err != KFONT_ERROR_SUCCESS) {
		kfont_free(*font);
	}
	return err;
}
