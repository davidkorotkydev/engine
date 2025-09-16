#pragma once

/*
    - `std::clamp` [[.](https://en.cppreference.com/w/cpp/algorithm/clamp.html)]
*/
#include <algorithm>
/*
    - `std::optional` [[.](https://en.cppreference.com/w/cpp/utility/optional.html)]
*/
#include <optional>
/*
    - `std::set` [[.](https://en.cppreference.com/w/cpp/container/set.html)]
*/
#include <set>
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
    struct Queue_Family_Indices
    {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;
        bool completed() { return graphics_family.has_value() && present_family.has_value(); }
    };

    struct Swapchain_Support
    {
        VkSurfaceCapabilitiesKHR surface_capabilities;
        std::vector<VkPresentModeKHR> present_modes;
        std::vector<VkSurfaceFormatKHR> surface_formats;
    };

    /* The sections below are ordered by call, except where noted. */

    /* # `initialize` # */

    void create_window();
    /* * */ SDL_Window *p_window{nullptr};
    /* * */ VkExtent2D window_extent{512 * 2, 342 * 2};
    void create_instance();
    /* * */ VkInstance instance;
#ifdef NDEBUG
    /* * */ const bool validation_layers_enabled{false};
#else
    /* * */ const bool validation_layers_enabled{true};
#endif
    /* * */ bool query_validation_layer_support();
    /* * */ /* * */ const std::vector<const char *> validation_layers{"VK_LAYER_KHRONOS_validation"};
    void create_debug_utils_messenger();
    void create_surface();
    /* * */ VkSurfaceKHR surface;
    void choose_physical_device();
    /* * */ VkPhysicalDevice physical_device{VK_NULL_HANDLE};
    /* * */ bool physical_device_suitable(VkPhysicalDevice);
    /* * */ /* * */ Queue_Family_Indices find_queue_families(VkPhysicalDevice);
    /* * */ /* * */ bool query_extension_support(VkPhysicalDevice);
    /* * */ /* * */ /* * */ std::vector<const char *> device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    /* * */ /* * */ Swapchain_Support query_swapchain_support(VkPhysicalDevice);
    void create_logical_device();
    /* * */ VkDevice device;
    /* * */ VkQueue graphics_queue;
    /* * */ VkQueue present_queue;
    void create_swapchain();
    /* * */ VkExtent2D swapchain_extent;
    /* * */ VkFormat swapchain_image_format;
    /* * */ VkSwapchainKHR swapchain;
    /* * */ std::vector<VkImage> swapchain_images;
    /* * */ VkSurfaceFormatKHR choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &);
    /* * */ VkPresentModeKHR choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> &);
    /* * */ VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR &);
    void create_image_views();
    /* * */ std::vector<VkImageView> swapchain_image_views;
};
