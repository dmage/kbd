#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include "version.h"
#include "kfont.h"
#include "xmalloc.h"

int main(int argc, char *argv[]) {
	set_progname(argv[0]);

	if (argc != 2) {
		fprintf(stderr, "usage: %s FONT.psf\n", progname);
		exit(1);
	}

	kfont_handler_t font;
	const char *const fonts_dirs[]         = { "", 0 };
	const char *const partial_fonts_dirs[] = { "", "../../data/partialfonts/", 0 };
	struct kfont_parse_options opts        = {
		.iunit              = 0,
		.fonts_dirs         = fonts_dirs,
		.partial_fonts_dirs = partial_fonts_dirs,
	};
	enum kfont_error err = kfont_load(argv[1], opts, &font);
	if (err != KFONT_ERROR_SUCCESS) {
		fprintf(stderr, "kfont_load: %d\n", err);
		exit(1);
	}

    uint32_t font_width  = kfont_get_width(font);
	uint32_t font_height = kfont_get_height(font);
    uint32_t font_len    = kfont_get_char_count(font);
	uint32_t pitch       = (font_width - 1) / 8 + 1;

    char *buf = (char *)malloc(pitch * 32 * font_len);
    for (int i = 0; i < font_len; i++) {
        memmove(buf + i*pitch*32, kfont_get_char_buffer(font, i), pitch * font_height);
    }

    struct console_font_op cfo;
    int fd = 0;

    cfo.op        = KD_FONT_OP_SET;
    cfo.flags     = 0;
    cfo.width     = font_width;
    cfo.height    = font_height;
    cfo.charcount = font_len;
    cfo.data      = buf;
    if (ioctl(fd, KDFONTOP, &cfo) != 0) {
        perror("ioctl: KDFONTOP");
        exit(1);
    }

    free(buf);
}
