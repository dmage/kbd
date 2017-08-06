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

	if (opts.parse_legacy) {
		err = kfontP_parse_legacy(&p, *font, opts.iunit);
		if (err != KFONT_ERROR_BAD_MAGIC) {
			goto ret;
		}
	}

ret:
	if (err != KFONT_ERROR_SUCCESS) {
		kfont_free(*font);
	}
	return err;
}

enum kfont_error kfont_append(kfont_handler_t font, kfont_handler_t other)
{
	if (font->width != other->width || font->height != other->height || font->char_size != other->char_size) {
		return KFONT_ERROR_FONT_METRICS_MISMATCH;
	}

	if (font->char_count > UINT32_MAX - other->char_count) {
		return KFONT_ERROR_FONT_LENGTH_TOO_BIG;
	}
	uint32_t char_count   = font->char_count + other->char_count;
	uint32_t other_offset = font->char_count;

	if (char_count > SIZE_MAX / font->char_size) {
		return KFONT_ERROR_FONT_LENGTH_TOO_BIG;
	}
	unsigned char *glyphs = xmalloc(font->char_size * char_count);

	memmove(glyphs,
	        font->glyphs,
	        font->char_size * font->char_count);
	memmove(glyphs + font->char_size * font->char_count,
	        other->glyphs,
	        font->char_size * other->char_count);

	font->char_count = char_count;
	font->glyphs     = glyphs;

	if (font->blob) {
		xfree(font->blob);
	}
	font->blob = glyphs;

	if (font->unimap_tail) {
		font->unimap_tail->next = other->unimap_head;
		font->unimap_tail       = other->unimap_tail;
	} else {
		font->unimap_head = other->unimap_head;
		font->unimap_tail = other->unimap_tail;
	}

	struct kfont_unimap_node *unimap = other->unimap_head;
	while (unimap) {
		unimap->font_pos += other_offset;
		unimap = unimap->next;
	}

	other->unimap_head = NULL;
	other->unimap_tail = NULL;

	kfont_free(other);

	return KFONT_ERROR_SUCCESS;
}

void kfont_free(kfont_handler_t font)
{
	kfont_free_unimap(font->unimap_head);
	font->unimap_head = NULL;
	font->unimap_tail = NULL;

	if (font->blob) {
		xfree(font->blob);
		font->blob = NULL;
	}

	xfree(font);
}
