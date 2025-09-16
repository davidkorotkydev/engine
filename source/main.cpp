#include "main.hpp"

static Engine engine;

/* This runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    try
    {
        engine.initialize();
    }
    catch (const std::exception &exception)
    {
        fprintf(stderr, "%s", exception.what());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

/* The heart of the program, this runs once each frame. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    engine.draw();
    return SDL_APP_CONTINUE;
}

/* This runs when a new event occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    /* This ends the program, reporting success to the OS. */
    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;
    engine.event(event);
    return SDL_APP_CONTINUE;
}

/* This runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    engine.clean();
}
