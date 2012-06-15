#include "Etex.h"

int main(void)
{
	Etex_Buffer *b;
	etex_init();

	b = etex_buffer_new(0);
	etex_buffer_string_set(b, "ABCDE", -1);
	printf("current string = %s\n", etex_buffer_string_get(b));
	etex_buffer_string_insert(b, "1234", -1, 3);
	printf("current string = %s\n", etex_buffer_string_get(b));
	etex_shutdown();
	return 0;
}
