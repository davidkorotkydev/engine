#pragma once

/*
    - `std::runtime_error` [[.](https://en.cppreference.com/w/cpp/error/runtime_error.html)]
*/
#include <stdexcept>
/*
    - `std::vector` [[.](https://en.cppreference.com/w/cpp/container/vector.html)]
*/
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "common.hpp"

class Engine
{
    public:
        /* These are are ordered by call. */

        void initialize();
        void draw();
        void event(SDL_Event *p_event);
        void clean();

    private:
        /* The sections below are ordered by call, except where noted. */

        /* # `initialize` # */

        void create_window();
        /* * */ SDL_Window *p_window{nullptr};
        /* * */ VkExtent2D window_extent{512 * 2, 342 * 2};
        void create_instance();
        /* * */ VkInstance instance;
};
