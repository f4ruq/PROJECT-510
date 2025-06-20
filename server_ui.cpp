#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <zmq.hpp>
#include <imgui.h>
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

#include <iostream>
#include <thread>
#include <string>
#include <mutex>
#include <atomic>

const std::string sentinel_code = "__::R7g!zPq$w9__";
std::string client_id_str;
std::mutex global_mutex;
std::atomic<bool> exit_check{false};

void zmq_thread_func(std::string* response_ptr, zmq::context_t* context_ptr)
{
    zmq::socket_t socket(*context_ptr, zmq::socket_type::router);
    socket.set(zmq::sockopt::linger, 0);
    socket.bind("tcp://*:5555");

    zmq::message_t identity;
    zmq::message_t request;

    while (!exit_check) {
        zmq::pollitem_t items[] = { { static_cast<void*>(socket), 0, ZMQ_POLLIN, 0 } };
        zmq::poll(items, 1, std::chrono::milliseconds(100));

        if (items[0].revents & ZMQ_POLLIN) {
            if (!socket.recv(identity, zmq::recv_flags::none)) continue;
            if (!socket.recv(request, zmq::recv_flags::none)) continue;

            std::string msg = request.to_string();
            {
                std::lock_guard<std::mutex> lock(global_mutex);
                client_id_str = identity.to_string();
                *response_ptr = msg;
            }
        }

        std::lock_guard<std::mutex> lock(global_mutex);
        if (*response_ptr != sentinel_code && !client_id_str.empty()) {
            zmq::message_t id_msg(client_id_str.begin(), client_id_str.end());
            zmq::message_t reply(response_ptr->begin(), response_ptr->end());

            socket.send(id_msg, zmq::send_flags::sndmore);
            socket.send(reply, zmq::send_flags::none);

            *response_ptr = sentinel_code;
        }
    }

    socket.close();
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    SDL_Window* window = SDL_CreateWindow("ImGui ZMQ Server",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    glewExperimental = GL_TRUE;
    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsDark();

    std::string response = sentinel_code;
    std::string* response_ptr = &response;
    zmq::context_t context(1);

    std::thread network_thread(zmq_thread_func, response_ptr, &context);
    
    while (!exit_check)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                exit_check = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("ZMQ Server");
        
        ImGui::Text("Last received:");
        {
            std::lock_guard<std::mutex> lock(global_mutex);
            if(*response_ptr != sentinel_code)
            {
                ImGui::TextWrapped("%s", response_ptr->c_str());
            }
        }
        if (ImGui::Button("Send Hello")) {
            std::lock_guard<std::mutex> lock(global_mutex);
            *response_ptr = "Hello from server!";
        }
        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    exit_check = true;
    network_thread.join();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
