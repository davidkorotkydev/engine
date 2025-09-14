#include "engine.hpp"

void Engine::initialize()
{
    create_window();
    create_instance();
}

void Engine::create_window()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags{SDL_WINDOW_VULKAN};
    p_window = SDL_CreateWindow("Hello, world.", window_extent.width, window_extent.height, window_flags);
    if (p_window == nullptr) throw std::runtime_error("The window could not be created.\n" + std::string(SDL_GetError()) + "\n");
}

void Engine::create_instance()
{
    VkApplicationInfo application_info{
        .sType{VK_STRUCTURE_TYPE_APPLICATION_INFO},
        // .pNext{},
        .pApplicationName{"engine"},
        .applicationVersion{VK_MAKE_VERSION(0, 0, 0)},
        // .pEngineName{},
        // .engineVersion{},
        .apiVersion{VK_API_VERSION_1_0},
    };

    VkInstanceCreateInfo create_info{
        .sType{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO},
        // .pNext{},
        // .flags{},
        .pApplicationInfo{&application_info},
        // .enabledLayerCount{},
        // .ppEnabledLayerNames{},
        // .enabledExtensionCount{},
        // .ppEnabledExtensionNames{},
    };

    /* - [[.](https://wiki.libsdl.org/SDL3/SDL_Vulkan_GetInstanceExtensions)] */
    Uint32 instance_extension_count;
    const char *const *instance_extensions{SDL_Vulkan_GetInstanceExtensions(&instance_extension_count)};
    if (instance_extensions == nullptr) throw std::runtime_error("The required Vulkan instance extensions could not be found.\n");
    /* Add one for `VK_EXT_DEBUG_( ... )`. */
    Uint32 extension_count{instance_extension_count + 1};
    const char **extension_names{(const char **)SDL_malloc(extension_count * sizeof(const char *))};
    /* - [[.](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_debug_utils.html)] */
    extension_names[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    SDL_memcpy(&extension_names[1], instance_extensions, instance_extension_count * sizeof(const char *));
    for (int i{0}; i < instance_extension_count; i++) fprintf(stdout, "%s\n", extension_names[i]);
    create_info.enabledExtensionCount = extension_count;
    create_info.ppEnabledExtensionNames = extension_names;
    CHECK(vkCreateInstance(&create_info, nullptr, &instance));
    SDL_free(extension_names);
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
