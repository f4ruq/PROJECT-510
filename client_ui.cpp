#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <SDL.h>
#include <SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include <vector>


const std::string sentinel_code = "__::R7g!zPq$w9__";
std::mutex globalMutex;
std::atomic<bool> exit_check{0};
std::atomic<bool> socket_active{0};
std::vector<std::string> message_log;
bool thread_started = 0;
std::string response = sentinel_code;
std::string* response_ptr = &response;
std::thread main_zmq_funcThread;

// main loop; receives messages, checks input, sends messages
void main_zmq_func(zmq::socket_t& socket_, zmq::context_t& context, std::string*& response_ptr)
{
        while(socket_active)
        {
            if(exit_check)
            {
                std::string exit_message = "client left the chat.";
                zmq::message_t request(exit_message.size());
                memcpy(request.data(), exit_message.data(), exit_message.size());
                socket_.send(request, zmq::send_flags::none);
                socket_.close();
                context.shutdown();
                context.close();
                break;
            }

            // poll for incoming server messages
            zmq::pollitem_t items[] = {
                { static_cast<void*>(socket_), 0, ZMQ_POLLIN, 0 }
            };

            zmq::poll(&items[0], 1, std::chrono::milliseconds(100));
            if(exit_check){break;}

            if(items[0].revents & ZMQ_POLLIN)
            {
                zmq::message_t reply;
                socket_.recv(reply, zmq::recv_flags::none);
                std::string received = reply.to_string();
                
                if(received != sentinel_code)
                {
                    std::lock_guard<std::mutex> lock(globalMutex);
                    message_log.push_back("server: " + received);
                    std::cout << "message from server: " << received << std::endl;
                }
            }

            //send the latest user input to the server
            std::string response_copy;
            {//mutex scope
                std::lock_guard<std::mutex> lock(globalMutex);
                response_copy = *response_ptr;
                *response_ptr = sentinel_code;
            }//
            zmq::message_t request(response_copy.size());
            memcpy(request.data(), response_copy.data(), response_copy.size());
            if(response_copy != sentinel_code){message_log.push_back("you: " + response_copy);}
            socket_.send(request, zmq::send_flags::none);
        }

}

int main()
{
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::dealer);
    socket.setsockopt(ZMQ_LINGER, 0);
    //socket.connect("tcp://localhost:5555");
    //socket.connect("tcp://0.tcp.eu.ngrok.io:12121");

    // sdl init
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return -1;
    }

    // sdl window with opengl context
    SDL_Window* window = SDL_CreateWindow("P-510 CLIENT APP",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // imgui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    // backend init
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();
    
    // main ui loop
    bool running = true;
    int current_window = 0;

    while (running) 
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                running = false;
        }

        // start imgui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        
        char user_input[256] = "";
        char adress_array[256] = "";
        static bool goster = false;
        ImVec2 displaySize = io.DisplaySize;
        float bottomHeight = displaySize.y * 0.8f;
        float fullWidth = displaySize.x; 
        ImGui::SetNextWindowPos(ImVec2(0, bottomHeight));
        ImGui::SetNextWindowSize(ImVec2(fullWidth, bottomHeight));

        if(current_window == 0)
        {
            socket_active = 0;
            exit_check = 1;
            if(main_zmq_funcThread.joinable()){main_zmq_funcThread.join();}
            socket.close();
            ImGui::Begin("ENTRY WINDOW");
            if(ImGui::InputText("enter the adress", adress_array, IM_ARRAYSIZE(adress_array), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                std::string adress_input(adress_array);
                
                socket = zmq::socket_t(context, zmq::socket_type::dealer);
                try
                {
                    socket.connect(adress_input);
                    main_zmq_funcThread = std::thread(main_zmq_func, std::ref(socket), std::ref(context), std::ref(response_ptr));
                    exit_check = 0;
                    socket_active = 1;
                    current_window = 1;
                }
                catch(zmq::error_t err)
                {
                    std::cerr << err.what() << std::endl;
                }  
            }
            ImGui::End();
        }
        else if(current_window == 1)
        {
            ImGui::Begin("...",nullptr,
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse);
                if(ImGui::Button("SET ADRESS")){current_window = 0;}
                if(ImGui::InputText("Mesaj", user_input, IM_ARRAYSIZE(user_input), ImGuiInputTextFlags_EnterReturnsTrue) or ImGui::Button("SEND"))    
                {
                    //std::cout << user_input << std::endl;
                    std::lock_guard<std::mutex> lock(globalMutex);
                    std::string user_input_str(user_input); 
                    *response_ptr = user_input_str;
                    ImGui::SetKeyboardFocusHere(-1);
                    
                }
                    
            ImGui::End();
            
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(fullWidth, bottomHeight));
            

            ImGui::Begin("MESSAGES");
        
                for (int i = 0; i < message_log.size(); ++i)
                {
                    ImGui::Text("%s", message_log[i].c_str());
                    ImGui::SetScrollHereY(1.0f);
                }

            ImGui::End();
        }
        
        // rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData()); 
        SDL_GL_SwapWindow(window);
    }
    
    //cleanup
    
    socket_active = 0;
    exit_check = 1;
    //socket.close();
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    if(main_zmq_funcThread.joinable()){main_zmq_funcThread.join();}

    return 0;
}
