#include <stddef.h>

#include "kfont.h"
#include "kfontP.h"

uint32_t kfont_get_width(kfont_handler_t font)
{
	return font->width;
}

uint32_t kfont_get_height(kfont_handler_t font)
{
	return font->height;
}

uint32_t kfont_get_char_count(kfont_handler_t font)
{
	return font->char_count;
}

const unsigned char *kfont_get_char_buffer(kfont_handler_t font, uint32_t font_pos)
{
	if (font_pos >= font->char_count) {
		return NULL;
	}
	return font->glyphs + font_pos * font->char_size;
}

struct kfont_unimap_node *kfont_get_unicode_map(kfont_handler_t font)
{
	return font->unimap_head;
}
