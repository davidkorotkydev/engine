cmake_minimum_required(VERSION 3.31)
project(engine)

set(BIN ${PROJECT_SOURCE_DIR}/bin)
set(DEPENDENCIES ${PROJECT_SOURCE_DIR}/dependencies)
set(SOURCE ${PROJECT_SOURCE_DIR}/source)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BIN})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN})

add_executable(${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

add_subdirectory(dependencies)
add_subdirectory(source)

message(STATUS "CMAKE_C_COMPILER ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER_ID ${CMAKE_CXX_COMPILER_ID}")

if(DEFINED CMAKE_OSX_SYSROOT)
    message(STATUS "CMAKE_OSX_SYSROOT ${CMAKE_OSX_SYSROOT}")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL AppleClang)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-braced-scalar-init)
endif()

# Shaders
# =======
set(GLSL_COMPILER $ENV{VULKAN_SDK}/bin/glslc)
# [[.](https://cmake.org/cmake/help/latest/command/cmake_path.html#normal-path)]
cmake_path(NORMAL_PATH GLSL_COMPILER)
message(STATUS "GLSL_COMPILER ${GLSL_COMPILER}")
set(SHADERS ${PROJECT_SOURCE_DIR}/shaders)

file(
    GLOB GLSL_SOURCE
    ${SHADERS}/*.comp
    ${SHADERS}/*.frag
    ${SHADERS}/*.vert
)

foreach(GLSL ${GLSL_SOURCE})
    get_filename_component(FILENAME ${GLSL} NAME)
    set(SPIR_V ${BIN}/${FILENAME}.spv)

    add_custom_command(
        OUTPUT ${SPIR_V}
        COMMAND ${GLSL_COMPILER} ${GLSL} -o ${SPIR_V}
        DEPENDS ${GLSL}
    )

    list(APPEND SPIR_VS ${SPIR_V})
endforeach(GLSL)

add_custom_target(shaders DEPENDS ${SPIR_VS})
add_dependencies(${PROJECT_NAME} shaders)
