/*
clang++ client_ui.cpp \
imgui/imgui*.cpp \
imgui/backends/imgui_impl_sdl2.cpp imgui/backends/imgui_impl_opengl2.cpp \
-I/opt/homebrew/include -L/opt/homebrew/lib -lzmq -Iimgui -Iimgui/backends \
-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
-L/opt/homebrew/lib \
-lSDL2 -framework OpenGL \
-std=c++17 -o imgui_blank
*/

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
std::string client_id_str;
std::atomic<bool> exit_check{false};
std::vector<std::string> message_log;
std::atomic<bool> socket_start{false};

void clear_terminal(){std::cout << "\033[2J\033[H";}

void clear_line(){std::cout << "\033[F\033[2K";}

// handles user input and updates the shared response pointer
void user_input(std::string*& response_ptr) 
{
    {//mutex scope
        std::lock_guard<std::mutex> lock(globalMutex);
        std::string connect_message = "server is connected.";
        *response_ptr = connect_message;
    }//

    while (true)   
    {
        std::string response;
        std::getline(std::cin, response); clear_line();
        std::cout << "you: " << response << std::endl;
        
        if(response == "exit")
        {
            exit_check = true;
            break;
        }

        {//mutex scope
            std::lock_guard<std::mutex> lock(globalMutex);
            *response_ptr = response;
        }//
    }
}

