#include "510.hpp"

SDL_Window* window = SDL_CreateWindow("P-510 CLIENT APP",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    
int sdl_init()
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) 
    {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return -1;
    }
}

void sdl_event_check()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) 
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
        running = false;
    }
}

ImFont* get_icon_font(ImGuiIO& io)
{
    static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = false;
    icons_config.PixelSnapH = true;
    icons_config.OversampleH = icons_config.OversampleV = 1; 
    ImFont* icons = io.Fonts->AddFontFromFileTTF("/Users/xubustein/Desktop/workspaces/xubusteins_masterpiece/assets/Font-Awesome-7-Free-Solid-900.otf", 20.0f, &icons_config, icons_ranges);
    return icons;
}

ImFont* get_main_font(ImGuiIO& io)
{
    ImFont* main_font = io.Fonts->AddFontFromFileTTF("/Users/xubustein/Desktop/workspaces/xubusteins_masterpiece/assets/Roboto_Condensed-Regular.ttf", 20.0f);
    return main_font;
}

void window_name_switch()
{
    client_exit_check = 1;
    client_socket_active = 1;
    ImGuiIO& io = ImGui::GetIO();
    //ImFont* icons = get_icon_font(io);
    if (zmq_client_funcThread.joinable()) { zmq_client_funcThread.join(); }

    server_exit_check = 1;
    server_socket_active = 0;
    if (zmq_server_funcThread.joinable()) { zmq_server_funcThread.join(); }

    float display_size_x = ImGui::GetIO().DisplaySize.x;
    float display_size_y = ImGui::GetIO().DisplaySize.y;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(display_size_x, display_size_y * 0.40f));

    ImGui::Begin("text window", nullptr, ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoCollapse);

        ImVec2 window_size = ImGui::GetWindowSize();
        float window_width = window_size.x;
        float window_height = window_size.y;
        ImGui::SetWindowFontScale(3.5f);
        ImGui::SetCursorPos(ImVec2(window_width * 0.25f + 20, window_height * 0.7f));
        //ImGui::Text("Choose an Application");
        ImGui::PushFont(get_main_font(io));
        ImGui::Text("Connect");
        ImGui::SetCursorPos(ImVec2(window_width * 0.50f + 60, window_height * 0.7));
        ImGui::Text("Host");    
        ImGui::PopFont();
    ImGui::End();


    ImGui::SetNextWindowPos(ImVec2(0, display_size_y * 0.40f));
    ImGui::SetNextWindowSize(ImVec2(display_size_x, display_size_y * 0.75f));

    ImGui::Begin("switch", nullptr, ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowFontScale(3.0f);
        ImGui::SetCursorPos(ImVec2(window_width * 0.5f + 5, 0));
        ImGui::PushFont(get_icon_font(io));
        if (ImGui::Button(u8"\uf015", ImVec2(window_width * 0.25f, 175)))//server button
        {
            try
            {
                server_exit_check = 1;
                server_socket_active = 0;
                if (zmq_server_funcThread.joinable()) { zmq_server_funcThread.join(); }

                client_exit_check = 1;
                client_socket_active = 1;
                if (zmq_client_funcThread.joinable()) { zmq_client_funcThread.join(); }

                socket.close();
                socket = zmq::socket_t(context, zmq::socket_type::router);
                socket.bind("tcp://*:5555");

                server_exit_check = 0;
                server_socket_active = 1;
                zmq_server_funcThread = std::thread(zmq_server_func,
                                                    std::ref(request), 
                                                    std::ref(identity), 
                                                    std::ref(socket),
                                                    std::ref(response_ptr), 
                                                    std::ref(context), 
                                                    std::ref(received_message_ptr));
                current_window = server;
            }
            
            catch (zmq::error_t err)
            {
                std::cerr << err.what() << std::endl;
            }
        }
        ImGui::PopFont();
        ImGui::SetCursorPos(ImVec2(window_width * 0.25f, 0));
        ImGui::PushFont(get_icon_font(io));
        if (ImGui::Button(u8"\uf075", ImVec2(window_width * 0.25f - 5, 175)))//client button
        {
            current_window = enter_adress;
        }
        ImGui::PopFont();
    ImGui::End();
}

