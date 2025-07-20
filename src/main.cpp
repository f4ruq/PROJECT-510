#include "510.hpp"

int main()
{
    socket.setsockopt(ZMQ_LINGER, 0);
    sdl_init();

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // imgui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext(); 
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    ImFontConfig cfg;
    cfg.SizePixels = 100.0f;

    ImFont* default_font = io.Fonts->AddFontFromFileTTF("assets/Inter_28pt-Regular.ttf", 18.0f);
    ImFont* bigg_font = io.Fonts->AddFontFromFileTTF("assets/Roboto_Condensed-Regular.ttf", 100.0f);
    ImFont* big_font = io.Fonts->AddFontFromFileTTF("assets/BitcountGridSingle_Cursive-Regular.ttf", 1.0f, &cfg);

    ImGui::StyleColorsDark();
    
    style.WindowBorderSize = 0.0f;
    style.FrameRounding = 6.0f;   
    style.PopupRounding = 8.0f;  
    style.ChildRounding = 10.0f;  
    style.GrabRounding = 4.0f;   
    //io.FontDefault = default_font;
    
    // backend init
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    while (running) 
    {
        sdl_event_check();
        // start imgui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        char user_input[256] = "";
    
        static bool goster = false;
        static std::string user_input_stc;
        ImVec2 displaySize = io.DisplaySize;
        float bottomHeight = displaySize.y * 0.8f;
        float halfHeight = displaySize.y * 0.5f;
        float fullHeight = displaySize.y;
        float fullWidth = displaySize.x; 
        float halfWidth = displaySize.x * 0.5f;        
        
        if(current_window == switch){window_name_switch();}
        else if(current_window == enter_adress){window_name_adress();}
        else if(current_window == client)
        {
            ImGui::SetNextWindowPos(ImVec2(0, bottomHeight));
            ImGui::SetNextWindowSize(ImVec2(fullWidth, bottomHeight));
            ImGui::Begin("...",nullptr, ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse);
                if(ImGui::Button("<")){current_window = enter_adress;}
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
        
                if(ImGui::Button("<")){current_window = switch;}
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
