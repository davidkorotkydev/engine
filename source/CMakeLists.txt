# Vulkan
# ======
find_package(Vulkan REQUIRED)
message(STATUS "Vulkan_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan_LIBRARIES ${Vulkan_LIBRARIES}")

# ---

target_sources(
    ${PROJECT_NAME} PRIVATE
    engine.cpp
    main.cpp
)

target_include_directories(
    ${PROJECT_NAME} PRIVATE
    ${SOURCE}
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    ${Vulkan_LIBRARIES}
    SDL3::SDL3
)
