#include <linux/keyboard.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ksyms.h"
#include "nls.h"

#include "keymap.h"

#include "syms.cp1250.h"
#include "syms.ethiopic.h"
#include "syms.iso8859_15.h"
#include "syms.iso8859_5.h"
#include "syms.iso8859_7.h"
#include "syms.iso8859_8.h"
#include "syms.iso8859_9.h"
#include "syms.koi8.h"
#include "syms.latin1.h"
#include "syms.latin2.h"
#include "syms.latin3.h"
#include "syms.latin4.h"
#include "syms.mazovia.h"
#include "syms.sami.h"
#include "syms.thai.h"

#include "syms.synonyms.h"

#include "syms.ktyp.h"

#define E(x) { x, sizeof(x) / sizeof(x[0]) }

syms_entry
const syms[] = {
	E(iso646_syms),		/* KT_LATIN */
	E(fn_syms),		/* KT_FN */
	E(spec_syms),		/* KT_SPEC */
	E(pad_syms),		/* KT_PAD */
	E(dead_syms),		/* KT_DEAD */
	E(cons_syms),		/* KT_CONS */
	E(cur_syms),		/* KT_CUR */
	E(shift_syms),		/* KT_SHIFT */
	{ 0, 0 },		/* KT_META */
	E(ascii_syms),		/* KT_ASCII */
	E(lock_syms),		/* KT_LOCK */
	{ 0, 0 },		/* KT_LETTER */
	E(sticky_syms),		/* KT_SLOCK */
	{ 0, 0 },		/*  */
	E(brl_syms)		/* KT_BRL */
};

#undef E

const unsigned int syms_size = sizeof(syms) / sizeof(syms_entry);
const unsigned int syn_size = sizeof(synonyms) / sizeof(synonyms[0]);

struct cs {
	const char *charset;
	sym *charnames;
	int start;
} charsets[] = {
	{ "",             NULL,                0 },
	{ "iso-8859-1",   latin1_syms,       160 },
	{ "iso-8859-2",   latin2_syms,       160 },
	{ "iso-8859-3",   latin3_syms,       160 },
	{ "iso-8859-4",   latin4_syms,       160 },
	{ "iso-8859-5",   iso_8859_5_syms,   160 },
	{ "iso-8859-7",   iso_8859_7_syms,   160 },
	{ "iso-8859-8",   iso_8859_8_syms,   160 },
	{ "iso-8859-9",   iso_8859_9_syms,   208 },
	{ "iso-8859-10",  latin6_syms,       160 },
	{ "iso-8859-15",  iso_8859_15_syms,  160 },
	{ "mazovia",      mazovia_syms,      128 },
	{ "cp-1250",      cp1250_syms,       128 },
	{ "koi8-r",       koi8_syms,         128 },
	{ "koi8-u",       koi8_syms,         128 },
	{ "tis-620",      tis_620_syms,      160 }, /* thai */
	{ "iso-10646-18", iso_10646_18_syms, 159 }, /* ethiopic */
	{ "iso-ir-197",   iso_ir_197_syms,   160 }, /* sami */
	{ "iso-ir-209",   iso_ir_209_syms,   160 }, /* sami */
};

static unsigned int charsets_size = sizeof(charsets) / sizeof(charsets[0]);
static unsigned int kmap_charset = 0;

/* Functions for both dumpkeys and loadkeys. */

void
list_charsets(FILE *f) {
	int lth,ct;
	unsigned int i, j;
	char *mm[] = { "iso-8859-", "koi8-" };

	for (j=0; j<sizeof(mm)/sizeof(mm[0]); j++) {
		if(j)
			fprintf(f, ",");
		fprintf(f, "%s{", mm[j]);
		ct = 0;
		lth = strlen(mm[j]);
		for(i=1; i < charsets_size; i++) {
			if(!strncmp(charsets[i].charset, mm[j], lth)) {
				if(ct++)
					fprintf(f, ",");
				fprintf(f, "%s", charsets[i].charset+lth);
			}
		}
		fprintf(f, "}");
	}
	for(i=1; i < charsets_size; i++) {
		for (j=0; j<sizeof(mm)/sizeof(mm[0]); j++) {
			lth = strlen(mm[j]);
			if(!strncmp(charsets[i].charset, mm[j], lth))
				goto nxti;
		}
		fprintf(f, ",%s", charsets[i].charset);
	nxti:;
	}
	fprintf(f, "\n");
}

