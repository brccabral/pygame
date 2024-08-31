#pragma once
#include "_pygame.h"

// base.c
SDL_Window *
GetDefaultWindow(void);
Surface *
GetDefaultWindowSurface(void);
void
SetDefaultWindow(SDL_Window *win);
void
SetDefaultWindowSurface(Surface *screen);

// surface.c
Surface *
Surface_New2(SDL_Surface *s, int owner);
int
Surface_SetSurface(Surface *self, SDL_Surface *s, int owner);
