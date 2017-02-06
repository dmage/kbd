// TODO(dmage): remove stdio
#include <stdio.h>

#include "kfont.h"
#include "kfontP.h"

enum kfont_error kfontP_parse_legacy(struct kfont_slice *p, kfont_handler_t font, uint8_t iunit)
{
	size_t len = p->end - p->ptr;
	if (iunit != 0 && len == 9780) {
		ptrdiff_t offset;
		switch (iunit) {
			case 8:
				offset = 7732;
				break;
			case 14:
				offset = 4142;
				break;
			case 16:
				offset = 40;
				break;
			default:
				return KFONT_ERROR_UNSUPPORTED_FONT_HEIGHT;
		}

		font->width      = 8;
		font->height     = 16;
		font->char_size  = 16;
		font->char_count = 256;
		font->glyphs     = p->ptr + offset;

		return KFONT_ERROR_SUCCESS;
	} else if (len == 32768) {
		// hwunit = 16?

		// TODO(dmage)

		font->width      = 8;
		font->height     = 32;
		font->char_size  = 32;
		font->char_count = 512;
		font->glyphs     = p->ptr;

		return KFONT_ERROR_SUCCESS;
	} else if (len % 256 == 0 || len % 256 == 40) {
		if (len % 256 == 40) {
			p->ptr += 40;
		}

		size_t height = (p->end - p->ptr) / 256;
		if (height > UINT32_MAX) {
			return KFONT_ERROR_CHAR_SIZE_TOO_BIG;
		}

		font->width      = 8;
		font->height     = height;
		font->char_size  = height;
		font->char_count = 256;
		font->glyphs     = p->ptr;

		return KFONT_ERROR_SUCCESS;
	}
	return KFONT_ERROR_BAD_MAGIC;
}
