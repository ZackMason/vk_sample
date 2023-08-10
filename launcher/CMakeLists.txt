cmake_minimum_required(VERSION 3.8.12)

project(launcher C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

if ( CMAKE_COMPILER_IS_GNUCC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()
if ( MSVC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W4 /WX /wd4702 /wd4100 /wd4201")
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:10000000")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(launcher 
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/bindings/imgui_impl_glfw.cpp
    ${PROJECT_SOURCE_DIR}/bindings/imgui_impl_opengl2.cpp
    ${PROJECT_SOURCE_DIR}/bindings/imgui_impl_glfw.h
    ${PROJECT_SOURCE_DIR}/bindings/imgui_impl_opengl2.h
)

target_include_directories(launcher PUBLIC src)
target_include_directories(launcher PUBLIC include)
target_include_directories(launcher PUBLIC bindings)

target_link_libraries(launcher ${CONAN_LIBS})

