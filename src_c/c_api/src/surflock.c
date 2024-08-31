#include <pygame_c_api.h>


void
Surface_Prep(Surface *surfobj)
{
    SubSurface_Data *data = ((Surface *)surfobj)->subsurface;
    if (data != NULL) {
        SDL_Surface *surf = Surface_AsSDLSurface(surfobj);
        SDL_Surface *owner = Surface_AsSDLSurface(data->owner);
        Surface_LockBy((Surface *)data->owner, (Surface *)surfobj);
        surf->pixels = ((char *)owner->pixels) + data->pixeloffset;
    }
}

void
Surface_Unprep(Surface *surfobj)
{
    SubSurface_Data *data = ((Surface *)surfobj)->subsurface;
    if (data != NULL) {
        Surface_UnlockBy((Surface *)data->owner,
                           (Surface *)surfobj);
    }
}

int
Surface_Lock(Surface *surfobj)
{
    return Surface_LockBy(surfobj, (Surface *)surfobj);
}

int
Surface_Unlock(Surface *surfobj)
{
    return Surface_UnlockBy(surfobj, (Surface *)surfobj);
}

int
Surface_LockBy(Surface *surfobj, Surface *lockobj)
{
    Surface *surf = (Surface *)surfobj;

    Surface *current_lock = surf;
    while (current_lock->locklist) {
        current_lock = current_lock->locklist;
    }
    current_lock->locklist = lockobj;

    if (surf->subsurface != NULL) {
        Surface_Prep(surfobj);
    }
    if (SDL_LockSurface(surf->surf) == -1) {
        ErrMsg("error locking surface");
        return 0;
    }
    return 1;
}

int
Surface_UnlockBy(Surface *surfobj, Surface *lockobj)
{
    Surface *surf = (Surface *)surfobj;
    int found = 0;
    int noerror = 1;

    if (surf->locklist != NULL) {
        Surface *item, *prev;
        prev = surf;
        item = (Surface *)surf->locklist;
        while (item && item != lockobj) {
            prev = item;
            item = (Surface *)item->locklist;
            if(item == lockobj) {
                found = 1;
            }
        }
        if(found && item) {
            prev->locklist = item->locklist;
            SDL_FreeSurface(item->surf);
            free(item);
        }
    }

    if (!found) {
        return noerror;
    }

    /* Release all found locks. */
    while (found > 0) {
        if (surf->surf != NULL) {
            SDL_UnlockSurface(surf->surf);
        }
        if (surf->subsurface != NULL) {
            Surface_Unprep(surfobj);
        }
        found--;
    }

    return noerror;
}
