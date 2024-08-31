#include <pygame_c_api.h>

// int _event_get(SDL_Event *events, size_t size_events, struct EventType *eventtype, int pump, struct EventType *exclude)
// {
//     if(!events)
//     {
//         return -1;
//     }
// }

#define _PG_HANDLE_PROXIFY(name) \
    case SDL_##name:             \
    case PGPOST_##name:          \
        return proxify ? PGPOST_##name : SDL_##name

#define _PG_HANDLE_PROXIFY_PGE(name) \
    case PGE_##name:                 \
    case PGPOST_##name:              \
        return proxify ? PGPOST_##name : PGE_##name

/* The next three functions are used for proxying SDL events to and from
 * PGPOST_* events.
 *
 * Some SDL1 events (SDL_ACTIVEEVENT, SDL_VIDEORESIZE and SDL_VIDEOEXPOSE)
 * are redefined with SDL2, they HAVE to be proxied.
 *
 * SDL_USEREVENT is not proxied, because with SDL2, pygame assigns a
 * different event in place of SDL_USEREVENT, and users use PGE_USEREVENT
 *
 * Each WINDOW_* event must be defined twice, once as an event, and also
 * again, as a proxy event. WINDOW_* events MUST be proxied.
 */

static Uint32
_event_proxify_helper(Uint32 type, Uint8 proxify)
{
    switch (type) {
        _PG_HANDLE_PROXIFY(ACTIVEEVENT);
        _PG_HANDLE_PROXIFY(APP_TERMINATING);
        _PG_HANDLE_PROXIFY(APP_LOWMEMORY);
        _PG_HANDLE_PROXIFY(APP_WILLENTERBACKGROUND);
        _PG_HANDLE_PROXIFY(APP_DIDENTERBACKGROUND);
        _PG_HANDLE_PROXIFY(APP_WILLENTERFOREGROUND);
        _PG_HANDLE_PROXIFY(APP_DIDENTERFOREGROUND);
        _PG_HANDLE_PROXIFY(AUDIODEVICEADDED);
        _PG_HANDLE_PROXIFY(AUDIODEVICEREMOVED);
        _PG_HANDLE_PROXIFY(CLIPBOARDUPDATE);
        _PG_HANDLE_PROXIFY(CONTROLLERAXISMOTION);
        _PG_HANDLE_PROXIFY(CONTROLLERBUTTONDOWN);
        _PG_HANDLE_PROXIFY(CONTROLLERBUTTONUP);
        _PG_HANDLE_PROXIFY(CONTROLLERDEVICEADDED);
        _PG_HANDLE_PROXIFY(CONTROLLERDEVICEREMOVED);
        _PG_HANDLE_PROXIFY(CONTROLLERDEVICEREMAPPED);
#if SDL_VERSION_ATLEAST(2, 0, 14)
        _PG_HANDLE_PROXIFY(CONTROLLERTOUCHPADDOWN);
        _PG_HANDLE_PROXIFY(CONTROLLERTOUCHPADMOTION);
        _PG_HANDLE_PROXIFY(CONTROLLERTOUCHPADUP);
        _PG_HANDLE_PROXIFY(CONTROLLERSENSORUPDATE);
#endif
        _PG_HANDLE_PROXIFY(DOLLARGESTURE);
        _PG_HANDLE_PROXIFY(DOLLARRECORD);
        _PG_HANDLE_PROXIFY(DROPFILE);
        _PG_HANDLE_PROXIFY(DROPTEXT);
        _PG_HANDLE_PROXIFY(DROPBEGIN);
        _PG_HANDLE_PROXIFY(DROPCOMPLETE);
        _PG_HANDLE_PROXIFY(FINGERMOTION);
        _PG_HANDLE_PROXIFY(FINGERDOWN);
        _PG_HANDLE_PROXIFY(FINGERUP);
        _PG_HANDLE_PROXIFY(KEYDOWN);
        _PG_HANDLE_PROXIFY(KEYUP);
        _PG_HANDLE_PROXIFY(KEYMAPCHANGED);
        _PG_HANDLE_PROXIFY(JOYAXISMOTION);
        _PG_HANDLE_PROXIFY(JOYBALLMOTION);
        _PG_HANDLE_PROXIFY(JOYHATMOTION);
        _PG_HANDLE_PROXIFY(JOYBUTTONDOWN);
        _PG_HANDLE_PROXIFY(JOYBUTTONUP);
        _PG_HANDLE_PROXIFY(JOYDEVICEADDED);
        _PG_HANDLE_PROXIFY(JOYDEVICEREMOVED);
#if SDL_VERSION_ATLEAST(2, 0, 14)
        _PG_HANDLE_PROXIFY(LOCALECHANGED);
#endif
        _PG_HANDLE_PROXIFY(MOUSEMOTION);
        _PG_HANDLE_PROXIFY(MOUSEBUTTONDOWN);
        _PG_HANDLE_PROXIFY(MOUSEBUTTONUP);
        _PG_HANDLE_PROXIFY(MOUSEWHEEL);
        _PG_HANDLE_PROXIFY(MULTIGESTURE);
        _PG_HANDLE_PROXIFY(NOEVENT);
        _PG_HANDLE_PROXIFY(QUIT);
        _PG_HANDLE_PROXIFY(RENDER_TARGETS_RESET);
        _PG_HANDLE_PROXIFY(RENDER_DEVICE_RESET);
        _PG_HANDLE_PROXIFY(SYSWMEVENT);
        _PG_HANDLE_PROXIFY(TEXTEDITING);
        _PG_HANDLE_PROXIFY(TEXTINPUT);
        _PG_HANDLE_PROXIFY(VIDEORESIZE);
        _PG_HANDLE_PROXIFY(VIDEOEXPOSE);
        _PG_HANDLE_PROXIFY_PGE(MIDIIN);
        _PG_HANDLE_PROXIFY_PGE(MIDIOUT);
        _PG_HANDLE_PROXIFY_PGE(WINDOWSHOWN);
        _PG_HANDLE_PROXIFY_PGE(WINDOWHIDDEN);
        _PG_HANDLE_PROXIFY_PGE(WINDOWEXPOSED);
        _PG_HANDLE_PROXIFY_PGE(WINDOWMOVED);
        _PG_HANDLE_PROXIFY_PGE(WINDOWRESIZED);
        _PG_HANDLE_PROXIFY_PGE(WINDOWSIZECHANGED);
        _PG_HANDLE_PROXIFY_PGE(WINDOWMINIMIZED);
        _PG_HANDLE_PROXIFY_PGE(WINDOWMAXIMIZED);
        _PG_HANDLE_PROXIFY_PGE(WINDOWRESTORED);
        _PG_HANDLE_PROXIFY_PGE(WINDOWENTER);
        _PG_HANDLE_PROXIFY_PGE(WINDOWLEAVE);
        _PG_HANDLE_PROXIFY_PGE(WINDOWFOCUSGAINED);
        _PG_HANDLE_PROXIFY_PGE(WINDOWFOCUSLOST);
        _PG_HANDLE_PROXIFY_PGE(WINDOWCLOSE);
        _PG_HANDLE_PROXIFY_PGE(WINDOWTAKEFOCUS);
        _PG_HANDLE_PROXIFY_PGE(WINDOWHITTEST);
        _PG_HANDLE_PROXIFY_PGE(WINDOWICCPROFCHANGED);
        _PG_HANDLE_PROXIFY_PGE(WINDOWDISPLAYCHANGED);
        default:
            return type;
    }
}

