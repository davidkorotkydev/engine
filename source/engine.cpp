#include "engine.hpp"

void Engine::initialize()
{
    create_window();
    create_instance();
    create_debug_utils_messenger();
    create_surface();
    choose_physical_device();
    create_logical_device();
    create_swapchain();
    create_image_views();
    create_render_pass();
    create_graphics_pipeline();
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
    if (validation_layers_enabled && !query_validation_layer_support()) throw std::runtime_error("The requested validation layers are not supported.\n");

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

bool Engine::query_validation_layer_support()
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

void Engine::create_surface()
{
    /* [[.](https://wiki.libsdl.org/SDL3/SDL_Vulkan_CreateSurface)] */
    if (!SDL_Vulkan_CreateSurface(p_window, instance, nullptr, &surface)) throw std::runtime_error("The window surface could not be created.\n");
}

void Engine::choose_physical_device()
{
    uint32_t count{0};
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0) throw std::runtime_error("No physical device with Vulkan support could be found.\n");
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

    if (this->physical_device == VK_NULL_HANDLE) throw std::runtime_error("A suitable physical device could not be found.\n");
}

bool Engine::physical_device_suitable(VkPhysicalDevice physical_device)
{
    Queue_Family_Indices indices{find_queue_families(physical_device)};
    bool extensions_supported{query_extension_support(physical_device)};
    bool swapchain_adequate{false};

    if (extensions_supported)
    {
        Swapchain_Support support{query_swapchain_support(physical_device)};
        swapchain_adequate = !support.surface_formats.empty() && !support.present_modes.empty();
    }

    return indices.completed() && extensions_supported && swapchain_adequate;
}

Engine::Queue_Family_Indices Engine::find_queue_families(VkPhysicalDevice physical_device)
{
    Queue_Family_Indices indices;
    uint32_t count{0};
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, properties.data());
    int index{0};

    for (const auto &property : properties)
    {
        if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphics_family = index;
        VkBool32 supported{false};
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, index, surface, &supported);
        if (supported) indices.present_family = index;
        if (indices.completed()) break;
        index++;
    }

    return indices;
}

bool Engine::query_extension_support(VkPhysicalDevice physical_device)
{
    uint32_t count;
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> properties(count);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, properties.data());
    bool all_supported{true};

    for (const auto &device_extension : device_extensions)
    {
        bool supported{false};

        for (const auto &property : properties)
        {
            if (strcmp(device_extension, property.extensionName) == 0)
            {
                supported = true;
                break;
            }
        }

        all_supported = all_supported || supported;
    }

    return all_supported;
}

Engine::Swapchain_Support Engine::query_swapchain_support(VkPhysicalDevice physical_device)
{
    Swapchain_Support support;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &support.surface_capabilities);
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);

    if (format_count != 0)
    {
        support.surface_formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, support.surface_formats.data());
    }

    uint32_t mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &mode_count, nullptr);

    if (mode_count != 0)
    {
        support.present_modes.resize(mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &mode_count, support.present_modes.data());
    }

    return support;
}

