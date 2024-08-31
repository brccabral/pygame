#include <pygame_c_api.h>


#if !SDL_VERSION_ATLEAST(2, 0, 10)
static Uint32
pg_map_rgb(SDL_Surface *surf, Uint8 r, Uint8 g, Uint8 b)
{
    /* SDL_MapRGB() returns wrong values for color keys
       for indexed formats since since alpha = 0 */
    Uint32 key;
    if (!surf->format->palette)
        return SDL_MapRGB(surf->format, r, g, b);
    if (!SDL_GetColorKey(surf, &key)) {
        Uint8 keyr, keyg, keyb;
        SDL_GetRGB(key, surf->format, &keyr, &keyg, &keyb);
        if (r == keyr && g == keyg && b == keyb)
            return key;
    }
    else
        SDL_ClearError();
    return SDL_MapRGBA(surf->format, r, g, b, SDL_ALPHA_OPAQUE);
}

static Uint32
pg_map_rgba(SDL_Surface *surf, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    if (!surf->format->palette)
        return SDL_MapRGBA(surf->format, r, g, b, a);
    return pg_map_rgb(surf, r, g, b);
}
#else /* SDL_VERSION_ATLEAST(2, 0, 10) */
#define pg_map_rgb(surf, r, g, b) SDL_MapRGB((surf)->format, (r), (g), (b))
#define pg_map_rgba(surf, r, g, b, a) \
    SDL_MapRGBA((surf)->format, (r), (g), (b), (a))
#endif /* SDL_VERSION_ATLEAST(2, 0, 10) */


/* surface object internals */
void
surface_cleanup(Surface *self)
{
    if (self->surf && self->owner) {
        SDL_FreeSurface(self->surf);
        self->surf = NULL;
    }
    if (self->subsurface) {
        free(self->subsurface);
        self->subsurface = NULL;
    }
    if (self->dependency) {
        self->dependency = NULL;
    }
    if (self->locklist) {
        self->locklist = NULL;
    }
    self->owner = 0;
}

int
Surface_SetSurface(Surface *self, SDL_Surface *s, int owner)
{
    if (!s) {
        ErrMsg(SDL_GetError());
        return -1;
    }
    if(!self) {
        return -1;
    }
    if (s == self->surf) {
        self->owner = owner;
        return 0;
    }

    surface_cleanup(self);
    self->surf = s;
    self->owner = owner;
    return 0;
}

Surface *
surf_subtype_new(SDL_Surface *s, int owner)
{
    Surface *self;

    if (!s) {
        ErrMsg(SDL_GetError());
        return NULL;
    }

    self = (Surface *)malloc(sizeof(Surface));
    memset(self, 0, sizeof(Surface));

    if (Surface_SetSurface(self, s, owner) < 0)
        return NULL;

    return self;
}

Surface *
Surface_New2(SDL_Surface *s, int owner)
{
    return surf_subtype_new(s, owner);
}

int
Surface_fill(Surface *self, Color *color, SDL_Rect *rect, int special_flags, SDL_Rect *drawn_area)
{
    SDL_Surface *surf = Surface_AsSDLSurface(self);

    if (!surf) {
        ErrMsg("display Surface quit");
        goto error;
    }

    SDL_Rect temp_drawn_area, temp;
    int result;
    Color *rgba_obj = color;
    Uint8 rgba[4];
    int blendargs = 0;
    Uint32 colorRGBA = 0;


    if (RGBAFromFuzzyColorObj(rgba_obj, rgba))
        colorRGBA = pg_map_rgba(surf, rgba[0], rgba[1], rgba[2], rgba[3]);
    else
        goto error; /* pg_RGBAFromFuzzyColorObj set an exception for us */

    if (!rect) {
        rect = &temp;
        temp.x = temp.y = 0;
        temp.w = surf->w;
        temp.h = surf->h;
    }

    /* we need a fresh copy so our Rect values don't get munged */
    if (rect != &temp) {
        memcpy(&temp, rect, sizeof(temp));
        rect = &temp;
    }

    if (rect->w < 0 || rect->h < 0 || rect->x > surf->w || rect->y > surf->h) {
        temp_drawn_area.x = temp_drawn_area.y = 0;
        temp_drawn_area.w = temp_drawn_area.h = 0;
    }
    else {
        temp_drawn_area.x = rect->x;
        temp_drawn_area.y = rect->y;
        temp_drawn_area.w = rect->w;
        temp_drawn_area.h = rect->h;

        // clip the rect to be within the surface.
        if (temp_drawn_area.x + temp_drawn_area.w <= 0 || temp_drawn_area.y + temp_drawn_area.h <= 0) {
            temp_drawn_area.w = 0;
            temp_drawn_area.h = 0;
        }

        if (temp_drawn_area.x < 0) {
            temp_drawn_area.x = 0;
        }
        if (temp_drawn_area.y < 0) {
            temp_drawn_area.y = 0;
        }

        if (temp_drawn_area.x + temp_drawn_area.w > surf->w) {
            temp_drawn_area.w = temp_drawn_area.w + (surf->w - (temp_drawn_area.x + temp_drawn_area.w));
        }
        if (temp_drawn_area.y + temp_drawn_area.h > surf->h) {
            temp_drawn_area.h = temp_drawn_area.h + (surf->h - (temp_drawn_area.y + temp_drawn_area.h));
        }

        if (temp_drawn_area.w <= 0 || temp_drawn_area.h <= 0) {
            goto error;
        }

        if (blendargs != 0) {
            result = surface_fill_blend(surf, &temp_drawn_area, colorRGBA, blendargs);
        }
        else {
            Surface_Prep(self);
            Surface_Lock((Surface *)self);
            result = SDL_FillRect(surf, &temp_drawn_area, colorRGBA);
            Surface_Unlock((Surface *)self);
            Surface_Unprep(self);
        }
        if (result == -1) {
            ErrMsg(SDL_GetError());
            goto error;
        }
    }

    if (drawn_area) {
        memcpy(&temp_drawn_area, drawn_area, sizeof(temp_drawn_area));
    }
    return 0;
error:
    if (drawn_area) {
        memset(drawn_area, 0, sizeof(*drawn_area));
    }
    return -1;
}
