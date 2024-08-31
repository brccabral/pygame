#include <pygame_c_api.h>
#include <assert.h>
#include "pgopengl.h"
#include "pygame.h"

#if !defined(__APPLE__)
char *icon_defaultname = "pygame_icon.bmp";
int icon_colorkey = 0;
#else
char *icon_defaultname = "pygame_icon_mac.bmp";
int icon_colorkey = -1;
#endif

SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;

typedef struct _display_state_s {
    char *title;
    void *icon;
    Uint16 *gamma_ramp;
    SDL_GLContext gl_context;
    int toggle_windowed_w;
    int toggle_windowed_h;
    Uint8 using_gl; /* using an OPENGL display without renderer */
    Uint8 scaled_gl;
    int scaled_gl_w;
    int scaled_gl_h;
    int fullscreen_backup_x;
    int fullscreen_backup_y;
    SDL_bool auto_resize;
} _DisplayState;

_DisplayState *state = NULL;

static void
_display_state_cleanup(_DisplayState *state)
{
    if (state->title) {
        free(state->title);
        state->title = NULL;
    }
    if (state->icon) {
        free(state->icon);
        state->icon = NULL;
    }
    if (state->gl_context) {
        SDL_GL_DeleteContext(state->gl_context);
        state->gl_context = NULL;
    }
    if (state->gamma_ramp) {
        free(state->gamma_ramp);
        state->gamma_ramp = NULL;
    }
}

static int
_get_display(SDL_Window *win)
{
    char *display_env = SDL_getenv("PYGAME_DISPLAY");
    int display = 0; /* default display 0 */

    if (win != NULL) {
        display = SDL_GetWindowDisplayIndex(win);
        return display;
    }
    else if (display_env != NULL) {
        display = SDL_atoi(display_env);
        return display;
    }
    /* On e.g. Linux X11, checking the mouse pointer requires that the
     * video subsystem is initialized to avoid crashes.
     *
     * Note that we do not bother raising an error here; the condition will
     * be rechecked after parsing the arguments and the function will throw
     * the relevant error there.
     */
    else if (SDL_WasInit(SDL_INIT_VIDEO)) {
        /* get currently "active" desktop, containing mouse ptr */
        int num_displays, i;
        SDL_Rect display_bounds;
        SDL_Point mouse_position;
        SDL_GetGlobalMouseState(&mouse_position.x, &mouse_position.y);
        num_displays = SDL_GetNumVideoDisplays();

        for (i = 0; i < num_displays; i++) {
            if (SDL_GetDisplayBounds(i, &display_bounds) == 0) {
                if (SDL_PointInRect(&mouse_position, &display_bounds)) {
                    display = i;
                    break;
                }
            }
        }
    }
    return display;
}

/*
** Looks at the SDL1 environment variables:
**    - SDL_VIDEO_WINDOW_POS
*         "x,y"
*         "center"
**    - SDL_VIDEO_CENTERED
*         if set the window should be centered.
*
*  Returns:
*      0 if we do not want to position the window.
*      1 if we set the x and y.
*          x, and y are set to the x and y.
*          center_window is set to 0.
*      2 if we want the window centered.
*          center_window is set to 1.
*/
static int
_get_video_window_pos(int *x, int *y, int *center_window)
{
    const char *sdl_video_window_pos = SDL_getenv("SDL_VIDEO_WINDOW_POS");
    const char *sdl_video_centered = SDL_getenv("SDL_VIDEO_CENTERED");
    int xx, yy;
    if (sdl_video_window_pos) {
        if (SDL_sscanf(sdl_video_window_pos, "%d,%d", &xx, &yy) == 2) {
            *x = xx;
            *y = yy;
            *center_window = 0;
            return 1;
        }
        if (SDL_strcmp(sdl_video_window_pos, "center") == 0) {
            sdl_video_centered = sdl_video_window_pos;
        }
    }
    if (sdl_video_centered) {
        *center_window = 1;
        return 2;
    }
    return 0;
}

