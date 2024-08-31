#include <pygame_c_api.h>

/* This file controls all the initialization of
 * the module and the various SDL subsystems
 */

/*platform specific init stuff*/

#ifdef MS_WIN32 /*python gives us MS_WIN32*/
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
extern int
SDL_RegisterApp(const char *, Uint32, void *);
#endif

#if defined(macintosh)
#if (!defined(__MWERKS__) && !TARGET_API_MAC_CARBON)
QDGlobals qd;
#endif
#endif

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define PAI_MY_ENDIAN '<'
#define PAI_OTHER_ENDIAN '>'
#define BUF_OTHER_ENDIAN '>'
#else
#define PAI_MY_ENDIAN '>'
#define PAI_OTHER_ENDIAN '<'
#define BUF_OTHER_ENDIAN '<'
#endif
#define BUF_MY_ENDIAN '='


int is_init = 0;
int sdl_was_init = 0;
SDL_Window *default_window = NULL;
Surface *default_screen = NULL;
char *env_blend_alpha_SDL2 = NULL;


int init()
{
    /*nice to initialize timer, so startup time will reflec init() time*/
#if defined(WITH_THREAD) && !defined(MS_WIN32) && defined(SDL_INIT_EVENTTHREAD)
    sdl_was_init = SDL_Init(SDL_INIT_EVENTTHREAD | SDL_INIT_TIMER |
                               SDL_INIT_NOPARACHUTE) == 0;
#else
    sdl_was_init = SDL_Init(SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE) == 0;
#endif

    env_blend_alpha_SDL2 = SDL_getenv("PYGAME_BLEND_ALPHA_SDL2");

    is_init = 1;

    return is_init;
}

void quit()
{
    if(default_screen->surf) {
        SDL_FreeSurface(default_screen->surf);
    }
    if (default_window) {
        SDL_DestroyWindow(default_window);
    }
    is_init = 0;
    if (sdl_was_init) {
        sdl_was_init = 0;
        SDL_Quit();
    }
}

/**
 * \brief Check if pygame is initialized.
 * \returns True if pygame is initialized, False otherwise.
 */
int base_get_init()
{
    return is_init;
}

/**
 * \brief Get the default SDL window created by a pygame.display.set_mode()
 * call, or *NULL*.
 *
 * \return The default window, or *NULL* if no window has been created.
 */
SDL_Window *
GetDefaultWindow(void)
{
    return default_window;
}

/**
 * \brief Set the default SDL window created by a pygame.display.set_mode()
 * call. The previous window, if any, is destroyed. Argument *win* may be
 * *NULL*. This function is called by pygame.display.set_mode().
 *
 * \param win The new default window. May be NULL.
 */
void
SetDefaultWindow(SDL_Window *win)
{
    /*Allows a window to be replaced by itself*/
    if (win == default_window) {
        return;
    }
    if (default_window) {
        SDL_DestroyWindow(default_window);
    }
    default_window = win;
}

/**
 * \brief Return a borrowed reference to the Pygame default window display
 * surface, or *NULL* if no default window is open.
 *
 * \return The default renderer, or *NULL* if no renderer has been created.
 */
Surface *
GetDefaultWindowSurface(void)
{
    /* return a borrowed reference*/
    return default_screen;
}

/**
 * \returns NULL if the environment variable PYGAME_BLEND_ALPHA_SDL2 is not
 * set, otherwise returns a pointer to the environment variable.
 */
char *
EnvShouldBlendAlphaSDL2(void)
{
    return env_blend_alpha_SDL2;
}


/**
 * \brief Set the Pygame default window display surface. The previous
 * surface, if any, is destroyed. Argument *screen* may be *NULL*. This
 * function is called by pygame.display.set_mode().
 *
 * \param screen The new default window display surface. May be NULL.
 */
void
SetDefaultWindowSurface(Surface *screen)
{
    /*a screen surface can be replaced with itself*/
    if (screen == default_screen) {
        return;
    }
    default_screen = screen;
}
