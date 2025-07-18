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
#include "510.hpp"
#define switch 0
#define client 1
#define enter_adress 2
#define server 3

std::string response = sentinel_code;
std::string* response_ptr = &response;
std::thread zmq_client_funcThread;
std::thread zmq_server_funcThread;

int main()
{
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::dealer);
    socket.setsockopt(ZMQ_LINGER, 0);
    zmq::message_t identity;
    zmq::message_t request;
    std::string* received_message_ptr;
    bool running = true;
    int current_window = 0;
    
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
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.WindowBorderSize = 0.0f;
    style.FrameRounding = 6.0f;   
    style.PopupRounding = 8.0f;  
    style.ChildRounding = 10.0f;  
    style.GrabRounding = 4.0f;   
    ImFont* default_font = io.Fonts->AddFontFromFileTTF("assets/Inter_28pt-Regular.ttf", 18.0f);
    //ImFont* big_font = io.Fonts->AddFontFromFileTTF("assets/Roboto_Condensed-Regular.ttf", 100.0f);
    io.FontDefault = default_font;
    // backend init
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    
    

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
        float halfHeight = displaySize.y * 0.5f;
        float fullHeight = displaySize.y;
        float fullWidth = displaySize.x; 
        float halfWidth = displaySize.x * 0.5f;        
        
        if(current_window == switch)
        {
            client_exit_check = 1;
            client_socket_active = 1;
            if(zmq_client_funcThread.joinable()){zmq_client_funcThread.join();}
            
            server_exit_check = 1;
            server_socket_active = 0;
            if(zmq_server_funcThread.joinable()){zmq_server_funcThread.join();}
            
                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(ImVec2(displaySize.x , displaySize.y * 0.40f));
                
                ImGui::Begin("text window", nullptr, ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse);
                ImVec2 window_size = ImGui::GetWindowSize();
                float window_height = window_size.y;
                float window_width = window_size.x;
                
                ImGui::SetWindowFontScale(2.5f);
                ImGui::SetCursorPos(ImVec2(window_width * 0.25f +20, window_height * 0.5f));
                ImGui::Text("Choose an Application");
                ImGui::End();
            
            
            ImGui::SetNextWindowPos(ImVec2(0, displaySize.y * 0.40f));
            ImGui::SetNextWindowSize(ImVec2(displaySize.x, displaySize.y * 0.75f));
            
            ImGui::Begin("switch", nullptr, ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                //ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse);
                
                ImGui::SetCursorPos(ImVec2(window_width * 0.5f + 5 , 0));
                
                if(ImGui::Button("Server", ImVec2(window_width * 0.25f, 175)))
                {
                    try
                    {
                        server_exit_check = 1;
                        server_socket_active = 0;
                        if(zmq_server_funcThread.joinable()){zmq_server_funcThread.join();}
                        client_exit_check = 1;
                        client_socket_active = 1;
                        if(zmq_client_funcThread.joinable()){zmq_client_funcThread.join();}
                        
                        
                        socket.close();
                        socket = zmq::socket_t(context, zmq::socket_type::router);
                        socket.bind("tcp://*:5555");
                        server_exit_check = 0;
                        server_socket_active = 1;
                        zmq_server_funcThread = std::thread(zmq_server_func, std::ref(request), std::ref(identity), std::ref(socket), 
                            std::ref(response_ptr), std::ref(context), std::ref(received_message_ptr));
                        current_window = server;
                    }
                    catch(zmq::error_t err)
                    {
                        std::cerr << err.what() << std::endl;
                    }
                }

                ImGui::SetCursorPos(ImVec2(window_width * 0.25f,0));
                if(ImGui::Button("Client", ImVec2(window_width * 0.25f - 5, 175))){current_window = enter_adress;}
            ImGui::End();
        }
        else if(current_window == enter_adress)
        {
            try
            {
                server_exit_check = 1;
                server_socket_active = 0;
                if(zmq_server_funcThread.joinable()){zmq_server_funcThread.join();}

                client_exit_check = 1;
                client_socket_active = 0;
                if(zmq_client_funcThread.joinable()){zmq_client_funcThread.join();}
                socket.close();
            }
            catch(zmq::error_t err){std::cerr << err.what() << std::endl;}
            
            ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.25f, displaySize.y * 0.25f));
            ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight));
            ImGui::Begin("ADRESS", nullptr, ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse);
            if(ImGui::Button("GO BACK")){current_window = switch;}
            if(ImGui::InputText("enter the adress", adress_array, IM_ARRAYSIZE(adress_array), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                std::string adress_input(adress_array);
                    
                socket = zmq::socket_t(context, zmq::socket_type::dealer);
                try
                {
                    socket.connect(adress_input);
                    zmq_client_funcThread = std::thread(zmq_client_func, std::ref(socket), std::ref(context), std::ref(response_ptr));
                    client_exit_check = 0;
                    client_socket_active = 1;
                    current_window = client;
                }
                catch(zmq::error_t err)
                {
                    std::cerr << err.what() << std::endl;
                }  
            }
            ImGui::End();
        }
        else if(current_window == client)
        {
            ImGui::SetNextWindowPos(ImVec2(0, bottomHeight));
            ImGui::SetNextWindowSize(ImVec2(fullWidth, bottomHeight));
            ImGui::Begin("...",nullptr, ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse);
                if(ImGui::Button("GO BACK")){current_window = enter_adress;}
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
            

            ImGui::Begin("MESSAGES", nullptr, ImGuiWindowFlags_NoTitleBar);
        
                for (int i = 0; i < message_log.size(); ++i)
                {
                    ImGui::Text("%s", message_log[i].c_str());
                    ImGui::SetScrollHereY(1.0f);
                }

            ImGui::End();
        }
        else if(current_window == server)
        {
            ImGui::SetNextWindowPos(ImVec2(0, bottomHeight));
            ImGui::SetNextWindowSize(ImVec2(fullWidth, bottomHeight));
            ImGui::Begin("...",nullptr, ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse);
        
                if(ImGui::Button("GO BACK")){current_window = switch;}
                if(ImGui::InputText("Mesaj", user_input, IM_ARRAYSIZE(user_input), ImGuiInputTextFlags_EnterReturnsTrue) or ImGui::Button("SEND"))
                {
                    //std::cout << user_input << std::endl;
                    std::lock_guard<std::mutex> lock(globalMutex);
                    std::string user_input_str(user_input);
                    if(user_input_str == "exit")
                    {
                        client_exit_check = 1;
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
            
            ImGui::Begin("MESSAGES", nullptr, ImGuiWindowFlags_NoTitleBar);
                
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
    
    server_socket_active = 0;
    client_socket_active = 0;
    client_exit_check = 1;
    server_exit_check = 1;
    socket.close();
    context.shutdown();
    context.close();
    //socket.close();
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    if(zmq_client_funcThread.joinable()){zmq_client_funcThread.join();}
    if(zmq_server_funcThread.joinable()){zmq_server_funcThread.join();}
    return 0;
}
