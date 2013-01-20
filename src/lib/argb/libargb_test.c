/* gcc -I. -Wall libargb_test.c -o libargb_test -mmmx -msse2 `pkg-config --cflags eina` */
#define LIBARGB_MMX 1
#define LIBARGB_SSE2 1
#define LIBARGB_DEBUG 0

#include "Eina.h"
#include "libargb.h"

int main(void)
{
	return 0;
}