void Engine::create_logical_device()
{
    Queue_Family_Indices indices{find_queue_families(physical_device)};
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> queue_family_indices{indices.graphics_family.value(), indices.present_family.value()};
    float queue_priority{1.0f};

    for (uint32_t queue_family_index : queue_family_indices)
    {
        VkDeviceQueueCreateInfo queue_create_info{
            .sType{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO},
            // .pNext{},
            // .flags{},
            .queueFamilyIndex{queue_family_index},
            .queueCount{1},
            .pQueuePriorities{&queue_priority},
        };

        queue_create_infos.push_back(queue_create_info);
    }

#ifdef __APPLE__
    /* [2025-9-15 11:07 AM ET] `VK_KHR_PORTABILITY_SUBSET( ... )` is currently defined in `vulkan_beta.h` (which can be used from `vulkan.h` by defining `VK_ENABLE_BETA_EXTENSIONS`). `VK_KHR_portability_subset` [[.](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_portability_subset.html)] is a _device_ extension that depends on the _instance_ extension `VK_KHR_get_physical( ... )2` enabled during instance creation. It is required by `vkCreateDevice` on macOS. */
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
    device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif /* __APPLE__ */
    VkPhysicalDeviceFeatures enabled_features{};

    VkDeviceCreateInfo create_info{
        .sType{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO},
        // .pNext{},
        // .flags{},
        .queueCreateInfoCount{static_cast<uint32_t>(queue_create_infos.size())},
        .pQueueCreateInfos{queue_create_infos.data()},
        /* `enabledLayerCount` is deprecated and should not be used. */
        // .enabledLayerCount{},
        /* `ppEnabledLayerNames` is deprecated and should not be used. */
        // .ppEnabledLayerNames{},
        .enabledExtensionCount{static_cast<uint32_t>(device_extensions.size())},
        .ppEnabledExtensionNames{device_extensions.data()},
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
    vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
}

void Engine::create_swapchain()
{
    Swapchain_Support support{query_swapchain_support(physical_device)};
    VkSurfaceFormatKHR surface_format{choose_swapchain_surface_format(support.surface_formats)};
    VkPresentModeKHR present_mode{choose_swapchain_present_mode(support.present_modes)};
    VkExtent2D extent{choose_swapchain_extent(support.surface_capabilities)};
    /* We request one more image than the minimum. */
    uint32_t image_count{support.surface_capabilities.minImageCount + 1};
    if (image_count > support.surface_capabilities.maxImageCount && support.surface_capabilities.maxImageCount > 0) image_count = support.surface_capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR create_info{
        .sType{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR},
        // .pNext{},
        // .flags{},
        .surface{surface},
        .minImageCount{image_count},
        .imageFormat{surface_format.format},
        .imageColorSpace{surface_format.colorSpace},
        .imageExtent{extent},
        .imageArrayLayers{1},
        .imageUsage{VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT},
        // .imageSharingMode{},
        // .queueFamilyIndexCount{},
        // .pQueueFamilyIndices{},
        .preTransform{support.surface_capabilities.currentTransform},
        .compositeAlpha{VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR},
        .presentMode{present_mode},
        .clipped{VK_TRUE},
        .oldSwapchain{VK_NULL_HANDLE},
    };

    Queue_Family_Indices indices{find_queue_families(physical_device)};
    uint32_t queue_family_indices[]{indices.graphics_family.value(), indices.present_family.value()};

    if (indices.graphics_family != indices.present_family)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    CHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain));
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
    swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images.data());
    swapchain_image_format = surface_format.format;
    swapchain_extent = extent;
}

VkSurfaceFormatKHR Engine::choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &surface_formats)
{
    for (const auto &surface_format : surface_formats)
    {
        if (surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && surface_format.format == VK_FORMAT_B8G8R8A8_SRGB) return surface_format;
    }

    /* Otherwise, settle with the first format specified. */
    return surface_formats[0];
}

