#include "Enesim_Text.h"

int main(void)
{
	Enesim_Text_Buffer *b;
	enesim_text_init();

	b = enesim_text_buffer_new(0);
	enesim_text_buffer_string_set(b, "ABCDE", -1);
	printf("current string = %s\n", enesim_text_buffer_string_get(b));
	enesim_text_buffer_string_insert(b, "1234", -1, 3);
	printf("current string = %s\n", enesim_text_buffer_string_get(b));
	enesim_text_shutdown();
	return 0;
}
