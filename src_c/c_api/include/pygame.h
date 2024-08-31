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
int
RGBAFromObj(Color *obj, Uint8 *RGBA);

// surface.c
Surface *
Surface_New2(SDL_Surface *s, int owner);
int
Surface_SetSurface(Surface *self, SDL_Surface *s, int owner);

// surface_fill.c
int
surface_fill_blend(SDL_Surface *surface, SDL_Rect *rect, Uint32 color,
                   int blendargs);

// surflock
void
Surface_Prep(Surface *surfobj);
void
Surface_Unprep(Surface *surfobj);
int
Surface_Lock(Surface *surfobj);
int
Surface_Unlock(Surface *surfobj);
int
Surface_LockBy(Surface *surfobj, Surface *lockobj);
int
Surface_UnlockBy(Surface *surfobj, Surface *lockobj);
