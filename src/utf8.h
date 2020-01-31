#ifndef _UTF8_H
#define _UTF8_H

#include <stddef.h>

extern unsigned int from_utf8(unsigned char **inptr, ptrdiff_t cnt, int *err);

#define UTF8_BAD (-1)
#define UTF8_SHORT (-2)

#endif /* _UTF8_H */
