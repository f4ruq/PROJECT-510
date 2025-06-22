#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include <SDL2/SDL.h>
#include <OpenGL/gl.h>

const std::string sentinel_code = "__::R7g!zPq$w9__";
std::mutex globalMutex;
std::atomic<bool> exit_check{false};
std::vector<std::string> message_log;
std::string input;

void zmq_communication(zmq::socket_t& socket, zmq::context_t& context)
{
    while (!exit_check)
    {
        // Mesaj varsa al
        zmq::pollitem_t items[] = {
            { static_cast<void*>(socket), 0, ZMQ_POLLIN, 0 }
        };
        zmq::poll(items, 1, 100);

        if (items[0].revents & ZMQ_POLLIN)
        {
            zmq::message_t reply;
            socket.recv(reply, zmq::recv_flags::none);
            std::string received = reply.to_string();

            if (received != sentinel_code)
            {
                std::lock_guard<std::mutex> lock(globalMutex);
                message_log.push_back("server: " + received);
            }
        }

        // input boş değilse gönder
        std::string to_send;
        {
            std::lock_guard<std::mutex> lock(globalMutex);
            to_send = input;
        }

        if (!to_send.empty())
        {
            zmq::message_t request(to_send.size());
            memcpy(request.data(), to_send.data(), to_send.size());
            socket.send(request, zmq::send_flags::none);

            std::lock_guard<std::mutex> lock(globalMutex);
            message_log.push_back("you: " + to_send);
            input.clear();

            if (to_send == "exit")
            {
                exit_check = true;
                socket.close();
                context.shutdown();
                context.close();
                break;
            }
        }
    }
}

int main()
{
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::dealer);
    socket.setsockopt(ZMQ_LINGER, 0);
    socket.connect("tcp://localhost:5555");

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* sdl_window = SDL_CreateWindow("ZMQ Client UI",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);

    SDL_GLContext gl_context = SDL_GL_CreateContext(sdl_window);
    SDL_GL_MakeCurrent(sdl_window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplSDL2_InitForOpenGL(sdl_window, gl_context);
    ImGui_ImplOpenGL2_Init();
    ImGui::StyleColorsDark();

    std::thread network_thread(zmq_communication, std::ref(socket), std::ref(context));

    bool show_window = true;
    char buf[512] = "";

    while (show_window && !exit_check)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                show_window = false;
        }

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Client Chat");
        
        {
            std::lock_guard<std::mutex> lock(globalMutex);
            for (const auto& msg : message_log)
                ImGui::TextWrapped("%s", msg.c_str());
        }

        ImGui::Separator();
        ImGui::InputText("Message", buf, sizeof(buf));
        if (ImGui::Button("Send"))
        {
            std::lock_guard<std::mutex> lock(globalMutex);
            input = std::string(buf);
            buf[0] = '\0';
        }

        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, 640, 480);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(sdl_window);
    }

    exit_check = true;
    network_thread.join();

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
    return 0;
}
