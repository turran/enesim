#include "Enesim_Image.h"

static void help(void)
{
	printf("enesim_image_example02 INFILE OUTFILE\n");
}

int main(int argc, char **argv)
{
	Enesim_Surface *s = NULL;
	char *fload;
	char *fsave;

	if (argc < 3)
	{
		help();
		return -1;
	}

	fload = argv[1];
	fsave = argv[2];

	printf("%s -> %s\n", fload, fsave);

	enesim_image_init();
	if (!enesim_image_file_load(fload, &s, ENESIM_FORMAT_ARGB8888, NULL, NULL))
	{
		printf("Impossible to load the image\n");
		goto end;
	}
	if (!enesim_image_file_save(fsave, s, NULL))
	{
		printf("Impossible to save the image\n");
		goto end;
	}

end:
	enesim_image_shutdown();
	return 0;
}

