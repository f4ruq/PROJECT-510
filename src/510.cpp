#include "510.hpp"

std::mutex globalMutex;
std::atomic<bool> server_exit_check{0};
std::atomic<bool> client_exit_check{0};
std::atomic<bool> server_socket_active{0};
std::atomic<bool> client_socket_active{0};
std::string client_id_str;
std::vector<std::string> message_log;
std::string response = sentinel_code;
std::string* response_ptr = &response;
std::thread zmq_client_funcThread;
std::thread zmq_server_funcThread;
zmq::context_t context(1);
zmq::socket_t socket(context, zmq::socket_type::dealer);
zmq::message_t identity;
zmq::message_t request;
std::string* received_message_ptr;
bool running = true;
int current_window = 0;
int my_image_width = 0;
int my_image_height = 0;
GLuint my_image_texture = 0;

SDL_Window* window = SDL_CreateWindow("P-510 CLIENT APP",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

void zmq_client_func(zmq::socket_t& socket_, zmq::context_t& context, std::string*& response_ptr)
{
    while(client_socket_active)
    {
        if(client_exit_check)
        {
            std::string exit_message = "client left the chat.";
            zmq::message_t request(exit_message.size());
            memcpy(request.data(), exit_message.data(), exit_message.size());
            socket_.send(request, zmq::send_flags::none);
                //socket_.close();
                
                /*context.shutdown();
                context.close();
                */
            break;
        }

        // poll for incoming server messages
        zmq::pollitem_t items[] = { { static_cast<void*>(socket_), 0, ZMQ_POLLIN, 0 } };
        zmq::poll(&items[0], 1, std::chrono::milliseconds(100));
        if(client_exit_check){break;}

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

void zmq_server_func(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_, std::string*& response_ptr, 
    zmq::context_t& context, std::string*& received_message_ptr) 
{
    while (server_socket_active) 
    {
        if(server_exit_check)
        {    
            std::string exit_message = "server left the chat.";
            zmq::message_t id_msg(client_id_str.begin(), client_id_str.end());
            zmq::message_t reply(exit_message.size());
            memcpy(reply.data(), exit_message.data(), exit_message.size());
            socket_.send(id_msg, zmq::send_flags::sndmore);
            socket_.send(reply, zmq::send_flags::none);
            //socket_.close();
            /*
            context.shutdown();
            context.close();
            */
           break;
        }
        
        // poll the socket for new incoming data with a 100ms timeout
        zmq::pollitem_t items[] = {{static_cast<void*>(socket_), 0, ZMQ_POLLIN, 0}};
        zmq::poll(items, 1, 100);
        if(server_exit_check){break;}
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

void window_name_switch()
{
    client_exit_check = 1;
    client_socket_active = 1;
    static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = false;
    icons_config.PixelSnapH = true;
    icons_config.OversampleH = icons_config.OversampleV = 1;    
    ImGuiIO& io = ImGui::GetIO();
    ImFont* icons = io.Fonts->AddFontFromFileTTF("assets/Font Awesome 7 Free-Solid-900.otf", 20.0f, &icons_config, icons_ranges);
    
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
        ImGui::SetWindowFontScale(2.5f);
        ImGui::SetCursorPos(ImVec2(window_width * 0.25f + 20, window_height * 0.7f));
        //ImGui::Text("Choose an Application");
        ImGui::Text("Connect");
        ImGui::SetCursorPos(ImVec2(window_width * 0.50f + 60, window_height * 0.7));
        ImGui::Text("Host");    
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0, display_size_y * 0.40f));
    ImGui::SetNextWindowSize(ImVec2(display_size_x, display_size_y * 0.75f));

    ImGui::Begin("switch", nullptr, ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowFontScale(2.0f);
        ImGui::SetCursorPos(ImVec2(window_width * 0.5f + 5, 0));
        ImGui::PushFont(icons);
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
        ImGui::PushFont(icons);
        if (ImGui::Button(u8"\uf075", ImVec2(window_width * 0.25f - 5, 175)))//client button
        {
            current_window = enter_adress;
        }
        ImGui::PopFont();
    ImGui::End();
}

void window_name_adress()
{
    static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = false;
    icons_config.PixelSnapH = true;
    icons_config.OversampleH = icons_config.OversampleV = 1;    
    ImGuiIO& io = ImGui::GetIO();
    ImFont* icons = io.Fonts->AddFontFromFileTTF("assets/Font Awesome 7 Free-Solid-900.otf", 20.0f, &icons_config, icons_ranges);
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
        ImGui::PushFont(icons);    
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
        ImGui::SetWindowFontScale(2.5f);
        ImGui::SetCursorPos(ImVec2(window_width * 0.25f - 10, window_height * 0.5f));
        ImGui::Text("Enter Adress");   
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(display_size_x * 0.25f, display_size_y * 0.5f));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight * 0.5f));
    ImGui::Begin("ADRESS", nullptr, ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowFontScale(1.25f);  
        ImGui::SetNextItemWidth(window_width - 65);
    
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
        ImGui::SameLine();
        ImGui::PushFont(icons);
        if(ImGui::Button(u8"\uf00c"))
        {
            
        }
        ImGui::PopFont();
    ImGui::End();
}

void window_name_client()
{
    static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = false;
    icons_config.PixelSnapH = true;
    icons_config.OversampleH = icons_config.OversampleV = 1;    
    ImGuiIO& io = ImGui::GetIO();
    ImFont* icons = io.Fonts->AddFontFromFileTTF("assets/Font Awesome 7 Free-Solid-900.otf", 20.0f, &icons_config, icons_ranges);
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
        ImGui::SetNextItemWidth(display_size_x - 65);
        if(ImGui::InputText("##", user_input, IM_ARRAYSIZE(user_input), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            send = 1;
            ImGui::SetKeyboardFocusHere(-1);
        }
        
        ImGui::SameLine();
        ImGui::PushFont(icons);
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
        ImGui::PushFont(icons);
        if(ImGui::Button(u8"\uf060")){current_window = enter_adress;}
        ImGui::PopFont();
        for (int i = 0; i < message_log.size(); ++i)
        {
            ImGui::Text("%s", message_log[i].c_str());
            ImGui::SetScrollHereY(1.0f);
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
    static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = false;
    icons_config.PixelSnapH = true;
    icons_config.OversampleH = icons_config.OversampleV = 1;    
    ImGuiIO& io = ImGui::GetIO();
    ImFont* icons = io.Fonts->AddFontFromFileTTF("assets/Font Awesome 7 Free-Solid-900.otf", 20.0f, &icons_config, icons_ranges);
    
    ImGui::SetNextWindowPos(ImVec2(0, bottomHeight));
            ImGui::SetNextWindowSize(ImVec2(display_size_x, bottomHeight));
            ImGui::Begin("...",nullptr, ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse);
                ImGui::SetNextItemWidth(display_size_x - 65);
                if(ImGui::InputText("##", user_input, IM_ARRAYSIZE(user_input), ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    send = 1;
                    ImGui::SetKeyboardFocusHere(-1);
                }
                
                ImGui::SameLine();
                ImGui::PushFont(icons);
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
                
                ImGui::PushFont(icons);
                if(ImGui::Button(u8"\uf060")){current_window = switch;}
                ImGui::PopFont();

                for (int i = 0; i < message_log.size(); ++i)
                {
                    ImGui::Text("%s", message_log[i].c_str());
                    
                }
                
                ImGui::SetScrollHereY(1.0f);
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