int
set_charset(const char *charset) {
	unsigned int i;

	for (i = 1; i < charsets_size; i++) {
		if (!strcasecmp(charsets[i].charset, charset)) {
			kmap_charset = i;
			return 0;
		}
	}
	return 1;
}

const char *
codetoksym(int code) {
	unsigned int i;
	int j;
	sym *p;

	if (code < 0)
		return NULL;

	if (code < 0x1000) {	/* "traditional" keysym */
		if (code < 0x80)
			return iso646_syms[code];

		if (KTYP(code) == KT_META)
			return NULL;

		if (KTYP(code) == KT_LETTER)
			code = K(KT_LATIN, KVAL(code));

		if (KTYP(code) > KT_LATIN)
			return syms[KTYP(code)].table[KVAL(code)];

		p = charsets[kmap_charset].charnames;
		if (p) {
			p += KVAL(code) - charsets[kmap_charset].start;
			if (p->name[0])
				return p->name;
		}

		for (i = 1; i < charsets_size; i++) {
			p = charsets[i].charnames;
			if (p) {
				p += KVAL(code) - charsets[i].start;
				if (p->name[0])
					return p->name;
			}
		}
	}

	else {			/* Unicode keysym */
		code ^= 0xf000;

		if (code < 0x80)
			return iso646_syms[code];

		i = kmap_charset;
		while (1) {
			p = charsets[i].charnames;
			if (p) {
				for (j = charsets[i].start; j < 256; j++, p++) {
					if (p->uni == code && p->name[0])
						return p->name;
				}
			}

			i++;

			if (i == charsets_size)
				i = 0;
			if (i == kmap_charset)
				break;
		}

	}

	return NULL;
}

/* Functions for loadkeys. */

static int
kt_latin(struct keymap *kmap, const char *s, int direction) {
	int i, max;

	if (kmap_charset) {
		sym *p = charsets[kmap_charset].charnames;

		max = (direction == TO_UNICODE ? 128 : 256);

		for (i = charsets[kmap_charset].start; i < max; i++, p++) {
			if(p->name[0] && !strcmp(s, p->name))
				return K(KT_LATIN, i);
		}
	}

	max = (direction == TO_UNICODE ? 128 : syms[KT_LATIN].size);

	for (i = 0; i < max; i++) {
		if (!strcmp(s, syms[KT_LATIN].table[i]))
			return K(KT_LATIN, i);
	}

	return -1;
}

