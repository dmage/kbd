// TODO(dmage): remove stdio and stdlib
#include <stdio.h>
#include <stdlib.h>

#include "kfont.h"
#include "kfontP.h"
#include "xmalloc.h"

static bool kfontP_read_psf1_header(struct kfont_slice *p, struct kfont_psf1_header *header)
{
	return read_uint8(p, &header->mode) &&
	       read_uint8(p, &header->char_size);
}

static bool kfontP_read_psf2_header(struct kfont_slice *p, struct kfont_psf2_header *header)
{
	return read_uint32(p, &header->version) &&
	       read_uint32(p, &header->header_size) &&
	       read_uint32(p, &header->flags) &&
	       read_uint32(p, &header->length) &&
	       read_uint32(p, &header->char_size) &&
	       read_uint32(p, &header->height) &&
	       read_uint32(p, &header->width);
}

static bool kfontP_read_psf1_unicode(struct kfont_slice *p, struct kfont_unimap_node **unimap)
{
	uint16_t uc;
	if (!read_uint16(p, &uc)) {
		return false;
	}

	if (uc == PSF1_SEPARATOR) {
		*unimap = NULL;
		return true;
	}

	if (uc == PSF1_START_SEQ) {
		fprintf(stderr, "psf1 read unicode map: <start seq>\n");
		abort();
		// TODO(dmage): handle <seq>*
	}

	*unimap = xmalloc(sizeof(struct kfont_unimap_node) + sizeof(uint32_t));

	(*unimap)->len    = 1;
	(*unimap)->seq[0] = uc;

	return true;
}

static bool kfontP_read_psf2_unicode(struct kfont_slice *p, struct kfont_unimap_node **unimap)
{
	if (p->ptr + 1 > p->end) {
		return false;
	}

	if (*p->ptr == PSF2_SEPARATOR) {
		p->ptr++;
		*unimap = NULL;
		return true;
	}

	if (*p->ptr == PSF2_START_SEQ) {
		fprintf(stderr, "psf2 read unicode map: <start seq>\n");
		abort();
		// TODO(dmage): handle <seq>*
	}

	uint32_t uc;
	if (!read_utf8_code_point(p, &uc)) {
		return false;
	}

	*unimap = xmalloc(sizeof(struct kfont_unimap_node) + sizeof(uint32_t));

	(*unimap)->len    = 1;
	(*unimap)->seq[0] = uc;

	return true;
}

static enum kfont_error kfontP_parse_psf_unimap(struct kfont_slice *p,
                                                kfont_handler_t font,
                                                bool (*read_unicode)(struct kfont_slice *p, struct kfont_unimap_node **unimap))
{
	for (uint32_t font_pos = 0; font_pos < font->char_count; font_pos++) {
		while (1) {
			struct kfont_unimap_node *unimap;
			if (!read_unicode(p, &unimap)) {
				return KFONT_ERROR_SHORT_UNICODE_TABLE;
			}
			if (!unimap) {
				// end of unicode map for this font position
				break;
			}

			if (unimap->len == 0) {
				// XXX: free(unimap)
				fprintf(stderr, "read unicode map %d: <empty sequence>\n", font_pos);
				abort(); // TODO(dmage)
			}
			for (unsigned int i = 0; i < unimap->len; i++) {
				if (unimap->seq[i] == INVALID_CODE_POINT) {
					// XXX: free(unimap)
					fprintf(stderr, "read unicode map %d: <invalid utf8>\n", font_pos);
					abort(); // TODO(dmage)
				}
			}

			unimap->font_pos = font_pos;

			if (font->unimap_tail) {
				font->unimap_tail->next = unimap;
			} else {
				font->unimap_head = unimap;
			}
			font->unimap_tail = unimap;
		}
	}
	return KFONT_ERROR_SUCCESS;
}

enum kfont_error kfontP_parse_psf1(struct kfont_slice *p, kfont_handler_t font)
{
	unsigned char *begin = p->ptr;

	if (!read_uint16_magic(p, PSF1_MAGIC)) {
		return KFONT_ERROR_BAD_MAGIC;
	}

