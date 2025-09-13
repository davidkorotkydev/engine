#include "engine.hpp"

void Engine::initialize()
{
    initialize_sdl();
}

void Engine::initialize_sdl()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags{SDL_WINDOW_VULKAN};
    p_window = SDL_CreateWindow("Hello, world.", window_extent.width, window_extent.height, window_flags);
    if (p_window == nullptr) throw std::runtime_error("The window could not be created.\n" + std::string(SDL_GetError()) + "\n");
}

void Engine::draw()
{
}

void Engine::event(SDL_Event *p_event)
{
}

void Engine::clean()
{
}
