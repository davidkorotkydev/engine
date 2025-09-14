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

if(DEFINED CMAKE_OSX_SYSROOT)
    message(STATUS "CMAKE_OSX_SYSROOT ${CMAKE_OSX_SYSROOT}")
endif()

message(STATUS "CMAKE_C_COMPILER ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER_ID ${CMAKE_CXX_COMPILER_ID}")

if(CMAKE_CXX_COMPILER_ID STREQUAL AppleClang)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-braced-scalar-init)
endif()
