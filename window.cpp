#include <stdint.h>
#include <SDL.h>
#include <cstdio>
//#include "opengl.h"
#include <GL/gl3w.h>
#include "gui/imgui.h"
#include "gui/imgui_impl_sdl_gl3.h"
#include "Window.h"

/*
// hack for SDL2 http://stackoverflow.com/a/32449318
FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE * __cdecl __iob_func(void)
{
return _iob;
}
*/

/*
namespace {
	void checkSDLError(int line = -1)
	{
		const char *error = SDL_GetError();
		if (*error != '\0')
		{
			if (line != -1) {
				logger->info("Line: %i", line);
			}

			logger->error("SDL Error: %s", error);
			SDL_ClearError();
		}
	}
}
*/

SDL_Window* Window_handle;
static SDL_GLContext mainContext;
WindowConfig Window_config;

int Window_deinit()
{
	ImGui_ImplSdlGL3_Shutdown();

	if (mainContext) {
		SDL_GL_DeleteContext(mainContext);
	}

	if (Window_handle) {
		SDL_DestroyWindow(Window_handle);
	}

	SDL_Quit();
	return 0;
}

int Window_init(int width, int height, bool fullscreen, bool vsync, const char* title)
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY); 
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

#ifdef _DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

	if (fullscreen) {
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	if (!title) { title = "SDL Window"; }

	Window_handle = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width, height, flags);

	if (!Window_handle) {
		fprintf(stderr, "Couldn't create SDL window.\n");
		return -1;
	}

	//checkSDLError(__LINE__);

	mainContext = SDL_GL_CreateContext(Window_handle);

	if (!Window_handle) {
		fprintf(stderr, "Couldn't create OpenGL context.\n");
		goto err;
	}

	//checkSDLError(__LINE__);

	/*if (ogl_LoadFunctions() == ogl_LOAD_FAILED) {
		logger->error("Couldn't init OpenGL functions!");
	}*/

	if (gl3wInit()) {
		fprintf(stderr, "Failed initializing OpenGL functions.\n");
	}

	SDL_GL_SetSwapInterval(vsync ? 1 : 0);
	printf("VSYNC: %s\n", vsync ? "on" : "off");
	printf("SDL initialized.\n");
	printf("GL_VERSION: %s\n", glGetString(GL_VERSION));

#ifdef _DEBUG
#ifndef _DEBUG_NO_GL
	//eng::gl::setupDebugOutput();
#endif
#endif

	ImGui_ImplSdlGL3_Init(Window_handle);

	return 0;

	err:
	return -1;
}
