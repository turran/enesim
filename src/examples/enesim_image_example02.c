#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include "Enesim.h"

static void help(void)
{
	printf("enesim_image_example02 INFILE OUTFILE\n");
}

int main(int argc, char **argv)
{
	Enesim_Buffer *b = NULL;
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

	enesim_init();
	if (!enesim_image_file_load(fload, &b, NULL, NULL))
	{
		printf("Impossible to load the image\n");
		goto end;
	}
	if (!enesim_image_file_save(fsave, b, NULL))
	{
		printf("Impossible to save the image\n");
		goto end;
	}

end:
	enesim_shutdown();
	return 0;
}

