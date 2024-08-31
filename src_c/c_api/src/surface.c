#include <pygame_c_api.h>

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

    if (Surface_SetSurface(self, s, owner) < 0)
        return NULL;

    return self;
}

Surface *
Surface_New2(SDL_Surface *s, int owner)
{
    return surf_subtype_new(s, owner);
}