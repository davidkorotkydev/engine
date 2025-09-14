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

    VkInstanceCreateFlags flags{};
    /* - [[.](https://wiki.libsdl.org/SDL3/SDL_Vulkan_GetInstanceExtensions)] */
    Uint32 instance_extension_count;
    const char *const *instance_extensions{SDL_Vulkan_GetInstanceExtensions(&instance_extension_count)};
    if (instance_extensions == nullptr) throw std::runtime_error("The required Vulkan instance extensions could not be found.\n");
    /* Add one for `VK_EXT_debug( ... )` [[.](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_debug_utils.html)]. */
    Uint32 extension_count{instance_extension_count + 1};
#ifdef __APPLE__
    flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    /* Add one for `VK_KHR_portability_enumeration` [[.](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_portability_enumeration.html)]. */
    extension_count += 1;
#endif /* __APPLE__ */
    const char **extension_names{(const char **)SDL_malloc(extension_count * sizeof(const char *))};
    /* Add this to the start. */
    extension_names[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#ifdef __APPLE__
    /* Add to the end. */
    extension_names[extension_count - 1] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
#endif /* __APPLE__ */
    SDL_memcpy(&extension_names[1], instance_extensions, instance_extension_count * sizeof(const char *));
    for (int i{0}; i < instance_extension_count; i++) fprintf(stdout, "%s\n", extension_names[i]);

    VkInstanceCreateInfo create_info{
        .sType{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO},
        // .pNext{},
        .flags{flags},
        .pApplicationInfo{&application_info},
        // .enabledLayerCount{},
        // .ppEnabledLayerNames{},
        .enabledExtensionCount{extension_count},
        .ppEnabledExtensionNames{extension_names},
    };

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
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(p_window);
}
