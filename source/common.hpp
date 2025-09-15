#pragma once

// #define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

// clang-format off
#define CHECK(RESULT) \
    do { \
        VkResult result = RESULT; \
        if (result) throw std::runtime_error("A Vulkan error was detected.\n" + std::string(string_VkResult(result)) + "\n"); \
    } while (0)
// clang-format on