void window_name_adress()
{
    ImGuiIO& io = ImGui::GetIO();
    float display_size_x = ImGui::GetIO().DisplaySize.x;
    float display_size_y = ImGui::GetIO().DisplaySize.y;
    float halfWidth = display_size_x * 0.5f;
    float halfHeight = display_size_y * 0.5f;
    char adress_array[256] = "";
    try
    {
        server_exit_check = 1;
        server_socket_active = 0;
        if(zmq_server_funcThread.joinable()){zmq_server_funcThread.join();}

        client_socket_active = 0;
        if(zmq_client_funcThread.joinable()){zmq_client_funcThread.join();}
        socket.close();
    }
    catch(zmq::error_t err){std::cerr << err.what() << std::endl;}
    
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(45, 46));
    
    ImGui::Begin("back button", nullptr, ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowFontScale(1.35f);    
    ImGui::PushFont(get_icon_font(io));    
        if(ImGui::Button(u8"\uf060")){current_window = switch;}
        ImGui::PopFont();  
    ImGui::End(); 
    
    ImGui::SetNextWindowPos(ImVec2(display_size_x * 0.25f, display_size_y * 0.25f));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight * 0.5f));

    ImGui::Begin("text window", nullptr, ImGuiWindowFlags_NoTitleBar |
                                         ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoCollapse);

        ImVec2 window_size = ImGui::GetWindowSize();
        float window_width = window_size.x;
        float window_height = window_size.y;
        ImGui::SetWindowFontScale(3.5f);
        ImGui::SetCursorPos(ImVec2(window_width * 0.25f - 10, window_height * 0.5f));
        ImGui::PushFont(get_main_font(io));
        ImGui::Text("Enter Adress");   
        ImGui::PopFont();
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(display_size_x * 0.25f, display_size_y * 0.5f));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight * 0.5f));
    ImGui::Begin("ADRESS", nullptr, ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowFontScale(1.75f);  
        ImGui::SetNextItemWidth(window_width-10);
        ImGui::PushFont(get_main_font(io));
        if(ImGui::InputText("##", adress_array, IM_ARRAYSIZE(adress_array), ImGuiInputTextFlags_EnterReturnsTrue))
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
        ImGui::PopFont();

        ImGui::PushFont(get_main_font(io));
        if(ImGui::Button("Connect", ImVec2(window_width-10, 40.0f)))
        {
            
        }
        ImGui::PopFont();
    ImGui::End();
}

