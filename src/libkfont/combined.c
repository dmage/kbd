// TODO(dmage): remove stdio and stdlib
#include <stdio.h>
#include <stdlib.h>

#include "kfont.h"
#include "kfontP.h"
#include "xmalloc.h"

static enum kfont_error kfontP_parse_combined_content(struct kfont_slice *p, kfont_handler_t font,
                                                      const char *const *partial_fonts_dirs)
{
	bool first = true;
	while (1) {
		const char *name = (char *)p->ptr;
		while (p->ptr != p->end) {
			if (*p->ptr == '\0') {
				// FIXME(dmage): \0 in text file
				return KFONT_ERROR_TRAILING_GARBAGE;
			}
			if (*p->ptr == '\n') {
				*p->ptr = '\0';
				break;
			}
			p->ptr++;
		}
		while (p->ptr == p->end) {
			// FIXME(dmage): no \n at the end of the file
			return KFONT_ERROR_TRAILING_GARBAGE;
		}
		p->ptr++;

		struct kfont_parse_options opts = {
			.iunit              = 0,
			.fonts_dirs         = partial_fonts_dirs, /* TODO(dmage): merge fonts_dirs and partial_fonts_dirs */
			.partial_fonts_dirs = 0,
		};
		kfont_handler_t out;
		enum kfont_error err = kfont_load(name, opts, &out);
		if (err != KFONT_ERROR_SUCCESS) {
			fprintf(stderr, "kfont_parse_combined: kfont_load error %d\n", err);
			// FIXME(dmage)
			abort();
		}

		if (first) {
			memmove(font, out, sizeof(struct kfont_handler));
			xfree(out);
			first = false;
		} else {
			err = kfont_append(font, out);
			if (err != KFONT_ERROR_SUCCESS) {
				fprintf(stderr, "kfont_parse_combined: kfont_append error %d\n", err);
				// FIXME(dmage)
				abort();
			}
		}

		if (p->ptr == p->end) {
			break;
		}
	}

	return KFONT_ERROR_SUCCESS;
}

enum kfont_error kfontP_parse_combined(struct kfont_slice *p, kfont_handler_t font,
                                       const char *const *partial_fonts_dirs)
{
	const unsigned char magic[] = "# combine partial fonts\n";
	if (!read_buf_magic(p, magic, sizeof(magic) - 1)) {
		return KFONT_ERROR_BAD_MAGIC;
	}

	// The content of the file should be released at the end of this function,
	// and shouldn't be destroyed by kfont_append.
	unsigned char *fileblob = font->blob;
	font->blob              = NULL;

	enum kfont_error err = kfontP_parse_combined_content(p, font, partial_fonts_dirs);

	if (fileblob) {
		xfree(fileblob);
	}

	return err;
}
