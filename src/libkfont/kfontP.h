#ifndef KFONTP_H
#define KFONTP_H

#include <stdint.h>

#include "kfont.h"
#include "slice.h"

#define PSF1_MAGIC 0x0436
#define PSF2_MAGIC 0x864ab572

#define PSF1_MODE512 0x01
#define PSF1_MODE_HAS_TAB 0x02
#define PSF1_MODE_HAS_SEQ 0x04
#define PSF1_MAXMODE 0x05 // TODO(dmage): why not 0x07?

#define PSF1_SEPARATOR 0xFFFF
#define PSF1_START_SEQ 0xFFFE

/* bits used in flags */
#define PSF2_HAS_UNICODE_TABLE 0x01

/* max version recognized so far */
#define PSF2_MAXVERSION 0

/* UTF8 separators */
#define PSF2_SEPARATOR 0xFF
#define PSF2_START_SEQ 0xFE

struct kfont_psf1_header {
	uint8_t mode;      /* PSF font mode */
	uint8_t char_size; /* character size */
};

struct kfont_psf2_header {
	uint32_t version;
	uint32_t header_size; /* offset of bitmaps in file */
	uint32_t flags;
	uint32_t length;        /* number of glyphs */
	uint32_t char_size;     /* number of bytes for each character */
	uint32_t height, width; /* max dimensions of glyphs */
	                        /* charsize = height * ((width + 7) / 8) */
};

struct kfont_handler {
	uint32_t width;
	uint32_t height;
	uint32_t char_size;
	uint32_t char_count;
	const unsigned char *glyphs;

	struct kfont_unimap_node *unimap_head;
	struct kfont_unimap_node *unimap_tail;

	unsigned char *blob;
};

enum kfont_error kfontP_parse_psf1(struct kfont_slice *p, kfont_handler_t font);

enum kfont_error kfontP_parse_psf2(struct kfont_slice *p, kfont_handler_t font);

enum kfont_error kfontP_parse_combined(struct kfont_slice *p, kfont_handler_t font,
                                       const char *const *partial_fonts_dirs);

enum kfont_error kfontP_parse_legacy(struct kfont_slice *p, kfont_handler_t font);

#endif
