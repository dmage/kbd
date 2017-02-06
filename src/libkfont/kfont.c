#include <stdio.h>
#include <string.h>

#include "kfont.h"
#include "kfontP.h"
#include "xmalloc.h"

const char *kfont_strerror(enum kfont_error err)
{
	if (err > 0) {
		return strerror(err);
	}

	switch (err) {
		case KFONT_ERROR_SUCCESS:
			return "No error";
		case KFONT_ERROR_READ:
			return "Read error";
	}

	// FIXME(dmage): it should be a reentrant function.
	static char msg[50];
	sprintf(msg, "Unknown error (%d)", (int)err);
	return msg;
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
	struct kfont_unimap_node *unimap = font->unimap_head;
	while (unimap) {
		struct kfont_unimap_node *next = unimap->next;
		xfree(unimap);
		unimap = next;
	}
	font->unimap_head = NULL;
	font->unimap_tail = NULL;

	if (font->blob) {
		xfree(font->blob);
		font->blob = NULL;
	}

	xfree(font);
}

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