void window_name_client()
{
    ImGuiIO& io = ImGui::GetIO();
    float display_size_x = ImGui::GetIO().DisplaySize.x;
    float display_size_y = ImGui::GetIO().DisplaySize.y;
    float halfWidth = display_size_x * 0.5f;
    float bottomHeight = display_size_y * 0.9f;
    bool send = 0;
    static char user_input[256] = "";
    
    ImGui::SetNextWindowPos(ImVec2(0, bottomHeight));
    ImGui::SetNextWindowSize(ImVec2(display_size_x, bottomHeight));
    
    ImGui::Begin("...",nullptr, ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse);
        //if(ImGui::Button("<")){current_window = enter_adress;}
        ImGui::SetWindowFontScale(1.5f);
        ImGui::SetNextItemWidth(display_size_x - 65);
        ImGui::PushFont(get_main_font(io));
        if(ImGui::InputText("##", user_input, IM_ARRAYSIZE(user_input), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            send = 1;
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::PushFont(get_icon_font(io));
        if(ImGui::Button(u8"\uf1d8"))
        {
            send = 1;
        }
        ImGui::PopFont();
        
        if(send)
        {
            std::lock_guard<std::mutex> lock(globalMutex);
            std::string user_input_str(user_input); 
            static std::string tmp_storage;
            tmp_storage = user_input_str;
            response_ptr = &tmp_storage;
            user_input[0] = '\0';
            send = 0;
        }
        
    ImGui::End();
        
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(display_size_x, bottomHeight));
        
        
    ImGui::Begin("MESSAGES", nullptr, ImGuiWindowFlags_NoTitleBar);
        ImGui::SetWindowFontScale(1.35f);
        ImGui::PushFont(get_icon_font(io));
        if(ImGui::Button(u8"\uf060")){current_window = enter_adress;}
        ImGui::PopFont();
        for (int i = 0; i < message_log.size(); ++i)
        {
            ImGui::PushFont(get_main_font(io));
            ImGui::Text("%s", message_log[i].c_str());
            ImGui::SetScrollHereY(1.0f);
            ImGui::PopFont();
        }
    
    ImGui::End();
        
            
    }
    
void window_name_server(SDL_GLContext& gl_context)
{
    float display_size_x = ImGui::GetIO().DisplaySize.x;
    float display_size_y = ImGui::GetIO().DisplaySize.y;
    float halfWidth = display_size_x * 0.5f;
    float bottomHeight = display_size_y * 0.9f;
    static char user_input[256] = "";
    bool send = 0;   
    ImGuiIO& io = ImGui::GetIO();
    
    ImGui::SetNextWindowPos(ImVec2(0, bottomHeight));
            ImGui::SetNextWindowSize(ImVec2(display_size_x, bottomHeight));
            ImGui::Begin("...",nullptr, ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse);
                ImGui::SetWindowFontScale(1.5f);
                ImGui::SetNextItemWidth(display_size_x - 65);
                ImGui::PushFont(get_main_font(io));
                if(ImGui::InputText("##", user_input, IM_ARRAYSIZE(user_input), ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    send = 1;
                    ImGui::SetKeyboardFocusHere(-1);
                }
                ImGui::PopFont();
                ImGui::SameLine();
                ImGui::PushFont(get_icon_font(io));
                if(ImGui::Button(u8"\uf1d8"))
                {
                    send = 1;
                }
                ImGui::PopFont();
                
                if(send)
                {
                    //ImGui::SetKeyboardFocusHere(-1);
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
                    static std::string tmp_storage;
                    tmp_storage = user_input_str;
                    response_ptr = &tmp_storage;
                    user_input[0] = '\0';    
                    send = 0;
                }

            ImGui::End();
            
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(display_size_x, bottomHeight));
            
            ImGui::Begin("MESSAGES", nullptr, ImGuiWindowFlags_NoTitleBar);
                ImGui::SetWindowFontScale(1.35f);
                ImGui::PushFont(get_icon_font(io));
                if(ImGui::Button(u8"\uf060")){current_window = switch;}
                ImGui::PopFont();

                for (int i = 0; i < message_log.size(); ++i)
                {
                    ImGui::PushFont(get_main_font(io));
                    ImGui::Text("%s", message_log[i].c_str());
                    ImGui::PopFont();
                }
                
                ImGui::SetScrollHereY(1.0f);
            ImGui::End();
}

void window_name_register()
{
    ImGuiIO& io = ImGui::GetIO();
    float display_size_x = ImGui::GetIO().DisplaySize.x;
    float display_size_y = ImGui::GetIO().DisplaySize.y;
    float halfWidth = display_size_x * 0.5f;
    float halfHeight = display_size_y * 0.5f;
    static char user_name[256] = "";
    static char password[256] = "";
    
    try
    {
        server_exit_check = 1;
        server_socket_active = 0;
        if(zmq_server_funcThread.joinable()){zmq_server_funcThread.join();}

        client_socket_active = 0;
        if(zmq_client_funcThread.joinable()){zmq_client_funcThread.join();}
        socket.close();
    }
    catch(zmq::error_t err){std::cerr << err.what() << std::endl;}
    
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(45, 46));
    ImGui::Begin("back button", nullptr, ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowFontScale(1.35f);
        ImGui::PushFont(get_icon_font(io));    
        if(ImGui::Button(u8"\uf060")){current_window = switch;}
        ImGui::PopFont();  
    ImGui::End(); 
    
    ImGui::SetNextWindowPos(ImVec2(display_size_x * 0.25f, display_size_y * 0.25f));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight * 0.5f));

    ImGui::Begin("text window", nullptr, ImGuiWindowFlags_NoTitleBar |
                                         ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoCollapse);

        ImVec2 window_size = ImGui::GetWindowSize();
        float window_width = window_size.x;
        float window_height = window_size.y;
        ImGui::SetWindowFontScale(3.5f);
        ImGui::SetCursorPos(ImVec2(window_width * 0.25f + 30.0f ,window_height * 0.5f));
        ImGui::PushFont(get_main_font(io));
        ImGui::Text("Register");   
        ImGui::PopFont();
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(display_size_x * 0.25f, display_size_y * 0.5f));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight * 0.5f));
    ImGui::Begin("username and password", nullptr, ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowFontScale(1.75f);  
        ImGui::PushFont(get_main_font(io));
        ImGui::SetNextItemWidth(window_width-10);
        ImGui::PushID("##");
        if(ImGui::InputText("##", user_name, IM_ARRAYSIZE(user_name), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            std::string username_str(user_name);
        }
        ImGui::PopID();
        ImGui::SetNextItemWidth(window_width-10);
        if(ImGui::InputText("##", password, IM_ARRAYSIZE(password)))
        {
            std::string password_str(password);
        }
        ImGui::PopFont();
        ImGui::PushFont(get_main_font(io));
        if(ImGui::Button("Register",ImVec2(window_width-10, 40.0f)))
        {

        }
        ImGui::PopFont();
    ImGui::End();
}

void window_name_login()
{
    ImGuiIO& io = ImGui::GetIO();
    float display_size_x = ImGui::GetIO().DisplaySize.x;
    float display_size_y = ImGui::GetIO().DisplaySize.y;
    float halfWidth = display_size_x * 0.5f;
    float halfHeight = display_size_y * 0.5f;
    static char user_name[256] = "";
    static char password[256] = "";
    
    try
    {
        server_exit_check = 1;
        server_socket_active = 0;
        if(zmq_server_funcThread.joinable()){zmq_server_funcThread.join();}

        client_socket_active = 0;
        if(zmq_client_funcThread.joinable()){zmq_client_funcThread.join();}
        socket.close();
    }
    catch(zmq::error_t err){std::cerr << err.what() << std::endl;}
    
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(45, 46));
    ImGui::Begin("back button", nullptr, ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowFontScale(1.35f);
        ImGui::PushFont(get_icon_font(io));    
        if(ImGui::Button(u8"\uf060")){current_window = switch;}
        ImGui::PopFont();  
    ImGui::End(); 
    
    ImGui::SetNextWindowPos(ImVec2(display_size_x * 0.25f, display_size_y * 0.25f));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight * 0.5f));

    ImGui::Begin("text window", nullptr, ImGuiWindowFlags_NoTitleBar |
                                         ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoCollapse);

        ImVec2 window_size = ImGui::GetWindowSize();
        float window_width = window_size.x;
        float window_height = window_size.y;
        ImGui::SetWindowFontScale(3.5f);
        ImGui::SetCursorPos(ImVec2(window_width * 0.25f + 50.0f ,window_height * 0.5f));
        ImGui::PushFont(get_main_font(io));
        ImGui::Text("Login");   
        ImGui::PopFont();
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(display_size_x * 0.25f, display_size_y * 0.5f));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight * 0.5f));
    ImGui::Begin("username and password", nullptr, ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowFontScale(1.75f);  
        ImGui::PushFont(get_main_font(io));
        ImGui::SetNextItemWidth(window_width-10);
        ImGui::PushID("##");
        if(ImGui::InputText("##", user_name, IM_ARRAYSIZE(user_name), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            std::string username_str(user_name);
        }
        ImGui::PopID();
        ImGui::SetNextItemWidth(window_width-10);
        if(ImGui::InputText("##", password, IM_ARRAYSIZE(password)))
        {
            std::string password_str(password);
        }
        ImGui::PopFont();
        ImGui::PushFont(get_main_font(io));
        if(ImGui::Button("Login",ImVec2(window_width-10, 40.0f)))
        {

        }
        ImGui::PopFont();
    ImGui::End();
}

void set_style()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    ImGui::StyleColorsDark();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.ChildRounding = 6.0f;
    style.PopupRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(8, 6);   
    style.FramePadding = ImVec2(10, 6);  // button height
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.IndentSpacing = 20.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f); // midle title
    style.WindowMenuButtonPosition = ImGuiDir_None; 
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;
    style.GrabRounding = 4.0f;  

    colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f); // window background
    colors[ImGuiCol_Button] = ImVec4(0.10f, 0.125f, 0.15f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.35f, 0.40f, 1.00f);    
    colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.45f, 0.50f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.18f, 0.20f, 1.00f);
}
