cmake_minimum_required(VERSION 3.16)
project(client_app)

set(CMAKE_CXX_STANDARD 17)

# Include directories
include_directories(
    include
    imgui
    imgui/backends
    /opt/homebrew/include
    /opt/homebrew/include/SDL2
)

# Link directories
link_directories(
    /opt/homebrew/lib
)

# Source files
file(GLOB IMGUI_SRC
    imgui/*.cpp
    imgui/backends/imgui_impl_sdl2.cpp
    imgui/backends/imgui_impl_opengl2.cpp
)

file(GLOB SRC_FILES
    src/*.cpp
)

add_executable(client_app
    ${SRC_FILES}
    ${IMGUI_SRC}
)

# Libraries to link
target_link_libraries(client_app
    SDL2
    "-framework OpenGL"
    zmq
)
