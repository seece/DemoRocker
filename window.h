#pragma once
#include <SDL.h>

struct WindowConfig {
	int width;
	int height;
	bool fullscreen;
	bool vsync;
};

extern WindowConfig Window_config;
extern SDL_Window* Window_handle;

int Window_init(int width, int height, bool fullscreen, bool vsync);
int Window_deinit();