// main loop; receives messages, checks input, sends messages
void main_zmq_func(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_, std::string*& response_ptr, 
    zmq::context_t& context, std::string*& received_message_ptr) 
{
    while(1)
    {
        while (socket_start) 
        {
            if(exit_check)
            {
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
            
            // poll the socket for new incoming data with a 100ms timeout
            zmq::pollitem_t items[] = {{static_cast<void*>(socket_), 0, ZMQ_POLLIN, 0}};
            zmq::poll(items, 1, 100);

            if (items[0].revents & ZMQ_POLLIN) 
            {
                //router socket requires receiving the client identity first
                socket_.recv(identity_, zmq::recv_flags::none);
                socket_.recv(request_, zmq::recv_flags::none);
                client_id_str = identity_.to_string();

                std::string received_message = request_.to_string();
                
                if (received_message != sentinel_code) 
                {
                    std::lock_guard<std::mutex> lock(globalMutex);
                    received_message_ptr = &received_message;
                    message_log.push_back("client: " + received_message);
                    std::cout << "message from client: " << received_message << std::endl;
                }
            }

            {//mutex scope  
                std::lock_guard<std::mutex> lock(globalMutex);
                std::string response_ = *response_ptr;
                
                if (response_ != sentinel_code && !client_id_str.empty()) 
                {
                    message_log.push_back("you: " + response_);
                    zmq::message_t id_msg(client_id_str.begin(), client_id_str.end());
                    zmq::message_t reply(response_.size());
                    memcpy(reply.data(), response_.data(), response_.size());
                    socket_.send(id_msg, zmq::send_flags::sndmore);
                    socket_.send(reply, zmq::send_flags::none);
                    *response_ptr = sentinel_code;
                }
                
            }//
        }
    }
}

//used for idle echoing/pinging if needed
void idling(std::string &idle_code, zmq::socket_t &socket, zmq::message_t& identity, zmq::message_t& request_)
{
    while(true)
    {
        socket.recv(identity, zmq::recv_flags::none);
        zmq::recv_result_t result = socket.recv(request_, zmq::recv_flags::none);
         
        if(!result)
        {
            std::cout << "can't connect to client." << std::endl;
        }

        zmq::message_t idle(idle_code.size());
        memcpy(idle.data(), idle_code.data(), idle_code.size());

        socket.send(identity, zmq::send_flags::sndmore);
        socket.send(idle, zmq::send_flags::none);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

//for manual sending inside main thread
void send_message(std::string& response_, zmq::message_t& identity_, zmq::socket_t& socket_)
{
        if(response_ != "exit")
        {
            zmq::message_t reply(response_.size());
            memcpy(reply.data(), response_.data(), response_.size());
            // send mesage
            socket_.send(identity_, zmq::send_flags::sndmore); // Önce istemcinin kimliğini gönder
            socket_.send(reply, zmq::send_flags::none); // Sonra mesajı gönder   
        } 
        else
        {
            std::string exit_message = "server left the chat";
            zmq::message_t reply(exit_message.size());
            memcpy(reply.data(), exit_message.data(), exit_message.size());
            socket_.send(identity_, zmq::send_flags::sndmore); // Önce istemcinin kimliğini gönder
            socket_.send(reply, zmq::send_flags::none); // Sonra mesajı gönder 
            
        }
}

//for receiving messages inside main thread manually
void receive_message(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_, std::string*& received_message_ptr)
{
    while(exit_check)
    {
        zmq::pollitem_t items[] = {{static_cast<void*>(socket_), 0, ZMQ_POLLIN, 0}};
        zmq::poll(items, 1, 100);

        if (items[0].revents & ZMQ_POLLIN) 
        {
            //router socket requires receiving the client identity first
            socket_.recv(identity_, zmq::recv_flags::none);
            socket_.recv(request_, zmq::recv_flags::none);
            client_id_str = identity_.to_string();
            std::string received_message = request_.to_string();
            
            if (received_message != sentinel_code) 
            {
                std::lock_guard<std::mutex> lock(globalMutex);
                std::cout << "message from client: " << received_message << std::endl;
                *received_message_ptr = received_message;
            }
        }
    }
}

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::router);
    socket.setsockopt(ZMQ_LINGER, 0);
    std::string adress = "tcp://*:5555";
    //socket.bind(adress);
    int current_window = 0;
    zmq::message_t identity;
    zmq::message_t request;
    std::string response = sentinel_code;
    std::string* response_ptr = &response;
    std::string* received_message_ptr;

    //std::thread receive_messageThread(receive_message, std::ref(request), std::ref(identity), std::ref(socket), std::ref(received_message_ptr));
    std::thread main_zmq_funcThread(main_zmq_func, std::ref(request), std::ref(identity), std::ref(socket), 
        std::ref(response_ptr), std::ref(context), std::ref(received_message_ptr));

    // SDL init
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return -1;
    }

    // sdl window with opengl context
    SDL_Window* window = SDL_CreateWindow("P-510 SERVER APP",
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
        static std::string user_input_stc;
        ImVec2 displaySize = io.DisplaySize;
        float bottomHeight = displaySize.y * 0.8f;
        float fullWidth = displaySize.x; 
        
        ImGui::SetNextWindowPos(ImVec2(0, bottomHeight));
        ImGui::SetNextWindowSize(ImVec2(fullWidth, bottomHeight));

        if(current_window == 0)
        {
            ImGui::Begin("ENTRY WINDOW");
            if(ImGui::InputText("enter the adress", adress_array, IM_ARRAYSIZE(adress_array), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                std::string adress_input(adress_array);
                socket.close();
                socket = zmq::socket_t(context, zmq::socket_type::router);
                try
                {
                    socket.bind(adress_input);
                    socket_start = 1;
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
        
                if(ImGui::InputText("Mesaj", user_input, IM_ARRAYSIZE(user_input), ImGuiInputTextFlags_EnterReturnsTrue) or ImGui::Button("SEND"))
                {
                    //std::cout << user_input << std::endl;
                    std::lock_guard<std::mutex> lock(globalMutex);
                    std::string user_input_str(user_input);
                    if(user_input_str == "exit")
                    {
                        exit_check = 1;
                        ImGui_ImplOpenGL2_Shutdown();
                        ImGui_ImplSDL2_Shutdown();
                        ImGui::DestroyContext();
                        SDL_GL_DeleteContext(gl_context);
                        SDL_DestroyWindow(window);
                        SDL_Quit();
                        running = 0;
                    }
                    user_input_stc = user_input_str;
                    response_ptr = &user_input_stc;
                    ImGui::SetKeyboardFocusHere(-1);
                }

            ImGui::End();
            
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(fullWidth, bottomHeight));
            
            ImGui::Begin("MESSAGES");
                
                for (int i = 0; i < message_log.size(); ++i)
                {
                    ImGui::Text("%s", message_log[i].c_str());
                    
                }
                
                ImGui::SetScrollHereY(1.0f);
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
    exit_check = 1;
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    main_zmq_funcThread.join();
    return 0;
}
