#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "version.h"
#include "kfont.h"

int main(int argc, char *argv[])
{
	set_progname(argv[0]);

	if (argc != 2) {
		fprintf(stderr, "usage: %s FILENAME.uni\n", progname);
		exit(1);
	}

	struct kfont_unimap_node *unimap;
	enum kfont_error err = kfont_load_unimap(argv[1], &unimap);
	if (err != KFONT_ERROR_SUCCESS) {
		fprintf(stderr, "kfont_load_unimap: %s\n", kfont_strerror(err));
		exit(1);
	}

	for (; unimap; unimap = unimap->next) {
		for (uint32_t i = 0; i < unimap->len; i++) {
			printf("U+%04X ", unimap->seq[i]);
		}
		printf("-> %u\n", unimap->font_pos);
	}

	// FIXME(dmage)
	// kfont_free(font);
}
