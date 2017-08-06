#include <stdio.h>
#include <string.h>

#include "kfont.h"

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
		case KFONT_ERROR_BAD_MAGIC:
			return "Unsupported font file";
		case KFONT_ERROR_FONT_LENGTH_TOO_BIG:
			return "Font length is too big";
	}

	// FIXME(dmage): it should be a reentrant function.
	static char msg[50];
	sprintf(msg, "Unknown error (%d)", (int)err);
	return msg;
}
