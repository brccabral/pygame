#include <pygame_c_api.h>

int main()
{
    init();

    Surface *surface = display_set_mode_size(100, 100);

    SDL_Event events[10];
    int len;

    while (1)
    {
        while((len = event_get(events, 10)) > 0)
        {
            for(int e = 0; e < len; ++e)
            {
                SDL_Event event = events[e];
                if(event.type == SDL_QUIT)
                {
                    quit();
                    return 0;
                }
            }
        }
        display_update();
    }
    quit();
}