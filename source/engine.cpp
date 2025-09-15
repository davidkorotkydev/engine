#include "engine.hpp"

void Engine::initialize()
{
    create_window();
    create_instance();
    create_debug_utils_messenger();
    pick_physical_device();
    create_logical_device();
}

void Engine::create_window()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags flags{SDL_WINDOW_VULKAN};
    p_window = SDL_CreateWindow("Hello, world.", window_extent.width, window_extent.height, flags);
    if (p_window == nullptr) throw std::runtime_error("The window could not be created.\n" + std::string(SDL_GetError()) + "\n");
}

void Engine::create_instance()
{
    if (validation_layers_enabled && !validation_layers_supported()) throw std::runtime_error("The requested validation layers are not supported.");

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
    /* [[.](https://wiki.libsdl.org/SDL3/SDL_Vulkan_GetInstanceExtensions)] */
    Uint32 instance_extension_count;
    const char *const *instance_extensions{SDL_Vulkan_GetInstanceExtensions(&instance_extension_count)};
    if (instance_extensions == nullptr) throw std::runtime_error("The required Vulkan instance extensions could not be found.\n");
    /* Add one for `VK_EXT_debug( ... )` [[.](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_debug_utils.html)]. */
    Uint32 extension_count{instance_extension_count + 1};
#ifdef __APPLE__
    flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    /* Add one for `VK_KHR_portability_enumeration` [[.](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_portability_enumeration.html)]. */
    extension_count += 1;
    /* Add one for `VK_KHR_get_physical( ... )2` [[.](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_get_physical_device_properties2.html)]. `VK_KHR_get_physical( ... )2` is a dependency of the `VK_KHR_portability_subset` device extension, which is required by `vkCreateDevice` on macOS. */
    extension_count += 1;
#endif /* __APPLE__ */
    const char **extension_names{(const char **)SDL_malloc(extension_count * sizeof(const char *))};
    /* Add this to the start. */
    extension_names[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#ifdef __APPLE__
    /* Add to the end. */
    extension_names[extension_count - 1] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    extension_names[extension_count - 1 - 1] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
#endif /* __APPLE__ */
    SDL_memcpy(&extension_names[1], instance_extensions, instance_extension_count * sizeof(const char *));
    // for (int i{0}; i < instance_extension_count; i++) fprintf(stdout, "%s\n", extension_names[i]);

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

    if (validation_layers_enabled)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    }

    CHECK(vkCreateInstance(&create_info, nullptr, &instance));
    SDL_free(extension_names);
}

bool Engine::validation_layers_supported()
{
    uint32_t count;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> properties(count);
    vkEnumerateInstanceLayerProperties(&count, properties.data());
    // for (int i{0}; i < count; i++) fprintf(stdout, "%s\n", properties[i].layerName);

    for (const auto &validation_layer : validation_layers)
    {
        bool found{false};

        for (const auto &property : properties)
        {
            if (strcmp(validation_layer, property.layerName) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found) return false;
    }

    return true;
}

void Engine::create_debug_utils_messenger()
{
}

void Engine::pick_physical_device()
{
    uint32_t count{0};
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0) throw std::runtime_error("No physical device with Vulkan support could be found.");
    std::vector<VkPhysicalDevice> physical_devices(count);
    vkEnumeratePhysicalDevices(instance, &count, physical_devices.data());

    for (const auto &physical_device : physical_devices)
    {
        if (physical_device_suitable(physical_device))
        {
            this->physical_device = physical_device;
            break;
        }
    }

    if (this->physical_device == VK_NULL_HANDLE) throw std::runtime_error("A suitable physical device could not be found.");
}

bool Engine::physical_device_suitable(VkPhysicalDevice physical_device)
{
    Queue_Family_Indices indices{find_queue_families(physical_device)};
    return indices.is_complete();
}

Engine::Queue_Family_Indices Engine::find_queue_families(VkPhysicalDevice physical_device)
{
    Queue_Family_Indices indices;
    uint32_t count{0};
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, properties.data());
    int i{0};

    for (const auto &property : properties)
    {
        if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphics_family = i;
        if (indices.is_complete()) break;
        i++;
    }

    return indices;
}

void Engine::create_logical_device()
{
    Queue_Family_Indices indices{find_queue_families(physical_device)};

    VkDeviceQueueCreateInfo queue_create_info{
        .sType{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO},
        // .pNext{},
        // .flags{},
        .queueFamilyIndex{indices.graphics_family.value()},
        .queueCount{1},
        // .pQueuePriorities{},
    };

    float queue_priority{1.0f};
    queue_create_info.pQueuePriorities = &queue_priority;
    std::vector<const char *> extension_names{};
#ifdef __APPLE__
    /* [2025-9-15 11:07 AM ET] `VK_KHR_PORTABILITY_SUBSET( ... )` is currently defined in `vulkan_beta.h` (which can be used from `vulkan.h` by defining `VK_ENABLE_BETA_EXTENSIONS`). For now, we define the `VK_KHR_( ... )` macro below to avoid depending on beta. `VK_KHR_portability_subset` [[.](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_portability_subset.html)] is a _device_ extension that depends on the _instance_ extension `VK_KHR_get_physical( ... )2` enabled during instance creation. It is required by `vkCreateDevice` on macOS. */
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
    extension_names.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif /* __APPLE__ */
    VkPhysicalDeviceFeatures enabled_features{};

    VkDeviceCreateInfo create_info{
        .sType{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO},
        // .pNext{},
        // .flags{},
        .queueCreateInfoCount{1},
        .pQueueCreateInfos{&queue_create_info},
        /* `enabledLayerCount` is deprecated and should not be used. */
        // .enabledLayerCount{},
        /* `ppEnabledLayerNames` is deprecated and should not be used. */
        // .ppEnabledLayerNames{},
        .enabledExtensionCount{static_cast<uint32_t>(extension_names.size())},
        .ppEnabledExtensionNames{extension_names.data()},
        .pEnabledFeatures{&enabled_features},
    };

    if (validation_layers_enabled)
    {
        /* Both `enabledLayerCount` and `ppEnabledLayerNames` are deprecated. We set them for compatibility with older versions of Vulkan. */
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    }

    CHECK(vkCreateDevice(physical_device, &create_info, nullptr, &device));
    vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
}

void Engine::draw()
{
}

void Engine::event(SDL_Event *p_event)
{
}

void Engine::clean()
{
    /* Device queues are destroyed when the device is destroyed. */
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(p_window);
}
