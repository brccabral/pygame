#pragma once

#include "_pygame.h"

int event_get(SDL_Event * events, size_t size_events);
// int event_get(SDL_Event * events, size_t size_events, struct EventType *eventtype);
// int event_get(SDL_Event * events, size_t size_events, int pump);
// int event_get(SDL_Event * events, size_t size_events, struct EventType *eventtype, struct EventType *exclude);
// int event_get(SDL_Event * events, size_t size_events, int pump, struct EventType *exclude);
// int event_get(SDL_Event * events, size_t size_events, struct EventType *eventtype, int pump, struct EventType *exclude);
