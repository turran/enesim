#include "Emage.h"


int main(int argc, char **argv)
{
	Enesim_Surface *s = NULL;
	char *fload;
	char *fsave;

	fload = argv[1];
	fsave = argv[2];

	printf("%s -> %s\n", fload, fsave);

	emage_init();
	if (!emage_load(fload, &s, ENESIM_FORMAT_ARGB8888, NULL, NULL))
	{
		printf("Impossible to load the image\n");
		goto end;
	}
	if (!emage_save(fsave, s, NULL))
	{
		printf("Impossible to save the image\n");
		goto end;
	}

end:
	emage_shutdown();
	return 0;
}

