#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <errno.h>

#include "version.h"
#include "kfont.h"
#include "xmalloc.h"

static void kernel_font_set(int fd, kfont_handler_t font) {
    struct console_font_op cfo;

    uint32_t font_width  = kfont_get_width(font);
	uint32_t font_height = kfont_get_height(font);
    uint32_t font_len    = kfont_get_char_count(font);
	uint32_t pitch       = (font_width - 1) / 8 + 1;

    char *buf = (char *)xmalloc(pitch * 32 * font_len);
    for (int i = 0; i < font_len; i++) {
        memmove(buf + i * pitch * 32, kfont_get_char_buffer(font, i), pitch * font_height);
    }

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

    xfree(buf);
}

static void kernel_put_unimap(int fd, kfont_handler_t font) {
	struct kfont_unimap_node *head = kfont_get_unicode_map(font), *unimap;
    if (!unimap) {
        return;
    }

    int pairs = 0;
    for (unimap = head; unimap; unimap = unimap->next) {
        if (unimap->len == 1) {
            ++pairs;
        }
    }

    struct unipair *up = (struct unimap *)xmalloc(pairs * sizeof(struct unipair));
    int i = 0;
    for (unimap = head; unimap; unimap = unimap->next) {
        if (unimap->len != 1) {
            continue;
        }
        up[i].unicode = unimap->seq[0];
        up[i].fontpos = unimap->font_pos;
        ++i;
    }
    // assert: i == pairs

    struct unimapdesc ud;
    ud.entry_ct = pairs;
    ud.entries  = up;

    struct unimapinit advice;
    advice.advised_hashsize = 0;
    advice.advised_hashstep = 0;
    advice.advised_hashlevel = 0;
    while (true) {
        if (ioctl(fd, PIO_UNIMAPCLR, &advice) != 0) {
            perror("ioctl: PIO_UNIMAPCLR");
            exit(1);
        }
        if (ioctl(fd, PIO_UNIMAP, ud) != 0) {
            if (errno == ENOMEM && advice.advised_hashlevel < 100) {
                perror("ioctl: ENOMEM (would retry)");
                advice.advised_hashlevel++;
                continue;
            }
            perror("ioctl: PIO_UNIMAP");
            exit(1);
        }
        break;
    }
}


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
		.parse_legacy       = true,
		.iunit              = 16,
		.fonts_dirs         = fonts_dirs,
		.partial_fonts_dirs = partial_fonts_dirs,
	};
	enum kfont_error err = kfont_load(argv[1], opts, &font);
	if (err != KFONT_ERROR_SUCCESS) {
		fprintf(stderr, "kfont_load: %d\n", err);
		exit(1);
	}

    kernel_font_set(0, font);
    kernel_put_unimap(0, font);
}
