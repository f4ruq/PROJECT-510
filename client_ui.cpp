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
std::atomic<bool> exit_check{false};
std::vector<std::string> message_log;

void clear_terminal(){std::cout << "\033[2J\033[H";}

void clear_line(){std::cout << "\033[F\033[2K";}

// handles user input and updates the shared response pointer
void user_input(std::string*& response_ptr) 
{
    {//mutex scope
        std::lock_guard<std::mutex> lock(globalMutex);
        std::string connect_message = "client is connected.";
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
void main_zmq_func(zmq::socket_t& socket_, zmq::context_t& context, std::string*& response_ptr)
{
    while(true)
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

        if(items[0].revents & ZMQ_POLLIN)
        {
            zmq::message_t reply;
            socket_.recv(reply, zmq::recv_flags::none);
            std::string received = reply.to_string();
            
            if(received != sentinel_code)
            {
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

//optional function for manual sending
void send_message(std::string& response_, zmq::socket_t& socket_, zmq::context_t& context)
{
    while(true)
    {
        std::cout << "type your message: ";
        std::getline(std::cin, response_);

        if (response_ != "exit")
        {
            zmq::message_t request(response_.size());
            memcpy(request.data(), response_.data(), response_.size());

            socket_.send(request, zmq::send_flags::none);
        }
        else
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
    }     
}

//optional function for receiving messages outside main thread
void receive_message(zmq::message_t& request_, zmq::socket_t& socket_)
{   
    while(true)
    {     
        zmq::recv_result_t result = socket_.recv(request_, zmq::recv_flags::none);
            
        if(result){
            std::string received_message = request_.to_string();
            std::cout << "message from server: " << received_message << std::endl;
        }
        else 
        {
            std::cout << "can't connect to server." << std::endl;
        }
    }
}

//used for idle echoing/pinging if needed
void idling(std::string &idle_code, zmq::socket_t &socket, zmq::message_t& request_)
{
    while(true)
    {
        zmq::message_t idle(idle_code.size());
        memcpy(idle.data(), idle_code.data(), idle_code.size());
        socket.send(idle, zmq::send_flags::none);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        zmq::recv_result_t result = socket.recv(request_, zmq::recv_flags::none);
            
        if(!result){
            std::cout << "can't connect to server." << std::endl;
        }
    }
}


int main()
{
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::dealer);
    socket.setsockopt(ZMQ_LINGER, 0);
    socket.connect("tcp://localhost:5555");

    std::string response = sentinel_code;
    std::string* response_ptr = &response;

    std::thread main_zmq_funcThread(main_zmq_func, std::ref(socket), std::ref(context), std::ref(response_ptr));

    // SDL init
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
    while (running) {
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
        static bool goster = false;

        ImGui::Begin("MAIN WINDOW");
       
            if(ImGui::Button("SEND") or ImGui::InputText("Mesaj", user_input, IM_ARRAYSIZE(user_input), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                //std::cout << user_input << std::endl;
                std::lock_guard<std::mutex> lock(globalMutex);
                std::string user_input_str(user_input);
                if(user_input_str == "exit")
                {
                    running = 0;
                    ImGui_ImplOpenGL2_Shutdown();
                    ImGui_ImplSDL2_Shutdown();
                    ImGui::DestroyContext();
                    SDL_GL_DeleteContext(gl_context);
                    SDL_DestroyWindow(window);
                    SDL_Quit();
                }
                response_ptr = &user_input_str;
                
                ImGui::SetKeyboardFocusHere(-1);
            }
        
        ImGui::End();

        ImGui::Begin("MESSAGES");
    
            for (int i = 0; i < message_log.size(); ++i)
            {
                ImGui::Text("%s", message_log[i].c_str());
                ImGui::SetScrollHereY(1.0f);
            }

        ImGui::End();
        
        // rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
    
    //cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    main_zmq_funcThread.join();

    return 0;
}
