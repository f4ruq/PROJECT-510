#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <vector>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include <SDL2/SDL.h>
#include <OpenGL/gl.h>

const std::string sentinel_code = "__::R7g!zPq$w9__";
std::mutex globalMutex;
std::string client_id_str;
std::atomic<bool> exit_check{false};

std::vector<std::string> message_log;
bool scroll_to_bottom = false;

void clear_terminal() { std::cout << "\033[2J\033[H"; }
void clear_line() { std::cout << "\033[F\033[2K"; }

void user_input(std::string*& response_ptr) {
    std::lock_guard<std::mutex> lock(globalMutex);
    std::string connect_message = "server is connected.";
    *response_ptr = connect_message;
}

void main_system_func(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_, std::string*& response_ptr, zmq::context_t& context) {
    while (true) {
        if (exit_check) {
            std::string exit_message = "server left the chat.";
            zmq::message_t id_msg(client_id_str.begin(), client_id_str.end());
            zmq::message_t reply(exit_message.size());
            memcpy(reply.data(), exit_message.data(), exit_message.size());
            socket_.send(id_msg, zmq::send_flags::sndmore);
            socket_.send(reply, zmq::send_flags::none);
            socket_.close();
            context.shutdown();
            context.close();
            break;
        }

        zmq::pollitem_t items[] = {{static_cast<void*>(socket_), 0, ZMQ_POLLIN, 0}};
        zmq::poll(items, 1, 100);

        if (items[0].revents & ZMQ_POLLIN) {
            socket_.recv(identity_, zmq::recv_flags::none);
            socket_.recv(request_, zmq::recv_flags::none);
            client_id_str = identity_.to_string();
            std::string received_message = request_.to_string();
            if (received_message != sentinel_code) {
                message_log.push_back("Client: " + received_message);
                scroll_to_bottom = true;
            }
        }

        std::lock_guard<std::mutex> lock(globalMutex);
        std::string response_ = *response_ptr;
        if (response_ != sentinel_code && !client_id_str.empty()) {
            zmq::message_t id_msg(client_id_str.begin(), client_id_str.end());
            zmq::message_t reply(response_.size());
            memcpy(reply.data(), response_.data(), response_.size());
            socket_.send(id_msg, zmq::send_flags::sndmore);
            socket_.send(reply, zmq::send_flags::none);
            message_log.push_back("You: " + response_);
            scroll_to_bottom = true;
            *response_ptr = sentinel_code;
        }
    }
}

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::router);
    socket.setsockopt(ZMQ_LINGER, 0);
    socket.bind("tcp://*:5555");

    zmq::message_t identity;
    zmq::message_t request;
    std::string response = sentinel_code;
    std::string* response_ptr = &response;

    std::thread net_thread(main_system_func, std::ref(request), std::ref(identity), std::ref(socket), std::ref(response_ptr), std::ref(context));

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* sdl_window = SDL_CreateWindow("ZMQ Server", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext gl_context = SDL_GL_CreateContext(sdl_window);
    SDL_GL_MakeCurrent(sdl_window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(sdl_window, gl_context);
    ImGui_ImplOpenGL2_Init();

    static char input[256] = "";
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;
        }

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Server UI");

        ImGui::BeginChild("Messages", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 10), true);
        for (const auto& msg : message_log)
            ImGui::TextWrapped("%s", msg.c_str());
        if (scroll_to_bottom) ImGui::SetScrollHereY(1.0f);
        scroll_to_bottom = false;
        ImGui::EndChild();

        ImGui::InputText("Message", input, IM_ARRAYSIZE(input));
        ImGui::SameLine();
        if (ImGui::Button("Send")) {
            std::string message = input;
            if (!message.empty()) {
                std::lock_guard<std::mutex> lock(globalMutex);
                *response_ptr = message;
                input[0] = '\0';
            }
        }
        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, 640, 480);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(sdl_window);
    }

    net_thread.join();
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    return 0;
}