VkPresentModeKHR Engine::choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> &present_modes)
{
    for (const auto &present_mode : present_modes)
    {
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) return present_mode;
        /* On mobile devices, where energy usage is more important, we may prefer to use `VK_PRESENT_MODE_FIFO_KHR`. */
    }

    /* Only `VK_PRESENT_MODE_FIFO_KHR` is guaranteed to be available. */
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Engine::choose_swapchain_extent(const VkSurfaceCapabilitiesKHR &surface_capabilities)
{
    // fprintf(stdout, "%d %d\n", surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
    // fprintf(stdout, "%d %d\n", surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
    if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return surface_capabilities.currentExtent;
    int w, h;
    /*
        The following functions are similar.

        - `SDL_GetRenderOutputSize(SDL_Renderer *, ( ... ))` [[.](https://wiki.libsdl.org/SDL3/SDL_GetRenderOutputSize)]
        - `SDL_GetWindowSize` [[.](https://wiki.libsdl.org/SDL3/SDL_GetWindowSize)]
        - `SDL_GetWindowSizeInPixels` [[.](https://wiki.libsdl.org/SDL3/SDL_GetWindowSizeInPixels)]
    */
    SDL_GetWindowSizeInPixels(p_window, &w, &h);
    VkExtent2D extent{static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    extent.width = std::clamp(extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
    return extent;
}

void Engine::create_image_views()
{
    swapchain_image_views.resize(swapchain_images.size());

    for (size_t i{0}; i < swapchain_images.size(); i++)
    {
        VkImageViewCreateInfo create_info{
            .sType{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO},
            // .pNext{},
            // .flags{},
            .image{swapchain_images[i]},
            .viewType{VK_IMAGE_VIEW_TYPE_2D},
            .format{swapchain_image_format},
            .components{
                .r{VK_COMPONENT_SWIZZLE_IDENTITY},
                .g{VK_COMPONENT_SWIZZLE_IDENTITY},
                .b{VK_COMPONENT_SWIZZLE_IDENTITY},
                .a{VK_COMPONENT_SWIZZLE_IDENTITY},
            },
            .subresourceRange{
                .aspectMask{VK_IMAGE_ASPECT_COLOR_BIT},
                .baseMipLevel{0},
                .levelCount{1},
                .baseArrayLayer{0},
                .layerCount{1},
            },
        };

        CHECK(vkCreateImageView(device, &create_info, nullptr, &swapchain_image_views[i]));
    }
}

void Engine::create_render_pass()
{
    VkAttachmentDescription attachment{
        // .flags{},
        .format{swapchain_image_format},
        // clang-format off
        // clang-format on
        .samples{VK_SAMPLE_COUNT_1_BIT},
        .loadOp{VK_ATTACHMENT_LOAD_OP_CLEAR},
        .storeOp{VK_ATTACHMENT_STORE_OP_STORE},
        .stencilLoadOp{VK_ATTACHMENT_LOAD_OP_DONT_CARE},
        .stencilStoreOp{VK_ATTACHMENT_STORE_OP_DONT_CARE},
        .initialLayout{VK_IMAGE_LAYOUT_UNDEFINED},
        .finalLayout{VK_IMAGE_LAYOUT_PRESENT_SRC_KHR},
    };

    VkAttachmentReference color_attachment{
        .attachment{0},
        .layout{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
    };

    VkSubpassDescription subpass{
        // .flags{},
        .pipelineBindPoint{VK_PIPELINE_BIND_POINT_GRAPHICS},
        // .inputAttachmentCount{},
        // .pInputAttachments{},
        .colorAttachmentCount{1},
        .pColorAttachments{&color_attachment},
        // .pResolveAttachments{},
        // .pDepthStencilAttachment{},
        // .preserveAttachmentCount{},
        // .pPreserveAttachments{},
    };

    VkRenderPassCreateInfo create_info{
        .sType{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO},
        // .pNext{},
        // .flags{},
        .attachmentCount{1},
        .pAttachments{&attachment},
        .subpassCount{1},
        .pSubpasses{&subpass},
        // .dependencyCount{},
        // .pDependencies{},
    };

    CHECK(vkCreateRenderPass(device, &create_info, nullptr, &render_pass));
}

void Engine::create_graphics_pipeline()
{
    VkShaderModule vert_shader_module{create_shader_module(read_file("bin/triangle.vert.spv"))};
    VkShaderModule frag_shader_module{create_shader_module(read_file("bin/triangle.frag.spv"))};

    VkPipelineShaderStageCreateInfo stages[]{
        {
            .sType{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
            // .pNext{},
            // .flags{},
            .stage{VK_SHADER_STAGE_VERTEX_BIT},
            .module{vert_shader_module},
            .pName{"main"},
            // .pSpecializationInfo{},
        },
        {
            .sType{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
            // .pNext{},
            // .flags{},
            .stage{VK_SHADER_STAGE_FRAGMENT_BIT},
            .module{frag_shader_module},
            .pName{"main"},
            // .pSpecializationInfo{},
        },
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_state{
        .sType{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO},
        // .pNext{},
        // .flags{},
        /* This is optional. */ .vertexBindingDescriptionCount{0},
        /* This is optional. */ .pVertexBindingDescriptions{nullptr},
        /* This is optional. */ .vertexAttributeDescriptionCount{0},
        /* This is optional. */ .pVertexAttributeDescriptions{nullptr},
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state{
        .sType{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO},
        // .pNext{},
        // .flags{},
        .topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST},
        .primitiveRestartEnable{VK_FALSE},
    };

    VkPipelineViewportStateCreateInfo viewport_state{
        .sType{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO},
        // .pNext{},
        // .flags{},
        .viewportCount{1},
        // .pViewports{},
        .scissorCount{1},
        // .pScissors{},
    };

    VkPipelineRasterizationStateCreateInfo rasterization_state{
        .sType{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO},
        // .pNext{},
        // .flags{},
        .depthClampEnable{VK_FALSE},
        .rasterizerDiscardEnable{VK_FALSE},
        .polygonMode{VK_POLYGON_MODE_FILL},
        .cullMode{VK_CULL_MODE_BACK_BIT},
        .frontFace{VK_FRONT_FACE_CLOCKWISE},
        .depthBiasEnable{VK_FALSE},
        /* This is optional. */ .depthBiasConstantFactor{0.0f},
        /* This is optional. */ .depthBiasClamp{0.0f},
        /* This is optional. */ .depthBiasSlopeFactor{0.0f},
        .lineWidth{1.0f},
    };

    VkPipelineMultisampleStateCreateInfo multisample_state{
        .sType{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO},
        // .pNext{},
        // .flags{},
        .rasterizationSamples{VK_SAMPLE_COUNT_1_BIT},
        .sampleShadingEnable{VK_FALSE},
        /* This is optional. */ .minSampleShading{1.0f},
        /* This is optional. */ .pSampleMask{nullptr},
        /* This is optional. */ .alphaToCoverageEnable{VK_FALSE},
        /* This is optional. */ .alphaToOneEnable{VK_FALSE},
    };

    VkPipelineColorBlendAttachmentState attachment{
        .blendEnable{VK_FALSE},
        /* This is optional. */ .srcColorBlendFactor{VK_BLEND_FACTOR_ONE},
        /* This is optional. */ .dstColorBlendFactor{VK_BLEND_FACTOR_ZERO},
        /* This is optional. */ .colorBlendOp{VK_BLEND_OP_ADD},
        /* This is optional. */ .srcAlphaBlendFactor{VK_BLEND_FACTOR_ONE},
        /* This is optional. */ .dstAlphaBlendFactor{VK_BLEND_FACTOR_ZERO},
        /* This is optional. */ .alphaBlendOp{VK_BLEND_OP_ADD},
        .colorWriteMask{VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT},
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state{
        .sType{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO},
        // .pNext{},
        // .flags{},
        .logicOpEnable{VK_FALSE},
        /* This is optional. */ .logicOp{VK_LOGIC_OP_COPY},
        .attachmentCount{1},
        .pAttachments{&attachment},
        .blendConstants{
            /* This is optional. */ 0.0f,
            /* This is optional. */ 0.0f,
            /* This is optional. */ 0.0f,
            /* This is optional. */ 0.0f,
        },
    };

    std::vector<VkDynamicState> dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state{
        .sType{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO},
        // .pNext{},
        // .flags{},
        .dynamicStateCount{static_cast<uint32_t>(dynamic_states.size())},
        .pDynamicStates{dynamic_states.data()},
    };

    VkPipelineLayoutCreateInfo layout_create_info{
        .sType{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO},
        // .pNext{},
        // .flags{},
        /* This is optional. */ .setLayoutCount{0},
        /* This is optional. */ .pSetLayouts{nullptr},
        /* This is optional. */ .pushConstantRangeCount{0},
        /* This is optional. */ .pPushConstantRanges{nullptr},
    };

    CHECK(vkCreatePipelineLayout(device, &layout_create_info, nullptr, &pipeline_layout));

    VkGraphicsPipelineCreateInfo create_info{
        .sType{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO},
        // .pNext{},
        // .flags{},
        .stageCount{2},
        .pStages{stages},
        .pVertexInputState{&vertex_input_state},
        .pInputAssemblyState{&input_assembly_state},
        // .pTessellationState{},
        .pViewportState{&viewport_state},
        .pRasterizationState{&rasterization_state},
        .pMultisampleState{&multisample_state},
        /* This is optional. */ .pDepthStencilState{nullptr},
        .pColorBlendState{&color_blend_state},
        .pDynamicState{&dynamic_state},
        .layout{pipeline_layout},
        .renderPass{render_pass},
        .subpass{0},
        /* This is optional. */ .basePipelineHandle{VK_NULL_HANDLE},
        /* This is optional. */ .basePipelineIndex{-1},
    };

    CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create_info, nullptr, &graphics_pipeline));
    vkDestroyShaderModule(device, frag_shader_module, nullptr);
    vkDestroyShaderModule(device, vert_shader_module, nullptr);
}

std::vector<char> Engine::read_file(const std::string &file_name)
{
    std::ifstream file(file_name, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("`" + file_name + "` could not be opened.\n");
    size_t file_size{(size_t)file.tellg()};
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();
    return buffer;
}

VkShaderModule Engine::create_shader_module(const std::vector<char> &code)
{
    VkShaderModule shader_module;

    VkShaderModuleCreateInfo create_info{
        .sType{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO},
        // .pNext{},
        // .flags{},
        .codeSize{code.size()},
        .pCode{reinterpret_cast<const uint32_t *>(code.data())},
    };

    CHECK(vkCreateShaderModule(device, &create_info, nullptr, &shader_module));
    return shader_module;
}

void Engine::draw()
{
}

void Engine::event(SDL_Event *p_event)
{
}

void Engine::clean()
{
    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    for (const auto &image_view : swapchain_image_views) vkDestroyImageView(device, image_view, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    /* Device queues are destroyed when the device is destroyed. */
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(p_window);
}