	struct kfont_psf1_header psf1_header;
	if (!kfontP_read_psf1_header(p, &psf1_header)) {
		return KFONT_ERROR_BAD_PSF1_HEADER;
	}

	if (psf1_header.mode > PSF1_MAXMODE) {
		return KFONT_ERROR_UNSUPPORTED_PSF1_MODE;
	}

	/* TODO: check char_size */
	size_t size = p->end - begin;
	if (psf1_header.char_size == 0) {
		return KFONT_ERROR_CHAR_SIZE_ZERO;
	}
	if (psf1_header.char_size > size /* - sizeof(header) */) {
		return KFONT_ERROR_CHAR_SIZE_TOO_BIG;
	}
	/* TODO: check char_count */

	font->width      = 8;
	font->height     = psf1_header.char_size;
	font->char_size  = psf1_header.char_size;
	font->char_count = (psf1_header.mode & PSF1_MODE512 ? 512 : 256);
	font->glyphs     = p->ptr;

	if (psf1_header.mode & (PSF1_MODE_HAS_TAB | PSF1_MODE_HAS_SEQ)) {
		p->ptr += font->char_size * font->char_count;

		enum kfont_error err = kfontP_parse_psf_unimap(p, font, kfontP_read_psf1_unicode);
		if (err != KFONT_ERROR_SUCCESS) {
			return err;
		}

		if (p->ptr != p->end) {
			return KFONT_ERROR_TRAILING_GARBAGE;
		}
	}

	return KFONT_ERROR_SUCCESS;
}

enum kfont_error kfontP_parse_psf2(struct kfont_slice *p, kfont_handler_t font)
{
	unsigned char *begin = p->ptr;

	if (!read_uint32_magic(p, PSF2_MAGIC)) {
		return KFONT_ERROR_BAD_MAGIC;
	}

	struct kfont_psf2_header psf2_header;
	if (!kfontP_read_psf2_header(p, &psf2_header)) {
		return KFONT_ERROR_BAD_PSF2_HEADER;
	}

	if (psf2_header.version > PSF2_MAXVERSION) {
		return KFONT_ERROR_UNSUPPORTED_PSF2_VERSION;
	}

	/* check that the buffer is large enough for the header and glyphs */
	size_t size = p->end - begin;
	if (psf2_header.header_size > size) {
		return KFONT_ERROR_FONT_OFFSET_TOO_BIG;
	}
	if (psf2_header.char_size == 0) {
		return KFONT_ERROR_CHAR_SIZE_ZERO;
	}
	if (psf2_header.char_size > size - psf2_header.header_size) {
		return KFONT_ERROR_CHAR_SIZE_TOO_BIG;
	}
	if (psf2_header.length > (size - psf2_header.header_size) / psf2_header.char_size) {
		return KFONT_ERROR_FONT_LENGTH_TOO_BIG;
	}

	/* check that width, height and char_size are consistent */
	if (psf2_header.width == 0) {
		return KFONT_ERROR_FONT_WIDTH_ZERO;
	}
	uint32_t pitch = (psf2_header.width - 1) / 8 + 1;
	if (psf2_header.height != psf2_header.char_size / pitch) {
		return KFONT_ERROR_PSF2_BAD_HEIGHT;
	}

	font->width      = psf2_header.width;
	font->height     = psf2_header.height;
	font->char_size  = psf2_header.char_size;
	font->char_count = psf2_header.length;
	font->glyphs     = begin + psf2_header.header_size;

	if (psf2_header.flags & PSF2_HAS_UNICODE_TABLE) {
		p->ptr = begin + psf2_header.header_size + font->char_size * font->char_count;

		enum kfont_error err = kfontP_parse_psf_unimap(p, font, kfontP_read_psf2_unicode);
		if (err != KFONT_ERROR_SUCCESS) {
			return err;
		}

		if (p->ptr != p->end) {
			return KFONT_ERROR_TRAILING_GARBAGE;
		}
	}

	return KFONT_ERROR_SUCCESS;
}
