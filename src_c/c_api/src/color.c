#include <pygame_c_api.h>

int
_RGBAFromColorObj(Color *color, Uint8 rgba[])
{
    if (color) {
        rgba[0] = color->data[0];
        rgba[1] = color->data[1];
        rgba[2] = color->data[2];
        rgba[3] = color->data[3];
        return 1;
    }

    /* Default action */
    return RGBAFromObj(color, rgba);
}

int _RGBA32FromColorObj(Color *color, Uint32 *rgba)
{
    *rgba = 0;
    if (color) {
        *rgba |= (Uint8)(color->data[0] << 24);
        *rgba |= (Uint8)(color->data[1] << 16);
        *rgba |= (Uint8)(color->data[2] << 8);
        *rgba |= (Uint8)color->data[3];
        return 0;
    }
    return 1;
}

int
_get_color(Color *val, Uint32 *color)
{
    if (!val || !color) {
        return 0;
    }

    if (val) {
        unsigned long longval;
        if (_RGBA32FromColorObj(val, &longval) || (longval > 0xFFFFFFFF)) {
            ErrMsg("invalid color argument");
            return 0;
        }
        *color = (Uint32)longval;
        return 1;
    }

    /* Failed */
    ErrMsg("invalid color argument");
    return 0;
}

int
_parse_color_from_single_object(Color *obj, Uint8 *rgba)
{
    /* At this point color is either tuple-like or a single integer. */
    if (!_RGBAFromColorObj(obj, rgba)) {
        /* Color is not a valid tuple-like. */
        Uint32 color;

        if (_get_color(obj, &color)) {
            /* Color is a single integer. */
            rgba[0] = (Uint8)(color >> 24);
            rgba[1] = (Uint8)(color >> 16);
            rgba[2] = (Uint8)(color >> 8);
            rgba[3] = (Uint8)color;
        }
        else {
            /* Exception already set by _get_color(). */
            return -1;
        }
    }
    return 0;
}

int
RGBAFromFuzzyColorObj(Color *color, Uint8 rgba[])
{
    return _parse_color_from_single_object(color, rgba) == 0;
}