static int SDLCALL
ResizeEventWatch(void *userdata, SDL_Event *event)
{
    SDL_Window *pygame_window;
    _DisplayState *state;
    SDL_Window *window;

    if (event->type != SDL_WINDOWEVENT)
        return 0;

    pygame_window = GetDefaultWindow();
    state = userdata;

    window = SDL_GetWindowFromID(event->window.windowID);
    if (window != pygame_window)
        return 0;

    if (renderer != NULL) {
        if (event->window.event == SDL_WINDOWEVENT_MAXIMIZED) {
            SDL_RenderSetIntegerScale(renderer, SDL_FALSE);
        }
        if (event->window.event == SDL_WINDOWEVENT_RESTORED) {
            SDL_RenderSetIntegerScale(
                renderer, !(SDL_GetHintBoolean(
                                 "SDL_HINT_RENDER_SCALE_QUALITY", SDL_FALSE)));
        }
        return 0;
    }

    if (state->using_gl) {
        if (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            GL_glViewport_Func p_glViewport =
                (GL_glViewport_Func)SDL_GL_GetProcAddress("glViewport");
            int wnew = event->window.data1;
            int hnew = event->window.data2;
            SDL_GL_MakeCurrent(pygame_window, state->gl_context);
            if (state->scaled_gl) {
                float saved_aspect_ratio =
                    ((float)state->scaled_gl_w) / (float)state->scaled_gl_h;
                float window_aspect_ratio = ((float)wnew) / (float)hnew;

                if (window_aspect_ratio > saved_aspect_ratio) {
                    int width = (int)(hnew * saved_aspect_ratio);
                    p_glViewport((wnew - width) / 2, 0, width, hnew);
                }
                else {
                    p_glViewport(0, 0, wnew, (int)(wnew / saved_aspect_ratio));
                }
            }
            else {
                p_glViewport(0, 0, wnew, hnew);
            }
        }
        return 0;
    }

    if (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        if (window == pygame_window) {
            SDL_Surface *sdl_surface = SDL_GetWindowSurface(window);
            Surface *old_surface = GetDefaultWindowSurface();
            if (sdl_surface != old_surface->surf) {
                old_surface->surf = sdl_surface;
            }
        }
    }
    return 0;
}


static int
_mac_display_init(void)
{
#if defined(__APPLE__) && defined(darwin)
    // TODO
#endif /* Mac */
    return 1;
}

static int _display_init()
{
    const char *drivername;
    /* Compatibility:
     * windib video driver was renamed in SDL2, and we don't want it to fail.
     */
    drivername = SDL_getenv("SDL_VIDEODRIVER");
    if (drivername &&
        !SDL_strncasecmp("windib", drivername, SDL_strlen(drivername)))
    {
        SDL_setenv("SDL_VIDEODRIVER", "windows", 1);
    }
    if (!SDL_WasInit(SDL_INIT_VIDEO))
    {
        if (!_mac_display_init())
            return 0;

        if (SDL_InitSubSystem(SDL_INIT_VIDEO))
            return 0;
    }
    return 1;
}

static int
_flip(_DisplayState *state)
{
    SDL_Window *win = GetDefaultWindow();
    int status = 0;

    /* Same check as VIDEO_INIT_CHECK() but returns -1 instead of NULL on
     * fail. */
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        ErrMsg("video system not initialized");
        return -1;
    }

    if (!win) {
        ErrMsg("Display mode not set");
        return -1;
    }

    if (state->using_gl) {
        SDL_GL_SwapWindow(win);
    }
    else {
        if (renderer != NULL) {
            SDL_Surface *screen =
                Surface_AsSurface(GetDefaultWindowSurface());
            SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
        else {
            /* Force a re-initialization of the surface in case it
             * has been resized to avoid "please call SDL_GetWindowSurface"
             * errors that the programmer cannot fix
             */
            Surface *screen = GetDefaultWindowSurface();
            SDL_Surface *new_surface = SDL_GetWindowSurface(win);

            if (new_surface != screen->surf) {
                screen->surf = new_surface;
            }
            status = SDL_UpdateWindowSurface(win);
        }
    }

    if (status < 0) {
        ErrMsg(SDL_GetError());
        return -1;
    }

    return 0;
}

