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
            zmq::pollitem_t items[] = {
                { static_cast<void*>(socket_), 0, ZMQ_POLLIN, 0 }
            };

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
    ImGui::SetCursorPos(ImVec2(window_width * 0.25f + 20, window_height * 0.5f));
    ImGui::Text("Choose an Application");
    
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0, display_size_y * 0.40f));
    ImGui::SetNextWindowSize(ImVec2(display_size_x, display_size_y * 0.75f));

    ImGui::Begin("switch", nullptr, ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse);


    ImGui::SetCursorPos(ImVec2(window_width * 0.5f + 5, 0));

    if (ImGui::Button("Server", ImVec2(window_width * 0.25f, 175)))
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
                std::ref(request), std::ref(identity), std::ref(socket),
                std::ref(response_ptr), std::ref(context), std::ref(received_message_ptr));
            current_window = server;
        }
        catch (zmq::error_t err)
        {
            std::cerr << err.what() << std::endl;
        }
    }

    ImGui::SetCursorPos(ImVec2(window_width * 0.25f, 0));
    if (ImGui::Button("Client", ImVec2(window_width * 0.25f - 5, 175)))
    {
        current_window = enter_adress;
    }

    ImGui::End();
}

void window_name_adress()
{
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

                client_exit_check = 1;
                client_socket_active = 0;
                if(zmq_client_funcThread.joinable()){zmq_client_funcThread.join();}
                socket.close();
            }
            catch(zmq::error_t err){std::cerr << err.what() << std::endl;}
            
            ImGui::SetNextWindowPos(ImVec2(display_size_x * 0.25f, display_size_y * 0.25f));
            ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight));
            ImGui::Begin("ADRESS", nullptr, ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse);
            if(ImGui::Button("<")){current_window = switch;}
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

void window_name_client()
{
    float display_size_x = ImGui::GetIO().DisplaySize.x;
    float display_size_y = ImGui::GetIO().DisplaySize.y;
    float halfWidth = display_size_x * 0.5f;
    float bottomHeight = display_size_y * 0.8f;
    char user_input[256] = "";
    
    ImGui::SetNextWindowPos(ImVec2(0, bottomHeight));
            ImGui::SetNextWindowSize(ImVec2(display_size_x, bottomHeight));
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
            ImGui::SetNextWindowSize(ImVec2(display_size_x, bottomHeight));
            

            ImGui::Begin("MESSAGES", nullptr, ImGuiWindowFlags_NoTitleBar);
        
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
    float bottomHeight = display_size_y * 0.8f;
    char user_input[256] = "";
    static std::string user_input_stc;
    ImGui::SetNextWindowPos(ImVec2(0, bottomHeight));
            ImGui::SetNextWindowSize(ImVec2(display_size_x, bottomHeight));
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
            ImGui::SetNextWindowSize(ImVec2(display_size_x, bottomHeight));
            
            ImGui::Begin("MESSAGES", nullptr, ImGuiWindowFlags_NoTitleBar);
                
                for (int i = 0; i < message_log.size(); ++i)
                {
                    ImGui::Text("%s", message_log[i].c_str());
                    
                }
                
                ImGui::SetScrollHereY(1.0f);
            ImGui::End();
}
