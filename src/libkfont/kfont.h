/**
 * @file kfont.h
 * @brief This file describes libkfont's public API.
 */
#ifndef KFONT_H
#define KFONT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Error codes.
 */
enum kfont_error {
	KFONT_ERROR_SUCCESS                  = 0,
	KFONT_ERROR_READ                     = -2,
	KFONT_ERROR_BAD_MAGIC                = -3,
	KFONT_ERROR_BAD_PSF1_HEADER          = -4,
	KFONT_ERROR_UNSUPPORTED_PSF1_MODE    = -5,
	KFONT_ERROR_BAD_PSF2_HEADER          = -6,
	KFONT_ERROR_UNSUPPORTED_PSF2_VERSION = -7,
	KFONT_ERROR_PSF2_BAD_HEIGHT          = -17,
	KFONT_ERROR_TRAILING_GARBAGE         = -8,
	KFONT_ERROR_SHORT_UNICODE_TABLE      = -9,
	KFONT_ERROR_FONT_OFFSET_TOO_BIG      = -10,
	KFONT_ERROR_CHAR_SIZE_ZERO           = -11,
	KFONT_ERROR_CHAR_SIZE_TOO_BIG        = -12,
	KFONT_ERROR_FONT_WIDTH_ZERO          = -16,
	KFONT_ERROR_FONT_LENGTH_TOO_BIG      = -13,
	KFONT_ERROR_FONT_METRICS_MISMATCH    = -14,
	KFONT_ERROR_NOT_FOUND                = -15,
	KFONT_ERROR_UNSUPPORTED_FONT_HEIGHT  = -18,
};

/**
 * @brief Returns the error message string corresponding to an error code.
 */
const char *kfont_strerror(enum kfont_error err);

/**
 * @brief Configuration options for the font parser.
 */
struct kfont_parse_options {
	bool parse_legacy;

	/**
	 * A desired font height for files with several point sizes.
	 * Value 0 means such fonts should be rejected.
	 */
	uint8_t iunit;

	/**
	 * TODO(dmage)
	 */
	const char *const *fonts_dirs;
	const char *const *partial_fonts_dirs;
};

/**
 * @brief An opaque font descriptor.
 */
typedef struct kfont_handler *kfont_handler_t;

/**
 * @brief Reads and parses a font from a file specified by name.
 * If the font is successfully loaded, it should be freed with @ref kfont_free.
 */
enum kfont_error kfont_load(const char *filename, struct kfont_parse_options opts, kfont_handler_t *font);

/**
 * @brief Parses a font from a buffer.
 * If the font is successfully loaded, it should be freed with @ref kfont_free.
 * The buffer should not be freed until the font is freed.
 */
enum kfont_error kfont_parse(unsigned char *buf, size_t size, struct kfont_parse_options opts, kfont_handler_t *font);

/**
 * @brief Combines two fonts. Resources associated with the font @p other will be released.
 */
enum kfont_error kfont_append(kfont_handler_t font, kfont_handler_t other);

/**
 * @brief Releases resources associated with a font.
 */
void kfont_free(kfont_handler_t font);

/**
 * @brief Returns the width of a font's characters.
 */
uint32_t kfont_get_width(kfont_handler_t font);

/**
 * @brief Returns the height of a font's characters.
 */
uint32_t kfont_get_height(kfont_handler_t font);

/**
 * @brief Returns the number of characters in a font.
 */
uint32_t kfont_get_char_count(kfont_handler_t font);

/**
 * @brief Returns a buffer with a symbol representation. It should not be freed.
 *
 * @code
 * // buf:
 * //      0  1  2[ 3] 4  5  6  7
 * //   0 -- ## ## ## ## ## -- --
 * // [ 1]## ## --[--]-- ## ## --
 * //   2 -- -- -- -- ## ## -- --
 * //   3 -- -- -- ## ## -- -- --
 * //   4 -- -- -- ## ## -- -- --
 * //   5 -- -- -- -- -- -- -- --
 * //   6 -- -- -- ## ## -- -- --
 * //   7 -- -- -- -- -- -- -- --
 *
 * width = 8
 * row = 1
 * col = 3
 *
 * byte_idx = row*ceil(width/8) + floor(col/8)
 * bit_idx = col % 8
 *
 * selected_bit = (buf[byte_idx] & (0x80 >> bit_idx))
 * @endcode
 */
const unsigned char *kfont_get_char_buffer(kfont_handler_t font, uint32_t font_pos);

/**
 * @brief A linked list of pairs (font_pos, seq[len]).
 */
struct kfont_unimap_node {
	/**
	 * A next element of the linked list or NULL.
	 */
	struct kfont_unimap_node *next;

	/**
	 * A font position.
	 */
	uint32_t font_pos;

	/**
	 * A length of @ref seq.
	 */
	unsigned int len;

	/**
	 * A sequence of Unicode characters which is represented by the glyph at
	 * the font position @ref font_pos.
	 */
	uint32_t seq[];
};

/**
 * @brief Returns a Unicode description of a font's glyphs.
 * The result is associated with the font and should not be freed.
 */
struct kfont_unimap_node *kfont_get_unicode_map(kfont_handler_t font);

/**
 * @brief Loads a unicode map from a file specified by name.
 * If the unicode map is successfully loaded, it should be freed with @ref kfont_free_unimap.
 */
enum kfont_error kfont_load_unimap(const char *filename, struct kfont_unimap_node **unimap);

/**
 * FIXME(dmage)
 */
enum kfont_error kfont_save_unimap(const char *filename, struct kfont_unimap_node *unimap);

/**
 * @brief Releases resources associated with the unicode map.
 */
void kfont_free_unimap(struct kfont_unimap_node *unimap);

#endif /* KFONT_H */