int
ksymtocode(struct keymap *kmap, const char *s, int direction) {
	unsigned int i;
	int n, j;
	int keycode;
	sym *p;

	if (direction == TO_AUTO)
		direction = kmap->prefer_unicode ? TO_UNICODE : TO_8BIT;

	if (!strncmp(s, "Meta_", 5)) {
		keycode = ksymtocode(kmap, s+5, TO_8BIT);
		if (KTYP(keycode) == KT_LATIN)
			return K(KT_META, KVAL(keycode));

		/* Avoid error messages for Meta_acute with UTF-8 */
		else if(direction == TO_UNICODE)
		        return (0);

		/* fall through to error printf */
	}

	if ((n = kt_latin(kmap, s, direction)) >= 0) {
		return n;
	}

	for (i = 1; i < syms_size; i++) {
		for (j = 0; j < syms[i].size; j++) {
			if (!strcmp(s,syms[i].table[j]))
				return K(i, j);
		}
	}

	for (i = 0; i < syn_size; i++)
		if (!strcmp(s, synonyms[i].synonym))
			return ksymtocode(kmap, synonyms[i].official_name, direction);

	if (direction == TO_UNICODE) {
		i = kmap_charset;

		while (1) {
			p = charsets[i].charnames;
			if (p) {
				for (j = charsets[i].start; j < 256; j++, p++) {
					if (!strcmp(s,p->name))
						return (p->uni ^ 0xf000);
				}
			}

			i++;

			if (i == charsets_size)
				i = 0;
			if (i == kmap_charset)
				break;
		}
	} else /* if (!chosen_charset[0]) */ {
		/* note: some keymaps use latin1 but with euro,
		   so set_charset() would fail */
		/* note: some keymaps with charset line still use
		   symbols from more than one character set,
		   so we cannot have the  `if (!chosen_charset[0])'  here */

		for (i = 0; i < 256 - 160; i++)
			if (!strcmp(s, latin1_syms[i].name)) {
				log_verbose(kmap, LOG_VERBOSE1,
					_("assuming iso-8859-1 %s\n"), s);
				return K(KT_LATIN, 160 + i);
			}

		for (i = 0; i < 256 - 160; i++)
			if (!strcmp(s, iso_8859_15_syms[i].name)) {
				log_verbose(kmap, LOG_VERBOSE1,
					_("assuming iso-8859-15 %s\n"), s);
				return K(KT_LATIN, 160 + i);
			}

		for (i = 0; i < 256 - 160; i++)
			if (!strcmp(s, latin2_syms[i].name)) {
				log_verbose(kmap, LOG_VERBOSE1,
					_("assuming iso-8859-2 %s\n"), s);
				return K(KT_LATIN, 160 + i);
			}

		for (i = 0; i < 256 - 160; i++)
			if (!strcmp(s, latin3_syms[i].name)) {
				log_verbose(kmap, LOG_VERBOSE1,
					_("assuming iso-8859-3 %s\n"), s);
				return K(KT_LATIN, 160 + i);
			}

		for (i = 0; i < 256 - 160; i++)
			if (!strcmp(s, latin4_syms[i].name)) {
				log_verbose(kmap, LOG_VERBOSE1,
					_("assuming iso-8859-4 %s\n"), s);
				return K(KT_LATIN, 160 + i);
			}
	}

	log_error(kmap, _("unknown keysym '%s'\n"), s);

	return CODE_FOR_UNKNOWN_KSYM;
}

int
convert_code(struct keymap *kmap, int code, int direction)
{
	const char *ksym;
	int unicode_forced = (direction == TO_UNICODE);
	int input_is_unicode = (code >= 0x1000);
	int result;

	if (direction == TO_AUTO)
		direction = kmap->prefer_unicode ? TO_UNICODE : TO_8BIT;

	if (KTYP(code) == KT_META)
		return code;
	else if (!input_is_unicode && code < 0x80)
		/* basic ASCII is fine in every situation */
		return code;
	else if (input_is_unicode && (code ^ 0xf000) < 0x80)
		/* so is Unicode "Basic Latin" */
		return code ^ 0xf000;
	else if ((input_is_unicode && direction == TO_UNICODE) ||
		 (!input_is_unicode && direction == TO_8BIT))
		/* no conversion necessary */
		result = code;
	else {
		/* depending on direction, this will give us either an 8-bit
		 * K(KTYP, KVAL) or a Unicode keysym xor 0xf000 */
		ksym = codetoksym(code);
		if (ksym)
			result = ksymtocode(kmap, ksym, direction);
		else
			result = code;
	}

	/* if direction was TO_UNICODE from the beginning, we return the true
	 * Unicode value (without the 0xf000 mask) */
	if (unicode_forced && result >= 0x1000)
		return result ^ 0xf000;
	else
		return result;
}

int
add_capslock(struct keymap *kmap, int code)
{
	if (KTYP(code) == KT_LATIN && (!(kmap->prefer_unicode) || code < 0x80))
		return K(KT_LETTER, KVAL(code));
	else if ((code ^ 0xf000) < 0x100)
		/* Unicode Latin-1 Supplement */
		/* a bit dirty to use KT_LETTER here, but it should work */
		return K(KT_LETTER, code ^ 0xf000);
	else
		return convert_code(kmap, code, TO_AUTO);
}