static Surface *_display_resource(char *)
{
    // TODO
    return NULL;
}

Surface *display_set_mode_size(int width, int height)
{
    const char *const DefaultTitle = "pygame window";
    SDL_Window *win = GetDefaultWindow();
    Surface *surface = GetDefaultWindowSurface();
    SDL_Surface *surf = NULL;
    SDL_Surface *newownedsurf = NULL;
    int depth = 0;
    int flags = 0;
    int w, h;
    int vsync = SDL_FALSE;
    /* display will get overwritten by parameters only if display
       parameter is given. By default, put the new window on the same
       screen as the old one */
    int display = _get_display(win);
    if(!state) {
        state = malloc(sizeof(_DisplayState));
        if(!state) {
            ErrMsg("Failed to allocate display state");
            return NULL;
        }
        memset(state, 0, sizeof(_DisplayState));
    }
    char *title = state->title;
    char *scale_env;

    scale_env = SDL_getenv("PYGAME_FORCE_SCALE");

    if (scale_env != NULL) {
        flags |= PGS_SCALED;
        if (strcmp(scale_env, "photo") == 0) {
            SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "best",
                                    SDL_HINT_NORMAL);
        }
    }

    w = width;
    h = height;

    // Cannot set negative sized display mode
    if (w < 0 || h < 0)
    {
        return NULL;
    }

    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        /* note SDL works special like this too */
        if (!_display_init(NULL, NULL))
            return NULL;
    }

    state->using_gl = (flags & PGS_OPENGL) != 0;
    state->scaled_gl = state->using_gl && (flags & PGS_SCALED) != 0;

    if (state->scaled_gl) {
        // SCALED|OPENGL is experimental and subject to change
        return NULL;
    }

    if (!state->title) {
        state->title = malloc((strlen(DefaultTitle) + 1) * sizeof(char));
        if (!state->title)
            return NULL;
        strcpy(state->title, DefaultTitle);
        title = state->title;
    }

    /* set these only in toggle_fullscreen, clear on set_mode */
    state->toggle_windowed_w = 0;
    state->toggle_windowed_h = 0;

    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }

    SDL_DelEventWatch(ResizeEventWatch, state);


    {
        Uint32 sdl_flags = 0;
        SDL_DisplayMode display_mode;

        if (SDL_GetDesktopDisplayMode(display, &display_mode) != 0) {
            ErrMsg(SDL_GetError());
            return NULL;
        }

        if (w == 0 && h == 0 && !(flags & PGS_SCALED)) {
            /* We are free to choose a resolution in this case, so we can
           avoid changing the physical resolution. This used to default
           to the max supported by the monitor, but we can use current
           desktop resolution without breaking compatibility. */
            w = display_mode.w;
            h = display_mode.h;
        }

        if (flags & PGS_FULLSCREEN) {
            if (flags & PGS_SCALED) {
                sdl_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
            }
            // else if (w == display_mode.w && h == display_mode.h) {
            //     /* No need to change physical resolution.
            //    Borderless fullscreen is preferred when possible */
            //     sdl_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
            // }
            else {
                sdl_flags |= SDL_WINDOW_FULLSCREEN;
            }
        }

        if (flags & PGS_SCALED) {
            if (w == 0 || h == 0) {
                ErrMsg("Cannot set 0 sized SCALED display mode");
                return NULL;
            }
        }

        if (flags & PGS_OPENGL)
            sdl_flags |= SDL_WINDOW_OPENGL;
        if (flags & PGS_NOFRAME)
            sdl_flags |= SDL_WINDOW_BORDERLESS;
        if (flags & PGS_RESIZABLE) {
            sdl_flags |= SDL_WINDOW_RESIZABLE;
            if (state->auto_resize)
                SDL_AddEventWatch(ResizeEventWatch, state);
        }
        if (flags & PGS_SHOWN)
            sdl_flags |= SDL_WINDOW_SHOWN;
        if (flags & PGS_HIDDEN)
            sdl_flags |= SDL_WINDOW_HIDDEN;
        if (!(sdl_flags & SDL_WINDOW_HIDDEN))
            sdl_flags |= SDL_WINDOW_SHOWN;
        if (flags & PGS_OPENGL) {
            /* Must be called before creating context */
            if (flags & PGS_DOUBLEBUF) {
                flags &= ~PGS_DOUBLEBUF;
                SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
            }
            else
                SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
        }

#pragma PG_WARN(Not setting bpp ?)
#pragma PG_WARN(Add mode stuff.)
        {
            int w_1 = w, h_1 = h;
            int scale = 1;
            int center_window = 0;
            int x = SDL_WINDOWPOS_UNDEFINED_DISPLAY(display);
            int y = SDL_WINDOWPOS_UNDEFINED_DISPLAY(display);

            _get_video_window_pos(&x, &y, &center_window);
            if (center_window) {
                x = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
                y = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
            }

            if (win) {
                if (SDL_GetWindowDisplayIndex(win) == display) {
                    // fullscreen windows don't hold window x and y as needed
                    if (SDL_GetWindowFlags(win) &
                        (SDL_WINDOW_FULLSCREEN |
                         SDL_WINDOW_FULLSCREEN_DESKTOP)) {
                        x = state->fullscreen_backup_x;
                        y = state->fullscreen_backup_y;

                        // if the program goes into fullscreen first the "saved
                        // x and y" are "undefined position" that should be
                        // interpreted as a cue to center the window
                        if (x == (int)SDL_WINDOWPOS_UNDEFINED_DISPLAY(display))
                            x = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
                        if (y == (int)SDL_WINDOWPOS_UNDEFINED_DISPLAY(display))
                            y = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
                    }
                    else {
                        int old_w, old_h;
                        SDL_GetWindowSize(win, &old_w, &old_h);

                        /* Emulate SDL1 behaviour: When the window is to be
                         * centred, the window shifts to the new centred
                         * location only when resolution changes and previous
                         * position is retained when the dimensions don't
                         * change.
                         * When the window is not to be centred, previous
                         * position is retained unconditionally */
                        if (!center_window || (w == old_w && h == old_h)) {
                            SDL_GetWindowPosition(win, &x, &y);
                        }
                    }
                }
                if (!(flags & PGS_OPENGL) !=
                    !(SDL_GetWindowFlags(win) & SDL_WINDOW_OPENGL)) {
                    SetDefaultWindow(NULL);
                    win = NULL;
                }
            }

            if (flags & PGS_SCALED && !(flags & PGS_FULLSCREEN)) {
                SDL_Rect display_bounds;
                int fractional_scaling = SDL_FALSE;

                if (0 !=
                    SDL_GetDisplayUsableBounds(display, &display_bounds)) {
                    ErrMsg(SDL_GetError());
                    return NULL;
                }

                if (SDL_GetHintBoolean("SDL_HINT_RENDER_SCALE_QUALITY",
                                       SDL_FALSE))
                    fractional_scaling = SDL_TRUE;
                if (state->scaled_gl)
                    fractional_scaling = SDL_TRUE;

                if (fractional_scaling) {
                    float aspect_ratio = ((float)w) / (float)h;

                    w_1 = display_bounds.w;
                    h_1 = display_bounds.h;

                    if (((float)w_1) / (float)h_1 > aspect_ratio) {
                        w_1 = (int)(h_1 * aspect_ratio);
                    }
                    else {
                        h_1 = (int)(w_1 / aspect_ratio);
                    }
                }
                else {
                    int xscale, yscale;

                    xscale = display_bounds.w / w;
                    yscale = display_bounds.h / h;

                    scale = xscale < yscale ? xscale : yscale;

                    if (scale < 1)
                        scale = 1;

                    w_1 = w * scale;
                    h_1 = h * scale;
                }
            }

            // SDL doesn't preserve window position in fullscreen mode
            // However, windows coming out of fullscreen need these to go back
            // into the correct position
            if (sdl_flags &
                (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) {
                state->fullscreen_backup_x = x;
                state->fullscreen_backup_y = y;
            }

            if (!win) {
                /*open window*/

                char *window_id = SDL_getenv("SDL_WINDOWID");
                if (window_id) {
                    long long win_long = SDL_strtol(window_id, NULL, 0);
                    win = SDL_CreateWindowFrom((const void *)win_long);
                }
                else {
                    win = SDL_CreateWindow(title, x, y, w_1, h_1, sdl_flags);
                }

                if (!win) {
                    ErrMsg(SDL_GetError());
                    return NULL;
                }
            }
            else {
                /* set min size to (1,1) to erase any previously set min size
                 * relevant for windows leaving SCALED, which sets a min size
                 * only relevant on Windows, I believe.
                 * See https://github.com/pygame/pygame/issues/2327 */
                SDL_SetWindowMinimumSize(win, 1, 1);

                /* change existing window.
                 this invalidates the display surface*/
                SDL_SetWindowTitle(win, title);
                SDL_SetWindowSize(win, w_1, h_1);

                /* The window must be brought out of fullscreen before the
                 * resize/bordered/hidden changes due to SDL ignoring those
                 * changes if the window is fullscreen
                 * See https://github.com/pygame/pygame/issues/2711 */
                if (0 !=
                    SDL_SetWindowFullscreen(
                        win, sdl_flags & (SDL_WINDOW_FULLSCREEN |
                                          SDL_WINDOW_FULLSCREEN_DESKTOP))) {
                    ErrMsg(SDL_GetError());
                    return NULL;
                }

                SDL_SetWindowResizable(win, flags & PGS_RESIZABLE);
                SDL_SetWindowBordered(win, (flags & PGS_NOFRAME) == 0);

                if ((flags & PGS_SHOWN) || !(flags & PGS_HIDDEN))
                    SDL_ShowWindow(win);
                else if (flags & PGS_HIDDEN)
                    SDL_HideWindow(win);

                SDL_SetWindowPosition(win, x, y);

                assert(surface);
            }
        }

        if (state->using_gl) {
            if (!state->gl_context) {
                state->gl_context = SDL_GL_CreateContext(win);
                if (!state->gl_context) {
                    _display_state_cleanup(state);
                    ErrMsg(SDL_GetError());
                    goto DESTROY_WINDOW;
                }
                /* SDL_GetWindowSurface can not be used when using GL.
                According to https://wiki.libsdl.org/SDL_GetWindowSurface

                So we make a fake surface.
                */
                surf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32,
                                            0xff << 16, 0xff << 8, 0xff, 0);
                newownedsurf = surf;
            }
            else {
                surf = Surface_AsSurface(surface);
            }
            if (flags & PGS_SCALED) {
                state->scaled_gl_w = w;
                state->scaled_gl_h = h;
            }

            /* Even if this succeeds, we can never *really* know if vsync
               actually works. There may be screen tearing, blocking double
               buffering, triple buffering, render-offloading where the driver
               for the on-board graphics *doesn't* have vsync enabled, or cases
               where the driver lies to us because the user has configured
               vsync to be always on or always off, or vsync is on by default
               for the whole desktop because of wayland GL compositing. */
            if (vsync == -1) {
                if (SDL_GL_SetSwapInterval(-1) != 0) {
                    ErrMsg("adaptive vsync for OpenGL not available");
                    _display_state_cleanup(state);
                    goto DESTROY_WINDOW;
                }
            }
            else {
                if (vsync == 1) {
                    if (SDL_GL_SetSwapInterval(1) != 0) {
                        ErrMsg("regular vsync for OpenGL not available");
                        _display_state_cleanup(state);
                        goto DESTROY_WINDOW;
                    }
                }
                else {
                    SDL_GL_SetSwapInterval(0);
                }
            }
        }
        else {
            if (state->gl_context) {
                SDL_GL_DeleteContext(state->gl_context);
                state->gl_context = NULL;
            }

            if (flags & PGS_SCALED) {
                if (renderer == NULL) {
                    SDL_RendererInfo info;

                    SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY,
                                            "nearest", SDL_HINT_DEFAULT);

                    if (vsync) {
                        renderer = SDL_CreateRenderer(
                            win, -1, SDL_RENDERER_PRESENTVSYNC);
                    }
                    else {
                        renderer = SDL_CreateRenderer(win, -1, 0);
                    }

                    if (renderer == NULL) {
                        ErrMsg("failed to create renderer");
                        return NULL;
                    }

                    /* use whole screen with uneven pixels on fullscreen,
                       exact scale otherwise.
                       we chose the window size for this to work */
                    SDL_RenderSetIntegerScale(
                        renderer,
                        !(flags & PGS_FULLSCREEN ||
                          SDL_GetHintBoolean("SDL_HINT_RENDER_SCALE_QUALITY",
                                             SDL_FALSE)));
                    SDL_RenderSetLogicalSize(renderer, w, h);
                    /* this must be called after creating the renderer!*/
                    SDL_SetWindowMinimumSize(win, w, h);

                    SDL_GetRendererInfo(renderer, &info);
                    if (vsync && !(info.flags & SDL_RENDERER_PRESENTVSYNC)) {
                        ErrMsg("could not enable vsync");
                        _display_state_cleanup(state);
                        goto DESTROY_WINDOW;
                    }
                    if (!(info.flags & SDL_RENDERER_ACCELERATED)) {
                        ErrMsg("no fast renderer available");
                        _display_state_cleanup(state);
                        goto DESTROY_WINDOW;
                    }

                    texture = SDL_CreateTexture(
                        renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, w, h);
                }
                surf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32,
                                            0xff << 16, 0xff << 8, 0xff, 0);
                newownedsurf = surf;
            }
            else {
                surf = SDL_GetWindowSurface(win);
            }
        }
        if (state->gamma_ramp) {
            int result = SDL_SetWindowGammaRamp(win, state->gamma_ramp,
                                                state->gamma_ramp + 256,
                                                state->gamma_ramp + 512);
            if (result) /* SDL Error? */
            {
                /* Discard a possibly faulty gamma ramp. */
                _display_state_cleanup(state);

                /* Recover error, then destroy the window */
                ErrMsg(SDL_GetError());
                goto DESTROY_WINDOW;
            }
        }

        if (state->using_gl && renderer != NULL) {
            _display_state_cleanup(state);
            ErrMsg("GL context and SDL_Renderer created at the same time");
            goto DESTROY_WINDOW;
        }

        if (!surf) {
            _display_state_cleanup(state);
            ErrMsg(SDL_GetError());
            goto DESTROY_WINDOW;
        }
        if (!surface) {
            surface = Surface_New2(surf, newownedsurf != NULL);
        }
        else {
            Surface_SetSurface(surface, surf, newownedsurf != NULL);
        }
        if (!surface) {
            if (newownedsurf)
                SDL_FreeSurface(newownedsurf);
            _display_state_cleanup(state);
            goto DESTROY_WINDOW;
        }

        /*no errors; make the window available*/
        SetDefaultWindow(win);
        SetDefaultWindowSurface(surface);

        /* ensure window is always black after a set_mode call */
        SDL_FillRect(surf, NULL, SDL_MapRGB(surf->format, 0, 0, 0));
        _flip(state);
    }

    /*set the window icon*/
    if (!state->icon) {
        state->icon = _display_resource(icon_defaultname);
        if (!state->icon)
            ErrMsg("could not display icon");
        else if (icon_colorkey != -1) {
            SDL_SetColorKey(Surface_AsSurface(state->icon), SDL_TRUE,
                            icon_colorkey);
        }
    }
    if (state->icon)
        SDL_SetWindowIcon(win, Surface_AsSurface(state->icon));

    /*probably won't do much, but can't hurt, and might help*/
    SDL_PumpEvents();

    return surface;