static Uint32
_event_proxify(Uint32 type)
{
    return _event_proxify_helper(type, 1);
}

static Uint32
_event_deproxify(Uint32 type)
{
    return _event_proxify_helper(type, 0);
}

static int
_translate_windowevent(void *_, SDL_Event *event)
{
    if (event->type == SDL_WINDOWEVENT) {
        event->type = PGE_WINDOWSHOWN + event->window.event - 1;
        return SDL_EventState(_event_proxify(event->type), SDL_QUERY);
    }
    return 1;
}

static void
_event_pump(int dopump)
{
    if (dopump) {
        SDL_PumpEvents();
    }
    /* We need to translate WINDOWEVENTS. But if we do that from the
     * from event filter, internal SDL stuff that rely on WINDOWEVENT
     * might break. So after every event pump, we translate events from
     * here */
    SDL_FilterEvents(_translate_windowevent, NULL);
}

int event_get(SDL_Event *eventbuf, size_t size_events)
{
    VIDEO_INIT_CHECK_C();
    if(!eventbuf || size_events <= 0)
    {
        return -1;
    }
    _event_pump(1);

    return SDL_PeepEvents(eventbuf, size_events, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
}

// int event_get(SDL_Event *events, size_t size_events, struct EventType *eventtype)
// {
//     return _event_get(events, size_events, eventtype, 1, NULL);
// }
// int event_get(SDL_Event *events, size_t size_events, int pump)
// {
//     return _event_get(events, size_events, NULL, pump, NULL);
// }
// int event_get(SDL_Event *events, size_t size_events, struct EventType *eventtype, struct EventType *exclude)
// {
//     // cannot pass exclude and eventtype at the same time
//     if(eventtype && exclude)
//     {
//         return -1;
//     }
//     return _event_get(events, size_events, eventtype, 1, exclude);
// }
// int event_get(SDL_Event *events, size_t size_events, int pump, struct EventType *exclude)
// {
//     return _event_get(events, size_events, NULL, pump, exclude);
// }
// int event_get(SDL_Event *events, size_t size_events, struct EventType *eventtype, int pump, struct EventType *exclude)
// {
//     // cannot pass exclude and eventtype at the same time
//     if(eventtype && exclude)
//     {
//         return -1;
//     }
//     return _event_get(events, size_events, eventtype, pump, exclude);
// }
