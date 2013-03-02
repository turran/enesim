#ifndef _EXAMPLE_H
#define _EXAMPLE_H

#define EXAMPLE(name)							\
	static Enesim_Renderer * name(void);				\
	int main(void)							\
	{								\
		Enesim_Renderer *r;					\
		Enesim_Surface *s;					\
		Enesim_Log *error = NULL;				\
									\
		enesim_init();						\
									\
		r = name();						\
		s = enesim_surface_new(ENESIM_FORMAT_ARGB8888,		\
				256, 256);				\
		if (!enesim_renderer_draw(r, s, NULL, 0, 0, &error))	\
		{							\
			enesim_log_dump(error);			\
		}							\
		enesim_image_file_save(#name ".png", s, NULL);		\
									\
		enesim_shutdown();					\
		return 0;						\
	}

#endif
