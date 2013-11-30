#include "enesim_example_renderer.h"

#if BUILD_GLX

static Display *display;
static GLXWindow glx_win;

static Eina_Bool glx_setup(Enesim_Example_Renderer_Options *options)
{
	Window win;
	Window root;
	XSetWindowAttributes swa;
	XVisualInfo *vInfo;
	GLXFBConfig *fbConfigs;
	GLXContext context = NULL;
	GLXFBConfig config;
	int sb_attributes[] = {
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE,   GLX_RGBA_BIT,
		GLX_DOUBLEBUFFER,  True,
		GLX_RED_SIZE,      1,
		GLX_GREEN_SIZE,    1,
		GLX_BLUE_SIZE,     1,
		None
	};
	char *display_name;
	int swaMask;
	int num_fbs;
	int screen; 

	display_name = XDisplayName(NULL);
	display = XOpenDisplay(display_name);
	if (!display)
	{
		printf("Impossible to open display\n");
		return EINA_FALSE;
	}

	root = DefaultRootWindow(display);
	screen = DefaultScreen(display);

	/* try to create a glx context for the given window */
	fbConfigs = glXChooseFBConfig(display, screen, sb_attributes, &num_fbs);
	config = fbConfigs[0];

	/* create the X window */
	vInfo = glXGetVisualFromFBConfig(display, config);
	swa.border_pixel = 0;
	swa.event_mask = StructureNotifyMask;
	swa.colormap = XCreateColormap(display, root, vInfo->visual, AllocNone);

	swaMask = CWBorderPixel | CWColormap | CWEventMask;

	win = XCreateWindow(display, root, 0, 0, options->width, options->height,
			0, vInfo->depth, InputOutput, vInfo->visual,
			swaMask, &swa);

	XSelectInput(display, win, ExposureMask | KeyPressMask);
	XMapWindow(display, win);

	/* Create a GLX context for OpenGL rendering */
	context = glXCreateNewContext(display, config, GLX_RGBA_TYPE, NULL, True);
	if (!context)
	{
		printf("Not possible to create the glx context\n");
		return EINA_FALSE;
	}
	glx_win = glXCreateWindow(display, config, win, NULL);
	if (!glx_win)
	{
		printf("Not possible to create the glx window\n");
		return EINA_FALSE;
	}
	if (glXMakeContextCurrent(display, glx_win, glx_win, context) != True)
	{
		printf("Impossible to make context current");
		return EINA_FALSE;
	}

	return EINA_TRUE;
}

static void glx_display_surface(Enesim_Surface *s)
{
	const Enesim_Buffer_OpenGL_Data *data;

	data = enesim_surface_opengl_data_get(s);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glBindTexture(GL_TEXTURE_2D, data->textures[0]);

	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
	glOrtho(0, WIDTH, 0, HEIGHT, -1, 1);

	glViewport(0, 0, WIDTH, HEIGHT);

	glMatrixMode(GL_TEXTURE);
        glLoadIdentity();

	glBegin(GL_QUADS);
		glTexCoord2d(0, 0); glVertex2f(0, 0);
		glTexCoord2d(1, 0); glVertex2f(WIDTH, 0);
		glTexCoord2d(1, 1); glVertex2f(WIDTH, HEIGHT);
		glTexCoord2d(0, 1); glVertex2f(0, HEIGHT);
        glEnd();
}

static void glx_run(Enesim_Renderer *r,
		Enesim_Example_Renderer_Options *options)
{
	Enesim_Pool *pool;
	Enesim_Surface *s;
	XEvent e;

	pool = enesim_pool_opengl_new();
	if (!pool)
	{
		printf("Failed to create the pool\n");
		return;
	}
	s = enesim_surface_new_pool_from(ENESIM_FORMAT_ARGB8888,
			options->width, options->height, pool);

	while (1)
	{
		XNextEvent(display, &e);

		if (e.type == Expose) {
			/* draw the surface */
			enesim_example_renderer_draw(r, s, options);
			glx_display_surface(s);
			glXSwapBuffers(display, glx_win);
		}

		if (e.type == KeyPress) {
			break;
		}
	}

}

static void glx_cleanup(void)
{

}

Enesim_Example_Renderer_Backend_Interface
enesim_example_renderer_backend_glx = {
	/* .setup 	= */ glx_setup,
	/* .cleanup 	= */ glx_cleanup,
	/* .run 	= */ glx_run,
};

#endif
