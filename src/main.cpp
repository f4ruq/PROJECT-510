#include "510.hpp"
#include "user_ops.hpp"
std::mutex globalMutex;
std::atomic<bool> server_exit_check{0};
std::atomic<bool> client_exit_check{0};
std::atomic<bool> server_socket_active{0};
std::atomic<bool> client_socket_active{0};
std::atomic<bool> login_successful{0};
std::atomic<int> current_user_index{-1};
std::atomic<bool> scroll{0};
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
std::atomic<bool> running{1};
int current_window = 0;
int my_image_width = 0;
int my_image_height = 0;
GLuint my_image_texture = 0;

int main()
{
    socket.setsockopt(ZMQ_LINGER, 0);
    sdl_init();

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    // imgui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext(); 
    ImGuiIO& io = ImGui::GetIO();
    icons = io.Fonts->AddFontFromFileTTF("/Users/xubustein/Desktop/workspaces/xubusteins_masterpiece/assets/Font-Awesome-7-Free-Solid-900.otf", 13.0f);
    main_font = io.Fonts->AddFontFromFileTTF("/Users/xubustein/Desktop/workspaces/xubusteins_masterpiece/assets/Roboto_Condensed-Regular.ttf", 20.0f);

    ImFontConfig cfg;
    cfg.SizePixels = 100.0f;

    set_style();
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
        main_ui_func(gl_context);
        // rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData()); 
        SDL_GL_SwapWindow(window);
    }
    
    //cleanup
    clean_memory();
    server_socket_active = 0;
    client_socket_active = 0;
    client_exit_check = 1;
    server_exit_check = 1;
    socket.close();
    context.shutdown();
    context.close();
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
