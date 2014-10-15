/* include glew before Enesim_OpenGL.h */
#include "enesim_example_renderer.h"

#if BUILD_OPENGL
#include <GL/glew.h>
#if BUILD_WGL

#define CHECK() 	\
{			\
	DWORD error;	\
	char tmp[1024];	\
	error = GetLastError(); \
	if (error != 0) {	\
		FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, \
				NULL, GetLastError(), 0, tmp, 1024, NULL); \
		printf("ERR: %s\n", tmp);	\
	} \
}

/* window handle */
static HWND hWnd;
/* rendering context */
static HGLRC hRC;
/* device context */
static HDC hDC;
/* renderer */
Enesim_Renderer *hR = NULL;
Enesim_Pool *pool = NULL;

static void wgl_display_surface(Enesim_Surface *s, Enesim_Example_Renderer_Options *options)
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
	glOrtho(0, options->width, 0, options->height, -1, 1);

	glViewport(0, 0, options->width, options->height);

	glMatrixMode(GL_TEXTURE);
        glLoadIdentity();

	glBegin(GL_QUADS);
		glTexCoord2d(0, 0); glVertex2f(0, 0);
		glTexCoord2d(1, 0); glVertex2f(options->width, 0);
		glTexCoord2d(1, 1); glVertex2f(options->width, options->height);
		glTexCoord2d(0, 1); glVertex2f(0, options->height);
        glEnd();
}

static Eina_Bool wgl_create(HWND hwnd)
{
	PIXELFORMATDESCRIPTOR pfd = {};
	int pf;

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;

	/* get the device context for window */
	hDC = GetDC(hwnd);

	pf = ChoosePixelFormat(hDC, &pfd);
	if (!pf)
	{
		return EINA_FALSE;
	}

	if (!SetPixelFormat(hDC, pf, &pfd))
	{
		return EINA_FALSE;
	}

	printf("hdc %p \n", hDC);
	/* create rendering context */
	hRC = wglCreateContext(hDC);
	if (!hRC)
	{
		CHECK();
	}
	printf("hrc %p \n", hRC);
	/* make rendering context current */
	wglMakeCurrent(hDC, hRC);
	return EINA_TRUE;
}

static void wgl_destroy(void)
{
	printf("destroy \n");
	wglMakeCurrent(hDC, NULL);	//deselect rendering context
	wglDeleteContext(hRC);		//delete rendering context
	PostQuitMessage(0);		//send wm_quit
}

static void wgl_paint(HWND hwnd)
{
	Enesim_Example_Renderer_Options *options;
	Enesim_Surface *s;

	printf("paint\n");
	options = (Enesim_Example_Renderer_Options *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	s = enesim_surface_new_pool_from(ENESIM_FORMAT_ARGB8888,
			options->width, options->height, pool);
	enesim_example_renderer_draw(hR, s, options);
	wgl_display_surface(s, options);
	wglSwapLayerBuffers(hDC, WGL_SWAP_MAIN_PLANE);
	enesim_surface_unref(s);
	enesim_pool_unref(pool);
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	printf("wnd proc!!! %08x\n", message);
	switch (message)
	{
		case WM_CREATE:
		wgl_create(hwnd);
		break;

		case WM_PAINT:
		wgl_paint(hwnd);
		break;
		
		case WM_DESTROY:
		wgl_destroy();
		break;

		default:
		break;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

static Eina_Bool wgl_setup(Enesim_Example_Renderer_Options *options)
{
	WNDCLASS wndClass;
	HINSTANCE hCurrentInst;

	printf("setting up ...%p %s\n", options, options->name);
	SetLastError(0);
	CHECK();

	hCurrentInst = (HINSTANCE)GetModuleHandle(NULL);
	CHECK();

	/* register window class */
	memset(&wndClass, 0, sizeof(wndClass));
	wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.hInstance = hCurrentInst;
	//wndClass.hbrBackground = GetStockObject(BLACK_BRUSH);
	wndClass.lpszClassName = "foo"; //options->name;
	if (!RegisterClass(&wndClass)) {
		CHECK();
		return EINA_FALSE;
	}

	/* create window */
	hWnd = CreateWindow("foo", options->name,
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		0, 0, options->width, options->height,
		NULL, NULL, hCurrentInst, NULL);
	if (!hWnd)
	{
		CHECK();
		return EINA_FALSE;
	}

	printf("done ...%p\n", hWnd);
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)options);

	return EINA_TRUE;
}

static void wgl_cleanup(void)
{

}

static void wgl_run(Enesim_Renderer *r,
		Enesim_Example_Renderer_Options *options EINA_UNUSED)
{
	MSG msg;

	printf("running ...\n");

	pool = enesim_pool_opengl_new();
	if (!pool) return;

	printf("we have the pool\n");
	/* set our renderer */
	hR = r;

	/* display window */
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	/* process messages */
	while (GetMessage(&msg, NULL, 0, 0) == TRUE) {
		printf("message received\n");
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	printf("done run\n");
}

Enesim_Example_Renderer_Backend_Interface
enesim_example_renderer_backend_wgl = {
	/* .setup 	= */ wgl_setup,
	/* .cleanup 	= */ wgl_cleanup,
	/* .run 	= */ wgl_run,
};

#endif
#endif