DESTROY_WINDOW:

    if (win == GetDefaultWindow())
        SetDefaultWindow(NULL);
    else if (win)
        SDL_DestroyWindow(win);
    return NULL;
}

Surface *display_set_mode()
{
    return display_set_mode_size(0, 0);
}

int
display_flip()
{
    if (_flip(state) < 0) {
        return -1;
    }
    return 0;
}

/*BAD things happen when out-of-bound rects go to updaterect*/
static SDL_Rect *
_screencroprect(SDL_Rect *r, int w, int h, SDL_Rect *cur)
{
    if (r->x > w || r->y > h || (r->x + r->w) <= 0 || (r->y + r->h) <= 0)
        return 0;

    int right = MIN(r->x + r->w, w);
    int bottom = MIN(r->y + r->h, h);
    cur->x = (short)MAX(r->x, 0);
    cur->y = (short)MAX(r->y, 0);
    cur->w = (unsigned short)right - cur->x;
    cur->h = (unsigned short)bottom - cur->y;
    return cur;
}

static int _display_update(SDL_Rect *rect, SDL_Rect *rect_list, int rect_count)
{
    SDL_Window *win = GetDefaultWindow();
    SDL_Rect *gr;
    int wide, high;

    VIDEO_INIT_CHECK_C();

    if (!win) {
        ErrMsg("Display mode not set");
        return -1;
    }

    if (renderer != NULL) {
        return display_flip();
    }
    SDL_GetWindowSize(win, &wide, &high);

    if (state->using_gl) {
        ErrMsg("Cannot update an OPENGL display");
        return -1;
    }

    /*determine type of argument we got*/
    if (!rect) {
        return display_flip();
    }

    gr = rect;
    if (gr) {
        SDL_Rect sdlr;

        if (_screencroprect(gr, wide, high, &sdlr))
            SDL_UpdateWindowSurfaceRects(win, &sdlr, 1);
    }
    else {
        size_t loop, num;
        int count;
        SDL_Rect *rects;
        if (rect && rect_list) {
            ErrMsg("update requires a rectstyle or sequence of rectstyles");
            return -1;
        }
        if (!rect_list) {
            ErrMsg("update requires a rectstyle or sequence of rectstyles");
            return -1;
        }

        num = rect_count;
        rects = malloc(sizeof(SDL_Rect) * rect_count);
        if (!rects)
            return -1;
        count = 0;
        for (loop = 0; loop < num; ++loop) {
            SDL_Rect *cur_rect = (rects + count);

            /*get rect from the sequence*/
            gr = (rect_list + loop);
            if (!gr) {
                continue;
            }

            if (gr->w < 1 && gr->h < 1)
                continue;

            /*bail out if rect not onscreen*/
            if (!_screencroprect(gr, wide, high, cur_rect))
                continue;

            ++count;
        }

        if (count) {
            SDL_UpdateWindowSurfaceRects(win, rects, count);
        }

        free(rects);
    }
    return 0;
}

int display_update()
{
    return _display_update(NULL, NULL, 0);
}

int display_update_rect(SDL_Rect *rect)
{
    return _display_update(rect, NULL, 0);
}

int display_update_rectlist(SDL_Rect *rect_list, int rect_count)
{
    return _display_update(NULL, rect_list, rect_count);
}